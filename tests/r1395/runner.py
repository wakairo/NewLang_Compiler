"""R1395 independent immutable-source CLI/C17/native observer driver.

Only source-derived CLI output and physical native allocation events are used.
Never interprets a post-free C pointer, derives guest authority, or changes
the immutable V1 oracle. An unused Copy field load may be optimized away.
"""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile


EXPECTED = {
    "P0": ("5c6bec74836661a44004a85531d5199152fa243a7ef0bb7e17692642b6b799a6", 16567),
    "N1": ("09046b60f803c278f1b3494d240b1b5990a14ca8c050e3fa9218a06ba3a132aa", 16564),
    "N2": ("b1579a9941b79b3374caacafa071df6bee317c23678d94b241454ff1340caaa1", 16576),
}
DIAGNOSTIC = re.compile(
    rb"error\((parser|semantic|cli)\)\[([A-Za-z0-9-]+)\]:"
)
PHYSICAL = re.compile(
    rb"R1395_PHYSICAL attempts=(\d+) allocations=(\d+) frees=(\d+) live=(\d+) anomalous=(\d+)"
)


def stop(stage, message):
    raise SystemExit("R1395_STOP " + stage + " " + message)


def execute(argv, *, env=None, timeout=35):
    try:
        return subprocess.run(argv, env=env, capture_output=True, timeout=timeout, check=False)
    except (OSError, subprocess.TimeoutExpired) as exc:
        stop("HARNESS_EXEC", repr(exc))


def source_bytes(path, label):
    data = pathlib.Path(path).read_bytes()
    digest, count = EXPECTED[label]
    actual = hashlib.sha256(data).hexdigest()
    if len(data) != count or actual != digest:
        stop("SOURCE_SHA", f"{label} size={len(data)} actual={actual} expected={digest}")
    print(f"R1395_IMMUTABLE_V1 {label} bytes={len(data)} sha256={actual}", flush=True)
    return digest


def check(cli, path, label, must_accept):
    proc = execute([cli, "--check", path])
    code = proc.returncode
    print(f"R1395_FIRST_CLI_LAYER {label} rc={code} stdout_bytes={len(proc.stdout)} stderr_bytes={len(proc.stderr)}", flush=True)
    if must_accept:
        if code != 0 or proc.stdout or proc.stderr:
            print(f"R1395_RAW_CHECK_STDERR {proc.stderr[:4096]!r}", flush=True)
            stop("P0_SOURCE_CHECK", f"positive did not pass CHECK; rc={code}; owned/C17/native NOT_REACHED")
        print("R1395_P0_SOURCE_CHECK_ACCEPTED; standalone owned evidence NOT_YET_CHECKED", flush=True)
    else:
        m = DIAGNOSTIC.search(proc.stderr)
        if code == 0:
            stop(label + "_UNSAFE_ACCEPT", "negative was accepted; native execution forbidden")
        if not m or code not in (2, 3, 4):
            print(f"R1395_RAW_CHECK_STDERR {proc.stderr[:4096]!r}", flush=True)
            stop(label + "_NONDIAGNOSTIC", f"rc={code}; not verified as compiler rejection")
        layer = m.group(1).decode("ascii")
        diag = m.group(2).decode("ascii")
        print(f"R1395_NEGATIVE_REJECT {label} first_reported_layer={layer} code={diag} rc={code}", flush=True)
        print("R1395_NEGATIVE_SEMANTIC_10_1=NOT_ESTABLISHED_BY_GENERIC_REJECTION; C17/NATIVE=NOT_REACHED", flush=True)
        print("R1395_RAW_DIAGNOSTIC " + repr(proc.stderr[:4096]), flush=True)
    return proc


def native(cli, path, cc, sanitizer, observer):
    source_bytes(path, "P0")
    check(cli, path, "P0", True)
    emission = execute([cli, path])
    if emission.returncode != 0 or not emission.stdout or emission.stderr:
        print(f"R1395_EMIT_STDERR {emission.stderr[:4096]!r}", flush=True)
        stop("P0_C17_EMIT", f"rc={emission.returncode}; NATIVE NOT_REACHED")
    print(f"R1395_C17_EMITTED bytes={len(emission.stdout)}", flush=True)
    with tempfile.TemporaryDirectory(prefix="r1395-") as temp:
        gen = pathlib.Path(temp, "p0.c")
        exe = pathlib.Path(temp, "p0")
        gen.write_bytes(emission.stdout)
        flags = ["-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror"]
        flags += ["-O2" if os.environ.get("R1395_RELEASE") == "1" else "-O0"]
        if sanitizer == "address":
            flags += ["-fsanitize=address", "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
        elif sanitizer == "undefined":
            flags += ["-fsanitize=undefined", "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
        elif sanitizer != "none":
            stop("HARNESS_CONFIG", f"unsupported sanitizer {sanitizer}")
        cmd = [cc] + flags + [str(gen), observer, "-Wl,--wrap=malloc", "-Wl,--wrap=calloc", "-Wl,--wrap=aligned_alloc", "-Wl,--wrap=free", "-o", str(exe)]
        built = execute(cmd, timeout=60)
        if built.returncode != 0:
            print(f"R1395_NATIVE_BUILD_STDERR {built.stderr[:4096]!r}", flush=True)
            stop("NATIVE_BUILD", f"status={built.returncode}; native NOT_REACHED")
        print(f"R1395_STRICT_C17_BUILD_OK compiler={cc} sanitizer={sanitizer}", flush=True)
        for fail_at in (1, 2, 3, 4, 5, 0):
            env = os.environ.copy()
            env["R1395_FAIL_AT"] = str(fail_at)
            result = execute([str(exe)], env=env, timeout=15)
            print(f"R1395_NATIVE_WORLD fail_at={fail_at} rc={result.returncode}", flush=True)
            if result.returncode != 0 or result.stdout:
                print(f"R1395_NATIVE_STDERR {result.stderr[:4096]!r}", flush=True)
                stop("NATIVE_EXECUTION", f"world={fail_at} rc={result.returncode}")
            m = PHYSICAL.search(result.stderr)
            if m is None:
                print(f"R1395_NATIVE_STDERR {result.stderr[:4096]!r}", flush=True)
                stop("PHYSICAL_NOT_REACHED", f"world={fail_at} no allocation observer marker")
            attempts, allocations, frees, live, anomalous = map(int, m.groups())
            expected = (fail_at, fail_at - 1, fail_at - 1, 0, 0) if fail_at else (5, 5, 5, 0, 0)
            print(f"R1395_OBSERVED_WORLD fail_at={fail_at} attempts={attempts} allocated={allocations} freed={frees} live={live} anomalous={anomalous}", flush=True)
            if (attempts, allocations, frees, live, anomalous) != expected:
                stop("PHYSICAL_ACCOUNTING", f"world={fail_at} expected={expected}; actual={(attempts, allocations, frees, live, anomalous)}")
    print("R1395_NATIVE_SIX_WORLDS_MATCH_PHYSICAL_ALLOC_FREE_COUNTS", flush=True)
    print("R1395_PHYSICAL_LINK_VALUE=NOT_OBSERVED; POSTFREE_B_NEXT_LOAD=NOT_OBSERVED; no freed-C access", flush=True)
    print("R1395_PUBLIC_OWNED_CERTIFICATE=SEPARATE_CTEST_NOT_INFERRED_FROM_NATIVE", flush=True)


def main(argv):
    if len(argv) != 7:
        stop("HARNESS_USAGE", "expected mode cli source label-or-cc sanitizer observer")
    mode, cli, src, label, sanitizer, observer = argv[1:]
    if mode == "native":
        native(cli, src, label, sanitizer, observer)
    elif mode in ("positive", "negative"):
        if label not in EXPECTED:
            stop("HARNESS_LABEL", label)
        source_bytes(src, label)
        check(cli, src, label, mode == "positive")
    else:
        stop("HARNESS_MODE", mode)


if __name__ == "__main__":
    main(sys.argv)
