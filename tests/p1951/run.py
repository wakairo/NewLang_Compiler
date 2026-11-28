"""Observe a genuine selected-byte suffix after two widening operands."""
from argparse import ArgumentParser
from contextlib import nullcontext
from pathlib import Path
import re
import runpy
import sys
from tempfile import TemporaryDirectory

sys.dont_write_bytecode = True
from fixtures import compositions, controls, negatives


support = Path(__file__).resolve().parents[1] / "p1940"
sys.path.insert(0, str(support))
try:
    tests = runpy.run_path(str(support / "run.py"))
finally:
    sys.path.pop(0)
caller, observe, prediction, trace, before_return, run, state = (
    tests[name] for name in ("caller", "observe", "prediction", "trace", "before_return", "run", "state")
)
LOCAL, FUNCTION = tests["LOCAL"], tests["FUNCTION"]


def verify_prefix(data, source, oracle):
    body, names = caller(data, source)
    code = body[1]
    seen = []
    for side in oracle["evaluation_order"]:
        i = int(side == "right")
        key = side.encode()
        kind = oracle["producer_pair"][i]
        wide = names[key + b"_wide"]
        source_name = b"left_byte" if oracle["shared"] else key + b"_byte"
        octet = names[source_name]
        assert wide[1] == b"size_t" and octet[1] == b"uint8_t"
        assert names[key + b"_value"][3] == wide[2]
        assert names[key + b"_source"][3] == names[key + b"_byte"][2]
        if kind == "r":
            assert re.fullmatch(rb"nl_call_\d+", wide[3])
            staged = re.search(rb"^    const size_t " + wide[3] + rb" = (" +
                               FUNCTION + rb")\(" + octet[2] + rb"\);$", code, re.M)
            assert staged and staged.end() < wide.start()
            seen.append(staged[1])
        elif kind == "l":
            assert wide[3] == b"(size_t)" + octet[2]
        else:
            value = str(oracle["octets"][i]).encode()
            assert wide[3] == b"((size_t)UINTMAX_C(" + value + b"))"
    one, two = [names[side.encode() + b"_wide"] for side in oracle["evaluation_order"]]
    assert one.start() < two.start() and one[2] != two[2]
    if len(seen) == 2:
        assert seen[0] == seen[1] and one[3] != two[3]
    selected = names[b"chosen"]
    conditional = re.fullmatch(rb"\((.+) \? 61 : 197\)", selected[3])
    assert conditional
    condition = conditional[1]
    if oracle["form"] != "direct":
        decision = names[b"decision"]
        assert decision[1] == b"_Bool" and decision.start() < selected.start()
        named = names[oracle["condition"].encode()]
        assert condition == named[2]
        if oracle["form"] == "copied":
            assert named[3] == decision[2] and decision.start() < named.start()
        condition = decision[3]
    a, b = [names[s.encode() + b"_value"][2] for s in oracle["operand_order"]]
    if oracle["operator"] == "==":
        assert condition == b"(" + a + b" == " + b + b")"
    else:
        ordered = re.fullmatch(rb"\((nl_less_left_(\d+)) < (nl_less_right_\2)\)", condition)
        assert ordered
        left = code.index(b"const size_t " + ordered[1] + b" = " + a + b";")
        right = code.index(b"const size_t " + ordered[3] + b" = " + b + b";")
        assert left < right < code.index(condition)
    return seen[0] if seen else None


def observed(data, source, oracle):
    widener = verify_prefix(data, source, oracle)
    data = observe(data, source, oracle)
    body, names = caller(data, source)
    at = body.start(1) + before_return(body[1])
    data = data[:at] + trace(b"prefix", [names[n.encode()][2] for n in oracle["prefix_names"]]) + data[at:]
    if widener:
        helper = tests["function"](data, b"size_t", widener, rb"uint8_t " + LOCAL)
        (formal,) = re.findall(LOCAL, helper[0].split(b"\n", 1)[0])
        (ret,) = re.finditer(rb"^    return (" + LOCAL + rb");$", helper[1], re.M)
        assert re.search(rb"^    const size_t " + ret[1] + rb" = \(size_t\)" + formal + rb";$",
                         helper[1], re.M)
        at = helper.start(1) + ret.start()
        data = data[:at] + trace(b"widen", [formal, ret[1]]) + data[at:]
    return data


def expected(oracle):
    calls = "".join(f"widen,{n},{n}\n" for n in oracle["widen_calls"]).encode()
    prefix = ("prefix," + ",".join(map(str, oracle["prefix_values"])) + "\n").encode()
    return calls + prediction(oracle) + prefix


def positive(directory, source, oracle, compiler, cc, sanitizer):
    raw, watched = directory / "raw.c", directory / "observed.c"
    checked = run(directory, "check", [compiler, "--check", source], 0)
    emitted = run(directory, "emit", [compiler, "--emit-c17-to", raw, source], 0)
    assert not (checked.stdout or checked.stderr or emitted.stdout or emitted.stderr)
    watched.write_bytes(observed(raw.read_bytes(), source, oracle))
    payload = expected(oracle)
    flags = [cc, "-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror"]
    if sanitizer and sanitizer != "none":
        flags += ["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all"]
        if "clang" in Path(cc).name:
            flags.append("-Wno-error=gcc-install-dir-libstdcxx")
    for opt in ("-O0", "-O2"):
        for cfile, stdout in ((raw, b""), (watched, payload)):
            exe = directory / (cfile.stem + opt)
            run(directory, exe.name + ".build", [*flags, opt, cfile, "-o", exe], 0)
            result = run(directory, exe.name + ".run", [exe], 0)
            assert result.stdout == stdout and not result.stderr
            exe.unlink()
    print(source.stem, "C17 O0/O2", payload.decode().strip().replace("\n", " -> "), flush=True)


def negative(directory, source, status, compiler):
    check, emit, code = status
    diagnostic = f"[{code}]".encode()
    checked = run(directory, "check", [compiler, "--check", source], check)
    assert not checked.stdout and (diagnostic in checked.stderr if check else not checked.stderr)
    named = directory / "named"
    named.mkdir()
    old, absent = named / "existing.c", named / "absent.c"
    sentinel = b"/* complete prior C17 output */\n"
    old.write_bytes(sentinel)
    before = state(old)
    for target in (old, absent):
        result = run(directory, target.stem, [compiler, "--emit-c17-to", target, source], emit)
        assert not result.stdout and diagnostic in result.stderr
    assert old.read_bytes() == sentinel and state(old) == before and list(named.iterdir()) == [old]
    print(source.stem, "CHECK", check, "C17", emit, "output preserved", flush=True)


def suite(args, root):
    accepted = rejected = 0
    for items in (compositions(), controls()):
        for name, text, oracle in items:
            directory = root / name
            directory.mkdir()
            source = directory / (name + ".nl")
            source.write_bytes(text.encode())
            positive(directory, source, oracle, args.compiler, args.cc, args.sanitizer)
            accepted += 1
    for name, text, status in negatives():
        directory = root / name
        directory.mkdir()
        source = directory / (name + ".nl")
        source.write_bytes(text.encode())
        negative(directory, source, status, args.compiler)
        rejected += 1
    assert accepted == 106 and rejected == 17
    print(f"{accepted} positive sources and {rejected} output-preserving refusals")


if __name__ == "__main__":
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("compiler")
    parser.add_argument("cc")
    parser.add_argument("sanitizer")
    parser.add_argument("--raw", type=Path)
    args = parser.parse_args()
    if args.raw is None:
        work = TemporaryDirectory(prefix="p1951-selected-byte-helper-")
    else:
        args.raw.mkdir()
        work = nullcontext(args.raw)
    with work as output:
        suite(args, Path(output))
