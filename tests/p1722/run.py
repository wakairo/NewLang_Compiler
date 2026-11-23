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
low = b"0,41,11,41,213\n"
high = b"65472,97,11,97,213\n"
admitted = {
    "return-first-low": (b"0", low),
    "return-first-high": (b"0", high),
    "return-second-low": (b"1", low),
    "return-second-high": (b"1", high),
    "tail-second": (b"1", high),
}
refused = {
    "duplicate-target-read": (0, 4, b"V1-BACKEND-UNSUPPORTED"),
    "second-change": (0, 4, b"V1-BACKEND-UNSUPPORTED"),
    "wrong-unit-result": (3, 3, b"P9-RETURN-TYPE"),
}


def one(code, pattern):
    matches = list(re.finditer(pattern, code, re.M))
    assert len(matches) == 1, (pattern, code)
    return matches[0]


def observe(code, field):
    ctor = one(code, rb"^    nl_type_\d+ (nl_local_\d+) = \{[^\n]*\};\n")
    root = ctor[1]
    target = root + rb"\.f" + field
    sibling = root + rb"\.f" + (b"1" if field == b"0" else b"0")
    size = one(code, rb"^    const size_t (nl_local_\d+) = \(\(size_t\)UINTMAX_C\(\d+\)\);\n")
    select = one(code, rb"^    const uint8_t (nl_local_\d+) = \(nl_local_\d+ \? \d+ : \d+\);\n")
    loan = one(code, rb"^        uint8_t \*const (nl_local_\d+) = &" + target + rb";\n")
    writer = loan[1]
    old_load = one(code, rb"^        const uint8_t (nl_replace_old_\d+) = \*" + writer + rb";\n")
    store = one(code, rb"^        \*" + writer + rb" = " + select[1] + rb";\n")
    one(code, rb"^        \*" + writer + rb" = [^;]+;\n")
    forward = one(code, rb"^        (nl_loan_result_\d+) = " + old_load[1] + rb";\n")
    old = one(code, rb"^    const uint8_t (nl_local_\d+) = " + forward[1] + rb";\n")
    updated = one(code, rb"^    const uint8_t (nl_local_\d+) = " + target + rb";\n")
    unchanged = one(code, rb"^    const uint8_t (nl_local_\d+) = " + sibling + rb";\n")
    copied = one(code, rb"^    const uint8_t (nl_local_\d+) = " + old[1] + rb";\n")
    assert ctor.start() < size.start() < select.start() < loan.start()
    assert loan.start() < old_load.start() < store.start() < forward.start()
    assert forward.start() < old.start() < updated.start() < unchanged.start() < copied.start()
    before = (b"    const uint8_t *original_target = &" + target.replace(rb"\.", b".") + b";\n"
              b"    const uint8_t *original_sibling = &" + sibling.replace(rb"\.", b".") + b";\n"
              b"    const uint8_t original_old = *original_target;\n"
              b"    const uint8_t original_flags = *original_sibling;\n")
    at_loan = (b"        assert(" + writer + b" == original_target && " + writer +
               b" != original_sibling);\n")
    after = (b"    assert(" + copied[1] + b" == original_old && " + updated[1] +
             b" == *original_target);\n"
             b"    assert(" + unchanged[1] + b" == original_flags && "
             b"*original_sibling == original_flags);\n"
             b'    printf("%zu,%u,%u,%u,%u\\n", ' + size[1] + b", (unsigned)" +
             select[1] + b", (unsigned)" + copied[1] + b", (unsigned)" +
             updated[1] + b", (unsigned)" + unchanged[1] + b");\n")
    return (b"#include <assert.h>\n#include <stdio.h>\n" + code[:ctor.end()] +
            before + code[ctor.end():loan.end()] + at_loan + code[loan.end():copied.end()] +
            after + code[copied.end():])


def verify(cli, fixtures, raw):
    hosts = (shutil.which("gcc"), shutil.which("clang-23") or shutil.which("clang"))
    assert all(hosts)
    for name in (*admitted, *refused):
        work = raw / name
        work.mkdir()
        src = fixtures / (name + ".nl")
        check_status, emit_status, diagnostic = refused.get(name, (0, 0, b""))
        check = run(work, "check", [cli, "--check", src], check_status)
        assert not check.stdout
        if check_status:
            assert b"[" + diagnostic + b"]" in check.stderr
        else:
            assert not check.stderr
        emit = run(work, "emit", [cli, src], emit_status)
        target, absent = work / "existing.c", work / "absent.c"
        sentinel = b"/* preserved output */\n"
        target.write_bytes(sentinel)
        before = state(target)
        existing = run(work, "named-existing", [cli, "--emit-c17-to", target, src], emit_status)
        created = run(work, "named-absent", [cli, "--emit-c17-to", absent, src], emit_status)
        assert not existing.stdout and not created.stdout and not list(work.glob("*.tmp.*"))
        if emit_status:
            assert not emit.stdout and not absent.exists()
            assert state(target) == before and target.read_bytes() == sentinel
            for run_result in (emit, existing, created):
                assert b"[" + diagnostic + b"]" in run_result.stderr
                assert b"Sanitizer" not in run_result.stderr and b"runtime error:" not in run_result.stderr
            print(f"{name} CHECK={check_status} C17={emit_status}, outputs preserved")
            continue
        assert not any(x.stderr for x in (emit, existing, created))
        assert emit.stdout == target.read_bytes() == absent.read_bytes()
        field, expected = admitted[name]
        observed = work / "observed.c"
        observed.write_bytes(observe(emit.stdout, field))
        for label, host in zip(("gcc", "clang"), hosts):
            native(work, host, [(label + "-original", target, b""),
                                (label + "-observed", observed, expected)])
        print(f"{name} field={field.decode()} bytes,mode,old,updated,sibling={expected.decode().strip()}")


if __name__ == "__main__":
    compiler, fixtures = (Path(arg).resolve() for arg in sys.argv[1:3])
    if len(sys.argv) == 4:
        destination = Path(sys.argv[3]).resolve()
        destination.mkdir()
        scope = nullcontext(destination)
    else:
        scope = TemporaryDirectory(prefix="p1722-")
    with scope as directory:
        verify(compiler, fixtures, Path(directory))
