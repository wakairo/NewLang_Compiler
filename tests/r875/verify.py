"""R875 independent frozen-source/semantic/native oracle; normal GitHub Actions only."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile

compiler, probe, cc, sanitizer, directory = sys.argv[1:]
base = Path(directory)
cases = [
    ("true_then_normal.nl", "5b135246445caa4fa33c38ffa470396ebe15e6d25a29db08bc9abbba6cd1e57e",
     (31, 211), (147, 8), (72, 211), (147, 72)),
    ("false_else_normal.nl", "072bdffbf07e00a493c5a13f72261b5e1a5d13e378766e18acf062ae69c4db29",
     (57, 193), (153, 22), (94, 193), (94, 22)),
]
negative = {
    "wrong_argument_type.nl": "ea1db69fd3bf4a9e45005e27dc410f8041f0d97d14b146669a86b6afa77b64b1",
    "third_argument.nl": "214c726e138c28a12a356314ec9084d4c57de49586d49c9108f9f1450629de11",
    "wrong_unit_return.nl": "22db198cca96bd6748063545e3a45ec05c6f5ba768be34f11b4d5ea0c2060c17",
    "unknown_original_field.nl": "55f7e36cba04fb4a9602f9b165ff2d1027640728b1574b78ff1af153b2116f2f",
}


def run(*args):
    return subprocess.run([str(a) for a in args], capture_output=True)


def success(*args):
    p = run(*args)
    assert p.returncode == 0, (args, p.returncode, p.stdout, p.stderr)
    return p


def single(pattern, text):
    matches = list(re.finditer(pattern, text, flags=re.MULTILINE))
    assert len(matches) == 1, (pattern, len(matches))
    return matches[0]


def function_scope(c, name):
    m = single(r"\b(?:static\s+)?(?:void|int)\s+" + re.escape(name)
               + r"\(void\)\s*\{", c)
    opening = m.end() - 1
    depth = 1
    tokens = re.finditer(r'''/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|[{}]''',
                         c[opening + 1:], re.DOTALL)
    for token in tokens:
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                return opening, opening + token.start() + 1
    raise AssertionError("no owning lexical scope close")


def observer(c, facts, initial_a, initial_b, final_a, final_b):
    # A borrowed pointer is used only while its exact original C lexical
    # object is live. No synthetic guest state, mirror arrays or new writes.
    first, second = facts["base"], facts["receiver"]
    expected_initial = {first: initial_a, second: initial_b}
    expected_final = {first: final_a, second: final_b}
    main_open, main_close = function_scope(c, "main")
    call = single(r"\bnl_fn_\d+\(\);", c[main_open + 1:main_close])
    owner_name = re.search(r"nl_fn_\d+", call.group()).group()
    owner_open, owner_close = function_scope(c, owner_name)
    assert owner_open < owner_close
    inserted = {}
    declarations = ["#include <stdlib.h>", "static volatile int r875_ok;"]
    for binding in expected_initial:
        for index in range(2):
            declarations.append(f"static const volatile uint8_t *r875_{binding}_{index};")
    for binding, values in expected_initial.items():
        ctor = single(r"(?:const )?nl_type_\d+ nl_local_" + str(binding)
                      + r" = \{[^;]+;", c)
        assert owner_open < ctor.start() < ctor.end() < owner_close
        checks = []
        for index, value in enumerate(values):
            name = f"r875_{binding}_{index}"
            checks.append(f"{name}=&nl_local_{binding}.f{index};")
            checks.append(f"if (*{name}!={value}) exit(93);")
        inserted[ctor.end()] = "\n    " + "\n    ".join(checks) + "\n"
    last_checks = []
    for binding, values in expected_final.items():
        for index, value in enumerate(values):
            last_checks.append(f"if (*r875_{binding}_{index}!={value}) exit(93);")
    last_checks += ["r875_ok=1;"]
    for binding in expected_initial:
        for index in range(2):
            last_checks.append(f"r875_{binding}_{index}=NULL;")
    inserted[owner_close] = "\n    " + "\n    ".join(last_checks) + "\n"
    wrapper_return = single(r"\breturn 0;\s*\}", c[main_open:main_close + 1])
    position = main_open + wrapper_return.start()
    inserted[position] = "if (!r875_ok) return 93;\n    "
    for position, text in sorted(inserted.items(), reverse=True):
        c = c[:position] + text + c[position:]
    return c.replace("typedef struct", "\n".join(declarations) + "\n\ntypedef struct", 1)


with tempfile.TemporaryDirectory(prefix="r875-") as scratch:
    tmp = Path(scratch)
    one, two = [base / "sources" / c[0] for c in cases]
    found = success(probe, one, two)
    records = [json.loads(line) for line in found.stdout.decode().splitlines()]
    assert len(records) == 2
    for (filename, sha, initial_a, initial_b, final_a, final_b), data in zip(cases, records):
        source = base / "sources" / filename
        assert hashlib.sha256(source.read_bytes()).hexdigest() == sha
        success(compiler, "--check", source)
        assert data["unknown"] and data["old"] == initial_a[0]
        assert data["incoming"] == final_a[0] and data["normal"] == cases.index(
            (filename, sha, initial_a, initial_b, final_a, final_b))
        unmodified = success(compiler, source).stdout.decode()
        assert unmodified and "typedef struct" in unmodified
        passive = observer(unmodified, data, initial_a, initial_b, final_a, final_b)
        named = tmp / "named.c"
        named.write_bytes(b"preexisting sentinel")
        success(compiler, "--emit-c17-to", named, source)
        assert named.read_text() == unmodified
        for opt in ("-O0", "-O2"):
            flags = [cc, "-std=c17", "-Wall", "-Wextra", "-Werror", "-pedantic-errors", opt]
            if sanitizer != "none":
                flags += ["-fsanitize=" + sanitizer, "-fno-omit-frame-pointer",
                          "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
            for variant, body in (("plain", unmodified), ("passive", passive)):
                file = tmp / (variant + ".c")
                executable = tmp / variant
                file.write_text(body)
                success(*flags, file, "-o", executable)
                result = success(executable)
                assert not result.stderr
        print(f"R875 {filename} source/real public old Copy/Change/return/sibling/"
              f"postcall Unknown and GCC-or-Clang O0/O2 plain+passive C17 PASS")
    for filename, sha in negative.items():
        path = base / "negative" / filename
        assert hashlib.sha256(path.read_bytes()).hexdigest() == sha
        for prefix in (["--check"], []):
            p = run(compiler, *prefix, path)
            assert p.returncode != 0 and not p.stdout, (filename, prefix, p.stderr)
        named.write_bytes(b"preexisting sentinel")
        p = run(compiler, "--emit-c17-to", named, path)
        assert p.returncode != 0 and not p.stdout
        assert named.read_bytes() == b"preexisting sentinel"
        print(f"R875 negative {filename} REJECT / unchanged named output PASS")
    print("R875 reached injected OOM: NOT TESTED")
