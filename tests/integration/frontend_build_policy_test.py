"""Default LLVM guards remain mandatory; explicit frontend opt-in skips them."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile


def main() -> None:
    cmake, repo, cc = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="newlang-build-policy-") as temp:
        root = Path(temp)
        marker = root / "llvm-invoked"
        config = root / "llvm-config-23"
        config.write_text(f"#!/bin/sh\nprintf invoked > '{marker}'\n"
                          "printf '22.1.8\\n'\n", encoding="utf-8")
        config.chmod(0o755)

        def configure(name: str, options: list[str], clean_path: bool = False):
            env = os.environ.copy()
            env.pop("LLVM_CONFIG", None)
            if clean_path:
                env["PATH"] = "/usr/bin:/bin"
            return subprocess.run(
                [cmake, "-S", repo, "-B", str(root / name),
                 "-G", "Unix Makefiles", f"-DCMAKE_C_COMPILER={cc}",
                 "-DBUILD_TESTING=OFF", *options], env=env,
                capture_output=True, text=True, timeout=60, check=False)

        # Even an explicitly supplied broken/mismatched config must never be
        # queried in frontend-only mode, rather than silently falling back.
        frontend = configure("frontend", ["-DNEWLANG_FRONTEND_ONLY=ON",
                                          f"-DLLVM_CONFIG={config}"])
        assert frontend.returncode == 0, frontend.stdout + frontend.stderr
        assert not marker.exists(), "frontend-only consulted llvm-config"
        assert "LLVM C API" not in frontend.stdout
        assert "Frontend-only:" in frontend.stdout
        wrong = configure("wrong", [f"-DLLVM_CONFIG={config}"])
        assert wrong.returncode != 0 and marker.exists()
        assert "LLVM must be exactly 23.1.2; found 22.1.8" in wrong.stderr
        cache = (root / "wrong/CMakeCache.txt").read_text(encoding="utf-8")
        assert "NEWLANG_FRONTEND_ONLY:BOOL=OFF" in cache
        # Suppress all find_program search categories for this dependency
        # probe, after selecting compiler/make explicitly. No fake success.
        missing = configure("missing", ["-DLLVM_CONFIG=LLVM_CONFIG-NOTFOUND",
            "-DCMAKE_MAKE_PROGRAM=/usr/bin/make",
            "-DCMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH=OFF",
            "-DCMAKE_FIND_USE_CMAKE_SYSTEM_PATH=OFF",
            "-DCMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH=OFF",
            "-DCMAKE_FIND_USE_CMAKE_PATH=OFF"], clean_path=True)
        assert missing.returncode != 0, missing.stdout + missing.stderr
        assert "Could not find LLVM_CONFIG" in missing.stderr, missing.stderr
    print("frontend-only makes no LLVM query; default OFF rejects missing/wrong LLVM")


if __name__ == "__main__":
    main()
