#!/usr/bin/env python3
"""R1288 frozen-source layer trace. Runs only in an authorized GitHub Actions CTest.

An observed compiler diagnostic is evidence of *where* execution stopped, not
proof of the negative's intended guard. Neither C17 emission nor a native
exit code validates internal source Copy values, public owned, or OOM worlds.
"""
import hashlib
import pathlib
import re
import subprocess
import sys
import tempfile

CASES = (
    ("P1", "f1e802e28026c127e349af3e37fa6b6acc8cfeff30ec57fb53d887240b54a854", "153ec7391c018fc3c4b54f273f0b7c93cf264fe41588cce797e26e558eae62b9", True, 0),
    ("N1", "3feaf8da92c7346b1d382ff6fffb9d8db7225f6230bb03dd320a9e372eb14582", "8bf00b4cff64083a32d7479efae5ce27a21a6b62f8410d48175d167161e6900e", False, 92),
    ("N2", "138ca20ebd63a118ac1256436245b26d33c241147e881ca0d450b6e9df82ab05", "2910af955be3b2f67c612cf450df3ad810fed6047d0bb043cc711e69f923c64b", False, 93),
    ("N3", "4c4dbd96f4addc4e0e979b6c98401ca7e0277082da5151055d16af6ad1eb9863", "5e2c9a924be1b65b5298aafc38c7aaee08faf874328d7746ba499f3faddaea63", False, 96),
    ("N4", "afa92183d07b60f92fb55ea4c7351f6ad2f7343161c58eb06b91b9c9389f26a9", "7b999dda0d81919c78481d9a5892390edf369884c71e723ec657c721d3aa14eb", False, 125),
)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def observe(label, command, timeout=40):
    print("R1288_COMMAND " + label + " argv=" + repr(command), flush=True)
    try:
        actual = subprocess.run(command, capture_output=True, timeout=timeout,
                                check=False)
    except (OSError, subprocess.TimeoutExpired) as error:
        print(f"R1288_EXECUTION_ERROR label={label} detail={error!r}", flush=True)
        raise
    # Hex is lossless raw-bytes preservation even if stderr has non-UTF8.
    print(f"R1288_ACTUAL label={label} rc={actual.returncode} "
          f"stdout_hex={actual.stdout.hex()} stderr_hex={actual.stderr.hex()}",
          flush=True)
    return actual


def positive_native(cli, host_cc, sanitizer, src):
    errors = []
    with tempfile.TemporaryDirectory(prefix="r1288-") as temporary:
        output = pathlib.Path(temporary) / "frozen_positive.c"
        emitted = observe("P1_C17_EMIT", [cli, "--emit-c17-to", str(output), str(src)])
        if emitted.returncode != 0 or not output.is_file():
            print("R1288_LAYER P1 C17=FAIL_OR_NOT_REACHED native=NOT_RUN "
                  "public_owned=NOT_VERIFIED", flush=True)
            return ["P1_C17_NOT_EMITTED"]
        data = output.read_bytes()
        if not data:
            print("R1288_LAYER P1 C17=EMPTY native=NOT_RUN", flush=True)
            return ["P1_C17_EMPTY"]
        print(f"R1288_LAYER P1 C17=EMITTED bytes={len(data)} "
              f"sha256={sha(data)} public_owned=NOT_VERIFIED", flush=True)
        exe = pathlib.Path(temporary) / "frozen_positive"
        args = [host_cc, "-std=c17", "-pedantic-errors",
                "-Wall", "-Wextra", "-Werror", "-O0", str(output),
                "-o", str(exe)]
        if sanitizer in ("address", "undefined"):
            args += ["-fsanitize=" + sanitizer, "-fno-omit-frame-pointer",
                     "-fno-sanitize-recover=all"]
        built = observe("P1_STRICT_C17_NATIVE_BUILD", args, timeout=70)
        if built.returncode != 0 or not exe.is_file():
            print("R1288_LAYER P1 strict_C17=FAIL native=NOT_RUN", flush=True)
            return ["P1_STRICT_C17_FAILED"]
        print("R1288_LAYER P1 strict_C17=BUILT native=ATTEMPTED", flush=True)
        executed = observe("P1_NATIVE_EXEC", [str(exe)], timeout=40)
        if executed.returncode != 0:
            errors.append("P1_NATIVE_NONZERO")
        print(f"R1288_LAYER P1 native_exit={executed.returncode} "
              f"sanitizer={sanitizer if sanitizer in ('address','undefined') else 'none'} "
              "public_owned=NOT_VERIFIED internal_T_values=NOT_OBSERVED "
              "OOM_worlds=NOT_RUN", flush=True)
    return errors


def main():
    if len(sys.argv) != 5:
        raise SystemExit("R1288 unexpected arguments")
    cli, host_cc, sanitizer, directory = sys.argv[1:]
    root = pathlib.Path(directory).resolve(strict=True)
    if not pathlib.Path(cli).is_file():
        raise SystemExit("R1288 compiled newlangc unavailable")
    faults = []
    for case, expected_sha, oracle_sha, is_positive, target_line in CASES:
        src = root / "sources" / (case + ".nl")
        oracle = root / "oracles" / (case + ".txt")
        if src.is_symlink() or oracle.is_symlink():
            raise SystemExit("R1288 symlink source/oracle forbidden")
        data = src.read_bytes()
        given_oracle = oracle.read_bytes()
        if sha(data) != expected_sha or sha(given_oracle) != oracle_sha:
            raise SystemExit("R1288 FROZEN_SOURCE_OR_ORACLE_SHA_DRIFT: " + case)
        print(f"R1288_FROZEN case={case} bytes={len(data)} "
              f"sha256={expected_sha} oracle_sha256={oracle_sha}", flush=True)
        actual = observe(case + "_SOURCE_CHECK", [cli, "--check", str(src)])
        stderr = actual.stderr.decode("utf-8", "replace")
        match = re.search(
            r":(\d+):\d+-\d+:\d+: error\((parser|semantic|cli)\)\[([A-Za-z0-9-]+)\]:",
            stderr)
        if is_positive:
            if actual.returncode != 0 or actual.stderr or actual.stdout:
                print(f"R1288_LAYER {case} CHECK=NOT_ACCEPTED "
                      f"first_diag={match.groups() if match else 'UNCLASSIFIED'} "
                      "owned=NOT_RUN C17=NOT_RUN native=NOT_RUN "
                      "OOM=NOT_RUN ASan=NOT_RUN UBSan=NOT_RUN",
                      flush=True)
                faults.append("P1_CHECK_NOT_ACCEPTED")
            else:
                print(f"R1288_LAYER {case} SOURCE_CHECK=ACCEPTED "
                      "public_owned=NOT_YET_VERIFIED", flush=True)
                faults.extend(positive_native(cli, host_cc, sanitizer, src))
        else:
            if actual.returncode == 0:
                print(f"R1288_UNSAFE_OR_UNEXPECTED case={case} "
                      "negative_CHECK=ACCEPTED: STOP C", flush=True)
                faults.append(case + "_NEGATIVE_ACCEPTED")
                continue
            if actual.returncode not in (2, 3, 4) or not match:
                print(f"R1288_LAYER {case} CHECK=UNCLASSIFIED_REJECTION "
                      "target_guard=NOT_VERIFIED deeper=NOT_RUN", flush=True)
                faults.append(case + "_REJECTION_UNCLASSIFIED")
                continue
            observed_line = int(match.group(1))
            category, code = match.group(2), match.group(3)
            # Target-site classification is deliberately weaker than a proof
            # that the precisely predicted semantic invariant caused rejection.
            target = (observed_line == target_line and
                      (category == "semantic" or
                       (case == "N2" and category == "parser")))
            print(f"R1288_LAYER {case} REJECTED diag_category={category} "
                  f"code={code} line={observed_line} "
                  f"predicted_target_line={target_line} "
                  f"target_site={'REACHED' if target else 'NOT_VERIFIED'} "
                  "owned=NOT_RUN C17=NOT_RUN native=NOT_RUN OOM=NOT_RUN",
                  flush=True)
            if not target:
                faults.append(case + "_TARGET_SAFETY_GATE_NOT_VERIFIED")
    print("R1288_NOTICE raw SOURCE/CHECK is actual CLI execution; "
          "public owned, internal Copy physical identity and failure worlds "
          "need independent evidence and are NOT proved by CHECK/native success",
          flush=True)
    if faults:
        print("R1288_STOP_OR_INCONCLUSIVE faults=" + ",".join(faults), flush=True)
        raise SystemExit(1)
    print("R1288_LAYER_TRACE_COMPLETED (bounded observations, NOT product proof)",
          flush=True)


if __name__ == "__main__":
    main()
