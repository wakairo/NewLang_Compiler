#!/usr/bin/env python3
"""R1109: independently frozen source CHECK and passive strict C17/native observer.

No mutation of compiler/P fixtures or checked issuer. Never treat CLI CHECK
alone as independently authenticated public owned verification.
"""
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

def run(argv, timeout=65):
    p = subprocess.run(argv, capture_output=True, text=True, timeout=timeout)
    return p

def source_bytes(path, sha):
    contents = Path(path).read_bytes()
    actual = hashlib.sha256(contents).hexdigest()
    if actual != sha:
        raise AssertionError("STOP source SHA drift: " + actual)
    return contents

HEADER = r"""
#ifndef R1109_OBSERVER_H
#define R1109_OBSERVER_H
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
static const uint8_t *r_fields[2][2];
static uintptr_t r_addresses[4];
static unsigned r_before_values[4];
static unsigned r_after_values[4];
static unsigned r_count_bind;
static unsigned r_count_before;
static unsigned r_count_after;
static unsigned r_old;
static unsigned r_target;
static unsigned r_bad;
static void r_bind(const uint8_t *a, const uint8_t *b)
{
    if (r_count_bind >= 2U) {
        r_bad++;
        return;
    }
    r_fields[r_count_bind][0] = a;
    r_fields[r_count_bind][1] = b;
    r_count_bind++;
}
static void r_before(const uint8_t *a, const uint8_t *b, size_t index)
{
    if (r_count_bind != 2U || r_count_before != 0U || index > 1U) {
        r_bad++;
        return;
    }
    r_count_before++;
    for (unsigned k = 0; k != 2U; ++k) {
        for (unsigned f = 0; f != 2U; ++f) {
            unsigned at = k * 2U + f;
            r_addresses[at] = (uintptr_t)(const void *)r_fields[k][f];
            r_before_values[at] = (unsigned)*r_fields[k][f];
        }
    }
    r_target = (a == r_fields[0][0] && b == r_fields[0][1]) ? 0U :
               (a == r_fields[1][0] && b == r_fields[1][1]) ? 1U : 99U;
    if (r_target == 99U || r_addresses[0] == r_addresses[2] ||
        r_addresses[1] == r_addresses[3]) {
        r_bad++;
    }
    fprintf(stderr,
            "R1109 RAW BEFORE target=%u index=%zu first=[%u,%u] second=[%u,%u] "
            "addr=[%" PRIxPTR ",%" PRIxPTR ",%" PRIxPTR ",%" PRIxPTR "]\n",
            r_target, index, r_before_values[0], r_before_values[1],
            r_before_values[2], r_before_values[3], r_addresses[0],
            r_addresses[1], r_addresses[2], r_addresses[3]);
}
static void r_after(const uint8_t *a, const uint8_t *b,
                    size_t index, uint8_t old)
{
    if (r_count_before != 1U || r_count_after != 0U ||
        index > 1U || r_target > 1U ||
        a != r_fields[r_target][0] || b != r_fields[r_target][1]) {
        r_bad++;
        return;
    }
    r_count_after++;
    r_old = (unsigned)old;
    for (unsigned k = 0; k != 2U; ++k) {
        for (unsigned f = 0; f != 2U; ++f) {
            unsigned at = k * 2U + f;
            if ((uintptr_t)(const void *)r_fields[k][f] != r_addresses[at]) {
                r_bad++;
            }
            r_after_values[at] = (unsigned)*r_fields[k][f];
        }
    }
    fprintf(stderr,
            "R1109 RAW AFTER old=%u first=[%u,%u] second=[%u,%u]\n",
            r_old, r_after_values[0], r_after_values[1],
            r_after_values[2], r_after_values[3]);
}
static int r_finish(unsigned result)
{
    static const unsigned a_before[4] = {19U, 7U, 91U, 23U};
    static const unsigned a_after[4] = {61U, 7U, 91U, 23U};
    static const unsigned b_before[4] = {31U, 13U, 31U, 13U};
    static const unsigned b_after[4] = {31U, 13U, 55U, 13U};
    const unsigned *want_before = R1109_CASE == 1 ? a_before : b_before;
    const unsigned *want_after = R1109_CASE == 1 ? a_after : b_after;
    unsigned want_target = R1109_CASE == 1 ? 0U : 1U;
    unsigned want_old = R1109_CASE == 1 ? 19U : 31U;
    if (r_count_bind != 2U || r_count_before != 1U ||
        r_count_after != 1U || r_target != want_target ||
        r_old != want_old || result != 1U) {
        r_bad++;
    }
    for (unsigned i = 0; i < 4U; ++i) {
        if (r_before_values[i] != want_before[i] ||
            r_after_values[i] != want_after[i]) {
            r_bad++;
        }
    }
    fprintf(stderr,
            "R1109 RAW VERDICT case=%d result=%u old=%u before_count=%u "
            "after_count=%u bound=%u identity_stable=checked bad=%u\n",
            R1109_CASE, result, r_old, r_count_before,
            r_count_after, r_count_bind, r_bad);
    return r_bad == 0U ? 0 : 11;
}
#define NL_LOOP_BIND(a, b) r_bind((a), (b))
#define NL_LOOP_READ(a, b) ((void)(a), (void)(b))
#define NL_LOOP_BEFORE(a, b, i) r_before((a), (b), (i))
#define NL_LOOP_AFTER(a, b, i, old) r_after((a), (b), (i), (old))
#endif
"""

def negative(cli, path):
    checked = run([cli, "--check", path])
    emitted = run([cli, path])
    pat = r"error\((?:parser|semantic|cli)\)\[[A-Za-z0-9-]+\]"
    if checked.returncode not in (3, 4) or not re.search(pat, checked.stderr):
        raise AssertionError("STOP no structured semantic negative CHECK\n"
                             + checked.stderr[:3000])
    if emitted.returncode not in (3, 4) or emitted.stdout or not re.search(pat, emitted.stderr):
        raise AssertionError("STOP invalid negative C17 rejection\n"
                             + emitted.stdout[:3000] + emitted.stderr[:3000])
    print("R1109 REAL NEGATIVE source=" + Path(path).name +
          " CHECK_RC=" + str(checked.returncode) +
          " EMIT_RC=" + str(emitted.returncode), flush=True)
    print("R1109 NEGATIVE RAW CHECK " + checked.stderr[:2500], flush=True)

def positive(cli, path, case, sanitizer):
    checked = run([cli, "--check", path])
    if checked.returncode != 0 or checked.stdout or checked.stderr:
        raise AssertionError("STOP positive CHECK not accepted: rc=" +
                             str(checked.returncode) + " stderr=" + checked.stderr[:3000])
    emitted = run([cli, path])
    if emitted.returncode != 0 or not emitted.stdout or emitted.stderr:
        raise AssertionError("STOP source-owned C17 not emitted: rc=" +
                             str(emitted.returncode) + " stderr=" + emitted.stderr[:3000])
    if "NL_LOOP_BEFORE" not in emitted.stdout or "NL_LOOP_AFTER" not in emitted.stdout:
        raise AssertionError("STOP passive physical observation hooks absent")
    matches = re.findall(r"static uint8_t (nl_fn_[0-9]+)\(void\)", emitted.stdout)
    if len(matches) != 1:
        raise AssertionError("STOP cannot uniquely locate source u8 probe in C17: " + repr(matches))
    compilers = []
    for candidate in ("gcc-13", "clang-23"):
        chosen = shutil.which(candidate)
        if chosen is None:
            chosen = shutil.which("gcc" if candidate.startswith("gcc") else "clang")
        if chosen is None:
            raise AssertionError("STOP missing C17 compiler family " + candidate)
        compilers.append((candidate.split("-")[0], chosen))
    with tempfile.TemporaryDirectory(prefix="r1109-") as td:
        work = Path(td)
        (work / "r_observer.h").write_text(HEADER, encoding="ascii")
        wrapper = (
            '#define R1109_CASE ' + str(case) + '\n'
            '#define NEWLANG_LOOP_OBSERVER "r_observer.h"\n'
            '#define main r_newlang_generated_main\n'
            + emitted.stdout + '\n#undef main\n'
            + 'int main(void) { uint8_t value = ' + matches[0]
            + '(); return r_finish((unsigned)value); }\n'
        )
        cfile = work / "r_native.c"
        cfile.write_text(wrapper, encoding="ascii")
        for label, compiler in compilers:
            for optimize in ("0", "2"):
                binary = work / ("probe_" + label + "_O" + optimize)
                flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                         "-O" + optimize, "-I" + str(work)]
                if sanitizer == "address":
                    flags += ["-fsanitize=address", "-fno-omit-frame-pointer"]
                if sanitizer == "undefined":
                    flags += ["-fsanitize=undefined", "-fno-sanitize-recover=all"]
                command = [compiler] + flags + [str(cfile), "-o", str(binary)]
                compiled = run(command)
                if compiled.returncode != 0:
                    raise AssertionError("STOP strict C17 " + label + " O" + optimize +
                                         " failure: " + compiled.stderr[:3000])
                actual = run([str(binary)], timeout=20)
                if actual.returncode != 0 or "R1109 RAW VERDICT" not in actual.stderr:
                    raise AssertionError("STOP native observer " + label + " O" + optimize +
                                         " rc=" + str(actual.returncode) +
                                         " stderr=" + actual.stderr[:3000])
                print("R1109 REAL NATIVE " + Path(path).name +
                      " compiler=" + compiler + " O" + optimize +
                      " sanitizer=" + sanitizer, flush=True)
                print(actual.stderr, flush=True)
    print("R1109 SOURCE CHECK + emitted C17 + native finite values PASS; "
          "independent public checked-owned validator separately NOT TESTED", flush=True)

def main():
    if len(sys.argv) != 6:
        raise SystemExit("usage: r1109_native.py CLI CASE SOURCE SHA SANITIZER")
    _, cli, case, source, digest, sanitizer = sys.argv
    source_bytes(source, digest)
    if case in ("positive1", "positive2"):
        positive(cli, source, 1 if case == "positive1" else 2, sanitizer)
    elif case == "negative":
        negative(cli, source)
    else:
        raise AssertionError("STOP unknown frozen case")
if __name__ == "__main__":
    main()
