from contextlib import nullcontext
from pathlib import Path
import re
import runpy
import sys
from tempfile import TemporaryDirectory

here = Path(__file__).resolve().parent
support = runpy.run_path(str(here.parent / "p1654" / "run.py"))
run, state = support["run"], support["state"]

products = {
    "tail-let": (b"391\n", (b"let",)),
    "tail-discard": (b"377\n", (b"discard",)),
    "repeated": (b"391\n391\n391\n", (b"let", b"discard", b"let")),
    "right-named": (b"377\n", (b"let",)),
    "boundary": (b"18446744073709551615\n", (b"let",)),
    "zero": (b"0\n", (b"let",)),
}
controls = ("entry-control", "old-helper-control", "continued-tail")
refused = {
    "overflow": (3, b"USIZE-MULTIPLICATION-OVERFLOW"),
    "wrong-type": (3, b"USIZE-MULTIPLICATION-TYPE"),
    "unknown": (3, b"USIZE-MULTIPLICATION-PRECISION"),
    "formal": (4, b"V1-BACKEND-UNSUPPORTED"),
    "relay": (4, b"V1-BACKEND-UNSUPPORTED"),
    "nested-actual": (4, b"V1-BACKEND-UNSUPPORTED"),
}


def observe(c, order):
    definitions = re.findall(rb"^static size_t (nl_qty_fn_\d+)\(void\)$", c, re.M)
    assert len(definitions) == 1, c
    calls = re.findall(
        rb"^    (size_t nl_local_\d+ = |\(void\)\()" + definitions[0] +
        rb"\(\)\)?;$", c, re.M,
    )
    assert tuple(b"discard" if call.startswith(b"(void)") else b"let"
                 for call in calls) == order, c
    assert re.search(
        rb"    const size_t nl_product_left = [^;]+;\n"
        rb"    const size_t nl_product_right = [^;]+;\n"
        rb"    return nl_product_left \* nl_product_right;\n", c,
    ), c
    original = b"    return nl_product_left * nl_product_right;\n"
    assert c.count(original) == 1
    return b"#include <stdio.h>\n" + c.replace(
        original,
        b"    const size_t observed = nl_product_left * nl_product_right;\n"
        b'    printf("%zu\\n", observed);\n'
        b"    return observed;\n",
    )


def native(work, host, variants):
    for label, src, expected in variants:
        for opt in ("O0", "O2"):
            stem = f"{label}-{opt}"
            binary = work / stem
            run(work, stem + "-build", [
                host, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra",
                "-Werror", "-" + opt, src, "-o", binary,
            ], 0)
            result = run(work, stem + "-native", [binary], 0)
            assert result.stdout == expected and not result.stderr, result


def verify(compiler, host, sources, raw):
    for name in (*products, *controls, *refused):
        work = raw / name
        work.mkdir()
        source = sources / (name + ".nl")
        status, reason = refused.get(name, (0, b""))
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
        if not status:
            assert emit.stdout == target.read_bytes() == absent.read_bytes()
            assert not check.stderr and not emit.stderr and not existing.stderr and not created.stderr
            variants = [("original", target, b"")]
            if name in products:
                value, order = products[name]
                observed = work / "observed.c"
                observed.write_bytes(observe(emit.stdout, order))
                variants.append(("observed", observed, value))
            native(work, host, variants)
        else:
            assert not emit.stdout and not absent.exists()
            assert before == after and target.read_bytes() == sentinel
            for result in (emit, existing, created):
                assert b"[" + reason + b"]" in result.stderr, result
            if status == 3:
                assert reason in check.stderr
        print(f"{name} CHECK={check.returncode} C17={emit.returncode} named verified")


if __name__ == "__main__":
    cli = Path(sys.argv[1]).resolve()
    host = sys.argv[2]
    sources = Path(sys.argv[3]).resolve()
    if len(sys.argv) == 5:
        destination = Path(sys.argv[4]).resolve()
        destination.mkdir()
        scope = nullcontext(destination)
    else:
        scope = TemporaryDirectory(prefix="p1682-")
    with scope as directory:
        verify(cli, host, sources, Path(directory))
