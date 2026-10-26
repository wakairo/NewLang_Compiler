#!/usr/bin/env python3
"""R939 independently frozen source-to-C17/native oracle; no P fixtures."""
import hashlib
import os
from pathlib import Path
import re
import resource
import shutil
import signal
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent
HASHES = {
    "meridian_true": "545cdf1a5a335a0b01d1e0385df0301ba477cd4c191d335db35af6603e551f00",
    "stratum_false": "349d01fd116ae862f345539abfbf314292334c7323d3ce060e6b2f2ca465aab1",
    "missing": "546bbd03ebcb0078652cdb391e3324eaebdd1a6ecf169268d613d789a3b6c840",
    "duplicate": "63916e18b5a44b2f2f51dca68d36e0866c9ad18ecf397524075d0e47ddf602b0",
    "nonbool": "a4ec1df01370030ee8837b639ce3f8ee95da016803311ea86aef31b4338f3eaf",
    "extra_reads": "1c8626ca13550f7f7e9d47b671c69f47900a1218791cf021358726908519a99f",
    "empty_arms": "a22820e0b7501a6f854733db970d138a19d28ec1d6a89a82e337bad6ed942d30",
    "nonempty_arm": "7c06874836fb4d49c0de1467423dc174dda93db2a620453ca8f13d67c4115428",
    "terminal_arm": "975e7cadf9fa9dba352b4a884bf8087aaaf5269b3b0cf9ed6bb27500c3e6ad4a",
    "nested_if": "6b7c2531113d53ba69407d042a87927386a2fff86a08039e3f9015c5839228c1",
    "effect_loan": "d9431bbfbf403757e7ce0cc5be821a359ffcbc3d51da9b707d9f74fcbd6fda5c",
}
ORACLE = {
    "meridian_true": (((0, 255), (73, 19)), 1, ((1, 0), (0, 1))),
    "stratum_false": (((203, 1), (254, 0)), 2, ((0, 1), (1, 0))),
}
SEM_INVALID = ("missing", "duplicate", "nonbool")
OUT_OF_PROFILE = ("extra_reads", "empty_arms", "nonempty_arm", "terminal_arm", "nested_if", "effect_loan")

def need(ok, message):
    if not ok:
        raise AssertionError(message)


def run(args, **kw):
    cmd = list(map(str, args))
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                       timeout=90, **kw)
    print("CMD", " ".join(cmd), "rc", p.returncode, "stdout_len",
          len(p.stdout), "stderr", p.stderr.decode("utf8", "replace")[:600], flush=True)
    return p


def emitted_originals(c, values, order):
    """Inspect immutable uninstrumented emitted source, not observer proxies."""
    pattern = re.compile(
        r"(?m)^\s*(?:const\s+)?nl_type_(\d+)\s+nl_local_(\d+)\s*=\s*\{([^{}\n]+)\};")
    found = list(pattern.finditer(c))
    need(len(found) == 2, "need exactly two distinct original C declarations")
    need(found[0].group(1) == found[1].group(1), "nominal physical type differs")
    need(found[0].group(2) != found[1].group(2), "originals share one C carrier")
    for i, match in enumerate(found):
        fields = re.findall(r"\.f([01])\s*=\s*(\d+)", match.group(3))
        need(len(fields) == 2 and tuple(int(x) for x, _ in fields) == order[i],
             "initializer source order/index changed")
        need(tuple(dict((int(k), int(v)) for k, v in fields)[j]
                   for j in (0, 1)) == values[i], "original values changed")
    return found


def witness_copy(c, matches, values, branch):
    """Passive copy: original declarations untouched; volatile observations only."""
    anchor = re.search(
        r"(?m)^    if \([^\n]*\) \{\n"
        r"(?P<then>(?:        [^\n]*\n)*)"
        r"    \} else \{\n"
        r"(?P<other>(?:        [^\n]*\n)*)"
        r"    \}\n", c[matches[1].end():])
    need(anchor is not None, "cannot locate exact emitted two-normal IF")
    start = matches[1].end() + anchor.start()
    end = matches[1].end() + anchor.end()
    original_if = c[start:end]
    need(original_if.count("} else {") == 1, "expected one real IF")
    need(original_if.count("(void)0;") == 2, "empty unit arms altered")
    type_id = matches[0].group(1)
    root1, root2 = (m.group(2) for m in matches)
    v0, v1 = values
    checks = (
        "if (r939_one->f0 != " + str(v0[0]) +
        " || r939_one->f1 != " + str(v0[1]) +
        " || r939_two->f0 != " + str(v1[0]) +
        " || r939_two->f1 != " + str(v1[1]) +
        " || (const volatile void *)r939_one == (const volatile void *)r939_two) abort();\n")
    setup = (
        "    const volatile nl_type_" + type_id + " *const r939_one = &nl_local_" + root1 + ";\n"
        "    const volatile nl_type_" + type_id + " *const r939_two = &nl_local_" + root2 + ";\n"
        "    volatile unsigned r939_marker = 0;\n"
        "    if (offsetof(nl_type_" + type_id + ", f1) <= offsetof(nl_type_" +
        type_id + ", f0)) abort();\n    " + checks)
    instrumented_if = original_if.replace(
        ") {\n", ") {\n        r939_marker = 1;\n", 1).replace(
        "} else {\n", "} else {\n        r939_marker = 2;\n", 1)
    tail = "    " + checks + "    if (r939_marker != " + str(branch) + ") abort();\n"
    need(c.count("#include <stdint.h>") == 1, "cannot add test-only abort header")
    return (c[:start] + setup + instrumented_if + tail + c[end:]).replace(
        "#include <stdint.h>", "#include <stdint.h>\n#include <stdlib.h>", 1)


def native(c, label, compiler, sanitizer, work):
    for opt in ("-O0", "-O2"):
        path = work / (label + opt[1:] + ".c")
        exe = work / (label + opt[1:])
        path.write_text(c, encoding="utf8", newline="\n")
        flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", opt]
        if sanitizer in ("address", "undefined"):
            flags += ["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all"]
        build = run([compiler, *flags, path, "-o", exe])
        need(build.returncode == 0, "strict native C17 compile failed " + label)
        result = run([exe])
        need(result.returncode == 0 and result.stdout == b"" and
             result.stderr == b"", "native/C17/sanitizer mismatch " + label)
        print("NATIVE PASS", label, compiler, opt, sanitizer, flush=True)


def compiler_paths(configured):
    seen = set()
    for hint in (configured, "gcc-13", "gcc", "clang-23", "clang"):
        path = shutil.which(hint)
        if not path or os.path.realpath(path) in seen:
            continue
        seen.add(os.path.realpath(path))
        version = run([path, "--version"])
        need(version.returncode == 0, "native compiler unavailable")
        headline = version.stdout.decode("utf8", "replace").splitlines()[0]
        print("HOST C", path, headline, flush=True)
        # Only report pinned matches. No unsupported/compiler-version substitution.
        if ("gcc" in headline.lower() and re.search(r"\b13\.", headline)) or (
                "clang" in headline.lower() and "23.1.3" in headline):
            yield path
    need(bool(seen), "no C compiler found")


def output_rollback(cli, source, dest, expected, work):
    dest.write_bytes(b"R939-PRESERVE-EXISTING\n")
    before = dest.read_bytes()
    r = run([cli, "--emit-c17-to", dest, source])
    need(r.returncode != 0 and dest.read_bytes() == before,
         "rejected output clobbered preexisting named C17")
    need(not list(work.glob(dest.name + ".tmp.*")), "orphaned named-output staging")
    if expected:
        need(r.stdout == b"", "failure emitted partial stdout")
    return r


def fault_and_retry(cli, source, baseline, work):
    dest = work / "fault_named.c"
    sentinel = b"R939-OLD-CARRIER-OUTPUT\n"
    dest.write_bytes(sentinel)
    def limit_file():
        signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
        resource.setrlimit(resource.RLIMIT_FSIZE, (0, 0))
    hit = run([cli, "--emit-c17-to", dest, source], preexec_fn=limit_file)
    need(hit.returncode != 0 and dest.read_bytes() == sentinel,
         "genuine named-output RLIMIT_FSIZE fault not hit or sentinel clobbered")
    need(not list(work.glob(dest.name + ".tmp.*")), "fault left staging orphan")
    need(b"V0-C-OUTPUT" in hit.stderr or b"V0-C-FILE" in hit.stderr,
         "fault did not reach named-output writer/flush/commit")
    good = run([cli, "--emit-c17-to", dest, source])
    need(good.returncode == 0 and good.stderr == b"" and
         dest.read_bytes() == baseline, "fault retry emitted different bytes")
    print("REAL FILE-SIZE WRITE/FLUSH FAULT HIT, same-source rollback/retry PASS")


def main():
    need(len(sys.argv) == 4, "expected CLI configured CC sanitizer")
    cli, cc, sanitizer = sys.argv[1:]
    for name, digest in HASHES.items():
        raw = (ROOT / (name + ".nl")).read_bytes()
        need(hashlib.sha256(raw).hexdigest() == digest, "frozen SHA mismatch " + name)
        need(raw.endswith(b"\n") and b"\r" not in raw,
             "frozen source changed newline " + name)
        print("FROZEN", name, len(raw), digest, flush=True)
    with tempfile.TemporaryDirectory(prefix="r939_") as temp:
        work = Path(temp)
        compilers = list(compiler_paths(cc))
        need(compilers, "neither pinned GCC13 nor Clang23.1.3 available")
        for name, (values, branch, order) in ORACLE.items():
            source = ROOT / (name + ".nl")
            checked = run([cli, "--check", source])
            need(checked.returncode == 0 and not checked.stderr,
                 "positive semantic CHECK failed " + name)
            stdout = run([cli, source])
            need(stdout.returncode == 0 and not stdout.stderr and stdout.stdout,
                 "positive C17 stdout emission failed " + name)
            output = work / (name + ".c")
            named = run([cli, "--emit-c17-to", output, source])
            need(named.returncode == 0 and not named.stdout and not named.stderr,
                 "positive named output failed " + name)
            need(output.read_bytes() == stdout.stdout,
                 "stdout/named output C17 byte mismatch " + name)
            raw = stdout.stdout
            print("C17 SHA256", name, hashlib.sha256(raw).hexdigest(), flush=True)
            decoded = raw.decode("utf8")
            found = emitted_originals(decoded, values, order)
            witness = witness_copy(decoded, found, values, branch)
            for compiler in compilers:
                tag = name + ("_gcc" if "gcc" in Path(compiler).name else "_clang")
                native(decoded, tag + "_original", compiler, sanitizer, work)
                native(witness, tag + "_passive_witness", compiler, sanitizer, work)
            print("REAL TWO ORIGINALS, DECL-INDEX INIT, TRUE/FALSE PASS", name)
        for name in (*SEM_INVALID, *OUT_OF_PROFILE):
            source = ROOT / (name + ".nl")
            check = run([cli, "--check", source])
            if name in SEM_INVALID:
                need(check.returncode != 0, "semantic-invalid source admitted " + name)
            elif name == "extra_reads":
                print("EXTRA COPY READ CHECK", check.returncode, "out-of-profile independent of checker precision")
            generated = run([cli, source])
            need(generated.returncode != 0 and generated.stdout == b"",
                 "unproven/out-of-profile source emitted C17: " + name)
            output_rollback(cli, source, work / (name + "_named.c"),
                            True, work)
            print("NEGATIVE PASS", name, "CHECK", check.returncode,
                  "EMIT", generated.returncode)
        baseline = run([cli, ROOT / "meridian_true.nl"])
        need(baseline.returncode == 0 and baseline.stdout, "fault baseline absent")
        fault_and_retry(cli, ROOT / "meridian_true.nl", baseline.stdout, work)
    print("R939 V3 LIMITED PASS — genuine normal CLI/native; public tamper NOT TESTED")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print("R939 STOP / FIRST UNEXPECTED", type(exc).__name__, str(exc),
              file=sys.stderr, flush=True)
        sys.exit(1)
