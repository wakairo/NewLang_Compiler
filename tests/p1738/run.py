from contextlib import nullcontext
from functools import partial
from pathlib import Path
import re
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
    "writer-tail-queue": (b"1", b"32640,31,11,31,91\n"),
    "writer-tail-timer": (b"0", b"0,7,73,7,229\n"),
}
reordered = {
    "split-queue-sibling-first": (b"1", b"32768,17,23,17,191\n"),
    "split-port-sibling-first": (b"0", b"0,2,63,2,207\n"),
    "writer-tail-queue-sibling-first": (b"1", b"32640,31,11,31,91\n"),
}
# Bytes, trial mode, first old, optional intermediate read, second old,
# final mode, unchanged sibling.
sequential = {
    "sequential-named": (b"1", b"4096,31,9,31,31,5,97\n"),
    "adjacent-named": (b"1", b"4096,31,9,31,5,97\n"),
    "sequential-tail": (b"1", b"4096,31,9,31,31,5,97\n"),
}
refused = {
    name: (0, 4, b"V1-BACKEND-UNSUPPORTED")
    for name in ("independent-actual", "independent-input", "independent-old",
                 "second-change", "late-descriptor",
                 "split-direct-sibling-first", "split-duplicate-target",
                 "writer-explicit-return", "writer-tail-wrong-actual",
                 "sequential-different-field", "sequential-wrong-old",
                 "sequential-wrong-actual")
}
refused["scope-exit"] = (3, 3, b"P8-EXIT-DEPENDENCY")


def observe(code, field, local_policy=False, named_policy=False,
            delegated_policy=False, sibling_first=False):
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
    first, second = (unchanged, updated) if sibling_first else (updated, unchanged)
    assert call.end() < first.start() < second.start() < copied.start()
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


def observe_sequence(code, field):
    constructor = one(code, rb"^    struct \{ uint8_t f0; uint8_t f1; \} (nl_local_\d+) = [^\n]+;\n")
    root = constructor[1]
    target = root + rb"\.f" + field
    sibling = root + rb"\.f" + (b"1" if field == b"0" else b"0")
    budget = one(code, rb"^    size_t (nl_local_\d+) = nl_qty_fn_\d+\(nl_local_\d+\);\n")
    selected = one(code, rb"^    uint8_t (nl_local_\d+) = nl_qty_fn_\d+\(" + budget[1] + rb"\);\n")
    first, second = re.finditer(
        rb"^    uint8_t (nl_local_\d+);\n"
        rb"    \{\n"
        rb"        uint8_t \*const (nl_local_\d+) = &" + target + rb";\n"
        rb"        \1 = (nl_qty_fn_\d+)\(\2, (nl_local_\d+)\);\n"
        rb"    \}\n",
        code, re.M,
    )
    assert first[3] == second[3] and first[4] == selected[1]
    exchange = one(
        code,
        rb"^static uint8_t " + first[3] +
        rb"\(uint8_t \*(nl_local_\d+), uint8_t (nl_local_\d+)\)\n"
        rb"\{\n    const uint8_t nl_previous = \*\1;\n"
        rb"    \*\1 = \2;\n    return nl_previous;\n\}\n",
    )
    recovery = one(code, rb"^    const uint8_t " + second[4] +
                        rb" = \(\(uint8_t\)UINT8_C\(\d+\)\);\n")
    reads = list(re.finditer(rb"^    const uint8_t (nl_local_\d+) = " + target + rb";\n", code, re.M))
    assert len(reads) in (1, 2)
    intermediate = reads[0] if len(reads) == 2 else None
    updated = reads[-1]
    unchanged = one(code, rb"^    const uint8_t (nl_local_\d+) = " + sibling + rb";\n")
    copied = [
        one(code, rb"^    const uint8_t (nl_local_\d+) = " + event[1] + rb";\n")
        for event in (first, second)
    ]
    assert exchange.end() < constructor.start() < budget.start() < selected.start()
    assert selected.end() <= first.start() < recovery.start() < second.start()
    assert second.end() <= updated.start() < unchanged.start() < copied[0].start() < copied[1].start()
    if intermediate:
        assert first.end() <= intermediate.start() < recovery.start()

    target = target.replace(rb"\.", b".")
    sibling = sibling.replace(rb"\.", b".")
    edits = [(constructor.end(),
              b"    const uint8_t *original = &" + target + b";\n"
              b"    const uint8_t *original_sibling = &" + sibling + b";\n"
              b"    const uint8_t initial = *original;\n"
              b"    const uint8_t initial_sibling = *original_sibling;\n")]
    for call in (first, second):
        edits.append((code.index(b"\n", call.start(2)) + 1,
                      b"        assert(" + call[2] + b" == original && original != original_sibling);\n"))
    final = (b"    assert(" + copied[0][1] + b" == initial && " + copied[1][1] + b" == " + selected[1] + b");\n"
             b"    assert(" + updated[1] + b" == " + second[4] + b" && " + updated[1] + b" == *original);\n"
             b"    assert(" + unchanged[1] + b" == initial_sibling && *original_sibling == initial_sibling);\n")
    values = [selected[1], copied[0][1]]
    if intermediate:
        final += b"    assert(" + intermediate[1] + b" == " + selected[1] + b");\n"
        values.append(intermediate[1])
    values.extend((copied[1][1], updated[1], unchanged[1]))
    final += (b'    printf("%zu' + b",%u" * len(values) + b'\\n", ' + budget[1] + b", " +
              b", ".join(b"(unsigned)" + value for value in values) + b");\n")
    edits.append((copied[1].end(), final))
    pieces = [b"#include <assert.h>\n#include <stdio.h>\n"]
    previous = 0
    for point, addition in edits:
        pieces.extend((code[previous:point], addition))
        previous = point
    pieces.append(code[previous:])
    return b"".join(pieces)


# Bytes, selected, second actual, then old/final/sibling for each original.
independent = {
    "dual-named": (b"11", b"4032,31,7,19,31,83,43,7,127\n"),
    "dual-direct-tail": (b"11", b"4032,31,7,19,31,83,43,7,127\n"),
    "dual-high": (b"11", b"4096,73,7,19,73,83,43,7,127\n"),
    "mixed-literal-low": (b"10", b"4032,35,19,41,35,117,101,19,149\n"),
    "mixed-literal-high": (b"01", b"4096,93,19,117,93,41,149,19,101\n"),
    "complete-literal-mixed": (b"01", b"4096,93,19,117,93,41,149,19,101\n"),
}
independent_refusals = {
    name: (0, 4, b"V1-BACKEND-UNSUPPORTED")
    for name in ("dual-inline", "dual-crossed-read", "dual-wrong-old", "dual-same-root")
}


# Bytes, selected, observed, active old/final/tag, standby old/final/tag.
mirrored = {
    "mirror-low": (b"11", b"4032,35,35,22,35,119,81,35,147\n"),
    "mirror-high": (b"11", b"4096,93,93,22,93,119,81,93,147\n"),
    "mirror-tail": (b"11", b"4032,35,35,22,35,119,81,35,147\n"),
    "mixed-current-low": (b"10", b"4032,35,35,41,35,117,101,35,149\n"),
    "mixed-current-high": (b"01", b"4096,93,93,117,93,41,149,93,101\n"),
    "complete-current": (b"11", b"4032,35,35,41,35,117,149,35,101\n"),
}
mirror_refusals = {
    name: (0, 4, b"V1-BACKEND-UNSUPPORTED")
    for name in ("mirror-sibling", "mirror-wrong-original", "mirror-stale",
                 "mirror-wrong-actual", "mixed-sibling", "mixed-stale",
                 "mixed-crossed-read", "complete-stale-current",
                 "complete-snapshot")
}


# Bytes, selected, second actual, and old/final/tag for each original.
displaced_modes = {
    "fallback-low": (b"11", b"4032,35,41,41,35,117,101,41,149\n"),
    "fallback-high": (b"11", b"4096,93,57,57,93,117,102,57,149\n"),
    "fallback-tail": (b"11", b"4032,35,41,41,35,117,101,41,149\n"),
    "complete-displaced-unit": (b"00", b"4096,93,117,117,93,41,101,117,149\n"),
}
displaced_refusals = {
    name: (0, 4, b"V1-BACKEND-UNSUPPORTED")
    for name in ("other-copy", "equal-byte-other-copy", "fallback-wrong-actual",
                 "pre-change-read", "mixed-old", "mixed-old-reverse",
                 "complete-mixed-displaced")
}


snapshots = {
    "snapshot-low": (b"11", b"4032,35,35,41,35,117,149,35,101\n"),
    "snapshot-high": (b"00", b"4096,93,93,117,93,41,101,93,149\n"),
    "snapshot-mixed": (b"10", b"4032,35,35,41,35,117,101,35,149\n"),
    "snapshot-mixed-reverse": (b"01", b"4096,93,93,117,93,41,149,93,101\n"),
}
snapshot_refusals = {
    name: (0, 4, b"V1-BACKEND-UNSUPPORTED")
    for name in ("snapshot-stale", "snapshot-sibling-actual",
                 "snapshot-wrong-original", "snapshot-mixed-sibling")
}


def observe_independent(code, positions, mode_source="literal",
                        snapshot_first=False):
    constructors = list(re.finditer(
        rb"^    struct \{ uint8_t f0; uint8_t f1; \} (nl_local_\d+) = [^\n]+;\n", code, re.M
    ))
    assert len(constructors) == 2 and constructors[0][1] != constructors[1][1]
    assert re.fullmatch(rb"[01]{2}", positions)
    fields = positions[:1], positions[1:]
    targets = [ctor[1] + rb"\.f" + field for ctor, field in zip(constructors, fields)]
    siblings = [ctor[1] + rb"\.f" + (b"0" if field == b"1" else b"1")
                for ctor, field in zip(constructors, fields)]
    planned = one(code, rb"^    size_t (nl_local_\d+) = nl_qty_fn_\d+\(nl_local_\d+\);\n")
    trial = one(code, rb"^    uint8_t (nl_local_\d+) = nl_qty_fn_\d+\(" + planned[1] + rb"\);\n")
    calls = [
        one(code, rb"^    uint8_t (nl_local_\d+);\n"
                  rb"    \{\n"
                  rb"        uint8_t \*const (nl_local_\d+) = &" + target + rb";\n"
                  rb"        \1 = (nl_qty_fn_\d+)\(\2, (nl_local_\d+)\);\n"
                  rb"    \}\n")
        for target in targets
    ]
    first, second = calls
    assert first[3] == second[3] and first[4] == trial[1]
    helper = one(code, rb"^static uint8_t " + first[3] +
                 rb"\(uint8_t \*(nl_local_\d+), uint8_t (nl_local_\d+)\)\n"
                 rb"\{\n    const uint8_t nl_previous = \*\1;\n"
                 rb"    \*\1 = \2;\n    return nl_previous;\n\}\n")
    source = {
        "literal": rb"\(\(uint8_t\)UINT8_C\(\d+\)\)",
        "current": targets[0],
        "displaced": first[1],
    }[mode_source]
    incoming = one(code, rb"^    const uint8_t " + second[4] + rb" = " + source + rb";\n")
    reads = []
    paths = (targets[0], siblings[0], targets[1], siblings[1])
    for i, field_path in enumerate(paths):
        pattern = re.compile(rb"^    const uint8_t (nl_local_\d+) = " + field_path + rb";\n", re.M)
        start = first.end() if snapshot_first and i < 2 else second.end()
        read, = pattern.finditer(code, start)
        reads.append(read)
    copied = []
    for call in calls:
        pattern = re.compile(rb"^    const uint8_t (nl_local_\d+) = " + call[1] + rb";\n", re.M)
        result, = pattern.finditer(code, second.end())
        copied.append(result)
    prefix = [helper, *constructors, planned, trial, first]
    if snapshot_first:
        assert mode_source == "current" and reads[0].span() == incoming.span()
        order = [*prefix, *reads[:2], second, *reads[2:], *copied]
    else:
        order = [*prefix, incoming, second, *reads, *copied]
    assert all(left.end() <= right.start() for left, right in zip(order, order[1:]))

    first_target, second_target = (p.replace(rb"\.", b".") for p in targets)
    first_tag, second_tag = (p.replace(rb"\.", b".") for p in siblings)
    setup = (b"    const uint8_t *const first_original = &" + first_target + b";\n"
             b"    const uint8_t *const second_original = &" + second_target + b";\n"
             b"    const uint8_t initial_first = *first_original, initial_second = *second_original;\n"
             b"    const uint8_t saved_first_tag = " + first_tag + b", saved_second_tag = " + second_tag + b";\n"
             b"    assert(first_original != second_original);\n")
    before_first = (b"        assert(" + first[2] + b" == first_original && " + first[2] +
                    b" != &" + first_tag + b");\n")
    before_second = (b"        assert(" + second[2] + b" == second_original && " + second[2] +
                     b" != &" + second_tag + b");\n"
                     b"        assert(*first_original == " + trial[1] +
                     b" && *second_original == initial_second && " + second_tag + b" == saved_second_tag);\n")
    if mode_source != "literal":
        expected_mode = b"*first_original" if mode_source == "current" else b"initial_first"
        before_second += b"        assert(" + second[4] + b" == " + expected_mode + b");\n"
    final = (b"    assert(" + copied[0][1] + b" == initial_first && " + copied[1][1] +
             b" == initial_second);\n"
             b"    assert(" + reads[0][1] + b" == " + trial[1] + b" && *first_original == " + reads[0][1] +
             b" && " + reads[2][1] + b" == " + second[4] + b" && *second_original == " + reads[2][1] + b");\n"
             b"    assert(" + reads[1][1] + b" == saved_first_tag && " + reads[3][1] +
             b" == saved_second_tag);\n")
    values = (trial[1], second[4], copied[0][1], reads[0][1], reads[1][1],
              copied[1][1], reads[2][1], reads[3][1])
    final += (b'    printf("%zu' + b",%u" * len(values) + b'\\n", ' + planned[1] + b", " +
              b", ".join(b"(unsigned)" + item for item in values) + b");\n")
    edits = [
        (constructors[1].end(), setup),
        (code.index(b"\n", first.start(2)) + 1, before_first),
        (code.index(b"\n", second.start(2)) + 1, before_second),
        (copied[1].end(), final),
    ]
    parts = [b"#include <assert.h>\n#include <stdio.h>\n"]
    previous = 0
    for position, content in edits:
        parts.extend((code[previous:position], content))
        previous = position
    parts.append(code[previous:])
    return b"".join(parts)


if __name__ == "__main__":
    cli, sources = (Path(arg).resolve() for arg in sys.argv[1:3])
    if len(sys.argv) == 4:
        raw = Path(sys.argv[3]).resolve()
        raw.mkdir()
        scope = nullcontext(raw)
    else:
        scope = TemporaryDirectory(prefix="p1738-")
    with scope as directory:
        raw = Path(directory)
        verify(cli, sources, raw, admitted, refused, observe)
        verify(cli, sources, raw, reordered, {}, partial(observe, sibling_first=True))
        verify(cli, sources, raw, sequential, {}, observe_sequence)
        verify(cli, sources, raw, independent, independent_refusals, observe_independent)
        verify(cli, sources, raw, mirrored, mirror_refusals,
               partial(observe_independent, mode_source="current"))
        verify(cli, sources, raw, displaced_modes, displaced_refusals,
               partial(observe_independent, mode_source="displaced"))
        verify(cli, sources, raw, snapshots, snapshot_refusals,
               partial(observe_independent, mode_source="current", snapshot_first=True))
