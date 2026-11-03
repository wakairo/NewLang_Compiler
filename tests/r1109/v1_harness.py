#!/usr/bin/env python3
"""Frozen independent R1109 oracle; accepts only external actual-observation reports."""
import json
import pathlib
import sys

base = pathlib.Path(__file__).resolve().parent
expected = json.loads((base / "v1_oracle.json").read_text(encoding="ascii"))
if len(sys.argv) != 2:
    raise SystemExit("usage: v1_harness.py ACTUAL_OBSERVATIONS_JSON")
observed = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
assert observed["target_sha"] == expected["target_sha"]
for name, oracle in expected["positive"].items():
    record = observed["positive"][name]
    assert record["source_check"] == "ACCEPTED", name
    assert record["public_owned_authentic"] is True, name
    assert record["strict_c17_gcc_clang_o0_o2"] is True, name
    assert record["native_proven"] is True, name
    for key, wanted in oracle.items():
        assert record[key] == wanted, (name, key, record.get(key), wanted)
for name in expected["reject"]:
    record = observed["reject"][name]
    assert record["source_check"] == "REJECTED", name
    assert record["c17_emitted"] is False, name
print("R1109 finite V1 actual-observation oracle PASS")
