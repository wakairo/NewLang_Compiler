"""One relay+leaf frame composition; observation reads real C carriers only."""
from pathlib import Path
import hashlib
import json
import re
import sys
import tempfile
from checked_c_u8_call_test import execute
from checked_c_v0_test import run


def observe(text: str, expected: int) -> tuple[str, str]:
    definitions = list(re.finditer(
        r"static uint8_t (nl_fn_\d+)\(uint8_t (nl_local_\d+)\)\n\{\n(.*?)\n\}",
        text, re.S))
    if len(definitions) != 2:
        raise SystemExit("missing exactly two real scalar C definitions")
    leaf = next((m for m in definitions if "nl_fn_" not in m[3]), None)
    relay = next((m for m in definitions if "nl_fn_" in m[3]), None)
    if leaf is None or relay is None or not re.search(
            r"const uint8_t nl_call_\d+ = " + leaf[1] + r"\(" + relay[2] + r"\);", relay[3]):
        raise SystemExit("no genuine nested runtime formal forwarding")
    if not re.search(r"return " + leaf[2] + r";", leaf[3]) or not re.search(
            r"return nl_call_\d+;", relay[3]):
        raise SystemExit("return carriers not connected to real callee results")
    result = text.replace("#include <stdint.h>",
                          "#include <stdint.h>\n#include <stdlib.h>\n"
                          "static unsigned int nl_test_leaf_calls, nl_test_relay_calls;")
    for m, counter in ((leaf, "leaf"), (relay, "relay")):
        marker = f"static uint8_t {m[1]}(uint8_t {m[2]})\n{{\n"
        result = result.replace(marker, marker +
                                f"    if ({m[2]} != {expected}) abort();\n"
                                f"    ++nl_test_{counter}_calls;\n")
    # Every nested result, outer result and caller receiver is read in situ.
    pattern = r"const uint8_t (nl_call_\d+) = nl_fn_\d+\([^\n]*\);"
    result, calls = re.subn(pattern, lambda m: m[0] +
                           f"\n    if ({m[1]} != {expected}) abort();", result)
    result, receivers = re.subn(r"const uint8_t (nl_local_\d+) = nl_call_\d+;",
                               lambda m: m[0] + f"\n    if ({m[1]} != {expected}) abort();", result)
    if calls != 3 or receivers != 2:
        raise SystemExit("wrong nested call/receiver structure")
    result = result.replace("    return 0;\n}",
                            "    if (nl_test_leaf_calls != 2 || nl_test_relay_calls != 2) abort();\n"
                            "    return 0;\n}")
    return result, leaf[2]


def main() -> None:
    compiler, probe, cc, evidence = sys.argv[1:]
    sources = {"nested9": 9, "nested255": 255, "renamed": 9}
    negatives = {
        "second_level": ("fn echo(x:u8)->u8{x} fn relay(y:u8)->u8{echo(y)} fn third(z:u8)->u8{relay(z)} fn main()->unit{third(u8(9));unit}", 4, "V1-BACKEND-UNSUPPORTED"),
        "recursion": ("fn echo(x:u8)->u8{echo(x)} fn main()->unit{echo(u8(9));unit}", 3, "P11-RECURSIVE-ANALYSIS-PRECISION"),
        "two_formals": ("fn echo(x:u8,z:u8)->u8{x} fn relay(y:u8)->u8{echo(y,y)} fn main()->unit{relay(u8(9));unit}", 4, "V1-BACKEND-UNSUPPORTED"),
        "ref": ("fn echo(x:ref<read,u8>)->unit{unit} fn main()->unit{let x=u8(9);loan_read(x){|r|echo(r);unit};unit}", 4, "V1-BACKEND-UNSUPPORTED"),
        "noncopy": ("fn echo(x:LifetimeDomain)->LifetimeDomain{x} fn main()->unit{let x=u8(9);let d=lifetime_domain(x);let out=echo(d);unit}", 3, "P8-SIGNATURE-PRECISION"),
        "range": ("fn echo(x:u8)->u8{x} fn relay(y:u8)->u8{echo(y)} fn main()->unit{relay(u8(256));unit}", 3, "V1-U8-LITERAL-RANGE"),
        "wrong_argument": ("fn echo(x:u8)->u8{x} fn relay(y:u8)->u8{echo(unit)} fn main()->unit{relay(u8(9));unit}", 3, "P3-ARGUMENT-RESULTS"),
        "wrong_return": ("fn echo(x:u8)->u8{unit} fn relay(y:u8)->u8{echo(y)} fn main()->unit{relay(u8(9));unit}", 3, "P8-BODY-RESULT"),
        "nested_actual": ("fn echo(x:u8)->u8{x} fn relay(y:u8)->u8{echo(echo(y))} fn main()->unit{relay(u8(9));unit}", 4, "V1-BACKEND-UNSUPPORTED"),
    }
    with tempfile.TemporaryDirectory(prefix="newlang-nested-u8-") as temp:
        root = Path(temp)
        for name, value in sources.items():
            path = Path(evidence) / f"{name}.nl"
            source = path.read_bytes()
            emitted, repeated = run([compiler, str(path)]), run([compiler, str(path)])
            if emitted.returncode or emitted.stderr or emitted.stdout != repeated.stdout or repeated.returncode:
                raise SystemExit(f"{name}: rejected/nondeterministic {emitted}")
            checked = run([probe, str(path), "--controls"])
            if checked.returncode or checked.stderr or "frames=4 emitter=1" not in checked.stdout or "CONTROLS 24 REJECT C0" not in checked.stdout:
                raise SystemExit(f"{name}: owned frame controls failed {checked}")
            frames = [dict(re.findall(r"(\w+)=(\d+)", line)) for line in checked.stdout.splitlines() if line.startswith("FRAME ")]
            if len({f["formal"] for f in frames}) != 4 or any(
                    f["actual"] != f["bound"] or int(f["scalar"]) != value for f in frames):
                raise SystemExit("bad per-invocation actual/formal identity")
            execute(root, cc, name + "_plain", emitted.stdout, True)
            observed, leaf_return = observe(emitted.stdout, value)
            execute(root, cc, name + "_observed", observed, True)
            # Two independently broken runtime returns; keep original observers.
            mutants = {
                "inner_return": observed.replace(f"return {leaf_return};", "return 0;"),
                "outer_return": re.sub(r"return nl_call_\d+;", "return 0;", observed),
            }
            for mutant_name, mutant in mutants.items():
                if mutant == observed:
                    raise SystemExit("mutant did not change return")
                statuses = execute(root, cc, name + mutant_name, mutant, False)
                print(json.dumps({"case": name, "mutant": mutant_name, "native_exits": statuses}))
            print(json.dumps({"case": name, "source_sha256": hashlib.sha256(source).hexdigest(),
                              "cli": 0, "c_bytes": len(emitted.stdout.encode()),
                              "owned": checked.stdout.splitlines(), "native": "plain + read-only frame observer O0/O2 PASS"}))
        for name, (source, status, code) in negatives.items():
            path = root / f"{name}.nl"
            path.write_text(source + "\n")
            result = run([compiler, str(path)])
            if result.returncode != status or result.stdout or code not in result.stderr:
                raise SystemExit(f"{name}: fail-closed mismatch {result}")
            print(json.dumps({"negative": name, "cli": status, "c_bytes": 0,
                              "source_sha256": hashlib.sha256(path.read_bytes()).hexdigest(), "diagnostic": result.stderr.strip()}))
    print("one-level nested u8 runtime frames PASS BOUNDED; mutated evidence and returns rejected")


if __name__ == "__main__":
    main()
