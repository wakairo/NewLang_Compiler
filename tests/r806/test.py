"""R806 FIRST-PASS: independent frozen source/oracle, candidate public proof and actual C17 native."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

mode, compiler, probe, cc, sanitizer, basedir = sys.argv[1:]
root = Path(basedir)
freeze = json.loads((root / "freeze.json").read_text())
oracle = json.loads((root / "oracle.json").read_text())
profiles = {"arm_index1": oracle["profile1"], "arm_index0": oracle["profile2"]}
sources = [root / "sources" / (stem + ".nl") for stem in profiles]
negatives = ["wrong_field.nl", "duplicate_owner.nl", "wrong_domain.nl", "bad_t_type.nl"]


def invoke(cmd, *, env=None):
    return subprocess.run([str(x) for x in cmd], capture_output=True, env=env)


def success(cmd, label, *, env=None):
    result = invoke(cmd, env=env)
    if result.returncode != 0:
        raise AssertionError(f"{label}: rc={result.returncode} stdout={result.stdout!r} stderr={result.stderr!r}")
    return result


def verify_hash(path):
    expected = freeze["files"][str(path.relative_to(root.parent.parent))]
    observed = hashlib.sha256(path.read_bytes()).hexdigest()
    assert observed == expected, (path, expected, observed)


def check_sources():
    for path in sources:
        verify_hash(path)
        result = success([compiler, "--check", path], f"CHECK {path.name}")
        assert not result.stdout and not result.stderr, (path, result.stdout, result.stderr)
    for name in negatives:
        path = root / "sources" / name
        verify_hash(path)
        result = invoke([compiler, "--check", path])
        assert result.returncode != 0 and not result.stdout, (path, result)
    print("R806_SOURCE_PASS frozen positives and all four source negatives", flush=True)


def proof():
    for stem, expected in profiles.items():
        path = root / "sources" / (stem + ".nl")
        verify_hash(path)
        output = success([probe, path], "source-issued public proof").stdout.decode()
        copies = re.findall(r"COPY .*?index=(\d+).*?old=(\d+) new=(\d+) sibling=(\d+) body_status=(\d+) parameters_status=(\d+)", output)
        required = (str(expected["changed_index"]), str(expected["old_copy"]),
                    str(expected["alpha_after"][expected["changed_index"]]),
                    str(expected["siblings"][0]), "0", "0")
        assert required in copies, (stem, copies, required, output)
        assert "same_ptr_root=1 same_allocation=1 same_domain=1" in output, output
        assert "local_valid=1 body_same_caller_world=1 entry_distinct_return=1" in output, output
        assert "post_proved=1 entry_original_exact=1" in output, output
        assert "NODE_BACKEND status=0" in output, output
        assert not re.search(r"all_closed=0", output), output
        print(f"R806_ISSUER_PASS {stem} public Copy body+params, H original R/A/D", flush=True)


def emitted(path):
    check = success([compiler, "--check", path], "CHECK")
    assert not check.stdout and not check.stderr
    c = success([compiler, path], "actual emitted C17")
    assert c.stdout and not c.stderr
    return c.stdout


WRAPPER = r"""
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void *__real_malloc(size_t);
void __real_free(void *);
static void *original[2];
static unsigned requests, successful, released;
static unsigned fail_at;
static int initialized;
static void finish(void) {
    unsigned expected_calls = fail_at == 1 ? 1u : 2u;
    unsigned expected_succeeded = fail_at == 1 ? 0u : fail_at == 2 ? 1u : 2u;
    if (requests != expected_calls || successful != expected_succeeded ||
        released != expected_succeeded) abort();
    printf("R806_NATIVE_PASS requests=%u allocations=%u frees=%u fail=%u\n",
           requests, successful, released, fail_at);
}
void *__wrap_malloc(size_t amount) {
    if (!initialized) {
        const char *e = getenv("R806_FAIL");
        if (!e || (strcmp(e,"0") && strcmp(e,"1") && strcmp(e,"2"))) abort();
        fail_at = (unsigned)(e[0] - '0');
        if (atexit(finish) != 0) abort();
        initialized = 1;
    }
    if (requests >= 2 || amount != 24) abort();
    ++requests;
    if (fail_at == requests) return NULL;
    void *p = __real_malloc(amount);
    if (!p || (successful && p == original[0])) abort();
    original[successful++] = p;
    return p;
}
void __wrap_free(void *p) {
    if (!p || released >= successful) abort();
    void *expected = successful == 2 && released == 0 ? original[1] : original[0];
    if (p != expected) abort();
    ++released;
    __real_free(p);
}
"""


def c17():
    with tempfile.TemporaryDirectory(prefix="r806-c17-") as tmp:
        folder = Path(tmp)
        for source in sources:
            content = emitted(source)
            c = folder / (source.stem + ".c")
            c.write_bytes(content)
            target = folder / "named.c"
            target.write_bytes(b"R806-preexisting-C17-target-sentinel")
            named = success([compiler, "--emit-c17-to", target, source], "named C17")
            assert not named.stdout and not named.stderr and target.read_bytes() == content
            # A failing negative must not overwrite an existing named output.
            for negative in negatives:
                target.write_bytes(b"R806-preexisting-C17-target-sentinel")
                p = invoke([compiler, "--emit-c17-to", target, root / "sources" / negative])
                assert p.returncode != 0 and target.read_bytes() == b"R806-preexisting-C17-target-sentinel"
            flags = ["-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror"]
            success([cc, *flags, "-O0", "-fsyntax-only", c], "strict C17")
            print(f"R806_C17_PASS {source.name} SHA256={hashlib.sha256(content).hexdigest()}", flush=True)


def native():
    flags = ["-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror"]
    if sanitizer != "none":
        flags += ["-fsanitize=" + sanitizer, "-fno-omit-frame-pointer",
                  "-fno-sanitize-recover=all"]
    with tempfile.TemporaryDirectory(prefix="r806-native-") as tmp:
        folder = Path(tmp)
        wrap = folder / "r806_platform.c"
        wrap.write_text(WRAPPER)
        for source in sources:
            c = folder / (source.stem + ".c")
            content = emitted(source)
            c.write_bytes(content)
            for opt in ("-O0", "-O2"):
                binary = folder / (source.stem + opt)
                success([cc, *flags, opt, c, wrap, "-Wl,--wrap=malloc",
                         "-Wl,--wrap=free", "-o", binary],
                        f"strict native compile {source.name} {opt}")
                for fail in ("0", "1", "2"):
                    env = dict(os.environ, R806_FAIL=fail)
                    env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
                    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
                    guest = success([binary], f"native {source.name} {opt} fail={fail}", env=env)
                    text = guest.stdout.decode()
                    expect = {"0": (2,2,2), "1": (1,0,0), "2": (2,1,1)}[fail]
                    assert f"requests={expect[0]} allocations={expect[1]} frees={expect[2]} fail={fail}" in text, text
                    print(f"R806_NATIVE {source.stem} {cc} {opt} fail={fail}: {text.strip()}", flush=True)


if mode == "source":
    check_sources()
elif mode == "issuer":
    proof()
elif mode == "c17":
    c17()
elif mode == "native":
    native()
else:
    raise SystemExit("unknown R806 mode")
