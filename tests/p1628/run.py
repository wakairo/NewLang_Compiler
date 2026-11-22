"""Exercise ordinary implicit unit completion after real quantity work."""

import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import sys
import tempfile


FIXTURES = Path(__file__).resolve().parent
P1623_SPEC = importlib.util.spec_from_file_location(
    "p1623_runner", FIXTURES.parent / "p1623" / "run.py"
)
p1623 = importlib.util.module_from_spec(P1623_SPEC)
P1623_SPEC.loader.exec_module(p1623)
harness = p1623.harness


def strict_plain(case, source, compilers):
    for cc in compilers:
        label = Path(cc).name
        binary = case / f"pristine-{label}.bin"
        harness.run(
            [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-O2", source, "-o", binary],
            0,
            case / f"pristine-{label}-compile",
        )
        result = harness.run([binary], 0, case / f"pristine-{label}-native")
        assert not result.stdout and not result.stderr


def main():
    cli, trace_probe, unit_probe, evidence = sys.argv[1:]
    for row in (FIXTURES / "SHA256SUMS").read_text().splitlines():
        expected, path = row.split()
        assert hashlib.sha256((FIXTURES / path).read_bytes()).hexdigest() == expected, path
    oracle = json.loads((FIXTURES / "oracle.json").read_bytes())
    compilers = (shutil.which("gcc"), shutil.which("clang-23") or shutil.which("clang"))
    assert all(compilers), compilers
    Path(evidence).mkdir(parents=True, exist_ok=True)
    raw = Path(tempfile.mkdtemp(dir=evidence))
    print(f"evidence={raw}", flush=True)

    for name, expected in oracle.items():
        case = raw / name
        case.mkdir()
        source = FIXTURES / "source" / f"{name}.nl"
        pristine = case / "pristine.c"
        checked = harness.run(
            [cli, "--check", source], expected.get("check", 0), case / "check"
        )
        if "emit" in expected:
            diagnostic = expected["diagnostic"].encode()
            assert diagnostic in checked.stderr
            pristine.write_bytes(b"keep existing named output\0\x81\n")
            before = harness.file_state(pristine)
            emitted = harness.run(
                [cli, "--emit-c17-to", pristine, source],
                expected["emit"],
                case / "emit",
            )
            assert diagnostic in emitted.stderr
            assert before == harness.file_state(pristine)
            print(f"{name}: fail-closed; named output preserved")
            continue

        assert not checked.stdout and not checked.stderr
        emitted = harness.run(
            [cli, "--emit-c17-to", pristine, source], 0, case / "emit"
        )
        assert not emitted.stdout and not emitted.stderr
        if expected.get("plain"):
            strict_plain(case, pristine, compilers)
            print(f"{name}: ordinary unit / GCC and Clang native PASS")
            continue

        if expected.get("implicit"):
            inspected = harness.run(
                [unit_probe, "inspect", source], 0, case / "implicit-proof"
            )
            assert b"ENTRY implicit" in inspected.stdout
        proof = harness.run([trace_probe, "trace", source], 0, case / "proof")
        functions, signatures = p1623.checked_facts(proof.stdout, expected)
        observed = case / "observed.c"
        observed.write_bytes(
            p1623.observe(pristine.read_bytes(), functions, signatures)
        )
        p1623.native(case, (pristine, observed), compilers, expected["trace"])
        print(f"{name}: source-owned unit completion / C17 / native PASS")


if __name__ == "__main__":
    main()
