"""Actual ctest -N inventory only; this never executes compiler tests."""
from pathlib import Path
import hashlib
import re
import subprocess
import sys

ctest, build = sys.argv[1:]
proc = subprocess.run([ctest, "--test-dir", build, "-N"],
                      capture_output=True, check=False)
assert proc.returncode == 0, proc.stderr.decode("utf-8", "replace")
text = proc.stdout.decode("utf-8")
rows = re.findall(r"(?m)^[ \t]*Test[ \t]+#[0-9]+:[ \t]+(.+)$", text)
assert len(rows) == len(set(rows)), "duplicate CTest names"
name = "r1144_loop_result_consumer.integration"
assert rows.count(name) == 1, "current R CTest must appear exactly once"
matched = [line.strip() for line in text.splitlines()
           if re.match(r"^[ \t]*Test[ \t]+#[0-9]+:", line) and
           line.strip().endswith(name)]
receipt = (
    "R1144 ACTUAL CTEST-N UNIQUE CURRENT REGISTRATION\n" +
    "command: " + repr([ctest, "--test-dir", build, "-N"]) + "\n" +
    "returncode: 0\n" +
    "full_inventory_sha256: " + hashlib.sha256(proc.stdout).hexdigest() + "\n" +
    "unique_test_count: " + str(len(rows)) + "\n" +
    "\n".join(matched) + "\n" +
    "semantic/native executions in inventory: 0\n")
(Path(build) / "r1144-registration.txt").write_text(receipt)
print(receipt, end="")
