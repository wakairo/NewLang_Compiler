#!/usr/bin/env python3
"""Track R #228, independent black-box corpus. Only normative Draft17.29 witness used."""
from pathlib import Path
import argparse, csv, hashlib, json, os, re, subprocess, sys

SPEC = Path("compiler/docs/reference/NewLang_v0_spec_Draft17_29.md")
OUT = Path("r228-evidence")
FIXED = "c2838d42f5bb4894ba431d48b25e00f0403b2ea7"

def sha(data):
    return hashlib.sha256(data).hexdigest()

def edit(s, old, new, *, count=1):
    n=s.count(old)
    if n!=count:
        raise ValueError(f"expected {count} occurrences, found {n}: {old[:100]!r}")
    return s.replace(old, new, count)

def cut(s, start, end, replacement):
    i=s.index(start)
    j=s.index(end,i)
    return s[:i]+replacement+s[j:]

def corpus():
    txt=SPEC.read_text(encoding="utf-8")
    part=txt.split("### 18.1c.4 Exact canonical source-shaped witness",1)[1].split("### 18.1c.5",1)[0]
    m=re.search(r"~~~newlang\s*\n(.*?)\n~~~",part,re.S)
    if not m: raise RuntimeError("normative witness not found")
    base=m.group(1).strip()+"\n"
    ans=[]
    def add(name, source, expectation, attack):
        ans.append((name,source,expectation,attack))
    add("00_canonical",base,"admit-semantic", "exact normative §18.1c.4 witness; original two-root custody")
    renamed=re.sub(r"\bpacket\b","transit_owner",base)
    renamed=re.sub(r"\bcustody\b","vault_local",renamed)
    add("01_rename_control",renamed,"admit-semantic","metamorphic alpha-renaming; no semantic change")
    add("02_refusal_control",edit(base,"let admit_flag = Option<ptr<Node>>::None;","let admit_flag = Option<ptr<Node>>::Some(ptr_h);"),"admit-semantic","Copy policy choice, refusal releases caller-owned tail")
    add("03_wrong_tail_pointer",edit(base,"head_w2@next, ptr_t, allocation_t, life_t)","head_w2@next, ptr_h, allocation_t, life_t)"),"reject","packet ptr points to O_h, but A_t/D_t target O_t (§18.1c.3)")
    add("04_wrong_tail_allocation",edit(base,"head_w2@next, ptr_t, allocation_t, life_t)","head_w2@next, ptr_t, allocation_h, life_t)"),"reject","packet claims head A_h and tail O_t/D_t; donor A_h duplicated")
    add("05_wrong_tail_domain",edit(base,"head_w2@next, ptr_t, allocation_t, life_t)","head_w2@next, ptr_t, allocation_t, life_h)"),"reject","cross-world D_h mislabeled as D_t and donor D_h consumed")
    add("06_double_adoption",edit(base,"recipient_adopt(sink, packet)\n", "recipient_adopt(sink, packet);\n                                recipient_adopt(sink, packet)\n"),"reject","same nonCopy packet consumed twice, second call also sink occupied")
    add("07_dropped_replace_old",cut(base,"    let displaced = replace(\n","    unit\n}\n\nfn receive_and_release_tail","    replace(sink, Option<LiveTail>::Some(packet));\n    unit\n}\n\n"),"reject","displace old nonDiscardable Option even when known None (§18.1c.2)")
    add("08_unconsumed_recipient",cut(base,"    let displaced = replace(\n","    unit\n}\n\nfn receive_and_release_tail","    unit\n}\n\n"),"reject","callee leaves its nonCopy packet Available on normal return")
    add("09_store_non_discardable",cut(base,"    let displaced = replace(\n","    unit\n}\n\nfn receive_and_release_tail","    store(sink, Option<LiveTail>::Some(packet));\n    unit\n}\n\n"),"reject","store cannot discard static nonDiscardable old Option")
    add("10_double_owner_custody",edit(base,"let custody = Option<LiveTail>::None;","let custody = Option<LiveTail>::Some(packet);"),"reject","preoccupied sink and packet consumed before recipient")
    add("11_read_only_sink",edit(base,"loan_write(custody) { |sink|\n                                recipient_adopt","loan_read(custody) { |sink|\n                                recipient_adopt"),"reject","read ref cannot satisfy ordinary write recipient formal")
    add("12_escape_scoped_sink",edit(base,"loan_write(custody) { |sink|\n                                recipient_adopt(sink, packet)\n                            };","let stolen_sink = loan_write(custody) { |sink| sink };\n                            recipient_adopt(stolen_sink, packet);"),"reject","scope-bound ref escapes into caller binding then reused")
    add("13_old_some_omitted",cut(base,"                    match recovered {\n","                    // custody itself is still nonDiscardable","                    match recovered {\n                        None => { unit },\n                    };\n\n"),"reject","one-arm consuming old match without known None: may hold original live owner")
    add("14_wildcard_owned_some",cut(base,"                    match recovered {\n","                    // custody itself is still nonDiscardable","                    match recovered {\n                        None => { unit },\n                        Some(_) => { unit },\n                    };\n\n"),"reject","Some(_) would discard nonDiscardable tail ownership")
    add("15_custody_early_consume",edit(base,"let recovered = loan_write(custody) { |sink|","match custody { None => { unit }, };\n                    let recovered = loan_write(custody) { |sink|"),"reject","use consumed sum after premature known-None match and potential Some")
    add("16_custody_not_consumed",edit(base,"match custody {\n                        None => { unit },\n                    };","unit;"),"reject","nonDiscardable caller custody never consumed")
    add("17_packet_post_call_reuse",edit(base,"recipient_adopt(sink, packet)\n","recipient_adopt(sink, packet);\n                                packet\n"),"reject","reuses moved nonCopy packet after direct call")
    add("18_donor_allocation_reuse",edit(base,"let custody = Option<LiveTail>::None;","allocation_t;\n                    let custody = Option<LiveTail>::None;"),"reject","donor consumes A_t into producer then accesses old A_t")
    add("19_wrong_terminal_pointer",edit(base,"receive_and_release_tail(\n                                owned_ptr, owned_allocation, owned_domain);","receive_and_release_tail(\n                                ptr_h, owned_allocation, owned_domain);"),"reject","refusal branch tries to free O_h using tail A_t/D_t")
    add("20_double_terminal_release",edit(base,"receive_and_release_tail(\n                                owned_ptr, owned_allocation, owned_domain);","receive_and_release_tail(\n                                owned_ptr, owned_allocation, owned_domain);\n                            receive_and_release_tail(\n                                owned_ptr, owned_allocation, owned_domain);"),"reject","duplicate terminal call after consuming unique A_t/D_t")
    add("21_alias_extra_take",edit(base,"                    let recovered = loan_write(custody) { |sink|","                    loan_write(custody) { |alias|\n                        replace(alias, Option<LiveTail>::None)\n                    };\n                    let recovered = loan_write(custody) { |sink|"),"reject","displaced live original Some discarded through second mutable alias")
    add("22_branch_failure_owner_mix",edit(edit(base,"head_w2@next, ptr_t, allocation_t, life_t)","head_w2@next, ptr_t, allocation_t, life_h)"),"let admit_flag = Option<ptr<Node>>::None;","let admit_flag = Option<ptr<Node>>::Some(ptr_h);"),"reject","combine wrong producer D and refusal path; no valid tail D_t")
    add("23_two_arm_old_none_corruption",edit(base,"match displaced {\n        None => { unit },\n    };","match displaced {\n        Some(saved) => { unit },\n    };"),"reject","single-arm purported Some cannot consume exact-None displaced sum")
    add("24_post_consume_policy_fail",edit(base,"recipient_adopt(sink, packet)\n","recipient_adopt(sink, packet);\n                                receive_and_release_tail(ptr_t, allocation_t, life_t)\n"),"reject","post-consume attempted caller tail free after transfer")
    add("25_combined_scope_alias",edit(edit(base,"loan_write(custody) { |sink|\n                                recipient_adopt(sink, packet)\n                            };","let leaked = loan_write(custody) { |sink| sink };\n                            recipient_adopt(leaked, packet);"),"match recovered {\n","match recovered {\n"),"reject","scoped ref leak plus uncertain caller state")
    return ans

def generate():
    OUT.mkdir(exist_ok=True)
    rows=[]
    for name,source,expected,attack in corpus():
        b=source.encode()
        p=OUT/(name+".nl")
        p.write_bytes(b)
        rows.append({"name":name,"sha256":sha(b),"expected":expected,"attack":attack})
    (OUT/"manifest.json").write_text(json.dumps({"fixed":FIXED,"cases":rows},indent=2)+"\n")
    print("R228_CORPUS_CASES",len(rows),flush=True)
    for v in rows: print("R228_CASE",v["name"],v["sha256"],v["expected"],flush=True)

def run(executable):
    exe=Path(executable).resolve()
    m=json.loads((OUT/"manifest.json").read_text())
    out=[]
    for v in m["cases"]:
        name=v["name"]
        p=OUT/(name+".nl")
        res=subprocess.run([str(exe),str(p)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=60)
        ob=res.stdout
        eb=res.stderr
        (OUT/(name+".stdout")).write_bytes(ob)
        (OUT/(name+".stderr")).write_bytes(eb)
        obj={**v,"exit":res.returncode,"stdout_bytes":len(ob),"stderr_bytes":len(eb),"emitted_c":bool(b"#include" in ob or b"int main(" in ob),"diagnostics":eb.decode("utf8","replace")[:1800]}
        out.append(obj)
        print("R228_RESULT",name,"exit="+str(res.returncode),"stdout_bytes="+str(len(ob)),"C="+str(obj["emitted_c"]),"stderr="+repr(obj["diagnostics"][:400]),flush=True)
    (OUT/"results.json").write_text(json.dumps({"fixed":FIXED,"result":out},indent=2)+"\n")
    with (OUT/"results.csv").open("w",newline="") as f:
        writer=csv.DictWriter(f,fieldnames=["name","sha256","expected","exit","stdout_bytes","stderr_bytes","emitted_c","diagnostics","attack"])
        writer.writeheader()
        writer.writerows(out)
    anomalies=[v for v in out if (v["expected"]=="reject" and v["exit"] in (0,4)) or (v["expected"]=="admit-semantic" and v["exit"] not in (0,4))]
    print("R228_ANOMALIES",json.dumps([v["name"] for v in anomalies]),flush=True)
    print("R228_DONE",len(out),flush=True)

if __name__=="__main__":
    ap=argparse.ArgumentParser()
    ap.add_argument("mode",choices=["generate","run"])
    ap.add_argument("compiler",nargs="?")
    args=ap.parse_args()
    if args.mode=="generate":generate()
    else:run(args.compiler)
