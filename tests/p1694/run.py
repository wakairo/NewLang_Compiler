from contextlib import nullcontext
from pathlib import Path
import re
import runpy
import shutil
import sys
from tempfile import TemporaryDirectory

here = Path(__file__).resolve().parent
support = runpy.run_path(str(here.parent / "p1682" / "run.py"))
run, state, native = (support[name] for name in ("run", "state", "native"))

accepted = {
    "tail-div": (b"/", 34),
    "tail-rem": (b"%", 19),
    "copy-result": (b"%", 19),
    "zero-product": (b"/", 0),
    "boundary": (b"/", 6148914691236517205),
    "split-control": (b"/", 34),
    "copy-control": (None, 1073),
}
refused = {
    "zero-divisor": (3, b"USIZE-DIVISION-ZERO"),
    "zero-remainder": (3, b"USIZE-REMAINDER-ZERO"),
    "overflow": (3, b"USIZE-MULTIPLICATION-OVERFLOW"),
    "named-divisor": (3, b"USIZE-DIVISION-SOURCE-PRECISION"),
    "same-value-decoy": (4, b"V1-BACKEND-UNSUPPORTED"),
    "wrong-result": (4, b"V1-BACKEND-UNSUPPORTED"),
    "unused-step": (4, b"V1-BACKEND-UNSUPPORTED"),
    "three-operations": (4, b"V1-BACKEND-UNSUPPORTED"),
}


def observe(c, operator, split):
    entry = re.search(rb"^static void nl_qty_fn_\d+\(void\)\n\{\n", c, re.M)
    assert entry
    helpers, main = c[:entry.end()], c[entry.end():]
    assert helpers.count(b"nl_product_left * nl_product_right") == 1, c
    operations = re.findall(rb"nl_local_\d+ ([/%]) \(\(size_t\)UINTMAX_C\(", helpers)
    assert operations == ([operator] if operator else []), c
    calls = list(re.finditer(
        rb"^    size_t (nl_local_\d+) = nl_qty_fn_\d+\([^;\n]*\);$", main, re.M,
    ))
    assert len(calls) == (2 if split else 1), c
    last = calls[-1]
    print_value = b'\n    printf("%zu\\n", ' + last[1] + b");"
    return b"#include <stdio.h>\n" + helpers + main[:last.end()] + print_value + main[last.end():]


def verify(cli, sources, raw):
    gcc = shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    assert gcc and clang
    for name in (*accepted, *refused):
        work = raw / name
        work.mkdir()
        source = sources / (name + ".nl")
        status, reason = refused.get(name, (0, b""))
        check = run(work, "check", [cli, "--check", source], 3 if status == 3 else 0)
        emit = run(work, "emit", [cli, source], status)
        target, absent = work / "existing.c", work / "absent.c"
        sentinel = b"/* original C17 output */\n"
        target.write_bytes(sentinel)
        before = state(target)
        existing = run(work, "named-existing", [cli, "--emit-c17-to", target, source], status)
        created = run(work, "named-absent", [cli, "--emit-c17-to", absent, source], status)
        after = state(target)
        (work / "stat.txt").write_text(f"{before}\n{after}\n")
        assert not check.stdout and not existing.stdout and not created.stdout
        assert not list(work.glob("*.tmp.*"))
        results = check, emit, existing, created
        for result in results:
            assert b"Sanitizer" not in result.stderr and b"runtime error:" not in result.stderr
        if not status:
            assert emit.stdout == target.read_bytes() == absent.read_bytes()
            assert not any(result.stderr for result in results)
            operator, value = accepted[name]
            observed = work / "observed.c"
            observed.write_bytes(observe(emit.stdout, operator, name == "split-control"))
            for label, driver in (("gcc", gcc), ("clang", clang)):
                native(work, driver, [(label + "-original", target, b""),
                                      (label + "-observed", observed, f"{value}\n".encode())])
        else:
            assert not emit.stdout and not absent.exists()
            assert before == after and target.read_bytes() == sentinel
            expected = b"[" + reason + b"]"
            assert all(expected in result.stderr for result in (emit, existing, created))
            assert (expected in check.stderr if status == 3 else not check.stderr)
        print(f"{name} CHECK={check.returncode} C17={emit.returncode} named verified")


if __name__ == "__main__":
    cli, sources = sys.argv[1:3]
    if len(sys.argv) == 4:
        destination = Path(sys.argv[3]).resolve()
        destination.mkdir()
        scope = nullcontext(destination)
    else:
        scope = TemporaryDirectory(prefix="p1694-")
    with scope as directory:
        verify(Path(cli).resolve(), Path(sources).resolve(), Path(directory))
