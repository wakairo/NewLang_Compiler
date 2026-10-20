"""R675 BLIND independent normal-CLI checks, passive emitted-C observer."""
import pathlib
import re
import subprocess
import sys
import tempfile

DIRECTORY = pathlib.Path(__file__).resolve().parent
POSITIVE = (
    ("positive_index0", "seam", 0, 91, 37, 146),
    ("positive_index1", "depth", 1, 173, 29, 61),
)
NEGATIVE = (
    "negative_unknown",
    "negative_initializer_type",
    "negative_decl_type",
    "negative_escape",
    "negative_replace_type",
)


def invoke(command):
    return subprocess.run(command, text=True, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, check=False)


def native_observe(text, target_index, initial, sibling, new):
    # Locate the *actual emitted original lexical record*, not a test proxy.
    records = list(re.finditer(
        r"(?m)^\s*(?:const\s+)?nl_type_\d+\s+(nl_local_\d+)\s*=\s*(\{[^;\n]*\})\s*;", text))
    print(f"R675 source-issued C records={len(records)}", flush=True)
    if len(records) != 1:
        raise AssertionError(f"original C carrier count {len(records)} != 1")
    root = records[0]
    variable = root.group(1)
    # C designated initializers follow source expression order, not declaration order.
    # Validate both fixed indices and every value instead of assuming f0 precedes f1.
    entries = re.findall(r"\.f([01])\s*=\s*(\d+)", root.group(2))
    if len(entries) != 2 or sorted(int(field) for field, _ in entries) != [0, 1]:
        raise AssertionError(f"missing/duplicate declared physical members {entries}")
    physical = {int(field): int(number) for field, number in entries}
    values = (physical[0], physical[1])
    expected = (initial, sibling) if target_index == 0 else (sibling, initial)
    if values != expected:
        raise AssertionError(f"original initializer index values {values} != {expected}")

    stores = list(re.finditer(r"(?m)^\s*\*(nl_local_\d+)\s*=\s*(\d+)\s*;", text))
    matches = [store for store in stores if int(store.group(2)) == new]
    if len(matches) != 1 or matches[0].start() <= root.end():
        raise AssertionError(f"write site for intended {new}: {len(matches)}")
    store = matches[0]
    pointer = store.group(1)
    expected_member = f"{variable}.f{target_index}"
    if not re.search(
            rf"\buint8_t\s*\*\s*const\s+{re.escape(pointer)}\s*=\s*&{re.escape(expected_member)}\s*;",
            text[:store.start()]):
        raise AssertionError("write pointer not taken from original declaration index")
    olds = list(re.finditer(
        rf"\b(nl_replace_old_\d+)\s*=\s*\*{re.escape(pointer)}\s*;",
        text[:store.start()]))
    if len(olds) != 1:
        raise AssertionError("old Copy not demonstrably loaded from original field")
    old_var = olds[0].group(1)
    # Both probes only read the generated original while it is live.
    # They create neither source ids nor alternate values, pointers or records.
    location = (f"(void *)&{variable}", f"(void *)&{variable}.f0",
                f"(void *)&{variable}.f1")
    values_expr = (f"(unsigned){variable}.f0", f"(unsigned){variable}.f1")
    before = (f'\n    printf("R675 BEFORE %p %p %p %zu %u %u\\n", '
              f'{location[0]}, {location[1]}, {location[2]}, sizeof({variable}), '
              f'{values_expr[0]}, {values_expr[1]});\n')
    after = (f'\n    printf("R675 AFTER %p %p %p %zu %u %u %u\\n", '
             f'{location[0]}, {location[1]}, {location[2]}, sizeof({variable}), '
             f'{values_expr[0]}, {values_expr[1]}, (unsigned){old_var});\n')
    instrumented = text[:store.end()] + after + text[store.end():]
    instrumented = instrumented[:root.end()] + before + instrumented[root.end():]
    return "#include <stdio.h>\n" + instrumented


def physical_validate(output, target_index, initial, sibling, new):
    lines = [line for line in output.splitlines() if line.startswith("R675 ")]
    print(output, end="", flush=True)
    if len(lines) != 2:
        raise AssertionError(f"physical guest observations {len(lines)} != 2")
    phases = []
    for line in lines:
        parts = line.split()
        if len(parts) not in (8, 9):
            raise AssertionError(f"malformed actual-guest observation {line!r}")
        phase = parts[1]
        if phase not in ("BEFORE", "AFTER"):
            raise AssertionError(f"unexpected phase {phase}")
        root, field0, field1 = (int(x, 16) for x in parts[2:5])
        size, v0, v1 = (int(x) for x in parts[5:8])
        if root <= 0 or field0 == field1 or not (root <= field0 < root + size) or not (
                root <= field1 < root + size):
            raise AssertionError("different or non-original guest field storage")
        phases.append((phase, root, field0, field1, size, (v0, v1),
                       int(parts[8]) if len(parts) == 9 else None))
    original = (initial, sibling) if target_index == 0 else (sibling, initial)
    result = (new, sibling) if target_index == 0 else (sibling, new)
    if ([x[0] for x in phases] != ["BEFORE", "AFTER"]
            or phases[0][1:5] != phases[1][1:5]
            or phases[0][5] != original or phases[1][5] != result
            or phases[1][6] != initial):
        raise AssertionError(f"wrong physical root/member/old Copy: {phases}")


def positive(compiler, probe, cc, sanitizer, tmp, name, field, index, old,
             sibling, new):
    source = DIRECTORY / (name + ".nl")
    check = invoke([compiler, "--check", str(source)])
    print(f"R675 {name} CHECK rc={check.returncode} "
          f"stdout={check.stdout!r} stderr={check.stderr!r}", flush=True)
    if check.returncode or check.stdout or check.stderr:
        raise AssertionError(f"{name}: candidate SOURCE/CHECK invalid")
    observed = invoke([probe, str(source), field, str(index)])
    print(f"R675 {name} PUBLIC rc={observed.returncode}\n"
          f"{observed.stdout}{observed.stderr}", flush=True)
    if observed.returncode:
        raise AssertionError(f"{name}: source-issued public owned evidence failed")
    emission = invoke([compiler, str(source)])
    print(f"R675 {name} C17 rc={emission.returncode} "
          f"stderr={emission.stderr!r}, bytes={len(emission.stdout)}", flush=True)
    if emission.returncode:
        if index == 1 and emission.returncode == 4 and not emission.stdout and (
                "BACKEND-UNSUPPORTED" in emission.stderr):
            print("R675 SOURCE-VALID / BACKEND4 limitation, C17 and native NOT REACHED",
                  flush=True)
            return
        raise AssertionError(f"{name}: unexpected backend rejection or partial C")
    if not emission.stdout or emission.stderr:
        raise AssertionError("C17 success without clean real output")
    cfile = tmp / (name + ".c")
    exe = tmp / (name + ".bin")
    cfile.write_text(native_observe(emission.stdout, index, old, sibling, new),
                     encoding="utf-8")
    command = [cc, "-std=c17", "-Wall", "-Wextra", "-Werror", "-pedantic",
               str(cfile), "-o", str(exe)]
    if sanitizer == "address":
        command.extend(["-fsanitize=address", "-fno-omit-frame-pointer"])
    if sanitizer == "undefined":
        command.extend(["-fsanitize=undefined", "-fno-sanitize-recover=all"])
    built = invoke(command)
    print(f"R675 {name} strict C17 rc={built.returncode} "
          f"stderr={built.stderr!r}", flush=True)
    if built.returncode:
        raise AssertionError("C17 compile failure")
    guest = invoke([str(exe)])
    print(f"R675 {name} guest rc={guest.returncode} stderr={guest.stderr!r}",
          flush=True)
    if guest.returncode or guest.stderr:
        raise AssertionError("actual guest/native mismatch")
    physical_validate(guest.stdout, index, old, sibling, new)
    print(f"R675 {name} strict C17/physical original root PASS", flush=True)


def negative(compiler, name):
    source = DIRECTORY / (name + ".nl")
    for mode in ("check", "emit"):
        args = [compiler, "--check", str(source)] if mode == "check" else [
            compiler, str(source)]
        r = invoke(args)
        print(f"R675 {name} {mode} rc={r.returncode} "
              f"stdout={r.stdout!r} stderr={r.stderr!r}", flush=True)
        if r.returncode == 0 or r.stdout or not r.stderr:
            raise AssertionError(f"{name}: accepted invalid source or partial C")
        if "cli:" in r.stderr.lower() and "usage" in r.stderr.lower():
            raise AssertionError(f"{name}: CLI failure rather than actual parse/check")


def main():
    compiler, probe, cc, sanitizer = sys.argv[1:5]
    with tempfile.TemporaryDirectory(prefix="r675-source-native-") as folder:
        tmp = pathlib.Path(folder)
        for case in POSITIVE:
            positive(compiler, probe, cc, sanitizer, tmp, *case)
        for name in NEGATIVE:
            negative(compiler, name)
    print("R675 BLIND first-pass SOURCE / owned / C17 / native / semantic negatives DONE",
          flush=True)


if __name__ == "__main__":
    main()
