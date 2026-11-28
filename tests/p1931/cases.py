"""P1931 ordinary NewLang source cases and source-level expected values."""
from itertools import product

SELECTOR = """fn sample_octet(load: usize) -> u8 {
    if (load < usize(64)) { u8(39) } else { u8(207) }
}
"""
WIDENER = """fn quantity_from_octet(input: u8) -> usize {
    let widened = usize(input);
    widened
}
"""

def program(producer, condition, operator, reverse, high, *, literal=False):
    verb = "equal" if operator == "==" else "less"
    order = "reverse" if reverse else "forward"
    value = 207 if high else 39
    bound = 39 if operator == "==" else 112
    operands = ("bound", "reading") if reverse else ("reading", "bound")
    expression = f"{operands[0]} {operator} {operands[1]}"
    a, b = (bound, value) if reverse else (value, bound)
    truth = a == b if operator == "==" else a < b
    header = "" if literal else SELECTOR
    widen = "usize(octet)"
    if producer == "registered":
        header += "\n" + WIDENER
        widen = "quantity_from_octet(octet)"
    rows = [] if literal else [f"let load = usize({64 if high else 63});"]
    sample = f"u8({value})" if literal else "sample_octet(load)"
    rows += [
        f"let octet = {sample};",
        f"let quantity = {widen};",
        "let reading = quantity;",
        f"let bound = usize({bound});",
        "let prior_equal = reading == bound;",
    ]
    if condition == "named":
        rows += [f"let compare = {expression};", "let flag = compare;"]
        expression = "flag"
    rows += [
        f"let chosen = if ({expression}) {{ u8(71) }} else {{ u8(163) }};",
        "let saved = chosen;",
        "let later_less = bound < reading;",
        "let summary = later_less;",
        "unit" if condition == "direct" else "return unit;",
    ]
    source = header + ("\n" if header else "") + "fn main() -> unit {\n"
    source += "".join(f"    {row}\n" for row in rows) + "}\n"
    name = f"{producer}-{condition}-{verb}-{order}-{'high' if high else 'low'}"
    if literal:
        name += "-literal"
    oracle = {
        "value": value, "bound": bound, "branch": int(truth),
        "selected": 71 if truth else 163, "equal_before": int(value == bound),
        "less_after": int(bound < value), "selector": not literal,
        "producer": producer, "condition": condition,
        "operator": operator, "reverse": reverse,
    }
    return name, source, oracle

def cases():
    for kind, form, op, swap, high in product(
        ("local", "registered"), ("direct", "named"), ("==", "<"), (False, True), (False, True),
    ):
        yield program(kind, form, op, swap, high)
    for high in (False, True):
        yield program("local", "direct", "<", False, high, literal=True)
        yield program("local", "named", "==", True, high, literal=True)

def mixed_producers():
    _, registered, _ = program("registered", "named", "<", True, True)
    return registered.replace("let bound = usize(112);", "let bound = usize(octet);")


def refusals():
    _, local, _ = program("local", "direct", "==", False, False)
    backend = (0, 4, "V1-BACKEND-UNSUPPORTED")
    return {
        "wrong-nominal": (
            local.replace("let bound = usize(39);", "let bound = u8(39);")
                 .replace("bound < reading", "reading == bound"), (3, 3, "USIZE-EQUALITY-TYPE")),
        "wrong-condition": (
            local.replace("if (reading == bound)", "if (reading)"), (3, 3, "P13-IF-CONDITION")),
        "inline-conversion": (
            local.replace("if (reading == bound)", "if (usize(octet) == bound)"),
            (3, 3, "USIZE-EQUALITY-SOURCE-PRECISION")),
        "extra-prefix-call": (
            local.replace("let octet = sample_octet(load);",
                          "let extra = sample_octet(load);\n    let octet = sample_octet(load);"), backend),
        "second-local-widening": (
            local.replace("let reading = quantity;", "let spare = usize(octet);\n    let reading = quantity;"), backend),
        "arm-call": (
            local.replace("{ u8(71) } else", "{ sample_octet(load) } else"), backend),
        "extra-arm-bindings": (
            local.replace("{ u8(71) } else",
                          "{ let first = u8(71); let second = first; u8(71) } else"), backend),
    }
