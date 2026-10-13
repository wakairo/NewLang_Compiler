"""G0: real owned expectations, unchanged emitted bodies/calls, native results.

The observer adds only entry counters to copied C bodies. It never supplies a
return value or replaces a generated call. The pristine C is also executed.
"""
from collections import Counter
from pathlib import Path
import hashlib
import json
import os
import re
import sys
import tempfile
from checked_c_v0_test import negative, run


def require_ok(result, label):
    if result.returncode != 0 or result.stderr:
        raise SystemExit(f"{label}: {result.returncode}\n{result.stdout}{result.stderr}")


def observe(code, calls):
    functions = sorted({c["function"] for c in calls})
    definitions = re.findall(r"(?m)^static uint8_t nl_fn_(\d+)\(void\)\n\{", code)
    if {int(f) for f in definitions} != set(functions):
        raise SystemExit("typed emitted callee identities differ from checked calls")
    counts = Counter(c["function"] for c in calls)
    expected = {c["function"]: c["value"] for c in calls}
    prefix = "#include <stdio.h>\nstatic unsigned g0_counts[%d];\n" % len(functions)
    prefix += "static void g0_enter(unsigned id) {\n"
    for index, function in enumerate(functions):
        prefix += f"if(id=={function}) ++g0_counts[{index}];\n"
    prefix += "}\n#define main nl_guest_main\n"
    # Only insert a counter at entry. Callee return computations and every
    # compiler-generated call remain exactly the emitted tokens.
    instrumented = re.sub(
        r"(?m)^(static uint8_t nl_fn_(\d+)\(void\)\n\{)",
        lambda m: m[1] + f"\n    g0_enter({m[2]});", code)
    suffix = "\n#undef main\nint main(void) {\nif(nl_guest_main()!=0) return 79;\n"
    for index, function in enumerate(functions):
        suffix += f"const unsigned calls_{index}=g0_counts[{index}];\n"
        suffix += f"if(calls_{index}!={counts[function]}) return 78;\n"
        suffix += f"const unsigned value_{index}=nl_fn_{function}();\n"
        suffix += (f'printf("{{\\"function\\":{function},\\"guest_calls\\":%u,'
                   f'\\"value\\":%u}}\\n",calls_{index},value_{index});\n')
        suffix += f"if(value_{index}!={expected[function]}) return 77;\n"
    return prefix + instrumented + suffix + "return 0;\n}\n"


def main():
    compiler, probe, cc, sanitizer = sys.argv[1:]
    flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O0"]
    if sanitizer != "none":
        flags += ["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all",
                  "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    report = {"sanitizer": sanitizer, "positive": [], "negative": [], "mutants": []}
    cases = {
        "seven": "fn scalar()->u8{u8(7)}\nfn main()->unit{scalar();unit}\n",
        "nine": "fn scalar()->u8{u8(9)}\nfn main()->unit{scalar();unit}\n",
        "twins": "fn amber()->u8{u8(0)}fn teal()->u8{u8(255)}"
                 "fn main()->unit{teal();amber();teal();unit}\n",
        "local": "fn result()->u8{let x=u8(9);x}fn main()->unit{result();unit}\n",
        "return": "fn result()->u8{return u8(7);}fn main()->unit{result();unit}\n",
    }
    with tempfile.TemporaryDirectory(prefix="newlang-g0-") as directory:
        root = Path(directory)

        def native(name, code, expected_exit=0):
            path, exe = root / (name + ".c"), root / name
            path.write_text(code)
            built = run([cc] + flags + [str(path), "-o", str(exe)])
            require_ok(built, name + " C17 build")
            result = run([str(exe)])
            if result.returncode != expected_exit or result.stderr:
                raise SystemExit(f"{name} native: {result}")
            return result

        for name, source in cases.items():
            path = root / (name + ".nl")
            path.write_text(source)
            owned = run([probe, str(path)])
            require_ok(owned, name + " checked probe")
            rows = [json.loads(row) for row in owned.stdout.splitlines()]
            if rows[0] != {"parse": 0, "register": 0, "check": 0, "owned": True}:
                raise SystemExit("not an actual successful source-owned artifact")
            calls = rows[1:]
            if not calls:
                raise SystemExit("no checked u8 call")
            emitted = run([compiler, str(path)])
            require_ok(emitted, name + " source emission")
            repeated = run([compiler, str(path)])
            require_ok(repeated, name + " repeated emission")
            if emitted.stdout != repeated.stdout:
                raise SystemExit("nondeterministic C")
            pristine = native(name + "-pristine", emitted.stdout)
            if pristine.stdout:
                raise SystemExit("unexpected pristine native output")
            observed = native(name + "-observed", observe(emitted.stdout, calls))
            observations = [json.loads(row) for row in observed.stdout.splitlines()]
            expected = {c["function"]: c["value"] for c in calls}
            if {o["function"]: o["value"] for o in observations} != expected:
                raise SystemExit("actual callee results differ from owned expectations")
            report["positive"].append({"name": name,
                "source": source,
                "source_sha256": hashlib.sha256(source.encode()).hexdigest(),
                "c_sha256": hashlib.sha256(emitted.stdout.encode()).hexdigest(),
                "checked": rows, "native": observations,
                "compiler_exit": 0, "pristine_exit": 0, "observer_exit": 0})
            if name == "seven":
                wrong = emitted.stdout.replace("return 7;", "return 8;", 1)
                if wrong == emitted.stdout:
                    raise SystemExit("result mutant not made")
                native("wrong-result", observe(wrong, calls), 77)
                function = calls[0]["function"]
                missed = emitted.stdout.replace(f"    nl_fn_{function}();\n", "", 1)
                if missed == emitted.stdout:
                    raise SystemExit("missed-call mutant not made")
                native("missed-call", observe(missed, calls), 78)
                report["mutants"] += [{"name": "wrong-result", "exit": 77},
                                      {"name": "missed-call", "exit": 78}]

        rejects = [
            ("range", "fn scalar()->u8{u8(256)}fn main()->unit{scalar();unit}",
             "V1-U8-LITERAL-RANGE"),
            ("type", "fn scalar()->u8{return unit;}fn main()->unit{scalar();unit}",
             "P9-RETURN-TYPE"),
            ("unknown", "fn main()->unit{missing();unit}", "P3-UNKNOWN-CALLEE"),
        ]
        for name, source, code in rejects:
            negative(root, compiler, name, source, code)
            path = root / (name + ".nl")
            rejected = run([compiler, str(path)])
            if rejected.returncode != 3:
                raise SystemExit("semantic rejection phase lost")
            report["negative"].append({"name": name, "exit": 3,
                                       "diagnostic": code, "c_bytes": 0, "source": source,
                                       "stderr": rejected.stderr.replace(str(root), "<fixture-dir>")})
        outside = {
            "parameter": "fn identity(x:u8)->u8{x}fn main()->unit{identity(u8(7));unit}",
            "receiving": "fn scalar()->u8{u8(7)}fn main()->unit{let x=scalar();x;unit}",
            "entry": "fn main()->u8{u8(7)}",
        }
        for name, source in outside.items():
            path = root / (name + ".nl")
            path.write_text(source)
            rejected = run([compiler, str(path)])
            if (rejected.returncode != 4 or rejected.stdout or
                    "V1-BACKEND-UNSUPPORTED" not in rejected.stderr):
                raise SystemExit(f"out-of-profile boundary lost: {rejected}")
            report["negative"].append({"name": name, "exit": 4,
                "diagnostic": "V1-BACKEND-UNSUPPORTED", "c_bytes": 0,
                "source": source, "stderr": rejected.stderr.replace(str(root), "<fixture-dir>")})
    if destination := os.environ.get("NEWLANG_G0_EVIDENCE"):
        Path(destination).write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
