# Friend input: custom columns next to the central NanoAOD

[Documentation index](README.md)

A private NanoAOD production (extra tables, extended collections) used to be
consumed as a full copy of NanoAOD plus the custom tables, and every analysis
module kept its own sample jsons and hand-copied normalisation for that copy.
A *friend* keeps the central NanoAODv15 RNTuple as the input and stores only
the custom columns in a second file per central file, aligned entry for entry.
Analyzers read friend columns exactly as central ones: generated branches,
typed handles and module row views cannot tell the two files apart.

## File contract (format 1)

One friend file per central file, at

    $SKNANO_FRIEND_ROOT/<production>/<path of the central file below $SKNANO_INPUT_ROOT>

(a central file outside the input root keeps its absolute path minus the
leading slash). The pairing is by path, never by position.

`Events` has the central file's entry count and order and holds

- `run`, `luminosityBlock`, `event` copied from the central file. The loader
  compares them with the central values on every entry and aborts the job on
  the first mismatch. They are never served to analyzers from the friend.
- `SKFriend_status` (uint8): `0` present; `1` the producer processed the lumi
  but the event is absent (its event filter rejected it); `2` the lumi was
  never processed. Status 2 entries never reach `executeEvent()`; status 1
  entries do, with every friend column empty. `SKNanoLoader::FriendStatus()`
  returns the current event's status.
- every top-level field of the production that the central file does not
  have, plus the complete collections listed as overrides. Cardinality fields
  (`nX`) are stored as plain `std::uint32_t`.

`SKFriendInfo` (one entry) records `formatVersion`, `production`,
`centralFile`, `centralEntries`, `overrides`, `fields`, `sourceFiles`,
`nStatus`, `missingRuns`/`missingLumis`, `missingSumW`, `missingSumSign`,
`missingCount` and `filterNote`. `RNTupleSource::addFriend` reads it to decide
which central collections the friend replaces and refuses a file built for a
different entry count or format version.

Rules enforced on attach: a friend field that also exists in the central file
is served from the friend only when its collection (`X` for `X_y` and `nX`) is
in `overrides`, otherwise the job fails with a configuration error; two
friends may not provide the same field.

## Declaring a production: `<module>/friends.yml`

The file lives at the module root or one directory below it, where the
sample jsons live (`Hadron_Analyzers/Hadron/friends.yml`).

```yaml
hadron_av6:
  analyzers: [HadronAnalyzer]     # analyzers that attach it (required)
  eras: ["2024"]                  # optional, every era when omitted
  overrides: [GenPart]            # central collections replaced by the friend
  exclude_prefixes: [LambdaCToPKPiLoose, D0ToKPiLoose]
  filter_note: "HLT_IsoMu24 || HLT_Ele30_WPTight_Gsf prefilter on data"
  sources:                        # builder only; {era} and {PD} are substituted
    - /gv0/DATA/SKNano/CustomNanoAODv15/hadron_av6_rntuple640/{era}/{PD}/**/*.root
  samples: [TTLJ_powheg, Muon0]   # optional allowlist of registry names
  on_missing: error               # or run_without when a sample has no friend
```

The submission (`SKNano.py`) and the builder read the same file, so a
production is described once. `SKNANO_FRIEND_ROOT` is set in `setup.sh` and
can be overridden in `config/config.$USER`; a production may pin its own
`root`.

## Building

`sknano_friend` (C++, `tools/sknano_friend/`) builds one friend:

```bash
sknano_friend build --central tree_1.root --aux NANO_1.root --aux NANO_2.root \
    --out $SKNANO_FRIEND_ROOT/hadron_av6/2024/.../tree_1.root --production hadron_av6 \
    --override GenPart --exclude-prefix D0ToKPiLoose --mc --report tree_1.root.build.json
sknano_friend check <friend> <central>
```

It reads the central event keys, indexes the aux files' `LuminosityBlocks`
and `Events` keys (a duplicate key is fatal), assigns the status of every
central entry, and copies the selected fields. The aux files are read in their
own order and staged in chunks sorted by central entry on local scratch, then
merged in central order, so all I/O is sequential: a multi-threaded `cmsRun`
scatters events, and reading the aux files in central order would load a
cluster per entry. For every extension column (a selected `X_y` whose
collection `X` the central file also has) the builder compares `nX` between
the two files on every matched event and refuses to build on a mismatch.

The driver runs it over a sample the way jobs resolve the sample:

```bash
scripts/sknano_friend.py build    -p hadron_av6 -e 2024 -i TTLJ_powheg [--condor]
scripts/sknano_friend.py build    -p hadron_av6 -e 2024 -i Muon0 --period C --condor
scripts/sknano_friend.py validate -p hadron_av6 -e 2024 -i TTLJ_powheg
scripts/sknano_friend.py status   -p hadron_av6 -e 2024 -i TTLJ_powheg
```

`build` caches a lumi → aux file index per (production, era, PD) under
`<production>/_index/`, picks the aux files covering each central file, and
runs the builder locally or as one condor job per central file
(`condor_submit` must be on `PATH`; on ai-tamsa1 use the ssh shim the Hadron
module ships). Existing friends are skipped unless `--force`. `validate`
requires every central file to have its friend and its `.build.json`, and
writes `<production>/<era>/<sample>/friend_summary.json` with the status
totals, the never-processed lumis and the missing `sumW`/`sumsign`. For data
with missing lumis it also writes `missing_lumis.json` in the run → lumi-range
form `brilcalc -i` and the golden json use.

## What a submission does

When `-a <Analyzer>` matches a production's `analyzers`, `SKNano.py` pairs a
friend with every central file (`module.AddFile(central, {friend})` in the
generated job), fails the submission when a file or the sample summary is
missing (`on_missing: error`), and

- MC: subtracts the summary's `missing_sumw`/`missing_sumsign` from the
  registry `sumW`/`sumsign` before writing them into the job;
- data: prints how many lumis were never processed; those events are skipped
  in the loop, and the recorded luminosity of `missing_lumis.json` has to be
  subtracted from the trigger luminosity by hand (brilcalc).

The resolved friend paths go into
`metadata/data_snapshot/resolved_inputs/<era>/<sample>.json`, the production,
status totals and the before/after normalisation into
`metadata/data_snapshot/friends/<era>/<sample>.json`, both listed in the
`run_manifest.json`. `--no-friends` runs the analyzer on the central files
alone. The performance report counts `friend_missing_events` and
`friend_filtered_events`.

## Limitations and follow-ups

- Skims (`SnapshotSelectedInput`) copy the central columns only; a skim of a
  friend-attached sample needs its own friend build against the skim files.
- A friend lumi is either fully processed or absent: a lumi that a CRAB job
  processed partially cannot be told from a filtered one. Recording the
  pre-filter event count per lumi in the producer would close that gap.
- Dropping the standard tables in the producer itself would shrink the
  private production on disk; the builder only needs the custom fields.
