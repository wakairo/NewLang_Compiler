import contextlib
import re
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path


class Capture:
    def __init__(self, out):
        self.out = out

    def run(self, label, *command):
        result = subprocess.run(command, capture_output=True, check=False, timeout=30)
        (self.out / (label + ".command")).write_text(shlex.join(map(str, command)) + "\n")
        (self.out / (label + ".stdout")).write_bytes(result.stdout)
        (self.out / (label + ".stderr")).write_bytes(result.stderr)
        (self.out / (label + ".status")).write_text(f"{result.returncode}\n")
        assert b"Sanitizer" not in result.stderr and b"runtime error:" not in result.stderr, result
        return result


def snapshot(path):
    stat = path.stat()
    return (stat.st_ino, stat.st_mode, stat.st_mtime_ns, stat.st_ctime_ns, path.read_bytes())


def observable(code, name):
    ordered = re.findall(
        r"    const size_t nl_mul_left_(\d+) = [^;]+;\n"
        r"    const size_t nl_mul_right_\1 = [^;]+;\n"
        r"    (?:const size_t nl_local_\d+ = |\(void\))"
        r"\(nl_mul_left_\1 \* nl_mul_right_\1\);\n",
        code,
    )
    assert len(ordered) == (8 if name == "product" else 1), (name, code)
    lines = []
    count = 0
    for line in code.splitlines(keepends=True):
        match = re.fullmatch(r"    const size_t (nl_local_\d+) = .* \* .*;\n", line)
        discard = re.fullmatch(r"    \(void\)(\(.* \* .*\));\n", line)
        if match:
            lines.extend((line, f'    printf("%zu\\n", {match[1]});\n'))
            count += 1
        elif discard:
            lines.append(f'    printf("%zu\\n", {discard[1]});\n')
            count += 1
        else:
            lines.append(line)
    assert count == (8 if name == "product" else 1), (name, code)
    return "#include <stdio.h>\n" + "".join(lines)


def verify(compiler, source, host, out):
    arithmetic = {
        "product": b"391\n493\n253\n247\n0\n23\n18446744073709551615\n0\n",
        "discard-product": b"333\n",
    }
    c = Capture(out)
    assert c.run("host-version", host, "--version").returncode == 0
    for name in (*arithmetic, "addition-control"):
        src = source / (name + ".nl")
        check = c.run(name + "-check", compiler, "--check", src)
        emit = c.run(name + "-emit", compiler, src)
        assert check.returncode == 0 and not check.stdout and not check.stderr, check
        assert emit.returncode == 0 and emit.stdout and not emit.stderr, emit
        named = out / (name + ".c")
        for phase in ("absent", "existing"):
            if phase == "existing":
                named.write_bytes(b"old output\n")
            write = c.run(name + "-named-" + phase, compiler, "--emit-c17-to", named, src)
            assert write.returncode == 0 and not write.stdout and not write.stderr, write
            assert named.read_bytes() == emit.stdout
        variants = [(name, named, b"")]
        if name in arithmetic:
            recorded = out / (name + "-observed.c")
            recorded.write_text(observable(emit.stdout.decode(), name))
            variants.append((name + "-observed", recorded, arithmetic[name]))
        for label, input_c, oracle in variants:
            for level in ("-O0", "-O2"):
                stem = label + level
                binary = out / stem
                native = c.run(stem + "-build", host, "-std=c17", "-pedantic-errors",
                               "-Wall", "-Wextra", "-Werror", level, input_c, "-o", binary)
                assert native.returncode == 0, native
                ran = c.run(stem + "-run", binary)
                assert ran.returncode == 0 and ran.stdout == oracle and not ran.stderr, ran
        print(f"{name}: CHECK + stdout/named identity + strict C17 O0/O2 PASS")

    refused = {
        "overflow": "USIZE-MULTIPLICATION-OVERFLOW",
        "overflow-reverse": "USIZE-MULTIPLICATION-OVERFLOW",
        "unknown": "USIZE-MULTIPLICATION-PRECISION",
        "unknown-zero": "USIZE-MULTIPLICATION-PRECISION",
        "wrong-type": "USIZE-MULTIPLICATION-TYPE",
        "chain": "USIZE-MULTIPLICATION-SOURCE-PRECISION",
        "grouped": "P5-EXPECTED-EXPRESSION",
        "call-operand": "USIZE-MULTIPLICATION-SOURCE-PRECISION",
        "mixed-precedence": "USIZE-ADDITION-SOURCE-PRECISION",
    }
    for name, diagnostic in refused.items():
        src = source / (name + ".nl")
        target = out / (name + ".c")
        absent = out / (name + "-absent.c")
        target.write_bytes(b"keep original named output\n")
        original = snapshot(target)
        for label, options in (
            ("check", ("--check",)),
            ("stdout", ()),
            ("named-existing", ("--emit-c17-to", target)),
            ("named-absent", ("--emit-c17-to", absent)),
        ):
            result = c.run(name + "-" + label, compiler, *options, src)
            assert result.returncode == 3 and not result.stdout, result
            assert f"[{diagnostic}]".encode() in result.stderr, result
            assert snapshot(target) == original and not absent.exists()
        print(f"{name}: {diagnostic}; no bytes published, existing output unchanged")
    assert not list(out.glob("*.tmp.*"))


if __name__ == "__main__":
    compiler, source, cc = sys.argv[1:4]
    if len(sys.argv) == 5:
        directory = Path(sys.argv[4]).resolve()
        directory.mkdir()
        scope = contextlib.nullcontext(directory)
    else:
        scope = tempfile.TemporaryDirectory(prefix="p1673-")
    with scope as raw:
        verify(Path(compiler).resolve(), Path(source).resolve(), cc, Path(raw))
