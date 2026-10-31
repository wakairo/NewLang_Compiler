"""P1011/C111 baseline F82 reproduction: scratch data only, fixed parent verifier."""
from pathlib import Path
import hashlib
import json
import runpy
import shutil
import sys
import tempfile

sys.dont_write_bytecode = True
root = Path(sys.argv[1]).resolve()
helper = root / "tests/support/versioned_freeze.py"
helper_bytes = helper.read_bytes()
oid = hashlib.sha1(b"blob " + str(len(helper_bytes)).encode() + b"\0" + helper_bytes).hexdigest()
assert oid == "27440b3a7710c32ac3f6daac04262804e27c5eb4"
verify = runpy.run_path(str(helper))["verify"]
current_name = "tests/p990/current-freeze-v4.json"
current_bytes = (root / current_name).read_bytes()
assert hashlib.sha256(current_bytes).hexdigest() == "2effb5d5da27f337a68147887428f0a06bdef5184dedbb8b8ceaec6d684510af"
current = json.loads(current_bytes)
original_inputs = {n: (root / n).read_bytes() for n in current["files"]}
target_name = "tests/p982/evidence/first-reds.json"
original_target = original_inputs[target_name]
assert hashlib.sha256(original_target).hexdigest() == "f4ef793e961c3d60a3f071fa3286c95197f55ec5c554a22d1d8ca05909ae9658"

def sha(data):
    return hashlib.sha256(data).hexdigest()

for case in ["original-current-v4", "honest-prospective-v5", "f82-coedited-first-red-and-v5-digest"]:
    with tempfile.TemporaryDirectory(prefix="p1011-parent-") as temp:
        work = Path(temp)
        for name, data in {**original_inputs, current_name: current_bytes}.items():
            path = work / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        selected = current_name
        if case != "original-current-v4":
            future = json.loads(current_bytes)
            future["generation"] = 5
            future["previous"] = {"path": current_name, "sha256": sha(current_bytes)}
            future["files"][current_name] = sha(current_bytes)
            future["stage"] = "P1011 scratch F82 reproduction only"
            if case == "f82-coedited-first-red-and-v5-digest":
                records = json.loads(original_target)
                assert isinstance(records, list) and isinstance(records[0], dict)
                records[0]["kind"] = "P1011 SIMULATED COHERENT HISTORY REWRITE"
                changed = (json.dumps(records, indent=2) + "\n").encode()
                assert sha(changed) != sha(original_target)
                (work / target_name).write_bytes(changed)
                future["files"][target_name] = sha(changed)
            selected = "tests/p990/p1011-parent-future-v5.json"
            (work / selected).write_text(json.dumps(future, indent=2) + "\n")
        count = verify(work, selected)
        assert count == (562 if case == "original-current-v4" else 563)
        assert (work / "tests/support/versioned_freeze.py").read_bytes() == helper_bytes
        for n in range(1, 5):
            path = f"tests/p990/current-freeze-v{n}.json"
            assert (work / path).read_bytes() == (root / path).read_bytes()
        print(json.dumps({"case": case, "parent_verifier": "ACCEPT",
                          "protected_count": count,
                          "observed_first_red_sha256": sha((work / target_name).read_bytes()),
                          "old_v1_v4_and_verifier_unchanged": True}, sort_keys=True))
assert helper.read_bytes() == helper_bytes
assert (root / target_name).read_bytes() == original_target
assert all((root / n).read_bytes() == data for n, data in original_inputs.items())
print("P1011 PARENT F82 GATE B REPRODUCED: original history bytes unchanged; coedited future digest accepted")
