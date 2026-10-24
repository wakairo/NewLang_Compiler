#!/usr/bin/env python3
"""R845 separately frozen independent CLI/native/fault tests (not P fixtures)."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

import physical

SHA = {
    "r839_azurite_1root.nl": "1e9e7f92872b86b82d69f2154391b41c51b39cc09c43703816c95514ddf6999c",
    "r839_terraces_2root.nl": "129ded0ef85cd486b89b41053ad878a51b91528f195c67525bc00bdaebec1d74",
    "r839_reject_arity3.nl": "6ba92a43981b0d3ca13915efa01c8723a6888112dfd220e149e7949d278f6b03",
    "r839_reject_element_u8.nl": "540f2c04ca2ad85bcebdcd9a64cde12d9a4df6928149b607a95f294f5d7fefb8",
    "r839_reject_indirect_none.nl": "337fa43880c41b6f6a0ea78e7115addf4382988e5a9744d40d33fa0ad851df75",
    "r839_reject_some_without_payload.nl": "0d8a54f2432174d9e63874a94d8b2dc7fdb03ff263235bd39c6df5e36e36ca97",
}
POS = ("r839_azurite_1root.nl", "r839_terraces_2root.nl")
NEG = tuple(name for name in SHA if name not in POS)


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def run(args, **kw):
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            timeout=25, **kw)
    return result


def validate_inputs(root):
    for name, expected in SHA.items():
        raw = (root / name).read_bytes()
        actual = hashlib.sha256(raw).hexdigest()
        require(actual == expected, name + ": original R839 source bytes changed")


def compiler_output(cli, source):
    checked = run([cli, "--check", str(source)])
    require(checked.returncode == 0 and not checked.stdout and not checked.stderr,
            source.name + ": ordinary --check mismatch " + repr(checked.stderr[:600]))
    emitted = run([cli, str(source)])
    require(emitted.returncode == 0 and emitted.stdout and not emitted.stderr,
            source.name + ": ordinary C17 emission failure " +
            repr(emitted.stderr[:600]))
    return emitted.stdout


def native_compile(c_source, out, cc, optimization):
    result = run([cc, "-std=c17", "-Wall", "-Wextra", "-Werror", "-pedantic",
                  optimization, str(c_source), "-o", str(out)])
    require(result.returncode == 0,
            cc + " " + optimization + " strict C17 failure " +
            repr(result.stderr[-1000:]))
    executed = run([str(out)])
    require(executed.returncode == 0 and not executed.stdout and not executed.stderr,
            cc + " " + optimization + " native divergence " +
            repr((executed.returncode, executed.stdout[:300], executed.stderr[:300])))


def plain_native(cli, root):
    compilers = [shutil.which("gcc"), shutil.which("clang")]
    require(all(compilers), "independent GCC and Clang required")
    with tempfile.TemporaryDirectory(prefix="r845-native-") as tmp:
        folder = Path(tmp)
        physical_passes = 0
        for i, name in enumerate(POS):
            c17 = compiler_output(cli, root / name)
            path = folder / ("original" + str(i) + ".c")
            path.write_bytes(c17)
            digest = hashlib.sha256(c17).hexdigest()
            for j, cc in enumerate(compilers):
                for optimization in ("-O0", "-O2"):
                    native_compile(path, folder / ("run-%s-%s-%s" % (i, j, optimization)),
                                   cc, optimization)
            print("R845 PLAIN GENERATED C17 ORIGINAL input=%s sha256=%s" %
                  (name, digest), flush=True)
            labels = (("cobalt_inventory",) if i == 0 else
                      ("valley_register", "terrace_register"))
            if physical.test(c17, labels, folder, compilers):
                physical_passes += 1
        print("R845 PHYSICAL DERIVATIVE witnessed=%d/2; %s" %
              (physical_passes, "FINITE PASS" if physical_passes == 2
               else "INCOMPLETE/NOT TESTED"), flush=True)


def reject_sources(cli, root):
    for name in NEG:
        path = str(root / name)
        for cmd in ([cli, "--check", path], [cli, path]):
            result = run(cmd)
            require(result.returncode != 0 and not result.stdout,
                    "negative unexpectedly accepted/generated C17: " + name)
            print("R845 NEGATIVE %s command=%s exit=%d" %
                  (name, "CHECK" if "--check" in cmd else "EMIT",
                   result.returncode), flush=True)


def named_fault(cli, root, shim_source, sanitizer):
    if sanitizer and sanitizer.lower() not in ("", "none", "off", "0"):
        print("R845 NAMED REACHED FAULT=NOT TESTED on sanitizer runner", flush=True)
        return
    with tempfile.TemporaryDirectory(prefix="r845-fault-") as tmp:
        folder = Path(tmp)
        so = folder / "fault.so"
        built = run(["gcc", "-std=c17", "-D_GNU_SOURCE", "-fPIC", "-shared", "-O0",
                     "-Wall", "-Wextra", "-Werror", str(shim_source), "-o", str(so)])
        require(built.returncode == 0, "allocator wrapper build: " +
                repr(built.stderr))
        # Keep target in a separate directory to enumerate actual temp siblings.
        dest = folder / "output"
        dest.mkdir()
        output = dest / "array-original.c"
        source = str(root / POS[1])
        sentinel = b"R845 named output SENTINEL\\x00\\xff\\n"
        reached = []
        for index in range(1, 97):
            output.write_bytes(sentinel)
            before = output.stat()
            env = os.environ.copy()
            env["R845_FAIL_AT"] = str(index)
            env["LD_PRELOAD"] = str(so)
            result = run([cli, "--emit-c17-to", str(output), source], env=env)
            if b"R845_REACHED_FAIL" not in result.stderr:
                continue
            if result.returncode == 0:
                # A failed optional allocation can legitimately be tolerated;
                # do not treat it as the compiler's error path.
                continue
            require(not result.stdout,
                    "fault path emitted stdout/C17; allocation index=%d" % index)
            require(output.read_bytes() == sentinel,
                    "fault clobbered named output; index=%d" % index)
            after = output.stat()
            require((before.st_dev, before.st_ino) ==
                    (after.st_dev, after.st_ino),
                    "fault replaced inode; index=%d" % index)
            require(set(p.name for p in dest.iterdir()) == {output.name},
                    "fault left temporary sibling; index=%d" % index)
            reached.append(index)
            print("R845 REACHED FAULT PHASE UNKNOWN index=%d return=%d "
                  "bytes=preserved inode=preserved siblings=0" %
                  (index, result.returncode), flush=True)
            if len(reached) == 2:
                break
        require(len(reached) >= 2, "fewer than two traced CLI reached failures")
        retry = run([cli, "--emit-c17-to", str(output), source])
        require(retry.returncode == 0 and not retry.stdout and not retry.stderr,
                "clean named retry failed: " + repr(retry.stderr[:600]))
        c17 = output.read_bytes()
        require(c17 != sentinel and bool(c17),
                "clean retry did not replace sentinel with C17")
        require(set(p.name for p in dest.iterdir()) == {output.name},
                "clean retry left temp siblings")
        emitted = compiler_output(cli, root / POS[1])
        require(c17 == emitted, "named retry C17 differs from ordinary stdout C17")
        gcc = shutil.which("gcc")
        require(gcc is not None, "GCC required for clean retry")
        native_compile(output, folder / "clean-retry", gcc, "-O0")
        print("R845 NAMED CLEAN RETRY C17 SHA256=" +
              hashlib.sha256(c17).hexdigest(), flush=True)


def main():
    require(len(sys.argv) == 6, "usage: test.py MODE NEWLANGC SRCDIR SHIM SANITIZER")
    mode, cli, directory, shim, sanitizer = sys.argv[1:]
    root = Path(directory)
    validate_inputs(root)
    if mode == "native":
        plain_native(cli, root)
    elif mode == "negative":
        reject_sources(cli, root)
    elif mode == "fault":
        named_fault(cli, root, Path(shim), sanitizer)
    else:
        raise RuntimeError("unknown test mode " + mode)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        print("R845 TEST FAILURE: " + str(error), file=sys.stderr)
        sys.exit(1)
