#!/usr/bin/env python3
"""R937 independently frozen source-to-C17/native oracle; no P fixtures."""
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
    "helix_true": "9221b6b2190b9794b25fe126a9b6d61468d78d68b6adb68210566c37c6e9b287",
    "quasar_false": "e647de44e6b41ae736d87014d96f072929f2823a9ad0bb7aab43d4bd6b8d6a8a",
    "missing": "58b6a51e5af901d80d1de5bf49a187d7cfcdaa84c32afcf7daccfd6160b6f54b",
    "duplicate": "7558a641c3711bc9176a9aa1c7fe5ce2e010757ee070f5ae11aa3ded66333687",
    "nonbool": "b5c45b580c0421aa0b60412db877b1185cd1cf3065b6e99a5953e818e2d12d9c",
    "reads": "0c7409c653de70e72f6232bbcf9390d568fa8d60602980644ced24da80241cbe",
    "nonempty": "6cbb41ef3ca876be815d1f5dd1d46d5b5860881c1fd5ca27f9996291cd112e16",
    "terminal": "397a7a985af45fa04a704cf89229d98b2827f7379b8e9a855e118b9817d724f7",
    "nested": "0121376648eed452490b53f5ccdc6c5093bd7befe9303586a1d807df7969409d",
    "effect": "c8f4a49225493662132d2f998a25c1702e639f5f1c269c7b6ef6dd17504fc19a",
}
ORACLE = {
    "helix_true": (((0, 255), (161, 42)), 1, ((1, 0), (0, 1))),
    "quasar_false": (((250, 3), (255, 0)), 2, ((0, 1), (1, 0))),
}
SEM_INVALID = ("missing", "duplicate", "nonbool")
OUT_OF_PROFILE = ("reads", "nonempty", "terminal", "nested", "effect")


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
        "if (r937_one->f0 != " + str(v0[0]) +
        " || r937_one->f1 != " + str(v0[1]) +
        " || r937_two->f0 != " + str(v1[0]) +
        " || r937_two->f1 != " + str(v1[1]) +
        " || (const volatile void *)r937_one == (const volatile void *)r937_two) abort();\n")
    setup = (
        "    const volatile nl_type_" + type_id + " *const r937_one = &nl_local_" + root1 + ";\n"
        "    const volatile nl_type_" + type_id + " *const r937_two = &nl_local_" + root2 + ";\n"
        "    volatile unsigned r937_marker = 0;\n"
        "    if (offsetof(nl_type_" + type_id + ", f1) <= offsetof(nl_type_" +
        type_id + ", f0)) abort();\n    " + checks)
    instrumented_if = original_if.replace(
        ") {\n", ") {\n        r937_marker = 1;\n", 1).replace(
        "} else {\n", "} else {\n        r937_marker = 2;\n", 1)
    tail = "    " + checks + "    if (r937_marker != " + str(branch) + ") abort();\n"
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
    dest.write_bytes(b"R937-PRESERVE-EXISTING\n")
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
    sentinel = b"R937-OLD-CARRIER-OUTPUT\n"
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
    with tempfile.TemporaryDirectory(prefix="r937_") as temp:
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
            elif name == "reads":
                need(check.returncode == 0,
                     "unexpected semantic rejection of valid extra Copy field reads")
            generated = run([cli, source])
            need(generated.returncode != 0 and generated.stdout == b"",
                 "unproven/out-of-profile source emitted C17: " + name)
            output_rollback(cli, source, work / (name + "_named.c"),
                            True, work)
            print("NEGATIVE PASS", name, "CHECK", check.returncode,
                  "EMIT", generated.returncode)
        baseline = run([cli, ROOT / "helix_true.nl"])
        need(baseline.returncode == 0 and baseline.stdout, "fault baseline absent")
        fault_and_retry(cli, ROOT / "helix_true.nl", baseline.stdout, work)
    print("R937 V2 LIMITED PASS — genuine normal CLI/native; public tamper NOT TESTED")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print("R937 STOP / FIRST UNEXPECTED", type(exc).__name__, str(exc),
              file=sys.stderr, flush=True)
        sys.exit(1)
