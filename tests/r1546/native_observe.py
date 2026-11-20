#!/usr/bin/env python3
"""R1546 independent source-first CLI and C17 runtime-value observer.

This observer does not read producer tests/oracles and does not supply computed
guest quotient/next values. It instruments *already emitted guest C17* solely
to report its actual intermediate values; the original emitted guest is also
compiled and run unmodified. An unrecognized emitter shape is INCONCLUSIVE.
"""
import hashlib
import pathlib
import re
import subprocess
import sys
import tempfile

FROZEN_SHA = {
    "positive": "cd6cce6fe32198ab07e15d8f811714a059741e262a71d46c05f419fdfb7323eb",
    "negative": "e1ca3d515b7d8288d45be6ac3f19e27144f1be62bc5b22715ddd9500e9e58099",
}


class ObservationError(Exception):
    pass


def call(args, timeout=110):
    p = subprocess.run(args, capture_output=True, text=True, timeout=timeout,
                       check=False)
    print("R1546_CMD=", repr(args), "return=", p.returncode, flush=True)
    if p.stderr:
        print("R1546_STDERR:\n" + p.stderr, flush=True)
    if p.stdout and not any(str(a).endswith("newlangc") for a in args):
        print("R1546_STDOUT:\n" + p.stdout, flush=True)
    return p


def functions(c):
    """Find simple emitted C17 function definition bodies without rewriting them."""
    pat = re.compile(
        r"(?m)^\s*(?:static\s+)?(?:inline\s+)?"
        r"(?:int|void|size_t|uint64_t|unsigned\s+long(?:\s+long)?)\s+"
        r"([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{"
    )
    found = []
    for m in pat.finditer(c):
        brace = c.find("{", m.start(), m.end())
        depth = 1
        end = brace + 1
        while end < len(c) and depth:
            if c[end] == "{":
                depth += 1
            elif c[end] == "}":
                depth -= 1
            end += 1
        if depth:
            raise ObservationError("unbalanced C17 function body")
        found.append((m.group(1), brace + 1, end - 1,
                      c[brace + 1:end - 1]))
    return found


def instrument(raw):
    blocks = functions(raw)
    # The helper must actually contain division by the independently chosen 23.
    candidates = [(name, begin, end, body) for name, begin, end, body in blocks
                  if "/" in body and re.search(r"\b23\b", body)]
    if len(candidates) != 1:
        raise ObservationError(
            "cannot uniquely locate emitted helper doing runtime /23: " +
            repr([(name, body[:250]) for name, _, _, body in candidates]))
    name, begin, _, helper = candidates[0]
    returns = list(re.finditer(r"\breturn\s+([^;]+);", helper))
    if len(returns) != 1:
        raise ObservationError(
            "cannot uniquely locate quotient return in " + name)
    ret = returns[0]
    expression = ret.group(1).strip()
    # Compute exactly once; the result is still the original returned quantity.
    replacement = (
        "\n    size_t r1546_observed_quotient = (size_t)(" +
        expression +
        ");\n    fprintf(stderr, \"R1546_Q=%zu\\n\", "
        "r1546_observed_quotient);\n"
        "    return r1546_observed_quotient;"
    )
    quotient_patch = (begin + ret.start(), begin + ret.end(), replacement)

    # Find the true guest caller's existing +1 assignment and print its result
    # immediately afterward. Never reconstruct it as 69+1 in the observer.
    add_pat = re.compile(
        r"(?m)^[ \t]*(?:const[ \t]+)?(?:size_t[ \t]+)?"
        r"(?P<var>[A-Za-z_]\w*)[ \t]*=[ \t]*"
        r"(?P<expr>[^;\n]*\+[^;\n]*);"
    )
    sums = []
    for fun_name, begin, _, body in blocks:
        if fun_name == name:
            continue
        for m in add_pat.finditer(body):
            expr = m.group("expr")
            if re.search(r"\b1(?:u|U|UL|ULL)?\b", expr):
                sums.append((fun_name, begin + m.end(), m.group("var"), expr))
    if len(sums) != 1:
        raise ObservationError(
            "cannot uniquely locate computed caller +1 assignment: " +
            repr(sums))
    _, insert_pos, variable, expr = sums[0]
    addition_patch = (
        insert_pos, insert_pos,
        "\n    fprintf(stderr, \"R1546_EXTRA=%zu\\n\", "
        "(size_t)(" + variable + "));"
    )
    # The patches only add an observer and preserve original arithmetic.
    patched = raw
    for lo, hi, text in sorted([quotient_patch, addition_patch], reverse=True):
        patched = patched[:lo] + text + patched[hi:]
    return "#include <stdio.h>\n" + patched


def native(compiler, source, cc, sanitizer):
    with tempfile.TemporaryDirectory(prefix="r1546-native-") as tmp:
        base = pathlib.Path(tmp)
        check = call([compiler, "--check", str(source)])
        if check.returncode != 0 or check.stdout or check.stderr:
            raise ObservationError("positive SOURCE/CHECK not accepted cleanly")
        emit = call([compiler, str(source)])
        if emit.returncode != 0 or not emit.stdout:
            raise ObservationError("emission missing: SOURCE/CHECK does not imply C17")
        raw = emit.stdout
        (base / "guest_raw.c").write_text(raw, encoding="utf-8")
        options = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O0"]
        if sanitizer not in ("none", "address", "undefined"):
            raise ObservationError("unexpected sanitizer profile " + sanitizer)
        if sanitizer != "none":
            options.extend(["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all"])
        plain = base / "guest_native"
        build = call([cc, *options, str(base / "guest_raw.c"), "-o", str(plain)])
        if build.returncode != 0:
            raise ObservationError("unmodified strict C17 compilation failed")
        original = call([str(plain)])
        if original.returncode != 0:
            raise ObservationError("unmodified native guest failed")
        try:
            modified = instrument(raw)
        except ObservationError:
            print("R1546_RAW_C17_FOR_REVIEW_BEGIN\n" + raw +
                  "\nR1546_RAW_C17_FOR_REVIEW_END", flush=True)
            raise
        (base / "guest_observed.c").write_text(modified, encoding="utf-8")
        binary = base / "guest_observed"
        built = call([cc, *options, str(base / "guest_observed.c"), "-o",
                      str(binary)])
        if built.returncode != 0:
            print("R1546_INSTRUMENTED_C17_BEGIN\n" + modified +
                  "\nR1546_INSTRUMENTED_C17_END", flush=True)
            raise ObservationError("read-only C17 instrumented build failed")
        observed = call([str(binary)])
        if observed.returncode != 0:
            raise ObservationError("instrumented native guest failed")
        qs = re.findall(r"(?m)^R1546_Q=(\d+)$", observed.stderr)
        extra = re.findall(r"(?m)^R1546_EXTRA=(\d+)$", observed.stderr)
        if qs != ["69"] or extra != ["70"]:
            raise ObservationError(
                "actual native numeric mismatch or unobserved, "
                f"quotient={qs} successor={extra}; exit0 is not proof")
        print("R1546_NATIVE_VERIFIED runtime_quotient=69 "
              "runtime_caller_extra=70 observed_from_generated_guest",
              flush=True)


def main():
    if len(sys.argv) != 6:
        raise ObservationError(
            "usage: native_observe.py MODE NEWLANGC SOURCE CC SANITIZER")
    mode, compiler, path, cc, sanitizer = sys.argv[1:]
    if mode not in ("check-positive", "check-negative", "native"):
        raise ObservationError("unknown R mode")
    source = pathlib.Path(path)
    intended = "negative" if mode == "check-negative" else "positive"
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    if digest != FROZEN_SHA[intended]:
        raise ObservationError("source changed since V3 FIRST: " + digest)
    if mode == "native":
        native(compiler, source, cc, sanitizer)
        return
    result = call([compiler, "--check", str(source)])
    if mode == "check-positive":
        if result.returncode != 0 or result.stdout or result.stderr:
            raise ObservationError("positive --check rejection")
        print("R1546_POSITIVE_SOURCE_CHECK=ACCEPT", flush=True)
    else:
        # Exit 3 covers parse/semantic failure in public CLI. R public probe
        # separately distinguishes parser refusal from semantic precondition.
        if result.returncode != 3 or result.stdout:
            raise ObservationError(
                "zero divisor erroneously accepted or wrong failure kind")
        print("R1546_NEGATIVE_CLI=REJECT (precise layer via public probe)",
              flush=True)


if __name__ == "__main__":
    try:
        main()
    except (ObservationError, OSError, subprocess.TimeoutExpired) as exc:
        print("R1546_STOP_OR_INCONCLUSIVE:", str(exc), file=sys.stderr,
              flush=True)
        sys.exit(1)
