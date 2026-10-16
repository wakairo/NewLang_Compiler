#!/usr/bin/env python3
"""R518 calibrated, original R516 source, live-frame passive native observer.
No guest state mutation. The prior R516 evidence remains untouched.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path.cwd()
SRC = ROOT / "tests/r516"
OUT = ROOT / "r518-evidence"
OUT.mkdir(exist_ok=True)
COMPILER = Path(os.environ["R518_COMPILER"]).resolve()
HASHES = {
    "true": ("c921596f72e64116630676b98d6ce43c529420a4bec26c437d01981cc39cc864", 108),
    "false": ("0dfc413cfc63fa379b6b4c1d20a6beea1421188add4e05b9d1bc660fc0ca1664", 173),
    "reject_arm": ("d9f6c09fdb63ba50e5beaeb26b6ada585cfe67c28883a53c53c7504ce6c0c272", None),
}
LOG = []
def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save():
    (OUT / "results.json").write_text(json.dumps(LOG, indent=2) + "\n")
def stop(code, detail):
    LOG.append({"STOP": code, "detail": str(detail)})
    save()
    print("R518 STOP", code, detail, flush=True)
    raise SystemExit(2)
def run(label, cmd, timeout=180):
    r = subprocess.run([str(x) for x in cmd], cwd=ROOT, capture_output=True,
                       text=True, timeout=timeout)
    LOG.append({"stage": label, "command": list(map(str, cmd)),
                "exit": r.returncode, "stdout": r.stdout, "stderr": r.stderr})
    save()
    print("R518", label, "exit", r.returncode,
          "out", r.stdout[:4000], "err", r.stderr[:4000], flush=True)
    return r
def match_unique(pattern, source, what, flags=0):
    found = list(re.finditer(pattern, source, flags))
    if len(found) != 1:
        stop("OBSERVER INCONCLUSIVE", f"{what} matches={len(found)}")
    return found[0]

for name, (expected_sha, _) in HASHES.items():
    if sha(SRC / (name + ".nl")) != expected_sha:
        stop("SOURCE HASH MISMATCH", name)
if (SRC / "true.nl").read_text().replace("operand_b = usize(29)",
                                              "operand_b = usize(30)") != (SRC / "false.nl").read_text():
    stop("SOURCE DIFF MISMATCH", "TRUE and FALSE differ unexpectedly")
print("R518 exact frozen original R516 sources confirmed", flush=True)
print("R518 compiler binary SHA256", sha(COMPILER), flush=True)
LOG.append({"candidate_binary_sha256": sha(COMPILER),
            "source_sha256": {k:v[0] for k,v in HASHES.items()},
            "candidate_commit": os.environ["R518_SHA"]})
save()

for name in ("true", "false"):
    r = run(name + ".CHECK", [COMPILER, "--check", SRC / (name + ".nl")])
    if r.returncode != 0 or r.stdout or r.stderr:
        stop("CHECK DISCREPANCY", name)
neg = run("reject_arm.CHECK", [COMPILER, "--check", SRC / "reject_arm.nl"])
if neg.returncode != 3 or "V1-U8-LITERAL-RANGE" not in neg.stderr:
    stop("SEMANTIC NEGATIVE DISCREPANCY", f"exit={neg.returncode}: {neg.stderr}")

def decode_observer_text(cpath):
    txt = cpath.read_text()
    lines = txt.splitlines()
    # Metadata extraction is used ONLY to find emitted guest C symbols and GDB
    # source locations. Guest expected values come solely from frozen PLAN.md.
    def one(pattern, what, flags=0):
        return match_unique(pattern, txt, what, flags)
    fn = one(r"static void (nl_fn_\d+)\(void\)\s*\{", "guest function").group(1)
    orig = one(r"\bnl_type_\d+\s+nl_local_(\d+)\s*=\s*\{", "original Pair").group(1)
    ref = one(r"\buint8_t\s+\*const\s+nl_local_(\d+)\s*=\s*&nl_local_" + orig + r"\.f0\s*;", "original field loan").group(1)
    write = one(r"\*\s*nl_local_" + ref + r"\s*=\s*[^;]+;", "actual guest write")
    old = one(r"\b(?:const\s+)?uint8_t\s+nl_local_(\d+)\s*=\s*nl_loan_result_\d+\s*;", "former local").group(1)
    left = one(r"\b(?:const\s+)?uint8_t\s+nl_local_(\d+)\s*=\s*nl_local_" + orig + r"\.f0\s*;", "new left local").group(1)
    right = one(r"\b(?:const\s+)?uint8_t\s+nl_local_(\d+)\s*=\s*nl_local_" + orig + r"\.f1\s*;", "right local").group(1)
    selected = one(r"\bconst uint8_t nl_local_(\d+)\s*=\s*\([^;]*\?[^;]*:[^;]*\)\s*;", "pure Copy IF selected").group(1)
    func_start = txt[:txt.index(fn + "(void)\n{")].count("\n")
    line_start = func_start + 1
    depth = 0
    end = None
    began = False
    for line_number in range(line_start, len(lines) + 1):
        line = lines[line_number-1]
        for char in line:
            if char == "{":
                depth += 1
                began = True
            elif char == "}":
                depth -= 1
        if began and depth == 0:
            end = line_number
            break
    if end is None:
        stop("OBSERVER INCONCLUSIVE", "no live guest-function closing brace")
    write_line = txt[:write.start()].count("\n") + 1
    if not (line_start < write_line < end):
        stop("OBSERVER INCONCLUSIVE", "write is not in guest function")
    if not (txt.find("nl_local_" + old + " =") < txt.find("nl_local_" + right + " =")):
        stop("OBSERVER INCONCLUSIVE", "source old/right ordering")
    if "int main(void)" not in txt:
        stop("OBSERVER INCONCLUSIVE", "no C wrapper to disambiguate")
    return {"function":fn, "original":orig, "ref":ref, "old":old,
            "left":left, "right":right, "selected":selected,
            "write_line":write_line, "end_line":end}

def native_observe(name, cc, cpath, binary, model, new, expected_old=37, expected_right=211):
    o, ref = model["original"], model["ref"]
    old, left, right, selected = [model[k] for k in ("old", "left", "right", "selected")]
    # Read-only GDB Python: no assignment, set, call, C-file instrumenting or
    # value-inserting. Observation at write and inside the *same* guest function
    # just before return. Never at the wrapper's return.
    addr = lambda expr: f'int(gdb.parse_and_eval("(unsigned long)({expr})"))'
    def body(fields):
        pairs = ", ".join(f'"{k}": {v}' for k, v in fields.items())
        return "{" + pairs + "}"
    common = {
        "frame": "str(gdb.selected_frame().name())",
        "root": addr(f"&nl_local_{o}"),
        "f0addr": addr(f"&nl_local_{o}.f0"),
        "f1addr": addr(f"&nl_local_{o}.f1"),
        "f0": f'int(gdb.parse_and_eval("nl_local_{o}.f0"))',
        "f1": f'int(gdb.parse_and_eval("nl_local_{o}.f1"))',
    }
    early = dict(common, pointer=addr(f"nl_local_{ref}"))
    late = dict(common,
                former=f'int(gdb.parse_and_eval("nl_local_{old}"))',
                now_left=f'int(gdb.parse_and_eval("nl_local_{left}"))',
                sibling=f'int(gdb.parse_and_eval("nl_local_{right}"))',
                selected=f'int(gdb.parse_and_eval("nl_local_{selected}"))')
    def block(label, fields):
        return "python\nimport gdb, json\nprint('R518_OBS " + label + " ' + json.dumps(" + body(fields) + "))\nend\n"
    script = OUT / f"{name}-{cc}.gdb"
    script.write_text(
        "set pagination off\nset confirm off\nset print pretty off\n"
        + f"break {cpath}:{model['write_line']}\n"
        + f"break {cpath}:{model['end_line']}\nrun\n"
        + block("before", early)
        + "next\n"
        + block("after", early)
        + "continue\n"
        + block("final", late)
        + "quit\n")
    r = run(f"{name}.{cc}.GDB_LIVE", ["gdb", "-q", "-batch", "-nx", "-x", script, binary])
    (OUT / f"{name}-{cc}.gdb.txt").write_text(r.stdout + "\n" + r.stderr)
    if r.returncode != 0:
        stop("NATIVE IDENTITY INCONCLUSIVE", f"GDB exit={r.returncode} {r.stderr}")
    observations = {}
    for line in r.stdout.splitlines():
        m = re.match(r"^R518_OBS (before|after|final) (\{.*\})$", line)
        if m:
            if m.group(1) in observations:
                stop("NATIVE IDENTITY INCONCLUSIVE", "duplicate live checkpoint")
            observations[m.group(1)] = json.loads(m.group(2))
    if set(observations) != {"before", "after", "final"}:
        stop("NATIVE IDENTITY INCONCLUSIVE", f"missing live checkpoint(s): {list(observations)}")
    b, a, z = (observations[x] for x in ("before", "after", "final"))
    if any(x["frame"] != model["function"] for x in (b,a,z)):
        stop("NATIVE IDENTITY INCONCLUSIVE", f"not inside live guest function: {observations}")
    for x in (b,a,z):
        if x["root"] == 0 or x["f0addr"] != x["root"] or x["f1addr"] != x["root"] + 1:
            stop("PHYSICAL IDENTITY FINDING", f"bad addresses {x}")
    if not (b["root"] == a["root"] == z["root"] and
            b["f0addr"] == a["f0addr"] == z["f0addr"] and
            b["f1addr"] == a["f1addr"] == z["f1addr"]):
        stop("PHYSICAL IDENTITY FINDING", "root/field address switched during execution")
    if b["pointer"] != b["f0addr"] or a["pointer"] != a["f0addr"]:
        stop("PHYSICAL IDENTITY FINDING", "write-ref does not point to original.left")
    if not (b["f0"] == expected_old and b["f1"] == expected_right and
            a["f0"] == new and a["f1"] == expected_right and
            z["f0"] == new and z["f1"] == expected_right and
            z["former"] == expected_old and z["now_left"] == new and
            z["sibling"] == expected_right and z["selected"] == new):
        stop("NATIVE VALUE FINDING", f"oracle mismatch: {observations}")
    LOG.append({"stage": f"{name}.{cc}.PHYSICAL_PASS", "observed": observations})
    save()
    print("R518 physical original guest", name, cc, json.dumps(observations), flush=True)

for name in ("true","false"):
    new = HASHES[name][1]
    emit = run(name+".EMIT", [COMPILER, SRC / (name+".nl")])
    if emit.returncode != 0 or not emit.stdout.strip():
        stop("C17 EMISSION FINDING", name+" "+emit.stderr)
    cpath = (OUT / (name + ".c")).resolve()
    cpath.write_text(emit.stdout)
    original_c_sha = sha(cpath)
    model = decode_observer_text(cpath)
    LOG.append({"stage": name+".BASELINE_C", "sha256": original_c_sha,
                "model": model})
    save()
    print("R518 original emitted C SHA256", name, original_c_sha, "live model", model, flush=True)
    for cc in ("gcc", "clang"):
        if not shutil.which(cc):
            stop("ENVIRONMENT", "missing "+cc)
        for opt in ("O0", "O2"):
            exe = OUT / f"{name}-{cc}-{opt}"
            flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic",
                     "-Werror", "-g", "-"+opt, "-fno-omit-frame-pointer"]
            r = run(f"{name}.{cc}.{opt}.COMPILE", [cc, *flags, cpath, "-o", exe])
            if r.returncode != 0:
                stop("C17 COMPILE FINDING", f"{name} {cc} {opt}")
            r = run(f"{name}.{cc}.{opt}.NATIVE", [exe])
            if r.returncode != 0:
                stop("C17 NATIVE FINDING", f"{name} {cc} {opt}")
            if opt == "O0":
                native_observe(name, cc, cpath, exe, model, new)
    exe = OUT / (name+"-ubsan")
    r = run(name+".UBSAN.COMPILE",
            ["clang", "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-g", "-O0", "-fsanitize=undefined", "-fno-sanitize-recover=all",
             cpath, "-o", exe])
    if r.returncode != 0: stop("UBSAN BUILD FINDING", name)
    r = run(name+".UBSAN.NATIVE", [exe])
    if r.returncode != 0: stop("UBSAN NATIVE FINDING", name)
    if sha(cpath) != original_c_sha:
        stop("BASELINE C TAMPERING", name)
print("R518 candidate finite native identity stages complete; public-owned review remains separate", flush=True)
