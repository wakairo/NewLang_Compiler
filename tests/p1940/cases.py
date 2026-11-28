"""Fixtures and independent observations for a scalar call after a caller IF."""
from itertools import product

SELECTOR = """fn sample_slots(load: usize) -> u8 {
    if (load < usize(80)) { u8(31) } else { u8(209) }
}
"""
WIDENER = """fn slots_count(octet: u8) -> usize {
    let count = usize(octet);
    count
}
"""
COPY_RESULT = """fn report_byte(input: u8) -> u8 {
    let kept = input;
    kept
}
"""
DISTINCT_RESULT = """fn report_byte(input: u8) -> u8 {
    let kept = input;
    let code = u8(87);
    code
}
"""
UNIT = """fn lodge_byte(input: u8) -> unit {
    let witnessed = input;
    witnessed;
    return unit;
}
"""
PRODUCERS = ("registered", "local")
FORMS = ("direct", "named", "copied")


def main(rows):
    return "fn main() -> unit {\n" + "".join(f"    {row}\n" for row in rows) + "}\n"


def composed(producer, form, op, high, alias, distinct=False, kind="scalar"):
    octet = 209 if high else 31
    bound = 209 if op == "==" else 127
    reverse = form == "copied"
    left, right = (bound, octet) if reverse else (octet, bound)
    comparison = (f"bound {op} count" if reverse else f"count {op} bound")
    truth = left == right if op == "==" else left < right
    selected = 52 if truth else 166
    headers = [SELECTOR]
    widening = "usize(octet)"
    if producer == "registered":
        widening = "slots_count(octet)"
        headers.append(WIDENER)
    rows = [
        f"let load = usize({80 if high else 79});",
        "let octet = sample_slots(load);",
        f"let quantity = {widening};",
        "let count = quantity;",
        f"let bound = usize({bound});",
    ]
    if form in ("named", "copied"):
        rows.append(f"let compared = {comparison};")
        comparison = "compared"
    if form == "copied":
        rows.append("let decision = compared;")
        comparison = "decision"
    rows.append(f"let selected = if ({comparison}) {{ u8(52) }} else {{ u8(166) }};")
    actual = "selected"
    if alias:
        rows.append("let argument = selected;")
        actual = "argument"
    result = None
    if kind == "scalar":
        result = 87 if distinct else selected
        headers.append(DISTINCT_RESULT if distinct else COPY_RESULT)
        rows.append(f"let returned = report_byte({actual});")
        if form == "copied":
            rows += ["let original_copy = selected;", "let returned_copy = returned;"]
        else:
            rows += ["let returned_copy = returned;", "let original_copy = selected;"]
    elif kind == "unit":
        headers.append(UNIT)
        rows += [f"lodge_byte({actual});", "let original_copy = selected;"]
    else:
        rows.append("let original_copy = selected;")
    rows.append("unit" if form == "direct" else "return unit;")
    direction = "equal" if op == "==" else "less"
    name = f"{producer}-{form}-{direction}-{'high' if high else 'low'}-{'alias' if alias else 'original'}"
    if distinct:
        name += "-distinct"
    if kind != "scalar":
        name += "-" + kind
    return name, "\n".join(headers + [main(rows)]), {
        "producer": producer, "kind": kind, "actual": actual,
        "selection": "selected", "selected": selected, "returned": result,
        "condition": int(truth), "input_octet": octet, "distinct": distinct,
    }


def selector_control(high, distinct):
    selected = 209 if high else 31
    source = "\n".join([SELECTOR, DISTINCT_RESULT if distinct else COPY_RESULT, main([
        f"let load = usize({80 if high else 79});",
        "let octet = sample_slots(load);",
        "let returned = report_byte(octet);",
        "let returned_copy = returned;",
        "let original_copy = octet;",
        "return unit;",
    ])])
    return f"selector-scalar-{'high' if high else 'low'}-{'distinct' if distinct else 'copy'}", source, {
        "kind": "selector", "actual": "octet", "selection": "octet",
        "selected": selected, "returned": 87 if distinct else selected, "distinct": distinct,
    }


def cases():
    for args in product(PRODUCERS, FORMS, ("==", "<"), (False, True), (False, True)):
        yield composed(*args)
    for producer, high, alias in product(PRODUCERS, (False, True), (False, True)):
        yield composed(producer, "named", "==", high, alias, distinct=True)
    for high, distinct in product((False, True), (False, True)):
        yield selector_control(high, distinct)
    for producer, high, kind in product(PRODUCERS, (False, True), ("if-only", "unit")):
        yield composed(producer, "named", "==", high, high, kind=kind)


def refusals():
    call = "    let returned = report_byte(argument);\n"
    former_return = "    let returned_copy = returned;\n"
    original_copy = "    let original_copy = selected;\n"
    nested = """fn inner_byte(incoming: u8) -> u8 { incoming }

fn report_byte(input: u8) -> u8 {
    let kept = input;
    inner_byte(kept)
}
"""
    for producer in PRODUCERS:
        _, src, _ = composed(producer, "named", "==", True, True)
        variants = {
            "unrelated-actual": src.replace(call, call.replace("(argument)", "(octet)")),
            "before-if": src.replace(call, "").replace(
                "    let quantity = ", call.replace("(argument)", "(octet)") + "    let quantity = "),
            "twice": src.replace(call, call + "    let again = report_byte(argument);\n"),
            "arm-call": src.replace("{ u8(52) } else", "{ let arm_byte = u8(52); report_byte(arm_byte) } else"),
            "result-copy-from-input": src.replace(former_return, former_return.replace("= returned;", "= selected;")),
            "original-copy-from-result": src.replace(original_copy, original_copy.replace("= selected;", "= returned;")),
            "discarded-result": src.replace(call + former_return, "    report_byte(argument);\n"),
            "explicit-body-return": src.replace("    kept\n}", "    return kept;\n}", 1),
            "nested-body-call": src.replace(COPY_RESULT, nested),
        }
        for label, text in variants.items():
            yield f"{producer}-{label}", text, (0, 4)
    _, src, _ = composed("registered", "named", "==", True, True)
    yield "wrong-scalar-actual-type", src.replace("report_byte(argument);", "report_byte(quantity);"), (3, 3)
