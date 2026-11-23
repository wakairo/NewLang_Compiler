"""Check the unary Bool quantity selector and its strict native output."""

from functools import partial
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


def observe(c, discard):
    helper = re.search(rb"static size_t (nl_qty_fn_\d+)\(_Bool (nl_local_\d+)\)", c)
    entry = re.search(rb"^static void nl_qty_fn_\d+\(void\)\n\{\n", c, re.MULTILINE)
    assert helper and entry and helper.end() < entry.start()
    assert re.search(rb"_Bool nl_condition_\d+ = " + helper[2] + rb";", c)
    prefix, main = c[:entry.end()], c[entry.end():]
    assert main.count(helper[1] + b"(") == 2
    flags = re.findall(rb"^    _Bool (nl_local_\d+) = ", main, re.MULTILINE)
    marker = b"#include <stdint.h>\n"
    assert prefix.count(marker) == 1
    prefix = prefix.replace(marker, marker + b"#include <stdio.h>\n")
    if discard:
        actuals = re.findall(
            rb"^    \(void\)\(" + helper[1] + rb"\((nl_local_\d+)\)\);$",
            main, re.MULTILINE,
        )
        assert len(actuals) == 2 and actuals == flags
        # Observe the result held in the existing IF temporary. Leave both
        # call sites and their discarded results untouched.
        prefix, count = re.subn(
            rb"^(    return (nl_if_\d+);)$",
            rb'    printf("%zu\\n", \2);\n\1', prefix, flags=re.MULTILINE,
        )
        assert count == 1
        return prefix + main
    calls = re.findall(
        rb"^    size_t (nl_local_\d+) = " + helper[1] + rb"\((nl_local_\d+)\);$",
        main, re.MULTILINE,
    )
    assert len(calls) == len(flags) == 2
    assert [arg for _, arg in calls] == flags
    observed, count = re.subn(
        rb"^(    size_t (nl_local_\d+) = " + helper[1] + rb"\(nl_local_\d+\);)\n",
        rb'\1\n    printf("%zu\\n", \2);\n', main, flags=re.MULTILINE,
    )
    assert count == 2
    return prefix + observed


def native(cli, source, raw, *, discard=False, fixed=None, values=None):
    c = run(raw, "stdout-c17", [cli, source], 0)
    assert c.stdout and not c.stderr
    pristine = raw / "pristine.c"
    named = run(raw, "named-c17", [cli, "--emit-c17-to", pristine, source], 0)
    assert not named.stdout and not named.stderr and pristine.read_bytes() == c.stdout
    if fixed:
        number, operator, divisors = fixed
        numerator = re.search(
            rb"size_t (nl_local_\d+) = \(\(size_t\)UINTMAX_C\(" + number + rb"\)\);",
            c.stdout,
        )
        assert numerator
        actual = re.findall(
            numerator[1] + b" " + re.escape(operator)
            + rb" \(\(size_t\)UINTMAX_C\((\d+)\)\);",
            c.stdout,
        )
        assert actual == divisors
    observed = raw / "observed.c"
    observed.write_bytes(observe(c.stdout, discard))
    compilers = (shutil.which("gcc"), shutil.which("clang-23") or shutil.which("clang"))
    assert all(compilers), compilers
    if values is None:
        values = b"23\n41\n" if discard else b"11\n19\n"
    for compiler in compilers:
        for opt in ("O0", "O2"):
            for unit, stdout in ((pristine, b""), (observed, values)):
                label = f"{Path(compiler).name}-{opt}-{unit.stem}"
                binary = raw / label
                run(raw, label + "-compile", [
                    compiler, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra",
                    "-Werror", "-" + opt, unit, "-o", binary,
                ], 0)
                result = run(raw, label + "-native", [binary], 0)
                assert result.stdout == stdout and not result.stderr, label
    first, second = values.decode().splitlines()
    print(f"{raw.name} strict native {first} then {second}; named C17 identical")


def state(path):
    stat = path.stat()
    return (stat.st_dev, stat.st_ino, stat.st_mode, stat.st_size,
            stat.st_mtime_ns, stat.st_ctime_ns)


def scope(cli, source, raw):
    diagnostic = b"error(cli)[V1-BACKEND-UNSUPPORTED]:"
    emitted = run(raw, "stdout-c17", [cli, source], 4)
    assert not emitted.stdout and emitted.stderr.startswith(diagnostic)
    named_dir = raw / "named"
    named_dir.mkdir()
    target = named_dir / "existing.c"
    sentinel = b"/* Preserved existing C17 output. */\nint original;\n"
    target.write_bytes(sentinel)
    before = state(target)
    named = run(raw, "named-c17", [cli, "--emit-c17-to", target, source], 4)
    assert not named.stdout and named.stderr.startswith(diagnostic)
    after = state(target)
    (raw / "named-stat.txt").write_text(f"{before}\n{after}\n", encoding="ascii")
    assert target.read_bytes() == sentinel and before == after
    assert list(named_dir.iterdir()) == [target]
    print(f"{raw.name} CHECK=0 C17=4, named output preserved")


def fixtures(selection, discard, local, remainder):
    before, original = selection.split("\n\n", 1)
    body = before.replace("let amount = if", "if").replace("};\n    amount", "}")
    yield "let-copy", selection, native
    yield "direct-tail", f"{body}\n\n{original}", native
    yield "discard", discard, partial(native, discard=True)
    yield "inline-discard", discard.replace(
        "select_mark(first_flag);", "select_mark(usize(30) < stock);"
    ), scope
    yield "arm-discard", discard.replace(
        "    select_mark(first_flag);",
        "    let guarded = if (first_flag) {\n"
        "        select_mark(first_flag);\n"
        "        usize(7)\n"
        "    } else {\n"
        "        usize(9)\n"
        "    };",
    ), scope
    yield "inline-actual", selection.replace(
        "choose(select_first)", "choose(usize(50) < stock)"
    ), scope
    yield "local-numerator", selection.replace(
        "        usize(11)", "        let numerator = usize(77);\n        numerator / usize(7)"
    ), scope
    yield "called-from-helper", selection.replace(
        "fn main()", "fn relay(flag: bool) -> usize { choose(flag) }\n\nfn main()"
    ).replace("choose(select_first)", "relay(select_first)"), scope
    local_division = partial(native, fixed=(b"95", b"/", [b"6", b"8"]), values=b"15\n11\n")
    yield "local-numerator-let", local, local_division
    yield "local-numerator-discard", local.replace("let first = ", "").replace(
        "let second = ", ""
    ), partial(local_division, discard=True)
    yield "local-remainder", local.replace(" / ", " % "), partial(
        native, fixed=(b"95", b"%", [b"6", b"8"]), values=b"5\n7\n"
    )
    local_remainder = partial(native, fixed=(b"83", b"%", [b"7", b"5"]), values=b"6\n3\n")
    yield "local-remainder-let", remainder, local_remainder
    yield "local-remainder-discard", remainder.replace("let first = ", "").replace(
        "let second = ", ""
    ), partial(local_remainder, discard=True)
    yield "mixed-arm-operators", remainder.replace("stock % usize(5)", "stock / usize(5)"), scope
    yield "aliased-remainder", remainder.replace(
        "    if (flag)", "    let alias = stock;\n    if (flag)"
    ).replace("stock %", "alias %"), scope
    yield "arm-local-remainder", remainder.replace(
        "    let stock = usize(83);\n", ""
    ).replace("        stock %", "        let stock = usize(83);\n        stock %"), scope
    yield "aliased-numerator", local.replace(
        "    if (flag)", "    let alias = stock;\n    if (flag)"
    ).replace("stock /", "alias /"), scope
    yield "arm-local-numerator", local.replace(
        "    let stock = usize(95);\n", ""
    ).replace("        stock /", "        let stock = usize(95);\n        stock /"), scope


def main():
    cli, root, selection, discard, local, remainder = map(Path, sys.argv[1:])
    root.mkdir(parents=True, exist_ok=True)
    raw = Path(tempfile.mkdtemp(dir=root))
    print(f"raw={raw}", flush=True)
    cases = []
    for name, text, verify in fixtures(
        *(path.read_text(encoding="utf-8") for path in (selection, discard, local, remainder))
    ):
        work = raw / name
        work.mkdir()
        source = work / "source.nl"
        source.write_text(text, encoding="utf-8")
        cases.append((source, work, verify))
    for source, work, verify in cases:
        check = run(work, "check", [cli, "--check", source], 0)
        assert not check.stdout and not check.stderr
        verify(cli, source, work)


if __name__ == "__main__":
    main()
