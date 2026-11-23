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
maximum = 2**64 - 1
inputs = (0, 1, 2, 1023, 1024, maximum)
admitted = {
    "unit-product": ((0, 64, 128, 65472, 0, 0), 65472),
    "unit-large": ((0, 64, 128, 65472, 0, 0), 0),
    "zero-factor": ((0, 0, 0, 0, 0, 0), 0),
    "exact-max": ((0, maximum, 0, 0, 0, 0), maximum),
    "scalar-addition": ((64, 65, 66, 1087, 1024, maximum), 72),
    "known-product": ((), 512),
}
rejected = {
    "too-wide": 3, "zero-bound": 3, "decoy": 3, "false-arm": 3,
    "let-arm": 3, "scalar-product": 4, "unit-addition": 4,
}


def native_observer(c, name):
    scalar = name == "scalar-addition"
    helper = re.search(rb"^static size_t (nl_qty_fn_\d+)\(size_t ", c, re.M)
    entry = re.search(
        rb"^static " + (rb"size_t" if scalar else rb"void") +
        rb" (nl_qty_fn_\d+)\(void\)\n\{\n", c, re.M,
    )
    assert entry and (helper if name != "known-product" else not helper)
    if not scalar:
        call = re.search(rb"^    size_t (nl_local_\d+) = nl_qty_fn_\d+\([^;]*\);$",
                         c[entry.end():], re.M)
        assert call, c
        at = entry.end() + call.end()
        c = c[:at] + b'\n    printf("caller=%zu\\n", ' + call[1] + b");" + c[at:]
    if helper and not scalar:
        block = c[helper.end():entry.start()]
        assert re.search(
            rb"    if \(nl_condition_\d+\) \{\n"
            rb"        const size_t nl_product_left = nl_local_\d+;\n"
            rb"        const size_t nl_product_right = [^;]+;\n"
            rb"        nl_if_\d+ = nl_product_left \* nl_product_right;\n"
            rb"    \} else \{", block,
        ), c

    wrapper = b"int main(int argc, char **argv)\n{\n"
    if helper:
        wrapper += (
            b"    for (int i = 1; i < argc; ++i) {\n"
            b"        size_t n = (size_t)strtoumax(argv[i], NULL, 10);\n"
            b'        printf("%zu=%zu\\n", n, ' + helper[1] + b"(n));\n"
            b"    }\n"
        )
    else:
        wrapper += b"    (void)argc;\n    (void)argv;\n"
    if scalar:
        wrapper += b'    printf("caller=%zu\\n", ' + entry[1] + b"());\n    return 0;\n"
    else:
        wrapper += b"    return nl_original_main();\n"
    return (b"#include <inttypes.h>\n#include <stdio.h>\n"
            b"#define main nl_original_main\n" + c + b"\n#undef main\n" + wrapper + b"}\n")


def verify(compiler, sources, raw):
    gcc = shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    assert gcc and clang
    for name in (*admitted, *rejected):
        work = raw / name
        work.mkdir()
        source = sources / (name + ".nl")
        status = rejected.get(name, 0)
        check = run(work, "check", [compiler, "--check", source], 3 if status == 3 else 0)
        emit = run(work, "emit", [compiler, source], status)
        target, absent = work / "existing.c", work / "absent.c"
        sentinel = b"/* original C17 output */\n"
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
            diagnostic = (b"[USIZE-MULTIPLICATION-PRECISION]" if status == 3
                          else b"[V1-BACKEND-UNSUPPORTED]")
            assert all(diagnostic in r.stderr for r in (emit, existing, created))
            assert (diagnostic in check.stderr if status == 3 else not check.stderr)
        else:
            assert emit.stdout == target.read_bytes() == absent.read_bytes()
            assert not any(r.stderr for r in (check, emit, existing, created))
            observed = work / "observed.c"
            observed.write_bytes(native_observer(emit.stdout, name))
            values, caller = admitted[name]
            data = inputs if values else ()
            expected = ("".join(f"{i}={v}\n" for i, v in zip(data, values, strict=True)) +
                        f"caller={caller}\n").encode()
            for label, cc in (("gcc", gcc), ("clang", clang)):
                for opt in ("O0", "O2"):
                    for kind, path in (("original", target), ("observed", observed)):
                        name_part = f"{label}-{opt}-{kind}"
                        binary = work / name_part
                        run(work, name_part + "-build", [
                            cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror",
                            "-" + opt, path, "-o", binary,
                        ], 0)
                        args = data if kind == "observed" else ()
                        output = run(work, name_part + "-native", [binary, *args], 0)
                        assert output.stdout == (expected if kind == "observed" else b"")
                        assert not output.stderr
        print(f"{name} CHECK={check.returncode} C17={emit.returncode} named verified")


if __name__ == "__main__":
    cli, sources = sys.argv[1:3]
    if len(sys.argv) == 4:
        destination = Path(sys.argv[3]).resolve()
        destination.mkdir()
        scope = nullcontext(destination)
    else:
        scope = TemporaryDirectory(prefix="p1699-")
    with scope as directory:
        verify(Path(cli).resolve(), Path(sources).resolve(), Path(directory))
