"""Isolated filesystem fixtures for the C source -> lexer integration test.

This driver only creates exact bytes and launches the instrumented C executable;
it does not implement/tokenize the expected grammar or replace module unit tests.
"""

from pathlib import Path
import subprocess
import sys
import tempfile


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="newlang-p1-") as temporary:
        root = Path(temporary)
        source = root / "answer.nl"
        source.write_bytes(b"fn answer() -> i32 {\r\n    i32(42)\r\n}\r\n")
        binary = root / "preserved-bytes.nl"
        binary.write_bytes(b"a\x00b\r\n\xef\xbb\xbf\xc3\xa9\xff\r")
        empty = root / "empty.nl"
        empty.write_bytes(b"")
        large = root / "multiple-blocks.nl"
        large.write_bytes(b"x\x00\r\n\xff" * 3000)
        exact_blocks = root / "long-atom.nl"
        exact_blocks.write_bytes(b"z" * 8192)
        return subprocess.run(
            [sys.argv[1], str(source), str(binary), str(empty), str(large),
             str(root / "missing.nl"), str(root), str(exact_blocks)],
            check=False,
            timeout=30,
        ).returncode


if __name__ == "__main__":
    raise SystemExit(main())
