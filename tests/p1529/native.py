"""Finite prefix-read/old-Copy regression; physical H0/H1/H2, strict C17."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

CASES = [("A-prefix-zero", 0, 1), ("B-prefix-nonzero", 37, 1),
         ("C-oldcopy-zero", 0, 0), ("D-combined-nonzero", 37, 1)]


def compile_and_run(cc, tests, raw, out, case, opt, sanitizer, run):
    name, sample, reads = case
    exe = out / (name + "-O" + str(opt))
    strict = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    flags = [] if sanitizer == "none" else ["-fsanitize=" + sanitizer,
        "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
    before = hashlib.sha256(raw.read_bytes()).hexdigest()
    run(exe.name + "-cc", [cc, *strict, "-O" + str(opt), *flags,
        "-DP1517_SAMPLE=" + str(sample), "-DP1529_PREFIX_READS=" + str(reads),
        '-DNEWLANG_NODE_OBSERVER="' + str(tests / "passive.h") + '"',
        '-DNEWLANG_HEAP_OBSERVER="' + str(tests / "passive.h") + '"',
        raw, tests / "../p1517/guest_wrap.c", "-Wl,--wrap=malloc",
        "-Wl,--wrap=free", "-o", exe], units=2)
    for fail in [1, 2, 0]:
        result = run(exe.name + "-H" + str(fail), [exe], guest=True,
                     setting={"P1517_FAIL_AT": str(fail)})
        assert b"receiver-exact PASS" in result.stdout
    assert hashlib.sha256(raw.read_bytes()).hexdigest() == before


def main():
    compiler, cc, tests_arg, output, sanitizer = sys.argv[1:]
    tests, out = Path(tests_arg).resolve(), Path(output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    rows = []

    def run(label, argv, units=1, guest=False, setting=None):
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        env.update(setting or {})
        argv = list(map(str, argv))
        r = subprocess.run(argv, capture_output=True, env=env)
        for suffix, data in [("stdout", r.stdout), ("stderr", r.stderr)]:
            (out / (label + "." + suffix)).write_bytes(data)
        rows.append(dict(label=label, argv=argv, translation_units=units,
                         guest=guest, exit=r.returncode))
        (out / "ledger.json").write_text(json.dumps(rows, indent=2) + "\n")
        assert r.returncode == 0, (label, r.stderr)
        return r

    for case in CASES:
        raw = out / (case[0] + ".c")
        run("emit-" + case[0], [compiler, "--emit-c17-to", raw,
                                tests / (case[0] + ".nl")])
        compile_and_run(cc, tests, raw, out, case, 2, sanitizer, run)
    print("P1529 four finite compositions x H0/H1/H2 PASS")


if __name__ == "__main__":
    main()
