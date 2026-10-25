"""R919 frozen NEW corpus: P905/P918 ordinary observer-OFF byte parity.

This is executed only from GitHub Actions CTest, never on a P-equivalent
local runtime. First source/oracle SHA256 were frozen in Issue #919.
"""
import hashlib
import pathlib
import subprocess
import sys
import tempfile

OLD = "6e8be7c313d19b41ca2d45ae608b4dabb01b22e7"
NEW = "58f54c155d2b9515ad4f277c96186bf73e599290"
ORIGIN = "https://github.com/wakairo/NewLang_Compiler.git"
HASHES = {
    "harbor_true.nl": "8c48b4bdb375febfd743a37101c09543ba497d65624247e692d3d91500e1bd32",
    "orchard_false.nl": "f3d6a4458e5b86263c3b77213e719f3c354f9a11cb7902bc496b5f8f18d537e2",
    "harbor_return_incoming.nl": "d0692d90b82b12061d616b0722b445d23c1aa942c6b4197c342671be5b6a7d83",
    "harbor_return_literal_old.nl": "9189ac073fcde5bd1012364adb0bfe2cf79c97fcf190a66b5600696085da5e8d",
    "orchard_return_incoming.nl": "fb4a42ef0f4ef83cf510cf83e2031cb579371d7044461d0125174ff2eec0db6f",
    "orchard_return_literal_old.nl": "10734e4bdacad5c0fb5c694285b8fd42e4bd4948e619fab4a21cab144094fb7a",
    "harbor_no_call.nl": "550810e5dc32bb79121b72085b1505ba65d81beb40db1158fae4eeb3ae2b5e7f",
    "orchard_duplicate_call.nl": "93a7e5eb665858e3a28a32c213bc1d1eeceedc026fde3cd2f406d44a7b0040be",
}


def run_checked(command, cwd=None):
    return subprocess.run(command, cwd=cwd, check=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          timeout=160)


def compile_exact(ref, root):
    checkout = root / ("checkout-" + ref[:12])
    checkout.mkdir()
    run_checked(["git", "init", "-q"], checkout)
    run_checked(["git", "fetch", "--quiet", "--depth", "1", ORIGIN, ref], checkout)
    run_checked(["git", "checkout", "--detach", "-q", "FETCH_HEAD"], checkout)
    sha = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=checkout, text=True).strip()
    if sha != ref:
        raise RuntimeError("STOP wrong immutable checkout " + sha)
    build = root / ("build-" + ref[:12])
    run_checked(["cmake", "-S", str(checkout), "-B", str(build),
                 "-DNEWLANG_FRONTEND_ONLY=ON", "-DBUILD_TESTING=OFF",
                 "-DCMAKE_BUILD_TYPE=Debug"])
    run_checked(["cmake", "--build", str(build), "--target", "newlangc",
                 "--parallel", "2"])
    binary = build / "newlangc"
    if not binary.is_file():
        raise RuntimeError("STOP no exact newlangc binary")
    return binary


def observe(binary, source, mode, temporary, positive):
    sentinel = temporary / "r919-original-sentinel.c"
    sentinel.write_bytes(b"R919_SOURCE_OWNED_SENTINEL\n")
    before = {p.name for p in temporary.iterdir()}
    if mode == "check":
        command = [str(binary), "--check", str(source)]
    elif mode == "c17":
        command = [str(binary), str(source)]
    elif mode == "named":
        command = [str(binary), "--emit-c17-to", str(sentinel), str(source)]
    else:
        raise RuntimeError("STOP unsupported CLI mode")
    result = subprocess.run(command, cwd=temporary, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, timeout=25)
    after = {p.name for p in temporary.iterdir()}
    if sentinel.read_bytes() != b"R919_SOURCE_OWNED_SENTINEL\n" or after != before:
        raise RuntimeError("STOP output sentinel or temporary file modified")
    if result.returncode == 0 or result.stdout:
        raise RuntimeError(f"STOP unexpected admission {source.name}/{mode}: "
                           f"exit={result.returncode} stdout={result.stdout!r}")
    if positive and (result.returncode != 3 or
                     b"P6-JOIN-PRECISION" not in result.stderr):
        raise RuntimeError(f"STOP CHECK3 first diagnostic mismatch "
                           f"{source.name}/{mode}: {result.stderr!r}")
    return result.returncode, result.stdout, result.stderr


def main():
    root = pathlib.Path(sys.argv[1]).resolve()
    for name, expected in HASHES.items():
        payload = (root / name).read_bytes()
        actual = hashlib.sha256(payload).hexdigest()
        if actual != expected or not payload.endswith(b"\n") or b"\r" in payload:
            raise RuntimeError(f"STOP independent source freeze mismatch "
                               f"{name}: {actual}")
    with tempfile.TemporaryDirectory(prefix="r919-ci-only-") as tmp:
        directory = pathlib.Path(tmp)
        old = compile_exact(OLD, directory)
        new = compile_exact(NEW, directory)
        for name in HASHES:
            source = root / name
            positive = name in ("harbor_true.nl", "orchard_false.nl")
            for mode in ("check", "c17", "named"):
                left = observe(old, source, mode, directory, positive)
                right = observe(new, source, mode, directory, positive)
                if left != right:
                    raise RuntimeError(
                        f"STOP byte-incompatible ordinary CLI {name}/{mode}: "
                        f"P905={left!r} P918={right!r}")
                print(f"R919 byte exact {name}/{mode} exit={left[0]} "
                      f"stderr_sha256={hashlib.sha256(left[2]).hexdigest()}",
                      flush=True)
    print("R919 8 independent source x 3 mode CLI parity PASS", flush=True)


if __name__ == "__main__":
    main()
