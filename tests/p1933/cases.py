"""P1933 source cases and independently calculated caller observations."""
from itertools import product

SELECTOR = """fn sample_slots(load: usize) -> u8 {
    if (load < usize(80)) { u8(27) } else { u8(203) }
}
"""
WIDENING = """fn slots_count(octet: u8) -> usize {
    let count = usize(octet);
    count
}
"""
UNIT = """fn retain_selected(input: u8) -> unit {
    let seen = input;
    seen;
    return unit;
}
"""

def program(producer, mode, operator, high, copied, call=True):
    value = 203 if high else 27
    bound = 203 if operator == "==" else 128
    reverse = mode == "copied"
    comparison = f"{'bound' if reverse else 'count'} {operator} {'count' if reverse else 'bound'}"
    a, b = (bound, value) if reverse else (value, bound)
    condition = a == b if operator == "==" else a < b
    prefix = SELECTOR + "\n"
    conversion = "usize(octet)"
    if producer == "registered":
        prefix += WIDENING + "\n"
        conversion = "slots_count(octet)"
    if call:
        prefix += UNIT + "\n"
    rows = [f"let load = usize({80 if high else 79});",
            "let octet = sample_slots(load);",
            f"let quantity = {conversion};",
            "let count = quantity;",
            f"let bound = usize({bound});"]
    if mode != "direct":
        rows.append(f"let compare = {comparison};")
        comparison = "compare"
    if mode == "copied":
        rows.append("let decision = compare;")
        comparison = "decision"
    rows.append(f"let selected = if ({comparison}) {{ u8(61) }} else {{ u8(179) }};")
    passed = "selected"
    if copied:
        rows.append("let argument = selected;")
        passed = "argument"
    if call:
        rows.append(f"retain_selected({passed});")
    rows += [f"let later = {passed};", "let original = selected;",
             "unit" if mode == "direct" else "return unit;"]
    source = prefix + "fn main() -> unit {\n" + "".join(f"    {row}\n" for row in rows) + "}\n"
    verb = "equal" if operator == "==" else "less"
    label = f"{producer}-{mode}-{verb}-{'high' if high else 'low'}-{'alias' if copied else 'original'}"
    if not call:
        label += "-if-only"
    expected = 61 if condition else 179
    return label, source, {"producer": producer, "case": "composition" if call else "if-only",
                           "selected": "selected", "passed": passed, "value": expected,
                           "condition": int(condition), "input_octet": value, "mode": mode}

def selector_alternative(high):
    value = 203 if high else 27
    source = SELECTOR + "\n" + UNIT + "\n" + f"""fn main() -> unit {{
    let load = usize({80 if high else 79});
    let octet = sample_slots(load);
    let argument = octet;
    retain_selected(argument);
    let later = argument;
    let original = octet;
    return unit;
}}
"""
    return f"selector-unit-{'high' if high else 'low'}", source, {
        "case": "alternative", "selected": "octet", "passed": "argument", "value": value,
    }

def cases():
    for producer, mode, op, high, copied in product(
        ("registered", "local"), ("direct", "named", "copied"), ("==", "<"), (False, True), (False, True),
    ):
        yield program(producer, mode, op, high, copied)
    for producer, high in product(("registered", "local"), (False, True)):
        yield program(producer, "named", "==", high, False, call=False)
    yield selector_alternative(False)
    yield selector_alternative(True)

def refusals():
    for producer in ("registered", "local"):
        _, source, _ = program(producer, "copied", "<", True, True)
        changes = {
            "unrelated-actual": source.replace("retain_selected(argument);", "retain_selected(octet);"),
            "before-if": source.replace("    retain_selected(argument);\n", "").replace(
                "    let selected = if", "    retain_selected(octet);\n    let selected = if"),
            "twice": source.replace("    retain_selected(argument);", "    retain_selected(argument);\n    retain_selected(argument);"),
            "arm-call": source.replace("{ u8(61) } else", "{ retain_selected(octet); u8(61) } else").replace(
                "    retain_selected(argument);\n", ""),
            "without-later-copy": source.replace("    let later = argument;\n    let original = selected;\n", ""),
            "extra-prefix-call": source.replace("    let octet = sample_slots(load);",
                "    let extra = sample_slots(load);\n    let octet = sample_slots(load);"),
        }
        for name, text in changes.items():
            yield producer + "-" + name, text, (0, 4)
    _, source, _ = program("registered", "named", "==", True, False)
    yield "wrong-unit-actual-type", source.replace("retain_selected(selected);", "retain_selected(quantity);"), (3, 3)
