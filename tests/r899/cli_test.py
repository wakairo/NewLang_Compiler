#!/usr/bin/env python3
"""R899 fixed independently frozen source CLI preservation, not native proof."""
import hashlib
import pathlib
import subprocess
import sys
import tempfile

CLI = pathlib.Path(sys.argv[1])
ROOT = pathlib.Path(sys.argv[2])
FROZEN = {
    "meridian.nl": "7fbf208da44fa6a04ad31652f22bc3408d8a11ec64500c63c148e89329b81b79",
    "relay_pair.nl": "8672a1237dbdece516d268db39dc19ba662fc839011d7b967bab63f33cc8bb2c",
}


def call(arguments):
    p = subprocess.run([str(CLI), *map(str, arguments)],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                       timeout=45, check=False)
    return p.returncode, p.stdout, p.stderr


def main():
    for name, sha in FROZEN.items():
        source = ROOT / name
        raw = source.read_bytes()
        assert hashlib.sha256(raw).hexdigest() == sha, "R899 frozen source drift"
        assert raw.endswith(b"\n") and b"\r" not in raw
        with tempfile.TemporaryDirectory(prefix="r899-") as d:
            target = pathlib.Path(d) / "proof.c"
            sentinel = b"R899-KEEP-EXISTING-NAMED-OUTPUT\n"
            target.write_bytes(sentinel)
            cases = [
                ("check", ["--check", source]),
                ("stdout-c17", [source]),
                ("named-c17", ["--emit-c17-to", target, source]),
            ]
            for mode, args in cases:
                status, stdout, stderr = call(args)
                if status == 0 or b"P6-JOIN-PRECISION" not in stderr or stdout:
                    raise AssertionError(
                        f"{name}:{mode} first barrier changed: "
                        f"status={status} stdout={stdout!r} stderr={stderr!r}")
                if target.read_bytes() != sentinel:
                    raise AssertionError(f"{name}:{mode} overwrote named sentinel")
                if list(pathlib.Path(d).glob("proof.c.tmp.*")):
                    raise AssertionError(f"{name}:{mode} left a named output temp")
                print(f"R899 CLI {name} {mode} status={status} P6-CHECK3 "
                      "no stdout, sentinel intact")
    print("R899 PRIVATE-ONLY CLI FINITE CANDIDATE REGRESSION PASS; "
          "base binary byte comparison NOT PERFORMED")


if __name__ == "__main__":
    main()
