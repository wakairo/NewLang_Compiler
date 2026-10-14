"""P421 fixed-main causal field-old -> ordinary call -> distinct field-new."""
from pathlib import Path
import hashlib
import json
import re
import sys
import tempfile
from checked_c_u8_call_test import execute
from checked_c_v0_test import run


def oracle(rows: list[dict], a_old: int, a_new: int, b_old: int,
           index: int) -> tuple[list[dict], list[dict], list[dict]]:
    roots = [r for r in rows if r["kind"] == "root"]
    changes = [r for r in rows if r["kind"] == "change"]
    bindings = [r for r in rows if r["kind"] == "binding"]
    calls = [r for r in rows if r["kind"] == "call"]
    reads = [r for r in rows if r["kind"] == "read"]
    initial_a = [a_old, 1] if index == 0 else [1, a_old]
    initial_b = [9, b_old] if index == 0 else [b_old, 9]
    if (len(roots) != 2 or len(changes) != 2 or len(calls) != 1 or
            len(reads) != 2 or len(bindings) != 5 or
            roots[0]["place"] == roots[1]["place"] or
            roots[0]["initial"] != initial_a or roots[1]["initial"] != initial_b or
            [r["value"] for r in bindings] != [a_old, a_old, b_old, a_new, a_old] or
            [(r["old"], r["new"]) for r in changes] != [(a_old, a_new), (b_old, a_old)] or
            (calls[0]["actual"], calls[0]["returned"]) != (a_old, a_old) or
            calls[0]["actual_symbol"] != bindings[0]["symbol"] or
            calls[0]["return_package"] != bindings[1]["package"]):
        raise SystemExit(f"independent causal oracle mismatch {rows}")
    for k in range(2):
        change, root, read = changes[k], roots[k], reads[k]
        if (change["source"] != root["symbol"] or change["parent"] != root["place"] or
                change["parent_incarnation"] != root["incarnation"] or
                change["index"] != (index if k == 0 else 1 - index) or
                change["old_package"] != bindings[0 if k == 0 else 2]["package"] or
                change["parent_post_fact"] != read["parent_fact"] or
                change["child_post_fact"] != read["child_fact"] or
                change["parent"] != read["parent"] or change["child"] != read["child"]):
            raise SystemExit("source root/fixed projection/post-currentness mismatch")
    return roots, changes, bindings


def observe(text: str, roots: list[dict], changes: list[dict],
            bindings: list[dict]) -> str:
    # Interpret only existing carriers. Values were checked against a source
    # oracle, not derived from the emitter under test. No guest data is supplied.
    result = text.replace("#include <stdint.h>",
                          "#include <stdint.h>\n#include <stdlib.h>\nstatic unsigned int nl_test_calls;")
    for root in roots:
        symbol = root["symbol"]
        pattern = rf"nl_type_\d+ nl_local_{symbol} = [^\n]+;"
        values = root["initial"]
        result, count = re.subn(pattern, lambda m: m[0] +
                               f"\n    if (nl_local_{symbol}.f0 != {values[0]} || nl_local_{symbol}.f1 != {values[1]}) abort();", result)
        if count != 1:
            raise SystemExit("missing original Pair carrier")
    for change in changes:
        node = change["node"]
        load = re.search(rf"const uint8_t nl_replace_old_{node} = \*(nl_local_\d+);", text)
        if load is None:
            raise SystemExit("old result is not a real memory load")
        result = result.replace(load[0], load[0] +
                                f"\n        if (nl_replace_old_{node} != {change['old']}) abort();")
        root = change["source"]
        index = change["index"]
        sibling = roots[0 if root == roots[0]["symbol"] else 1]["initial"][1 - index]
        result, count = re.subn(r"\*" + load[1] + r" = [^\n]+;",
                               lambda m: m[0] + f"\n        if (nl_local_{root}.f{index} != {change['new']} || nl_local_{root}.f{1-index} != {sibling}) abort();", result)
        if count != 1:
            raise SystemExit("missing real field store")
    for binding in bindings:
        pattern = rf"const uint8_t nl_local_{binding['symbol']} = [^\n]+;"
        result, count = re.subn(pattern, lambda m: m[0] +
                               f"\n    if (nl_local_{binding['symbol']} != {binding['value']}) abort();", result)
        if count != 1:
            raise SystemExit("missing caller Copy receiver")
    call = re.search(r"const uint8_t (nl_call_\d+) = (nl_fn_\d+)\(nl_local_" + str(bindings[0]["symbol"]) + r"\);", result)
    formal = re.search(r"static uint8_t " + call[2] + r"\(uint8_t (nl_local_\d+)\)\n\{\n", result) if call else None
    if formal is None or f"return {formal[1]};" not in result:
        raise SystemExit("missing actual source-old/formal/returned C call")
    result = result.replace(formal[0], formal[0] +
                            f"    ++nl_test_calls;\n    if ({formal[1]} != {bindings[0]['value']}) abort();\n")
    result = result.replace(call[0], call[0] + f"\n    if ({call[1]} != {bindings[1]['value']}) abort();")
    if not re.search(r"\*nl_local_\d+ = nl_local_" + str(bindings[1]["symbol"]) + r";", text):
        raise SystemExit("transported return not used as actual B store")
    final = []
    for root, change in zip(roots, changes):
        values = root["initial"].copy()
        values[change["index"]] = change["new"]
        final.append(f"nl_local_{root['symbol']}.f0 != {values[0]} || nl_local_{root['symbol']}.f1 != {values[1]}")
    result = result.replace("    (void)0;\n}", "    if (" + " || ".join(final) + ") abort();\n    (void)0;\n}")
    return result.replace("    return 0;\n}", "    if (nl_test_calls != 1) abort();\n    return 0;\n}")


def main() -> None:
    compiler, probe, call_probe, cc, evidence = sys.argv[1:]
    cases = {"primary": (7, 11, 2, 0), "variation": (80, 123, 4, 1)}
    with tempfile.TemporaryDirectory(prefix="newlang-p421-") as temp:
        root = Path(temp)
        for name, expected in cases.items():
            path = Path(evidence) / f"{name}.nl"
            source = path.read_bytes()
            checked = run([probe, str(path)])
            if checked.returncode or checked.stderr:
                raise SystemExit(f"source-owned probe failed {checked}")
            rows = [json.loads(line) for line in checked.stdout.splitlines()]
            if rows[-1] != {"kind": "status", "parse": 0, "register": 0, "check": 0, "owned": True, "roots": 2, "changes": 2, "calls": 1, "reads": 2}:
                raise SystemExit("SOURCE/CHECK/OWNED status mismatch")
            roots, changes, bindings = oracle(rows, *expected)
            authenticated = run([call_probe, str(path)])
            if authenticated.returncode or authenticated.stderr or "OWNED 1" not in authenticated.stdout:
                raise SystemExit("existing registered-call authentication/controls failed")
            emitted, repeated = run([compiler, str(path)]), run([compiler, str(path)])
            if emitted.returncode or emitted.stderr or repeated.returncode or emitted.stdout != repeated.stdout:
                raise SystemExit(f"actual fixed-main emission failed {emitted}")
            execute(root, cc, name, emitted.stdout, True)
            observed = observe(emitted.stdout, roots, changes, bindings)
            execute(root, cc, name + "_observed", observed, True)
            wrong_return, count = re.subn(r"return nl_local_\d+;", "return 0;", observed)
            if count != 1:
                raise SystemExit("wrong return mutant not unique")
            second_ref = re.search(r"const uint8_t nl_replace_old_" + str(changes[1]["node"]) + r" = \*(nl_local_\d+);", observed)
            wrong_store, count = re.subn(r"\*" + second_ref[1] + r" = nl_local_" + str(bindings[1]["symbol"]) + r";", "*" + second_ref[1] + " = 0;", observed)
            if count != 1:
                raise SystemExit("wrong B store mutant not unique")
            mutants = {"wrong_return": execute(root, cc, name + "_wrong_return", wrong_return, False),
                       "ignore_transport": execute(root, cc, name + "_wrong_store", wrong_store, False)}
            print(json.dumps({"case": name, "source_sha256": hashlib.sha256(source).hexdigest(),
                              "cli": 0, "c_bytes": len(emitted.stdout.encode()), "owned": rows,
                              "call_authentication": authenticated.stdout.splitlines(),
                              "native": "plain/read-only observed O0/O2 PASS", "mutants": mutants}))
        primary = (Path(evidence) / "primary.nl").read_text()
        negatives = {
            "read_field_profile": (primary.replace("loan_write(a@left)", "loan_read(a@left)"), "LOCAL-LOAN-PROFILE"),
            "wrong_store_type": (primary.replace("replace(w,transported)", "replace(w,unit)"), "P3-TYPE-MISMATCH"),
            "wrong_return": (primary.replace("fn relay(x:u8)->u8{x}", "fn relay(x:u8)->u8{unit}"), "P8-BODY-RESULT"),
            "scope_escape": (primary.replace("replace(w,u8(11))", "w"), "P8-EXIT-DEPENDENCY"),
        }
        for name, (source, code) in negatives.items():
            path = root / f"{name}.nl"
            path.write_text(source)
            result = run([compiler, str(path)])
            if result.returncode != 3 or result.stdout or code not in result.stderr:
                raise SystemExit(f"negative lost existing refusal {result}")
            print(json.dumps({"negative": name, "cli": 3, "c_bytes": 0, "diagnostic": result.stderr.strip()}))
    print("P421 EXISTING-MAIN causal field+call native PASS; test-only evidence")


if __name__ == "__main__":
    main()
