from contextlib import nullcontext
from pathlib import Path
import re
import runpy
import shutil
import sys
from tempfile import TemporaryDirectory


here = Path(__file__).resolve().parent
support = runpy.run_path(str(here.parent / "p1682" / "run.py"))
run, state, native = support["run"], support["state"], support["native"]
admitted = {
    "guarded-late-1023": (b"1023,65472,97,11,97,213\n", b"0"),
    "guarded-late-1024": (b"1024,0,41,11,41,213\n", b"0"),
    "reverse-declaration": (b"1023,65472,97,11,97,213\n", b"1"),
}
refused = ("literal-replacement", "wrong-old-copy", "wrong-sibling", "extra-change")


def one(pattern, code):
    matches = list(re.finditer(pattern, code, re.M))
    assert len(matches) == 1, (pattern, code)
    return matches[0]


def observe(code, field):
    other = b"1" if field == b"0" else b"0"
    scalar = one(
        rb"^    size_t (nl_local_\d+) = nl_qty_fn_\d+\((nl_local_\d+)\);\n"
        rb"    \(void\)\1;\n    uint8_t (nl_local_\d+) = nl_qty_fn_\d+\(\1\);\n",
        code,
    )
    size, count, mode = scalar.groups()
    ctor = one(
        rb"^    struct \{ uint8_t f0; uint8_t f1; \} (nl_local_\d+) = [^\n]+;\n",
        code,
    )
    root = ctor[1]
    target = root + rb"\.f" + field
    sibling = root + rb"\.f" + other
    loan = one(rb"^        uint8_t \*const (nl_local_\d+) = &" + target + rb";\n", code)
    writer = loan[1]
    store = one(rb"^        (nl_local_\d+) = \*" + writer + rb";\n"
                rb"        \*" + writer + rb" = " + mode + rb";\n", code)
    one(rb"^        \*" + writer + rb" = [^\n]+;\n", code)
    updated = one(rb"^    const uint8_t (nl_local_\d+) = " + target + rb";\n", code)
    untouched = one(rb"^    const uint8_t (nl_local_\d+) = " + sibling + rb";\n", code)
    old = one(rb"^    const uint8_t (nl_local_\d+) = " + store[1] + rb";\n", code)
    assert scalar.end() < ctor.start() < loan.start() < store.start()
    assert store.end() < updated.start() < untouched.start() < old.start()
    declare = (b"    const uint8_t *observed_target = &" + root + b".f" + field + b";\n"
               b"    const uint8_t *observed_sibling = &" + root + b".f" + other + b";\n"
               b"    const uint8_t original_value = *observed_target;\n"
               b"    const uint8_t sibling_value = *observed_sibling;\n")
    before = b"        assert(" + writer + b" == observed_target && "
    before += writer + b" != observed_sibling);\n"
    after = (b"    assert(" + old[1] + b" == original_value);\n"
             b"    assert(" + updated[1] + b" == *observed_target);\n"
             b"    assert(" + untouched[1] + b" == *observed_sibling && "
             b"*observed_sibling == sibling_value);\n"
             b'    printf("%zu,%zu,%u,%u,%u,%u\\n", ' + count + b", " + size +
             b", (unsigned)" + mode + b", (unsigned)" + old[1] +
             b", (unsigned)" + updated[1] + b", (unsigned)" + untouched[1] + b");\n")
    return (b"#include <assert.h>\n#include <stdio.h>\n" + code[:ctor.end()] +
            declare + code[ctor.end():loan.end()] + before +
            code[loan.end():old.end()] + after + code[old.end():])


def verify(cli, sources, raw):
    hosts = (shutil.which("gcc"), shutil.which("clang-23") or shutil.which("clang"))
    assert all(hosts)
    for name in (*admitted, *refused):
        work = raw / name
        work.mkdir()
        source = sources / (name + ".nl")
        status = 0 if name in admitted else 4
        check = run(work, "check", [cli, "--check", source], 0)
        assert not check.stdout and not check.stderr
        emit = run(work, "emit", [cli, source], status)
        target, absent = work / "existing.c", work / "absent.c"
        sentinel = b"/* preserved output */\n"
        target.write_bytes(sentinel)
        before = state(target)
        existing = run(work, "named-existing", [cli, "--emit-c17-to", target, source], status)
        created = run(work, "named-absent", [cli, "--emit-c17-to", absent, source], status)
        assert not existing.stdout and not created.stdout and not list(work.glob("*.tmp.*"))
        if status:
            assert not emit.stdout and not absent.exists()
            assert before == state(target) and target.read_bytes() == sentinel
            for result in (emit, existing, created):
                assert b"[V1-BACKEND-UNSUPPORTED]" in result.stderr, result
                assert b"Sanitizer" not in result.stderr and b"runtime error:" not in result.stderr
            print(f"{name} C17=4, both destinations preserved")
            continue
        assert not any(result.stderr for result in (emit, existing, created))
        assert emit.stdout == target.read_bytes() == absent.read_bytes()
        wanted, field = admitted[name]
        observed = work / "observed.c"
        observed.write_bytes(observe(emit.stdout, field))
        for label, host in zip(("gcc", "clang"), hosts):
            native(work, host, [(label + "-original", target, b""),
                                (label + "-observed", observed, wanted)])
        print(f"{name} count,bytes,mode,old,updated,sibling={wanted.decode().strip()}")


if __name__ == "__main__":
    compiler, fixtures = (Path(arg).resolve() for arg in sys.argv[1:3])
    if len(sys.argv) == 4:
        destination = Path(sys.argv[3]).resolve()
        destination.mkdir()
        scope = nullcontext(destination)
    else:
        scope = TemporaryDirectory(prefix="p1718-")
    with scope as directory:
        verify(compiler, fixtures, Path(directory))
