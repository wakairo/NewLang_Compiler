#!/usr/bin/env python3
"""Echo only unmodified R1440 result blocks from this job's LastTest.log."""
import pathlib
import sys


def main() -> int:
    if len(sys.argv) != 2:
        print("R1440_READOUT_NOT_OBSERVED reason=bad_arguments")
        return 0
    log = pathlib.Path(sys.argv[1])
    if not log.is_file():
        print("R1440_READOUT_NOT_OBSERVED reason=LastTest_missing")
        return 0
    text = log.read_text(encoding="utf-8", errors="replace")
    active = None
    cases = set()
    for line in text.splitlines(keepends=True):
        if line.startswith("R1440_BEGIN case="):
            active = line.split("case=", 1)[1].split()[0]
            cases.add(active)
        if active is not None:
            sys.stdout.write(line)
        if active is not None and line.startswith(f"R1440_END case={active}"):
            active = None
    for case in ("R1", "R2", "R3"):
        if case not in cases:
            print(f"R1440_READOUT_NOT_OBSERVED case={case}")
    if active is not None:
        print(f"R1440_READOUT_TRUNCATED case={active}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
