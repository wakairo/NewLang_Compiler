from contextlib import nullcontext
from pathlib import Path
import re
import runpy
import shutil
import sys
from tempfile import TemporaryDirectory


here = Path(__file__).resolve().parent
support = runpy.run_path(str(here.parent / "p1682" / "run.py"))
run, state = support["run"], support["state"]
admitted = {
    "scalar-1023": b"1023,65472,97\n",
    "scalar-1024": b"1024,0,41\n",
    "scalar-tail": b"1023,65472,97\n",
    "scalar-bind-only": b"1023,65472,97\n",
    "selector-known-only": None,
}
refused = {
    "descriptor-1023": 4,
    "descriptor-1024": 4,
    "wrong-actual": 4,
    "copy-alias": 4,
    "extra-call": 4,
    "bound-local": 4,
    "too-wide": 3,
}


def observe(c):
    signature = re.search(
        rb"^static uint8_t (nl_qty_fn_\d+)\(size_t (nl_local_\d+)\)\n\{\n"
        rb"    return \2 < [^;\n]+ \? \(\(uint8_t\)UINT8_C\(\d+\)\)"
        rb" : \(\(uint8_t\)UINT8_C\(\d+\)\);$", c, re.M,
    )
    assert signature, c
    bindings = re.search(
        rb"^    size_t (nl_local_\d+) = nl_qty_fn_\d+\((nl_local_\d+)\);\n"
        rb"    \(void\)\1;\n    uint8_t (nl_local_\d+) = " + signature[1] +
        rb"\(\1\);$", c, re.M,
    )
    assert bindings, c
    at = bindings.end()
    return (b"#include <stdio.h>\n" + c[:at] + b'\n    printf("%zu,%zu,%u\\n", ' +
            bindings[2] + b", " + bindings[1] + b", (unsigned)" + bindings[3] +
            b");" + c[at:])


def verify(compiler, sources, raw):
    gcc = shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    assert gcc and clang
    for name in (*admitted, *refused):
        work = raw / name
        work.mkdir()
        source = sources / (name + ".nl")
        status = refused.get(name, 0)
        check = run(work, "check", [compiler, "--check", source], 3 if status == 3 else 0)
        emit = run(work, "emit", [compiler, source], status)
        target, absent = work / "existing.c", work / "absent.c"
        sentinel = b"/* preserve original C17 output */\n"
        target.write_bytes(sentinel)
        before = state(target)
        existing = run(work, "named-existing", [compiler, "--emit-c17-to", target, source], status)
        created = run(work, "named-absent", [compiler, "--emit-c17-to", absent, source], status)
        after = state(target)
        (work / "stat.txt").write_text(f"{before}\n{after}\n")
        assert not check.stdout and not existing.stdout and not created.stdout
        assert not list(work.glob("*.tmp.*"))
        for result in (check, emit, existing, created):
            assert b"Sanitizer" not in result.stderr and b"runtime error:" not in result.stderr
        if status:
            assert not emit.stdout and not absent.exists()
            assert before == after and target.read_bytes() == sentinel
            code = (b"[USIZE-MULTIPLICATION-PRECISION]" if status == 3
                    else b"[V1-BACKEND-UNSUPPORTED]")
            assert all(code in result.stderr for result in (emit, existing, created))
            assert (code in check.stderr if status == 3 else not check.stderr)
            print(f"{name} C17={status} named outputs preserved")
            continue
        assert not any(result.stderr for result in (check, emit, existing, created))
        assert emit.stdout == target.read_bytes() == absent.read_bytes()
        expected = admitted[name]
        if expected:
            observed = work / "observed.c"
            observed.write_bytes(observe(emit.stdout))
            for label, host in (("gcc", gcc), ("clang", clang)):
                support["native"](work, host, [
                    (label + "-original", target, b""),
                    (label + "-observed", observed, expected),
                ])
            print(f"{name} count,bytes,mode={expected.decode().strip()}")
        else:
            print(f"{name} CHECK=0 C17=0 named outputs match")


if __name__ == "__main__":
    cli, sources = sys.argv[1:3]
    if len(sys.argv) == 4:
        destination = Path(sys.argv[3]).resolve()
        destination.mkdir()
        scope = nullcontext(destination)
    else:
        scope = TemporaryDirectory(prefix="p1712-")
    with scope as directory:
        verify(Path(cli).resolve(), Path(sources).resolve(), Path(directory))
