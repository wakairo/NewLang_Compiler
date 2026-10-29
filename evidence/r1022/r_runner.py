#!/usr/bin/env python3
"""R1022 original-source runner candidate: source-only publication, NEVER run by C119.
Future C release and exact GitHub Actions checkout are mandatory. Results are
unjudged raw observations, NOT public-owned/current or native physical proof.
No P/T/B fixture, bridge, runner, oracle or synthetic receipt is used.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import resource
import stat
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
CASES = ("POS-A-register-before-H", "POS-B-H-before-register",
         "NEG-1-missing-Some-link", "NEG-2-wrong-tail-original-pointer",
         "NEG-3-double-nonCopy-terminal", "NEG-4-field-loan-escape")
CAP = 65536

def safe_read(path, maximum):
    if not path.is_relative_to(HERE) or path.is_symlink() or path.parent.is_symlink():
        raise RuntimeError("unsafe custody path")
    st = path.lstat()
    if not stat.S_ISREG(st.st_mode) or st.st_nlink != 1 or st.st_size > maximum:
        raise RuntimeError("aliased, non-file or oversized evidence")
    fd = os.open(path, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0))
    try:
        got = os.fstat(fd)
        data = os.read(fd, maximum + 1)
        again = path.lstat()
        if ((st.st_dev, st.st_ino, st.st_size, st.st_mtime_ns) !=
            (got.st_dev, got.st_ino, got.st_size, got.st_mtime_ns) or
            (st.st_dev, st.st_ino, st.st_size, st.st_mtime_ns) !=
            (again.st_dev, again.st_ino, again.st_size, again.st_mtime_ns)):
            raise RuntimeError("unstable custody")
        return data
    finally:
        os.close(fd)

def verify():
    doc = json.loads(safe_read(HERE / "manifest.json", 32768))
    if (doc["schema"] != "R1022_SOURCE_PUBLICATION_V1" or
        doc["pr1015_head"] != "c9fbdb8d3ce4c238c4e33fb710fefc20ff6654bb" or
        tuple(v["id"] for v in doc["cases"]) != CASES):
        raise RuntimeError("foreign cohort/target")
    for v in doc["cases"] + [doc["oracle"]]:
        wanted = ("sources/" + v["id"] + ".nl") if "id" in v else (
            "oracle/R1016-FIRST-ORIGINAL-v1.txt")
        if v["path"] != wanted:
            raise RuntimeError("manifest path substitution")
        payload = safe_read(HERE / wanted, 16384)
        if (len(payload) != v["bytes"] or
            hashlib.sha256(payload).hexdigest() != v["sha256"] or
            not payload.endswith(b"\n") or b"\r" in payload):
            raise RuntimeError("original custody mismatch: " + wanted)
    return doc

def future_gate(sha):
    if os.environ.get("GITHUB_ACTIONS") != "true" or not (
        len(sha) == 40 and all(c in "0123456789abcdef" for c in sha) and
        os.environ.get("R1022_C_APPROVED_SHA") == sha and
        os.environ.get("GITHUB_SHA") == sha):
        raise RuntimeError("future C release and Actions exact SHA required")
    for argv, expected in (
        (["/usr/bin/git", "rev-parse", "HEAD"], sha),
        (["/usr/bin/git", "status", "--porcelain"], "")):
        res = subprocess.run(argv, cwd=ROOT, capture_output=True,
                             text=True, timeout=10, check=True)
        if res.stdout.strip() != expected:
            raise RuntimeError("unclean or foreign checkout; PR merge needs review")

def limits():
    resource.setrlimit(resource.RLIMIT_AS, (2 * 1024**3, 2 * 1024**3))
    resource.setrlimit(resource.RLIMIT_CPU, (45, 45))
    resource.setrlimit(resource.RLIMIT_FSIZE, (CAP, CAP))

def run(argv, directory, label, env=None):
    flags = os.O_EXCL | os.O_CREAT | os.O_WRONLY | getattr(os, "O_NOFOLLOW", 0)
    paths = [directory / (label + suffix) for suffix in (".stdout", ".stderr")]
    a, b = [os.open(p, flags, 0o600) for p in paths]
    try:
        try:
            r = subprocess.run(argv, cwd=ROOT, env=env or {
                "PATH": "/usr/bin:/bin", "LANG": "C"}, stdout=a, stderr=b,
                timeout=60, preexec_fn=limits)
            code = r.returncode
        except subprocess.TimeoutExpired:
            code = "TIMEOUT"
    finally:
        os.close(a)
        os.close(b)
    if any(p.stat().st_size > CAP for p in paths):
        raise RuntimeError("unbounded captured output")
    return {"exit": code, "stdout": paths[0].read_text(errors="replace"),
            "stderr": paths[1].read_text(errors="replace")}

def capture(doc, directory):
    compiler = ROOT / "build-frontend/newlangc"
    if not compiler.is_file() or compiler.is_symlink():
        raise RuntimeError("future actual compiler binary missing")
    record = {}
    for v in doc["cases"]:
        name = v["id"]
        source = HERE / v["path"]
        first = {"original_sha256": v["sha256"]}
        first["check"] = run([str(compiler), "--check", str(source)],
                             directory, name + "-check")
        if v["kind"] == "negative":
            out = directory / (name + "-named.c")
            out.write_bytes(b"R1022-ORIGINAL-NO-CLOBBER\n")
            st = out.stat()
            first["named_emit"] = run([str(compiler), "--emit-c17-to",
                                       str(out), str(source)], directory,
                                      name + "-emit-negative")
            st2 = out.stat()
            first["no_clobber"] = ((st.st_dev, st.st_ino) ==
                (st2.st_dev, st2.st_ino) and
                out.read_bytes() == b"R1022-ORIGINAL-NO-CLOBBER\n")
            first["verdict"] = "FIRST-RAW-NEGATIVE-UNJUDGED"
        elif first["check"]["exit"] != 0:
            first["verdict"] = "FIRST-RAW-POS-CHECK-RED_OR_INCONCLUSIVE"
        else:
            out = directory / (name + ".c")
            first["emit"] = run([str(compiler), "--emit-c17-to",
                                 str(out), str(source)], directory,
                                name + "-emit-positive")
            if first["emit"]["exit"] != 0 or not out.is_file():
                first["verdict"] = "FIRST-RAW-POS-C17-UNSUPPORTED_OR_RED"
            else:
                data = out.read_bytes()
                if not data or len(data) > 1048576:
                    raise RuntimeError("invalid generated C size")
                first["unaltered_c17_sha256"] = hashlib.sha256(data).hexdigest()
                first["native"] = {}
                for tag, cc in (("gcc", "/usr/bin/gcc-13"),
                                ("clang", "/usr/bin/clang-23")):
                    if not Path(cc).exists():
                        raise RuntimeError("missing fixed toolchain")
                    for opt in ("0", "2"):
                        key = tag + "-O" + opt
                        exe = directory / (name + "-" + key)
                        common = [cc, "-std=c17", "-Wall", "-Wextra", "-Werror",
                                  "-pedantic-errors", "-O" + opt]
                        b = run(common + [str(out), "-o", str(exe)],
                                directory, name + "-" + key + "-build")
                        first["native"][key] = {"clean_build": b}
                        if b["exit"] != 0:
                            continue
                        first["native"][key]["clean"] = run(
                            [str(exe)], directory, name + "-" + key + "-clean")
                        obs = directory / (name + "-" + key + "-instrumented")
                        ob = run(common + ["-I", str(HERE),
                            '-DNEWLANG_HEAP_OBSERVER="r_observer.h"',
                            str(out), "-o", str(obs)],
                            directory, name + "-" + key + "-observer-build")
                        first["native"][key]["observer_build"] = ob
                        if ob["exit"] != 0:
                            continue
                        for world in (0, 1, 2):
                            env = {"PATH": "/usr/bin:/bin", "LANG": "C",
                                   "R1022_GUEST_SUCCESS_BUDGET": str(world)}
                            first["native"][key]["guest" + str(world)] = run(
                                [str(obs)], directory,
                                name + "-" + key + "-world-" + str(world), env)
                first["verdict"] = "NATIVE-CAPTURE-ONLY-ISSUER_AND_PHYSICAL_UNPROVED"
        record[name] = first
    (directory / "first-raw-record.json").write_text(
        json.dumps(record, indent=2, sort_keys=True) + "\n")
    return record

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--inventory", action="store_true")
    ap.add_argument("--execute", action="store_true")
    ap.add_argument("--approved-sha", default="")
    args = ap.parse_args()
    if args.inventory == args.execute:
        ap.error("choose exactly one mode")
    doc = verify()
    if args.inventory:
        print("R1022 frozen hashes verified; NO execution or result claim")
        return
    future_gate(args.approved_sha)
    temp = Path(os.environ.get("RUNNER_TEMP", "/nonexistent"))
    if not temp.is_dir() or temp.is_symlink():
        raise RuntimeError("invalid future Actions temp directory")
    with tempfile.TemporaryDirectory(prefix="r1022-", dir=temp) as folder:
        evidence = capture(doc, Path(folder))
        print("RAW CAPTURE ONLY; not persisted or validated: " +
              ",".join(evidence.keys()))

if __name__ == "__main__":
    main()
