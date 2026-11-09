"""Fresh per-source census; every allocator PC and finite ordinal selection."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

probe, destination = sys.argv[1:]
here = Path(__file__).resolve().parent
parent = Path(destination)
parent.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="run-", dir=parent))
for name in ["A-linked-same-arm.nl", "B-known-back-isolation.nl"]:
    for phase in ["check", "emit"]:
        command = [probe, str(here / "sources" / name), phase]
        stem = name + "-" + phase
        with (out / (stem + ".stdout")).open("wb") as stdout, (out / (stem + ".stderr")).open("wb") as stderr:
            result = subprocess.run(command, stdout=stdout, stderr=stderr)
        (out / (stem + ".json")).write_text(json.dumps({"argv": command, "status": result.returncode}) + "\n")
        assert result.returncode == 0, (name, phase, out)
        final = [s for s in (out / (stem + ".stdout")).read_text().splitlines() if s.startswith("OOM-PASS")]
        assert len(final) == 1
        print(name, final[0])
print("P1280 actual CHECK/emitter OOM rollback/retry PASS", out)
