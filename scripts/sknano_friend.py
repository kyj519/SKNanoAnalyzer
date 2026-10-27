#!/usr/bin/env python3
"""Build and validate friend RNTuples for a production declared in a module's
friends.yml (docs/FriendInput.md).

    sknano_friend.py build    -p hadron_av6 -e 2024 -i TTLJ_powheg[,Muon0] [--period C] [--condor]
    sknano_friend.py validate -p hadron_av6 -e 2024 -i TTLJ_powheg [--period C]
    sknano_friend.py status   -p hadron_av6 -e 2024 -i TTLJ_powheg [--period C]

`build` resolves the sample's central files exactly like SKNano.py does, finds
the private-production files covering each one through a cached lumi index,
and runs `sknano_friend build` per central file (locally, or one condor job
each with --condor).  `validate` checks every central file has its friend and
writes the per-sample friend_summary.json the submission reads for its
normalisation correction.
"""

from __future__ import annotations

import argparse
import datetime
import glob
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

SKNANO_HOME = os.environ.get("SKNANO_HOME") or str(Path(__file__).resolve().parents[1])
sys.path.insert(0, SKNANO_HOME)

from python import friend_config as fc  # noqa: E402
from python.sample_paths import resolve_sample_paths  # noqa: E402

SKNANO_DATA = os.environ.get("SKNANO_DATA", "")
BINARY = os.path.join(os.environ.get("SKNANO_INSTALLDIR", ""), "bin", "sknano_friend")
INDEX_DIRNAME = "_index"
JOBS_DIRNAME = "_jobs"


def module_dirs() -> list[str]:
    raw = os.environ.get("SKNANO_ANALYSIS_MODULE_DIRS", "")
    dirs = []
    for entry in raw.replace(";", os.pathsep).split(os.pathsep):
        if entry and os.path.isdir(entry):
            dirs.append(os.path.realpath(os.path.expanduser(entry)))
    return dirs


def sample_registry(era: str) -> dict:
    """CommonSampleInfo plus the module sample jsons, as SKNano.py merges them."""
    path = os.path.join(SKNANO_DATA, era, "Sample", "CommonSampleInfo.json")
    with open(path, encoding="utf-8") as handle:
        registry = json.load(handle)
    for module_dir in module_dirs():
        for pattern in (os.path.join(module_dir, "data", "Sample", era, "*.json"),
                        os.path.join(module_dir, "*", "data", "Sample", era, "*.json")):
            for sample_path in sorted(glob.glob(pattern)):
                with open(sample_path, encoding="utf-8") as handle:
                    payload = json.load(handle)
                registry[payload.get("name", Path(sample_path).stem)] = payload
    return registry


def production_by_name(name: str) -> fc.FriendProduction:
    for production in fc.load_friends(module_dirs()):
        if production.name == name:
            return production
    raise SystemExit(f"no friends.yml in {module_dirs()} declares production '{name}'")


def split_sample(sample: str, registry: dict) -> tuple[str, str | None]:
    """'Muon0_C' -> ('Muon0', 'C') for data; MC samples carry no period."""
    if sample in registry and registry[sample].get("isMC"):
        return sample, None
    match = re.match(r"^(.*)_([A-Z](?:_v\d+)?)$", sample)
    if match and match.group(1) in registry:
        return match.group(1), match.group(2)
    if sample in registry:
        return sample, None
    raise SystemExit(f"sample '{sample}' is not in the {registry_era} registry")


def lumi_table(path: str) -> set[tuple[int, int]]:
    # uproot rather than PyROOT: an RNTupleView outliving its reader at
    # function exit segfaults, and this runs over thousands of files.
    import uproot  # noqa: PLC0415  (slow import, only when needed)
    table = uproot.open(path)["LuminosityBlocks"].arrays(
        ["run", "luminosityBlock"], library="np")
    return set(zip(table["run"].tolist(), table["luminosityBlock"].tolist()))


def aux_index(production: fc.FriendProduction, era: str, pd: str) -> tuple[list[str], dict]:
    """lumi -> aux file, cached per (production, era, PD) until the pool changes."""
    files = production.source_files(era, pd)
    if not files:
        raise SystemExit(f"no aux files for {era}/{pd} under {production.source_globs(era, pd)}")
    path = os.path.join(production.production_dir, INDEX_DIRNAME, era, f"{pd}.json")
    if os.path.isfile(path):
        with open(path, encoding="utf-8") as handle:
            cached = json.load(handle)
        if cached.get("files") == files:
            return files, {tuple(map(int, k.split(":"))): v for k, v in cached["lumis"].items()}
    print(f"indexing {len(files)} aux files for {era}/{pd} ...", flush=True)
    lumis: dict[tuple[int, int], int] = {}
    for index, file in enumerate(files):
        for key in lumi_table(file):
            if key in lumis and lumis[key] != index:
                raise SystemExit(f"lumi {key} is in both {files[lumis[key]]} and {file}; "
                                 "a lumi must live in exactly one aux file")
            lumis[key] = index
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path + ".tmp", "w", encoding="utf-8") as handle:
        json.dump({"files": files, "lumis": {f"{r}:{l}": v for (r, l), v in lumis.items()}}, handle)
    os.replace(path + ".tmp", path)
    return files, lumis


def build_command(production, central, aux, out, is_mc, args) -> list[str]:
    command = [BINARY, "build", "--central", central, "--out", out,
               "--production", production.name, "--report", out + ".build.json",
               "--scratch", args.scratch, "--chunk", str(args.chunk)]
    for file in aux:
        command += ["--aux", file]
    for collection in production.overrides:
        command += ["--override", collection]
    for collection in production.exclude_prefixes:
        command += ["--exclude-prefix", collection]
    if is_mc:
        command.append("--mc")
    if production.filter_note:
        command += ["--filter-note", production.filter_note]
    return command


def plan_sample(production, era, sample, registry, args):
    name, period = split_sample(sample, registry)
    spec = dict(registry[name], name=name)
    is_mc = bool(spec.get("isMC"))
    central_files = resolve_sample_paths(spec, era, period)
    pd = spec.get("PD") or name
    aux_files, index = aux_index(production, era, pd)
    jobs, done = [], []
    for central in central_files:
        out = production.friend_path(central)
        if os.path.isfile(out) and not args.force:
            done.append(out)
            continue
        aux = sorted({aux_files[index[key]] for key in lumi_table(central) if key in index})
        if not aux:
            print(f"\033[93mWarning: no aux file covers any lumi of {central}; "
                  "every event will carry status 2\033[0m")
            aux = [aux_files[0]]  # still build: the friend then records the missing lumis
        jobs.append(build_command(production, central, aux, out, is_mc, args))
    return central_files, jobs, done, is_mc


def run_condor(production, era, sample, jobs):
    stamp = datetime.datetime.now().strftime("%Y_%m_%d_%H%M%S")
    job_dir = os.path.join(production.production_dir, JOBS_DIRNAME, era, sample, stamp)
    os.makedirs(os.path.join(job_dir, "logs"))
    # condor's "new" argument syntax: the whole list is one double-quoted
    # string (in the .sub), arguments with spaces are single-quoted inside it.
    def condor_arg(part: str) -> str:
        if not re.search(r"[\s'\"]", part):
            return part
        return "'" + part.replace("'", "''").replace('"', '""') + "'"
    with open(os.path.join(job_dir, "jobs.txt"), "w", encoding="utf-8") as handle:
        for command in jobs:
            handle.write(" ".join(condor_arg(part) for part in command[1:]) + "\n")
    runner = os.path.join(job_dir, "run.sh")
    with open(runner, "w", encoding="utf-8") as handle:
        handle.write(f"""#!/bin/bash
source "{SKNANO_HOME}/setup.sh" > /dev/null 2>&1
exec "{BINARY}" "$@"
""")
    os.chmod(runner, 0o755)
    submit = os.path.join(job_dir, "submit.sub")
    with open(submit, "w", encoding="utf-8") as handle:
        handle.write(f"""executable = {runner}
arguments = "$(args)"
getenv = True
request_memory = {args_memory_mb} MB
request_cpus = 1
log = {job_dir}/logs/job.$(Process).log
output = {job_dir}/logs/job.$(Process).out
error = {job_dir}/logs/job.$(Process).err
batch_name = sknano_friend_{production.name}_{sample}
queue args from {job_dir}/jobs.txt
""")
    condor_submit = shutil.which("condor_submit")
    if not condor_submit:
        print(f"condor_submit is not on PATH; submit on the schedd host with:\n  condor_submit {submit}")
        return
    subprocess.run([condor_submit, submit], check=True)
    print(f"submitted {len(jobs)} jobs, logs in {job_dir}/logs")


def cmd_build(args):
    production = production_by_name(args.production)
    registry = sample_registry(args.era)
    for sample in samples_from(args):
        central_files, jobs, done, _ = plan_sample(production, args.era, sample, registry, args)
        print(f"{args.era}/{sample}: {len(central_files)} central files, "
              f"{len(done)} friends present, {len(jobs)} to build")
        if not jobs:
            continue
        if args.dry_run:
            print("  " + " ".join(jobs[0]))
            continue
        if args.condor:
            run_condor(production, args.era, sample, jobs)
            continue
        for i, command in enumerate(jobs, 1):
            print(f"  [{i}/{len(jobs)}] {command[3]}", flush=True)
            subprocess.run(command, check=True, stdout=subprocess.DEVNULL)


def brilcalc_mask(missing: list[tuple[int, int]]) -> dict:
    """Run -> [[first, last], ...] as brilcalc -i / the golden json expect."""
    mask: dict[str, list[list[int]]] = {}
    for run, lumi in sorted(missing):
        ranges = mask.setdefault(str(run), [])
        if ranges and ranges[-1][1] == lumi - 1:
            ranges[-1][1] = lumi
        else:
            ranges.append([lumi, lumi])
    return mask


def cmd_validate(args):
    production = production_by_name(args.production)
    registry = sample_registry(args.era)
    failed = False
    for sample in samples_from(args):
        name, period = split_sample(sample, registry)
        spec = dict(registry[name], name=name)
        is_mc = bool(spec.get("isMC"))
        central_files = resolve_sample_paths(spec, args.era, period)
        files, missing_files, missing = {}, [], set()
        totals = {"n_status": [0, 0, 0], "missing_sumw": 0., "missing_sumsign": 0.,
                  "missing_count": 0, "central_entries": 0, "aux_only": 0}
        for central in central_files:
            out = production.friend_path(central)
            report_path = out + ".build.json"
            if not os.path.isfile(out) or not os.path.isfile(report_path):
                missing_files.append(out)
                continue
            with open(report_path, encoding="utf-8") as handle:
                report = json.load(handle)
            files[out] = {"central": central, "n_status": report["n_status"],
                          "missing_lumis": report["missing_lumis"],
                          "missing_sumw": report["missing_sumw"],
                          "central_entries": report["central_entries"]}
            for i in range(3):
                totals["n_status"][i] += report["n_status"][i]
            for key in ("missing_sumw", "missing_sumsign", "missing_count", "central_entries", "aux_only"):
                totals[key] += report.get(key, 0)
            missing.update(tuple(x) for x in report["missing_lumis"])
        status = f"{args.era}/{sample}: {len(files)}/{len(central_files)} friends"
        if missing_files:
            failed = True
            print(f"\033[91m{status}, {len(missing_files)} missing, e.g. {missing_files[0]}\033[0m")
            continue
        summary = dict(totals, era=args.era, sample=sample, production=production.name,
                       is_mc=is_mc, files=files,
                       missing_lumis=[list(x) for x in sorted(missing)],
                       validated_at=datetime.datetime.now().isoformat(timespec="seconds"))
        sample_dir = production.sample_dir(args.era, sample)
        os.makedirs(sample_dir, exist_ok=True)
        if missing and not is_mc:
            mask_path = os.path.join(sample_dir, "missing_lumis.json")
            with open(mask_path, "w", encoding="utf-8") as handle:
                json.dump(brilcalc_mask(sorted(missing)), handle, indent=1)
            summary["missing_lumis_json"] = mask_path
            summary["brilcalc"] = (f"brilcalc lumi -u /pb --normtag <normtag> -i {mask_path}  "
                                   "# recorded lumi to subtract from the trigger lumi")
        with open(production.summary_path(args.era, sample), "w", encoding="utf-8") as handle:
            json.dump(summary, handle, indent=2)
        note = f"{status}, status counts {totals['n_status']}"
        if missing:
            note += f", {len(missing)} lumis never processed"
            note += (f", missing sumW {totals['missing_sumw']}" if is_mc else
                     f", mask {summary['missing_lumis_json']}")
        print(note)
        update_production_json(production, args.era, sample)
    if failed:
        raise SystemExit(1)


def update_production_json(production, era, sample):
    path = production.production_json_path()
    payload = {}
    if os.path.isfile(path):
        with open(path, encoding="utf-8") as handle:
            payload = json.load(handle)
    payload.update({"production": production.name, "format_version": 1,
                    "module_dir": production.module_dir,
                    "overrides": production.overrides,
                    "exclude_prefixes": production.exclude_prefixes,
                    "filter_note": production.filter_note, "sources": production.sources,
                    "updated_at": datetime.datetime.now().isoformat(timespec="seconds")})
    payload.setdefault("samples", {}).setdefault(era, {})[sample] = production.summary_path(era, sample)
    with open(path, "w", encoding="utf-8") as handle:
        json.dump(payload, handle, indent=2, sort_keys=True)


def cmd_status(args):
    production = production_by_name(args.production)
    registry = sample_registry(args.era)
    for sample in samples_from(args):
        name, period = split_sample(sample, registry)
        spec = dict(registry[name], name=name)
        central_files = resolve_sample_paths(spec, args.era, period)
        present = sum(os.path.isfile(production.friend_path(c)) for c in central_files)
        summary = production.summary_path(args.era, sample)
        print(f"{args.era}/{sample}: {present}/{len(central_files)} friends, "
              f"summary {'present' if os.path.isfile(summary) else 'missing'}")


def samples_from(args) -> list[str]:
    samples = [s for s in args.samples.split(",") if s]
    if args.period:
        samples = [f"{s}_{args.period}" for s in samples]
    return samples


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=["build", "validate", "status"])
    parser.add_argument("-p", "--production", required=True)
    parser.add_argument("-e", "--era", required=True)
    parser.add_argument("-i", "--samples", required=True, help="comma-separated registry names")
    parser.add_argument("--period", default="", help="data period, e.g. C or I_v2")
    parser.add_argument("--condor", action="store_true", help="one condor job per central file")
    parser.add_argument("--memory", type=int, default=3000,
                        help="condor request_memory in MB (a 356-field build peaks at 1.6 GB)")
    parser.add_argument("--scratch", default=os.environ.get("TMPDIR", "/tmp"))
    parser.add_argument("--chunk", type=int, default=5000, help="aux entries buffered per stage file")
    parser.add_argument("--force", action="store_true", help="rebuild friends that already exist")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    global registry_era, args_memory_mb
    registry_era = args.era
    args_memory_mb = args.memory
    if args.command != "status" and not os.path.isfile(BINARY) and args.command == "build":
        raise SystemExit(f"{BINARY} not found; build the framework first")
    {"build": cmd_build, "validate": cmd_validate, "status": cmd_status}[args.command](args)


if __name__ == "__main__":
    main()
