"""R644 calibrated independent ordinary CLI/native and negative observer.
Producer source is only a syntax/calibration reference; R expectations frozen.
"""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def call(argv, **kw):
    p = subprocess.run(list(map(str, argv)), text=True, capture_output=True,
                       timeout=45, **kw)
    return p


def demand(ok, message):
    if not ok:
        raise AssertionError(message)


def positive(compiler, probe, cc, root, filename, oracle, sanitizer):
    path = root / filename
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    chk = call([compiler, "--check", path])
    compiled = call([compiler, path])
    demand(chk.returncode == compiled.returncode == 0 and
           not chk.stdout and not chk.stderr and not compiled.stderr,
           f"{filename}: CHECK/C17 {chk} {compiled}")
    code = compiled.stdout
    again = call([compiler, path])
    demand(again.returncode == 0 and again.stdout == code,
           "unstable C17 bytes")
    public = call([probe, "public", path])
    demand(public.returncode == 0 and not public.stderr,
           f"public owned {filename}: {public}")
    observed = json.loads(public.stdout)
    demand(observed["bool"] == oracle["bool"] and
           observed["arms"] == oracle["arms"] and
           observed["distinct"] and observed["ended"],
           f"unexpected public IF/world {filename}: {observed}")
    selected = re.search(
        r"const uint8_t (nl_local_\d+) = (nl_if_result_\d+);", code)
    demand(selected is not None and code.count("} else {") == 1,
           "no actual arm-local IF lowering")
    physical = re.search(
        r"nl_type_\d+ (nl_local_\d+) = \{ \.f0 = " +
        str(oracle["old"]) + r", \.f1 = " +
        str(oracle["sibling"]) + r", \};", code)
    demand(physical is not None, "original physical Pair not emitted")
    root_name = physical.group(1)
    watched = code.replace("#include <stdint.h>",
                           "#include <stdint.h>\n#include <stdlib.h>")
    demand(watched != code, "guard include missing")
    guards = [
        (selected.group(0),
         selected.group(1) + " != " + str(oracle["selected"])),
        (physical.group(0),
         root_name + ".f0 != " + str(oracle["old"]) + " || " +
         root_name + ".f1 != " + str(oracle["sibling"])),
    ]
    write = re.search(r"\*nl_local_\d+ = " +
                      re.escape(selected.group(1)) + r";", code)
    demand(write is not None, "scoped write missing")
    guards.append((write.group(0),
                   root_name + ".f0 != " + str(oracle["selected"]) +
                   " || " + root_name + ".f1 != " +
                   str(oracle["sibling"])))
    for anchor, expr in guards:
        demand(watched.count(anchor) == 1, f"nonunique {anchor}")
        watched = watched.replace(anchor, anchor + "\n    if (" +
                                  expr + ") exit(91);", 1)
    # This validates the selected branch's physical readback without writing
    # any guest value; the unmodified native binary is always run separately.
    native = {}
    flags = ["-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror"]
    if sanitizer != "none":
        flags += [f"-fsanitize={sanitizer}", "-fno-sanitize-recover=all"]
    with tempfile.TemporaryDirectory(prefix="r644-c17-") as td:
        for opt in ["-O0", "-O2"]:
            for mode, unit in [("plain", code), ("passive", watched)]:
                src, exe = Path(td) / "guest.c", Path(td) / "guest"
                src.write_text(unit)
                result = call([cc, *flags, opt, src, "-o", exe])
                demand(result.returncode == 0, f"{filename} {opt} compile {result}")
                run = call([exe])
                demand(run.returncode == 0,
                       f"{filename} {opt} {mode} guest {run}")
                native[f"{opt}/{mode}"] = {"rc": run.returncode,
                                           "stdout": run.stdout,
                                           "stderr": run.stderr}
    report = {"case": filename, "class": oracle["class"],
              "sha256": digest, "check": chk.returncode, "owned": observed,
              "c17_sha256": hashlib.sha256(code.encode()).hexdigest(),
              "c17_bytes": len(code), "native": native}
    print(json.dumps(report, sort_keys=True), flush=True)


def negative(compiler, root, filename, kind):
    path = root / filename
    chk, emission = call([compiler, "--check", path]), call([compiler, path])
    if kind == "semantic":
        demand(chk.returncode == emission.returncode == 3 and
               not chk.stdout and not emission.stdout and
               "error(" in chk.stderr,
               f"semantic negative failed closed: {filename} {chk} {emission}")
    else:
        demand(chk.returncode == 0 and emission.returncode == 4 and
               not emission.stdout and
               "V1-BACKEND-UNSUPPORTED" in emission.stderr,
               f"backend unsupported unexpected: {filename} {chk} {emission}")
    print(json.dumps({"case": filename, "sha256": hashlib.sha256(
        path.read_bytes()).hexdigest(), "check": chk.returncode,
        "emit": emission.returncode, "bytes": len(emission.stdout),
        "stderr": emission.stderr.strip()}), flush=True)


def main():
    compiler, probe, cc, directory, sanitizer = sys.argv[1:]
    root = Path(directory)
    cases = {
        "calibration.nl": {"class": "PRODUCER_CALIBRATION",
                           "bool": 1, "arms": [83, 24], "selected": 83,
                           "old": 18, "sibling": 209},
        "r_false.nl": {"class": "R_VARIANT", "bool": 0,
                       "arms": [142, 37], "selected": 37, "old": 11,
                       "sibling": 243},
        "r_equal.nl": {"class": "R_VARIANT", "bool": 1,
                       "arms": [67, 67], "selected": 67, "old": 55,
                       "sibling": 202},
    }
    for name, oracle in cases.items():
        positive(compiler, probe, cc, root, name, oracle, sanitizer)
    for name, kind in [("r_wrong_type.nl", "semantic"),
                       ("r_range.nl", "semantic"),
                       ("r_nested.nl", "backend")]:
        negative(compiler, root, name, kind)
    for name in ["r_false.nl", "r_equal.nl"]:
        attack = call([probe, "attack", root / name])
        demand(attack.returncode == 0 and "rejected=3 malformed=12" in
               attack.stdout and not attack.stderr,
               f"R-owned hostile proof failed {name}: {attack}")
        print(json.dumps({"attack_case": name,
                          "observation": attack.stdout.strip(),
                          "provenance": "R-only shallow copies + issuer-owned"
                                        " original"}), flush=True)
    print("R644 PASS FINITE CALIBRATED EXECUTION", flush=True)


if __name__ == "__main__":
    main()
