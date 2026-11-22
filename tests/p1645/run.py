"""Exercise one registered unary call with a fixed division actual in an IF."""

from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


def run(raw, name, command, expected):
    args = list(map(str, command))
    result = subprocess.run(args, capture_output=True, check=False)
    (raw / f"{name}.command").write_text("\n".join(args) + "\n", encoding="utf-8")
    (raw / f"{name}.status").write_text(f"{result.returncode}\n", encoding="ascii")
    (raw / f"{name}.stdout").write_bytes(result.stdout)
    (raw / f"{name}.stderr").write_bytes(result.stderr)
    assert result.returncode == expected, (name, result.returncode, result.stderr)
    return result


def check(cli, source, raw):
    checked = run(raw, "check", [cli, "--check", source], 0)
    assert not checked.stdout and not checked.stderr


def verify_arm_order(c, divisors, direct_actuals):
    branches = re.findall(
        rb"    _Bool nl_condition_(\d+) = [^\n]+;\n"
        rb"    if \(nl_condition_\1\) \{\n(.*?)"
        rb"    \} else \{\n(.*?)    \}\n",
        c,
        re.DOTALL,
    )
    assert len(branches) == 1
    _, *arms = branches[0]
    for arm, expected_divisors, direct in zip(arms, divisors, direct_actuals, strict=True):
        occurrences = re.findall(rb" / \(\(size_t\)UINTMAX_C\((\d+)\)\)", arm)
        assert tuple(map(int, occurrences)) == expected_divisors
        calls = re.findall(rb"        nl_if_\d+ = nl_qty_fn_\d+\((.+)\);\n", arm)
        assert len(calls) == 1 and (b" / " in calls[0]) == direct
        assert arm.count(b"nl_qty_fn_") == 1
        if expected_divisors and not direct:
            assert arm.find(b" / ") < arm.find(b"nl_qty_fn_")


def observe_main(c):
    match = re.search(rb"^static void nl_qty_fn_\d+\(void\)\n\{\n", c, re.MULTILINE)
    assert match
    prefix, main_and_wrapper = c[:match.end()], c[match.end():]
    observed, count = re.subn(
        rb"^(    size_t (nl_local_\d+) = nl_qty_fn_\d+\([^\n]+\);)\n",
        rb'\1\n    printf("%zu\\n", \2);\n',
        main_and_wrapper,
        flags=re.MULTILINE,
    )
    assert count == 2
    marker = b"#include <stdint.h>\n"
    assert prefix.count(marker) == 1
    return prefix.replace(marker, marker + b"#include <stdio.h>\n") + observed


def native(cli, source_dir, raw):
    cases = (
        ("arm-direct", ((3,), ()), (True, False), b"2\n11\n"),
        ("arm-named-copy", ((3,), ()), (False, False), b"2\n11\n"),
        ("arm-both", ((4, 4), (9,)), (True, True), b"11\n12\n"),
    )
    compilers = (shutil.which("gcc"), shutil.which("clang-23") or shutil.which("clang"))
    assert all(compilers), compilers
    for name, divisors, direct, values in cases:
        source = source_dir / f"{name}.nl"
        work = raw / name
        work.mkdir()
        check(cli, source, work)
        emitted = run(work, "stdout-c17", [cli, source], 0)
        assert emitted.stdout and not emitted.stderr
        pristine = work / "pristine.c"
        named = run(work, "named-c17", [cli, "--emit-c17-to", pristine, source], 0)
        assert not named.stdout and not named.stderr
        assert pristine.read_bytes() == emitted.stdout
        verify_arm_order(emitted.stdout, divisors, direct)
        observed = work / "observed.c"
        observed.write_bytes(observe_main(emitted.stdout))
        for cc in compilers:
            for level in ("-O0", "-O2"):
                for program, output in ((pristine, b""), (observed, values)):
                    label = f"{program.stem}-{Path(cc).name}-{level[1:]}"
                    binary = work / label
                    built = run(work, f"{label}-compile",
                                [cc, "-std=c17", "-pedantic-errors", "-Wall",
                                 "-Wextra", "-Werror", level, program, "-o", binary], 0)
                    assert not built.stdout and not built.stderr
                    executed = run(work, f"{label}-native", [binary], 0)
                    assert executed.stdout == output and not executed.stderr
        first, second = values.decode().splitlines()
        print(f"{name} strict native true={first} false={second}, order and named=PASS")


def replace_once(text, old, new):
    assert text.count(old) == 1
    return text.replace(old, new)


def fixtures(source_dir):
    direct = (source_dir / "arm-direct.nl").read_text(encoding="utf-8")
    both = (source_dir / "arm-both.nl").read_text(encoding="utf-8")
    call = "trim(available / usize(3))"
    yield "remainder", replace_once(direct, call, "trim(available % usize(3))")
    yield "nested-call", replace_once(direct, call, f"trim({call})")
    pair = replace_once(direct, "fn trim(value: usize)", "fn trim(value: usize, spare: usize)")
    pair = replace_once(pair, call, "trim(available / usize(3), available)")
    yield "two-actuals", replace_once(pair, "trim(carried)", "trim(carried, available)")
    yield "binding-call", replace_once(direct, call, f"let total = {call};\n        total")
    normal_sibling = replace_once(
        both, "reduce(copied / usize(9))", "let quotient = copied / usize(9);\n        reduce(quotient)"
    )
    yield "return-arm", replace_once(
        normal_sibling, "reduce(copied / usize(4))", "return reduce(copied / usize(4));"
    )
    yield "one-normal-arm", replace_once(both, "reduce(copied / usize(4))", "return spare;")


def state(path):
    stat = path.stat()
    return (stat.st_dev, stat.st_ino, stat.st_mode, stat.st_size,
            stat.st_mtime_ns, stat.st_ctime_ns)


def scope(cli, source_dir, raw):
    for name, text in fixtures(source_dir):
        work = raw / name
        work.mkdir()
        source = work / f"{name}.nl"
        source.write_text(text, encoding="utf-8")
        check(cli, source, work)
        diagnostic = b"error(cli)[V1-BACKEND-UNSUPPORTED]:"
        emitted = run(work, "stdout-c17", [cli, source], 4)
        assert not emitted.stdout and emitted.stderr.startswith(diagnostic)
        output = work / "named"
        output.mkdir()
        target = output / "keep.c"
        sentinel = b"/* P1645 existing output. */\nint original;\n"
        target.write_bytes(sentinel)
        before = state(target)
        (work / "named-before.stat").write_text(f"{before}\n", encoding="ascii")
        named = run(work, "named-c17", [cli, "--emit-c17-to", target, source], 4)
        assert not named.stdout and named.stderr.startswith(diagnostic)
        after = state(target)
        (work / "named-after.stat").write_text(f"{after}\n", encoding="ascii")
        assert target.read_bytes() == sentinel and before == after
        assert list(output.iterdir()) == [target]
        print(f"{name} CHECK=0 C17=4 named output preserved")


def main():
    mode, cli, root, source_dir = sys.argv[1:]
    root = Path(root)
    root.mkdir(parents=True, exist_ok=True)
    raw = Path(tempfile.mkdtemp(dir=root))
    print(f"raw={raw}", flush=True)
    {"native": native, "scope": scope}[mode](Path(cli), Path(source_dir), raw)


if __name__ == "__main__":
    main()
