"""P896 does not change CLI source admission; private diagnostics are off."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

compiler, root = sys.argv[1], Path(sys.argv[2])
freeze = json.loads((root / "freeze.json").read_text())
for name, digest in freeze["sources"].items():
    assert hashlib.sha256((root / "sources" / name).read_bytes()).hexdigest() == digest
old = {
    "p793_router_true.nl": "8534e4a80e43aaea5e6a0879632e4046fde59c7b512a22dad204e5a7c7b67a18",
    "p793_router_false.nl": "4d16abc9e185fd9a2bc7cf266307458c69cdeeb22036c6414a45ea052f63d521",
    "p892_budget_true.nl": "a803e3fdc0129aa69d5033997e315413fd9ffb393362198d65365480a6ea47cf",
    "p892_budget_false.nl": "b91d5a29f7f3307c56ab3371a9ce85e815c252ebc1bcf5e21cdfd1a6ca2fc7cf",
}
for name, digest in old.items():
    assert hashlib.sha256((root / "sources" / name).read_bytes()).hexdigest() == digest
for source in sorted((root / "sources").glob("*.nl")):
    check = subprocess.run([compiler, "--check", str(source)], capture_output=True)
    if source.name == "no_call.nl":
        assert check.returncode == 0 and not check.stdout and not check.stderr
        continue
    assert check.returncode == 3 and b"P6-JOIN-PRECISION" in check.stderr
    emitted = subprocess.run([compiler, str(source)], capture_output=True)
    assert emitted.returncode == 3 and not emitted.stdout
    with tempfile.TemporaryDirectory() as temp:
        output = Path(temp) / "saved.c"
        output.write_bytes(b"prior correct output\n")
        named = subprocess.run([compiler, "--emit-c17-to", str(output), str(source)], capture_output=True)
        assert named.returncode == 3 and not named.stdout
        assert output.read_bytes() == b"prior correct output\n"
        assert sorted(Path(temp).iterdir()) == [output]
    print(source.name, "CHECK3 / C0 / named preserved / private observer off")
