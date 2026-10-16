"""Normal compiler: source quantities -> checked bool -> real C17 comparison.

Observers read existing carriers only; separate malformed-C controls retain the
same observer. Original frozen inputs and historical P497 evidence never change.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(args, text=True, capture_output=True, check=False)


def require(ok, message):
    if not ok:
        raise SystemExit(message)


def execute(root, cc, name, text, status, sanitizer):
    source = root / f"{name}.c"
    source.write_text(text)
    statuses = []
    extra = [] if sanitizer == "none" else [f"-fsanitize={sanitizer}", "-fno-sanitize-recover=all"]
    for optimization in ("-O0", "-O2"):
        exe = root / f"{name}{optimization}"
        built = run([cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                     optimization, *extra, str(source), "-o", str(exe)])
        require(built.returncode == 0, f"strict C17 failed: {built.stderr}")
        result = run([str(exe)])
        require(result.returncode == status and not result.stdout and not result.stderr,
                f"native mismatch {name}/{optimization}: {result}")
        statuses.append(result.returncode)
    return statuses


def observe(text, evidence, bindings):
    pattern = r"const (size_t|_Bool) (nl_local_(\d+)) = ([^\n]+);"
    declarations = list(re.finditer(pattern, text))
    require(len(declarations) == len(bindings), "missing actual source receiver")
    seen = set()
    comparisons = []
    for m in declarations:
        symbol = int(m[3])
        require(symbol in bindings and symbol not in seen, "foreign/duplicate C carrier")
        seen.add(symbol)
        kind, _ = bindings[symbol]
        require(m[1] == ("_Bool" if kind == "bool" else "size_t"), "wrong physical scalar type")
        if " == " in m[4]:
            comparisons.append(m)
    require(len(comparisons) == 1, "not one real emitted comparison")
    comparison = comparisons[0]
    operands = [f"nl_local_{symbol}" if symbol else f"((size_t)UINTMAX_C({value}))"
                for symbol, value in zip(evidence["symbols"], (evidence["left"], evidence["right"]))]
    expression = f"({operands[0]} == {operands[1]})"
    require(comparison[4] == expression and int(comparison[3]) == evidence["bool_symbol"],
            "comparison is not connected to original checked operands")
    require("_Static_assert(SIZE_MAX == UINTMAX_C(" in text, "lost target-width guard")
    result = text.replace("#include <stddef.h>", "#include <stddef.h>\n#include <stdlib.h>")
    result = re.sub(pattern, lambda m: m[0] +
                    f"\n    if ({m[2]} != {bindings[int(m[3])][1]}) exit(91);", result)
    mutants = {"wrong_comparison": result.replace(expression, expression.replace(" == ", " != "), 1)}
    copies = [m for m in declarations if m[1] == "_Bool" and re.fullmatch(r"nl_local_\d+", m[4])]
    if copies:
        copy = copies[0]
        mutants["wrong_copy"] = result.replace(copy[0], copy[0].replace(" = ", " = !", 1), 1)
    return result, mutants


def main():
    compiler, public_probe, gate_probe, cc, directory, sanitizer = sys.argv[1:]
    root = Path(directory)
    frozen = json.loads((root / "frozen.json").read_text())
    failures = {"wrong_type.nl": (3, "semantic", "USIZE-EQUALITY-TYPE"),
                "bool_as_ref.nl": (3, "semantic", "P3-WRITE-REF-REQUIRED"),
                "unknown.nl": (3, "precision", "USIZE-EQUALITY-PRECISION"),
                "chain.nl": (3, "unsupported", "USIZE-EQUALITY-SOURCE-PRECISION"),
                "split_operator.nl": (3, "unsupported", "P5-EXPRESSION-UNSUPPORTED"),
                "full_guard_true.nl": (3, "precision", "P6-JOIN-PRECISION"),
                "full_guard_false.nl": (3, "precision", "P6-JOIN-PRECISION")}
    for name, digest in frozen["p497_migration"]["immutable_sources"].items():
        require(hashlib.sha256((root.parent / "p497" / name).read_bytes()).hexdigest() == digest,
                f"adopted source changed: {name}")
    with tempfile.TemporaryDirectory(prefix="p504-native-") as folder:
        temporary = Path(folder)
        for name, digest in frozen["sources"].items():
            source = root / name
            require(hashlib.sha256(source.read_bytes()).hexdigest() == digest, f"frozen source changed: {name}")
            checked = run([compiler, "--check", str(source)])
            emitted = run([compiler, str(source)])
            if name in frozen["oracles"]["native"]:
                require(checked.returncode == emitted.returncode == 0 and not checked.stdout and
                        not checked.stderr and not emitted.stderr, f"source/native admission failed: {checked} {emitted}")
                public = run([public_probe, str(source)])
                require(public.returncode == 0 and not public.stderr, f"source-owned linkage failed: {public}")
                lines = public.stdout.splitlines()
                evidence = json.loads(lines[0])
                a, b, truth = frozen["oracles"]["native"][name]
                require([evidence["left"], evidence["right"], evidence["bool"]] == [a, b, truth] and
                        evidence["authority"] == 0 and evidence["if_checks"] == 0,
                        f"incorrect original operands/bool: {evidence}")
                bindings = {}
                for line in lines[1:]:
                    tag, symbol, kind, value = line.split()
                    require(tag == "BIND", "unknown observation record")
                    bindings[int(symbol)] = (kind, int(value))
                gate = run([gate_probe, str(source)])
                require(gate.returncode == 0 and not gate.stderr and
                        "original=PASS snapshot=UNCHANGED" in gate.stdout,
                        f"emitter corrupted-evidence gate failed: {gate}")
                observed, mutants = observe(emitted.stdout, evidence, bindings)
                statuses = {"plain": execute(temporary, cc, name + "plain", emitted.stdout, 0, sanitizer),
                            "observed": execute(temporary, cc, name + "observed", observed, 0, sanitizer)}
                for mutant, text in mutants.items():
                    statuses[mutant] = execute(temporary, cc, name + mutant, text, 91, sanitizer)
                print(json.dumps(dict(source=name, sha256=digest, check=0, public=evidence,
                                      bindings=bindings, private_gate=gate.stdout.strip(), c17=0,
                                      c_sha256=hashlib.sha256(emitted.stdout.encode()).hexdigest(),
                                      native=statuses, sanitizer=sanitizer)))
            elif name in ("if_true.nl", "if_false.nl"):
                require(checked.returncode == 0 and not checked.stdout and not checked.stderr and
                        emitted.returncode == 4 and not emitted.stdout and
                        "V1-BACKEND-UNSUPPORTED" in emitted.stderr, "IF boundary was bypassed")
                public = run([public_probe, str(source)])
                require(public.returncode == 0 and not public.stderr and
                        json.loads(public.stdout.splitlines()[0])["if_checks"] == 1,
                        "genuine IF CHECK evidence missing")
                print(json.dumps(dict(source=name, check=0, c17=4, c_bytes=0, native="NOT REACHED")))
            else:
                status, category, code = failures[name]
                for result in (checked, emitted):
                    require(result.returncode == status and not result.stdout and
                            f"error({category})[{code}]" in result.stderr, f"wrong refusal: {result}")
                print(json.dumps(dict(source=name, exit=status, diagnostic=checked.stderr.strip(), c_bytes=0)))
    print("P504 normal usize equality native FINITE; IF effects/Pair join HOLD")


if __name__ == "__main__":
    main()
