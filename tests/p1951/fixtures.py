"""Fixed ordinary sources for one readonly call after a widened caller IF."""
from itertools import product


SELECTOR = """fn sample_octet(load: usize) -> u8 {
    if (load < usize(90)) { u8(37) } else { u8(181) }
}
"""
WIDENER = """fn quantity_from_octet(input: u8) -> usize {
    let amount = usize(input);
    amount
}
"""
UNIT = """fn lodge_byte(input: u8) -> unit {
    let kept = input;
    kept;
    return unit;
}
"""
SCALAR = """fn report_byte(input: u8) -> u8 {
    let kept = input;
    kept
}
"""
SIDES = ("left", "right")
PAIRS = ("ll", "lr", "rl", "rr")
FORMS = ("direct", "named", "copied")


def program(kind, pair, form, high, right_first, *, known=False, shared=False):
    op = "==" if form == "named" else "<"
    octets = [181 if high else 37, 37 if op == "==" else 118]
    headers, rows = [], []
    if known:
        rows.append(f"let left_byte = u8({octets[0]});")
    else:
        headers.append(SELECTOR)
        rows += [f"let load = usize({90 if high else 89});",
                 "let left_byte = sample_octet(load);"]
    if shared:
        octets[1] = octets[0]
    rows.append("let right_byte = left_byte;" if shared else f"let right_byte = u8({octets[1]});")
    if "r" in pair:
        headers.append(WIDENER)
    order = (1, 0) if right_first else (0, 1)
    widen_calls = []
    for i in order:
        actual = "left_byte" if shared else f"{SIDES[i]}_byte"
        if pair[i] == "r":
            init = f"quantity_from_octet({actual})"
            widen_calls.append(octets[i])
        elif pair[i] == "l":
            init = f"usize({actual})"
        else:
            init = f"usize({octets[i]})"
        rows.append(f"let {SIDES[i]}_wide = {init};")
    rows += ["let left_value = left_wide;", "let right_value = right_wide;",
             "let left_source = left_byte;", "let right_source = right_byte;"]
    swapped = right_first ^ (form == "copied")
    a, b = (1, 0) if swapped else (0, 1)
    expression = f"{SIDES[a]}_value {op} {SIDES[b]}_value"
    condition = expression
    yes = octets[a] == octets[b] if op == "==" else octets[a] < octets[b]
    trace_names = ["left_byte", "right_byte", "left_wide", "right_wide",
                   "left_value", "right_value", "left_source", "right_source"]
    trace_values = octets * 4
    if form != "direct":
        rows.append(f"let decision = {condition};")
        condition = "decision"
        trace_names.append(condition)
        trace_values.append(int(yes))
    if form == "copied":
        rows.append("let flag = decision;")
        condition = "flag"
        trace_names.append(condition)
        trace_values.append(int(yes))
    chosen = 61 if yes else 197
    rows.append(f"let chosen = if ({condition}) {{ u8(61) }} else {{ u8(197) }};")
    actual = "chosen"
    if kind != "if-only" and (form == "copied" or right_first):
        rows.append("let argument = chosen;")
        actual = "argument"
    if kind == "unit":
        headers.append(UNIT)
        rows += [f"lodge_byte({actual});", "let original_copy = chosen;",
                 "let left_after = left_wide;", "let right_after = right_wide;"]
        trace_names += ["left_after", "right_after"]
        trace_values += octets
    elif kind == "scalar":
        headers.append(SCALAR)
        rows.append(f"let returned = report_byte({actual});")
        copies = ["let returned_copy = returned;", "let original_copy = chosen;"]
        rows += copies[::-1] if right_first else copies
    else:
        rows.append("let original_copy = chosen;")
    rows.append("unit" if form == "direct" else "return unit;")
    source = "\n".join(headers + [
        "fn main() -> unit {\n" + "".join(f"    {row}\n" for row in rows) + "}\n"
    ])
    label = f"{kind}-{pair}-{form}-{'high' if high else 'low'}-{'right-first' if right_first else 'left-first'}"
    if known:
        label += "-known-shared" if shared else "-known"
    return label, source, {
        "kind": kind, "producer_pair": pair, "form": form, "operator": op,
        "actual": actual, "selection": "chosen", "selected": chosen,
        "returned": chosen if kind == "scalar" else None, "distinct": False,
        "known": known, "shared": shared, "octets": octets,
        "evaluation_order": [SIDES[i] for i in order],
        "operand_order": [SIDES[a], SIDES[b]], "condition": condition,
        "widen_calls": widen_calls, "true_arm": yes,
        "prefix_names": trace_names, "prefix_values": trace_values,
    }


def compositions():
    for args in product(("unit", "scalar"), PAIRS, FORMS, (False, True), (False, True)):
        yield program(*args)
    for kind in ("unit", "scalar"):
        yield program(kind, "rr", "named", True, True, known=True, shared=True)


def controls():
    for kind, pair in product(("unit", "scalar"), ("lk", "rk")):
        yield program(kind, pair, "copied", pair == "rk", True)
    for pair in PAIRS:
        yield program("if-only", pair, "named", pair in ("rl", "rr"), False)


def negatives():
    _, unit, _ = program("unit", "rr", "named", False, False)
    _, scalar, _ = program("scalar", "rr", "named", False, False)
    ret = "    let returned_copy = returned;"
    original = "    let original_copy = chosen;"
    unit_call = "    lodge_byte(chosen);"
    scalar_call = "    let returned = report_byte(chosen);"
    choice = "    let chosen = "
    with_other = "    let unrelated = u8(61);\n" + choice
    backend = (0, 4, "V1-BACKEND-UNSUPPORTED")
    cases = {
        "unit-copy-before-call": (unit.replace(unit_call + "\n" + original, original + "\n" + unit_call), backend),
        "unit-unrelated-equal-actual": (unit.replace(choice, with_other).replace(unit_call, "    lodge_byte(unrelated);"), backend),
        "scalar-unrelated-equal-actual": (scalar.replace(choice, with_other).replace("report_byte(chosen)", "report_byte(unrelated)"), backend),
        "scalar-original-copy-before": (scalar.replace(original+"\n","").replace(scalar_call,original+"\n"+scalar_call), backend),
        "scalar-copy-result-from-original": (scalar.replace(ret, "    let returned_copy = chosen;"), backend),
        "scalar-copy-original-from-result": (scalar.replace(original, "    let original_copy = returned;"), backend),
        "scalar-unrelated-after": (scalar.replace(original,original+"\n    let unrelated = left_byte;"), backend),
        "scalar-literal-result": (scalar.replace("    kept\n}", "    let answer = u8(73);\n    answer\n}"), backend),
        "scalar-explicit-return": (scalar.replace("    kept\n}", "    return kept;\n}"), backend),
        "scalar-discard": (scalar.replace(scalar_call+"\n"+ret, "    report_byte(chosen);"), backend),
        "two-unit-suffixes": (unit.replace(unit_call,unit_call+"\n"+unit_call), backend),
        "two-scalar-suffixes": (scalar.replace(scalar_call,scalar_call+"\n    let another = report_byte(chosen);"), backend),
        "mixed-suffixes": (UNIT+"\n"+scalar.replace(scalar_call,unit_call+"\n"+scalar_call), backend),
        "arm-call": (scalar.replace("{ u8(61) } else", "{ report_byte(left_byte) } else"), backend),
        "unused-third-producer": (unit.replace(choice,"    let unused = usize(left_byte);\n"+choice), backend),
        "wrong-nominal-actual": (scalar.replace("report_byte(chosen)","report_byte(left_wide)"), (3,3,"P3-TYPE-MISMATCH")),
        "wrong-arm-type": (unit.replace("else { u8(197) }","else { usize(197) }"), (3,3,"P13-IF-RESULT")),
    }
    for name, (src, status) in cases.items():
        yield name, src, status
