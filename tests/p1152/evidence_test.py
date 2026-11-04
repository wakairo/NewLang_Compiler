"""C228 preserves the diagnostic FIRST, its prediction, and later harness inputs."""
from pathlib import Path
import hashlib
import json
import zipfile

p = Path(__file__).resolve().parent
sha = lambda b: hashlib.sha256(b).hexdigest()
archives = {
    "inputs-prefreeze-v1.zip": "7407b08af138153e4860bb5d7e63a0d89efb0f3fdcf47bd485c898208d452e83",
    "gate0-cli.zip": "f53f59320f531b456ebe2a24488069b0c17c96933d09f9597915d5f2dfcebdaf",
    "harness-inputs-v1.zip": "bd6e9abed51ffc6aed4cabd49d8f7c90063de70df0e1fbce48e5f1637ce19e84",
    "candidate-v1.zip": "1bd3b42ea61e10ca4b70c1afcae13ef7e1bfe3dbca1faef420ba37cae384aed3",
}
for name, expected in archives.items():
    assert sha((p / "evidence" / name).read_bytes()) == expected, name
freeze = json.loads((p / "gate0-freeze.json").read_bytes())
with zipfile.ZipFile(p / "evidence/inputs-prefreeze-v1.zip") as z:
    for name, expected in freeze["files"].items():
        assert sha(z.read(name)) == expected, name
        assert sha((p / name).read_bytes()) == expected, name
assert sha((p / "evidence/first-cli.log").read_bytes()) == (
    "f3d0688811d0b277883e02f005dab55fd524dee7d49fdcc7c2b705146012e9b0")
with zipfile.ZipFile(p / "evidence/harness-inputs-v1.zip") as z:
    for name, expected in json.loads(z.read("freeze-v1.json")).items():
        assert sha(z.read(name)) == expected, name
        if name == "evidence/boundary-probe.c":
            assert sha((p / "evidence/boundary-probe.c.source").read_bytes()) == expected
for name, expected in json.loads((p / "freeze-v2.json").read_bytes()).items():
    assert sha((p / name).read_bytes()) == expected, name
assert json.loads((p / "oracle.json").read_bytes())["controls"][
    "82-different-child-control"]["check"] == 0
assert json.loads((p / "observed-layers-v1.json").read_bytes())["controls"][
    "82-different-child-control"]["check"] == 3
print("C228 original prediction/source/FIRST/raw and versioned harness PASS")
