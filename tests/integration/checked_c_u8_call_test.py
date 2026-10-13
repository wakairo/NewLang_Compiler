"""Ordinary one-u8 Copy call: source-owned evidence and native forwarding.

Observers inspect post-call runtime carriers in a TEST COPY of generated C.
They never supply arguments, replace results, or create guest I/O/exit ABI.
"""
from pathlib import Path
import hashlib
import json
import re
import sys
import tempfile
from checked_c_v0_test import run


def execute(root: Path, cc: str, name: str, text: str, success: bool) -> list[int]:
    c = root / f"{name}.c"
    exe = root / name
    c.write_text(text, encoding="utf-8")
    statuses = []
    for optimization in ("-O0", "-O2"):
        built = run([cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                     optimization, str(c), "-o", str(exe)])
        if built.returncode:
            raise SystemExit(f"{name}: C17 build failed {built.stderr}")
        result = run([str(exe)])
        statuses.append(result.returncode)
        if success:
            if result.returncode or result.stdout or result.stderr:
                raise SystemExit(f"{name}: native observation failed {result}")
        elif result.returncode == 0:
            raise SystemExit(f"{name}: independent observer missed C mutant")
    return statuses


def observe(text: str, values: list[int]) -> str:
    """Only read already generated call-result and receiving-local carriers."""
    pattern = r"const uint8_t (nl_call_\d+) = nl_fn_\d+\([^\n]*\);"
    calls = list(re.finditer(pattern, text))
    if len(calls) != len(values):
        raise SystemExit("missing runtime callee calls/stable result carriers")
    expected = {m[1]: value for m, value in zip(calls, values)}
    result = text.replace("#include <stdint.h>", "#include <stdint.h>\n#include <stdlib.h>")
    result = re.sub(pattern, lambda m: m[0] + f"\n    if ({m[1]} != {expected[m[1]]}) abort();", result)
    result = re.sub(r"const uint8_t (nl_local_\d+) = (nl_call_\d+);",
                    lambda m: m[0] + f"\n    if ({m[1]} != {expected[m[2]]}) abort();", result)
    return result


def main() -> None:
    compiler, probe, cc = sys.argv[1:]
    cases = {
        "echo7": ("fn echo(x:u8)->u8{x} fn main()->unit{echo(u8(7));unit}\n", [7]),
        "echo9": ("fn echo(x:u8)->u8{x} fn main()->unit{echo(u8(9));unit}\n", [9]),
        "two": ("fn echo(x:u8)->u8{x} fn main()->unit{echo(u8(7));echo(u8(9));unit}\n", [7, 9]),
        "forward": ("fn echo(x:u8)->u8{x} fn main()->unit{let a=echo(u8(9));let b=echo(a);b;unit}\n", [9, 9]),
        "renamed_local": ("fn relay(arg:u8)->u8{let saved=arg;saved} fn main()->unit{relay(u8(007));relay(u8(9));unit}\n", [7, 9]),
        "explicit_return": ("fn relay(arg:u8)->u8{return arg;} fn main()->unit{let x=relay(u8(255));x;unit}\n", [255]),
    }
    negatives = {
        "range": ("fn echo(x:u8)->u8{x} fn main()->unit{echo(u8(256));unit}", 3, "V1-U8-LITERAL-RANGE"),
        "wrong_argument": ("fn echo(x:u8)->u8{x} fn main()->unit{echo(unit);unit}", 3, "P3-ARGUMENT-RESULTS"),
        "wrong_pointer_type": ("fn echo(x:u8)->u8{x} fn main()->unit{let x=u8(7);let p=loan_read(x){|r|ptr_from_ref(r)};echo(p);unit}", 3, "P3-TYPE-MISMATCH"),
        "wrong_arity": ("fn echo(x:u8)->u8{x} fn main()->unit{echo();unit}", 3, "P3-ARITY"),
        "wrong_return": ("fn echo(x:u8)->u8{unit} fn main()->unit{unit}", 3, "P8-BODY-RESULT"),
        "two_arguments": ("fn echo(x:u8,y:u8)->u8{x} fn main()->unit{echo(u8(7),u8(9));unit}", 4, "V1-BACKEND-UNSUPPORTED"),
        "richer_body": ("fn echo(x:u8)->u8{{x}} fn main()->unit{echo(u8(7));unit}", 4, "V1-BACKEND-UNSUPPORTED"),
        "ref_argument": ("fn touch(x:ref<read,u8>)->unit{unit} fn main()->unit{let x=u8(7);loan_read(x){|r|touch(r);unit};unit}", 4, "V1-BACKEND-UNSUPPORTED"),
    }
    with tempfile.TemporaryDirectory(prefix="newlang-u8-call-") as temp:
        root = Path(temp)
        outputs = {}
        for name, (source, values) in cases.items():
            path = root / f"{name}.nl"
            path.write_text(source, encoding="utf-8")
            first, second = run([compiler, str(path)]), run([compiler, str(path)])
            if first.returncode or first.stderr or first.stdout != second.stdout or second.returncode:
                raise SystemExit(f"{name}: nondeterministic/rejected source {first} {second}")
            owned = run([probe, str(path)])
            if owned.returncode or owned.stderr or f"OWNED {len(values)}" not in owned.stdout:
                raise SystemExit(f"{name}: source-issued owned proof failed {owned}")
            lines = [line.split() for line in owned.stdout.splitlines() if line.startswith("CALL ")]
            if [int(line[-1]) for line in lines] != values:
                raise SystemExit(f"{name}: wrong checked return packages {lines}")
            if len(lines) > 1 and len({line[3] for line in lines}) != len(lines):
                raise SystemExit("formal binding reused across invocations")
            if not re.search(r"static uint8_t nl_fn_\d+\(uint8_t nl_local_\d+\)", first.stdout):
                raise SystemExit("missing exact typed u8 prototype/definition")
            execute(root, cc, name, first.stdout, True)
            observed = observe(first.stdout, values)
            execute(root, cc, name + "_observed", observed, True)
            outputs[name] = observed
            if name == "forward":
                receiver = re.search(r"const uint8_t (nl_local_\d+) = nl_call_\d+;", first.stdout)
                if receiver is None or not re.search(r"nl_fn_\d+\(" + receiver[1] + r"\)", first.stdout):
                    raise SystemExit("caller Copy result not forwarded as runtime argument")
            print(json.dumps({"case": name, "source_sha256": hashlib.sha256(source.encode()).hexdigest(),
                              "cli": 0, "c_bytes": len(first.stdout.encode()), "owned": owned.stdout.splitlines(),
                              "native": {"plain": "PASS", "observed": values, "optimization": ["O0", "O2"]}}))
        # Actual compiler output, changed callee only; observer expectations stay.
        for name, replacement in (("wrong_result", "0"), ("unused_argument", "7")):
            mutant, count = re.subn(r"return nl_local_\d+;", f"return {replacement};", outputs["echo9"])
            if count != 1:
                raise SystemExit("C mutant did not change exactly one callee return")
            statuses = execute(root, cc, name, mutant, False)
            print(json.dumps({"mutant": name, "observation": "REJECT", "native_exit": statuses, "optimization": ["O0", "O2"]}))
        for name, (source, status, code) in negatives.items():
            path = root / f"{name}.nl"
            path.write_text(source + "\n", encoding="utf-8")
            result = run([compiler, str(path)])
            if result.returncode != status or result.stdout or code not in result.stderr:
                raise SystemExit(f"{name}: fail-closed boundary changed {result}")
            print(json.dumps({"case": name, "cli": status, "c_bytes": 0, "diagnostic": code}))
    print("one-u8 source-owned/runtime formal and Copy return PASS; controls reject")


if __name__ == "__main__":
    main()
