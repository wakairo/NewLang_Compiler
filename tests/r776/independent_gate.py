#!/usr/bin/env python3
"""R #776 independently frozen source→CLI→unchanged C17→native gate.
No producer fixtures, oracle imports, or source mutations.
"""
import hashlib
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

MODE, CLI, ROOT, SANITIZER = sys.argv[1:]
ROOT = pathlib.Path(ROOT)
FROZEN = ROOT / "tests" / "r776" / "frozen"
MANIFEST = json.loads((FROZEN / "oracle.json").read_text(encoding="ascii"))
CASES = {
    "h_first": ("positive_h_first.nl", 61, 173),
    "t_interposed": ("positive_t_interposed.nl", 83, 237),
}
OBS_C = r"""
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
extern void *__real_malloc(size_t n);
extern void __real_free(void *p);
static void *seen[2];
static size_t sizes[2];
static int attempts, made, releases, bad, fail_at, order[2];
static unsigned head_byte, tail_byte;
static int has_byte(const void *p, size_t n, unsigned byte)
{
    const unsigned char *s = (const unsigned char *)p;
    for (size_t i = 0; i < n; i++)
        if (s[i] == (unsigned char)byte)
            return 1;
    return 0;
}
static void finish_observation(void)
{
    int expected_made = fail_at == 1 ? 0 : fail_at == 2 ? 1 : 2;
    int expected_attempts = fail_at == 1 ? 1 : 2;
    if (attempts != expected_attempts || made != expected_made ||
        releases != expected_made)
        bad = 1;
    if (expected_made == 2 && (order[0] != 2 || order[1] != 1))
        bad = 1;
    if (expected_made == 1 && order[0] != 1)
        bad = 1;
    char line[180];
    int n = snprintf(line, sizeof(line),
       "R776_OBS %s attempts=%d made=%d frees=%d order=%d,%d fail_at=%d\n",
       bad ? "FAIL" : "PASS", attempts, made, releases,
       order[0], order[1], fail_at);
    if (n > 0 && (size_t)n < sizeof(line))
        (void)write(2, line, (size_t)n);
    if (bad)
        _Exit(86);
}
__attribute__((constructor)) static void observer_init(void)
{
    const char *p = getenv("R776_FAIL_AT");
    const char *h = getenv("R776_HEAD_BYTE");
    const char *t = getenv("R776_TAIL_BYTE");
    fail_at = p ? atoi(p) : 0;
    head_byte = h ? (unsigned)atoi(h) : 0;
    tail_byte = t ? (unsigned)atoi(t) : 0;
    (void)atexit(finish_observation);
}
void *__wrap_malloc(size_t n)
{
    ++attempts;
    if (attempts == fail_at)
        return NULL;
    void *p = __real_malloc(n);
    if (p && made < 2) {
        seen[made] = p;
        sizes[made] = n;
        ++made;
    } else {
        bad = 1;
    }
    return p;
}
void __wrap_free(void *p)
{
    if (!p)
        return;
    int idx = -1;
    for (int i = 0; i < made; i++)
        if (seen[i] == p)
            idx = i;
    if (idx < 0 || releases >= 2) {
        bad = 1;
    } else {
        for (int i = 0; i < releases; i++)
            if (order[i] == idx + 1)
                bad = 1;
        if (!has_byte(p, sizes[idx], idx == 0 ? head_byte : tail_byte))
            bad = 1;
        order[releases] = idx + 1;
        ++releases;
    }
    __real_free(p);
}
"""
def call(args, *, env=None, timeout=45):
    return subprocess.run(args, capture_output=True, env=env, timeout=timeout)

def insist(ok, description):
    if not ok:
        raise AssertionError(description)

def verify_frozen():
    hashes = MANIFEST["source_sha256"]
    for rel, expected in hashes.items():
        data = (ROOT / rel).read_bytes()
        actual = hashlib.sha256(data).hexdigest()
        insist(actual == expected, "FROZEN SOURCE SHA MISMATCH " + rel)
    print("R776 frozen hashes PASS " + str(len(hashes)), flush=True)

def emit(case, work):
    f = FROZEN / case
    check = call([CLI, "--check", str(f)])
    print("R776 source-check", case, "rc", check.returncode, "stderr",
          check.stderr.decode(errors="replace")[:500], flush=True)
    insist(check.returncode == 0 and not check.stdout and not check.stderr,
           "SOURCE/CHECK FIRST RED " + case)
    out = call([CLI, str(f)])
    print("R776 stdout emitter", case, "rc", out.returncode, "size",
          len(out.stdout), "stderr", out.stderr.decode(errors="replace")[:500],
          flush=True)
    insist(out.returncode == 0 and len(out.stdout) > 64 and not out.stderr,
           "PUBLIC/BACKEND FIRST RED " + case)
    insist(b"#include" in out.stdout and b"main" in out.stdout,
           "missing complete standalone C17 entry")
    named = work / "existing.c"
    named.write_bytes(b"R776-NAMED-SENTINEL\n")
    to = call([CLI, "--emit-c17-to", str(named), str(f)])
    insist(to.returncode == 0 and not to.stdout and not to.stderr,
           "named output emission failed " + case)
    insist(named.read_bytes() == out.stdout,
           "named output differs from stdout C17")
    c = work / "guest.c"
    c.write_bytes(out.stdout)
    print("R776 C17 emitted sha256", hashlib.sha256(out.stdout).hexdigest(),
          "size", len(out.stdout), flush=True)
    return c

def get_compilers():
    gcc = shutil.which("gcc-13") or shutil.which("gcc")
    clang = shutil.which("clang-23") or shutil.which("clang")
    insist(gcc is not None, "GCC absent")
    if clang is None:
        print("R776 clang native NOT TESTED: no Clang in PATH", flush=True)
    return [("GCC", gcc)] + ([("Clang", clang)] if clang else [])

def native(case_key):
    case, head, tail = CASES[case_key]
    with tempfile.TemporaryDirectory(prefix="r776-") as temp:
        work = pathlib.Path(temp)
        c = emit(case, work)
        obs = work / "observer.c"
        obs.write_text(OBS_C, encoding="ascii")
        compilers = get_compilers()
        insist(len(compilers) > 0, "no native compiler")
        for label, cc in compilers:
            for opt in ("O0", "O2"):
                sanitizer = (["-fsanitize=" + SANITIZER, "-fno-omit-frame-pointer"]
                             if SANITIZER != "none" else [])
                prefix = work / (label + opt)
                guest_obj = str(prefix) + ".guest.o"
                obs_obj = str(prefix) + ".observer.o"
                exe = str(prefix) + ".native"
                strict = call([cc, "-std=c17", "-pedantic-errors",
                               "-Wall", "-Wextra", "-Werror", "-"+opt,
                               "-g", *sanitizer, "-c", str(c), "-o", guest_obj])
                insist(strict.returncode == 0,
                       "STRICT C17 FIRST RED " + label + opt + " " +
                       strict.stderr.decode(errors="replace")[-1500:])
                build_obs = call([cc, "-std=gnu17", "-Wall", "-Wextra",
                                  "-Werror", "-"+opt, "-g", *sanitizer,
                                  "-c", str(obs), "-o", obs_obj])
                insist(build_obs.returncode == 0,
                       "R observer build error " + build_obs.stderr.decode(errors="replace")[-1200:])
                link = call([cc, *sanitizer, guest_obj, obs_obj,
                             "-Wl,--wrap=malloc", "-Wl,--wrap=free", "-o", exe])
                insist(link.returncode == 0,
                       "native link FIRST RED " + link.stderr.decode(errors="replace")[-1200:])
                for fail_at in (0, 1, 2):
                    env = dict(os.environ,
                               R776_FAIL_AT=str(fail_at),
                               R776_HEAD_BYTE=str(head),
                               R776_TAIL_BYTE=str(tail))
                    r = call([exe], env=env, timeout=20)
                    msg = r.stderr.decode(errors="replace")
                    print("R776_NATIVE", case_key, label, opt, "fail_at", fail_at,
                          "rc", r.returncode, "observer",
                          [v for v in msg.splitlines() if "R776_OBS" in v],
                          flush=True)
                    insist(r.returncode == 0 and not r.stdout and
                           "R776_OBS PASS" in msg and "R776_OBS FAIL" not in msg,
                           "NATIVE FIRST RED " + case_key + label + opt +
                           " world=" + str(fail_at) + " " + msg[-1600:])
        print("R776 native H malloc/free physical observer completed;", 
              "T local runtime values NOT directly observable in this harness",
              flush=True)

NEGATIVE = (
    ("negative_wrong_allocation.nl", "semantic"),
    ("negative_stale_link.nl", "semantic"),
    ("negative_unknown_field.nl", "source"),
    ("negative_read_to_write.nl", "source-or-semantic"),
    ("negative_two_t_nominals.nl", "source"),
    ("negative_double_consume.nl", "semantic"),
)
def negatives():
    with tempfile.TemporaryDirectory(prefix="r776-neg-") as temp:
        work = pathlib.Path(temp)
        for case, predicted_layer in NEGATIVE:
            src = FROZEN / case
            checked = call([CLI, "--check", str(src)])
            compiled = call([CLI, str(src)])
            named = work / (case + ".c")
            sentinel = b"R776-KEEP-ORIGINAL-C17\n"
            named.write_bytes(sentinel)
            file_out = call([CLI, "--emit-c17-to", str(named), str(src)])
            print("R776_NEG", case, "predicted=", predicted_layer,
                  "check_rc=", checked.returncode, "emit_rc=",
                  compiled.returncode, "named_rc=", file_out.returncode,
                  "check_diag=", checked.stderr.decode(errors="replace")[:400],
                  "emit_diag=", compiled.stderr.decode(errors="replace")[:400],
                  flush=True)
            insist(checked.returncode != 0, "NEGATIVE CHECK UNSOUND ACCEPT " + case)
            insist(compiled.returncode != 0 and not compiled.stdout,
                   "NEGATIVE EMITTER UNSOUND ACCEPT/PARTIAL C " + case)
            insist(file_out.returncode != 0 and named.read_bytes() == sentinel,
                   "NEGATIVE NAMED OUTPUT LOSS " + case)
            insist(bool(checked.stderr) and bool(compiled.stderr),
                   "negative rejection lacked diagnostic " + case)
        print("R776 6 negative fail-closed source/semantic cases observed", flush=True)

def atomicity():
    with tempfile.TemporaryDirectory(prefix="r776-output-") as temp:
        work = pathlib.Path(temp)
        c = emit(CASES["h_first"][0], work)
        sentinel = b"R776-NAMED-COMMITTED-BEFORE-REFUSAL\n"
        target = work / "old.c"
        target.write_bytes(sentinel)
        bad = FROZEN / "negative_wrong_allocation.nl"
        r = call([CLI, "--emit-c17-to", str(target), str(bad)])
        insist(r.returncode != 0 and target.read_bytes() == sentinel,
               "named C17 transaction dirty after semantic rejection")
        insist(not list(work.glob("old.c.tmp.*")), "temporary output leaked")
        bad_path = work / "special"
        bad_path.mkdir()
        special = call([CLI, "--emit-c17-to", str(bad_path), str(FROZEN / CASES["h_first"][0])])
        insist(special.returncode != 0 and bad_path.is_dir(),
               "nonregular named output target was replaced")
        print("R776 named rollback/refusal observed; genuine emitter OOM",
              "NOT TESTED without verified injection at emitter stage", flush=True)

verify_frozen()
if MODE == "nativeA":
    native("h_first")
elif MODE == "nativeB":
    native("t_interposed")
elif MODE == "negative":
    negatives()
elif MODE == "atomicity":
    atomicity()
else:
    raise SystemExit("unexpected R776 mode " + MODE)
