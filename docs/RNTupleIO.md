# RNTuple I/O

[Documentation index](README.md)

SKNanoAnalyzer uses ROOT 6.40 RNTuple input exclusively. `SKNanoLoader` opens
the `Events` RNTuple in each input file, and the generated `BranchScalar`,
`BranchVector`, collection views, and lazy activation APIs bind directly to
RNTuple fields.

The framework code generator consumes only the canonical NanoAOD schema.
External modules bind custom scalar and vector fields through typed runtime
handles and provide their own event-scoped row views. Custom fields of a
private production can live in a friend file attached next to each central
file instead of a full private copy; see [Friend Input](FriendInput.md).

## Run an analyzer

```bash
SKNano.py -a ExampleRun -i 'MyRNTupleSample*' -e 2024
```

Generated jobs call `GetInputEntries()`, so reduction works across multi-file
RNTuple inputs. A direct C++ setup is equivalent:

```cpp
ExampleRun module;
module.SetRNTupleName("Events");
module.AddFile("input.rntuple.root");
```

All files in one job must use a compatible RNTuple schema. Multi-file jobs
switch readers at file boundaries while preserving lazy field state.

## I/O behavior

- Scalar and vector views are created lazily on first access.
- `RNTupleSource` snapshots the top-level field names and types when it opens a
  file. `hasField()`/`fieldType()` answer from that snapshot, because
  `RNTupleReader::GetDescriptor()` clones the descriptor under a lock and
  `FindFieldId()` scans every top-level field linearly -- roughly 20 us per call
  on a NanoAOD-sized schema, which is not viable on a per-event path.
- Typed `ColumnHandle`/`ScalarHandle` cache the column they resolve to. The
  cache is keyed on `BranchManager::generation()`, which is bumped whenever an
  RNTuple is attached or the manager is cleared, so a new file re-resolves every
  handle and optional fields stay honest when availability differs per file.
- Arithmetic `RVec<T>` fields expose page-backed contiguous views without an
  intermediate analyzer buffer copy.
- `RVec<bool>` is materialized as bytes because it has no stable `bool *`
  representation compatible with the existing view contract.
- ROOT's RNTuple cluster cache is enabled. It uses one I/O thread that issues
  the page reads of the next cluster one request at a time, which on a
  high-latency network mount (the GlusterFS `/gv0`) caps a job at a few MB/s.
- Input read-ahead: `RNTupleSource` starts a page-cache warmer thread per
  input file (`InputPrefetcher`, `AnalyzerFramework/src/RNTupleSource.cc`).
  From a descriptor snapshot taken at open it knows every cluster's page
  ranges per top-level field; as the entry cursor crosses a cluster boundary
  (`SKNanoLoader::PrepareEntry` -> `RNTupleSource::noteEntry`) it reads the
  pages of the next `N` clusters with 4 MiB sequential `pread()` calls on a
  private descriptor, so the cluster pool's page reads hit the kernel page
  cache. It follows the analyzer's lazy field activation: the loader hands the
  activated top-level fields over at each boundary and only their columns are
  read ahead (the whole cluster span while none is active yet, i.e. the first
  cluster of the first file). Active pages up to 1 MiB apart are read as one
  range. A physical column belongs to every top-level field that reads it:
  CustomNanoAOD RNTuples store each collection once (`_collectionN`) and expose
  the flat names (`Jet_pt`) as projected fields through alias columns, 1033 of
  the 2398 top-level fields of the TTLJ smoke file. It never hands data to
  ROOT, so it cannot change a result. Default window `N = 2` clusters; `SKNANO_INPUT_PREFETCH=0` turns it
  off, `N > 1` sets the window. Remote (`root://`, `http://`) inputs are left
  alone. Measured on one CustomBPH data file at idle storage: ROOT's own
  read-wait time went from 7.9 s to 1.4 s (data) and 4.2 s to 0.5 s (MC, 60k
  events); under a loaded mount the same read of 262 columns took 138 s cold
  against 38 s page-cache-warm.
- Until 2026-11-04 the page table gave a column only to the first field that
  lists it, so an activated projection had no pages. The read-ahead then
  covered 59 MB of the 2.11 GB of active pages per TTLJ file, for every flag
  set, and ROOT read the rest itself. On the same TTLJ files (table below),
  fixing it moved the default read-ahead from 0.755 GB / 25 ranges to
  2.72-2.73 GB / 118 (production flags) and 1.84 GB / 137 (V0Only). ROOT's
  read wait fell from 5.9 to 2.0 s and from 5.7 to 0.7 s. The bytes from
  `/gv0` rose by 5 MB and 30 MB per file: the 1 MiB merging streams inactive
  pages that ROOT does not read.
- `SKNANO_INPUT_PREFETCH_SKIP_INACTIVE=1` (off by default) makes the read-ahead
  read no page of an inactive column:
  - A range is a run of active pages in file order, and an inactive page ends
    it. The default merges active pages up to 1 MiB apart and streams the
    inactive pages between them.
  - When nothing is active at open (the first file of a job), the read-ahead
    waits until no field has been activated for 100 entries. The default reads
    the whole span of the first `N + 1` clusters instead. The loader opens the
    first file twice (`Init`, then the loop), so this happens twice: 703 MB on
    the TTLJ smoke file, the second time from the page cache.
  - ROOT's cluster reads still coalesce across gaps
    (`rntuple_read_overhead_bytes`, about 21 % of the payload on TTLJ). Those
    inactive bytes are read in both modes. With the cluster cache also off
    (`SKNanoLoader::rntupleClusterCache = false`; no environment switch), ROOT
    reads single pages from the page cache and `/gv0` serves 2.202 GB
    (production) and 1.210 GB (V0Only) in one run each.
  - TTLJ files (274k events, 2.83 GB), one file per job, 2-3 runs per row on
    different files. The file's page cache is dropped before each run, and the
    bytes from `/gv0` are its page-cache residency after the run:

    | flags | mode | bytes from `/gv0` | read-ahead bytes / ranges | ROOT read wait |
    |---|---|---|---|---|
    | production (417 fields) | default | 2.649 GB | 2.72–2.73 GB / 118 | 1.5–2.5 s |
    | production | skip | 2.577 GB | 2.10–2.11 GB / 1226–1229 | 3.3–4.2 s |
    | V0Only (343 fields) | default | 1.577 GB | 1.84 GB / 137 | 0.6–0.7 s |
    | V0Only | skip | 1.363 GB | 1.108 GB / 1203–1207 | 1.7–4.6 s |

    Skip saves 72 MB and 215 MB per one-file job on the same file. Most of it
    is the inactive pages of the first three clusters, which are read once per
    job. The 1 MiB merging adds about 5 MB and 30 MB per file. Skip issues ten
    times more ranges and its ROOT read wait was longer here, likely because
    ROOT reads the start of the first file cold until the hold ends. Analyzer
    outputs are bit-identical in the two modes.
- Analyzer jobs do not inherit the submitting shell's environment, so
  `SKNano.py` writes every `SKNANO_INPUT_PREFETCH*` variable that is set at
  submission into the job's `run.sh`.
- Analyzer datasets booked through `Output().Book()` are written as RNTuples.
  Histograms remain ordinary ROOT histogram objects in the same file.
- Analyzer RNTuple output uses LZ4 level 1 (levels above 1 switch ROOT to
  LZ4HC, ~2 % of the event loop for little size gain), 64 MiB target clusters,
  buffered writes, page checksums, and bounded compression parallelism from
  `--ncpu`. The sparse multi-dataset profile uses a 4 MiB buffer budget per
  dataset.
- `RNTupleHandle::SetHalfPrecision(field)` stores every float/double leaf of
  a booked field (scalars, vectors, arrays, nested) as IEEE half precision on
  disk (RNTuple `kReal16`); the in-memory type stays `float` and readers
  (ROOT, uproot >= 5.6) get floats back. Use it for bulk model-input arrays
  whose consumers tolerate ~1e-3 relative precision.
- Batch merging goes through `scripts/sknano_merge.py`. It checks every shard's
  RNTuple and histogram schemas, explicitly stages `hadd -fk404` in batches of
  at most 100 inputs, and fully deserializes every intermediate file before it
  can feed the next stage. It then verifies merged dataset entries, histogram
  entries, and compression, atomically publishes the target, and deletes
  shards only after all checks pass. Staged partials are placed under the
  output filesystem (or `--temp-dir`) so a large merge cannot silently exhaust
  the node-local `/tmp` filesystem.
- Merging is NFS-latency bound rather than CPU bound, so the batches of a stage
  run as independent `hadd` processes over disjoint inputs. `hadd -n` and
  `hadd -j` are never used: both corrupt output on ROOT 6.40.02, and every
  merge output is screened for the resulting repeated key cycles.
- Skimming records selected global entries during analysis and writes the
  original input schema through the RNTuple Snapshot backend.

## Performance report

Every job that has an output path writes `<output>.performance.json` with
wall/CPU time, phase timers and counters. Phases: `event_total`, `header_io`
(entry bookkeeping only; there is no per-event header read), `object_view`,
`correction`, `rntuple_fill` (the output `Fill`, including the cluster flush
and write whenever a Fill completes a cluster), plus analyzer phases. Phase
names are looked up by `std::string_view`, so a literal at a per-candidate
call site allocates nothing.

Input byte counters come from the page source and the read-ahead thread, not
from `TFile` (RNTuple reads bypass it): `prefetch_bytes`, `prefetch_ranges`,
`prefetch_clusters`, `prefetch_window_clusters` are always filled;
`rntuple_read_payload_bytes`, `rntuple_read_overhead_bytes`,
`rntuple_read_calls`, `rntuple_readv_calls`, `rntuple_unzip_bytes`,
`rntuple_read_wall_seconds`, `rntuple_unzip_wall_seconds` are ROOT's own
metrics and are only populated when `SKNANO_PERFORMANCE_REPORT=/path.json` is
set, because those metrics wrap every page read in an `RNTupleTimer` (two
`clock()` calls per read). Jobs that only get a report path from
`SetOutfilePath()` leave them at zero and do not pay for the timers. The
standalone `test_rntuple_source` target validates scalar and vector access
against an RNTuple file.
