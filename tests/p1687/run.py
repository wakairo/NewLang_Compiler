from contextlib import nullcontext
import csv
from pathlib import Path
import re
import runpy
import shutil
import sys
from tempfile import TemporaryDirectory

here = Path(__file__).resolve().parent
support = runpy.run_path(str(here.parent / "p1682" / "run.py"))
run, state, native = (support[name] for name in ("run", "state", "native"))


def observe(c, direct, expected_calls):
    header = re.search(rb"^static size_t (nl_qty_fn_\d+)\(void\)\n\{\n", c, re.M)
    assert header, c
    end = c.index(b"\n}\n", header.end())
    helper, caller = c[header.end():end], c[end:]
    staged = re.search(
        rb"^    const size_t nl_product_left = [^;]+;\n"
        rb"    const size_t nl_product_right = [^;]+;\n", helper, re.M,
    )
    assert staged and helper.count(b"nl_product_left * nl_product_right") == 1, c
    returned = re.search(rb"^    return ([^;]+);$", helper, re.M)
    assert returned and staged.end() <= returned.start(), c
    if direct:
        assert returned[1] == b"nl_product_left * nl_product_right", c
    else:
        product = re.match(rb"    size_t (nl_local_\d+) = "
                           rb"nl_product_left \* nl_product_right;\n",
                           helper[staged.end():])
        assert product, c
        origin = product[1]
        for alias, value in re.findall(
            rb"^    size_t (nl_local_\d+) = (nl_local_\d+);$", helper, re.M,
        ):
            if value == origin:
                origin = alias
        assert returned[1] == origin, c
    calls = re.findall(rb"^    (size_t nl_local_\d+ = |\(void\)\()" + header[1] +
                       rb"\(\)\)?;$", caller, re.M)
    expected = [b"let", b"discard", b"let"] if expected_calls == 3 else [b"let"]
    assert [b"discard" if call.startswith(b"(void)") else b"let"
            for call in calls] == expected, c
    lines = (b"    const size_t observed = " + returned[1] + b";\n"
             b'    printf("%zu\\n", observed);\n    return observed;')
    return (b"#include <stdio.h>\n" + c[:header.end()] +
            helper[:returned.start()] + lines + helper[returned.end():] + caller)


def verify(cli, sources, raw):
    gcc = shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    assert gcc and clang, (gcc, clang)
    with (sources / "oracle.tsv").open() as f:
        cases = list(csv.reader(f, delimiter="\t"))[1:]
    for name, check_status, _, status, values, reason in cases:
        work = raw / name
        work.mkdir()
        source = sources / (name + ".nl")
        check = run(work, "check", [cli, "--check", source], int(check_status))
        emit = run(work, "emit", [cli, source], int(status))
        target, absent = work / "existing.c", work / "absent.c"
        sentinel = b"/* original C17 output */\n"
        target.write_bytes(sentinel)
        before = state(target)
        existing = run(work, "named-existing",
                       [cli, "--emit-c17-to", target, source], int(status))
        created = run(work, "named-absent",
                      [cli, "--emit-c17-to", absent, source], int(status))
        after = state(target)
        (work / "stat.txt").write_text(f"{before}\n{after}\n")
        assert not check.stdout and not existing.stdout and not created.stdout
        assert not list(work.glob("*.tmp.*"))
        for result in (check, emit, existing, created):
            assert b"Sanitizer" not in result.stderr and b"runtime error:" not in result.stderr
        if status == "0":
            assert emit.stdout == target.read_bytes() == absent.read_bytes()
            assert not any(result.stderr for result in (check, emit, existing, created))
            result_values = values.split(",")
            observed = work / "observed.c"
            observed.write_bytes(observe(emit.stdout, name == "direct-control",
                                         len(result_values)))
            expected = "".join(v + "\n" for v in result_values).encode()
            for label, driver in (("gcc", gcc), ("clang", clang)):
                native(work, driver, [(label + "-original", target, b""),
                                      (label + "-observed", observed, expected)])
        else:
            assert not emit.stdout and not absent.exists()
            assert before == after and target.read_bytes() == sentinel
            expected = b"[" + reason.encode() + b"]"
            assert all(expected in r.stderr for r in (emit, existing, created))
            assert (not check.stderr if check_status == "0" else expected in check.stderr)
        print(f"{name} CHECK={check.returncode} C17={emit.returncode} named verified")


if __name__ == "__main__":
    cli, sources = sys.argv[1:3]
    if len(sys.argv) == 4:
        destination = Path(sys.argv[3]).resolve()
        destination.mkdir()
        scope = nullcontext(destination)
    else:
        scope = TemporaryDirectory(prefix="p1687-")
    with scope as directory:
        verify(Path(cli).resolve(), Path(sources).resolve(), Path(directory))
