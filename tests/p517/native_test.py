"""Calibrated R516 replay; immutable replicas and passive physical reads."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(args, capture_output=True, text=True, check=False)


def require(ok, message):
    if not ok:
        raise SystemExit(message)


def execute(root, cc, name, text, expected, sanitizer):
    file = root / f"{name}.c"
    file.write_text(text)
    flags = [] if sanitizer == "none" else [f"-fsanitize={sanitizer}", "-fno-sanitize-recover=all"]
    statuses = []
    for opt in ("-O0", "-O2"):
        exe = root / f"{name}{opt}"
        built = run([cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                     opt, *flags, str(file), "-o", str(exe)])
        require(built.returncode == 0, f"C17 {name}:{built.stderr}")
        result = run([str(exe)])
        require(result.returncode == expected and not result.stdout and not result.stderr,
                f"native {name}/{opt}: {result}")
        statuses.append(result.returncode)
    return statuses


def observe(text, e, oracle):
    root = f"nl_local_{e['original_symbol']}"
    selected = f"nl_local_{e['selected_symbol']}"
    incoming = f"nl_local_{e['input_symbol']}"
    ref = f"nl_local_{e['ref_symbol']}"
    condition = f"nl_local_{e['condition_symbol']}"
    arms = [f"nl_local_{s}" if s else str(v) for s, v in zip(e['arm_symbols'], e['arms'])]
    expr = f"({condition} ? {arms[0]} : {arms[1]})"
    a, b = e['operand_symbols']
    equality = f"(nl_local_{a} == nl_local_{b})"
    comparison = f"const _Bool nl_local_{e['equal_symbol']} = {equality};" if e['equal_symbol'] else ''
    if not e['condition_symbol']:
        condition = equality
        expr = f"({condition} ? {arms[0]} : {arms[1]})"
    choice = f"const uint8_t {selected} = {expr};"
    pointer = f"uint8_t *const {ref} = &{root}.f0;"
    write = f"*{ref} = {incoming};"
    capture = f"const uint8_t nl_replace_old_{e['replace_node']} = *{ref};"
    physical = re.search(rf"nl_type_\d+ {root} = \{{ .f0 = {oracle['old']}, .f1 = {oracle['sibling']}, \}};", text)
    require(physical is not None and text.index(physical[0]) < text.index(choice) <
            text.index(pointer) < text.index(capture) < text.index(write), "lost original pre-IF carrier/order")
    require(all(text.count(x) == 1 for x in (choice, comparison, pointer, capture, write) if x) and
            text.count(" ? ") == 1 and len(re.findall(r"\*nl_local_\d+ = nl_local_\d+;", text)) == 1,
            "not one conditional and one physical write")
    if incoming != selected:
        require(f"const uint8_t {incoming} = {selected};" in text, "lost source-issued selected Copy")
    old = f"const uint8_t nl_local_{e['old_symbol']} = nl_loan_result_{e['scope']};"
    after = f"const uint8_t nl_local_{e['after_symbol']} = {root}.f0;"
    sibling = f"const uint8_t nl_local_{e['sibling_symbol']} = {root}.f1;"
    require(all(x in text for x in (old, after, sibling)), "lost original reads/old forwarding")
    if comparison:
        require(text.index(physical[0]) < text.index(comparison) < text.index(choice), "lost named comparison order")
    require(equality in text, "missing real operand comparison")
    observed = text.replace("#include <stdint.h>", "#include <stdint.h>\n#include <stdlib.h>")
    checks = {
        physical[0]: f"{root}.f0 != {oracle['old']} || {root}.f1 != {oracle['sibling']}",
        choice: f"{selected} != {oracle['selected']}",
        pointer: f"{ref} != &{root}.f0",
        capture: f"nl_replace_old_{e['replace_node']} != {oracle['old']}",
        write: f"{root}.f0 != {oracle['after']} || {root}.f1 != {oracle['sibling']}",
        old: f"nl_local_{e['old_symbol']} != {oracle['old']}",
        after: f"nl_local_{e['after_symbol']} != {oracle['after']}",
        sibling: f"nl_local_{e['sibling_symbol']} != {oracle['sibling']}",
    }
    if comparison:
        checks[comparison] = f"nl_local_{e['equal_symbol']} != {oracle['bool']}"
    for point, check in checks.items():
        observed = observed.replace(point, point + f"\n    if ({check}) exit(91);", 1)
    mutants = {
        "swapped_arms": observed.replace(expr, f"({condition} ? {arms[1]} : {arms[0]})", 1),
        "wrong_direction": observed.replace(expr, f"(!{condition} ? {arms[0]} : {arms[1]})", 1),
        "selected_carrier": observed.replace(write, f"*{ref} = 99;", 1),
        "wrong_old": observed.replace(capture, capture.replace(f"*{ref}", "99"), 1),
        "wrong_sibling": observed.replace(physical[0], physical[0].replace(f".f1 = {oracle['sibling']}", ".f1 = 0"), 1),
        "wrong_field": observed.replace(pointer, pointer.replace(".f0", ".f1"), 1),
        "wrong_original": observed.replace(pointer, physical[0].replace(root, "nl_wrong_root") + "\n" +
                                            pointer.replace(f"&{root}", "&nl_wrong_root"), 1),
    }
    if incoming != selected:
        copy = f"const uint8_t {incoming} = {selected};"
        mutants['wrong_copy'] = observed.replace(copy, copy.replace(f"= {selected}", "= 99"), 1)
    return observed, mutants


def main():
    compiler, probe, gate, old_gate, cc, directory, sanitizer = sys.argv[1:]
    root = Path(directory)
    manifest = json.loads((root / "frozen.json").read_text())
    errors = {'r516_reject_arm.nl': ('semantic', 'V1-U8-LITERAL-RANGE')}
    with tempfile.TemporaryDirectory(prefix='p517-native-') as tmp:
        folder = Path(tmp)
        for name, digest in manifest['source_sha256'].items():
            file = root / name
            require(hashlib.sha256(file.read_bytes()).hexdigest() == digest, f'frozen input changed: {name}')
            check, emitted = run([compiler, '--check', str(file)]), run([compiler, str(file)])
            if name in manifest['positive_oracles']:
                oracle = manifest['positive_oracles'][name]
                require(check.returncode == emitted.returncode == 0 and not check.stdout and
                        not check.stderr and not emitted.stderr, f'CHECK/C17 refusal {check} {emitted}')
                public = run([probe, str(file)])
                require(public.returncode == 0 and not public.stderr, f'public observation failure {public}')
                evidence = json.loads(public.stdout)
                require(evidence['bool'] == oracle['bool'] and evidence['arms'] == [108, 173] and evidence['old'] == oracle['old'] and evidence['sibling'] == oracle['sibling'] and
                        evidence['joined_unknown'] and evidence['scope_ended'], 'owned mismatch')
                poison = run([gate, str(file)])
                require(poison.returncode == 0 and 'worlds=UNCHANGED driver_allocations=0' in poison.stdout,
                        f'private gate failure {poison}')
                observed, mutants = observe(emitted.stdout, evidence, oracle)
                native = {'plain': execute(folder, cc, name+'plain', emitted.stdout, 0, sanitizer),
                          'observed': execute(folder, cc, name+'observed', observed, 0, sanitizer)}
                for label, mutated in mutants.items():
                    native[label] = execute(folder, cc, name+label, mutated, 91, sanitizer)
                print(json.dumps(dict(source=name, sha256=digest, check=0, owned=evidence, c17=0,
                                      c_sha256=hashlib.sha256(emitted.stdout.encode()).hexdigest(),
                                      native=native, gate=poison.stdout.strip(), sanitizer=sanitizer)))
            elif name in errors:
                rollback = run([old_gate, 'reject', str(file)])
                require(rollback.returncode == 0 and 'artifact=NULL' in rollback.stdout and
                        'rollback=PASS' in rollback.stdout,
                        f'semantic failure rollback:{rollback}')
                category, code = errors[name]
                require(all(x.returncode == 3 and not x.stdout and f'error({category})[{code}]' in x.stderr
                            for x in (check, emitted)), f'wrong first refusal {check} {emitted}')
                print(json.dumps(dict(source=name, check=3, diagnostic=check.stderr.strip(), rollback=rollback.stdout.strip())))
            else:
                require(check.returncode == 0 and not check.stderr and not check.stdout and
                        emitted.returncode == 4 and not emitted.stdout and 'V1-BACKEND-UNSUPPORTED' in emitted.stderr,
                        f'broader finite boundary changed: {check} {emitted}')
                print(json.dumps(dict(source=name, check=0, c17=4, native='NOT REACHED')))
    print('P517 calibrated immutable R replay; original R first-pass FINDING unchanged')


if __name__ == '__main__':
    main()
