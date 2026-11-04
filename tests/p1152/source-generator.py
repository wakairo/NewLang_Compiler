from pathlib import Path
import json

root = Path("/tmp/p1152-input-v1")
(root / "sources").mkdir(parents=True, exist_ok=True)
cases = [
    dict(name="01-add-less-true", nominal="CompassCell", fields=["mark", "twin"],
         values=[66, 192], index=0, start=2, limit=12, step=4, direction="add",
         trace=[2, 6, 10, 14], result=14, operator="<", compare=15, truth=True,
         incoming=[137, 23]),
    dict(name="02-zero-less-false", nominal="FlagCell", fields=["steady", "choice"],
         values=[41, 91], index=1, start=12, limit=12, step=4, direction="add",
         trace=[12], result=12, operator="<", compare=12, truth=False,
         incoming=[58, 209]),
    dict(name="03-sub-equal-true", nominal="ValueCell", fields=["neighbor", "target"],
         values=[148, 76], index=1, start=23, limit=8, step=6, direction="sub",
         trace=[23, 17, 11, 5], result=5, operator="==", compare=5, truth=True,
         incoming=[184, 39]),
    dict(name="04-one-equal-false", nominal="ToneCell", fields=["tone", "trace"],
         values=[52, 164], index=0, start=7, limit=10, step=4, direction="add",
         trace=[7, 11], result=11, operator="==", compare=10, truth=False,
         incoming=[97, 218]),
]


def declaration(e):
    return "struct " + e["nominal"] + " { " + e["fields"][0] + ": u8, " + e["fields"][1] + ": u8 }\n"


def construction(e):
    return ("    let original = " + e["nominal"] + "{" + e["fields"][0] +
            ": u8(" + str(e["values"][0]) + "), " + e["fields"][1] +
            ": u8(" + str(e["values"][1]) + ")};\n")


def loop(e):
    operation = "+" if e["direction"] == "add" else "-"
    cont = "            continue(cursor " + operation + " usize(" + str(e["step"]) + "));\n"
    brk = "            break cursor;\n"
    yes, no = (cont, brk) if operation == "+" else (brk, cont)
    return ("    let result = loop (cursor = usize(" + str(e["start"]) + ")) {\n"
            "        if (cursor < usize(" + str(e["limit"]) + ")) {\n" + yes +
            "        } else {\n" + no + "        }\n    };\n")


def choice(e, condition):
    field = e["fields"][e["index"]]
    return ("    let decision = " + condition + ";\n"
            "    let old = if (decision) {\n"
            "        loan_write(original@" + field + ") { |yes| replace(yes, u8(" + str(e["incoming"][0]) + ")) }\n"
            "    } else {\n"
            "        loan_write(original@" + field + ") { |no| replace(no, u8(" + str(e["incoming"][1]) + ")) }\n"
            "    };\n")


def reads(e):
    return ("    let updated = original@" + e["fields"][e["index"]] + ";\n"
            "    let sibling = original@" + e["fields"][1 - e["index"]] + ";\n"
            "    let old_copy = old;\n    return unit;\n}\n")


def source(e):
    condition = "copied " + e["operator"] + " usize(" + str(e["compare"]) + ")"
    return (declaration(e) + "fn main() -> unit {\n" + construction(e) + loop(e) +
            "    let copied = result;\n" + choice(e, condition) + reads(e))


positive = {}
for e in cases:
    (root / "sources" / (e["name"] + ".nl")).write_text(source(e))
    positive[e["name"]] = {
        **e, "old": e["values"][e["index"]],
        "updated": e["incoming"][0 if e["truth"] else 1],
        "sibling": e["values"][1-e["index"]],
        "check": 0, "emit": 0, "condition_evaluations": 1,
        "selected_arm": 0 if e["truth"] else 1, "store_count": 1,
        "identity": "same pre-loop original root/child/incarnation and C field address",
        "static_condition": "dependency-free Unknown Bool from Unknown loop-result Copy",
    }

e = cases[0]
controls = {
    "80-known-condition": declaration(e) + "fn main() -> unit {\n" + construction(e) +
        "    let known = usize(3);\n" + choice(e, "known < usize(9)") + reads(e),
    "81-existing-loop-consumer": declaration(e) + "fn main() -> unit {\n" + construction(e) + loop(e) +
        "    let copied = result;\n"
        "    let old = loan_write(original@mark) { |writer| replace(writer, u8(137)) };\n" + reads(e),
    "82-different-child-control": source(e).replace(
        "loan_write(original@mark) { |no|", "loan_write(original@twin) { |no|"),
}
for name, text in controls.items():
    (root / "sources" / (name + ".nl")).write_text(text)
base = source(e)
negative = {
    "90-overflow": base.replace("cursor + usize(4)", "cursor + usize(18446744073709551615)"),
    "91-read-mode": base.replace("loan_write(original@mark)", "loan_read(original@mark)"),
    "92-ending-reference": base.replace("replace(yes, u8(137))", "yes").replace("replace(no, u8(23))", "no"),
    "93-incoming-type": base.replace("replace(yes, u8(137))", "replace(yes, copied)").replace("replace(no, u8(23))", "replace(no, copied)"),
}
for name, text in negative.items():
    (root / "sources" / (name + ".nl")).write_text(text)

oracle = {
    "authority": "C228 / local fixed PR1149 d66fdf7; normative mainc120dd1 / Draft17.34",
    "positive": positive,
    "controls": {
        "80-known-condition": {"check": 0, "emit": 0},
        "81-existing-loop-consumer": {"check": 0, "emit": 0},
        "82-different-child-control": {"check": 0, "emit": 4,
            "classification": "legal different-child join, outside the same-child IF backend family"},
    },
    "negative": {name: {"check": 3, "emit": 3, "named": 3}
                 for name in negative},
    "prediction": "Composite CHECK and existing owned loop/IF may pass; fixed backend known-Bool consumer rejects before emission. Preserve an earlier actual refusal or unexpected acceptance as FIRST.",
    "native": {"compilers": ["GCC13", "Clang23"], "opts": ["O0", "O2"],
               "versions": ["raw unchanged C", "read-only passive observations"], "exit": 0},
    "scope": "One pre-existing two-u8 original, one guarded usize slot, one C/B, root literal comparison and one same-child two-normal IF; no wrapper/new issuer/bridge/source law.",
}
(root / "oracle.json").write_text(json.dumps(oracle, indent=2) + "\n")
print("Prepared complete independent sources: four positive, three controls, four negative; execution0")
