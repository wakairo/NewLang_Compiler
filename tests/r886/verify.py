"""Independent R886 calibrated oracle, issued proof and C17/native receipt.
Executed only by authorized GitHub Actions. The frozen source/oracle is not modified.
"""
from pathlib import Path
import base64
import hashlib
import json
import re
import subprocess
import sys
import tempfile

compiler, probe, cc, sanitizer, directory = sys.argv[1:]
base = Path(directory)
cases = [
    ("normal.nl", (67, 109), (211, 29), (143, 109), (53, 29),
     1, 0, 67, 73, 73),
    ("terminal.nl", (48, 161), (216, 81), (48, 161), (35, 81),
     0, 1, None, 44, 45),
]


def run(*args):
    return subprocess.run([str(a) for a in args], capture_output=True)


def success(*args):
    completed = run(*args)
    assert completed.returncode == 0, (
        args, completed.returncode, completed.stdout, completed.stderr
    )
    return completed


def single(pattern, source):
    found = list(re.finditer(pattern, source, flags=re.MULTILINE))
    assert len(found) == 1, (pattern, len(found))
    return found[0]


def function_scope(source, name):
    found = single(r"\b(?:static\s+)?(?:void|int)\s+" + re.escape(name)
                   + r"\([^)]*\)\s*\{", source)
    opening = found.end() - 1
    depth = 1
    tokens = re.finditer(
        r"""'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|[{}]'""".strip("'"),
        source[opening + 1:],
        re.DOTALL,
    )
    for token in tokens:
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                return opening, opening + token.start() + 1
    raise AssertionError("no closing brace for owning live scope")


def passive_derivative(raw, facts, specimen):
    (filename, initial_original, initial_receiver, final_original,
     final_receiver, normal_count, return_count, dynamic_old,
     left, right) = specimen
    main_open, main_close = function_scope(raw, "main")
    call = single(r"\bnl_fn_\d+\(\);", raw[main_open + 1:main_close])
    owner = re.search(r"nl_fn_\d+", call.group()).group()
    owner_open, owner_close = function_scope(raw, owner)
    assert owner_open < owner_close < main_open
    callee_header = single(
        r"static void (nl_fn_\d+)\(uint8_t \*nl_local_\d+,\s*uint8_t nl_local_\d+\)\s*\{",
        raw,
    )
    callee_name = callee_header.group(1)
    callee_open, callee_close = function_scope(raw, callee_name)
    assert callee_open > owner_close or callee_close < owner_open
    actual_callee = raw[callee_open:callee_close]
    assert "UINTMAX_C(" + str(left) + ")" in actual_callee
    assert "UINTMAX_C(" + str(right) + ")" in actual_callee
    write = single(r"\*nl_local_\d+\s*=\s*nl_local_\d+;", actual_callee)
    returned = single(r"\breturn;", actual_callee)
    old_decl = single(
        r"const uint8_t (nl_replace_old_\d+)\s*=\s*\*nl_local_\d+;",
        actual_callee,
    )
    assert old_decl.end() < write.start() < returned.start()
    insert = {}
    # Global latches are instrumentation only: no fabricated guest state.
    defs = ["#include <stdlib.h>", "static volatile unsigned r886_success;",
            "static volatile unsigned r886_nwrites;",
            "static volatile unsigned r886_nreturns;",
            "static volatile unsigned r886_last_old=655u;"]
    for root in (facts["base"], facts["receiver"]):
        for index in range(2):
            defs.append(
                f"static const volatile uint8_t *r886_original_{root}_{index};"
            )
    assert facts["base"] != facts["receiver"]
    expected_initial = {
        facts["base"]: initial_original,
        facts["receiver"]: initial_receiver,
    }
    expected_final = {
        facts["base"]: final_original,
        facts["receiver"]: final_receiver,
    }
    for root, initial in expected_initial.items():
        ctor = single(
            r"(?:const )?nl_type_\d+ nl_local_" + str(root)
            + r"\s*=\s*\{[^;]+;", raw
        )
        assert owner_open < ctor.start() < ctor.end() < owner_close
        checks = []
        for index, value in enumerate(initial):
            name = f"r886_original_{root}_{index}"
            checks.append(f"{name}=&nl_local_{root}.f{index};")
            checks.append(f"if (*{name}!={value}) exit(93);")
        insert[ctor.end()] = "\n    " + "\n    ".join(checks) + "\n"
    final_checks = []
    for root, final in expected_final.items():
        for index, value in enumerate(final):
            final_checks.append(
                f"if (*r886_original_{root}_{index}!={value}) exit(93);"
            )
    final_checks.extend([
        f"if (r886_nwrites!={normal_count}) exit(93);",
        f"if (r886_nreturns!={return_count}) exit(93);",
        "if (r886_last_old!=" + (
            str(dynamic_old) if dynamic_old is not None else "655u"
        ) + ") exit(93);",
        "r886_success=1u;",
    ])
    for root in expected_initial:
        for index in range(2):
            final_checks.append(f"r886_original_{root}_{index}=NULL;")
    insert[owner_close] = "\n    " + "\n    ".join(final_checks) + "\n"
    insert[callee_open + old_decl.end()] = (
        "\n        r886_last_old=" + old_decl.group(1) + ";"
    )
    insert[callee_open + write.start()] = "r886_nwrites++;\n        "
    insert[callee_open + returned.start()] = "r886_nreturns++;\n        "
    wrapper_return = single(r"\breturn 0;\s*\}", raw[main_open:main_close + 1])
    insert[main_open + wrapper_return.start()] = (
        "if (!r886_success) return 93;\n    "
    )
    derived = raw
    for point, added in sorted(insert.items(), reverse=True):
        derived = derived[:point] + added + derived[point:]
    derived = derived.replace("typedef struct",
                              "\n".join(defs) + "\n\ntypedef struct", 1)
    assert derived != raw and "r886_nreturns++;" in derived
    print(f"R886 PASSIVE_SCOPE {filename} owner={owner} callee={callee_name} "
          f"original={facts['base']} separate_receiver={facts['receiver']} "
          f"normal_writes={normal_count} executed_returns={return_count} "
          f"dynamic_old={'NOT_REACHED' if dynamic_old is None else dynamic_old}",
          flush=True)
    return derived


def receipt(label, filename, content):
    data = content.encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    print(f"R886 {label} {filename} bytes={len(data)} sha256={digest}", flush=True)
    print(f"R886 {label}_BEGIN {filename}", flush=True)
    print(content, end="" if content.endswith("\n") else "\n", flush=True)
    print(f"R886 {label}_END {filename}", flush=True)
    print(f"R886 {label}_BASE64 {filename} "
          f"{base64.b64encode(data).decode('ascii')}", flush=True)
    assert base64.b64decode(base64.b64encode(data)) == data
    return digest


if __name__ == "__main__":
    frozen = {}
    for line in (base / "SHA256SUMS.GATE_A").read_text().splitlines():
        sha, rel = line.split("  ", 1)
        frozen[rel] = sha
        actual = hashlib.sha256((base.parent.parent / rel).read_bytes()).hexdigest()
        assert actual == sha, (rel, sha, actual)
    oracle_file = base / "oracle.json"
    oracle = json.loads(oracle_file.read_text())
    assert oracle["candidate"] == "3485401972ffe881df08d64d2e2cf5208b9b7c43"
    assert oracle["normal"]["calleeReplaceCount"] == 1
    assert oracle["terminal"]["calleeReturnCount"] == 1

    with tempfile.TemporaryDirectory(prefix="r886-actions-") as scratch:
        tmp = Path(scratch)
        one, two = [base / c[0] for c in cases]
        issued = success(probe, one, two)
        records = [json.loads(line) for line in issued.stdout.decode().splitlines()]
        assert len(records) == 2
        for specimen, public in zip(cases, records):
            (filename, initial_a, initial_b, final_a, final_b,
             writes, returns, old_dynamic, _, _) = specimen
            source = base / filename
            assert public["normal"] == 0 and public["index"] == 0
            assert public["old"] == initial_a[0]
            assert public["incoming"] == (143 if writes else 238)
            assert public["unknown"] is True
            assert public["base"] != public["receiver"]
            checked = success(compiler, "--check", source)
            assert not checked.stdout and not checked.stderr
            generated = success(compiler, source)
            raw = generated.stdout.decode("utf-8")
            assert raw and "typedef struct" in raw and not generated.stderr
            sha = receipt("RAW_C17", filename, raw)
            named = tmp / ("named_" + filename + ".c")
            named.write_bytes(b"preexisting sentinel")
            success(compiler, "--emit-c17-to", named, source)
            assert named.read_bytes() == raw.encode()
            passive = passive_derivative(raw, public, specimen)
            passive_sha = receipt("PASSIVE_C17", filename, passive)
            for opt in ("-O0", "-O2"):
                flags = [
                    cc, "-std=c17", "-Wall", "-Wextra",
                    "-Werror", "-pedantic-errors", opt
                ]
                if sanitizer != "none":
                    flags += [
                        "-fsanitize=" + sanitizer, "-fno-omit-frame-pointer",
                        "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"
                    ]
                for variant, body in (("plain", raw), ("passive", passive)):
                    file = tmp / (filename + "_" + variant + ".c")
                    executable = tmp / (filename + "_" + variant)
                    file.write_text(body)
                    success(*flags, file, "-o", executable)
                    result = success(executable)
                    assert not result.stdout and not result.stderr
                    print(f"R886 GUEST {filename} cc={cc} {opt} san={sanitizer} "
                          f"{variant}: compile=0 run=0 no_output=PASS",
                          flush=True)
            print(f"R886 CASE {filename} SOURCE CHECK PUBLIC COPY/IF-RETURN "
                  f"CHANGE sibling postcall-Unknown original-root separate "
                  f"receiver native terminal={returns} writes={writes} "
                  f"raw={sha} passive={passive_sha} PASS", flush=True)
        negatives = sorted((base / "negative").glob("*.nl"))
        assert len(negatives) == 5
        for path in negatives:
            for opts in (("--check",), ()):
                invalid = run(compiler, *opts, path)
                assert invalid.returncode != 0 and not invalid.stdout, (
                    path, opts, invalid.returncode, invalid.stderr
                )
            named = tmp / "negative.c"
            named.write_bytes(b"preexisting sentinel")
            invalid = run(compiler, "--emit-c17-to", named, path)
            assert invalid.returncode != 0 and not invalid.stdout
            assert named.read_bytes() == b"preexisting sentinel"
            print(f"R886 SOURCE NEGATIVE {path.name} CHECK/EMIT rejected "
                  f"named-output preserved PASS", flush=True)
        print(f"R886 COMPLETE cases=2 negatives={len(negatives)} "
              "source-issued public/tamper=PASS and real terminal/native=PASS "
              f"compiler={compiler} cc={cc} sanitizer={sanitizer}", flush=True)
        print("R886 OOM NOT TESTED: no R-owned reached fault index", flush=True)
