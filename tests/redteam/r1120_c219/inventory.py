#!/usr/bin/env python3
"""Print and validate only this R CTest inventory; never execute tests."""
import re
import subprocess
import sys

expected = "r1120_c219_v3.integration"
result = subprocess.run([sys.argv[1], "--test-dir", sys.argv[2], "-N",
                         "-R", "^r1120_c219_v3[.]integration$"],
                        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                        text=True, check=False)
print(result.stdout, end="", flush=True)
print(result.stderr, end="", file=sys.stderr, flush=True)
names = re.findall(r"(?m)^\s*Test\s+#\d+:\s*(\S+)\s*$", result.stdout)
if result.returncode != 0 or names != [expected]:
    raise SystemExit("R1120_C219 STOP: actual ctest -N registration is not unique")
print("R1120_C219 INVENTORY PASS: one exact R CTest; ZERO tests executed.", flush=True)
