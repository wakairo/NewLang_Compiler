#!/usr/bin/env python3
"""Track R #230: independently designed source and native black-box corpus.
No P code, test, observer, report or previous R source is read.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

FROZEN = "a374df32dab3a619d40ac02b39cc1d07605c362f"
ROOT = Path(__file__).resolve().parents[1]
FROZEN_ROOT = Path(os.environ["R230_FROZEN_ROOT"]).resolve()
COMPILER = Path(os.environ["R230_COMPILER"]).resolve()
OUT = (ROOT / "r230" / "results").resolve()
OUT.mkdir(parents=True, exist_ok=True)
SPEC = FROZEN_ROOT / "docs/reference/NewLang_v0_spec_Draft17_29.md"
OBSERVER = ROOT / "r230" / "observer.c"
CC = os.environ.get("CC_NATIVE", "cc")


def digest(data):
    if isinstance(data, str):
        data = data.encode()
    return hashlib.sha256(data).hexdigest()


def write(name, content):
    path = OUT / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf8")
    return path


def run(argv, env=None, timeout=50):
    try:
        p = subprocess.run([str(x) for x in argv], capture_output=True,
                           text=True, errors="replace", env=env, timeout=timeout)
        return dict(exit=p.returncode, stdout=p.stdout, stderr=p.stderr)
    except subprocess.TimeoutExpired as ex:
        return dict(exit="TIMEOUT", stdout=str(ex.stdout), stderr=str(ex.stderr))
    except Exception as ex:
        return dict(exit="EXEC_ERROR", stdout="", stderr=repr(ex))


def change(text, old, new):
    if old not in text:
        raise AssertionError("Expected canonical source substring not found: " + old[:100])
    return text.replace(old, new, 1)


def make_corpus(seed):
    cases = {}
    cases["A00_canonical_accept"] = (seed, "accept", "full native")
    cases["A01_refusal"] = (
        change(seed, "let admit_flag = Option<ptr<Node>>::None;",
               "let admit_flag = Option<ptr<Node>>::Some(ptr_h);"), "accept", "full native")
    cases["A02_alpha_rename"] = (
        seed.replace("recipient_adopt", "adopter_x")
            .replace("custody", "vault_x")
            .replace("packet", "token_x"),
        "accept", "full native")
    cases["A03_producer_and_receiver_rename"] = (
        seed.replace("detach_and_return_tail", "unlink_and_handoff")
            .replace("receive_and_release_tail", "close_original_tail")
            .replace("admit_flag", "policy_choice"), "accept", "full native")
    cases["A04_reorder_copy_policy"] = (
        change(seed,
               "let custody = Option<LiveTail>::None;\n\n"
               "                    let admit_flag = Option<ptr<Node>>::None;",
               "let admit_flag = Option<ptr<Node>>::None;\n\n"
               "                    let custody = Option<LiveTail>::None;"),
        "accept", "full native")
    cases["A05_rename_struct"] = (
        seed.replace("Node", "Cell").replace("head_bundle", "first_bag")
            .replace("tail_bundle", "second_bag"), "accept", "full native")
    # Hostile source cases; rejections are expected before any generated C.
    cases["N00_wrong_tail_pointer"] = (
        change(seed, "head_w2@next, ptr_t, allocation_t, life_t)",
               "head_w2@next, ptr_h, allocation_t, life_t)"), "reject", "owner ptr differs")
    cases["N01_wrong_tail_allocation"] = (
        change(seed, "head_w2@next, ptr_t, allocation_t, life_t)",
               "head_w2@next, ptr_t, allocation_h, life_t)"), "reject", "A_h substituted for A_t")
    cases["N02_wrong_tail_domain"] = (
        change(seed, "head_w2@next, ptr_t, allocation_t, life_t)",
               "head_w2@next, ptr_t, allocation_t, life_h)"), "reject", "D_h substituted for D_t")
    cases["N03_readonly_custody"] = (
        change(seed, "loan_write(custody) { |sink|\n                                recipient_adopt",
               "loan_read(custody) { |sink|\n                                recipient_adopt"),
        "reject", "read scoped loan instead of write")
    cases["N04_store_nondiscardable"] = (
        change(seed, "let displaced = replace(\n        sink, Option<LiveTail>::Some(packet));",
               "let displaced = store(\n        sink, Option<LiveTail>::Some(packet));"),
        "reject", "drop of occupied or empty nonDiscardable sum")
    cases["N05_drop_displaced_none"] = (
        change(seed,
               "let displaced = replace(\n        sink, Option<LiveTail>::Some(packet));\n    match displaced {\n        None => { unit },\n    };",
               "replace(sink, Option<LiveTail>::Some(packet));"),
        "reject", "lost nonDiscardable old sum")
    cases["N06_callee_forgets_packet"] = (
        change(seed, "let displaced = replace(\n        sink, Option<LiveTail>::Some(packet));",
               "let displaced = Option<LiveTail>::None;"),
        "reject", "callee exits with owner live")
    cases["N07_double_consume"] = (
        change(seed,
               "recipient_adopt(sink, packet)\n                            };",
               "recipient_adopt(sink, packet);\n"
               "                                let copy_of_consumed = packet;\n"
               "                                unit\n                            };"),
        "reject", "original packet after call")
    cases["N08_wrong_receiver_domain"] = (
        change(seed,
               "owned_ptr, owned_allocation, owned_domain);\n                            unit\n                        },",
               "owned_ptr, owned_allocation, life_h);\n                            unit\n                        },"),
        "reject", "receiver wrong domain in refusal arm")
    cases["N09_double_terminal_consume"] = (
        change(seed,
               "receive_and_release_tail(\n                                owned_ptr, owned_allocation, owned_domain);\n                            unit",
               "receive_and_release_tail(\n                                owned_ptr, owned_allocation, owned_domain);\n"
               "                            receive_and_release_tail(\n"
               "                                owned_ptr, owned_allocation, owned_domain);\n                            unit"),
        "reject", "same original A/D reused")
    cases["N10_wrong_sink_current_some"] = (
        change(seed, "let custody = Option<LiveTail>::None;",
               "let custody = Option<LiveTail>::Some(packet);"),
        "reject", "sink already contains nonCopy owner")
    cases["N11_nonexhaustive_later_match"] = (
        change(seed,
               "match recovered {\n                        None => { unit },\n                        Some(saved) => {",
               "match recovered {\n                        Some(saved) => {"),
        "reject", "omitted None in recovered unknown")
    cases["N12_wildcard_owner_discard"] = (
        change(seed,
               "Some(saved) => {\n                            let LiveTail",
               "Some(_) => {\n                            let LiveTail"),
        "reject", "cannot discard nonCopy owned payload")
    cases["N13_wrong_proved_none_match"] = (
        change(seed,
               "match custody {\n                        None => { unit },\n                    };",
               "match recovered {\n                        None => { unit },\n                    };"),
        "reject", "consumed old sum and no current None proof")
    cases["N14_stale_original_packet"] = (
        change(seed,
               "let recovered = loan_write(custody) { |sink|",
               "let stolen_again = packet;\n"
               "                    let recovered = loan_write(custody) { |sink|"),
        "reject", "after conditional original packet move")
    cases["N15_illegal_callee_return_ref"] = (
        change(seed, ") -> unit {\n    let displaced = replace(",
               ") -> ref<write, Option<LiveTail>> {\n    let displaced = replace("),
        "reject", "scoped sink escaping function")
    cases["N16_double_adopt"] = (
        change(seed,
               "recipient_adopt(sink, packet)\n                            };",
               "recipient_adopt(sink, packet);\n"
               "                                recipient_adopt(sink, packet)\n                            };"),
        "reject", "two calls consume same owner and reuse occupied sink")
    return cases


def build_native(name, cpath, mode, instrument=False, sanitize=False):
    exe = OUT / "native" / (name + "_" + mode)
    exe.parent.mkdir(exist_ok=True)
    argv = [CC, "-std=c17", "-g", "-O0", "-fno-omit-frame-pointer",
            "-fno-pie", "-no-pie"]
    if sanitize:
        argv += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all"]
    if instrument:
        argv += ["-finstrument-functions", "-Wl,--wrap=malloc",
                 "-Wl,--wrap=calloc", "-Wl,--wrap=realloc", "-Wl,--wrap=free",
                 str(cpath), str(OBSERVER), "-ldl"]
    else:
        argv += [str(cpath)]
    argv += ["-o", str(exe)]
    res = run(argv, timeout=90)
    record = dict(argv=argv, **res)
    record["executable_sha256"] = digest(exe.read_bytes()) if exe.exists() else None
    record["executable"] = str(exe.relative_to(ROOT)) if exe.exists() else None
    return exe, record


def exercise(name, cpath, expected, collect):
    outputs = {}
    for mode, instrument, sanitize in [
        ("plain", False, False), ("observer", True, False),
        ("sanitized", True, True),
    ]:
        exe, comp = build_native(name, cpath, mode, instrument, sanitize)
        outputs[mode] = {"compilation": comp}
        if comp["exit"] != 0:
            continue
        failure_options = [0] if mode == "plain" else [0, 1, 2]
        for fail_at in failure_options:
            env = os.environ.copy()
            env["R230_FAIL_ALLOCATION_N"] = str(fail_at)
            env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
            p = run([exe], env=env, timeout=15)
            stdoutpath = write("native/logs/" + name+"_"+mode+"_fail"+str(fail_at)+".txt",
                              "exit: "+str(p["exit"])+"\nstdout:\n"+p["stdout"]+
                              "\nstderr:\n"+p["stderr"])
            outputs[mode]["fail"+str(fail_at)] = {
                "exit":p["exit"],
                "stdout":p["stdout"][-6000:],
                "stderr":p["stderr"][-18000:],
                "log":str(stdoutpath.relative_to(ROOT))
            }
    return outputs


def main():
    assert run(["git", "-C", FROZEN_ROOT, "rev-parse", "HEAD"])["stdout"].strip() == FROZEN
    spec = SPEC.read_text(encoding="utf8")
    section = spec.split("### 18.1c.4 Exact canonical source-shaped witness")[1]
    found = re.search(r"~~~newlang\n(.*?)\n~~~", section, re.S)
    if not found:
        raise RuntimeError("spec embedded witness not found")
    seed = "\n".join(line for line in found.group(1).splitlines()
                     if not line.lstrip().startswith("//")) + "\n"
    cases = make_corpus(seed)
    manifest = {}
    for name, (source, expectation, reason) in cases.items():
        path = write("sources/"+name+".nl", source)
        manifest[name] = {"sha256":digest(source), "expectation":expectation,
                          "reason":reason,"path":str(path.relative_to(ROOT))}
    write("source-manifest.json", json.dumps(manifest,indent=2,ensure_ascii=False))
    meta = {
        "frozen_commit":FROZEN,
        "frozen_head":run(["git","-C",FROZEN_ROOT,"rev-parse","HEAD"]),
        "review_head":run(["git","-C",ROOT,"rev-parse","HEAD"]),
        "spec_sha256":digest(spec),
        "compiler_sha256":digest(COMPILER.read_bytes()),
        "compiler_version":run([COMPILER,"--version"]),
        "compiler_file":run(["file",COMPILER]),
        "cc_version":run([CC,"--version"]),
        "uname":run(["uname","-a"]),
        "cases":{}
    }
    for i,(name,(src, expectation,reason)) in enumerate(cases.items()):
        source = OUT / "sources" / (name+".nl")
        p = run([COMPILER,source],timeout=45)
        case = {"expectation":expectation, "reason":reason,
                "compiler_exit":p["exit"],"diagnostics":p["stderr"][-12000:],
                "stdout_bytes":len(p["stdout"].encode()),
                "generated_sha256":digest(p["stdout"]) if p["stdout"] else None}
        # stdout may contain errors or non-C. Persist exact bytes for inspection.
        if p["stdout"]:
            c = write("generated/"+name+".c",p["stdout"])
            case["generated_file"]=str(c.relative_to(ROOT))
            if p["exit"] == 0 and expectation == "accept":
                case["native"] = exercise(name,c,expectation,meta)
        meta["cases"][name]=case
        write("result.json",json.dumps(meta,indent=2,ensure_ascii=False))
        print("CASE",name,"expect",expectation,"compiler_status",p["exit"],
              "generated_bytes",len(p["stdout"]),"sha",case["generated_sha256"],flush=True)
        if "native" in case:
            for mode,m in case["native"].items():
                print("  native",mode,"compile_status",m["compilation"]["exit"],flush=True)
                for k,v in m.items():
                    if k.startswith("fail"):
                        lines=[ln for ln in v["stderr"].splitlines()
                               if ln.startswith("R230 ") or "ERROR:" in ln or
                               "runtime error:" in ln]
                        print("   ",k,"exit",v["exit"],"trace",
                              " | ".join(lines[:24])[:2000],flush=True)
        elif p["stderr"]:
            print("  diagnostic:",p["stderr"][-700:].replace("\n"," "),flush=True)
    print("SUMMARY",json.dumps({
        "accepted_native_compiled":sum(
            1 for x in meta["cases"].values()
            if x.get("native",{}).get("plain",{}).get("compilation",{}).get("exit")==0),
        "expected_rejections_that_produced_C": [
            n for n,x in meta["cases"].items() if x["expectation"]=="reject"
            and x["compiler_exit"]==0 and x["stdout_bytes"]>0],
        "unexpected_positive_rejections":[
            n for n,x in meta["cases"].items() if x["expectation"]=="accept"
            and x["compiler_exit"]!=0]
    }),flush=True)


if __name__ == "__main__":
    main()
