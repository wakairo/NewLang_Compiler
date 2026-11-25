from pathlib import Path
import runpy
import sys
from tempfile import TemporaryDirectory


tests = Path(__file__).resolve().parents[1]
support = runpy.run_path(str(tests / "p1722" / "run.py"))
one, verify = support["one"], support["verify"]
admitted = {
    "guarded-delegated-1023": (b"0", b"65472,109,17,109,211\n"),
    "guarded-delegated-1024": (b"0", b"0,53,17,53,211\n"),
    "guarded-delegated-reversed": (b"1", b"65472,109,17,109,211\n"),
}
refused = {
    name: (0, 4, b"V1-BACKEND-UNSUPPORTED")
    for name in ("independent-actual", "independent-input", "independent-old",
                 "second-change", "late-descriptor")
}
refused["scope-exit"] = (3, 3, b"P8-EXIT-DEPENDENCY")


def observe(code, field, local_policy=False, named_policy=False, delegated_policy=False):
    constructor = one(code, rb"^    struct \{ uint8_t f0; uint8_t f1; \} (nl_local_\d+) = [^\n]+;\n")
    root = constructor[1]
    target = root + rb"\.f" + field
    sibling = root + rb"\.f" + (b"1" if field == b"0" else b"0")
    product = one(code, rb"^    size_t (nl_local_\d+) = nl_qty_fn_\d+\((nl_local_\d+)\);\n")
    count = one(code, rb"^    size_t " + product[2] + rb" = [^;]+;\n")
    loan = one(code, rb"^        uint8_t \*const (nl_local_\d+) = &" + target + rb";\n")
    selector = None if local_policy else one(
        code, rb"^    uint8_t (nl_local_\d+) = nl_qty_fn_\d+\(" + product[1] + rb"\);\n")
    actual = product[1] if local_policy else selector[1]
    call = one(code, rb"^        (nl_local_\d+) = (nl_qty_fn_\d+)\(" + loan[1] + b", " + actual + rb"\);\n")
    scalar_type = b"size_t" if local_policy else b"uint8_t"
    helper = one(code, rb"^static uint8_t " + call[2] +
                 rb"\(uint8_t \*(nl_local_\d+), " + scalar_type + rb" (nl_local_\d+)\)\n\{\n")
    load = one(code, rb"^    const uint8_t (nl_previous) = \*" + helper[1] + rb";\n")
    if named_policy:
        assert local_policy
        choice = one(code, rb"^    const uint8_t (nl_local_\d+) = (nl_qty_fn_\d+)\(" +
                     helper[2] + rb"\);\n")
        threshold = choice[2]
        if delegated_policy:
            wrapper = one(code, rb"^static uint8_t " + threshold +
                          rb"\(size_t (nl_local_\d+)\)\n\{\n"
                          rb"    return (nl_qty_fn_\d+)\(\1\);\n\}\n")
            assert wrapper.end() < helper.start()
            threshold = wrapper[2]
        definition = one(code, rb"^static uint8_t " + threshold +
                         rb"\(size_t (nl_local_\d+)\)\n\{\n"
                         rb"    return \1 < \(\(size_t\)UINTMAX_C\(\d+\)\) \? "
                         rb"\(\(uint8_t\)UINT8_C\(\d+\)\) : "
                         rb"\(\(uint8_t\)UINT8_C\(\d+\)\);\n\}\n")
        assert definition.end() < (wrapper.start() if delegated_policy else helper.start())
        assert choice.start() == helper.end() and choice.end() == load.start()
        mode = choice[1]
    elif local_policy:
        choice = one(code, rb"^    const uint8_t (nl_local_\d+) = " + helper[2] +
                     rb" < \(\(size_t\)UINTMAX_C\(\d+\)\) \? "
                     rb"\(\(uint8_t\)UINT8_C\(\d+\)\) : "
                     rb"\(\(uint8_t\)UINT8_C\(\d+\)\);\n")
        assert choice.start() == helper.end() and choice.end() == load.start()
        mode = choice[1]
    else:
        mode = helper[2]
    store = one(code, rb"^    \*" + helper[1] + rb" = " + mode + rb";\n")
    one(code, rb"^ *\*" + helper[1] + rb" = [^;]+;\n")
    returned = one(code, rb"^    return " + load[1] + rb";\n")
    updated = one(code, rb"^    const uint8_t (nl_local_\d+) = " + target + rb";\n")
    unchanged = one(code, rb"^    const uint8_t (nl_local_\d+) = " + sibling + rb";\n")
    copied = one(code, rb"^    const uint8_t (nl_local_\d+) = " + call[1] + rb";\n")
    assert helper.end() <= load.start() < store.start() < returned.start()
    assert constructor.end() < count.start() < product.start() < loan.start() < call.start()
    if selector is not None:
        assert product.start() < selector.start() < loan.start()
    assert call.end() < updated.start() < unchanged.start() < copied.start()
    target, sibling = (name.replace(rb"\.", b".") for name in (target, sibling))
    before = (b"    const uint8_t *original_target = &" + target + b";\n"
              b"    const uint8_t *original_sibling = &" + sibling + b";\n"
              b"    const uint8_t initial = *original_target;\n"
              b"    const uint8_t flags = *original_sibling;\n")
    inside = b"        assert(" + loan[1] + b" == original_target && " + loan[1] + b" != original_sibling);\n"
    selected = updated[1] if local_policy else selector[1]
    mode_check = (b"" if local_policy else
                  b"    assert(" + updated[1] + b" == " + selected + b");\n")
    after = (b"    assert(" + call[1] + b" == initial && " + copied[1] + b" == initial);\n"
             b"    assert(" + selected + b" == *original_target);\n"
             + mode_check +
             b"    assert(" + unchanged[1] + b" == flags && *original_sibling == flags);\n"
             b'    printf("%zu,%u,%u,%u,%u\\n", ' + product[1] + b", (unsigned)" + selected +
             b", (unsigned)" + call[1] + b", (unsigned)" + updated[1] + b", (unsigned)" + unchanged[1] + b");\n")
    return (b"#include <assert.h>\n#include <stdio.h>\n" + code[:constructor.end()] +
            before + code[constructor.end():loan.end()] + inside +
            code[loan.end():copied.end()] + after + code[copied.end():])


if __name__ == "__main__":
    cli, sources = (Path(arg).resolve() for arg in sys.argv[1:3])
    if len(sys.argv) == 4:
        raw = Path(sys.argv[3]).resolve()
        raw.mkdir()
        verify(cli, sources, raw, admitted, refused, observe)
    else:
        with TemporaryDirectory(prefix="p1738-") as directory:
            verify(cli, sources, Path(directory), admitted, refused, observe)
