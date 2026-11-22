"""Check the C17 scope of a quantity IF with one source Return arm."""

import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


EXPECTED_SOURCE = "a759077a0fbd4bfe316219162de0b51b34c067d0439f9dfaff383d3ce5f33c48"
SCOPE_SOURCE = "3eb41468d5a3adad6d98571cb2ed1138e73584704ae955d63c131ed8dba3af79"


def run(command, expected, evidence):
    result = subprocess.run(command, capture_output=True, check=False)
    evidence.with_suffix(".command").write_text(
        "\n".join(map(str, command)) + "\n", encoding="utf-8"
    )
    evidence.with_suffix(".stdout").write_bytes(result.stdout)
    evidence.with_suffix(".stderr").write_bytes(result.stderr)
    evidence.with_suffix(".status").write_text(
        f"{result.returncode}\n", encoding="ascii"
    )
    assert result.returncode == expected, (command, result.returncode, result.stderr)
    return result


def observed(pristine):
    lines = pristine.decode().splitlines(keepends=True)
    output = []
    calls = 0
    for line in lines:
        output.append(line)
        if line == "#include <stdint.h>\n":
            output.append("#include <stdio.h>\n")
        match = re.fullmatch(
            r"    size_t (nl_local_\d+) = nl_qty_fn_\d+\(nl_actual_\d+_0, "
            r"nl_actual_\d+_1\);\n",
            line,
        )
        if match:
            calls += 1
            output.append(f'    printf("%zu\\n", {match[1]});\n')
    assert calls == 2
    result = "".join(output).encode()
    stripped = result.replace(b"#include <stdio.h>\n", b"")
    stripped = re.sub(
        rb'    printf\("%zu\\n", nl_local_\d+\);\n', b"", stripped
    )
    assert stripped == pristine
    return result


def check(cli, source, raw):
    checked = run([cli, "--check", source], 0, raw / "check")
    assert not checked.stdout and not checked.stderr


def exercise_native(cli, source, raw, *, early_return):
    check(cli, source, raw)
    pristine = raw / "pristine.c"
    emitted = run([cli, "--emit-c17-to", pristine, source], 0, raw / "emit")
    assert not emitted.stdout and not emitted.stderr
    generated = pristine.read_bytes()
    assert generated.count(b"return nl_local_") == (2 if early_return else 1)
    assert b"if (nl_condition_" in generated and b"} else {" in generated
    return_in_arm = re.search(rb"return nl_local_\d+;\n    } else \{", generated)
    assert bool(return_in_arm) == early_return
    instrumented = raw / "observed.c"
    instrumented.write_bytes(observed(generated))

    compilers = (shutil.which("gcc"), shutil.which("clang-23") or shutil.which("clang"))
    assert all(compilers), compilers
    for compiler in compilers:
        for optimization in ("-O0", "-O2"):
            label = f"{Path(compiler).name}-{optimization[1:]}"
            for name, c_source, output in (
                ("pristine", pristine, b""),
                ("observed", instrumented, b"12\n5\n"),
            ):
                binary = raw / f"{name}-{label}"
                run(
                    [
                        compiler,
                        "-std=c17",
                        "-Wall",
                        "-Wextra",
                        "-Wpedantic",
                        "-Werror",
                        optimization,
                        c_source,
                        "-o",
                        binary,
                    ],
                    0,
                    raw / f"{name}-{label}-compile",
                )
                native = run([binary], 0, raw / f"{name}-{label}-native")
                assert native.stdout == output and not native.stderr
    print(f"{source.name} C17 GCC/Clang O0/O2 true=12 false=5 PASS")


def file_identity(path):
    info = path.stat()
    return {
        "device": info.st_dev,
        "inode": info.st_ino,
        "mode": info.st_mode,
        "size": info.st_size,
        "mtime_ns": info.st_mtime_ns,
    }


def reject_c17(cli, source, raw):
    check(cli, source, raw)
    diagnostic = b"error(cli)[V1-BACKEND-UNSUPPORTED]:"
    standard = run([cli, source], 4, raw / "stdout-emit")
    assert not standard.stdout and standard.stderr.startswith(diagnostic)

    output_dir = raw / "named"
    output_dir.mkdir()
    target = output_dir / "keep.c"
    sentinel = b"keep existing C17\n\x00untouched\n"
    target.write_bytes(sentinel)
    before = file_identity(target)
    (raw / "named-before.bin").write_bytes(sentinel)
    (raw / "named-before.json").write_text(json.dumps(before), encoding="ascii")
    named = run([cli, "--emit-c17-to", target, source], 4, raw / "named-emit")
    after = file_identity(target)
    actual = target.read_bytes()
    (raw / "named-after.bin").write_bytes(actual)
    (raw / "named-after.json").write_text(json.dumps(after), encoding="ascii")
    assert not named.stdout and named.stderr.startswith(diagnostic)
    assert actual == sentinel and before == after
    assert list(output_dir.iterdir()) == [target]
    print(f"{source.name} CHECK=0 C17=4 stdout=0 named=no-clobber PASS")


def main():
    cli, evidence_arg, mode, *sources = sys.argv[1:]
    root = Path(evidence_arg)
    root.mkdir(parents=True, exist_ok=True)
    raw = Path(tempfile.mkdtemp(dir=root))
    print(f"evidence={raw}", flush=True)

    if mode == "early-return":
        (source,) = map(Path, sources)
        assert hashlib.sha256(source.read_bytes()).hexdigest() == EXPECTED_SOURCE
        exercise_native(cli, source, raw, early_return=True)
    elif mode == "scope":
        rejected, normal = map(Path, sources)
        source_bytes = rejected.read_bytes()
        assert hashlib.sha256(source_bytes).hexdigest() == SCOPE_SOURCE
        assert normal.read_bytes() == source_bytes.replace(
            b"        return quotient;\n", b"        quotient\n", 1
        )
        reject_c17(cli, rejected, raw)
        control = raw / "normal-if"
        control.mkdir()
        exercise_native(cli, normal, control, early_return=False)
    else:
        raise ValueError(f"unknown mode: {mode}")


if __name__ == "__main__":
    main()
