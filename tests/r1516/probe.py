#!/usr/bin/env python3
"""Independent R1516 calibrated V2 original frozen source and native observer.
No local/P runtime: this script runs only as a CTest in GitHub Actions.
"""
import hashlib
import pathlib
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
MANIFEST = {
    "first_oracle.txt": (5963, "ec416ef331be20c5e65905d482f2130b9f2753b8f07f708b08cc2a1edf1f3fcb"),
    "V2P0-real-zero-field0.nl": (3241, "9d1b7c2e2f4086043369a1893ba77bab4984978cea86310bd01f569f5e3ae480"),
    "V2P1-real-229-field1.nl": (3303, "735a5243837ea9d3d9554c10fb1030622fad00998ea05d122e870de0769bc295"),
    "V2N1-foreign-domain.nl": (3242, "2d23e0c3e07bf5bbdb7473f80f29e36fbc36bf5b889ae760b5d7c229b41f038c"),
    "V2N2-post-EndRoot-read.nl": (3241, "b77cd71697e117b9222a220e49e59806a616dbb9474be7445f45febad90b25ad"),
    "V2N3-wrong-allocation.nl": (3242, "a45e7e7b973f858e59b8f970f51e3154f30fa9e13d50db00ec800db9b1f1955e"),
    "V2N4-wrong-named-formal.nl": (3242, "37b7394d2e8830acc98b08d509a1fce27eaff55cc2c88575639d0aae0db168b2"),
}
POSITIVES = (
    ("V2P0-real-zero-field0.nl", "zero"),
    ("V2P1-real-229-field1.nl", "high"),
)
NEGATIVES = {
    "V2N1-foreign-domain.nl": "ALLOCATED-DOMAIN-MISMATCH",
    "V2N2-post-EndRoot-read.nl": "P3-STALE-POINTER",
    "V2N3-wrong-allocation.nl": "P4-ALLOCATION-MISMATCH",
    "V2N4-wrong-named-formal.nl": "P3-TYPE-MISMATCH",
}


def verify():
    for name, (length, digest) in MANIFEST.items():
        path = (HERE / "sources" / name) if name.endswith(".nl") else (HERE / name)
        contents = path.read_bytes()
        actual = hashlib.sha256(contents).hexdigest()
        print("R1516 FIRST BYTES SHA", name, len(contents), actual, flush=True)
        if len(contents) != length or actual != digest:
            raise RuntimeError("STOP: calibrated V2 original FIRST tampered: " + name)
    print("R1516 FIRST INTEGRITY 7/7 PASS; eligibility NOT yet tested", flush=True)


def invoke(phase, arguments, timeout=90):
    argv = [str(x) for x in arguments]
    print("R1516 COMMAND", phase, repr(argv), flush=True)
    try:
        proc = subprocess.run(argv, capture_output=True, text=True,
                              errors="replace", timeout=timeout, check=False)
    except subprocess.TimeoutExpired as err:
        raise RuntimeError("STOP: timeout at " + phase) from err
    print("R1516 RESULT", phase, "exit", proc.returncode, flush=True)
    print("R1516 STDOUT BEGIN", phase, flush=True)
    print(proc.stdout, end="" if proc.stdout.endswith("\n") else "\n", flush=True)
    print("R1516 STDOUT END", phase, flush=True)
    print("R1516 STDERR BEGIN", phase, flush=True)
    print(proc.stderr, end="" if proc.stderr.endswith("\n") else "\n", flush=True)
    print("R1516 STDERR END", phase, flush=True)
    return proc


def native(case, generated, cc, sanitizer, temp):
    case_name = "zero" if case == "zero" else "high"
    header = temp / "r1516_passive.h"
    driver = temp / "r1516_driver.c"
    header.write_bytes((HERE / "native-observer.h.txt").read_bytes())
    driver.write_bytes((HERE / "native-driver.c.txt").read_bytes())
    for opt in ("0", "2"):
        base = ["-std=c17", "-pedantic", "-Wall", "-Wextra", "-Werror",
                "-O" + opt]
        if sanitizer in ("address", "undefined"):
            base.extend(["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all",
                         "-fno-omit-frame-pointer"])
        guest_obj = temp / ("guest_" + case_name + "_" + opt + ".o")
        driver_obj = temp / ("driver_" + case_name + "_" + opt + ".o")
        executable = temp / ("native_" + case_name + "_" + opt)
        injection = '-DNEWLANG_HEAP_OBSERVER="' + str(header) + '"'
        stages = (
            ("C17_GUEST_O" + opt, [cc, *base, "-Dmain=r1516_guest_main",
                                  injection, "-c", generated, "-o", guest_obj]),
            ("C17_OBSERVER_O" + opt, [cc, *base, "-c", driver, "-o", driver_obj]),
            ("LINK_O" + opt, [cc, *base, guest_obj, driver_obj,
                             "-Wl,--wrap=malloc", "-Wl,--wrap=free", "-o",
                             executable]),
        )
        for name, argv in stages:
            if invoke(case_name + "/" + name, argv).returncode:
                raise RuntimeError("STOP: strict C17/observer/link failure at " + name)
        for world in (0, 1, 2):
            r = invoke(case_name + "/NATIVE_O" + opt + "/H" + str(world),
                       [executable, case_name, str(world)])
            expected = "R1516_NATIVE_OK case=" + ("0" if case_name == "zero" else "1")
            if r.returncode or expected not in r.stderr or (
                " world=" + str(world)) not in r.stderr:
                raise RuntimeError("STOP C: actual native H0/H1/H2/Copy/oom failure "
                                   + case_name + " O" + opt + " H" + str(world))
    print("R1516 STRICT NATIVE OBSERVER PASS", case_name,
          "O0/O2 and H0/H1/H2 under", cc, sanitizer, flush=True)


def cohort(compiler, cc, sanitizer):
    failures = []
    with tempfile.TemporaryDirectory(prefix="r1516-independent-") as tmp:
        temp = pathlib.Path(tmp)
        for name, case in POSITIVES:
            source = HERE / "sources" / name
            check = invoke(name + "/SOURCE_CHECK", [compiler, "--check", source])
            if check.returncode:
                failures.append(name + " source/CHECK first refusal " +
                                str(check.returncode))
                print("R1516 NOT_REACHED public-owned/C17/native/oom: " + name,
                      flush=True)
                continue
            c = temp / (case + "_guest.c")
            emit = invoke(name + "/EMIT_C17",
                          [compiler, "--emit-c17-to", c, source])
            if emit.returncode or not c.is_file() or not c.read_bytes():
                failures.append(name + " C17 backend fail-closed")
                print("R1516 NOT_REACHED public-owned/C17/native/oom: " + name,
                      flush=True)
                continue
            print("R1516 EMITTED_C17", name, "bytes", c.stat().st_size,
                  "sha256", hashlib.sha256(c.read_bytes()).hexdigest(),
                  flush=True)
            try:
                native(case, c, cc, sanitizer, temp)
            except RuntimeError as exc:
                failures.append(str(exc))
                print("R1516 NATIVE RED", exc, flush=True)
                # Material real native mismatch stops the cohort; no further
                # native execution of another candidate in this CI.
                break
        for name, diagnostic in NEGATIVES.items():
            source = HERE / "sources" / name
            check = invoke(name + "/NEGATIVE_CHECK",
                           [compiler, "--check", source])
            if check.returncode == 0 or diagnostic not in check.stderr:
                failures.append(name + " unsafe/unknown negative first layer: " +
                                str(check.returncode))
                print("R1516 STOP C: negative not rejected at expected layer",
                      name, flush=True)
                break
            out = temp / (name + ".c")
            original_sentinel = b"R1516_NO_CLOBBER_SENTINEL\n"
            out.write_bytes(original_sentinel)
            emit = invoke(name + "/NEGATIVE_NO_CLOBBER",
                          [compiler, "--emit-c17-to", out, source])
            if emit.returncode == 0 or out.read_bytes() != original_sentinel:
                failures.append(name + " negative output clobbered/unsafe")
                print("R1516 STOP C: negative unsafe C17 or no-clobber", flush=True)
                break
            print("R1516 NEGATIVE EXPECTED REJECT + NO_CLOBBER PASS",
                  name, diagnostic, flush=True)
        if failures:
            for failure in failures:
                print("R1516 FIRST V2 RED/NOT_REACHED", failure, flush=True)
            raise RuntimeError("STOP C: calibrated V2 original cohort has "
                               + str(len(failures)) + " failed expectations")
    print("R1516 V2 BOUNDED PASS: two authentic native positives O0/O2 H0/H1/H2 "
          "and four semantic negatives; no general soundness or product GO",
          flush=True)


def main():
    if len(sys.argv) != 5 or sys.argv[1] not in ("integrity", "cohort"):
        raise RuntimeError("usage: probe.py integrity|cohort compiler cc sanitizer")
    mode, compiler, cc, sanitizer = sys.argv[1:]
    verify()
    if mode == "cohort":
        cohort(compiler, cc, sanitizer)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, OSError) as exc:
        print("R1516 STOP", exc, file=sys.stderr, flush=True)
        sys.exit(1)
