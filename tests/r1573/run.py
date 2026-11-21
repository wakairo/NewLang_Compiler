#!/usr/bin/env python3
"""R1573 original CLI and native observation (GitHub Actions only)."""
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

from verify import check as verify_frozen

ROOT = Path(__file__).resolve().parent
POSITIVE = ("P1_zero_helper_unit_tail.nl",
            "P2_mixed_helper_final_unit.nl")
INVALID = ("N1_zero_divisor.nl", "N2_unit_result_mismatch.nl")
EXCLUDED = ("C1_named_nonzero_divisor.nl",
            "C2_numeric_named_main.nl")

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def command(args):
    result = subprocess.run(args, capture_output=True, timeout=120,
                            check=False)
    print(f"R1573_PROCESS|{' '.join(map(str, args))}|exit={result.returncode}"
          f"|stdout_hex={result.stdout.hex()}|stderr_hex={result.stderr.hex()}",
          flush=True)
    return result

def require(condition, name):
    if not condition:
        raise AssertionError("R1573_STOP: " + name)

def read_only_copy(raw, path):
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError:
        print("R1573_OBSERVER_NOT_OBSERVABLE|C17_non_utf8", flush=True)
        return None
    match = re.compile(
        r"^\s*(?:const\s+)?(?:size_t|uint(?:8|16|32|64)_t|"
        r"unsigned\s+(?:int|long(?:\s+long)?))\s+"
        r"([A-Za-z_]\w*)\s*=\s*[^;]*\s[/\%]\s[^;]*;\s*$")
    inserted = []
    parts = [b"#include <stdio.h>\n"]
    for line in text.splitlines(keepends=True):
        parts.append(line.encode("utf-8"))
        found = match.fullmatch(line.rstrip("\n"))
        if found:
            name = found.group(1)
            indent = line[:len(line) - len(line.lstrip())]
            extra = (f'{indent}fprintf(stderr, "R1573_NATIVE_VALUE|'
                     f'{name}=%zu\\n", (size_t){name});\n').encode()
            parts.append(extra)
            inserted.append(extra)
    if not inserted:
        print("R1573_OBSERVER_NOT_OBSERVABLE|no_safe_C17_lvalue", flush=True)
        return None
    derived = b"".join(parts)
    restored = derived.removeprefix(b"#include <stdio.h>\n")
    for addition in inserted:
        require(restored.count(addition) == 1, "observer_copy_ambiguous")
        restored = restored.replace(addition, b"", 1)
    require(restored == raw, "byte_exact_C17_reconstruction")
    path.write_bytes(derived)
    print(f"R1573_OBSERVER_COPY|source_sha256={sha(raw)}"
          f"|observer_sha256={sha(derived)}|n={len(inserted)}"
          f"|reconstruct_exact=1", flush=True)
    return path

def native(pristine, source_name, directory):
    found = []
    for name in ("gcc-13", "gcc", "clang-23", "clang"):
        path = shutil.which(name)
        if path is not None and path not in found:
            found.append(path)
    require(any("gcc" in Path(path).name for path in found), "GCC_missing")
    observer = read_only_copy(pristine.read_bytes(),
                              directory / (source_name + ".observed.c"))
    for cc in found:
        for optimization in ("-O0", "-O2"):
            tag = re.sub("[^A-Za-z0-9]+", "_", Path(cc).name)
            executable = directory / f"{source_name}.{tag}.{optimization[1:]}"
            flags = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic",
                     "-Werror", optimization]
            require(command(flags + [str(pristine), "-o",
                                     str(executable)]).returncode == 0,
                    "pristine_native_build")
            actual = command([str(executable)])
            require(actual.returncode == 0 and not actual.stdout
                    and not actual.stderr, "pristine_native_unit_contract")
            print(f"R1573_NATIVE_PRISTINE|{source_name}|{cc}|"
                  f"{optimization}|exit0|internal_numeric=NOT_OBSERVABLE",
                  flush=True)
            if observer is not None:
                exe2 = directory / (executable.name + ".observer")
                require(command(flags + [str(observer), "-o",
                                         str(exe2)]).returncode == 0,
                        "observer_native_build")
                observed = command([str(exe2)])
                require(observed.returncode == 0 and not observed.stdout
                        and b"R1573_NATIVE_VALUE|" in observed.stderr,
                        "observer_native_values")
                print(f"R1573_NATIVE_OBSERVER|{source_name}|{cc}|"
                      f"{optimization}|mapping_to_source=NOT_OBSERVABLE"
                      f"_WITHOUT_PUBLIC_LINK", flush=True)

def main():
    require(len(sys.argv) == 2, "newlangc_argument_required")
    verify_frozen()
    compiler = sys.argv[1]
    with tempfile.TemporaryDirectory(prefix="r1573_") as work:
        output = Path(work)
        for name in POSITIVE:
            source = ROOT / "sources" / name
            checked = command([compiler, "--check", str(source)])
            require(checked.returncode == 0 and not checked.stdout
                    and not checked.stderr, "positive_checked_" + name)
            emitter = command([compiler, str(source)])
            require(emitter.returncode == 0 and emitter.stdout
                    and not emitter.stderr, "positive_emit_" + name)
            named = output / (name + ".named.c")
            emitted_to = command([compiler, "--emit-c17-to", str(named),
                                  str(source)])
            require(emitted_to.returncode == 0
                    and named.read_bytes() == emitter.stdout,
                    "C17_byte_exact_two_routes_" + name)
            raw = output / (name + ".raw.c")
            raw.write_bytes(emitter.stdout)
            print(f"R1573_C17_RAW|{name}|size={len(emitter.stdout)}"
                  f"|sha256={sha(emitter.stdout)}|two_routes_equal=1",
                  flush=True)
            native(raw, name, output)
        for name in INVALID:
            result = command([compiler, "--check",
                              str(ROOT / "sources" / name)])
            require(result.returncode != 0, "invalid_accepted_" + name)
            print(f"R1573_TRUE_INVALID_REJECT|{name}|"
                  "exact_diagnostic_layer=NOT_ATTESTED_BY_EXIT",
                  flush=True)
        for name in EXCLUDED:
            result = command([compiler, "--check",
                              str(ROOT / "sources" / name)])
            print(f"R1573_LEGAL_EXCLUDED|{name}|exit={result.returncode}|"
                  "no_mandatory_reject_oracle", flush=True)
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as fp:
            fp.write("\n### R1573 C1298 original C17/native evidence\n"
                     "Original FIRST was byte/SHA-verified. Pristine emitted "
                     "C17 was checked and executed; native exit0 cannot "
                     "authenticate internal scalar values. "
                     "Unobservable links remain NOT_OBSERVABLE.\n")
    print("R1573_CLI_NATIVE_EXECUTION_COMPLETE", flush=True)

if __name__ == "__main__":
    try:
        main()
    except (AssertionError, OSError, subprocess.TimeoutExpired) as exc:
        print(f"R1573_STOP|{exc}", file=sys.stderr, flush=True)
        raise SystemExit(1) from exc
