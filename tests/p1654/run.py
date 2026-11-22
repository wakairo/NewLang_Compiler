"""Check the unary Bool quantity selector and its strict native output."""

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


def observe(c):
    helper = re.search(rb"static size_t (nl_qty_fn_\d+)\(_Bool (nl_local_\d+)\)", c)
    entry = re.search(rb"^static void nl_qty_fn_\d+\(void\)\n\{\n", c, re.MULTILINE)
    assert helper and entry and helper.end() < entry.start()
    assert re.search(rb"_Bool nl_condition_\d+ = " + helper[2] + rb";", c)
    prefix, main = c[:entry.end()], c[entry.end():]
    assert main.count(helper[1] + b"(") == 2
    calls = re.findall(
        rb"^    size_t (nl_local_\d+) = " + helper[1] + rb"\((nl_local_\d+)\);$",
        main, re.MULTILINE,
    )
    flags = re.findall(rb"^    _Bool (nl_local_\d+) = ", main, re.MULTILINE)
    assert len(calls) == len(flags) == 2
    assert [arg for _, arg in calls] == flags
    marker = b"#include <stdint.h>\n"
    assert prefix.count(marker) == 1
    observed, count = re.subn(
        rb"^(    size_t (nl_local_\d+) = " + helper[1] + rb"\(nl_local_\d+\);)\n",
        rb'\1\n    printf("%zu\\n", \2);\n', main, flags=re.MULTILINE,
    )
    assert count == 2
    return prefix.replace(marker, marker + b"#include <stdio.h>\n") + observed


def native(cli, source, raw):
    c = run(raw, "stdout-c17", [cli, source], 0)
    assert c.stdout and not c.stderr
    pristine = raw / "pristine.c"
    named = run(raw, "named-c17", [cli, "--emit-c17-to", pristine, source], 0)
    assert not named.stdout and not named.stderr and pristine.read_bytes() == c.stdout
    observed = raw / "observed.c"
    observed.write_bytes(observe(c.stdout))
    compilers = (shutil.which("gcc"), shutil.which("clang-23") or shutil.which("clang"))
    assert all(compilers), compilers
    for compiler in compilers:
        for opt in ("O0", "O2"):
            for unit, stdout in ((pristine, b""), (observed, b"11\n19\n")):
                label = f"{Path(compiler).name}-{opt}-{unit.stem}"
                binary = raw / label
                run(raw, label + "-compile", [
                    compiler, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra",
                    "-Werror", "-" + opt, unit, "-o", binary,
                ], 0)
                result = run(raw, label + "-native", [binary], 0)
                assert result.stdout == stdout and not result.stderr, label
    print(f"{raw.name} strict native 11 then 19; named C17 identical")


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


def fixtures(selection):
    before, original = selection.split("\n\n", 1)
    body = before.replace("let amount = if", "if").replace("};\n    amount", "}")
    yield "let-copy", selection, native
    yield "direct-tail", f"{body}\n\n{original}", native
    yield "inline-actual", selection.replace(
        "choose(select_first)", "choose(usize(50) < stock)"
    ), scope
    yield "local-numerator", selection.replace(
        "        usize(11)", "        let numerator = usize(77);\n        numerator / usize(7)"
    ), scope
    yield "called-from-helper", selection.replace(
        "fn main()", "fn relay(flag: bool) -> usize { choose(flag) }\n\nfn main()"
    ).replace("choose(select_first)", "relay(select_first)"), scope


def main():
    cli, root, selection = map(Path, sys.argv[1:])
    root.mkdir(parents=True, exist_ok=True)
    raw = Path(tempfile.mkdtemp(dir=root))
    print(f"raw={raw}", flush=True)
    cases = []
    for name, text, verify in fixtures(selection.read_text(encoding="utf-8")):
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
