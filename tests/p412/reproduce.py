"""Fixed production code, public owned evidence, first emission refusal only."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

compiler, probe, fixtures = sys.argv[1:]
fixtures = Path(fixtures)
cases = ["node_two_normal_join", "node_join_both_arms", "node_join_actual_none",
         "node_join_same_locator", "node_join_post_write", "node_join_saved_copy"]
records = []
for case in cases:
    source = fixtures / (case + ".nl")
    cli = subprocess.run([compiler, str(source)], capture_output=True, text=True, timeout=30)
    assert cli.returncode == 4 and not cli.stdout
    assert "V1-BACKEND-UNSUPPORTED" in cli.stderr
    owned = subprocess.run([probe, str(source)], capture_output=True, text=True, timeout=30)
    assert owned.returncode == 0 and not owned.stderr, owned.stderr
    records.append({"source": case, "sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
                    "CLI": cli.returncode, "C_bytes": len(cli.stdout),
                    "diagnostic": cli.stderr, "public_owned": owned.stdout.splitlines(),
                    "C17": "NOT REACHED", "NATIVE": "NOT REACHED"})
print(json.dumps({"fixed_compiler": "adc63b64b656b3d6e0501f29eaeee5181927dc14",
                  "production_changes": 0, "cases": records}, indent=2))
