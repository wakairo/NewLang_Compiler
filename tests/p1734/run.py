from pathlib import Path
import runpy
import sys
from tempfile import TemporaryDirectory


tests = Path(__file__).resolve().parents[1]
support = runpy.run_path(str(tests / "p1722" / "run.py"))
one, verify = support["one"], support["verify"]
admitted = {
    "known-delegated-0": (b"0", b"0,41,11,41,213\n"),
    "known-delegated-65472": (b"0", b"65472,97,11,97,213\n"),
    "second-field": (b"1", b"65472,97,11,97,213\n"),
}
refused = {
    name: (0, 4, b"V1-BACKEND-UNSUPPORTED")
    for name in ("independent-actual", "independent-store", "independent-old", "second-change")
}
refused["scope-exit"] = (3, 3, b"P8-EXIT-DEPENDENCY")


def observe_delegate(code, field):
    original = one(code, rb"^    nl_type_\d+ (nl_local_\d+) = [^\n]+;\n")
    target = original[1] + b".f" + field
    sibling = original[1] + b".f" + (b"1" if field == b"0" else b"0")
    byte_count = one(code, rb"^    const size_t (nl_local_\d+) = \(\(size_t\)UINTMAX_C\(\d+\)\);\n")
    select = one(code, rb"^    const uint8_t (nl_call_\d+) = (nl_fn_\d+)\(" + byte_count[1] + rb"\);\n")
    one(code, rb"^static uint8_t " + select[2] + rb"\(size_t nl_local_\d+\)\n\{")
    incoming = one(code, rb"^    const uint8_t (nl_local_\d+) = " + select[1] + rb";\n")
    loan = one(code, rb"^        uint8_t \*const (nl_local_\d+) = &" + target.replace(b".", rb"\.") + rb";\n")
    call = one(code, rb"^        const uint8_t (nl_call_\d+) = (nl_fn_\d+)\(" + loan[1] + b", " + incoming[1] + rb"\);\n")
    callee = one(code, rb"^static uint8_t " + call[2] + rb"\(uint8_t \*(nl_local_\d+), uint8_t (nl_local_\d+)\)\n\{")
    load = one(code, rb"^    const uint8_t (nl_replace_old_\d+) = \*" + callee[1] + rb";\n")
    store = one(code, rb"^    \*" + callee[1] + rb" = " + callee[2] + rb";\n")
    one(code, rb"^ *\*" + callee[1] + rb" = [^;]+;\n")
    ret = one(code, rb"^    const uint8_t (nl_local_\d+) = " + load[1] + rb";\n")
    one(code, rb"^    return " + ret[1] + rb";\n")
    forwarded = one(code, rb"^        (nl_loan_result_\d+) = " + call[1] + rb";\n")
    old = one(code, rb"^    const uint8_t (nl_local_\d+) = " + forwarded[1] + rb";\n")
    updated = one(code, rb"^    const uint8_t (nl_local_\d+) = " + target.replace(b".", rb"\.") + rb";\n")
    unchanged = one(code, rb"^    const uint8_t (nl_local_\d+) = " + sibling.replace(b".", rb"\.") + rb";\n")
    copied = one(code, rb"^    const uint8_t (nl_local_\d+) = " + old[1] + rb";\n")
    assert original.start() < byte_count.start() < select.start() < incoming.start()
    assert incoming.start() < loan.start() < call.start() < forwarded.start()
    assert forwarded.start() < old.start() < updated.start() < unchanged.start() < copied.start()
    assert callee.start() < load.start() < store.start() < ret.start()
    before = (b"    const uint8_t *target = &" + target + b";\n"
              b"    const uint8_t *sibling = &" + sibling + b";\n"
              b"    const uint8_t initial = *target;\n"
              b"    const uint8_t flags = *sibling;\n")
    borrowed = b"        assert(" + loan[1] + b" == target && target != sibling);\n"
    after = (b"    assert(" + old[1] + b" == initial && " + copied[1] + b" == initial);\n"
             b"    assert(" + updated[1] + b" == " + incoming[1] + b" && " + updated[1] + b" == *target);\n"
             b"    assert(" + unchanged[1] + b" == flags && *sibling == flags);\n"
             b'    printf("%zu,%u,%u,%u,%u\\n", ' + byte_count[1] + b", (unsigned)" + incoming[1] +
             b", (unsigned)" + old[1] + b", (unsigned)" + updated[1] + b", (unsigned)" + unchanged[1] + b");\n")
    return (b"#include <assert.h>\n#include <stdio.h>\n" + code[:original.end()] + before +
            code[original.end():loan.end()] + borrowed + code[loan.end():copied.end()] +
            after + code[copied.end():])


if __name__ == "__main__":
    compiler, fixtures = (Path(arg).resolve() for arg in sys.argv[1:3])
    if len(sys.argv) == 4:
        destination = Path(sys.argv[3]).resolve()
        destination.mkdir()
        verify(compiler, fixtures, destination, admitted, refused, observe_delegate)
    else:
        with TemporaryDirectory(prefix="p1734-") as directory:
            verify(compiler, fixtures, Path(directory), admitted, refused, observe_delegate)
