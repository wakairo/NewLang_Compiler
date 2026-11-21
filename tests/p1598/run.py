"""Check formal-result origins and compare pristine and observed C17."""

from hashlib import sha256
import json
from pathlib import Path
import shutil
import sys
import tempfile


FIXTURES = Path(__file__).resolve().parent
sys.path.insert(0, str(FIXTURES.parent))
from p1596.run import file_state, observed_c, run, verify_native


def verify_copies(probe, source, expected, case):
    traced = run([probe, "strict", source, expected["leaf"]], 0, case / "copy")
    seen = []
    for line in traced.stdout.decode().splitlines():
        if not line.startswith("COPY "):
            continue
        _, name, *fields = line.split()
        assert name == expected["leaf"]
        facts = dict(field.split("=") for field in fields)
        assert facts["known"] in ("0", "1")
        seen.append([int(facts["copies"]), facts["known"] == "1", int(facts["number"])])
    assert seen == expected["copies"], seen


def main():
    cli, graph_probe, copy_probe, output = sys.argv[1:]
    for row in (FIXTURES / "SHA256SUMS").read_text().splitlines():
        expected, path = row.split()
        assert sha256((FIXTURES / path).read_bytes()).hexdigest() == expected, path
    compilers = (shutil.which("gcc"), shutil.which("clang-23") or shutil.which("clang"))
    assert all(compilers), compilers
    Path(output).mkdir(parents=True, exist_ok=True)
    raw = Path(tempfile.mkdtemp(dir=output))
    print(f"evidence={raw}", flush=True)
    for name, expected in json.loads((FIXTURES / "oracle.json").read_bytes()).items():
        case = raw / name
        case.mkdir()
        source = FIXTURES / "source" / f"{name}.nl"
        pristine = case / "pristine.c"
        check_status = expected.get("check", 0)
        checked = run([cli, "--check", source], check_status, case / "check")
        assert not checked.stdout
        if "emit" in expected:
            diagnostic = expected["diagnostic"].encode()
            if check_status:
                assert diagnostic in checked.stderr
            else:
                assert not checked.stderr
            pristine.write_bytes(b"preserve this file\0\x83\n")
            before = file_state(pristine)
            rejected = run([cli, "--emit-c17-to", pristine, source], expected["emit"],
                           case / "emit")
            assert diagnostic in rejected.stderr and file_state(pristine) == before
            print(f"{name}: fail-closed with named output preserved")
            continue
        assert not checked.stderr
        if "leaf" in expected:
            verify_copies(copy_probe, source, expected, case)
        graph = run([graph_probe, "strict", source], 0, case / "graph")
        emitted = run([cli, "--emit-c17-to", pristine, source], 0, case / "emit")
        assert not emitted.stderr and not emitted.stdout
        observed = case / "observed.c"
        observed.write_bytes(observed_c(pristine.read_bytes(), graph.stdout, expected))
        verify_native(case, pristine, observed, expected, compilers)
        print(f"{name}: genuine copy/result and C17 native PASS")


if __name__ == "__main__":
    main()
