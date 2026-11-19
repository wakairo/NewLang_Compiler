"""Same-D discarded receiver root read, real H0/H1/H2 and original T."""
import json
import os
from pathlib import Path
import subprocess
import sys


def compile_guest(run, cc, raw, tests, output, sample, sanitizer="none",
                  initial=91, sibling=23, first=67, prefix=0, has_read=1):
    flags = [] if sanitizer == "none" else [
        f"-fsanitize={sanitizer}", "-fno-omit-frame-pointer",
        "-fno-sanitize-recover=all"]
    run(output.name + "-cc", [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic",
        "-Werror", "-O2", *flags, f"-DP1517_SAMPLE={sample}",
        f"-DP1535_INITIAL={initial}", f"-DP1535_SIBLING={sibling}",
        f"-DP1535_FIRST_BYTE={first}", f"-DP1535_PREFIX_READS={prefix}",
        f"-DP1535_HAS_READ={has_read}",
        f'-DNEWLANG_NODE_OBSERVER="{tests / "passive.h"}"',
        f'-DNEWLANG_HEAP_OBSERVER="{tests / "passive.h"}"',
        raw, tests.parent / "p1517/guest_wrap.c", "-Wl,--wrap=malloc",
        "-Wl,--wrap=free", "-o", output])


def execute_worlds(run, exe):
    for fail in [1, 2, 0]:
        run(exe.name + f"-fail{fail}", [exe], native=1,
            env={"P1517_FAIL_AT": str(fail),
                 "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
                 "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})


def main():
    compiler, cc, tests_arg, output_arg, sanitizer = sys.argv[1:]
    tests = Path(tests_arg).resolve()
    output = Path(output_arg).resolve()
    output.mkdir(parents=True, exist_ok=True)
    records = []

    def run(label, argv, native=0, env=None):
        r = subprocess.run(list(map(str, argv)), capture_output=True,
                           env=dict(os.environ, **(env or {})))
        (output / (label + ".stdout")).write_bytes(r.stdout)
        (output / (label + ".stderr")).write_bytes(r.stderr)
        records.append(dict(label=label, argv=list(map(str, argv)),
                            native=native, exit=r.returncode, env=env))
        (output / "commands.json").write_text(json.dumps(records, indent=2))
        assert r.returncode == 0, (label, r.returncode, r.stderr)

    for variant, sample in [("zero", 0), ("nonzero", 42), ("composed", 37)]:
        raw = output / (variant + ".c")
        run(variant + "-emit", [compiler, "--emit-c17-to", raw,
                                 tests / ("P-" + variant + ".nl")])
        exe = output / variant
        extra = dict(initial=83, sibling=27, first=83, prefix=2) if variant == "composed" else {}
        compile_guest(run, cc, raw, tests, exe, sample, sanitizer, **extra)
        execute_worlds(run, exe)
    print("P1535 root-read-only receiver O2 zero/42/composed H0/H1/H2 PASS")


if __name__ == "__main__":
    main()
