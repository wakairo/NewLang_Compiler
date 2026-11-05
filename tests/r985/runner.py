#!/usr/bin/env python3
"""R985 first-pass source-first checker; never creates checked-owned receipts."""
import hashlib
import pathlib
import shutil
import subprocess
import sys
import tempfile


def invoke(*args, allow_failure=False):
    cmd = [str(arg) for arg in args]
    print("COMMAND", " ".join(cmd), flush=True)
    result = subprocess.run(cmd, capture_output=True, text=True, check=False)
    print("EXIT", result.returncode, flush=True)
    print("STDOUT_BEGIN\n" + result.stdout + "\nSTDOUT_END", flush=True)
    print("STDERR_BEGIN\n" + result.stderr + "\nSTDERR_END", flush=True)
    if result.returncode and not allow_failure:
        raise RuntimeError("FAILED command: " + " ".join(cmd))
    return result


def verify(mode, cli, source, c_compiler, sanitizer):
    source = pathlib.Path(source)
    content = source.read_bytes()
    print("R985 SOURCE", source.name, "BYTES", len(content),
          "SHA256", hashlib.sha256(content).hexdigest(), flush=True)
    with tempfile.TemporaryDirectory(prefix="r985-") as temp:
        dest = pathlib.Path(temp) / "generated.c"
        sentinel = b"R985_SENTINEL_no_partial_C17\\n"
        if mode == "negative":
            dest.write_bytes(sentinel)
            chk = invoke(cli, "--check", source, allow_failure=True)
            emission = invoke(cli, "--emit-c17-to", dest, source, allow_failure=True)
            if dest.read_bytes() != sentinel:
                raise RuntimeError("NEGATIVE CLOBBERED named C17 destination")
            if chk.returncode == 0 or emission.returncode == 0:
                raise RuntimeError("UNSOUND/UNEXPECTED NEGATIVE SOURCE ACCEPTANCE")
            print("R985 REJECTED SOURCE (exact guard unclassified); no C17/native",
                  flush=True)
            return
        check = invoke(cli, "--check", source, allow_failure=True)
        if check.returncode:
            raise RuntimeError("FIRST CHECK REJECTION (source eligibility/precision): "
                               + source.name)
        emitted = invoke(cli, "--emit-c17-to", dest, source, allow_failure=True)
        if emitted.returncode or not dest.is_file():
            raise RuntimeError("FIRST C17 EMITTER REJECTION: " + source.name)
        cbytes = dest.read_bytes()
        print("R985 C17 SHA256", hashlib.sha256(cbytes).hexdigest(),
              "BYTES", len(cbytes), flush=True)
        print("R985 ORIGINAL UNMODIFIED GENERATED C17 BEGIN", flush=True)
        sys.stdout.buffer.write(cbytes)
        sys.stdout.buffer.flush()
        print("\nR985 ORIGINAL UNMODIFIED GENERATED C17 END", flush=True)
        ccset = [c_compiler]
        for candidate in ("gcc-13", "clang-23", "clang"):
            found = shutil.which(candidate)
            if found is not None and found not in ccset:
                ccset.append(found)
        if not any("gcc" in pathlib.Path(x).name for x in ccset):
            print("R985 GCC NOT AVAILABLE; no fabricated native evidence", flush=True)
        if not any("clang" in pathlib.Path(x).name for x in ccset):
            print("R985 CLANG NOT AVAILABLE; no fabricated native evidence", flush=True)
        for compiler in ccset:
            for opt in ("O0", "O2"):
                exe = pathlib.Path(temp) / ("native_" + str(len(str(compiler))) + "_"
                                            + pathlib.Path(compiler).name + "_" + opt)
                invoke(compiler, "-std=c17", "-Wall", "-Wextra", "-Wpedantic",
                       "-Werror", "-" + opt, dest, "-o", exe)
                invoke(exe)
                print("R985 NATIVE EXIT0 ONLY; old-Copy guest value,"
                      " original physical identity, W0/W1/W2 fault injection"
                      " NOT OBSERVED", flush=True)
        print("R985 CHECK+EMIT+C17 smoke complete; source-owned GET/current,"
              " physical allocation and native guest values NOT OBSERVED", flush=True)


if __name__ == "__main__":
    if len(sys.argv) != 6 or sys.argv[1] not in ("positive", "negative"):
        raise SystemExit("usage: runner.py positive|negative newlangc source.nl cc sanitizer")
    verify(*sys.argv[1:])
