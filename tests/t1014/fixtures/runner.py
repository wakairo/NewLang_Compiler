"""T1014 TEST-ONLY fake receipt maker; never runs compiler/native/guest."""
import hashlib
import json
from pathlib import Path
import sys

assert len(sys.argv) == 7 and sys.argv[1] == "--native-suite"
_, _, _fake_cli, manifest, _observer, oracle, receipt = sys.argv
base = Path(manifest).resolve().parents[2]
info = json.loads(Path(oracle).read_text())
lines = Path(manifest).read_text().splitlines()


def stage(**kw):
    return {"status": "pass", "observed": True,
            "evidence_sha256": hashlib.sha256(b"T1014 SYNTHETIC - NO REAL OBSERVATION").hexdigest(), **kw}

cases = []
for line in lines[5:]:
    _, cid, mode, source, sh = line.split("|")
    if mode == "positive":
        keys = ["check", "owned_current", "old_copy", "emitted_c17_original", "gcc_o0",
                "gcc_o2", "clang_o0", "clang_o2", "guest0", "guest1", "guest2", "asan",
                "ubsan", "teardown"]
        stages = {key: stage() for key in keys}
        stages["owned_current"] = stage(source_issued=True)
        stages["old_copy"] = stage(old=11, current=22)
        stages["emitted_c17_original"] = stage(unchanged=True)
        for world in (0, 1, 2):
            stages[f"guest{world}"] = stage(reached=True, allocation_successes=world,
                                               frees=world)
    else:
        stages = {"check_reject": stage(reason="semantic", code="P3-SYNTHETIC-REJECTION",
                                          safety_reject=True),
                  "no_clobber": stage(unchanged=True)}
    cases.append({"id": cid, "mode": mode, "source_sha256": sh, "stages": stages})
response = {"schema": "NL_R_NATIVE_RECEIPT_V1", "cohort": lines[1].split("|")[1],
            "head": info["head"], "parent": info["parent"],
            "manifest_sha256": hashlib.sha256(Path(manifest).read_bytes()).hexdigest(),
            "cases": cases}
Path(receipt).write_text(json.dumps(response, separators=(",", ":")))
print("T1014 SYNTHETIC RECEIPT CLAIM ONLY; NO NATIVE/OBSERVER EXECUTION")
