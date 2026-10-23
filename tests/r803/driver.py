"""R803 independent frozen-source oracle: no changes to compiler or guest source."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

DIGESTS = {
    "one.nl": "eb2423e7b9eab273fe21b5eff11c7a808b9fd383e48542a7f7400ad2cc09505d",
    "two.nl": "e1f95235eac4c3fb029251ed8a1cf51ad9b0fef0c0ea2aac09a5a9e987423a61",
    "arity_three.nl": "cb0479b27d9e4d41c96ac0aeb5114bc3f7d9eae5bb0d17d99a87ba233ed4cca2",
    "arity_five.nl": "4b83dc08830a9aa281b8c42ddd61b8445194a1652100ce08f20591b0065a2539",
    "wrong_element.nl": "f39d750b64f0514bbc9026d460d99b82f8754392e9484f5b651404420c35658c",
    "wrong_nominal.nl": "429414b9b7b71822245ea22857fa54d208106f71fed1ce0df01fcc64512075ad",
    "copy_unsupported.nl": "b50b3b2bf674b5679016d6ac1d63a2d6bf1ab6951eb7c46d4603b2c980c85097",
    "bare_option_unsupported.nl": "981f30410d858b80523d75de5deaea0d7ceffeb3d1c14597d625f30542c59d23",
    "return_option_unsupported.nl": "6bbbdaad90c19bc8d3f8e3e4c61213ca17790a90a45f020ef3174b0154312e3e",
    "indirect_leaf_unsupported.nl": "d3bea3f66bff0408e63bc9cf8bab50df6cba9f89ae14c26edb1b224c3cd94dc1",
}


def execute(*args):
    return subprocess.run([str(a) for a in args], capture_output=True, text=True)


def success(*args):
    p = execute(*args)
    assert p.returncode == 0, (args, p.returncode, p.stderr, p.stdout)
    return p


def frozen(folder):
    oracle = (folder / "ORACLE.md").read_text()
    for name, digest in DIGESTS.items():
        path = folder / name
        assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, name
        assert digest + "  tests/r803/" + name in oracle, name


def proof(probe, source, count):
    p = success(probe, source, count)
    data = json.loads(p.stdout)
    assert data["count"] == count and len(data["roots"]) == count, data
    assert data["mutations_rejected"] == 9 * count, data
    assert data["same_number_foreign_rejected"] == count, data
    assert all(x["child_count"] == 4 for x in data["roots"])
    assert len({x["root"] for x in data["roots"]}) == count
    assert len({x["incarnation"] for x in data["roots"]}) == count
    return data


def negatives(compiler, folder):
    with tempfile.TemporaryDirectory(prefix="r803-negative-") as tmp:
        target = Path(tmp) / "preserved.c"
        for name in list(DIGESTS)[2:]:
            source = folder / name
            target.write_bytes(b"R803_GUARD\n")
            before = target.stat()
            for opts in (["--check"], [], ["--emit-c17-to", target]):
                p = execute(compiler, *opts, source)
                assert p.returncode != 0 and not p.stdout, (name, opts, p)
                assert target.read_bytes() == b"R803_GUARD\n"
                assert target.stat().st_ino == before.st_ino
                if name in ("arity_three.nl", "arity_five.nl",
                            "wrong_element.nl"):
                    assert "error(semantic)" in p.stderr, (name, p.stderr)
                elif name in ("copy_unsupported.nl",
                              "bare_option_unsupported.nl",
                              "return_option_unsupported.nl",
                              "indirect_leaf_unsupported.nl"):
                    assert "error(unsupported)" in p.stderr, (name, p.stderr)
                else:
                    assert "error(" in p.stderr, (name, p.stderr)
            assert len(list(Path(tmp).iterdir())) == 1, name
            print("R803 negative", name, "fail-closed", p.returncode)


def oom(parser, issuer, fault, folder):
    with tempfile.TemporaryDirectory(prefix="r803-oom-") as tmp:
        target = Path(tmp) / "preserved.c"
        for name in ("one.nl", "two.nl"):
            source = folder / name
            p = success(parser, source)
            assert re.search(r"OOM positions=([1-9][0-9]*)", p.stdout), p.stdout
            p = success(issuer, "oom", source)
            reg = re.search(r"registration=([1-9][0-9]*)", p.stdout)
            entry = re.search(r"main=([1-9][0-9]*)", p.stdout)
            assert reg and entry and "rollback=PASS" in p.stdout, p.stdout
            p = success(fault, source, target)
            reached = re.search(r"allocation failures=([1-9][0-9]*)", p.stdout)
            assert reached and "recovery=PASS" in p.stdout, p.stdout
            print("R803 reached injected failure", name, reg.group(1),
                  entry.group(1), reached.group(1))


def observe(text, info):
    option = info["element_type"]
    array = info["array_type"]
    roots = info["roots"]
    assert text.count("typedef nl_option_%d nl_array_%d[4];" %
                      (option, array)) == 1
    assert text.count("{ .tag = NL_ARRAY4_NONE }") == 4 * len(roots)
    assert "memset" not in text
    locals_ = ["nl_local_%d" % root["symbol"] for root in roots]
    assert len(set(locals_)) == len(locals_)
    for local in locals_:
        assert text.count("nl_array_%d %s = {" % (array, local)) == 1
    lines = []
    for i, local in enumerate(locals_):
        lines.append(
            "    const uintptr_t r803_base_%d = "
            "(uintptr_t)(const void *)&%s[0];" % (i, local)
        )
    if len(locals_) == 2:
        a, b = locals_
        lines.append(
            "    if (r803_base_0 == r803_base_1 || "
            "(r803_base_0 < r803_base_1 ? "
            "r803_base_1 - r803_base_0 < sizeof(%s) : "
            "r803_base_0 - r803_base_1 < sizeof(%s))) abort();" % (a, b)
        )
    for i, local in enumerate(locals_):
        lines.append(
            "    if (sizeof(%s) != 4 * sizeof(nl_option_%d)) abort();"
            % (local, option)
        )
        lines.append("    for (size_t j = 0; j < 4; ++j) {")
        lines.append(
            "        uintptr_t here = (uintptr_t)(const void *)&%s[j];"
            % local
        )
        lines.append(
            "        if (here - r803_base_%d != "
            "j * sizeof(nl_option_%d) || "
            "%s[j].tag != NL_ARRAY4_NONE) abort();"
            % (i, option, local)
        )
        lines.append(
            '        printf("root=%d element=%%zu base=%%" PRIuPTR '
            '" address=%%" PRIuPTR " stride=%%zu tag=%%u\\n", '
            'j, r803_base_%d, here, sizeof(nl_option_%d), '
            '(unsigned)%s[j].tag);' % (i, i, option, local)
        )
        lines.append("    }")
    mark = "    (void)%s;" % locals_[-1]
    assert text.count(mark) == 1
    prefix = "#include <stdint.h>\n"
    assert text.count(prefix) == 1
    headers = "#include <stddef.h>\n#include <inttypes.h>\n"
    headers += "#include <stdio.h>\n#include <stdlib.h>\n"
    return text.replace(prefix, prefix + headers).replace(
        mark, "\n".join(lines) + "\n" + mark)


def strict_native(compiler, probe, cc, sanitizer, folder):
    with tempfile.TemporaryDirectory(prefix="r803-native-") as tmp:
        root = Path(tmp)
        flags = [] if sanitizer == "none" else [
            "-fsanitize=" + sanitizer, "-fno-sanitize-recover=all"]
        for name, count in (("one.nl", 1), ("two.nl", 2)):
            source = folder / name
            info = proof(probe, source, count)
            check = success(compiler, "--check", source)
            emitted = success(compiler, source)
            assert not check.stdout and not check.stderr and not emitted.stderr
            target = root / "named.c"
            success(compiler, "--emit-c17-to", target, source)
            assert target.read_text() == emitted.stdout
            assert not target.is_symlink()
            derivatives = [("untouched", emitted.stdout, False),
                           ("passive", observe(emitted.stdout, info), True)]
            for label, body, passive in derivatives:
                cfile = root / (name + label + ".c")
                cfile.write_text(body)
                for opt in ("-O0", "-O2"):
                    exe = root / (name + label + opt)
                    success(cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic",
                            "-Werror", opt, *flags, cfile, "-o", exe)
                    p = success(exe)
                    lines = p.stdout.splitlines()
                    assert len(lines) == (4 * count if passive else 0), (
                        name, label, opt, p.stdout)
                    if passive:
                        rows = [dict(v.split("=") for v in line.split())
                                for line in lines]
                        assert [int(v["root"]) for v in rows] == sum(
                            ([i] * 4 for i in range(count)), [])
                        assert [int(v["element"]) for v in rows] == (
                            list(range(4)) * count)
                        assert len({r["base"] for r in rows}) == count
                        for row in rows:
                            assert int(row["stride"]) > 0
                            assert int(row["address"]) - int(row["base"]) == (
                                int(row["element"]) * int(row["stride"]))
                    print("R803", name, label, cc, opt, "PASS")


def main():
    mode, compiler, probe, parser, issuer, fault, cc, sanitizer, root = sys.argv[1:]
    root = Path(root)
    frozen(root)
    if mode == "negative":
        negatives(compiler, root)
    elif mode == "oom":
        oom(parser, issuer, fault, root)
    elif mode == "native":
        strict_native(compiler, probe, cc, sanitizer, root)
    else:
        raise RuntimeError("unknown R803 mode: " + mode)


if __name__ == "__main__":
    main()
