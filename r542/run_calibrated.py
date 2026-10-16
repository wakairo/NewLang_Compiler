#!/usr/bin/env python3
"""R542 calibrated independent source-owned proof and ORIGINAL emitted-C observer.
This is NOT a replay of BLIND R540 and does not change R540-01.
No private artifact construction, no candidate implementation mutation.
"""
import difflib
import hashlib
import json
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path.cwd()
R = ROOT / "r"
CANDIDATE = ROOT / "compiler"
OUT = ROOT / "r542-results"
OUT.mkdir(exist_ok=True)
COMPILER_SHA = "7f639a3a89d55f44848e361e1458529fe5395345"
record = {"authority": {"fixed_compiler": COMPILER_SHA,
           "canonical": "Draft17.32",
           "main": "ebb8c4d034a6df2ec6fbfae7e47ebb37650accae"},
          "mode": "NON-BLIND CALIBRATED only",
          "R540_original": "BLIND FINDING R540-01 two-root precision mismatch (unchanged)",
          "steps": [], "result": "INCONCLUSIVE"}
def sha(blob):
    if isinstance(blob, pathlib.Path):
        blob = blob.read_bytes()
    return hashlib.sha256(blob).hexdigest()
def save():
    (OUT / "r542_report.json").write_text(json.dumps(record, indent=2) + "\n")
def required(ok, why):
    if not ok:
        record["result"] = "STOP / INCONCLUSIVE or CALIBRATED FINDING"
        record["stop_reason"] = why
        save()
        raise RuntimeError("R542 STOP: " + why)
def command(name, argv, expect=0):
    args = list(map(str, argv))
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            cwd=ROOT, timeout=180, check=False)
    (OUT / (name + ".stdout")).write_bytes(result.stdout)
    (OUT / (name + ".stderr")).write_bytes(result.stderr)
    record["steps"].append({"name":name, "argv":args, "code":result.returncode,
                            "stdout_sha256":sha(result.stdout),
                            "stderr_sha256":sha(result.stderr),
                            "stderr_sample":result.stderr.decode("utf-8", "replace")[:320]})
    save()
    print(f"R542 {name} exit={result.returncode}", flush=True)
    if expect is not None:
        required(result.returncode == expect, name + " expected exit " + str(expect) +
                 " got " + str(result.returncode))
    return result
def observe(plain, array_type, option_type, symbol):
    """Only insert includes and one passive observer into original live function.
    Do NOT replace/rebuild/clone the C array or its initializer or producer.
    """
    code = plain.decode("utf-8")
    local = "nl_local_" + str(symbol)
    typ = "nl_option_" + str(option_type)
    arrayname = "nl_array_" + str(array_type)
    declaration = f"nl_array_{array_type} {local} = {{"
    required(code.count(declaration) == 1, "one exact original emitted C Array declaration")
    required(code.count("{ .tag = NL_ARRAY4_NONE }") == 4,
             "four complete source-emitted None representations")
    required(f"typedef {typ} {arrayname}[4];" in code,
             "original typed full-envelope C Array typedef")
    required("memset" not in code, "source C17 cannot synthesize None by memset")
    start = code.index("static void nl_fn_")
    marker = f"    (void){local};"
    required(code.count(marker) == 1 and code.index(marker) > start,
             "one original live-function observation point")
    added = f"""
    /* R542 passive observer: read only the ORIGINAL local before it leaves scope. */
    const unsigned char *const r542_first =
        (const unsigned char *)(const void *)&{local}[0];
    if ((const void *)&{local} != (const void *)&{local}[0] ||
        sizeof({local}) != 4u * sizeof({local}[0]) ||
        sizeof({local}[0]) != sizeof({typ})) abort();
    for (size_t r542_i = 0; r542_i < 4u; ++r542_i) {{
        const unsigned char *const r542_here =
            (const unsigned char *)(const void *)&{local}[r542_i];
        const ptrdiff_t r542_offset = r542_here - r542_first;
        if (r542_offset != (ptrdiff_t)(r542_i * sizeof({typ})) ||
            {local}[r542_i].tag != NL_ARRAY4_NONE) abort();
        printf("R542_PHYS index=%zu addr=%p offset=%td stride=%zu size=%zu tag=%u payload=%u\\n",
               r542_i, (const void *)&{local}[r542_i], r542_offset,
               sizeof({typ}), sizeof({local}),
               (unsigned){local}[r542_i].tag,
               (unsigned){local}[r542_i].payload);
    }}
"""
    headers = "#include <stdint.h>\n"
    required(code.count(headers) == 1, "original one stdint include")
    result = code.replace(headers, headers +
        "#include <stddef.h>\n#include <stdio.h>\n#include <stdlib.h>\n")
    result = result.replace(marker, added + marker)
    required(result.count("R542_PHYS index=") == 1 and
             result.count(declaration) == 1, "observer inserted once")
    (OUT / "passive_observer.unified.diff").write_text("".join(difflib.unified_diff(
        code.splitlines(keepends=True), result.splitlines(keepends=True),
        fromfile="original_emitted.c", tofile="observer_added.c")))
    return result.encode("utf-8")

try:
    oracle = json.loads((R/"r542/frozen_oracle.json").read_text())
    source = R / oracle["source"]
    required(sha(source) == oracle["source_sha256"], "before-run frozen source hash mismatch")
    required(oracle["authority"]["under_test"] == COMPILER_SHA, "wrong frozen authority")
    record["frozen_source_sha256"] = sha(source)
    record["frozen_oracle_sha256"] = sha(R/"r542/frozen_oracle.json")
    record["git_R_branch"] = command("r_head", ["git","-C",R,"rev-parse","HEAD"]).stdout.decode().strip()
    observed_sha = command("compiler_head", ["git","-C",CANDIDATE,"rev-parse","HEAD"]).stdout.decode().strip()
    required(observed_sha == COMPILER_SHA, "fixed candidate checkout mismatch")
    optin = CANDIDATE/"r542-build/newlangc_p534"
    normal = CANDIDATE/"r542-build/newlangc"
    required(optin.is_file() and normal.is_file(), "missing pinned execution binaries")
    record["binaries"] = {"optin": sha(optin), "default": sha(normal)}
    save()
    default = command("default_off", [normal, "--check", source], 3)
    required(b"P5-EXPRESSION-UNSUPPORTED" in default.stderr,
             "default source isolation parser failure mismatch")
    passed = command("source_semantic_check", [optin, "--check", source], 0)
    required(not passed.stdout and not passed.stderr, "positive CHECK non-silent")
    emitted = command("source_emitted_C17", [optin, source], 0)
    required(not emitted.stderr and bool(emitted.stdout), "no genuine C17 from original source")
    original_c = OUT/"original_emitted.c"
    original_c.write_bytes(emitted.stdout)
    record["original_emitted_c_sha256"] = sha(original_c)
    save()

    # Public auditor is compiled from independently authored r542 C against
    # the *documented* candidate public headers/library, not P probe executable.
    all_libs = sorted((CANDIDATE/"r542-build").glob("libnewlang_*.a"))
    required(len(all_libs) >= 7, "missing standalone public library linkage")
    audit_exe = OUT/"public_audit"
    cc_args = ["gcc","-std=c17","-Wall","-Wextra","-Wpedantic","-Werror",
               "-I"+str(CANDIDATE/"include"),
               "-I"+str(CANDIDATE/"src"),
               "-I"+str(CANDIDATE/"tests/support"),
               R/"r542/public_audit.c","-Wl,--start-group",
               *all_libs,"-Wl,--end-group","-o",audit_exe]
    command("compile_independent_public_audit",cc_args,0)
    public_emitted = OUT/"public_exported_original.c"
    proof = command("public_issued_mutant_check",
                    [audit_exe,source,public_emitted],0)
    required(not proof.stderr and b"PUBLIC_OK" in proof.stdout,
             "public view consumer cannot verify source-backed original")
    proof_text = proof.stdout.decode("utf-8")
    proof_line = [line for line in proof_text.splitlines() if line.startswith("PUBLIC_OK ")]
    required(len(proof_line) == 1, "one public report line")
    fields = dict(re.findall(r"([A-Za-z_]+)=([0-9]+)", proof_line[0]))
    for key, value in [("independent_roots",1),("live_places",5),
                       ("children",4),("source_AST_torn_down",1),("final_root_ended",1)]:
        required(int(fields.get(key,-1)) == value, "public field mismatch "+key)
    mutants = int(fields.get("mutants_rejected","0"))
    required(mutants >= 40, "insufficient independent tamper refusals")
    required(public_emitted.read_bytes() == original_c.read_bytes(),
             "public authenticated C17 output differs from original CLI C17")
    record["public"] = {"fields":fields, "mutants_rejected":mutants,
                        "exported_c_sha256":sha(public_emitted),
                        "published_readonly_validator":"invoked and passed; no seed"}
    save()

    tagged = re.search(rb"NL_ARRAY4_NONE = ([0-9]+)", emitted.stdout)
    required(tagged is not None, "compiler None tag absent")
    optiontype = int(fields["option_type"])
    arraytype = int(fields["array_type"])
    symbol = int(fields["symbol"])
    passive = observe(emitted.stdout,arraytype,optiontype,symbol)
    observed_file = OUT/"original_with_passive_observer.c"
    observed_file.write_bytes(passive)
    record["physical"] = {"None_tag_compiler_chosen":int(tagged.group(1)),
                          "full_Option_type":optiontype, "Array_type":arraytype,
                          "source_root_symbol":symbol,
                          "plain_C_sha256":sha(original_c),
                          "observer_C_sha256":sha(observed_file),
                          "insertion_diff_sha256":sha(OUT/"passive_observer.unified.diff"),
                          "observer_c_only_reads_original_local":True}
    save()

    for compiler in ("gcc","clang"):
        v = command(compiler+"_version",[compiler,"--version"],0)
        record.setdefault("cc_versions",{})[compiler] = v.stdout.decode("utf-8","replace").splitlines()[0]
        for mode in ("O0","O2"):
            for label,src in (("plain",original_c),("passive",observed_file)):
                name = compiler+"_"+mode+"_"+label
                binary = OUT/name
                command("build_"+name,[compiler,"-std=c17","-Wall","-Wextra",
                    "-Wpedantic","-Werror","-"+mode,src,"-o",binary],0)
                native = command("native_"+name,[binary],0)
                required(not native.stderr, name+" unexpected native stderr")
                if label == "plain":
                    required(not native.stdout,name+" unexpected output without observer")
                else:
                    lines = native.stdout.decode().splitlines()
                    required(len(lines)==4, name+" not four live child readings")
                    entries=[]
                    for i,line in enumerate(lines):
                        m = re.fullmatch(r"R542_PHYS index=(\d+) addr=(0x[0-9a-fA-F]+) "
                             r"offset=(\d+) stride=(\d+) size=(\d+) tag=(\d+) payload=(\d+)",line)
                        required(m is not None,name+" malformed passive measurement")
                        idx, address, offset, stride, size, tag, payload = (
                            int(m.group(1)),int(m.group(2),16),*map(int,m.groups()[2:]))
                        required(idx==i and offset==i*stride and
                                 address==entries[0]["address"]+i*stride if i else
                                 idx==i and offset==0,
                                 name+" noncontiguous original physical address")
                        required(size==4*stride and tag==int(tagged.group(1)),
                                 name+" None/full Option original physical mismatch")
                        entries.append({"index":idx, "address":address,
                                        "offset":offset,"stride":stride,
                                        "full_array_size":size,"tag":tag,
                                        "payload":payload})
                    record["physical"][name]=entries
                    save()
    record["result"] = "CALIBRATED LIMITED PASS — single source-authentic Array; R540-01 remains BLIND FINDING"
    record["out_of_scope"] = ("two-root canonical precision discrepancy uncorrected; "
        "P541 not read/validated; no general ABI, nonCopy owner/Ring proof or product GO")
    save()
    print("R542 CALIBRATED LIMITED PASS; R540-01 HISTORICAL BLIND FINDING UNCHANGED",flush=True)
except Exception as err:
    if record["result"] == "INCONCLUSIVE":
        record["result"] = "INCONCLUSIVE / HARNESS STOP"
        record["stop_reason"] = repr(err)
    save()
    print("R542 STOP:",repr(err),file=sys.stderr,flush=True)
    sys.exit(1)
