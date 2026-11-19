"""Frozen scalar-helper sources through ordinary CLI/API and physical H worlds."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import time


def main():
    compiler, cc, api, tests_arg, output_arg, sanitizer = sys.argv[1:]
    tests = Path(tests_arg).resolve()
    output = Path(output_arg).resolve()
    output.mkdir(parents=True, exist_ok=True)
    # Reuse the existing passive observer and malloc/free wrapper unchanged.
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location(
        "p1535_native", tests.parent / "p1535/native.py")
    apparatus = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(apparatus)
    records = []

    def run(label, argv, native=0, env=None):
        started = time.monotonic()
        result = subprocess.run(list(map(str, argv)), capture_output=True,
                                env=dict(os.environ, **(env or {})))
        (output / (label + ".stdout")).write_bytes(result.stdout)
        (output / (label + ".stderr")).write_bytes(result.stderr)
        records.append(dict(label=label, argv=list(map(str, argv)),
                            selected=2 if label.endswith("-cc") else 1,
                            native=native, exit=result.returncode, env=env,
                            seconds=time.monotonic() - started))
        (output / "commands.json").write_text(json.dumps(records, indent=2))
        assert result.returncode == 0, (label, result.returncode, result.stderr)
        if label.endswith("-check"):
            assert not result.stdout and not result.stderr, label

    for variant, sample in [("zero", 0), ("nonzero", 37)]:
        source = tests / ("P-" + variant + ".nl")
        raw = output / (variant + ".c")
        run(variant + "-check", [compiler, "--check", source])
        run(variant + "-emit", [compiler, "--emit-c17-to", raw, source])
        run(variant + "-api", [api, "agree", source, raw])
        exe = output / variant
        extra = (dict(initial=83, sibling=27, first=83, prefix=2)
                 if variant == "nonzero" else {})
        apparatus.compile_guest(run, cc, raw, tests.parent / "p1535", exe,
                                sample, sanitizer, **extra)
        apparatus.execute_worlds(run, exe)
    selected = sum(record["selected"] for record in records)
    guests = sum(record["native"] for record in records)
    assert selected == 16 and guests == 6
    print("P1539 CLI CHECK/API bytes, scalar-return -> original T, "
          "strict C17 O2 zero/37 H0/H1/H2 PASS")
    print(f"P1539 cost selected={selected} guest={guests}")


if __name__ == "__main__":
    main()
