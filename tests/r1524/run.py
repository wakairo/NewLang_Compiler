#!/usr/bin/env python3
"""R1524 frozen FIRST. Runs only as the explicitly registered Actions CTest."""
import hashlib
import json
import os
import pathlib
import shlex
import shutil
import subprocess
import sys

from native_observer import instrument

CLI, PROBE, SOURCE_DIR, OUTPUT_DIR, SANITIZER = sys.argv[1:]
SOURCE_DIR = pathlib.Path(SOURCE_DIR)
OUTPUT_DIR = pathlib.Path(OUTPUT_DIR)
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
if SANITIZER not in ("none", "address", "undefined"):
    raise SystemExit("R1524 STOP unknown sanitizer configuration")


def stop(layer, detail):
    raise RuntimeError(f"R1524 STOP layer={layer}: {detail}")


def command(argv, expected=None):
    argv = [str(item) for item in argv]
    print("R1524 COMMAND " + json.dumps(argv), flush=True)
    completed = subprocess.run(
        argv, text=True, capture_output=True, timeout=60, check=False
    )
    print(f"R1524 COMMAND_EXIT {completed.returncode}", flush=True)
    if completed.stdout:
        print(completed.stdout, end="", flush=True)
    if completed.stderr:
        print(completed.stderr, end="", file=sys.stderr, flush=True)
    if expected is not None and completed.returncode != expected:
        stop("COMMAND", f"expected exit={expected}, actual={completed.returncode}")
    return completed


def metadata():
    event = {}
    event_path = os.environ.get("GITHUB_EVENT_PATH")
    if event_path:
        event = json.loads(pathlib.Path(event_path).read_text(encoding="utf-8"))
    pull = event.get("pull_request", {})
    if (os.environ.get("GITHUB_ACTIONS") != "true"
            or os.environ.get("GITHUB_EVENT_NAME") != "pull_request"
            or pull.get("head", {}).get("ref") !=
               "r-fast/r1533-prefix-replay"
            or pull.get("base", {}).get("ref") !=
               "p1531-terminal-integration-c1209"
            or pull.get("base", {}).get("sha") !=
               "b1a6f1725abe25981ad29088175de581f956f919"):
        stop("FIXED_SUBJECT", "unexpected Actions event or candidate base")
    print("R1524 ACTIONS " + json.dumps({
        "run": os.environ.get("GITHUB_RUN_ID"),
        "attempt": os.environ.get("GITHUB_RUN_ATTEMPT"),
        "event": os.environ.get("GITHUB_EVENT_NAME"),
        "ref": os.environ.get("GITHUB_REF"),
        "head_ref": os.environ.get("GITHUB_HEAD_REF"),
        "event_sha": os.environ.get("GITHUB_SHA"),
        "pr_head": event.get("pull_request", {}).get("head", {}).get("sha"),
        "pr_base": event.get("pull_request", {}).get("base", {}).get("sha"),
        "sanitizer": SANITIZER,
        "fixed_target": "b1a6f1725abe25981ad29088175de581f956f919",
    }, sort_keys=True), flush=True)
    command(["git", "rev-parse", "HEAD"], 0)
    command(["git", "show", "-s", "--format=%H%n%T%n%P", "HEAD"], 0)


def verify_first():
    manifest = json.loads((SOURCE_DIR / "first_manifest.json").read_text())
    for name, wanted in manifest.items():
        data = (SOURCE_DIR / name).read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        if len(data) != wanted["bytes"] or digest != wanted["sha256"]:
            stop("FIRST_HASH", name)
        print(f"R1524 FIRST {name} bytes={len(data)} sha256={digest}", flush=True)
    if len(manifest) != 7:
        stop("FIRST_MEMBERSHIP", str(len(manifest)))


def negative(name):
    source = SOURCE_DIR / name
    result = command([CLI, "--check", source])
    if result.returncode != 3 or result.stdout or not result.stderr:
        stop("NEGATIVE_CHECK", f"{name}: exit={result.returncode}")
    detail = command([PROBE, source])
    if detail.returncode not in (31, 32, 33):
        stop("NEGATIVE_PUBLIC_LAYER", f"{name}: exit={detail.returncode}")
    marker = b"R1524-FIRST-PRESERVE\x00\xff\n"
    output = OUTPUT_DIR / (name + ".existing.c")
    output.write_bytes(marker)
    emitted = command([CLI, "--emit-c17-to", output, source])
    if emitted.returncode != 3 or output.read_bytes() != marker:
        stop("NEGATIVE_NAMED_OUTPUT", name)
    absent = OUTPUT_DIR / (name + ".absent.c")
    if absent.exists():
        absent.unlink()
    emitted = command([CLI, "--emit-c17-to", absent, source])
    if emitted.returncode != 3 or absent.exists():
        stop("NEGATIVE_ABSENT_OUTPUT", name)
    print(f"R1524 NEGATIVE_COMPLETE {name} first_layer_exit={detail.returncode} "
          "no_clobber=1 absent_stays_absent=1 later_owned_native=NOT_TESTED",
          flush=True)


def compilers():
    result = []
    for family, candidates in (
        ("GCC", ("gcc-13", "gcc")),
        ("Clang", ("clang-23", "clang")),
    ):
        compiler = next((shutil.which(name) for name in candidates
                         if shutil.which(name)), None)
        if compiler is None:
            stop("HARNESS_ENVIRONMENT", f"{family} compiler missing")
        command([compiler, "--version"], 0)
        result.append((family, compiler))
    return result


def positive(name, payload, selected_compilers):
    source = SOURCE_DIR / name
    checked = command([CLI, "--check", source])
    if checked.returncode != 0 or checked.stdout or checked.stderr:
        stop("POSITIVE_CHECK", f"{name}: exit={checked.returncode}; "
             "owned/backend/native NOT_TESTED")
    public_c = OUTPUT_DIR / (name + ".public.c")
    public = command([PROBE, source, public_c, str(payload)])
    if public.returncode != 0:
        stop("PUBLIC_OWNED_OR_BACKEND", f"{name}: probe exit={public.returncode}; "
             "native NOT_TESTED")
    normal = command([CLI, source])
    if normal.returncode != 0 or normal.stderr or not normal.stdout:
        stop("NORMAL_C17", f"{name}: exit={normal.returncode}")
    generated = normal.stdout
    (OUTPUT_DIR / (name + ".normal.c")).write_text(generated, encoding="utf-8")
    if public_c.read_text(encoding="utf-8") != generated:
        print("R1524 OWN_NORMAL_C_BEGIN\n" + generated +
              "R1524 OWN_NORMAL_C_END", flush=True)
        stop("PUBLIC_CLI_C17_IDENTITY", name)
    print("R1524 GENERATED_C sha256=" +
          hashlib.sha256(generated.encode()).hexdigest(), flush=True)
    try:
        observed = instrument(generated)
    except Exception as error:
        print("R1524 OWN_GENERATED_C_BEGIN\n" + generated +
              "R1524 OWN_GENERATED_C_END", flush=True)
        stop("HARNESS_MAPPING", str(error))
    observed_path = OUTPUT_DIR / (name + ".observed.c")
    observed_path.write_text(observed, encoding="utf-8")
    for family, compiler in selected_compilers:
        for optimization in ("-O0", "-O2"):
            executable = OUTPUT_DIR / (
                name + "." + family + optimization + "." + SANITIZER
            )
            flags = [
                "-std=c17", optimization, "-Wall", "-Wextra",
                "-Wpedantic", "-Werror",
            ]
            if SANITIZER != "none":
                flags += [
                    "-fsanitize=" + SANITIZER,
                    "-fno-omit-frame-pointer", "-fno-sanitize-recover=all",
                ]
            compiled = command([
                compiler, *flags, observed_path, "-o", executable
            ])
            if compiled.returncode != 0:
                print("R1524 OWN_OBSERVED_C_BEGIN\n" + observed +
                      "R1524 OWN_OBSERVED_C_END", flush=True)
                stop("NATIVE_COMPILE", f"{name}/{family}/{optimization}")
            for fail_at, world in ((1, "H0"), (2, "H1"), (0, "H2")):
                result = command([executable, str(fail_at), str(payload)])
                if result.returncode != 0:
                    stop("NATIVE_OBSERVATION",
                         f"{name}/{family}/{optimization}/{world}")
                if "R1524 NATIVE_PASS" not in result.stdout:
                    stop("NATIVE_DIAGNOSTIC_VISIBILITY", world)
                print("R1524 WORLD_COMPLETE " + json.dumps({
                    "source": name, "world": world, "payload": payload,
                    "compiler": family, "optimization": optimization,
                    "sanitizer": SANITIZER, "assertions_executed": True,
                }, sort_keys=True), flush=True)
    print(f"R1524 POSITIVE_COMPLETE {name} native_world_runs=12", flush=True)


def main():
    metadata()
    verify_first()
    for name in (
        "r1524_n1_wrong_original_pointer.nl",
        "r1524_n2_wrong_allocation.nl",
        "r1524_n3_wrong_domain.nl",
        "r1524_n4_missing_callee_deallocate.nl",
    ):
        negative(name)
    selected_compilers = compilers()
    positive("r1524_p1_nonzero.nl", 211, selected_compilers)
    positive("r1524_p2_zero.nl", 0, selected_compilers)
    print("R1524 FINITE_PASS negatives=4 positives=2 native_world_runs=24 "
          "sanitizer=" + SANITIZER +
          " Product_GO_PAUSED H1_HOLD", flush=True)


try:
    main()
except Exception as failure:
    print(str(failure), file=sys.stderr, flush=True)
    sys.exit(1)
