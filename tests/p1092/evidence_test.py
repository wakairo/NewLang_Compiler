"""Historical inputs authenticate archived bytes, not future Production."""
import hashlib
import json
from pathlib import Path
import zipfile

root = Path(__file__).resolve().parents[2]
data = root / "tests/p1092"
for version in range(1, 10):
    frozen = json.loads((data / f"harness-freeze-v{version}.json").read_bytes())["files"]
    with zipfile.ZipFile(data / f"evidence/harness-v{version}-inputs.zip") as archive:
        assert set(archive.namelist()) == set(frozen)
        for name, expected in frozen.items():
            assert hashlib.sha256(archive.read(name)).hexdigest() == expected, (version, name)
for version in (1, 2):
    frozen = json.loads((data / f"source-freeze-v{version}.json").read_bytes())["files"]
    for name, expected in frozen.items():
        assert hashlib.sha256((root / name).read_bytes()).hexdigest() == expected, name
red = (data / "evidence/a4-FIRST-RED.log").read_bytes()
assert hashlib.sha256(red).hexdigest() == "64549728c03482809b9983d00c510856ee8938658ad822e5020971ffce8b48dc"
assert b"fault=21 status=0 changes=2 pair/frame/B=0" in red
old = (root / "tests/p1079/run-v4.py").read_text()
forward = (data / "p1079_forward.py").read_text()
expected = old.replace(
    '"""P1079 existing IF-field current receipt -> actual old Copy and common join."""',
    '"""P1092 prospective P1079 runner: preserve v4; legal A loop CHECK0/C17 refusal."""',
).replace(
    "cli=Path(cli).resolve();data=Path(__file__).resolve().parent;repo=data.parents[1]",
    "cli=Path(cli).resolve();data=Path(__file__).resolve().parents[1]/'p1079';repo=data.parents[1]",
).replace("(data/'sources'/'alternative_loop.nl',3)", "(data/'sources'/'alternative_loop.nl',4)")
assert forward == expected, "prospective P1079 runner must retain every other original assertion"
print("P1092 original full source/oracle, pre-run candidate inputs and FIRST RED preserved")
