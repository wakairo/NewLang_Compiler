"""Frozen actual-source known quantity equality: CHECK and owned, not native."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys


def run(args):
    return subprocess.run(args, text=True, capture_output=True, check=False)


def require(ok, message):
    if not ok:
        raise SystemExit(message)


def main():
    default, experimental, probe, directory = sys.argv[1:]
    root = Path(directory)
    frozen = json.loads((root / "frozen.json").read_text())
    successes = {"equal_true.nl": (2, 2, 1, 0), "equal_false.nl": (2, 3, 0, 0),
                 "copied_renamed.nl": (17, 17, 1, 0), "if_true.nl": (2, 2, 1, 1),
                 "if_false.nl": (2, 3, 0, 1)}
    failures = {"wrong_type.nl": ("USIZE-EQUALITY-TYPE", "semantic"),
                "bool_as_ref.nl": ("P3-WRITE-REF-REQUIRED", "semantic"),
                "unknown.nl": ("USIZE-EQUALITY-PRECISION", "precision"),
                "chain.nl": ("USIZE-EQUALITY-SOURCE-PRECISION", "unsupported"),
                "split_operator.nl": ("P5-EXPRESSION-UNSUPPORTED", "unsupported"),
                "full_guard_true.nl": ("P6-JOIN-PRECISION", "precision"),
                "full_guard_false.nl": ("P6-JOIN-PRECISION", "precision")}
    for name, digest in frozen["sources"].items():
        source = root / name
        require(hashlib.sha256(source.read_bytes()).hexdigest() == digest,
                f"immutable source changed: {name}")
        # Default source admission and original adopted P500 tests stay intact.
        for flags in (["--check"], []):
            original = run([default, *flags, str(source)])
            require(original.returncode == 3 and not original.stdout and
                    "P5-EXPRESSION-UNSUPPORTED" in original.stderr,
                    f"default-off admission changed: {original}")
        checked = run([experimental, "--check", str(source)])
        emitted = run([experimental, str(source)])
        if name in successes:
            require(checked.returncode == 0 and not checked.stderr and not checked.stdout,
                    f"real CHECK rejected: {checked}")
            require(emitted.returncode == 4 and not emitted.stdout and
                    "V1-BACKEND-UNSUPPORTED" in emitted.stderr,
                    f"unproved backend admission: {emitted}")
            public = run([probe, str(source)])
            require(public.returncode == 0 and not public.stderr,
                    f"public source linkage failed: {public}")
            evidence = json.loads(public.stdout)
            a, b, boolean, conditionals = successes[name]
            require((evidence["left"], evidence["right"], evidence["bool"],
                     evidence["if_checks"], evidence["authority"]) ==
                    (a, b, boolean, conditionals, 0), f"wrong source bool: {evidence}")
            print(json.dumps(dict(source=name, sha256=digest, check=0, public=evidence,
                                  c17=4, c_bytes=0, native="NOT REACHED")))
        else:
            code, category = failures[name]
            for result in (checked, emitted):
                require(result.returncode == 3 and not result.stdout and
                        f"error({category})[{code}]" in result.stderr,
                        f"first refusal changed: {result}")
            print(json.dumps(dict(source=name, sha256=digest, exit=3,
                                  diagnostic=checked.stderr.strip(), public="NOT ISSUED")))
    print("P501 bool source/owned FINITE; Pair join precision and all C17/native HOLD")


if __name__ == "__main__":
    main()
