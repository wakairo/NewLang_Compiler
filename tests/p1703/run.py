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
accepted = {
    "long-queue": b"size=512 queue=9\n",
    "short-queue": b"size=497 queue=7\n",
    "maximum": b"size=18446744073709551615 queue=9\n",
    "scalar-control": b"size=512\n",
    "no-overhead": b"size=496 queue=7\n",
}
refused = {
    "no-decision": 4, "wrong-binding": 4, "known-actual": 4,
    "overflow-bound": 3, "wrong-guard": 3,
}


def observe(c, name):
    if name == "scalar-control":
        caller = re.search(rb"^static size_t (nl_qty_fn_\d+)\(void\)$", c, re.M)
        assert caller
        return (b"#include <stdio.h>\n#define main nl_original_main\n" + c +
                b"\n#undef main\nint main(void)\n{\n" +
                b'    printf("size=%zu\\n", ' + caller[1] + b"());\n    return 0;\n}\n")

    caller = re.search(rb"^static void nl_qty_fn_\d+\(void\)\n\{\n", c, re.M)
    assert caller
    body = c[caller.end():]
    decisions = list(re.finditer(
        rb"^    _Bool nl_condition_(\d+) = (nl_local_\d+) < "
        rb"\(\(size_t\)UINTMAX_C\(\d+\)\);$", body, re.M,
    ))
    assert len(decisions) == 2
    choice, size = decisions[-1].groups()
    prefix = body[:decisions[-1].start()]
    if name != "no-overhead":
        call = re.search(
            rb"^    size_t " + size + rb" = nl_qty_fn_\d+\(nl_local_\d+\);$",
            prefix, re.M,
        )
        assert call, c
    queue = re.search(
        rb"^    size_t (nl_local_\d+) = nl_if_" + choice + rb";$", body, re.M,
    )
    assert queue
    at = caller.end() + queue.end()
    observed = b'\n    printf("size=%zu queue=%zu\\n", ' + size + b", " + queue[1] + b");"
    return b"#include <stdio.h>\n" + c[:at] + observed + c[at:]


def verify(compiler, sources, raw):
    gcc = shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    assert gcc and clang
    for name in (*accepted, *refused):
        work = raw / name
        work.mkdir()
        source = sources / (name + ".nl")
        status = refused.get(name, 0)
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
            diagnostic = (b"[USIZE-ADDITION-PRECISION]" if status == 3
                          else b"[V1-BACKEND-UNSUPPORTED]")
            assert all(diagnostic in r.stderr for r in (emit, existing, created))
            assert (diagnostic in check.stderr if status == 3 else not check.stderr)
        else:
            assert emit.stdout == target.read_bytes() == absent.read_bytes()
            assert not any(r.stderr for r in (check, emit, existing, created))
            observed = work / "observed.c"
            observed.write_bytes(observe(emit.stdout, name))
            for label, cc in (("gcc", gcc), ("clang", clang)):
                for opt in ("O0", "O2"):
                    for variant, path in (("original", target), ("observed", observed)):
                        tag = f"{label}-{opt}-{variant}"
                        binary = work / tag
                        run(work, tag + "-build", [
                            cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror",
                            "-" + opt, path, "-o", binary,
                        ], 0)
                        output = run(work, tag + "-native", [binary], 0)
                        assert output.stdout == (accepted[name] if variant == "observed" else b"")
                        assert not output.stderr
        print(f"{name} CHECK={check.returncode} C17={emit.returncode} named verified")


if __name__ == "__main__":
    cli, sources = sys.argv[1:3]
    if len(sys.argv) == 4:
        destination = Path(sys.argv[3]).resolve()
        destination.mkdir()
        scope = nullcontext(destination)
    else:
        scope = TemporaryDirectory(prefix="p1703-")
    with scope as directory:
        verify(Path(cli).resolve(), Path(sources).resolve(), Path(directory))
