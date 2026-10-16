"""Source-owned pure Copy choice; passive reads, no guest-value injection."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(args, text=True, capture_output=True, check=False)


def require(ok, detail):
    if not ok:
        raise SystemExit(detail)


def execute(root, cc, name, text, expected, sanitizer):
    source = root / f"{name}.c"
    source.write_text(text)
    statuses = []
    flags = [] if sanitizer == "none" else [f"-fsanitize={sanitizer}", "-fno-sanitize-recover=all"]
    for optimization in ("-O0", "-O2"):
        exe = root / f"{name}{optimization}"
        built = run([cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                     optimization, *flags, str(source), "-o", str(exe)])
        require(built.returncode == 0, f"strict C17 build failed: {built.stderr}")
        result = run([str(exe)])
        require(result.returncode == expected and not result.stdout and not result.stderr,
                f"native {name}/{optimization}: {result}")
        statuses.append(result.returncode)
    return statuses


def observe(text, evidence, bindings):
    pattern = r"const (size_t|_Bool|uint8_t) (nl_local_(\d+)) = ([^\n;]+);"
    declarations = list(re.finditer(pattern, text))
    require(len(declarations) == len(bindings), "missing real scalar carrier")
    seen = set()
    by_symbol = {}
    for m in declarations:
        symbol = int(m[3])
        require(symbol in bindings and symbol not in seen, "foreign/duplicate C local")
        seen.add(symbol)
        kind, value = bindings[symbol]
        require(m[1] == {"usize": "size_t", "bool": "_Bool", "u8": "uint8_t"}[kind],
                "wrong physical C carrier type")
        by_symbol[symbol] = m
    require("_Static_assert(SIZE_MAX == UINTMAX_C(" in text, "lost target-width guard")
    a, b = evidence["operand_symbols"]
    comparison = by_symbol[evidence["equal_symbol"]]
    require(comparison[4] == f"(nl_local_{a} == nl_local_{b})", "not actual operand comparison")
    selected = by_symbol[evidence["selected_symbol"]]
    arms = [f"nl_local_{sym}" if sym else str(value)
            for sym, value in zip(evidence["arm_symbols"], evidence["arms"])]
    expression = f"(nl_local_{evidence['condition_symbol']} ? {arms[0]} : {arms[1]})"
    require(selected[4] == expression and text.count(" ? ") == 1,
            "not one source-linked C conditional")
    copied = by_symbol[evidence["copied_symbol"]]
    require(copied[4] == f"nl_local_{evidence['selected_symbol']}", "not real selected Copy")
    # Read each real source-derived scalar after its actual declaration. No
    # assignments to guest locals, comparison operands, or conditional result.
    observed = text.replace("#include <stdint.h>", "#include <stdint.h>\n#include <stdlib.h>")
    observed = re.sub(pattern, lambda m: m[0] +
                      f"\n    if ({m[2]} != {bindings[int(m[3])][1]}) exit(91);", observed)
    chosen = evidence["bool"] == 1
    wrong = "8" if chosen else "7"
    mutated_arm = f"(nl_local_{evidence['condition_symbol']} ? " + (
        f"{wrong} : {arms[1]})" if chosen else f"{arms[0]} : {wrong})")
    mutants = {
        "wrong_chosen_arm": observed.replace(expression, mutated_arm, 1),
        "wrong_direction": observed.replace(expression, expression.replace("(nl_local_", "(!nl_local_", 1), 1),
        "injected_constant": observed.replace(expression, wrong, 1),
        "wrong_copy": observed.replace(copied[0], copied[0].replace(copied[4], "((uint8_t)99)"), 1),
    }
    return observed, mutants


def main():
    compiler, probe, gate, cc, directory, sanitizer = sys.argv[1:]
    root = Path(directory)
    manifest = json.loads((root / "frozen.json").read_text())
    errors = {"wrong_condition.nl": ("semantic", "P13-IF-CONDITION"),
              "invalid_else.nl": ("semantic", "P3-WRITE-REF-REQUIRED"),
              "ref_escape.nl": ("semantic", "P8-EXIT-DEPENDENCY"),
              "noncopy_arms.nl": ("unsupported", "ALLOCATED-SOURCE-PROFILE")}
    with tempfile.TemporaryDirectory(prefix="p508-native-") as temporary:
        folder = Path(temporary)
        for name, digest in manifest["source_sha256"].items():
            path = root / name
            require(hashlib.sha256(path.read_bytes()).hexdigest() == digest, f"frozen source changed:{name}")
            check = run([compiler, "--check", str(path)])
            emitted = run([compiler, str(path)])
            if name in manifest["oracles"]:
                require(check.returncode == emitted.returncode == 0 and not check.stdout and
                        not check.stderr and not emitted.stderr, f"CHECK/emitter refusal:{check} {emitted}")
                public = run([probe, str(path)])
                require(public.returncode == 0 and not public.stderr, f"source-owned probe:{public}")
                lines = public.stdout.splitlines()
                evidence = json.loads(lines[0])
                a, b, boolean, selected, copied = manifest["oracles"][name]
                require(evidence["bool"] == boolean and evidence["arms"] == [7, 8] and
                        evidence["joined_unknown"] and evidence["authority"] == 0,
                        f"wrong qualified arm evidence:{evidence}")
                bindings = {}
                for line in lines[1:]:
                    tag, symbol, kind, known, value = line.split()
                    require(tag == "BIND", "bad public observation")
                    bindings[int(symbol)] = (kind, int(value) if int(known) else selected)
                require([bindings[s][1] for s in evidence["operand_symbols"]] == [a, b], "wrong real operands")
                poison = run([gate, "poison", str(path)])
                require(poison.returncode == 0 and not poison.stderr and
                        "original=PASS worlds=UNCHANGED driver_allocations=0" in poison.stdout,
                        f"private driver poison failure:{poison}")
                observed, mutants = observe(emitted.stdout, evidence, bindings)
                results = {"plain": execute(folder, cc, name+"plain", emitted.stdout, 0, sanitizer),
                           "observed": execute(folder, cc, name+"observed", observed, 0, sanitizer)}
                for tag, corrupt in mutants.items():
                    results[tag] = execute(folder, cc, name+tag, corrupt, 91, sanitizer)
                print(json.dumps(dict(source=name, sha256=digest, check=0, public=evidence,
                                      c17=0, c_sha256=hashlib.sha256(emitted.stdout.encode()).hexdigest(),
                                      native=results, private_gate=poison.stdout.strip(), sanitizer=sanitizer)))
            elif name == "field_roots.nl":
                require(check.returncode == 0 and not check.stderr and not check.stdout and
                        emitted.returncode == 4 and not emitted.stdout and
                        "V1-BACKEND-UNSUPPORTED" in emitted.stderr, "broader root/field profile escaped gate")
                print(json.dumps(dict(source=name, check=0, c17=4, native="NOT REACHED")))
            else:
                category, code = errors[name]
                for result in (check, emitted):
                    require(result.returncode == 3 and not result.stdout and
                            f"error({category})[{code}]" in result.stderr, f"wrong first refusal:{result}")
                print(json.dumps(dict(source=name, exit=3, diagnostic=check.stderr.strip())))
    print("P508 finite pure u8 IF native selection; no Pair Change/ownership/Product claim")


if __name__ == "__main__":
    main()
