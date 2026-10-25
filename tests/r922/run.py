"""R922 independently frozen V2 oracle and fixed-sha CLI byte parity.
Execute only on GitHub Actions. No producer fixture or oracle is imported.
"""
import hashlib
import pathlib
import subprocess
import sys
import tempfile

CANDIDATE = "02d3d32e96a834f8918569a2c4e1f282d6476965"
BASELINE = "58f54c155d2b9515ad4f277c96186bf73e599290"
REPOSITORY = "https://github.com/wakairo/NewLang_Compiler.git"
ORACLE_SHA = "6de9176ff35576c723ff0ca727418dd7da1b2332e441373b1124640a3bad90cd"
POSITIVE = {
    "r922_v2_meridian.nl": "36879c1c4f80635a1248eff30505046aebed888276740257d6c5deb1d17e91c6",
    "r922_v2_glacier.nl": "a879ddffb819221f347ba77a5a47ed90f33f630ce2fc56bb93526bea1b332b72",
}
MUTATIONS = {
    "A-BAD-label": ("r922_v2_meridian.nl", "loan_write(outer@pin)", "loan_write(outer@absent)", "8ca14212a3e623b66cac23b700ee9fd2184de241b1763c3758ec46ca20df0e05"),
    "A-NO-known-CALL": ("r922_v2_meridian.nl", "lift_meridian(handle, u8(63))", "replace(handle, u8(63))", "3b5a1e4ecd249e9cebdcecf93385ae97c770352253e6fd61754ea5e4ed0561f3"),
    "A-RETURN-literal-old": ("r922_v2_meridian.nl", "return displaced;", "return u8(26);", "5f21156a2348319008d98bfa4cfcd869e006dd3b76bdf56c80ba26136d6dcecf"),
    "A-RETURN-new": ("r922_v2_meridian.nl", "return displaced;", "return fresh;", "d5c10bca711f20e0d2176cdf56174ea8417b3c76ef670fba465cc0d200944d91"),
    "A-WRONG-field": ("r922_v2_meridian.nl", "loan_write(outer@pin)", "loan_write(outer@reserve)", "2a33178d27ab84855df4c1a8c30e4782dfc012c1f325bce444a68dbd313777ba"),
    "B-BAD-label": ("r922_v2_glacier.nl", "loan_write(summer@spark)", "loan_write(summer@missing)", "899dfc838abbf7bb9f0c80a9322af7edb2f969e9dfcae45c3c15af4d9bfd7dbd"),
    "B-NO-known-CALL": ("r922_v2_glacier.nl", "reset_glacier(gate, u8(142))", "replace(gate, u8(142))", "faa66a5684de4c108e03d18da612210271118a01fb691a5f55b470a3bdb4db43"),
    "B-RETURN-literal-old": ("r922_v2_glacier.nl", "return earlier;", "return u8(75);", "9800c13ded4e00932651605d81bee083a7be894605d072eed4969a85d604fdce"),
    "B-RETURN-new": ("r922_v2_glacier.nl", "return earlier;", "return replacement;", "50a61734a7822de189825b40cdd5a4c3db4239dcd2ef7b94c29f20d1986c47f9"),
    "B-WRONG-field": ("r922_v2_glacier.nl", "loan_write(summer@spark)", "loan_write(summer@quiet)", "fc9f24c6b0bea7a5a5042e0d0906cd5f7800468a98c2cb3560baa8d5f9f58d73"),
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def run(args, cwd=None, timeout=150):
    return subprocess.run(args, cwd=cwd, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, timeout=timeout)


def must(args, cwd=None, timeout=150):
    result = run(args, cwd, timeout)
    if result.returncode:
        raise RuntimeError(
            f"STOP command {args} exit={result.returncode} stderr={result.stderr[-2048:]!r}"
        )
    return result


def fixed_source(directory):
    sources = {}
    for name, expected in POSITIVE.items():
        path = directory / name
        raw = path.read_bytes()
        if sha(raw) != expected or not raw.endswith(b"\n") or b"\r" in raw:
            raise RuntimeError(f"STOP frozen positive SHA {name}")
        sources[name] = raw
        print(f"R922 SOURCE {name} bytes={len(raw)} sha256={expected}", flush=True)
    content = (directory / "r922_v2_oracle.json").read_bytes()
    if sha(content) != ORACLE_SHA:
        raise RuntimeError("STOP frozen canonical oracle SHA")
    return sources


def mutate(sources, work):
    files = []
    for label, (base, old, new, expected) in MUTATIONS.items():
        old = old.encode("ascii")
        new = new.encode("ascii")
        if old not in sources[base]:
            raise RuntimeError(f"STOP missing deterministic mutation {label}")
        raw = sources[base].replace(old, new, 1)
        if sha(raw) != expected:
            raise RuntimeError(f"STOP mutated source hash {label}: {sha(raw)}")
        target = work / (label + ".nl")
        target.write_bytes(raw)
        files.append(target)
        print(f"R922 NEGATIVE {label} sha256={expected}", flush=True)
    return files


def build_exact(sha_ref, work):
    source = work / ("source-" + sha_ref[:12])
    source.mkdir()
    must(["git", "init", "-q"], source)
    must(["git", "fetch", "-q", "--depth", "1", REPOSITORY, sha_ref], source)
    must(["git", "checkout", "-q", "--detach", "FETCH_HEAD"], source)
    actual = must(["git", "rev-parse", "HEAD"], source).stdout.decode().strip()
    if actual != sha_ref:
        raise RuntimeError(f"STOP fixed checkout mismatch {actual}")
    binary_dir = work / ("build-" + sha_ref[:12])
    must(["cmake", "-S", str(source), "-B", str(binary_dir),
          "-DNEWLANG_FRONTEND_ONLY=ON", "-DBUILD_TESTING=OFF",
          "-DCMAKE_BUILD_TYPE=Debug"])
    must(["cmake", "--build", str(binary_dir), "--target", "newlangc",
          "--parallel", "2"])
    path = binary_dir / "newlangc"
    if not path.is_file():
        raise RuntimeError(f"STOP fixed binary missing {sha_ref}")
    return path


def sample(binary, source, mode, work):
    sentinel = work / "named-sentinel.c"
    sentinel.write_bytes(b"R922_V2_UNCHANGED_SENTINEL\n")
    before = {p.name for p in work.iterdir()}
    if mode == "default":
        args = [str(binary), str(source)]
    elif mode == "check":
        args = [str(binary), "--check", str(source)]
    else:
        args = [str(binary), "--emit-c17-to", str(sentinel), str(source)]
    result = run(args, work, 30)
    after = {p.name for p in work.iterdir()}
    if sentinel.read_bytes() != b"R922_V2_UNCHANGED_SENTINEL\n" or before != after:
        raise RuntimeError("STOP named output/sentinel/temp side effect")
    return result.returncode, result.stdout, result.stderr, sentinel.read_bytes()


def main():
    mode, folder = sys.argv[1], pathlib.Path(sys.argv[2]).resolve()
    with tempfile.TemporaryDirectory(prefix="r922-only-github-actions-") as temp:
        work = pathlib.Path(temp)
        sources = fixed_source(folder)
        negatives = mutate(sources, work)
        positives = [folder / name for name in POSITIVE]
        if mode == "negative":
            command = [sys.argv[3], "negative", *map(str, positives),
                       *map(str, negatives)]
            probe = run(command, work, 90)
            sys.stdout.write(probe.stdout.decode(errors="replace"))
            sys.stderr.write(probe.stderr.decode(errors="replace"))
            if probe.returncode:
                raise RuntimeError(f"STOP private negative suite exit={probe.returncode}")
        elif mode == "cli":
            old, new = build_exact(BASELINE, work), build_exact(CANDIDATE, work)
            for source in positives + negatives:
                for command_mode in ("default", "check", "named"):
                    x = sample(old, source, command_mode, work)
                    y = sample(new, source, command_mode, work)
                    if x != y:
                        raise RuntimeError(f"STOP P918/P921 mismatch {source.name}/{command_mode}: {x!r} != {y!r}")
                    if source in positives and (
                        x[0] != 3 or x[1] != b"" or b"P6-JOIN-PRECISION" not in x[2]
                    ):
                        raise RuntimeError(f"STOP frozen V2 CLI prediction {source.name}/{command_mode}: {x!r}")
                    print(f"R922 CLI BYTE-EXACT {source.name} {command_mode}"
                          f" exit={x[0]} stdout_sha={sha(x[1])}"
                          f" stderr_sha={sha(x[2])} file_sha={sha(x[3])}", flush=True)
        else:
            raise RuntimeError(f"STOP unknown mode {mode}")


if __name__ == "__main__":
    main()
