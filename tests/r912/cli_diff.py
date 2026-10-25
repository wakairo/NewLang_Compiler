"""R912 immutable P897 vs P905 ordinary observer-OFF CLI byte differential.

Only launched from R-owned GitHub Actions CTest. Do not run locally.
"""
import hashlib
import pathlib
import subprocess
import sys
import tempfile

OLD = "b65c39fac1cf56e276c03a53fff802fb0c243957"
NEW = "6e8be7c313d19b41ca2d45ae608b4dabb01b22e7"
URL = "https://github.com/wakairo/NewLang_Compiler.git"
SOURCES = (
    "r907_quartz.nl",
    "r907_tide.nl",
    "r907_return_incoming.nl",
    "r907_return_literal_old.nl",
)


def checked(command, cwd=None):
    subprocess.run(command, cwd=cwd, check=True, stdout=subprocess.PIPE,
                   stderr=subprocess.PIPE, timeout=130)


def build_fixed(ref, root):
    checkout = root / ref[:12]
    checkout.mkdir()
    checked(["git", "init", "-q"], checkout)
    checked(["git", "fetch", "--quiet", "--depth", "1", URL, ref], checkout)
    checked(["git", "checkout", "-q", "--detach", "FETCH_HEAD"], checkout)
    sha = subprocess.check_output(["git", "rev-parse", "HEAD"],
                                  cwd=checkout, text=True).strip()
    if sha != ref:
        raise RuntimeError(f"STOP checkout changed: {sha} != {ref}")
    build = root / ("build-" + ref[:12])
    checked(["cmake", "-S", str(checkout), "-B", str(build),
             "-DNEWLANG_FRONTEND_ONLY=ON", "-DBUILD_TESTING=OFF",
             "-DCMAKE_BUILD_TYPE=Debug"])
    checked(["cmake", "--build", str(build), "--target", "newlangc",
             "--parallel", "2"])
    binary = build / "newlangc"
    if not binary.is_file():
        raise RuntimeError("STOP missing exact fixed newlangc")
    return binary


def sample(binary, source, mode, work):
    sentinel = work / "named-sentinel.c"
    sentinel.write_bytes(b"R912_IMMUTABLE_SENTINEL\n")
    before = {p.name for p in work.iterdir()}
    if mode == "named":
        command = [str(binary), "--emit-c17-to", str(sentinel), str(source)]
    elif mode == "c17":
        command = [str(binary), str(source)]
    else:
        command = [str(binary), "--check", str(source)]
    result = subprocess.run(command, cwd=work, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, timeout=20)
    after = {p.name for p in work.iterdir()}
    canary = sentinel.read_bytes()
    if canary != b"R912_IMMUTABLE_SENTINEL\n" or after != before:
        raise RuntimeError("STOP named sentinel or temporary file changed")
    if result.returncode != 3 or result.stdout or \
            b"P6-JOIN-PRECISION" not in result.stderr:
        raise RuntimeError(
            f"STOP first CLI diagnostic {source.name}/{mode}: "
            f"rc={result.returncode} out={result.stdout!r} "
            f"err={result.stderr!r}"
        )
    return result.returncode, result.stdout, result.stderr, canary


def main():
    sources = pathlib.Path(sys.argv[1])
    with tempfile.TemporaryDirectory(prefix="r912-github-actions-") as base:
        work = pathlib.Path(base)
        old = build_fixed(OLD, work)
        new = build_fixed(NEW, work)
        for name in SOURCES:
            source = sources / name
            payload = source.read_bytes()
            print(f"R912 SRC {name} sha256={hashlib.sha256(payload).hexdigest()}",
                  flush=True)
            for mode in ("check", "c17", "named"):
                old_sample = sample(old, source, mode, work)
                new_sample = sample(new, source, mode, work)
                if old_sample != new_sample:
                    raise RuntimeError(
                        f"STOP non-identical P897/P905 {name}/{mode}: "
                        f"old={old_sample!r} new={new_sample!r}"
                    )
                digest = hashlib.sha256(old_sample[2]).hexdigest()
                print(f"R912 BYTE-EXACT PASS old={OLD} new={NEW} "
                      f"file={name} mode={mode} "
                      f"rc={old_sample[0]} stderr_sha256={digest}", flush=True)


if __name__ == "__main__":
    main()
