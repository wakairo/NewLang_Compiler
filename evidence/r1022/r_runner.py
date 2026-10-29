#!/usr/bin/env python3
"""R1023 C120 SOURCE ONLY. Future original FIRST requires a separate C release.
No P fixture/runner, T bridge, shell manifest, synthetic issuer or auto-PASS.
Never execute during C120. Every future stage is raw, append-only evidence.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import resource
import signal
import stat
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
ORIGINAL = "0d933749005f0a2d9ed5777627e91732bb568547"
TARGET = "c9fbdb8d3ce4c238c4e33fb710fefc20ff6654bb"
TARGET_TREE = "ccd82e0bffa3fa0f5d3a5b04083df1d8e18ce7cc"
CASES = (
    ("POS-A-register-before-H", "d3d6c24be29d8961b74ece10cdca1bbb04d129a864f30841950e7fd751cdd128"),
    ("POS-B-H-before-register", "8c182063787b8508bf4c303fddd273b42ce8fa55242cb7381aab5c89ccf057dd"),
    ("NEG-1-missing-Some-link", "8846b35fc4236db2fbf26a35239d6b3a5eb91f11bfb0fcd84d1610b1a3ad7de1"),
    ("NEG-2-wrong-tail-original-pointer", "13f773cfa72556a18e3a2d4bc6c161d67599a840816bb10ad5a027bc45d1077c"),
    ("NEG-3-double-nonCopy-terminal", "1e0186d565e9fc5a87e37c8ef8cbdb7bee19e14c5da5dd4d373457612626b6f7"),
    ("NEG-4-field-loan-escape", "34f2cf52aa41d4d3151f5a6e82ae7e126e34449bba155c1018e48cc9268e8d30"),
)
ORACLE = "6310ec8f87a3a5c87540b0a16047277c2fd742af61b27d0f9dc823c3fb9507f1"
WORKFLOW = ".github/workflows/r-original-first.yml" # currently absent: fail closed
LOG_CAP = 65536
C_CAP = 1048576
CHILD_FSIZE = 2 * 1024 * 1024
APPROVED_TOOLS = {"/usr/bin/git", "/usr/bin/gcc-13", "/usr/bin/clang-23",
                  "/usr/bin/python3"}

def sha(data):
    return hashlib.sha256(data).hexdigest()

def regular(p, cap, within=None):
    if within is not None and (not p.is_relative_to(within) or
       any(x.is_symlink() for x in (p, p.parent, within))):
        raise RuntimeError("unsafe path: " + str(p))
    st = p.lstat()
    if not stat.S_ISREG(st.st_mode) or st.st_nlink != 1 or st.st_size > cap:
        raise RuntimeError("aliased, nonregular, oversized: " + str(p))
    fd = os.open(p, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0))
    try:
        fst = os.fstat(fd)
        data = os.read(fd, cap + 1)
        again = p.lstat()
        identity = lambda x: (x.st_dev, x.st_ino, x.st_size, x.st_mtime_ns)
        if identity(st) != identity(fst) or identity(st) != identity(again) or len(data) > cap:
            raise RuntimeError("unstable evidence bytes: " + str(p))
        return data
    finally:
        os.close(fd)

def inventory():
    m = json.loads(regular(HERE / "manifest.json", 32768, HERE))
    if m["schema"] != "R1022_SOURCE_PUBLICATION_V1" or m["pr1015_head"] != TARGET or m["pr1015_tree"] != TARGET_TREE:
        raise RuntimeError("foreign immutable manifest")
    if len(m["cases"]) != 6:
        raise RuntimeError("cohort count substitution")
    for (name, frozen), v in zip(CASES, m["cases"]):
        path = "sources/" + name + ".nl"
        data = regular(HERE / path, 16384, HERE)
        if (v["id"], v["path"], v["sha256"], v["bytes"]) != (name, path, frozen, len(data)):
            raise RuntimeError("foreign case manifest: " + name)
        if sha(data) != frozen or not data.endswith(b"\n") or b"\r" in data:
            raise RuntimeError("immutable source mismatch: " + name)
    o = m["oracle"]
    data = regular(HERE / "oracle/R1016-FIRST-ORIGINAL-v1.txt", 16384, HERE)
    if o["path"] != "oracle/R1016-FIRST-ORIGINAL-v1.txt" or o["bytes"] != len(data) or o["sha256"] != ORACLE or sha(data) != ORACLE:
        raise RuntimeError("original oracle changed")
    return m

def plain(argv, timeout=10):
    if not argv or argv[0] not in APPROVED_TOOLS:
        raise RuntimeError("foreign provenance command")
    p = subprocess.run(argv, cwd=ROOT, env={"PATH": "/usr/bin:/bin", "LANG": "C"},
                       capture_output=True, timeout=timeout, check=True)
    return p.stdout.decode("utf-8", "strict").strip()

def gate(approved, journal):
    if (os.environ.get("GITHUB_ACTIONS") != "true" or
        os.environ.get("GITHUB_EVENT_NAME") != "push" or
        not os.environ.get("GITHUB_REF", "").startswith("refs/heads/r-fast/") or
        os.environ.get("GITHUB_REPOSITORY") != "wakairo/NewLang_Compiler"):
        raise RuntimeError("unapproved event/ref; reviewed FIRST workflow not yet installed")
    if len(approved) != 40 or any(c not in "0123456789abcdef" for c in approved):
        raise RuntimeError("invalid external C-approved SHA")
    if (os.environ.get("R1023_C_REVIEWED_COMMIT") != approved or
        os.environ.get("GITHUB_SHA") != approved or
        plain(["/usr/bin/git", "rev-parse", "HEAD"]) != approved or
        plain(["/usr/bin/git", "status", "--porcelain"]) != ""):
        raise RuntimeError("checkout/event/reviewed executor SHA mismatch")
    if plain(["/usr/bin/git", "rev-parse", ORIGINAL + "^"]) != TARGET:
        raise RuntimeError("immutable R1022 first publication parent mismatch")
    if plain(["/usr/bin/git", "rev-parse", TARGET + "^{tree}"]) != TARGET_TREE:
        raise RuntimeError("fixed P1015 target tree mismatch")
    if plain(["/usr/bin/git", "merge-base", TARGET, "HEAD"]) != TARGET:
        raise RuntimeError("foreign compiler ancestry")
    # Production is never changed by R; separately approved future workflow\n    # must be independently pinned and reviewed in a later C release.
    subprocess.run(["/usr/bin/git", "diff", "--quiet", TARGET, "HEAD", "--",
                   "src", "include", "cmake", "CMakeLists.txt"],
                   cwd=ROOT, check=True, timeout=10)
    for name, _ in CASES:
        path = "evidence/r1022/sources/" + name + ".nl"
        if plain(["/usr/bin/git", "rev-parse", "HEAD:" + path]) != plain(
           ["/usr/bin/git", "rev-parse", ORIGINAL + ":" + path]):
            raise RuntimeError("published original blob differs from first freeze")
    path = "evidence/r1022/oracle/R1016-FIRST-ORIGINAL-v1.txt"
    if plain(["/usr/bin/git", "rev-parse", "HEAD:" + path]) != plain(
       ["/usr/bin/git", "rev-parse", ORIGINAL + ":" + path]):
        raise RuntimeError("original oracle git object differs")
    expected_ref = "wakairo/NewLang_Compiler/" + WORKFLOW + "@" + os.environ["GITHUB_REF"]
    if os.environ.get("GITHUB_WORKFLOW_REF") != expected_ref:
        raise RuntimeError("foreign workflow ref")
    w = plain(["/usr/bin/git", "rev-parse", "HEAD:" + WORKFLOW])
    if len(os.environ.get("R1023_C_WORKFLOW_BLOB", "")) != 40 or w != os.environ.get("R1023_C_WORKFLOW_BLOB"):
        raise RuntimeError("workflow source not independently pinned")
    for name in ("r_runner.py", "r_probe.c", "r_observer.h"):
        real = sha(regular(HERE / name, 131072, HERE))
        if os.environ.get("R1023_C_" + name.upper().replace(".", "_") + "_SHA256") != real:
            raise RuntimeError("independent code SHA pin absent or mismatched: " + name)
        journal.write({"code": name, "sha256": real})
    return w

class Journal:
    def __init__(self, folder):
        self.folder = folder
        self.fd = os.open(folder / "FIRST-RAW.jsonl",
            os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, "O_NOFOLLOW", 0), 0o600)
    def write(self, x):
        data = (json.dumps(x, sort_keys=True, ensure_ascii=True) + "\n").encode()
        if len(data) > 16384:
            raise RuntimeError("unbounded journal record")
        os.write(self.fd, data)
        os.fsync(self.fd)
    def close(self):
        os.fsync(self.fd)
        os.close(self.fd)

def limits():
    resource.setrlimit(resource.RLIMIT_AS, (2 * 1024**3, 2 * 1024**3))
    resource.setrlimit(resource.RLIMIT_CPU, (45, 45))
    resource.setrlimit(resource.RLIMIT_FSIZE, (CHILD_FSIZE, CHILD_FSIZE))
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))

def stage(argv, folder, name, journal, env=None):
    # No candidate-driven argv, shell, cwd, executable or environment.
    if not argv or (argv[0] not in APPROVED_TOOLS and\n        argv[0] != str(ROOT / "build-frontend/newlangc") and\n        not str(argv[0]).startswith(str(folder.parent) + os.sep)):
        raise RuntimeError("unapproved child command")
    journal.write({"stage": name, "phase": "BEGIN", "argv": argv})
    paths = [folder / (name + "." + suffix) for suffix in ("stdout", "stderr")]
    fds = [os.open(p, os.O_CREAT | os.O_EXCL | os.O_WRONLY |
                  getattr(os, "O_NOFOLLOW", 0), 0o600) for p in paths]
    code = None
    try:
        p = subprocess.Popen(argv, cwd=ROOT, stdout=fds[0], stderr=fds[1],
                env=env or {"PATH": "/usr/bin:/bin", "LANG": "C"},
                preexec_fn=limits, start_new_session=True)
        try:
            code = p.wait(timeout=60)
        except subprocess.TimeoutExpired:
            os.killpg(p.pid, signal.SIGKILL)
            p.wait()
            code = "TIMEOUT"
    except Exception as exc:
        code = "LAUNCH_ERROR:" + type(exc).__name__
    finally:
        for fd in fds:
            os.fsync(fd)
            os.close(fd)
    items = []
    for path in paths:
        size = path.stat().st_size
        items.append({"path": path.name, "bytes": size,
                      "sha256": sha(regular(path, CHILD_FSIZE, folder))})
    result = {"stage": name, "phase": "END", "exit": code, "streams": items}
    journal.write(result)
    if any(x["bytes"] > LOG_CAP for x in items):
        raise RuntimeError("captured stdout/stderr over 64KiB; raw retained")
    return result

def snapshot(path, folder, name, journal):
    if path.exists():
        data = regular(path, C_CAP, folder)
        target = folder / (name + ".bytes")
        with target.open("xb") as out:
            out.write(data)
            out.flush()
            os.fsync(out.fileno())
        metadata = {"exists": True, "sha256": sha(data), "bytes": len(data),
                    "dev": path.stat().st_dev, "ino": path.stat().st_ino,
                    "saved": target.name}
    else:
        metadata = {"exists": False}
    journal.write({"snapshot": name, **metadata})
    return metadata

def observe(m, folder, work, journal):
    compiler = ROOT / "build-frontend/newlangc"
    probe = work / "r1023-probe"
    if not compiler.is_file() or compiler.is_symlink() or compiler.stat().st_nlink != 1:
        raise RuntimeError("actual compiler binary missing")
    real_binary = sha(regular(compiler, 32 * 1024 * 1024))
    if os.environ.get("R1023_C_COMPILER_BINARY_SHA256") != real_binary:
        raise RuntimeError("built compiler SHA not externally attested")
    journal.write({"compiler_binary_sha256": real_binary, "target_source_tree": TARGET_TREE,
                   "binary_build_provenance": "requires separately audited Actions job"})
    libs = ["newlang_semantic", "newlang_checked", "newlang_parser",
            "newlang_syntax", "newlang_lexer", "newlang_source", "newlang_diagnostic"]
    build_probe = ["/usr/bin/gcc-13", "-std=c17", "-Wall", "-Wextra",
                   "-Werror", "-pedantic-errors", "-I", str(ROOT / "include"),
                   str(HERE / "r_probe.c"), "-o", str(probe),
                   "-Wl,--start-group"] + [str(ROOT / "build-frontend" / ("lib" + x + ".a"))
                      for x in libs] + ["-Wl,--end-group"]
    result = stage(build_probe, folder, "public-probe-build", journal)
    if result["exit"] != 0:
        journal.write({"visibility": "NOT_OBSERVABLE", "layer": "probe_build"})
    for index, (name, frozen) in enumerate(CASES):
        src = HERE / "sources" / (name + ".nl")
        journal.write({"case": name, "phase": "FIRST_BEGIN", "original_sha256": frozen,
                       "case_index": index})
        check = stage([str(compiler), "--check", str(src)], folder, name+"-check", journal)
        if result["exit"] == 0:
            stage([str(probe), str(src)], folder, name+"-public-current", journal)
        else:
            journal.write({"case": name, "public_current": "NOT_OBSERVABLE",
                           "first_layer": "probe_build"})
        out = folder / (name + "-named.c")
        if index >= 2:
            with out.open("xb") as f:
                f.write(b"R1023-NEGATIVE-NAMED-SENTINEL\n")
                f.flush();os.fsync(f.fileno())
            before = snapshot(out,folder,name+"-before",journal)
            negative = stage([str(compiler), "--emit-c17-to", str(out), str(src)],
                              folder,name+"-negative-emit",journal)
            after = snapshot(out,folder,name+"-after",journal)
            same = before.get("exists") and after.get("exists") and all(
                before[x] == after[x] for x in ("bytes", "sha256", "dev", "ino"))
            journal.write({"case": name, "first_raw": "NEGATIVE_UNJUDGED",
                "check_exit": check["exit"], "emit_exit": negative["exit"],
                "named_output_no_clobber": bool(same),
                "negative_semantic_reason": "INCONCLUSIVE until diagnostic classified"})
        elif check["exit"] != 0:
            journal.write({"case": name, "first_raw": "POS_CHECK_RED_OR_INCONCLUSIVE",
                           "first_layer": "CHECK", "native": "NOT_OBSERVABLE"})
        else:
            emit = stage([str(compiler), "--emit-c17-to", str(out), str(src)],
                         folder,name+"-positive-emit",journal)
            if emit["exit"] != 0 or not out.exists():
                journal.write({"case": name, "first_raw": "POS_C17_UNSUPPORTED_OR_RED",
                               "first_layer": "emitter", "native": "NOT_OBSERVABLE"})
            else:
                c_data = regular(out,C_CAP,folder)
                if not c_data: raise RuntimeError("empty C17 output")
                journal.write({"case": name, "unaltered_generated_c17": out.name,
                               "sha256": sha(c_data), "bytes": len(c_data)})
                for cc in ("/usr/bin/gcc-13", "/usr/bin/clang-23"):
                    for opt in ("0", "2"):
                        key = name+"-"+Path(cc).name+"-O"+opt
                        exe = work / key
                        base = [cc, "-std=c17", "-Wall", "-Wextra", "-Werror",
                                "-pedantic-errors", "-O"+opt]
                        b = stage(base + [str(out), "-o", str(exe)],
                                  folder,key+"-clean-build",journal)
                        if b["exit"] != 0: continue
                        stage([str(exe)],folder,key+"-clean",journal)
                        obs = work / (key+"-observer")
                        ob = stage(base + ["-I",str(HERE),
                                '-DNEWLANG_HEAP_OBSERVER="r_observer.h"',
                                str(out),"-o",str(obs)],
                                folder,key+"-observer-build",journal)
                        if ob["exit"] != 0: continue
                        for budget in (0,1,2):
                            stage([str(obs)],folder,key+"-world-"+str(budget),journal,
                                  {"PATH":"/usr/bin:/bin","LANG":"C",
                                   "R1022_GUEST_SUCCESS_BUDGET":str(budget)})
                journal.write({"case": name, "first_raw": "NATIVE_RAW_UNJUDGED",
                    "original_H_T_physical": "NOT_OBSERVABLE without verified live address/field witness",
                    "public_current": "see raw probe; not a physical PASS"})
        journal.write({"case": name, "phase": "FIRST_END"})
        # FIRST-RAW.jsonl is already fsync'd, even if later cases STOP.

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--inventory",action="store_true")
    ap.add_argument("--execute",action="store_true")
    ap.add_argument("--approved-sha",default="")
    a=ap.parse_args()
    if a.inventory==a.execute:ap.error("one mode required")
    if a.inventory:
        inventory()
        print("FROZEN ORIGINAL HASHES ONLY; no test or validation run")
        return
    parent=Path(os.environ.get("RUNNER_TEMP","/nonexistent"))
    if not parent.is_dir() or parent.is_symlink() or parent.resolve() == ROOT.resolve():
        raise RuntimeError("Actions temp required; no local/P-like execution")
    run=os.environ.get("GITHUB_RUN_ID","")
    attempt=os.environ.get("GITHUB_RUN_ATTEMPT","")
    if not run.isdecimal() or not attempt.isdecimal():
        raise RuntimeError("Actions run identity required")
    folder=parent / ("R1023-FIRST-"+run+"-"+attempt)
    folder.mkdir(mode=0o700) # fail if already exists: no overwrite of FIRST
    journal=Journal(folder)
    try:
        journal.write({"phase":"FIRST_OPEN","run_id":run,"attempt":attempt,
                       "event":os.environ.get("GITHUB_EVENT_NAME"),"ref":os.environ.get("GITHUB_REF"),
                       "event_sha":os.environ.get("GITHUB_SHA"),"approved_sha":a.approved_sha})
        inventory()
        journal.write({"phase":"ORIGINAL_FROZEN_7_OF_7"})
        workflow=gate(a.approved_sha,journal)
        journal.write({"workflow_blob":workflow,"checkout_sha":a.approved_sha})
        with tempfile.TemporaryDirectory(prefix="r1023-transient-",dir=parent) as scratch:
            observe(None,folder,Path(scratch),journal)
        journal.write({"phase":"FIRST_CAPTURE_END","claim":"RAW_ONLY_NOT_PASS"})
    except BaseException as exc:
        journal.write({"phase":"FIRST_STOP","error_type":type(exc).__name__,
                       "error":str(exc)[:1024],"claim":"RED_OR_INCONCLUSIVE_IMMUTABLE"})
        raise
    finally:
        journal.close() # directory intentionally never cleaned by runner
        # Future trusted workflow MUST upload folder with if:always(), immutable
        # unique artifact name and bounded retention BEFORE Actions workspace ends.
if __name__=="__main__":
    main()
