#!/usr/bin/env python3
"""friend_config: friends.yml parsing, path mirroring and pairing rules."""

from __future__ import annotations

import json
import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from python import friend_config as fc  # noqa: E402

FRIENDS_YML = """
hadron_av6:
  analyzers: [HadronAnalyzer]
  eras: ["2024"]
  overrides: [GenPart]
  exclude_prefixes: [D0ToKPiLoose]
  filter_note: "HLT prefilter"
  sources:
    - "{root}/pool/{era}/{PD}/**/*.root"
  samples: [TTLJ_powheg, Muon0]
  on_missing: run_without
other:
  analyzers: [Vcb]
"""


class FriendConfigTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        base = Path(self.tmp.name)
        self.module = base / "Module"
        self.module.mkdir()
        (self.module / fc.FRIENDS_FILENAME).write_text(
            FRIENDS_YML.replace("{root}", str(base)), encoding="utf-8")
        self.input_root = base / "input"
        self.friend_root = base / "friend"
        os.environ[fc.INPUT_ROOT_ENV] = str(self.input_root)
        os.environ[fc.FRIEND_ROOT_ENV] = str(self.friend_root)
        self.central = [str(self.input_root / "2024" / "PD" / "x" / f"tree_{i}.root")
                        for i in range(3)]

    def tearDown(self):
        self.tmp.cleanup()

    def production(self):
        return fc.friends_for([str(self.module)], "HadronAnalyzer", "2024", "TTLJ_powheg")[0]

    def test_load_and_applies(self):
        productions = fc.load_friends([str(self.module)])
        self.assertEqual([p.name for p in productions], ["hadron_av6", "other"])
        p = productions[0]
        self.assertTrue(p.applies("HadronAnalyzer", "2024", "Muon0"))
        self.assertFalse(p.applies("HadronAnalyzer", "2022", "Muon0"))
        self.assertFalse(p.applies("HadronAnalyzer", "2024", "DYJets"))
        self.assertFalse(p.applies("Vcb", "2024", "Muon0"))
        self.assertEqual(p.overrides, ["GenPart"])
        self.assertEqual(p.source_globs("2024", "TTtoLNu2Q"),
                         [f"{self.tmp.name}/pool/2024/TTtoLNu2Q/**/*.root"])
        self.assertEqual(fc.friends_for([str(self.module)], "Nobody", "2024"), [])
        # one directory below the module root, like <Module>/<Analysis>/data/Sample
        nested = self.module / "Analysis"
        nested.mkdir()
        (nested / fc.FRIENDS_FILENAME).write_text("nested:\n  analyzers: [A]\n", encoding="utf-8")
        self.assertEqual([p.name for p in fc.load_friends([str(self.module)])],
                         ["hadron_av6", "other", "nested"])

    def test_bad_yaml_is_rejected(self):
        (self.module / fc.FRIENDS_FILENAME).write_text(
            "x:\n  analyzers: [A]\n  on_missing: maybe\n", encoding="utf-8")
        with self.assertRaises(ValueError):
            fc.load_module_friends(str(self.module))
        (self.module / fc.FRIENDS_FILENAME).write_text(
            "x:\n  analyzers: [A]\n  typo: 1\n", encoding="utf-8")
        with self.assertRaises(ValueError):
            fc.load_module_friends(str(self.module))

    def test_friend_path_mirrors_the_central_path(self):
        p = self.production()
        self.assertEqual(p.friend_path(self.central[0]),
                         str(self.friend_root / "hadron_av6" / "2024" / "PD" / "x" / "tree_0.root"))
        self.assertEqual(p.friend_path("/other/place/a.root"),
                         str(self.friend_root / "hadron_av6" / "other" / "place" / "a.root"))

    def test_exclude_files_leave_the_sources_and_the_sample(self):
        from python.sample_paths import resolve_sample_paths
        pool = Path(self.tmp.name) / "pool" / "2024" / "PD" / "task"
        pool.mkdir(parents=True)
        for name in ("a", "b", "c"):
            (pool / f"{name}.root").write_bytes(b"")
        (self.module / "skip.txt").write_text(f"# CRAB failed\n{pool}/b.root  # redone by c\n",
                                              encoding="utf-8")
        (self.module / fc.FRIENDS_FILENAME).write_text(
            f"p:\n  analyzers: [A]\n  sources: ['{self.tmp.name}/pool/{{era}}/{{PD}}/**/*.root']\n"
            "  exclude_files: skip.txt\n", encoding="utf-8")
        p = fc.load_module_friends(str(self.module))[0]
        self.assertEqual(p.exclude_files, [str(pool / "b.root")])
        self.assertEqual(p.source_files("2024", "PD"), [str(pool / "a.root"), str(pool / "c.root")])
        spec = {"isMC": 1, "path_glob": f"{pool}/*.root", "exclude_files": [str(pool / "b.root")]}
        self.assertEqual(resolve_sample_paths(spec, "2024"), [str(pool / "a.root"), str(pool / "c.root")])

    def test_schema_declares_the_carried_collections(self):
        (self.module / "schema").mkdir()
        (self.module / "schema" / "b.json").write_text(
            json.dumps({"collections": {"Lambda": {}, "Jet": {}}, "ignored_collections": ["X"]}),
            encoding="utf-8")
        (self.module / fc.FRIENDS_FILENAME).write_text(
            "p:\n  analyzers: [A]\n  schema: schema/b.json\nq:\n  analyzers: [A]\n", encoding="utf-8")
        p, q = fc.load_module_friends(str(self.module))
        self.assertEqual((p.schema, p.collections), ("schema/b.json", ["Lambda", "Jet"]))
        self.assertEqual(q.collections, [])

    def _publish(self, p, paths, missing_sumw=0.0):
        for path in paths:
            Path(path).parent.mkdir(parents=True, exist_ok=True)
            Path(path).write_bytes(b"")
        summary = p.summary_path("2024", "TTLJ_powheg")
        Path(summary).parent.mkdir(parents=True, exist_ok=True)
        Path(summary).write_text(json.dumps({
            "files": {path: {"n_status": [1, 0, 0]} for path in paths},
            "n_status": [3, 0, 0], "missing_lumis": [],
            "missing_sumw": missing_sumw, "missing_sumsign": 1.0}), encoding="utf-8")

    def test_pairing_attaches_every_file_and_corrects_sumw(self):
        p = self.production()
        friend_paths = [p.friend_path(c) for c in self.central]
        self._publish(p, friend_paths, missing_sumw=2.5)
        paired = fc.pair_friends([p], self.central, "2024", "TTLJ_powheg", True)
        self.assertEqual(paired.paths, [[f] for f in friend_paths])
        self.assertEqual(len(paired.attached), 1)
        self.assertEqual(paired.missing_sumw, 2.5)
        self.assertEqual(paired.missing_sumsign, 1.0)
        self.assertEqual(paired.attached[0]["production"], "hadron_av6")

    def test_filtered_production_replaces_the_normalisation(self):
        p = self.production()
        p.filtered = True
        friend_paths = [p.friend_path(c) for c in self.central]
        self._publish(p, friend_paths, missing_sumw=2.5)
        summary = p.summary_path("2024", "TTLJ_powheg")
        with self.assertRaises(RuntimeError):  # no 'norm' yet
            fc.pair_friends([p], self.central, "2024", "TTLJ_powheg", True)
        payload = json.loads(Path(summary).read_text(encoding="utf-8"))
        payload["norm"] = {"nmc": 10, "sumw": 7.0, "sumsign": 6.0}
        Path(summary).write_text(json.dumps(payload), encoding="utf-8")
        paired = fc.pair_friends([p], self.central, "2024", "TTLJ_powheg", True)
        self.assertEqual(paired.normalisation(100.0, 90.0), (7.0, 6.0))
        p.filtered = False
        paired = fc.pair_friends([p], self.central, "2024", "TTLJ_powheg", True)
        self.assertEqual(paired.normalisation(100.0, 90.0), (97.5, 89.0))
        (self.module / fc.FRIENDS_FILENAME).write_text(
            "x:\n  analyzers: [A]\n  filtered: yes please\n", encoding="utf-8")
        with self.assertRaises(ValueError):
            fc.load_module_friends(str(self.module))

    def test_missing_files_follow_on_missing(self):
        p = self.production()
        paired = fc.pair_friends([p], self.central, "2024", "TTLJ_powheg", True)
        self.assertEqual(paired.attached, [])          # run_without: silently dropped
        self.assertEqual(paired.paths, [[], [], []])
        p.on_missing = "error"
        with self.assertRaises(RuntimeError):
            fc.pair_friends([p], self.central, "2024", "TTLJ_powheg", True)

    def test_files_absent_from_the_summary_are_rejected(self):
        p = self.production()
        friend_paths = [p.friend_path(c) for c in self.central]
        self._publish(p, friend_paths[:2])
        Path(friend_paths[2]).write_bytes(b"")
        with self.assertRaises(RuntimeError):
            fc.pair_friends([p], self.central, "2024", "TTLJ_powheg", True)


if __name__ == "__main__":
    try:
        import yaml  # noqa: F401
    except ImportError:
        sys.exit(77)
    unittest.main()
