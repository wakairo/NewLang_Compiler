#!/usr/bin/env python3
"""Rootless, checksum-locked binary toolchain bootstrap; never rewrites pins.

The committed lock was derived from signature-verified repository metadata.
TLS and SHA-256 remain required on every download. No package maintainer
scripts run; only the C/LLVM development subset is extracted into .deps.
"""
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import shutil
import subprocess
import time
import urllib.error
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
LOCK = ROOT / "scripts/toolchain-lock.json"


def sha256(path: Path) -> str:
    result = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(chunk)
    return result.hexdigest()


def run(args: list[str], environment: dict[str, str] | None = None) -> str:
    try:
        return subprocess.check_output(args, env=environment, text=True,
                                       stderr=subprocess.STDOUT).strip()
    except subprocess.CalledProcessError as error:
        # Show the already-captured tool diagnostic; never dump environment.
        output = (error.output or "").strip()[:1200]
        raise SystemExit(
            f"Tool failed (exit={error.returncode}): "
            f"{shlex.join(args)}; output={output!r}") from error


def download(package: dict[str, str], path: Path) -> None:
    """Three attempts at the locked URL; only observed HTTP404 is retried."""
    temporary = path.with_suffix(".partial")
    for attempt in range(1, 4):
        try:
            with urllib.request.urlopen(package["url"], timeout=60) as response:
                with temporary.open("wb") as output:
                    shutil.copyfileobj(response, output)
            if sha256(temporary) != package["sha256"]:
                raise SystemExit(f"checksum mismatch: {package['name']}")
            temporary.replace(path)
            return
        except urllib.error.HTTPError as error:
            if error.code != 404 or attempt == 3:
                raise
            error.close()
            print(f"retry {package['name']}: HTTP404 attempt {attempt}/3; "
                  f"wait {attempt}s", flush=True)
            time.sleep(attempt)
        finally:
            temporary.unlink(missing_ok=True)


def _legacy_main() -> None:
    if platform.system() != "Linux" or platform.machine() != "x86_64":
        raise SystemExit("P0 bootstrap supports Linux x86_64 only")
    if shutil.which("dpkg-deb") is None or shutil.which("gcc") is None:
        raise SystemExit("Install dpkg and the host C toolchain first; see README")
    release = platform.freedesktop_os_release()
    target = f"{release['ID']}-{release['VERSION_ID']}-x86_64"
    lock = json.loads(LOCK.read_text())
    legacy_version = lock["platforms"][target].get("llvm_version", lock["llvm_version"])
    llvm_major = legacy_version.split(".")[0]
    if target not in lock["platforms"]:
        raise SystemExit(f"No reviewed toolchain lock for {target}")
    if target == "ubuntu-24.04-x86_64":
        # The tag-matching Trixie SDK uses the locked Z3 4.13.3 runtime on
        # Ubuntu too. Its GLIBCXX_3.4.32 requirement needs the distro's GCC 14
        # runtime, independently of using GCC 13 as the C host compiler.
        runtime = run(["dpkg-query", "-W", "-f=${Version}", "libstdc++6"])
        if subprocess.run(["dpkg", "--compare-versions", runtime, "ge", "14"]).returncode:
            raise SystemExit("Ubuntu requires libstdc++6 >=14; install README prerequisites")
    identity = hashlib.sha256(LOCK.read_bytes()).hexdigest()[:16]
    prefix = ROOT / ".deps/toolchains" / f"{target}-{identity}"
    cache = ROOT / ".deps/downloads"
    prefix.mkdir(parents=True, exist_ok=True)
    cache.mkdir(parents=True, exist_ok=True)
    completed = prefix / ".complete"
    # Dedicated content-addressed prefix; partial extraction is retried from
    # the same verified artifacts. Dependencies retained in the snapshot.
    for package in lock["platforms"][target]["packages"]:
        path = cache / f"{package['name']}-{package['sha256']}.deb"
        if not path.is_file() or sha256(path) != package["sha256"]:
            print(f"download {package['name']} {package['version']}", flush=True)
            download(package, path)
        if not completed.is_file():
            subprocess.run(["dpkg-deb", "-x", str(path), str(prefix)], check=True)

    environment = os.environ.copy()
    bindir = str(prefix / "usr/bin")
    libdir = str(prefix / "usr/lib/x86_64-linux-gnu")
    environment["PATH"] = bindir + os.pathsep + environment["PATH"]
    environment["LD_LIBRARY_PATH"] = libdir + (
        os.pathsep + environment["LD_LIBRARY_PATH"]
        if environment.get("LD_LIBRARY_PATH") else "")
    llvm_config = prefix / f"usr/bin/llvm-config-{llvm_major}"
    llvm = run([str(llvm_config), "--version"], environment)
    if llvm != legacy_version:
        raise SystemExit(f"LLVM version mismatch: {llvm}")
    clang = run([str(prefix / f"usr/bin/clang-{llvm_major}"), "--version"], environment)
    clang_format = run([str(prefix / f"usr/bin/clang-format-{llvm_major}"), "--version"], environment)
    if legacy_version not in clang_format:
        raise SystemExit(f"clang-format version mismatch: {clang_format}")
    cmake = run(["cmake", "--version"], environment)
    gcc = run(["gcc", "-dumpfullversion"], environment)
    print(f"LLVM {llvm}\n{clang.splitlines()[0]}\n{clang_format.splitlines()[0]}\n"
          f"GCC {gcc}\n{cmake.splitlines()[0]}\nPython {platform.python_version()}")

    # Activation is required for shared libraries and compiler executables.
    activate = ROOT / ".deps/activate.sh"
    marker = "# Generated by NewLang P0 bootstrap; no credentials.\n"
    if activate.exists() and not activate.read_text().startswith(marker):
        raise SystemExit("Preserving unfamiliar .deps/activate.sh; move it before bootstrap")
    activate.write_text(marker +
        f"export PATH={shlex.quote(bindir)}:\"$PATH\"\n" +
        f"export LD_LIBRARY_PATH={shlex.quote(libdir)}"
        '"${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\n' +
        f"export LLVM_CONFIG={shlex.quote(str(llvm_config))}\n")
    completed.write_text(identity + "\n")
    print("Bootstrap verified. Run: . .deps/activate.sh")


# Curated, official-upstream-derived LLVM 23.1.3 SDK.  An exact GitHub Release
# asset SHA is the authenticated identity here: unlike the upstream archive,
# the curated archive has no independent GPG signature. Cache is untrusted.
import posixpath
import tarfile
import tempfile


def _sdk_download(artifact: dict, destination: Path) -> None:
    expected_size = artifact["size"]
    fd, temporary_name = tempfile.mkstemp(
        prefix=".download-", dir=destination.parent)
    temporary = Path(temporary_name)
    try:
        with os.fdopen(fd, "wb") as output:
            with urllib.request.urlopen(artifact["url"], timeout=120) as response:
                if not response.geturl().startswith("https://"):
                    raise SystemExit("SDK transfer did not remain HTTPS")
                total = 0
                while True:
                    data = response.read(1024 * 1024)
                    if not data:
                        break
                    total += len(data)
                    if total > expected_size:
                        raise SystemExit("SDK archive over published size")
                    output.write(data)
        if temporary.stat().st_size != expected_size:
            raise SystemExit("SDK downloaded byte count mismatch")
        if sha256(temporary) != artifact["sha256"]:
            raise SystemExit("SDK downloaded SHA256 mismatch")
        temporary.replace(destination)
    finally:
        temporary.unlink(missing_ok=True)


def _sdk_valid_archive(artifact: dict, cache: Path) -> None:
    if cache.is_symlink() or not cache.is_file():
        raise SystemExit("SDK cache entry is not a regular file")
    if cache.stat().st_size != artifact["size"]:
        raise SystemExit("SDK cache size mismatch (fail closed)")
    if sha256(cache) != artifact["sha256"]:
        raise SystemExit("SDK cache SHA256 mismatch (fail closed)")


def _sdk_decode_and_extract(artifact: dict, archive: Path, destination: Path) -> None:
    # Decode only authenticated compressed bytes into a bounded temporary file.
    # Disk is bounded before tar iteration; stderr and exit code are preserved.
    with tempfile.TemporaryFile(dir=destination.parent) as decoded, \
            tempfile.TemporaryFile(dir=destination.parent) as errors:
        started = time.monotonic()
        proc = subprocess.Popen(
            ["zstd", "--long=30", "-dc", "--", str(archive)],
            stdout=subprocess.PIPE, stderr=errors)
        total = 0
        try:
            while True:
                part = proc.stdout.read(1024 * 1024)
                if not part:
                    break
                total += len(part)
                if total > artifact["tar_size"]:
                    proc.kill()
                    raise SystemExit("SDK decompressed tar exceeds pinned limit")
                decoded.write(part)
        finally:
            proc.stdout.close()
            if proc.poll() is None:
                proc.wait(timeout=30)
        errors.seek(0)
        diagnostic = errors.read(2048).decode("utf-8", "replace")
        if proc.returncode != 0 or total != artifact["tar_size"]:
            raise SystemExit(
                f"SDK zstd/tar mismatch exit={proc.returncode} "
                f"bytes={total} stderr={diagnostic[:500]}")
        print(f"SDK authenticated decompression {total} bytes in "
              f"{time.monotonic() - started:.2f}s", flush=True)
        decoded.seek(0)
        with tarfile.open(fileobj=decoded, mode="r:") as contents:
            items = contents.getmembers()
            if not items or len(items) > 15000:
                raise SystemExit("SDK tar member count outside bound")
            regular = 0
            for member in items:
                name = member.name
                stripped = name.removeprefix("./")
                normalized = posixpath.normpath(stripped)
                if (name.startswith("/") or "\x00" in name
                        or ".." in stripped.split("/")
                        or normalized == ".." or normalized.startswith("../")):
                    raise SystemExit("SDK tar unsafe path: " + repr(name[:120]))
                if normalized in ("", "."):
                    if not member.isdir():
                        raise SystemExit("SDK tar unexpected root member")
                    continue
                if member.isfile():
                    regular += member.size
                elif not member.isdir() and not member.issym() and not member.islnk():
                    raise SystemExit("SDK tar special member refused")
                if member.issym() or member.islnk():
                    link = member.linkname
                    if not link or link.startswith("/") or "\x00" in link:
                        raise SystemExit("SDK tar absolute/empty link refused")
                    target = posixpath.normpath(
                        posixpath.join(posixpath.dirname(normalized), link)
                        if member.issym() else link)
                    if target in ("..", ".") or target.startswith("../"):
                        raise SystemExit("SDK tar link escapes extraction root")
            if regular > artifact["tar_size"]:
                raise SystemExit("SDK tar regular-file budget exceeded")
            if not hasattr(tarfile, "data_filter"):
                raise SystemExit("Python tarfile.data_filter required")
            # data_filter independently rejects link-chain and parent symlink
            # escapes, strips privilege metadata, and preserves normal modes.
            contents.extractall(path=destination, filter="data")
        print(f"SDK fail-closed tar inspection and rootless extraction "
              f"{len(items)} members", flush=True)


def _sdk_root(where: Path) -> Path:
    if (where / "bin").is_dir():
        return where
    dirs = list(where.iterdir())
    if len(dirs) == 1 and dirs[0].is_dir() and (dirs[0] / "bin").is_dir():
        return dirs[0]
    raise SystemExit("SDK unexpected install-root topology")


def _sdk_verify_tools(sdk: Path, version: str) -> None:
    bin_dir = sdk / "bin"
    for alias, executable in (
            ("llvm-config-23", "llvm-config"),
            ("clang-23", "clang"),
            ("clang-format-23", "clang-format")):
        path = bin_dir / alias
        actual = bin_dir / executable
        if not actual.is_file() or not os.access(actual, os.X_OK):
            raise SystemExit("SDK required executable missing: " + executable)
        if not path.exists() and not path.is_symlink():
            path.symlink_to(executable)
        if not path.resolve().is_relative_to(sdk.resolve()):
            raise SystemExit("SDK executable alias escapes prefix")
    environment = os.environ.copy()
    environment["PATH"] = str(bin_dir) + os.pathsep + environment["PATH"]
    config = bin_dir / "llvm-config-23"
    if run([str(config), "--version"], environment) != version:
        raise SystemExit("SDK llvm-config wrong version")
    if run([str(config), "--link-static", "--shared-mode",
             "core", "analysis"], environment) != "static":
        raise SystemExit("SDK must be reviewed static-only LLVM configuration")
    for switch in ("--includedir", "--libdir"):
        value = Path(run([str(config), switch], environment)).resolve()
        if not value.is_relative_to(sdk.resolve()):
            raise SystemExit("SDK llvm-config nonrelocatable " + switch)
    lib_dir = Path(run([str(config), "--libdir"], environment))
    if not (lib_dir / "libLLVMCore.a").is_file():
        raise SystemExit("SDK static libLLVMCore.a missing")
    includes = Path(run([str(config), "--includedir"], environment))
    if not (includes / "llvm-c/Core.h").is_file():
        raise SystemExit("SDK llvm-c/Core.h missing")
    run([str(config), "--link-static", "--libs", "core", "analysis",
         "--system-libs"], environment)
    for tool in ("clang-23", "clang-format-23"):
        output = run([str(bin_dir / tool), "--version"], environment)
        if version not in output:
            raise SystemExit("SDK " + tool + " wrong version")
    if not (sdk / "PROVENANCE.md").is_file() or not (sdk / "licenses").is_dir():
        raise SystemExit("SDK audited provenance/notice files missing")


def _sdk_main(lock: dict, target: str) -> None:
    if target != "ubuntu-24.04-x86_64":
        raise SystemExit("Curated SDK is locked for Ubuntu 24.04 x86_64 only")
    artifact = lock["platforms"][target]["sdk_archive"]
    if artifact["sha256"] != "5369d292ef513d32758634f98ec7cf62c52f82a0a333eb330ecf9a325648b133":
        raise SystemExit("Curated SDK provenance lock unexpectedly changed")
    if shutil.which("zstd") is None or shutil.which("g++") is None:
        raise SystemExit("Install signed Ubuntu zstd and g++ prerequisites")
    identity = hashlib.sha256(LOCK.read_bytes()).hexdigest()[:16]
    prefix = ROOT / ".deps/toolchains" / f"{target}-{identity}"
    archive_dir = ROOT / ".deps/downloads"
    archive_dir.mkdir(parents=True, exist_ok=True)
    prefix.parent.mkdir(parents=True, exist_ok=True)
    archive = archive_dir / artifact["filename"]
    started = time.monotonic()
    if not archive.exists() and not archive.is_symlink():
        print("SDK cache miss: HTTPS exact tagged GitHub Release", flush=True)
        _sdk_download(artifact, archive)
    else:
        print("SDK cache candidate: verify exact SHA and size", flush=True)
    _sdk_valid_archive(artifact, archive)
    print(f"SDK archive independently SHA256 verified "
          f"({archive.stat().st_size} bytes) in "
          f"{time.monotonic() - started:.2f}s", flush=True)
    marker = prefix / ".complete"
    if prefix.is_symlink() or (prefix.exists() and not marker.is_file()):
        raise SystemExit("SDK incomplete or symlink prefix; fail closed")
    if not marker.is_file():
        if shutil.disk_usage(prefix.parent).free < 3 * 1024 ** 3:
            raise SystemExit("SDK staging requires >=3GiB available disk")
        staging = Path(tempfile.mkdtemp(
            prefix=".t449-stage-", dir=prefix.parent))
        try:
            _sdk_decode_and_extract(artifact, archive, staging)
            sdk = _sdk_root(staging)
            _sdk_verify_tools(sdk, lock["llvm_version"])
            (staging / ".complete").write_text(identity + "\n")
            staging.rename(prefix)  # atomic, same filesystem
        finally:
            if staging.exists():
                shutil.rmtree(staging)
    else:
        if marker.read_text() != identity + "\n":
            raise SystemExit("SDK completion marker inconsistent")
    sdk = _sdk_root(prefix)
    _sdk_verify_tools(sdk, lock["llvm_version"])
    activate = ROOT / ".deps/activate.sh"
    marker_text = "# Generated by NewLang P0 bootstrap; no credentials.\n"
    if activate.exists() and not activate.read_text().startswith(marker_text):
        raise SystemExit("Preserving unfamiliar .deps/activate.sh")
    bindir = sdk / "bin"
    activate.write_text(
        marker_text
        + f"export PATH={shlex.quote(str(bindir))}:\"$PATH\"\n"
        + f"export LLVM_CONFIG={shlex.quote(str(bindir / 'llvm-config-23'))}\n"
        + f"export NEWLANG_SDK_ROOT={shlex.quote(str(sdk))}\n"
        + f"export CLANG_FORMAT={shlex.quote(str(bindir / 'clang-format-23'))}\n")
    print(f"LLVM {lock['llvm_version']} exact curated SDK ready at {sdk}; "
          f"total {time.monotonic() - started:.2f}s", flush=True)


def main() -> None:
    if platform.system() != "Linux" or platform.machine() != "x86_64":
        raise SystemExit("P0 supports Linux x86_64 only")
    target_info = platform.freedesktop_os_release()
    target = f"{target_info['ID']}-{target_info['VERSION_ID']}-x86_64"
    lock = json.loads(LOCK.read_text())
    if target not in lock["platforms"]:
        raise SystemExit(f"No reviewed toolchain lock for {target}")
    if "sdk_archive" in lock["platforms"][target]:
        _sdk_main(lock, target)
    else:
        _legacy_main()

if __name__ == "__main__":
    main()
