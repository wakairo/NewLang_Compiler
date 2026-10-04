"""Exact isolated file fixtures; all parsing/assertions execute in C."""

from pathlib import Path
import subprocess
import sys
import tempfile


def main() -> int:
    fixtures = (
        b"ref < write , ptr < T > >\r\n",
        b" \tf(g(x),\r\n y)\t",
        b"let old = replace(dst, value)\r\n",
        b"\tloan exclusive read p using stable as r {\r\n { f(r) } if x { y; }\r\n}\n",
        b"loan read x as r { \x00 }",
    )
    with tempfile.TemporaryDirectory(prefix="newlang-p2-") as temporary:
        paths = []
        for index, contents in enumerate(fixtures):
            path = Path(temporary) / f"fragment-{index}.nl"
            path.write_bytes(contents)
            paths.append(str(path))
        return subprocess.run([sys.argv[1], *paths], check=False, timeout=30).returncode


if __name__ == "__main__":
    raise SystemExit(main())
