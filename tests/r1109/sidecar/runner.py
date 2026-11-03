#!/usr/bin/env python3
"""R1109 C205: actual source -> CHECK -> generated strict C17 -> passive native.

This is R-owned/non-blind V2. Does not issue or forge public checked-owned proof.
Original frozen V1 source/oracle, first runs and P implementation untouched.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

def sha(data):
    return hashlib.sha256(data).hexdigest()

def save(outdir, name, data):
    (outdir / name).write_bytes(data if isinstance(data, bytes) else data.encode("utf-8"))

def execute(argv, timeout=60):
    p = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                       timeout=timeout, check=False)
    return {"argv": argv, "rc": p.returncode, "stdout": p.stdout.decode("utf-8", "replace"),
            "stderr": p.stderr.decode("utf-8", "replace")}

def fail_if(bad, label, result=None):
    if bad:
        raise AssertionError("R1109_C205_STOP " + label +
                             (" " + json.dumps(result, ensure_ascii=True)[:2500]
                              if result is not None else ""))

def positive(cli, src, case, gcc, clang, outdir):
    checked = execute([cli, "--check", str(src)], timeout=30)
    save(outdir, "check.json", json.dumps(checked, indent=2) + "\n")
    fail_if(checked["rc"] != 0 or checked["stdout"] or checked["stderr"],
            "positive source CHECK", checked)
    print("R1109_C205 SOURCE CHECK PASS " + src.name, flush=True)
    emitted = execute([cli, str(src)], timeout=30)
    save(outdir, "emit.json", json.dumps({
        "argv": emitted["argv"], "rc": emitted["rc"],
        "stderr": emitted["stderr"],
        "c17_sha256": sha(emitted["stdout"].encode("utf-8")),
        "c17_bytes": len(emitted["stdout"].encode("utf-8"))
    }, indent=2) + "\n")
    save(outdir, "emitted-raw.c", emitted["stdout"])
    fail_if(emitted["rc"] != 0 or emitted["stderr"] or
            not emitted["stdout"].startswith("/*") or
            "NL_LOOP_BEFORE" not in emitted["stdout"] or
            "NL_LOOP_AFTER" not in emitted["stdout"],
            "genuine source C17 emission", emitted)
    print("R1109_C205 RAW_C17_SHA256 " +
          sha(emitted["stdout"].encode("utf-8")), flush=True)
    wrapped = (
        "#define R1109_CASE " + str(case) + "\n"
        '#define NEWLANG_LOOP_OBSERVER "observer.inc"\n'
        "#define main r1109_guest_original_main\n"
        + emitted["stdout"]
        + "\n#undef main\n"
        + "int main(void) { int original_exit = r1109_guest_original_main(); "
          "return r1109_finish(original_exit); }\n"
    )
    save(outdir, "wrapped-observer.c", wrapped)
    # Instrumentation only: raw emitter bytes separately preserved. Never use
    # an expected-value file or synthesize authority/current from the oracle.
    fail_if(wrapped.count("NL_LOOP_BEFORE(") < 1 or
            wrapped.count("NL_LOOP_AFTER(") < 1,
            "passive hooks absent")
    version = {}
    witness = []
    with tempfile.TemporaryDirectory(prefix="r1109-native-") as tmp:
        for label, compiler in (("gcc", gcc), ("clang", clang)):
            vc = execute([compiler, "--version"], timeout=15)
            fail_if(vc["rc"] != 0, "compiler version", vc)
            version[label] = vc["stdout"].splitlines()[0]
            for opt in ("O0", "O2"):
                for sanitizer in ("none", "address", "undefined"):
                    outfile = Path(tmp) / (label + "_" + opt + "_" + sanitizer)
                    flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                             "-" + opt, "-I", str(Path(__file__).resolve().parent)]
                    if sanitizer == "address":
                        flags += ["-fsanitize=address", "-fno-omit-frame-pointer"]
                    elif sanitizer == "undefined":
                        flags += ["-fsanitize=undefined", "-fno-sanitize-recover=all"]
                    compile_result = execute(
                        [compiler, *flags, str(outdir / "wrapped-observer.c"),
                         "-o", str(outfile)], timeout=90)
                    runlabel = label + "_" + opt + "_" + sanitizer
                    save(outdir, runlabel + "_compile.json",
                         json.dumps(compile_result, indent=2) + "\n")
                    fail_if(compile_result["rc"] != 0,
                            "strict emitted C17 native compile " + runlabel,
                            compile_result)
                    native = execute([str(outfile)], timeout=25)
                    record = {
                        "compiler": label, "compiler_version": version[label],
                        "opt": opt, "sanitizer": sanitizer, "exit_code": native["rc"],
                        "actual_raw_stdout": native["stdout"],
                        "actual_raw_stderr": native["stderr"],
                        "native_binary_sha256": sha(outfile.read_bytes()),
                        "unmodified_emitted_c17_sha256": sha(emitted["stdout"].encode("utf-8")),
                    }
                    save(outdir, runlabel + "_native.json",
                         json.dumps(record, indent=2) + "\n")
                    fail_if(native["rc"] != 0 or
                            "R1109_NATIVE_OBSERVER_PASS" not in native["stderr"] or
                            "R1109 ACTUAL BEFORE" not in native["stderr"] or
                            "R1109 ACTUAL AFTER" not in native["stderr"] or
                            "R1109 ACTUAL post_BREAK_READ" not in native["stderr"],
                            "native actual original identity " + runlabel, record)
                    witness.append(record)
                    print("R1109_C205 LIVE_NATIVE_PASS case=" + str(case) +
                          " compiler=" + label + " opt=" + opt +
                          " sanitizer=" + sanitizer, flush=True)
                    print(native["stderr"], end="", flush=True)
    save(outdir, "native-full-index.json", json.dumps({
        "source": src.name, "case": case, "checked": "PASS",
        "emitted_c17_sha256": sha(emitted["stdout"].encode("utf-8")),
        "public_checked_owned_authenticity": "NOT TESTED",
        "matrix": [{"compiler": x["compiler"], "opt": x["opt"],
                    "sanitizer": x["sanitizer"],
                    "native_binary_sha256": x["native_binary_sha256"],
                    "real_observer": "PASS"} for x in witness]
    }, indent=2) + "\n")
    print("R1109_C205 V2_NATIVE_FINITE_PASS " + src.name +
          " 2 compilers x 2 opt x 3 sanitizer = 12 real runs; "
          "PUBLIC_CHECKED_OWNED_NOT_TESTED", flush=True)

def negative(cli, src, outdir):
    checked = execute([cli, "--check", str(src)], timeout=30)
    emitted = execute([cli, str(src)], timeout=30)
    save(outdir, "negative-check.json", json.dumps(checked, indent=2) + "\n")
    save(outdir, "negative-emit.json", json.dumps(emitted, indent=2) + "\n")
    structured = r"error\((?:parser|semantic|cli)\)\[[A-Za-z0-9-]+\]"
    fail_if(checked["rc"] not in (2, 3, 4) or
            not re.search(structured, checked["stderr"]) or
            emitted["rc"] not in (2, 3, 4) or
            not re.search(structured, emitted["stderr"]) or
            emitted["stdout"],
            "negative canonical-invalid must reject without C17",
            {"check": checked, "emit": emitted})
    print("R1109_C205 NEGATIVE_REJECT_PASS " + src.name +
          " check_rc=" + str(checked["rc"]) +
          " emit_rc=" + str(emitted["rc"]) + " c17_bytes=0", flush=True)
    print("R1109 NEGATIVE CHECK RAW: " + checked["stderr"], flush=True)

def main():
    if len(sys.argv) != 9:
        raise SystemExit(
            "usage: runner.py CLI MODE SOURCE EXPECT_SHA CASE GCC CLANG RAW_BASE")
    _, cli, mode, source, expected_sha, case_text, gcc, clang, raw_base = sys.argv
    src = Path(source).resolve(strict=True)
    fail_if(sha(src.read_bytes()) != expected_sha, "frozen SHA256 drift " + src.name)
    outdir = Path(raw_base) / src.stem
    fail_if(outdir.is_symlink(), "symlink raw path")
    outdir.mkdir(parents=True, exist_ok=True)
    save(outdir, "input-sha256.txt", expected_sha + "  " + src.name + "\n")
    if mode == "positive":
        fail_if(case_text not in ("1", "2"), "unknown positive case")
        positive(cli, src, int(case_text), gcc, clang, outdir)
    elif mode == "negative":
        negative(cli, src, outdir)
    else:
        raise AssertionError("R1109_C205_STOP invalid test mode")
if __name__ == "__main__":
    main()
