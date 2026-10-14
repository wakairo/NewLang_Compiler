"""Actual source/check/native separation for the one-field two-normal join."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def main():
    compiler, probe, fixtures = sys.argv[1:]
    fixtures = Path(fixtures)
    records = []
    cases = ["node_two_normal_join", "node_join_actual_none",
             "node_join_other_original", "node_join_both_arms",
             "node_join_clear_occurrence", "node_join_saved_copy",
             "node_join_same_locator", "node_join_post_write"]
    for name in cases:
        source = fixtures / (name + ".nl")
        run = subprocess.run([compiler, str(source)], capture_output=True)
        assert run.returncode == 0 and run.stdout, (name, run)
        assert not run.stderr, (name, run.stderr)
        checked = subprocess.run([probe, "evidence", str(source)], capture_output=True)
        assert checked.returncode == 0, (name, checked.stderr)
        assert b"source-owned=true" in checked.stdout, (name, checked.stdout)
        records.append({"source": name, "sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
                        "exit": run.returncode, "C_bytes": len(run.stdout),
                        "diagnostic": None,
                        "source_owned_public_views": checked.stdout.decode().strip()})
    baseline = subprocess.run([compiler, str(fixtures / "node_link_semantic.nl")],
                              capture_output=True)
    assert baseline.returncode == 0 and baseline.stdout and not baseline.stderr
    print(json.dumps({"baseline_exit": baseline.returncode,
                      "baseline_C_bytes": len(baseline.stdout), "cases": records}, indent=2))


if __name__ == "__main__":
    main()
