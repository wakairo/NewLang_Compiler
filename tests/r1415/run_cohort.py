"""Finite R1415 execution. Original files are verified before execution."""
import argparse
import hashlib
import json
import pathlib
import re
import shlex
import subprocess
import sys

FROZEN = {
    "amber_true.nl": "ec97c4b3b3997820d9bd2c09c8b989f1cde534605030652094ef1c3e24fbbb61",
    "violet_false.nl": "7e705e9d943f7daaff65c2f06ed1d4d2171453236dfbfcdb8a9f8cc6143afc3c",
    "wrong_domain.nl": "1f41c1f09db8aff4076f7a87f36042e5839264d8783be81f9da2661ca3a57368",
    "oracle-v1.txt": "73e243b1ced8015014ce2fe9831f214d3b6dbe08bfd42c806e1b8d982123b8dd",
}
COMBINED = "94ba616e858a0136a343277f24dee55d3e36b50cbdabc5d042d19e64df17d942"


def log(**fields):
    print("R1415 " + json.dumps(fields, sort_keys=True), flush=True)


def execute(command, label, directory):
    log(command=command, stage=label)
    try:
        p = subprocess.run(command, stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE, text=True, timeout=180)
    except subprocess.TimeoutExpired as exc:
        log(stage=label, stop="timeout", stdout=str(exc.stdout),
            stderr=str(exc.stderr))
        raise RuntimeError("finite subprocess timeout") from exc
    (directory / (label + ".stdout.txt")).write_text(p.stdout)
    (directory / (label + ".stderr.txt")).write_text(p.stderr)
    print(p.stdout, end="", flush=True)
    print(p.stderr, end="", file=sys.stderr, flush=True)
    log(stage=label, returncode=p.returncode)
    return p


def verify(directory):
    combined = bytearray()
    for name, expected in FROZEN.items():
        raw = (directory / name).read_bytes()
        actual = hashlib.sha256(raw).hexdigest()
        if actual != expected:
            raise RuntimeError("frozen file identity changed: " + name)
        combined.extend(("tests/r1415/" + name).encode() + b"\0" + raw)
        log(file=name, bytes=len(raw), sha256=actual)
    if hashlib.sha256(combined).hexdigest() != COMBINED:
        raise RuntimeError("combined frozen identity changed")


def carrier_schema(code):
    """Recognize a finite C carrier; no layout or guest state is fabricated."""
    clean = re.sub(r"/\*.*?\*/|//[^\n]*", "", code, flags=re.S)
    structures = {}
    aliases = {}
    for tag, alias in re.findall(
            r"typedef\s+struct\s+(\w+)\s+(\w+)\s*;", clean):
        aliases[alias] = "struct " + tag
    for m in re.finditer(
            r"(typedef\s+)?struct\s*(\w+)?\s*\{([^{}]*)\}\s*(\w+)?\s*;",
            clean):
        typedef, tag, body, alias = m.groups()
        name = alias if typedef and alias else "struct " + (tag or "")
        fields = []
        for declaration in body.split(";"):
            declaration = " ".join(declaration.split())
            if not declaration:
                continue
            field = re.fullmatch(r"(.+[\s*])(\w+)", declaration)
            if not field:
                fields = []
                break
            fields.append((" ".join(field[1].split()), field[2]))
        if fields:
            structures[name] = fields
            if tag:
                aliases["struct " + tag] = name
            if alias:
                aliases[alias] = name

    def resolve(name):
        for _ in range(4):
            target = aliases.get(name, name)
            if target == name:
                break
            name = target
        return name

    candidates = []
    for name, fields in structures.items():
        if len(fields) != 2:
            continue
        marks = [(t, n) for t, n in fields
                 if t in ("uint8_t", "unsigned char")]
        if len(marks) != 1:
            continue
        mark = marks[0][1]
        option = [(t, n) for t, n in fields if n != mark][0]
        option_fields = structures.get(resolve(option[0]), [])
        if len(option_fields) != 2:
            continue
        tag_types = ("bool", "_Bool")
        tags = [(t, n) for t, n in option_fields if t in tag_types]
        pointers = [(t, n) for t, n in option_fields if "*" in t]
        if len(tags) != 1 or len(pointers) != 1:
            continue
        pointee = pointers[0][0].replace("*", "").strip()
        if resolve(pointee) != resolve(name):
            continue
        candidates.append(dict(ctype=name, mark=mark, option=option[1],
                               tag=tags[0][1], pointer=pointers[0][1]))
    if len(candidates) != 1:
        return None
    if not re.search(r"\bint\s+main\s*\(\s*(?:void)?\s*\)", clean):
        return None
    return candidates[0]


NATIVE = r'''
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_aligned_alloc(size_t, size_t);
void __real_free(void *);
#define main r1415_guest_main
#include "generated.c"
#undef main

static void *live[2];
static uintptr_t coordinates[2];
static size_t requests, successes, releases;
static unsigned fail_request;
static uintmax_t none_tag;
static int bad;

static void die(const char *why)
{
    fprintf(stderr, "R1415 NATIVE STOP %s\n", why);
    exit(90);
}

static void *allocated(void *p, size_t n)
{
    if (!p)
        die("unexpected real allocation failure");
    if (releases || successes >= 2 || n != sizeof(@CTYPE@))
        die("observer allocation shape unsupported");
    live[successes] = p;
    coordinates[successes] = (uintptr_t)p;
    ++successes;
    return p;
}

void *__wrap_malloc(size_t n)
{
    ++requests;
    if (requests == fail_request)
        return NULL;
    return allocated(__real_malloc(n), n);
}

void *__wrap_calloc(size_t n, size_t z)
{
    ++requests;
    if (requests == fail_request)
        return NULL;
    if (z && n > SIZE_MAX / z)
        die("calloc size overflow");
    return allocated(__real_calloc(n, z), n * z);
}

void *__wrap_aligned_alloc(size_t a, size_t n)
{
    ++requests;
    if (requests == fail_request)
        return NULL;
    return allocated(__real_aligned_alloc(a, n), n);
}

void __wrap_free(void *p)
{
    if (!p)
        return;
    size_t index = 2;
    for (size_t i = 0; i < 2; ++i)
        if (live[i] && p == live[i])
            index = i;
    if (index == 2 || index != releases)
        die("unknown, duplicate or out-of-order original release");
    if (requests != 2 ||
        (fail_request == 0 &&
         (successes != 2 || !live[1] || coordinates[0] == coordinates[1])) ||
        (fail_request == 2 && successes != 1))
        die("allocation world not complete before original release");
    const @CTYPE@ *object = p;
    unsigned mark = (unsigned)object->@MARK@;
    uintmax_t tag = (uintmax_t)object->@OPTION@.@TAG@;
    bool expected_some = index == 0 && fail_request == 0;
    if (mark != (index == 0 ? 47u : 193u))
        bad = 1;
    if (fail_request == 0 && ((tag != none_tag) != expected_some))
        bad = 1;
    uintptr_t observed = 0;
    if (expected_some && tag != none_tag) {
        observed = (uintptr_t)object->@OPTION@.@POINTER@;
        if (observed != coordinates[@SELECTED@])
            bad = 1;
    }
    printf("R1415 NATIVE release=%zu mark=%u tag=%" PRIuMAX
           " observed=%" PRIuPTR " expected=%" PRIuPTR "\n",
           index, mark, tag, observed,
           expected_some ? coordinates[@SELECTED@] : 0);
    live[index] = NULL;
    ++releases;
    __real_free(p);
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 91;
    fail_request = (unsigned)strtoul(argv[1], NULL, 10);
    none_tag = strtoumax(argv[2], NULL, 10);
    int status = r1415_guest_main();
    size_t wanted_requests = fail_request == 1 ? 1 : 2;
    size_t wanted_successes =
        fail_request == 1 ? 0 : fail_request == 2 ? 1 : 2;
    printf("R1415 NATIVE world=%u status=%d requests=%zu successes=%zu "
           "releases=%zu bad=%d\n", fail_request, status, requests,
           successes, releases, bad);
    return status != 0 || bad || requests != wanted_requests ||
           successes != wanted_successes || releases != wanted_successes ||
           live[0] || live[1];
}
'''


def native(args, generated, case, directory):
    code = generated.read_text()
    schema = carrier_schema(code)
    if schema is None:
        log(case=case, stage="native",
            result="NOT_REACHED_OBSERVER_CARRIER_UNSUPPORTED",
            emitted_c_sha256=hashlib.sha256(generated.read_bytes()).hexdigest())
        print("R1415 OWN GENERATED C BEGIN\n" + code +
              "\nR1415 OWN GENERATED C END", flush=True)
        return False
    log(case=case, stage="native_schema", observed=schema)
    wrapper = NATIVE
    for key, value in schema.items():
        wrapper = wrapper.replace("@" + key.upper() + "@", value)
    wrapper = wrapper.replace("@SELECTED@", "0" if case == "amber_true" else "1")
    (directory / "observer.c").write_text(wrapper)
    executable = directory / "native"
    command = [args.compiler, *shlex.split(args.cflags), "-std=c17",
               "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O0",
               str(directory / "observer.c"), "-o", str(executable),
               "-Wl,--wrap=malloc,--wrap=calloc,--wrap=aligned_alloc,--wrap=free"]
    if args.sanitizer != "none":
        command.extend(["-fsanitize=" + args.sanitizer,
                        "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"])
    compiled = execute(command, case + "_strict_c17", directory)
    if compiled.returncode:
        raise RuntimeError("strict C17 compilation failed; preserve output")
    none_tag = "0"
    for failure in (1, 2, 0):
        p = execute([str(executable), str(failure), none_tag],
                    case + "_native_w" + str(failure), directory)
        if p.returncode:
            raise RuntimeError("native original identity/failure/cleanup mismatch")
        if failure == 2:
            found = re.findall(r"release=0 mark=47 tag=(\d+)", p.stdout)
            if len(found) != 1:
                raise RuntimeError("native None carrier observation missing")
            none_tag = found[0]
    log(case=case, stage="native", result="REACHED_ALL_THREE_ALLOCATION_WORLDS")
    return True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--harness", required=True)
    parser.add_argument("--compiler-cli", required=True)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--cflags", default="")
    parser.add_argument("--sanitizer", required=True)
    parser.add_argument("--source-dir", type=pathlib.Path, required=True)
    parser.add_argument("--output-dir", type=pathlib.Path, required=True)
    args = parser.parse_args()
    verify(args.source_dir)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    complete = True
    supported = None
    records = []
    for case in ("amber_true", "violet_false", "wrong_domain"):
        negative = case == "wrong_domain"
        directory = args.output_dir / case
        directory.mkdir(parents=True, exist_ok=True)
        source = args.source_dir / (case + ".nl")
        cli = execute([args.compiler_cli, "--check", str(source)],
                      case + "_cli_check", directory)
        if negative and cli.returncode == 0:
            raise RuntimeError("negative CLI semantic acceptance; never run native")
        generated = directory / "generated.c"
        mode = "negative" if negative else "baseline"
        baseline = execute([args.harness, str(source), mode, str(generated)],
                           case + "_public", directory)
        records.append(dict(case=case, cli=cli.returncode,
                            public=baseline.returncode))
        if baseline.returncode in (70, 71) or baseline.returncode < 0:
            raise RuntimeError("first unexpected semantic or harness outcome")
        if baseline.returncode not in (0, 77, 78):
            raise RuntimeError("unclassified public harness status")
        if baseline.returncode != 0:
            complete = False
            log(case=case, result="FINITE_LIMIT",
                downstream="NOT_REACHED; not native PASS")
            continue
        if negative:
            continue
        if cli.returncode != 0:
            raise RuntimeError("CLI/public acceptance discrepancy")
        if supported is None:
            supported = (source, directory)
            for stage in ("source", "parser", "register", "entry", "emit"):
                p = execute([args.harness, str(source), stage,
                             str(directory / ("oom_" + stage + ".c"))],
                            case + "_oom_" + stage, directory)
                if p.returncode not in (0, 77):
                    raise RuntimeError("OOM/no-clobber/rollback mismatch")
                log(stage="oom_" + stage,
                    result="REACHED" if p.returncode == 0 else "NOT_TRIGGERED")
        if not native(args, generated, case, directory):
            complete = False
    if supported is None:
        log(stage="host_OOM", result="NOT_REACHED_NO_SUPPORTED_BASELINE")
    log(stage="ptr_if_public_authentication",
        result="NOT_REACHED_NO_APPLICABLE_PUBLIC_TYPED_INTERFACE")
    summary = dict(records=records, complete_native=complete,
                   proof_limit="typed ptr-IF candidate negatives NOT_REACHED",
                   provenance="inherited independent FIRST; C875 CI exposure")
    (args.output_dir / "summary.json").write_text(json.dumps(summary, indent=2))
    log(stage="FINAL", **summary)
    return 0 if complete else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        log(stage="STOP", reason=str(error))
        raise SystemExit(70)
