from contextlib import nullcontext
from pathlib import Path
import re
import runpy
import shutil
import sys
from tempfile import TemporaryDirectory


here = Path(__file__).resolve().parent
support = runpy.run_path(str(here.parent / "p1682" / "run.py"))
run, state = support["run"], support["state"]

admitted = {
    "nested-7": (3584, 9),
    "nested-8": (0, 7),
    "nested-1": (512, 7),
    "nested-0": (0, 7),
    "exact-bound": (2**64 - 1, 9),
    "flat-7": (3584, 9),
    "flat-8": (0, 7),
    "fixed-divisor": (None, 5),
}
refused = {
    "too-wide": 3,
    "other-condition": 4,
    "nonliteral-arm": 4,
    "forward-only": 4,
    "extra-layer": 4,
}


def observe(c, name):
    decision = re.search(
        rb"^    \(void\)\((nl_local_\d+)\);\n    return;$", c, re.M
    )
    assert decision, c
    at = decision.start()
    c = c[:at] + b'    printf("mode=%zu\\n", ' + decision[1] + b");\n" + c[at:]
    if name != "fixed-divisor":
        assignment = re.search(
            rb"^    size_t (nl_local_\d+) = (nl_qty_fn_\d+)\(nl_local_\d+\);\n"
            rb"    \(void\)\1;\n    size_t nl_if_\d+;\n"
            rb"    _Bool nl_condition_\d+ = \1 < ", c, re.M,
        )
        assert assignment, c
        product = re.search(
            rb"^static size_t " + assignment[2] + rb"\(size_t (nl_local_\d+)\)\n"
            rb"\{\n    size_t (nl_if_\d+);\n"
            rb"    _Bool (nl_condition_\d+) = \1 < [^;]+;\n"
            rb"    if \(\3\) \{\n"
            rb"        const size_t nl_product_left = \1;\n"
            rb"        const size_t nl_product_right = [^;]+;\n"
            rb"        \2 = nl_product_left \* nl_product_right;\n"
            rb"    \} else \{\n"
            rb"        \2 = \(\(size_t\)UINTMAX_C\(0\)\);\n"
            rb"    \}\n    return \2;\n\}", c, re.M,
        )
        assert product, c
        at = assignment.start(0) + assignment[0].index(b"\n") + 1
        c = c[:at] + b'    printf("bytes=%zu\\n", ' + assignment[1] + b");\n" + c[at:]
    return b"#include <stdio.h>\n" + c


def verify(compiler, sources, raw):
    gcc = shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    assert gcc and clang
    for name in (*admitted, *refused):
        work = raw / name
        work.mkdir()
        source = sources / (name + ".nl")
        status = refused.get(name, 0)
        check = run(work, "check", [compiler, "--check", source], 3 if status == 3 else 0)
        emit = run(work, "emit", [compiler, source], status)
        target, absent = work / "existing.c", work / "absent.c"
        sentinel = b"/* preserve original C17 output */\n"
        target.write_bytes(sentinel)
        before = state(target)
        existing = run(work, "named-existing", [compiler, "--emit-c17-to", target, source], status)
        created = run(work, "named-absent", [compiler, "--emit-c17-to", absent, source], status)
        after = state(target)
        (work / "stat.txt").write_text(f"{before}\n{after}\n")
        assert not check.stdout and not existing.stdout and not created.stdout
        assert not list(work.glob("*.tmp.*"))
        for result in (check, emit, existing, created):
            assert b"Sanitizer" not in result.stderr and b"runtime error:" not in result.stderr
        if status:
            assert not emit.stdout and not absent.exists()
            assert before == after and target.read_bytes() == sentinel
            code = (b"[USIZE-MULTIPLICATION-PRECISION]" if status == 3
                    else b"[V1-BACKEND-UNSUPPORTED]")
            assert all(code in result.stderr for result in (emit, existing, created))
            assert (code in check.stderr if status == 3 else not check.stderr)
        else:
            assert not any(result.stderr for result in (check, emit, existing, created))
            assert emit.stdout == target.read_bytes() == absent.read_bytes()
            observed = work / "observed.c"
            observed.write_bytes(observe(emit.stdout, name))
            byte_count, mode = admitted[name]
            output = f"mode={mode}\n".encode()
            if byte_count is not None:
                output = f"bytes={byte_count}\n".encode() + output
            for label, host in (("gcc", gcc), ("clang", clang)):
                support["native"](work, host, [
                    (label + "-original", target, b""),
                    (label + "-observed", observed, output),
                ])
            print(f"{name} CHECK=0 C17=0 {output.decode().strip().replace(chr(10), ' ')}")
        if status:
            print(f"{name} C17={status} named outputs preserved")


if __name__ == "__main__":
    cli, sources = sys.argv[1:3]
    if len(sys.argv) == 4:
        destination = Path(sys.argv[3]).resolve()
        destination.mkdir()
        scope = nullcontext(destination)
    else:
        scope = TemporaryDirectory(prefix="p1707-")
    with scope as directory:
        verify(Path(cli).resolve(), Path(sources).resolve(), Path(directory))
