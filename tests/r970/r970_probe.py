#!/usr/bin/env python3
"""R970 immutable first-pass source-to-CHECK-to-C17/native Actions probe.

Output is stage-qualified: early parser rejection never passes semantic negative.
Owner identity/OOM/Copy observation is explicitly NOT proven by CLI/native exit 0.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def run(argv):
    try:
        p = subprocess.run(argv, capture_output=True, text=True, timeout=120)
    except (OSError, subprocess.TimeoutExpired) as e:
        raise RuntimeError(f"HOST EXCEPTION {argv[0]}: {e}") from e
    print("R970 CMD", " ".join(map(str, argv)), "RC", p.returncode, flush=True)
    print("R970 STDOUT", p.stdout[:2500], flush=True)
    print("R970 STDERR", p.stderr[:2500], flush=True)
    return p


def stage(p):
    s = (p.stderr + "\n" + p.stdout).upper()
    if "V0-PARSER" in s or "PARSE ERROR" in s or "PARSE-ERROR" in s:
        return "PARSER"
    if "V0-SEMANTIC" in s or "SEMANTIC" in s or "V0-CHECK" in s:
        return "SEMANTIC"
    if "V1-BACKEND-UNSUPPORTED" in s or "BACKEND-UNSUPPORTED" in s:
        return "BACKEND-UNSUPPORTED"
    return "CHECK-ACCEPTED" if p.returncode == 0 else "OTHER-REJECT"


def main():
    if len(sys.argv) != 6:
        raise RuntimeError("usage: probe manifest case compiler cc sanitizer")
    mp, name, compiler, cc, sanitizer = sys.argv[1:]
    m = json.loads(Path(mp).read_text(encoding="utf-8"))
    case = next(c for c in m["cases"] if c["name"] == name)
    source = Path(mp).parent / "sources" / (name + ".nl")
    data = source.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    print("R970 FROZEN CASE", name, "bytes", len(data),
          "sha256", digest, "issue-comment", case["comment"], flush=True)
    if len(data) != case["bytes"] or digest != case["sha256"]:
        raise RuntimeError("PRE-RUN STOP: R963 original source SHA/bytes mismatch")
    if not data.endswith(b"\n") or data.endswith(b"\n\n"):
        raise RuntimeError("PRE-RUN STOP: R963 original EOF altered")
    checked = run([compiler, "--check", str(source)])
    first = stage(checked)
    print("R970 FIRST CHECK STAGE", name, first, flush=True)

    with tempfile.TemporaryDirectory(prefix="r970-blind-") as t:
        out = Path(t) / "generated.c"
        if case["expect"] != "positive":
            sentinel = b"R970 untouched named output\x00\n"
            out.write_bytes(sentinel)
            old = out.stat()
            emit = run([compiler, "--emit-c17-to", str(out), str(source)])
            new = out.stat()
            unchanged = (old.st_dev == new.st_dev and old.st_ino == new.st_ino
                         and out.read_bytes() == sentinel)
            print("R970 NEG RESULT", name, first, stage(emit),
                  "named-unchanged", unchanged, flush=True)
            if checked.returncode == 0 or emit.returncode == 0 or not unchanged:
                raise RuntimeError("FAIL: negative accepted or output mutated")
            if case["expect"] == "grammar_negative":
                if first != "PARSER":
                    raise RuntimeError("INCONCLUSIVE: mandatory ELSE not parser-rejected")
            elif first != "SEMANTIC":
                raise RuntimeError("INCONCLUSIVE: semantic negative rejected earlier/later: " + first)
            print("R970 EXPECTED REJECTION STAGE, NOT FOREIGN CHECK INSTANCE PROOF",
                  flush=True)
            return
        if checked.returncode:
            raise RuntimeError("INCONCLUSIVE: positive CHECK profile stop at " + first)
        emit = run([compiler, "--emit-c17-to", str(out), str(source)])
        if emit.returncode:
            raise RuntimeError("INCONCLUSIVE: positive C17 emitter stop at " + stage(emit))
        if not out.exists() or not out.stat().st_size:
            raise RuntimeError("FAIL: empty C17 after accepted CHECK")
        print("R970 FULL EMITTED C17 SHA256", hashlib.sha256(out.read_bytes()).hexdigest(),
              "bytes", out.stat().st_size, flush=True)
        for level in (0, 2):
            exe = Path(t) / ("native-" + str(level))
            flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                     "-pedantic-errors", "-O" + str(level)]
            if sanitizer in ("address", "undefined"):
                flags += ["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all"]
            compile_result = run([cc, *flags, str(out), "-o", str(exe)])
            if compile_result.returncode:
                raise RuntimeError("FAIL: strict C17 native build O" + str(level))
            original = os.environ.get("ASAN_OPTIONS")
            try:
                if sanitizer == "address":
                    os.environ["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
                native_result = run([str(exe)])
            finally:
                if original is None:
                    os.environ.pop("ASAN_OPTIONS", None)
                else:
                    os.environ["ASAN_OPTIONS"] = original
            if native_result.returncode:
                raise RuntimeError("FAIL: native O" + str(level))
        print("R970 native exit-only success; UNMEASURED real R/A/D identity,"
              " guest heap worlds 0/1/2, Copy-old fields, genuine CHECK A/B",
              flush=True)


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError, StopIteration, ValueError, KeyError) as e:
        print("R970 FIRST-PASS DISPOSITION:", e, file=sys.stderr, flush=True)
        sys.exit(1)
