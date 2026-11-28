"""Owned ordinary-source cases for two widening results in a caller IF."""
from itertools import product


SELECTOR = """fn sample_octet(load: usize) -> u8 {
    if (load < usize(105)) { u8(29) } else { u8(173) }
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
FORMS = ("direct", "named", "copied")
PAIRS = ("ll", "lr", "rl", "rr")


def composition(pair, form, op, high, right_first, swapped, *, known=False, shared=False):
    bv = [173 if high else 29, 29 if op == "==" else 101]
    rows = []
    headers = []
    if known:
        rows.append(f"let left_byte = u8({bv[0]});")
    else:
        headers.append(SELECTOR)
        rows += [f"let load = usize({105 if high else 104});",
                 "let left_byte = sample_octet(load);"]
    if shared:
        bv[1] = bv[0]
    rows.append("let right_byte = left_byte;" if shared else f"let right_byte = u8({bv[1]});")
    if "r" in pair:
        headers.append(WIDENER)
    execution = (1, 0) if right_first else (0, 1)
    calls = []
    for i in execution:
        side = SIDES[i]
        function = "quantity_from_octet" if pair[i] == "r" else "usize"
        actual = "left_byte" if shared else f"{side}_byte"
        rows.append(f"let {side}_wide = {function}({actual});")
        if pair[i] == "r":
            calls.append(bv[i])
    rows += ["let left_seen = left_wide;", "let right_seen = right_wide;"]
    a, b = (1, 0) if swapped else (0, 1)
    expr = f"{SIDES[a]}_seen {op} {SIDES[b]}_seen"
    condition = expr
    yes = bv[a] == bv[b] if op == "==" else bv[a] < bv[b]
    observations = ["left_byte", "right_byte", "left_wide", "right_wide", "left_seen", "right_seen"]
    values = bv * 3
    if form != "direct":
        rows.append(f"let decision = {condition};")
        observations.append("decision")
        values.append(int(yes))
        condition = "decision"
    if form == "copied":
        rows.append("let flag = decision;")
        observations.append("flag")
        values.append(int(yes))
        condition = "flag"
    selected = 47 if yes else 191
    rows += [f"let chosen = if ({condition}) {{ u8(47) }} else {{ u8(191) }};",
             "let saved = chosen;",
             "let left_again = left_wide;", "let right_again = right_wide;",
             "let left_original = left_byte;", "let right_original = right_byte;",
             "unit" if form == "direct" else "return unit;"]
    observations += ["chosen", "saved", "left_again", "right_again", "left_original", "right_original"]
    values += [selected, selected, *bv, *bv]
    source = "\n".join(headers + [
        "fn main() -> unit {\n" + "".join(f"    {row}\n" for row in rows) + "}\n"
    ])
    label = f"{pair}-{form}-{'eq' if op == '==' else 'lt'}-{'high' if high else 'low'}"
    label += f"-{'right-first' if right_first else 'left-first'}-{'swapped' if swapped else 'forward'}"
    if known:
        label += "-known-shared" if shared else "-known"
    return label, source, {
        "names": " ".join(observations), "values": values, "calls": calls,
        "operator": op, "expression": expr, "true_arm": bool(yes), "selected": selected,
        "left_byte": bv[0], "right_byte": bv[1], "producer_pair": pair,
        "evaluation_order": [SIDES[i] for i in execution],
        "operand_order": [SIDES[a], SIDES[b]],
    }


def positives():
    for pair, form, op, high, right_first in product(PAIRS, FORMS, ("==", "<"), (False, True), (False, True)):
        swapped = right_first ^ high ^ (form == "copied")
        yield composition(pair, form, op, high, right_first, swapped)
    for pair, high in product(PAIRS, (False, True)):
        yield composition(pair, "named", "==", high, high, not high, known=True, shared=high)


def negatives():
    _, src, _ = composition("rr", "direct", "<", True, False, False)
    marker = "    let left_seen = left_wide;"
    selected = "    let saved = chosen;"
    wrong = "left_seen < right_seen"
    backend = (0, 4, "V1-BACKEND-UNSUPPORTED")
    variants = {
        "unused-third-local": (src.replace(marker, "    let spare = usize(left_byte);\n" + marker), backend),
        "unused-third-registered": (src.replace(marker, "    let spare = quantity_from_octet(right_byte);\n" + marker), backend),
        "comparison-ignores-second": (src.replace(wrong, "left_seen < usize(101)"), backend),
        "comparison-reuses-first": (src.replace(wrong, "left_seen < left_seen"), backend),
        "duplicate-selector": (src.replace("    let left_byte = ", "    let spare = sample_octet(load);\n    let left_byte = "), backend),
        "late-extra-conversion": (src.replace(selected, selected + "\n    let extra = usize(left_byte);"), backend),
        "arm-call": (src.replace("{ u8(47) } else", "{ sample_octet(load) } else"), backend),
        "extra-arm-bindings": (src.replace("{ u8(47) } else", "{ let branch = u8(47); branch } else"), backend),
        "unit-suffix": (UNIT + "\n" + src.replace(selected, selected + "\n    lodge_byte(chosen);"), backend),
        "scalar-suffix": (SCALAR + "\n" + src.replace(selected, selected + "\n    let result = report_byte(chosen);\n    let kept = result;"), backend),
        "wrong-nominal": (src.replace(wrong, "left_seen < right_byte"), (3, 3, "USIZE-LESS-THAN-TYPE")),
        "wrong-arm-type": (src.replace("else { u8(191) }", "else { usize(191) }"), (3, 3, "P13-IF-RESULT")),
    }
    for label, pair in variants.items():
        yield label, *pair
