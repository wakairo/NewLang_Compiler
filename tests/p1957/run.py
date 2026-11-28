"""Check the computed total, guarded byte, and later caller Copy separately."""
from argparse import ArgumentParser
from contextlib import nullcontext
from pathlib import Path
import re
import runpy
from tempfile import TemporaryDirectory


SUPPORT = runpy.run_path(str(Path(__file__).resolve().parents[1] / "p1654" / "run.py"))
run, state = SUPPORT["run"], SUPPORT["state"]
LOCAL = r"nl_local_\d+"
FUNCTION = r"nl_qty_fn_\d+"
ARMS = (
    "u8(total)",
    "let narrowed = u8(total); narrowed",
    "let narrowed = u8(total); let saved = narrowed; saved",
)


def source(key, arm, bound=256, fallback="u8(173)"):
    return f"""fn stripe_bytes(count: usize) -> usize {{
    if (count < usize(1024)) {{
        count * usize(64)
    }} else {{
        usize(0)
    }}
}}

fn field_byte(total: usize) -> u8 {{
    if (total < usize({bound})) {{ {arm} }} else {{ {fallback} }}
}}

fn main() -> unit {{
    let key = usize({key});
    let count = if (key < usize(2)) {{ usize(3) }} else {{ usize(4) }};
    let bytes = stripe_bytes(count);
    let field = field_byte(bytes);
    let observed = field;
    return unit;
}}
"""


def only(pattern, text, flags=re.M):
    match, = re.finditer(pattern, text, flags)
    return match


def function(code, name, result, formal):
    return only(rf"^static {result} {name}\({formal}\)\n\{{\n(?P<body>.*?)^\}}",
                code, re.M | re.S)


def branches(body, formal, bound):
    match = only(
        rf"^    _Bool nl_condition_(\d+) = {formal} < \(\(size_t\)UINTMAX_C\({bound}\)\);\n"
        r"    if \(nl_condition_\1\) \{\n(?P<yes>.*?)^    \} else \{\n"
        r"(?P<no>.*?)^    \}\n", body, re.M | re.S,
    )
    return match, f"nl_if_{match[1]}"


def observer(raw, key, copies):
    code = raw.decode("ascii")
    entry = only(rf"^int main\(void\)\n\{{\n    ({FUNCTION})\(\);\n    return 0;\n\}}$", code)
    caller = function(code, entry[1], "void", "void")
    key_decl, count, total, byte, copied = re.findall(
        rf"^    (?:size_t|uint8_t) ({LOCAL}) = (.+);$", caller["body"], re.M
    )
    assert key_decl[1] == f"((size_t)UINTMAX_C({key}))"
    by_count, total_if = branches(caller["body"], key_decl[0], 2)
    assert count[1] == total_if
    assert by_count["yes"].strip() == f"{total_if} = ((size_t)UINTMAX_C(3));"
    assert by_count["no"].strip() == f"{total_if} = ((size_t)UINTMAX_C(4));"

    product_name = re.fullmatch(rf"({FUNCTION})\({count[0]}\)", total[1])[1]
    byte_name = re.fullmatch(rf"({FUNCTION})\({total[0]}\)", byte[1])[1]
    assert copied[1] == byte[0] and product_name != byte_name
    prod = function(code, product_name, "size_t", rf"size_t (?P<formal>{LOCAL})")
    narrow = function(code, byte_name, "uint8_t", rf"size_t (?P<formal>{LOCAL})")
    product_choice, product_if = branches(prod["body"], prod["formal"], 1024)
    assert product_choice["yes"] == (
        f"        const size_t nl_product_left = {prod['formal']};\n"
        "        const size_t nl_product_right = ((size_t)UINTMAX_C(64));\n"
        f"        {product_if} = nl_product_left * nl_product_right;\n"
    )
    assert product_choice["no"].strip() == f"{product_if} = ((size_t)UINTMAX_C(0));"
    byte_choice, byte_if = branches(narrow["body"], narrow["formal"], 256)
    assert byte_choice["no"].strip() == f"{byte_if} = ((uint8_t)UINT8_C(173));"
    yes = byte_choice["yes"]
    cast = f"((uint8_t){narrow['formal']})"
    carrier = cast
    declarations = re.findall(rf"^        uint8_t ({LOCAL}) = (.+);$", yes, re.M)
    assert len(declarations) == copies
    for name, initializer in declarations:
        assert initializer == carrier
        carrier = name
    assert yes.count(cast) == 1 and yes.rstrip().endswith(f"{byte_if} = {carrier};")
    assert narrow["body"].count(cast) == 1
    assert f"return {product_if};" in prod["body"] and f"return {byte_if};" in narrow["body"]

    def line(label, values, indent):
        formats = ",".join("%ju" for _ in values)
        args = ", ".join(f"(uintmax_t){name}" for name in values)
        return f'{indent}printf("{label},{formats}\\n", {args});\n'

    emitted = [
        (prod.start("body") + product_choice.end("yes"),
         line("product", (prod["formal"], product_if), "        ")),
        (narrow.start("body") + byte_choice.end("yes"),
         line("byte,cast", (narrow["formal"], byte_if), "        ")),
        (narrow.start("body") + byte_choice.end("no"),
         line("byte,fallback", (narrow["formal"], byte_if), "        ")),
        (caller.start("body") + caller["body"].rindex("    return;"),
         line("caller", (count[0], total[0], byte[0], copied[0]), "    ")),
    ]
    for position, insertion in sorted(emitted, reverse=True):
        code = code[:position] + insertion + code[position:]
    records = key + 2
    total = records * 64
    edge, field = ("cast", total) if total <= 255 else ("fallback", 173)
    expected = (f"product,{records},{total}\nbyte,{edge},{total},{field}\n"
                f"caller,{records},{total},{field},{field}\n").encode("ascii")
    return b"#include <stdio.h>\n" + code.encode("ascii"), expected


def native(work, cc, sanitizer, file, expected):
    flags = [cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror"]
    if sanitizer != "none":
        flags += [f"-fsanitize={sanitizer}", "-fno-sanitize-recover=all"]
        if "clang" in Path(cc).name:
            flags.append("-Wno-error=gcc-install-dir-libstdcxx")
    for opt in ("-O0", "-O2"):
        exe = work / (file.stem + opt)
        run(work, exe.name + ".build", [*flags, opt, file, "-o", exe], 0)
        result = run(work, exe.name + ".run", [exe], 0)
        assert result.stdout == expected and not result.stderr
        exe.unlink()


def positive(work, cli, cc, sanitizer, key, copies):
    fixture, raw = work / "input.nl", work / "raw.c"
    fixture.write_text(source(key, ARMS[copies]))
    check = run(work, "check", [cli, "--check", fixture], 0)
    emission = run(work, "emit", [cli, "--emit-c17-to", raw, fixture], 0)
    assert not (check.stdout or check.stderr or emission.stdout or emission.stderr)
    native(work, cc, sanitizer, raw, b"")
    watched = work / "observed.c"
    data, expected = observer(raw.read_bytes(), key, copies)
    watched.write_bytes(data)
    native(work, cc, sanitizer, watched, expected)
    print(work.name, expected.decode().strip().replace("\n", " -> "))


def refusal(work, cli, text):
    fixture = work / "input.nl"
    fixture.write_text(text)
    diagnostic = b"[NUMERIC-CONVERSION-PRECISION]"
    check = run(work, "check", [cli, "--check", fixture], 3)
    assert not check.stdout and diagnostic in check.stderr

    outputs = work / "named"
    outputs.mkdir()
    existing, absent = outputs / "existing.c", outputs / "absent.c"
    sentinel = b"/* prior complete C17 output */\n"
    existing.write_bytes(sentinel)
    before = state(existing)
    for target in (existing, absent):
        result = run(work, target.stem, [cli, "--emit-c17-to", target, fixture], 3)
        assert not result.stdout and diagnostic in result.stderr
    assert existing.read_bytes() == sentinel and state(existing) == before
    assert list(outputs.iterdir()) == [existing]
    print(work.name, diagnostic.decode(), "output preserved")


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("compiler")
    parser.add_argument("cc")
    parser.add_argument("sanitizer")
    parser.add_argument("--raw", type=Path)
    args = parser.parse_args()
    if args.raw:
        args.raw.mkdir()
    with nullcontext(args.raw) if args.raw else TemporaryDirectory(prefix="p1957-byte-") as root:
        root = Path(root)
        for key in (1, 2):
            for copies, shape in enumerate(("direct", "bound", "copied")):
                work = root / f"{shape}-{key}"
                work.mkdir()
                positive(work, args.compiler, args.cc, args.sanitizer, key, copies)
        for name, text in (
            ("insufficient-bound", source(2, ARMS[0], bound=257)),
            ("false-arm", source(2, ARMS[0], fallback="u8(total)")),
        ):
            work = root / name
            work.mkdir()
            refusal(work, args.compiler, text)


if __name__ == "__main__":
    main()
