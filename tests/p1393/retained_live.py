"""Exercise the ordinary checker and output boundary of the selected source."""

import argparse
from hashlib import sha256
from pathlib import Path
import re
import subprocess
import tempfile
from textwrap import dedent, indent


def replace_one(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def insert_before(text, marker, block):
    line = re.search(r"^([ ]*)" + re.escape(marker), text, re.M)
    assert line is not None, marker
    return text[:line.start()] + indent(block, line[1]) + text[line.start():]


def negative_sources(source):
    dst_cleanup = "let empty_dst = loan_exclusive_read(life_dst)"
    match = "match cached_tail_while_C_live {"
    start = source.index(dst_cleanup)
    c_cleanup = re.search(
        r"^ +let empty_C = loan_exclusive_read\(life_C\).*?"
        r"^ +deallocate\(allocation_C, full_C\);\n",
        source[start:], re.M | re.S,
    )[0]
    without_c = source[:start] + replace_one(source[start:], c_cleanup, "")
    yield (
        "wrong-domain", "ALLOCATED-DOMAIN-MISMATCH",
        replace_one(source, "loan_read(life_B) { |stable_B_cached|",
                    "loan_read(life_C) { |stable_B_cached|"),
    )
    yield (
        "substituted-tail", "CAPTURED-CLOSURE-PRECISION",
        replace_one(source, "read, actual_tail_ptr, stable_B_cached",
                    "read, ptr_B, stable_B_cached"),
    )
    yield "missing-C-cleanup", "ALLOCATED-CAPTURED-JOIN-PRECISION", without_c
    yield (
        "C-ended-between-read-and-match", "CAPTURED-CLOSURE-PRECISION",
        insert_before(without_c, match, dedent(c_cleanup)),
    )
    yield (
        "C-ended-before-dst-cleanup", "CAPTURED-CLOSURE-PRECISION",
        insert_before(without_c, dst_cleanup, dedent(c_cleanup)),
    )


def shared_writer_negatives(source):
    dst_cleanup = "let empty_dst = loan_exclusive_read(life_dst)"
    # These source-valid initial links are not among the nine recorded
    # Changes, but the selected five-root continuation requires them empty.
    for root, field, target in (
        ("A", "child", "ptr_src"),
        ("B", "child", "ptr_A"),
        ("C", "next", "ptr_B"),
        ("C", "child", "ptr_B"),
        ("dst", "next", "ptr_C"),
        ("dst", "prev", "ptr_C"),
    ):
        start = source.index(f"initialize(vacant_{root}, Node {{")
        end = source.index(f"}}, stable_{root})", start)
        initial = source[start:end]
        changed = replace_one(
            initial, f"{field}: Option<ptr<Node>>::None",
            f"{field}: Option<ptr<Node>>::Some({target})")
        yield (
            f"{root}-{field}-nonempty", "CAPTURED-CLOSURE-PRECISION",
            source[:start] + changed + source[end:],
        )
    start = source.index(dst_cleanup)
    c_cleanup = re.search(
        r"^ +let empty_C = loan_exclusive_read\(life_C\).*?"
        r"^ +deallocate\(allocation_C, full_C\);\n",
        source[start:], re.M | re.S,
    )[0]
    without_c = source[:start] + replace_one(source[start:], c_cleanup, "")
    yield (
        "shared-C-ended-before-read", "CAPTURED-CLOSURE-PRECISION",
        insert_before(without_c, "let cached_tail_while_C_live =",
                      dedent(c_cleanup)),
    )
    start = source.index("let ptr_dst =")
    end = source.index("// Pre-detach checkpoint", start)
    initial = source[start:end]
    yield (
        "dst-child-nonempty", "CAPTURED-CLOSURE-PRECISION",
        source[:start] + replace_one(
            initial, "child: Option<ptr<Node>>::None",
            "child: Option<ptr<Node>>::Some(ptr_C)") + source[end:],
    )
    for site, original, first, second, name in (
        (3, "B", "A", "src", "failure-3-order-swapped"),
        (4, "C", "B", "A", "failure-4-order-swapped"),
        (5, "dst", "C", "B", "failure-order-swapped"),
    ):
        start = source.index(
            "None => {",
            source.index(f"independent allocation site {site}: {original}"))
        end = source.index(f"Some(bundle_{original}) => {{", start)
        failure = source[start:end]

        def cleanup_range(binding):
            begin = failure.rfind(
                "\n", 0, failure.index(f"let empty_{binding} =")) + 1
            last = failure.index(
                f"deallocate(allocation_{binding}, full_{binding});")
            return begin, failure.index("\n", last) + 1

        first_start, first_end = cleanup_range(first)
        second_start, second_end = cleanup_range(second)
        assert first_end == second_start
        yield (
            name, "CAPTURED-CLOSURE-PRECISION",
            source[:start] + failure[:first_start] +
            failure[second_start:second_end] +
            failure[first_start:first_end] + failure[second_end:] +
            source[end:],
        )
    yield (
        "different-field", "FIVE-ROOT-WRITER-DEFINITION",
        replace_one(source, "replace(head@prev,", "replace(head@next,"),
    )
    yield (
        "different-installed-original", "CAPTURED-CLOSURE-PRECISION",
        replace_one(source, "replace_cached_tail(w_A_prev, ptr_C)",
                    "replace_cached_tail(w_A_prev, ptr_B)"),
    )
    yield (
        "one-call", "CAPTURED-CLOSURE-PRECISION",
        replace_one(source, "replace_cached_tail(w_A_prev, ptr_C)",
                    "replace(w_A_prev@prev, Option<ptr<Node>>::Some(ptr_C))"),
    )
    yield (
        "uncalled-helper", "CAPTURED-CLOSURE-PRECISION",
        replace_one(
            replace_one(source, "replace_cached_tail(w_A_prev, ptr_C)",
                        "replace(w_A_prev@prev, Option<ptr<Node>>::Some(ptr_C))"),
            "replace_cached_tail(w_A_unlink, ptr_B)",
            "replace(w_A_unlink@prev, Option<ptr<Node>>::Some(ptr_B))",
        ),
    )


def run(root, label, command, status, diagnostic=None):
    result = subprocess.run(command, capture_output=True, check=False)
    for stream in ("stdout", "stderr"):
        (root / f"{label}.{stream}").write_bytes(getattr(result, stream))
    (root / f"{label}.status").write_text(f"{result.returncode}\n")
    assert result.returncode == status, (label, result.returncode, result.stderr)
    if diagnostic is None:
        assert not result.stderr, (label, result.stderr)
    else:
        assert f"[{diagnostic}]".encode() in result.stderr, (label, result.stderr)
        assert not result.stdout, label


def identity(path):
    st = path.stat()
    return st.st_dev, st.st_ino, st.st_mode, st.st_size, st.st_mtime_ns


def exercise(root, compiler, owned, target, shared_writer):
    frozen = target.read_bytes()
    assert sha256(frozen).hexdigest() == (
        "1eab0d6c41d9066e907538ad6d42884eddd52360d130c5237dfb36b01016b255"
        if shared_writer else
        "294bcf1bbb75e76c46768484826d2873480d478e139a12df73f28b4cdc4da879"
    )
    run(root, "retained-live.check", [compiler, "--check", target], 0)
    source_text = frozen.decode()
    cases = list(negative_sources(source_text))
    if shared_writer:
        run(root, "shared-writer.owned", [owned, target, "--shared-writer"], 0)
        renamed = root / "renamed-helper.nl"
        renamed.write_text(source_text.replace("replace_cached_tail", "update_cache")
                           .replace("(head: ref<write, Node>, tail: ptr<Node>)",
                                    "(entry: ref<write, Node>, new_tail: ptr<Node>)")
                           .replace("replace(head@prev, Option<ptr<Node>>::Some(tail))",
                                    "replace(entry@prev, Option<ptr<Node>>::Some(new_tail))"))
        run(root, "renamed-helper.check", [compiler, "--check", renamed], 0)
        run(root, "renamed-helper.owned", [owned, renamed, "--shared-writer"], 0)
        cases.extend(shared_writer_negatives(source_text))
    else:
        run(root, "retained-live.owned", [owned, "owned-live", target], 0)
    output_dir = root / "named-output"
    output_dir.mkdir()
    output = output_dir / "out.c"
    sentinel = b"/* pre-existing output */\n"
    for name, diagnostic, contents in cases:
        source = root / f"{name}.nl"
        source.write_bytes(contents.encode())
        run(root, f"{name}.check", [compiler, "--check", source], 3, diagnostic)
        output.write_bytes(sentinel)
        before = identity(output)
        command = [compiler, "--emit-c17-to", output, source]
        run(root, f"{name}.emit-existing", command, 3, diagnostic)
        assert output.read_bytes() == sentinel and identity(output) == before
        assert set(output_dir.iterdir()) == {output}
        output.unlink()
        run(root, f"{name}.emit-absent", command, 3, diagnostic)
        assert not list(output_dir.iterdir())
        print(name, diagnostic, "existing and absent named outputs preserved")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("compiler", type=Path)
    parser.add_argument("owned", type=Path)
    parser.add_argument("target", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--shared-writer", action="store_true")
    args = parser.parse_args()
    if args.output is not None:
        args.output.mkdir()
        exercise(args.output, args.compiler, args.owned, args.target, args.shared_writer)
    else:
        with tempfile.TemporaryDirectory(prefix="p1393-retained-live-") as temp:
            exercise(Path(temp), args.compiler, args.owned, args.target, args.shared_writer)


if __name__ == "__main__":
    main()
