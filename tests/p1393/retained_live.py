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


def exercise(root, compiler, owned, target):
    frozen = target.read_bytes()
    assert sha256(frozen).hexdigest() == (
        "294bcf1bbb75e76c46768484826d2873480d478e139a12df73f28b4cdc4da879"
    )
    run(root, "retained-live.check", [compiler, "--check", target], 0)
    run(root, "retained-live.owned", [owned, "owned-live", target], 0)
    output_dir = root / "named-output"
    output_dir.mkdir()
    output = output_dir / "out.c"
    sentinel = b"/* pre-existing output */\n"
    for name, diagnostic, contents in negative_sources(frozen.decode()):
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
    args = parser.parse_args()
    if args.output is not None:
        args.output.mkdir()
        exercise(args.output, args.compiler, args.owned, args.target)
    else:
        with tempfile.TemporaryDirectory(prefix="p1393-retained-live-") as temp:
            exercise(Path(temp), args.compiler, args.owned, args.target)


if __name__ == "__main__":
    main()
