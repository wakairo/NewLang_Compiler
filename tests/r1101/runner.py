"""R1101: frozen-source independent CHECK/C17/native audit (GitHub Actions only).

Independent expectations are the PRE-REF C194 V1 source/oracle, never producer fixtures.
Instrumentation observes live generated-C original field pointers; it creates no guest state.
"""
from __future__ import annotations

import hashlib
import json
import os
import pathlib
import re
import subprocess
import sys
import tempfile

COMPILER, CC, SANITIZER, DATA, MODE = sys.argv[1:]
ROOT = pathlib.Path(DATA)
EXPECTED_SHA = {
    "r1101_p01_break_old_index0.nl": "574774848bdaf6bd0fe2f1b702c6b956ab8747c34ddbf8877156fa4328bd3f11",
    "r1101_p02_break_old_known_call_index1.nl": "89395505eeed8e848ade296aafec5edb473206873862de0b3a646b11afd3fb22",
    "r1101_n01_bare_break.nl": "e631da199fc07b29858fe4674e022729beb9e1dee3c2223af9aea920ab95f0bc",
    "r1101_n02_loan_ref_escape_break.nl": "8744ec771dc61ebd64d9a5c43b40aa0db91aef48c3afff3d2b5636cbfecc0226",
    "r1101_n03_unknown_field_break.nl": "24115dd646f39ce81aedabff7ab47776d890b3e2fe02247141d32d96fbd982b1",
    "r1101_n04_break_type_mismatch.nl": "f19dc3802d32e5e0ccebfcc0ad8b3dbf21b8c4e1eecd2e2c2d12ebf5ee12f24c",
    "r1101_oracle.json": "986fa4723a99f4cb2116730db696c8b4c57ac4e2ca6159b1605c56438b5f84a8",
}
POS = {
    "p01": "r1101_p01_break_old_index0",
    "p02": "r1101_p02_break_old_known_call_index1",
}
NEG = ("r1101_n01_bare_break", "r1101_n02_loan_ref_escape_break",
       "r1101_n03_unknown_field_break", "r1101_n04_break_type_mismatch")


def die(category: str, detail: str) -> None:
    raise RuntimeError(f"R1101 STOP [{category}] {detail}")


def execute(command: list[str], label: str) -> subprocess.CompletedProcess:
    try:
        p = subprocess.run([str(x) for x in command], capture_output=True, timeout=70)
    except subprocess.TimeoutExpired:
        die("TIMEOUT", label + ": command exceeded 70 seconds")
    print(f"R1101 RAW phase={label} argv={command!r} returncode={p.returncode} "
          f"stdout_sha256={hashlib.sha256(p.stdout).hexdigest()} "
          f"stderr_sha256={hashlib.sha256(p.stderr).hexdigest()} "
          f"stdout_bytes={len(p.stdout)} stderr_bytes={len(p.stderr)}", flush=True)
    artifact_dir = os.environ.get("R1101_ARTIFACT_DIR")
    if artifact_dir:
        destination = pathlib.Path(artifact_dir)
        destination.mkdir(parents=True, exist_ok=True)
        safe_name = re.sub(r"[^A-Za-z0-9_.-]", "_", label)
        (destination / (safe_name + ".stdout")).write_bytes(p.stdout)
        (destination / (safe_name + ".stderr")).write_bytes(p.stderr)
    if p.stderr:
        print(f"R1101 RAW STDERR {label}: {p.stderr.decode('utf-8', 'replace')[:3000]!r}",
              flush=True)
    return p


def freeze() -> dict:
    for name, digest in EXPECTED_SHA.items():
        file = ROOT / ("sources" if name.endswith(".nl") else "") / name
        raw = file.read_bytes()
        observed = hashlib.sha256(raw).hexdigest()
        if observed != digest or not raw.endswith(b"\n") or b"\r" in raw:
            die("FREEZE", f"{file}: expected={digest} actual={observed}")
        print(f"R1101 V2 FORWARD-ONLY BYTE-FREEZE {name} bytes={len(raw)} sha256={observed}",
              flush=True)
    oracle = json.loads((ROOT / "r1101_oracle.json").read_bytes())
    assert oracle["authority"] == {
        "main": "1de346f545afd638cacbe7589909cbeda3bf4e71",
        "spec": "docs/reference/NewLang_v0_spec_Draft17_34.md",
        "pr_head": "6ee04e0eaef593e151641da2bf5dda50f9fcb47e",
    }, "frozen authority differs"
    return oracle


# This is compiled as a HEADER in genuine emitted C17, not as a synthetic
# alternate program. All root pointers originate from real generated C locals.
# No guest field is written by any observer function.
NATIVE_OBSERVER = r"""
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static const uint8_t *r_roots[2][2];
static unsigned r_bind_count, r_before_count, r_after_count, r_read_count;
static uint8_t r_old_seen, r_new_seen, r_values[4];
static const uint8_t r_initial[2][2] = {
    {R_INIT00, R_INIT01}, {R_INIT10, R_INIT11}
};
static const unsigned r_read_root[4] = {R_RROOT0, R_RROOT1, R_RROOT2, R_RROOT3};
static const unsigned r_read_field[4] = {R_RFIELD0, R_RFIELD1, R_RFIELD2, R_RFIELD3};
static const uint8_t r_read_expected[4] = {R_RVAL0, R_RVAL1, R_RVAL2, R_RVAL3};
static void r_stop(const char *reason)
{
    fprintf(stderr, "R1101 NATIVE STOP: %s\n", reason);
    exit(83);
}
static void r_bind(const uint8_t *f0, const uint8_t *f1)
{
    if (!f0 || !f1 || f0 == f1 || r_bind_count >= 2)
        r_stop("invalid source-original field placement");
    for (unsigned root = 0; root < r_bind_count; ++root)
        for (unsigned field = 0; field < 2; ++field)
            if (f0 == r_roots[root][field] || f1 == r_roots[root][field])
                r_stop("two original roots overlap");
    unsigned root = r_bind_count++;
    if (*f0 != r_initial[root][0] || *f1 != r_initial[root][1])
        r_stop("root bind observed unexpected real initial bits");
    r_roots[root][0] = f0;
    r_roots[root][1] = f1;
}
static void r_before(const uint8_t *f0, const uint8_t *f1, size_t index)
{
    if (r_bind_count != 2 || r_before_count++ != 0 || r_after_count != 0)
        r_stop("wrong number/order of real source Change");
    if (index != R_INDEX || f0 != r_roots[0][0] || f1 != r_roots[0][1])
        r_stop("Change before uses wrong physical original/field");
    if (*(r_roots[0][index]) != R_OLD ||
        *(r_roots[0][1 - index]) != R_SIBLING)
        r_stop("before-Change source-original state mismatch");
}
static void r_after(const uint8_t *f0, const uint8_t *f1,
                    size_t index, uint8_t old)
{
    if (r_before_count != 1 || r_after_count++ != 0)
        r_stop("duplicate or unordered Change");
    if (index != R_INDEX || f0 != r_roots[0][0] || f1 != r_roots[0][1])
        r_stop("after-Change has different physical original");
    if (old != R_OLD || *(r_roots[0][index]) != R_NEW ||
        *(r_roots[0][1 - index]) != R_SIBLING)
        r_stop("old Copy/current/sibling incorrect after Change");
    r_old_seen = old;
    r_new_seen = *(r_roots[0][index]);
}
static void r_read(const uint8_t *actual, uint8_t observed)
{
    if (r_bind_count != 2 || r_before_count != 1 || r_after_count != 1 ||
        r_read_count >= 4)
        r_stop("field read count or control-flow order incorrect");
    unsigned n = r_read_count++;
    if (actual != r_roots[r_read_root[n]][r_read_field[n]])
        r_stop("field read is not same lexical original incarnation");
    if (*actual != observed || observed != r_read_expected[n])
        r_stop("caller reads differ from actual live original field");
    r_values[n] = observed;
    if (r_read_count == 4)
        printf("R1101_NATIVE_PASS old=%u postcurrent=%u "
               "reads=%u,%u,%u,%u "
               "physical_roots=%p/%p;%p/%p before=1 after=1\n",
               (unsigned)r_old_seen, (unsigned)r_new_seen,
               (unsigned)r_values[0], (unsigned)r_values[1],
               (unsigned)r_values[2], (unsigned)r_values[3],
               (void *)r_roots[0][0], (void *)r_roots[0][1],
               (void *)r_roots[1][0], (void *)r_roots[1][1]);
}
#define NL_LOOP_BIND(a,b) r_bind((a),(b))
#define NL_LOOP_BEFORE(a,b,i) r_before((a),(b),(i))
#define NL_LOOP_AFTER(a,b,i,old) r_after((a),(b),(i),(old))
#define NL_LOOP_READ(p,v) r_read((p),(v))
"""


def native_plan(which: str, oracle: dict) -> tuple[dict, list]:
    e = oracle["cases"][POS[which]]
    if which == "p01":
        # Frozen Ledger: old_field is physical index0; receiver is independent.
        a = [[37, 148], [6, 209]]
        reads = [(0, 0, e["current_after_break"]),
                 (0, 1, e["untouched_sibling"]),
                 (1, 1, e["receiver_after"]),
                 (1, 0, e["receiver_sibling"])]
        index, old, new, sibling = 0, e["old_from_break"], e["current_after_break"], e["untouched_sibling"]
    else:
        # Frozen Beacons: high is declaration index1, notwithstanding constructor order.
        a = [[12, 173], [57, 4]]
        reads = [(0, 1, e["current"]), (0, 0, e["untouched_low"]),
                 (1, 0, e["mirror_current"]), (1, 1, e["mirror_untouched_high"])]
        index, old, new, sibling = 1, e["returned"], e["current"], e["untouched_low"]
    d = {"INDEX": index, "OLD": old, "NEW": new, "SIBLING": sibling}
    for root in range(2):
        for field in range(2):
            d[f"INIT{root}{field}"] = a[root][field]
    for n, (root, field, bits) in enumerate(reads):
        d[f"RROOT{n}"] = root
        d[f"RFIELD{n}"] = field
        d[f"RVAL{n}"] = bits
    return d, reads


def inspect_real_emission(c: bytes, plan: dict, reads: list) -> None:
    text = c.decode("utf-8")
    bind = re.findall(r"NL_LOOP_BIND\(&nl_local_(\d+)\.f0, &nl_local_\1\.f1\);", text)
    if len(bind) != 2 or len(set(bind)) != 2:
        die("PHYSICAL IDENTITY", f"actual aggregate bind set wrong: {bind}")
    idx = plan["INDEX"]
    before_pattern = (r"NL_LOOP_BEFORE\(&nl_local_(\d+)\.f0, "
                      r"&nl_local_\1\.f1, " + str(idx) + r"\);")
    before = re.search(before_pattern, text)
    after_pattern = (r"NL_LOOP_AFTER\(&nl_local_(\d+)\.f0, "
                     r"&nl_local_\1\.f1, " + str(idx) +
                     r", nl_local_(\d+)\);")
    after = re.search(after_pattern, text)
    if not before or not after or before.group(1) != bind[0] or after.group(1) != bind[0]:
        die("PHYSICAL IDENTITY", "before/after not sourced from same bound original")
    if before.end() >= after.start():
        die("PHYSICAL IDENTITY", "Change observer ordering invalid")
    change = text[before.end():after.start()]
    old_match = re.search(
        r"(?:const )?uint8_t nl_local_(\d+) = nl_local_" +
        re.escape(bind[0]) + r"\.f" + str(idx) + r";", change)
    store = re.search(
        r"nl_local_" + re.escape(bind[0]) + r"\.f" + str(idx) +
        r" = " + str(plan["NEW"]) + r";", change)
    if not old_match or not store or old_match.start() >= store.start() or old_match.group(1) != after.group(2):
        die("PHYSICAL IDENTITY", "generated C17 old load/store do not share original and returned old")
    if len(re.findall(r"NL_LOOP_BEFORE\(&nl_local_", text)) != 1 or len(re.findall(r"NL_LOOP_AFTER\(&nl_local_", text)) != 1:
        die("NO DOUBLE CHANGE", "unexpected generated C observer invocations")
    observed_reads = re.findall(r"NL_LOOP_READ\(&nl_local_(\d+)\.f(\d+), nl_local_\d+\);", text)
    expected_reads = [(bind[root], str(field)) for root, field, _ in reads]
    if observed_reads != expected_reads:
        die("PHYSICAL IDENTITY", f"generated caller read address sequence {observed_reads} != {expected_reads}")
    print("R1101 GENERATED C17 identity: genuine same root and physical field "
          "for one old load + one store + later four source caller field reads; "
          f"root-local tokens={bind} source_bytes={len(c)} "
          f"sha256={hashlib.sha256(c).hexdigest()}", flush=True)


def run_negative(oracle: dict) -> None:
    for name in NEG:
        source = ROOT / "sources" / (name + ".nl")
        check = execute([COMPILER, "--check", source], name + ".CHECK")
        if check.returncode == 0:
            die("UNSAFE FALSE ACCEPT", name + " canonical-invalid V1 --check returned zero")
        c17 = execute([COMPILER, source], name + ".C17")
        if c17.returncode == 0 or c17.stdout:
            die("UNSAFE C17", name + " canonical-invalid V1 emitted C17 or returned success")
        print(f"R1101 NEGATIVE_REJECT {name} check_status={check.returncode} "
              f"emit_status={c17.returncode} first-layer diagnostic above "
              f"frozen={oracle['cases'][name]['check']}", flush=True)


def run_positive(which: str, oracle: dict) -> None:
    key = POS[which]
    source = ROOT / "sources" / (key + ".nl")
    check = execute([COMPILER, "--check", source], key + ".CHECK")
    if check.returncode:
        if which == "p02":
            print("R1101 P02 LEGAL-CANONICAL / IMPLEMENTATION CHECK REFUSAL: "
                  "C17/native NOT VERIFIED. Do not classify as canonical-invalid.", flush=True)
            return
        die("P01 CHECK NOT VERIFIED", "canonical-legal frozen P01 rejected; inspect syntax vs support")
    if check.stdout:
        die("CHECK", key + " successful check emitted unexpected stdout")
    print(f"R1101 POSITIVE CHECK ACCEPT {key} (native still requires separate evidence)",
          flush=True)
    emission = execute([COMPILER, source], key + ".C17")
    if emission.returncode:
        if emission.stdout:
            die("C17 SAFETY", key + " failure emitted partial C")
        if which == "p02":
            print("R1101 P02 CHECK_ACCEPTED / C17 UNSUPPORTED; NATIVE NOT VERIFIED",
                  flush=True)
            return
        die("P01 C17 NOT VERIFIED", "legal checked P01 not emitted; first layer C17 refusal")
    if not emission.stdout:
        die("C17", key + " successful emission has empty output")
    plan, reads = native_plan(which, oracle)
    inspect_real_emission(emission.stdout, plan, reads)
    with tempfile.TemporaryDirectory(prefix="r1101-genuine-") as temp:
        root = pathlib.Path(temp)
        c_path = root / "genuine_generated.c"
        c_path.write_bytes(emission.stdout)
        h_path = root / "r1101_readonly_observer.h"
        header = "".join(f"#define R_{key} {value}\n" for key, value in plan.items())
        h_path.write_text(header + NATIVE_OBSERVER, encoding="ascii")
        named = root / "named.c"
        named.write_bytes(b"R1101_PREEXISTING_TARGET\x00\xff\n")
        p = execute([COMPILER, "--emit-c17-to", named, source], key + ".NAMED")
        if p.returncode or p.stdout or named.read_bytes() != emission.stdout:
            die("NAMED C17", key + " named emission differs from ordinary emitted C")
        for opt in ("-O0", "-O2"):
            output = root / ("native" + opt)
            flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", opt,
                     '-DNEWLANG_LOOP_OBSERVER="' + str(h_path) + '"']
            if "clang" in pathlib.Path(CC).name:
                flags += ["-Wno-error=gcc-install-dir-libstdcxx"]
            if SANITIZER != "none":
                flags += ["-fsanitize=" + SANITIZER, "-fno-omit-frame-pointer",
                          "-fno-pie", "-no-pie"]
            build = execute([CC, *flags, c_path, "-o", output], key + "." + opt + ".C17_BUILD")
            if build.returncode:
                die("STRICT C17", key + " generated native C17 does not compile with warnings-as-errors")
            native = execute([output], key + "." + opt + ".NATIVE")
            if native.returncode != 0 or native.stderr or native.stdout.count(b"R1101_NATIVE_PASS") != 1:
                die("NATIVE/PHYSICAL IDENTITY", key + " read-only native original observer failed")
            print(f"R1101 VERIFIED {key} {pathlib.Path(CC).name} {opt} {SANITIZER} "
                  + native.stdout.decode("ascii").strip(), flush=True)


def main() -> None:
    oracle = freeze()
    if MODE == "negative":
        run_negative(oracle)
    elif MODE in POS:
        run_positive(MODE, oracle)
    else:
        die("HARNESS", "invalid test mode")
    print("R1101 TEST_SCOPE_COMPLETE mode=" + MODE, flush=True)


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, AssertionError, OSError, ValueError) as e:
        print(str(e), file=sys.stderr, flush=True)
        sys.exit(1)
