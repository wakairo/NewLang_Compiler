#!/usr/bin/env python3
"""R638 frozen-source normal CLI → source CHECK → C17 → passive native observer.
Only test-owned observer changes emitted C; it never assigns/initializes guest values.
This test does NOT claim private proof mutation or shared-validator safety."""
import hashlib
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

compiler, public_probe, directory, sanitizer = sys.argv[1:]
source_dir = pathlib.Path(directory)
positive = {
    "lit_true.nl": 29,
    "lit_false.nl": 211,
    "copy_equal.nl": 74,
    "field_true.nl": None,
}
unsupported_ok = {"field_true.nl", "nested_legal.nl", "second_local.nl"}
legal_extra = ["nested_legal.nl", "second_local.nl"]
negatives = ["wrong_type.nl", "out_of_range.nl"]
hashes = {
    "lit_true.nl": "116a6bb20e23de4369bffe110b636a93606556dbfaf9244a7f0bd9170e21878d",
    "lit_false.nl": "c4daae65a267e6f5bf88dc4da5cf90e4304204fd75c49c3ce23c8b7d2b785e8d",
    "copy_equal.nl": "c9be52298fd1f2628b1f42ebeffb199280177cf996bb670ba65afe43e673b69e",
    "field_true.nl": "d395e9fb925e27e95c6284038d365ce992599cf8fa37893e0fd9ec6c41496a70",
    "wrong_type.nl": "5e886e57de91fcd034df765e552677056399dea208b1c8357e2fb9575cc7d6b8",
    "out_of_range.nl": "767fc0a16d9112f36ce0d224f1e8184e1ae2703c48cceaef26456c2b77184cf6",
    "nested_legal.nl": "afd7c03b2d3a51709a5889267f9de89fc0f322817995bc87f1147bae4dfbac2e",
    "second_local.nl": "e9bd4fb7788541571bb76efcb0aa3deed6da3d5cadecb741fc47a869ddf7b662",
}

def invoke(args):
    return subprocess.run(args, capture_output=True, text=True, timeout=65, check=False)

def fail(message, details=""):
    raise AssertionError(message + ("\n" + details if details else ""))

def assert_semantic_check(path, expect_ok):
    check = invoke([compiler, "--check", str(path)])
    if bool(check.returncode == 0) != expect_ok or check.stdout:
        fail("CHECK disagrees with frozen oracle: " + path.name,
             "rc=%s stderr=%r stdout=%r" % (check.returncode, check.stderr, check.stdout))
    if not expect_ok and not check.stderr:
        fail("negative had no diagnostic: " + path.name)
    print("R638 CHECK", path.name, "OK" if expect_ok else "REJECT",
          "diagnostic=" + repr(check.stderr[:180]), flush=True)

def passive_observer(c, name):
    # The last source expression-statement is literally 'cargo;'.
    # Backend emits its value-use as '(void)nl_local_ID;' inside original main().
    # Capture guest local by read only, without assigning to it or rewriting an arm.
    positions = list(re.finditer(r"(?m)^([ \t]*)\(void\)nl_local_(\d+);[ \t]*$", c))
    if not positions:
        fail("passive observer anchor absent, never fabricate guest value: " + name)
    tail = positions[-1]
    symbol = tail.group(2)
    observe = (tail.group(1) +
        'printf("R638_OBS:%u\\n", (unsigned)nl_local_' + symbol + ');\n')
    observed = c[:tail.start()] + observe + c[tail.start():]
    return '#include <stdio.h>\n' + observed, symbol

def available_compilers():
    found = []
    for family, choices in (
        ("GCC", ("gcc-13", "gcc")),
        ("Clang", ("clang-23", "clang")),
    ):
        exe = next((path for name in choices if (path := shutil.which(name))), None)
        if exe:
            found.append((family, exe))
    if not found:
        fail("no C17 compilers available")
    return found

def native_matrix(workdir, name, original_c, expected):
    guest_path = workdir / (name + ".c")
    guest_path.write_text(original_c, encoding="utf-8")
    print("R638 ORIGINAL-C17", name,
          hashlib.sha256(original_c.encode()).hexdigest(), str(guest_path), flush=True)
    for family, cc in available_compilers():
        for opt in ("-O0", "-O2"):
            binary = workdir / (name + "_" + family + opt[1:])
            cmd = [cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra",
                   "-Werror", opt, str(guest_path), "-o", str(binary)]
            result = invoke(cmd)
            if result.returncode:
                fail("strict native compile failure " + name + " " + family + opt,
                     repr(result.stderr) + "\nORIGINAL C17:\n" + original_c)
            guest = invoke([str(binary)])
            if guest.returncode:
                fail("native guest failure " + name + " " + family + opt,
                     repr(guest.stderr))
            print("R638 NATIVE", name, family, opt, "return=0", flush=True)
    if expected is None:
        print("R638 ORIGINAL field/readback native observer: NOT TESTED (no safe anchor)", flush=True)
        return
    observed, symbol = passive_observer(original_c, name)
    observation_path = workdir / (name + "_passive.c")
    observation_path.write_text(observed, encoding="utf-8")
    print("R638 OBSERVER", name, "read-only symbol", symbol, flush=True)
    for family, cc in available_compilers():
        for opt in ("-O0", "-O2"):
            binary = workdir / (name + "_passive_" + family + opt[1:])
            flags = ["-std=c17", "-pedantic-errors", "-Wall", "-Wextra",
                     "-Werror", opt]
            if sanitizer == "address" and family == "Clang":
                flags += ["-fsanitize=address", "-fno-omit-frame-pointer"]
            if sanitizer == "undefined" and family == "Clang":
                flags += ["-fsanitize=undefined", "-fno-sanitize-recover=all"]
            cmd = [cc, *flags, str(observation_path), "-o", str(binary)]
            result = invoke(cmd)
            if result.returncode:
                fail("passive instrumentation failed " + name + " " + family + opt,
                     repr(result.stderr) + "\nPASSIVE SOURCE:\n" + observed)
            guest = invoke([str(binary)])
            if guest.returncode or guest.stdout != "R638_OBS:%u\n" % expected:
                fail("native observed value mismatch " + name + " " + family + opt,
                     "rc=%d stdout=%r stderr=%r expected=%d" %
                     (guest.returncode, guest.stdout, guest.stderr, expected))
            print("R638 OBSERVED", name, family, opt, "value", expected, flush=True)

with tempfile.TemporaryDirectory(prefix="r638-") as temporary:
    work = pathlib.Path(temporary)
    for name in [*positive, *legal_extra, *negatives]:
        path = source_dir / name
        actual_hash = hashlib.sha256(path.read_bytes()).hexdigest()
        if actual_hash != hashes[name]:
            fail("frozen SOURCE SHA mismatch " + name + " " + actual_hash)
        assert_semantic_check(path, name not in negatives)
        compiled = invoke([compiler, str(path)])
        if name in negatives:
            if compiled.returncode == 0 or compiled.stdout or not compiled.stderr:
                fail("negative compiled or wrote guest bytes " + name,
                     "rc=%d stdout=%r stderr=%r" %
                     (compiled.returncode, compiled.stdout, compiled.stderr))
            print("R638 NEGATIVE", name, "reject", compiled.returncode,
                  repr(compiled.stderr[:200]), flush=True)
            continue
        if compiled.returncode != 0:
            if name not in unsupported_ok or compiled.returncode != 4 or compiled.stdout:
                fail("unexpected positive backend rejection " + name,
                     "rc=%d stderr=%r stdout=%r" %
                     (compiled.returncode, compiled.stderr, compiled.stdout))
            print("R638 UNSUPPORTED/PRECISION", name, compiled.stderr[:220], flush=True)
            continue
        if not compiled.stdout or "int main(void)" not in compiled.stdout:
            fail("no genuine generated C17 for " + name)
        print("R638 EMITTED", name, "bytes", len(compiled.stdout), flush=True)
        if name in positive:
            probe = invoke([public_probe, str(path)])
            if probe.returncode:
                fail("genuine registered source-issued public proof failed " + name,
                     "rc=%d stdout=%r stderr=%r" %
                     (probe.returncode, probe.stdout, probe.stderr))
            print(probe.stdout.strip(), flush=True)
        native_matrix(work, name.replace(".nl", ""), compiled.stdout,
                      positive.get(name))
print("R638 FINITE LAYER COMPLETE; private world-corruption/shared validator: NOT TESTED", flush=True)
