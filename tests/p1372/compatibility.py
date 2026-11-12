"""Preserve measured old C bytes and first-Copy use after a second call."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

compiler, parent = sys.argv[1:3]
root = Path(__file__).resolve().parents[2]
spec = json.loads((Path(__file__).parent / "legacy-C.json").read_text())
targeted = json.loads((Path(__file__).parent / "p1304-C.json").read_text())
assert not (spec["expected"].keys() & targeted["expected"].keys())
expected_profiles = dict(spec["expected"], **targeted["expected"])
Path(parent).mkdir(parents=True, exist_ok=True)
work = Path(tempfile.mkdtemp(prefix="C-compat-", dir=parent))
for source, expected in expected_profiles.items():
    path = root / source
    assert hashlib.sha256(path.read_bytes()).hexdigest() == expected["source_sha256"]
    target = work / (path.parent.name + "-" + path.stem + ".c")
    r = subprocess.run([compiler, "--emit-c17-to", str(target), str(path)], capture_output=True, timeout=60)
    assert r.returncode == 0 and not r.stdout and not r.stderr
    data = target.read_bytes()
    assert hashlib.sha256(data).hexdigest() == expected["C_sha256"], source
    if path.parent.name == "p1347":
        # These are the actual generated C variables and stores. Equal guest
        # bits cannot hide substitution of the second call's Copy here.
        calls = list(re.finditer(rb"const uint8_t (nl_call_\d+) = nl_fn_\d+\(", data))
        assert len(calls) == 2
        first, second = calls
        first_binding = re.search(rb"const uint8_t (nl_local_\d+) = " + first[1] + rb";", data)
        assert first_binding
        kept = re.search(rb"const uint8_t (nl_local_\d+) = " + first_binding[1] + rb";", data)
        second_binding = re.search(rb"const uint8_t (nl_local_\d+) = " + second[1] + rb";", data)
        assert kept and second_binding and kept[1] != second_binding[1]
        function_end = data.index(b"\n}\n", second.end())
        stores = re.findall(rb"\*nl_local_\d+ = (nl_local_\d+);", data[second.end():function_end])
        assert stores == [kept[1]], (source, stores, kept[1])
        print(source, "first-Copy C binding origin after second call PASS", flush=True)
    print(source, "C byte identity PASS", flush=True)
print(work)
