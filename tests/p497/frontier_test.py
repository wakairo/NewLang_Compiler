"""Frozen Phase1 source -> genuine checked value -> C17 -> passive native read.

No comparison/branch success is inferred from these discarded literal values.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(args, text=True, capture_output=True, check=False)


def require(ok, message):
    if not ok:
        raise SystemExit(message)


def main():
    compiler, probe, cc, directory = sys.argv[1:]
    source_dir = Path(directory)
    frozen = json.loads((source_dir / "frozen.json").read_text())
    for name, digest in frozen["sources"].items():
        require(hashlib.sha256((source_dir / name).read_bytes()).hexdigest() == digest,
                f"frozen source changed: {name}")
    with tempfile.TemporaryDirectory(prefix="p497-usize-") as folder:
        temp = Path(folder)
        for name, expected in (("literal.nl", 3), ("literal9.nl", 9)):
            source = source_dir / name
            check = run([compiler, "--check", str(source)])
            emitted = run([compiler, str(source)])
            public = run([probe, str(source)])
            require(check.returncode == 0 and not check.stdout and not check.stderr,
                    f"CHECK failed: {check}")
            require(emitted.returncode == 0 and not emitted.stderr,
                    f"emission failed: {emitted}")
            require(public.returncode == 0 and not public.stderr and
                    f"value={expected} copied={expected} " in public.stdout and
                    "authority=0" in public.stdout, f"public views failed: {public}")
            text = emitted.stdout
            pattern = r"const size_t (nl_local_\d+) = \(\(size_t\)UINTMAX_C\((\d+)\)\);"
            carriers = list(re.finditer(pattern, text))
            require(len(carriers) == 1 and int(carriers[0][2]) == expected and
                    "_Static_assert(SIZE_MAX == UINTMAX_C(" in text,
                    "missing original size_t carrier or target-width guard")
            # Observation only reads the existing initialized local; no guest
            # state/condition or C callee result is supplied by the test host.
            observed = text.replace("#include <stddef.h>",
                                    "#include <stddef.h>\n#include <stdlib.h>")
            observed = re.sub(pattern, lambda m: m[0] +
                              f"\n    if ({m[1]} != {expected}) exit(91);", observed)
            mutant = observed.replace(f"((size_t)UINTMAX_C({expected}))",
                                      f"((size_t)UINTMAX_C({expected + 1}))")
            statuses = {}
            for mode, content in (("plain", text), ("observed", observed),
                                  ("broken_carrier", mutant)):
                statuses[mode] = []
                path = temp / f"{name}.{mode}.c"
                path.write_text(content)
                for optimization in ("-O0", "-O2"):
                    exe = temp / f"{name}.{mode}.{optimization}"
                    built = run([cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic",
                                 "-Werror", optimization, str(path), "-o", str(exe)])
                    require(built.returncode == 0, f"strict C17 failed: {built.stderr}")
                    result = run([str(exe)])
                    wanted = 91 if mode == "broken_carrier" else 0
                    require(result.returncode == wanted and not result.stdout and
                            not result.stderr, f"native oracle failed: {result}")
                    statuses[mode].append(result.returncode)
            # Negative control: reject a foreign-width assumption in the same
            # generated C, without claiming cross compilation was measured.
            width_mutant = re.sub(r"SIZE_MAX == UINTMAX_C\(\d+\)",
                                  "SIZE_MAX == UINTMAX_C(0)", text)
            path = temp / "wrong_target.c"
            path.write_text(width_mutant)
            refused = run([cc, "-std=c17", "-c", str(path), "-o", str(temp / "bad.o")])
            require(refused.returncode != 0 and "NewLang usize target mismatch" in refused.stderr,
                    "target guard did not reject mismatch")
            print(json.dumps(dict(source=name, checked=0, public=public.stdout.strip(),
                                  c_sha256=hashlib.sha256(text.encode()).hexdigest(),
                                  native=statuses, target_guard="REJECT")))
        # These remain SOURCE-PROFILE refusals, not semantic safety evidence.
        for name in ("guard_true.nl", "guard_false.nl", "wrong_type.nl", "false_authority.nl"):
            for flags in (["--check"], []):
                result = run([compiler, *flags, str(source_dir / name)])
                require(result.returncode == 3 and not result.stdout and
                        "P5-EXPRESSION-UNSUPPORTED" in result.stderr,
                        f"comparison frontier changed: {result}")
                print(json.dumps(dict(source=name, flags=flags, exit=3,
                                      diagnostic=result.stderr.strip(), stage="PARSER")))
        for flags in (["--check"], []):
            result = run([compiler, *flags, str(source_dir / "literal_overflow.nl")])
            require(result.returncode == 3 and not result.stdout and
                    "error(semantic)[USIZE-LITERAL-RANGE]" in result.stderr,
                    f"overflow rejection failed: {result}")
        # Compatibility negatives use existing semantic rules (no equality).
        negatives = {
            "argument": ("fn echo(x:u8)->u8{x} fn main()->unit{echo(usize(3));unit}",
                         "P3-TYPE-MISMATCH"),
            "return": ("fn echo()->u8{usize(3)} fn main()->unit{echo();unit}",
                       "P8-BODY-RESULT"),
            "authority": ("fn main()->unit{replace(usize(3),u8(7));unit}",
                          "P3-WRITE-REF-REQUIRED"),
        }
        for name, (source, code) in negatives.items():
            path = temp / f"{name}.nl"
            path.write_text(source)
            result = run([compiler, str(path)])
            require(result.returncode == 3 and not result.stdout and code in result.stderr,
                    f"typed quantity safety negative failed: {result}")
            print(json.dumps(dict(negative=name, exit=3, diagnostic=result.stderr.strip())))
    print("P497 PHASE1 ONLY: comparison/if/Pair and equality-authority safety UNTESTED")


if __name__ == "__main__":
    main()
