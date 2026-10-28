"""R979 original FIRST-PASS BLIND: immutable source/expectation regression.

No host-fabricated source authority: all semantic CHECK/emit is the genuine
production CLI. Physical/root ownership is explicitly NOT inferred by this
harness from success or numerically equal addresses.
"""
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile

mode, compiler, cc, sanitizer, directory = sys.argv[1:]
root = Path(directory)
FROZEN = {
    "positive_beacons.nl": "862b6c19941cac0c83f6d4706f63ff682bdbdf8b53cf8384118c502ebe2a7e05",
    "positive_link_counters.nl": "54b60e8c281499beb9402e251e3247b591aa4871a82a48b7f441d110d560f1d3",
    "negative_unknown_field.nl": "2358e56267043d2319a873082c35a45c2ecd45ad219d9a8cdcbdf07ed59b29b2",
    "negative_loan_escape.nl": "6acec1af5ef46a8ae15648c7d02a5838cc097956cd22802d9c8c5080d2af487a",
    "negative_wrong_link_current.nl": "9ba4ed20375b9d15b116315665fe3c4b7e8a0f5ac4cf94a63ea8f86cf9af6682",
    "negative_foreign_owner.nl": "e81ac13f82c4554f06c3e707986c063b148aa877164dd3be0055b6d799554414",
}
POSITIVES = ("positive_beacons.nl", "positive_link_counters.nl")
NEGATIVES = tuple(x for x in FROZEN if x.startswith("negative_"))


def invoke(*args):
    p = subprocess.run([str(x) for x in args], capture_output=True,
                       text=True, timeout=60, check=False)
    print(f"CALL {list(map(str, args))!r} rc={p.returncode} "
          f"stdout={p.stdout[:3000]!r} stderr={p.stderr[:6000]!r}",
          flush=True)
    return p


def assert_frozen():
    for name, expected in FROZEN.items():
        actual = hashlib.sha256((root / name).read_bytes()).hexdigest()
        print(f"FROZEN name={name} SHA256={actual}", flush=True)
        if actual != expected:
            raise AssertionError(f"STOP: source SHA drift: {name}")


def positive():
    with tempfile.TemporaryDirectory(prefix="r979-firstpass-") as td:
        target = Path(td)
        for name in POSITIVES:
            src = root / name
            check = invoke(compiler, "--check", src)
            if check.returncode != 0 or check.stdout or check.stderr:
                raise AssertionError(
                    f"R979 ORIGINAL FIRST PASS {name}: SOURCE/CHECK "
                    "REFUSAL (not a native success; preserve raw diagnostic)")
            print(f"R979 {name}: SOURCE CHECK ACCEPTED; "
                  "public-issued owned and physical identity NOT TESTED",
                  flush=True)
            emitted = invoke(compiler, src)
            if emitted.returncode != 0 or not emitted.stdout or emitted.stderr:
                raise AssertionError(
                    f"R979 {name}: C17 BACKEND NOT REACHED/FAIL-CLOSED; "
                    "native NOT TESTED")
            c = target / f"{src.stem}.c"
            c.write_text(emitted.stdout, encoding="utf-8")
            digest = hashlib.sha256(c.read_bytes()).hexdigest()
            print(f"R979 {name}: C17_SHA256={digest}", flush=True)
            out = target / f"{src.stem}.named.c"
            out.write_text("R979-existing-named-output-SENTINEL\n",
                           encoding="utf-8")
            named = invoke(compiler, "--emit-c17-to", out, src)
            if named.returncode != 0 or named.stdout or named.stderr:
                raise AssertionError(
                    f"R979 {name}: named-output success mismatch")
            if out.read_bytes() != c.read_bytes():
                raise AssertionError(
                    f"R979 {name}: named-output bytes != stdout C17")
            for optimize in (0, 2):
                binary = target / f"{src.stem}-O{optimize}"
                command = [cc, "-std=c17", "-pedantic-errors",
                           "-Wall", "-Wextra", "-Werror",
                           f"-O{optimize}"]
                if sanitizer in ("address", "undefined"):
                    command += [f"-fsanitize={sanitizer}",
                                "-fno-sanitize-recover=all"]
                command += [c, "-o", binary]
                compilation = invoke(*command)
                if compilation.returncode:
                    raise AssertionError(
                        f"R979 {name}: strict C17 compile FAIL O{optimize}")
                native = invoke(binary)
                if native.returncode != 0:
                    raise AssertionError(
                        f"R979 {name}: native status FAIL O{optimize}")
            print(f"R979 {name}: native status 0 O0/O2 on host CC; "
                  "old-Copy/physical-root/issuer/OOM W0/W1/W2 "
                  "NOT OBSERVED by this harness", flush=True)


def negative():
    with tempfile.TemporaryDirectory(prefix="r979-negatives-") as td:
        target = Path(td) / "preserved.c"
        sentinel = b"R979-do-not-clobber-existing-output\n"
        for name in NEGATIVES:
            src = root / name
            check = invoke(compiler, "--check", src)
            if check.returncode == 0:
                raise AssertionError(
                    f"R979 UNEXPECTED SOURCE ACCEPT: {name}; STOP semantic")
            if check.stdout or not check.stderr:
                raise AssertionError(
                    f"R979 invalid source diagnostic contract: {name}")
            print(f"R979 {name}: rejected at CHECK (diagnostic above); "
                  "exact cause to review, no semantic PASS inferred",
                  flush=True)
            emitted = invoke(compiler, src)
            if emitted.returncode == 0 or emitted.stdout:
                raise AssertionError(
                    f"R979 NEGATIVE EMISSION: {name}; STOP safety")
            target.write_bytes(sentinel)
            named = invoke(compiler, "--emit-c17-to", target, src)
            if named.returncode == 0 or named.stdout:
                raise AssertionError(
                    f"R979 NEGATIVE NAMED OUTPUT ACCEPTED: {name}; STOP")
            if target.read_bytes() != sentinel:
                raise AssertionError(
                    f"R979 NAMED OUTPUT CLOBBER: {name}; STOP")
            print(f"R979 {name}: named sentinel preserved", flush=True)


if __name__ == "__main__":
    assert_frozen()
    if mode == "positive":
        positive()
    elif mode == "negative":
        negative()
    else:
        raise SystemExit("unknown R979 test mode")
