#!/usr/bin/env python3
"""Track R #254: independent post-first-pass source transformations from canonical Draft.
Not a production test, not a native-C claim. Zero changes to checker or fixture.
"""
import base64
import hashlib
import json
import pathlib
import re
import subprocess
import sys

PIN = "d8765a5ae11919d3076cec0d1432973a0f18fdcd"
root = pathlib.Path(__file__).resolve().parents[1]
out = root / "r254_second_pass" / "evidence"
out.mkdir(parents=True, exist_ok=True)
spec = (root / "docs/reference/NewLang_v0_spec_Draft17_30.md").read_text()
part = spec.split("### 3.2b.3 Exact five-site full source-shaped positive witness", 1)[1]
canonical = part.split("~~~newlang\n", 1)[1].split("~~~", 1)[0]
base = "".join(line for line in canonical.splitlines(keepends=True)
               if not line.lstrip().startswith("//"))
assert base.count("try_allocate_one<Node>()") == 5
assert base.count("replace(") == 6

def exact(s, old, new, n=1):
    if s.count(old) < n:
        raise AssertionError(("missing match", old, s.count(old), n))
    return s.replace(old, new, n)

names = ["src_child", "A_prev", "A_next", "B_prev", "B_next", "C_prev"]
begin = base.rfind("\n", 0, base.index("let old_src_child =")) + 1
finish = base.rfind("\n", 0, base.index("let empty_dst =")) + 1
chunk = base[begin:finish]
matches = list(re.finditer(r"(?m)^([ ]+)let old_(src_child|A_prev|A_next|B_prev|B_next|C_prev) =", chunk))
assert [m.group(2) for m in matches] == names
wires = {m.group(2): chunk[m.start(): matches[i+1].start() if i+1<len(matches) else len(chunk)]
         for i,m in enumerate(matches)}
indent = matches[0].group(1)

def wiring(order=names, altered=None, prefix=""):
    altered = altered or {}
    assert set(order) == set(names)
    text = prefix + "".join(altered.get(x, wires[x]) for x in order)
    return base[:begin] + text + base[finish:]

def wire_replace(field, old, new):
    return exact(wires[field], old, new)

# Preserve five original sites for ALL cases; branch/world controls only mutate cleanup.
def fail_span(s, site):
    assert site in (2,3,4,5)
    ix = -1
    for _ in range(site):
        ix = s.index("match try_allocate_one<Node>()", ix+1)
    start = s.index("None => {", ix)
    end = s.index("Some(bundle_",start)
    return start, end

def fail_change(site, change):
    a,b=fail_span(base,site)
    return base[:a]+change(base[a:b])+base[b:]

def cleanup_chunks(arm, order):
    found={}
    for name in ("src","A","B","C"):
        marker=f"let empty_{name} = loan_exclusive_read(life_{name})"
        if marker not in arm:
            continue
        start=arm.rfind("\n",0,arm.index(marker))+1
        token=f"deallocate(allocation_{name}, full_{name});"
        end=arm.index("\n",arm.index(token))+1
        found[name]=(start,end,arm[start:end])
    expected=set(order)
    assert set(found)==expected, (set(found),expected)
    first=min(x[0] for x in found.values())
    last=max(x[1] for x in found.values())
    assert sorted((v[0],v[1]) for v in found.values()) == [(a,b) for a,b in sorted((v[0],v[1]) for v in found.values())]
    return arm[:first]+"".join(found[n][2] for n in order)+arm[last:]

cases=[]
def add(name, family, cause, source, expected, kind):
    assert source.count("try_allocate_one<Node>()")==5,(name,"sites")
    assert source!=base,(name,"not new")
    cases.append(dict(name=name,family=family,cause=cause,expected=expected,kind=kind,source=source))

# A: cross-root ordering and live same-root sibling interactions, not isolated swaps.
add("A01_six_link_long_permutation","A","Reorder six disjoint field changes across five live roots",wiring(["C_prev","B_next","A_next","src_child","B_prev","A_prev"]),"admit","core-positive")
alias_intro=indent+"let saved_B = ptr_B;\n"+indent+"let saved_C = ptr_C;\n"
add("A02_alias_interleaved_reorder","A","Interleave six writes with two independent Copy locators",
    wiring(["A_next","C_prev","B_prev","src_child","A_prev","B_next"],
       {"A_next":wire_replace("A_next","Some(ptr_B)","Some(saved_B)"),
        "B_next":wire_replace("B_next","Some(ptr_C)","Some(saved_C)")},alias_intro),
    "admit","core-positive")
merged_A=indent+"""let old_A_prev = loan_read(life_A) { |stable_A_joint|
"""+indent+"""    let both_A = ref_from_ptr(write, ptr_A, stable_A_joint);
"""+indent+"""    let first_A = replace(both_A@prev, Option<ptr<Node>>::Some(ptr_C));
"""+indent+"""    let after_prev_A = read(both_A@prev);
"""+indent+"""    replace(both_A@next, Option<ptr<Node>>::Some(ptr_B))
"""+indent+"""};\n"""
add("A03_same_loan_two_A_siblings","A","Same scoped write root: Change(prev), Copy read(prev), Change(next)",
    wiring(altered={"A_prev":merged_A,"A_next":""}),"admit","source-precision")
merged_B=indent+"""let old_B_prev = loan_read(life_B) { |stable_B_joint|
"""+indent+"""    let both_B = ref_from_ptr(write, ptr_B, stable_B_joint);
"""+indent+"""    let first_B = replace(both_B@prev, Option<ptr<Node>>::Some(ptr_A));
"""+indent+"""    let after_prev_B = read(both_B@prev);
"""+indent+"""    replace(both_B@next, Option<ptr<Node>>::Some(ptr_C))
"""+indent+"""};\n"""
add("A04_same_loan_two_B_siblings","A","Same B ref: read(prev) between sibling writes",
    wiring(altered={"B_prev":merged_B,"B_next":""}),"admit","source-precision")
s=wires["A_next"]
s=exact(s,"replace(w_A_next@next,", "let before_sibling = read(w_A_next@prev);\n"+indent+"    let result_A_next = replace(w_A_next@next,")
s=exact(s,"Option<ptr<Node>>::Some(ptr_B))","Option<ptr<Node>>::Some(ptr_B));\n"+indent+"    let after_sibling = read(w_A_next@prev);\n"+indent+"    result_A_next")
add("A05_prev_read_survives_next_change","A","Read A.prev immediately before/after A.next Change, same H ref",wiring(altered={"A_next":s}),"admit","source-precision")
s=wires["B_next"]
s=exact(s,"replace(w_B_next@next,","let earlier_B_prev = read(w_B_next@prev);\n"+indent+"    let old_B_next_value = replace(w_B_next@next,")
s=exact(s,"Option<ptr<Node>>::Some(ptr_C))","Option<ptr<Node>>::Some(ptr_C));\n"+indent+"    let later_B_prev = read(w_B_next@prev);\n"+indent+"    old_B_next_value")
add("A06_prev_B_copy_across_next","A","Copied field Option before/after B.next reset",wiring(altered={"B_next":s}),"admit","source-precision")

# B: same-place Reset, including same-address and None transitions; no 7th Change demanded.
add("B01_A_prev_some_to_same","B","A.prev Some(C)->Some(C) with identical pointer; no change of original owner",
    wiring(altered={"A_next":wire_replace("A_next","w_A_next@next,","w_A_next@prev,").replace("Some(ptr_B)","Some(ptr_C)")}),
    "admit","source-precision")
add("B02_A_prev_some_to_none","B","A.prev Some(C)->None while A.next stays empty",
    wiring(altered={"A_next":wire_replace("A_next","w_A_next@next,","w_A_next@prev,").replace("Some(ptr_B)","None")}),
    "admit","source-precision")
add("B03_B_prev_some_to_same","B","B.prev Some(A)->Some(A) repeated value",
    wiring(altered={"B_next":wire_replace("B_next","w_B_next@next,","w_B_next@prev,").replace("Some(ptr_C)","Some(ptr_A)")}),
    "admit","source-precision")
add("B04_B_prev_some_to_none","B","B.prev Some(A)->None with distinct B.next current unaffected",
    wiring(altered={"B_next":wire_replace("B_next","w_B_next@next,","w_B_next@prev,").replace("Some(ptr_C)","None")}),
    "admit","source-precision")
b_again=wire_replace("B_prev","life_B","life_A")
b_again=exact(b_again,"ptr_B, stable_B_prev","ptr_A, stable_B_prev")
b_again=exact(b_again,"Some(ptr_A)","Some(ptr_C)")
add("B05_A_prev_some_none_same","B","A.prev Some(C)->None->Some(C), relocate B.prev write to A.prev; six writes only",
    wiring(altered={"A_next":wire_replace("A_next","w_A_next@next,","w_A_next@prev,").replace("Some(ptr_B)","None"),
                    "B_prev":b_again}),
    "admit","source-precision")
s=wires["A_next"]
s=exact(s,"replace(w_A_next@next,","let before_reset = read(w_A_next@prev);\n"+indent+"    let old_copy = replace(w_A_next@prev,")
s=exact(s,"Some(ptr_B))","Some(ptr_C));\n"+indent+"    before_reset;\n"+indent+"    old_copy")
add("B06_copy_old_option_across_reset","B","Copy whole old Option before same-pointer re-store, not an occurrence ref",
    wiring(altered={"A_next":s}),"admit","source-precision")
s=indent+"""let old_A_next = loan_read(life_A) { |stable_A_occ|
"""+indent+"""    let root_A_occ = ref_from_ptr(write, ptr_A, stable_A_occ);
"""+indent+"""    match root_A_occ@prev {
"""+indent+"""        None => { unit },
"""+indent+"""        Some(old_payload_ref) => {
"""+indent+"""            let old_some = replace(root_A_occ@prev, Option<ptr<Node>>::Some(ptr_C));
"""+indent+"""            old_payload_ref;
"""+indent+"""            unit
"""+indent+"""        },
"""+indent+"""    }
"""+indent+"""};\n"""
add("B07_borrowed_payload_old_after_reset","B","Keep borrowed occurrence-dependent payload ref alive through same-address Reset",
    wiring(altered={"A_next":s}),"reject","semantic-or-unsupported")
s=exact(s,"replace(root_A_occ@prev","replace(root_A_occ@next")
add("B08_borrowed_payload_sibling_reset","B","Borrowed A.prev payload ref survives independent A.next Change (should be safe if source supported)",
    wiring(altered={"A_next":s}),"admit","source-precision")

# C: per-world independent original A/D disposition, with in-arm alternates.
add("C01_fourth_none_reorder_cleanup","C","C-None cleanup: src, B, A independent original releases",
    fail_change(4,lambda arm: cleanup_chunks(arm,["src","B","A"])),"admit","core-positive")
add("C02_fifth_none_reorder_cleanup","C","dst-None cleanup: A, src, C, B original releases",
    fail_change(5,lambda arm: cleanup_chunks(arm,["A","src","C","B"])),"admit","core-positive")
add("C03_fifth_none_second_reorder","C","dst-None cleanup B,A,C,src with distinct loan endings",
    fail_change(5,lambda arm: cleanup_chunks(arm,["B","A","C","src"])),"admit","core-positive")
add("C04_fourth_none_partial_B_receipt","C","Destroy/finalize B but strand original allocation_B only in fourth None world",
    fail_change(4,lambda arm:exact(arm,"deallocate(allocation_B, full_B);","unit;")),"reject","semantic")
add("C05_fifth_none_cross_B_C_receipt","C","C's recovered full Storage paired with B's original Allocation on fifth None",
    fail_change(5,lambda arm:exact(arm,"deallocate(allocation_C, full_C)","deallocate(allocation_B, full_C)")),
    "reject","semantic")
def add_token(arm):
    arm=exact(arm,"let empty_B = loan_exclusive_read", "let copied_B_before_end = ptr_B;\n"+indent+"let empty_B = loan_exclusive_read")
    return exact(arm,"unit\n", "copied_B_before_end;\n"+indent+"unit\n")
add("C06_fourth_none_dangling_copy_allowed","C","Copy B locator before B EndRoot, observe token only after all frees",
    fail_change(4,add_token),"admit","source-precision")
def stale_token(arm):
    arm=exact(arm,"let empty_B = loan_exclusive_read", "let token_B = ptr_B;\n"+indent+"let empty_B = loan_exclusive_read")
    after="deallocate(allocation_B, full_B);"
    return exact(arm,after,after+"\n"+indent+"let forbidden = loan_read(life_B) { |stable_B_dead| let rr_B = ref_from_ptr(read, token_B, stable_B_dead); read(rr_B@next) };")
add("C07_fourth_none_reborrow_freed_B","C","EndRoot/dealloc B, then attempt fresh scoped loan from copied locator",
    fail_change(4,stale_token),"reject","semantic")

# D: same nominal type is NOT a license to alias separate governing domains.
a_scope=wire_replace("A_prev","replace(w_A_prev@prev,",
    "let invalid_EndRoot = loan_exclusive_read(life_A) { |ending_A_inner| destroy(ptr_A, ending_A_inner) };\n"+indent+"    replace(w_A_prev@prev,")
add("D01_end_A_inside_its_field_loan","D","EndRoot A during active A.prev stability loan, then write sibling A.prev",
    wiring(altered={"A_prev":a_scope}),"reject","semantic")
b_scope=wire_replace("B_prev","replace(w_B_prev@prev,",
    "let invalid_EndRoot = loan_exclusive_read(life_B) { |ending_B_inner| destroy(ptr_B, ending_B_inner) };\n"+indent+"    replace(w_B_prev@prev,")
add("D02_end_B_inside_field_loan","D","EndRoot B while scoped B.prev ref / domain loan active",
    wiring(altered={"B_prev":b_scope}),"reject","semantic")
wrong=wire_replace("C_prev","ptr_C, stable_C_prev","ptr_A, stable_C_prev")
add("D03_cross_world_wrong_domain_in_C_projection","D","C-scoped D reloan used to write A.prev-like H of same nominal type",
    wiring(altered={"C_prev":wrong}),"reject","semantic")
wrong=wire_replace("B_next","ptr_B, stable_B_next","ptr_C, stable_B_next")
add("D04_cross_world_wrong_domain_in_B_projection","D","B scoped D applied to different C H with identical static type",
    wiring(altered={"B_next":wrong}),"reject","semantic")
add("D05_original_free_order_amid_copied_links","D","Destroy all original roots in nonreverse order while link pointers still stored",
    exact(base,base[base.rfind("\n",0,base.index("let empty_dst ="))+1:base.rfind("\n",0,base.index("unit\n",base.index("let empty_dst =")))+1],
          base[base.rfind("\n",0,base.index("let empty_dst ="))+1:base.rfind("\n",0,base.index("unit\n",base.index("let empty_dst =")))+1]) if False else
    # Swap dst and C two full teardown groups in the successful arm.
    (lambda txt: (lambda p,q,r: txt[:p]+txt[q:r]+txt[p:q]+txt[r:])(
      txt.rfind("\n",0,txt.index("let empty_dst ="))+1,
      txt.rfind("\n",0,txt.index("let empty_C =",txt.index("let empty_dst =")))+1,
      txt.rfind("\n",0,txt.index("let empty_B =",txt.index("let empty_dst =")))+1))(base),
    "admit","core-positive")

assert len(cases)>=16 and len({c["name"] for c in cases})==len(cases)
assert {c["family"] for c in cases}==set("ABCD")
compiler = pathlib.Path(sys.argv[1]).resolve()
results = []
sources = out/"sources"
sources.mkdir(exist_ok=True)
for item in cases:
    src = item.pop("source")
    name = item["name"]
    p = sources/(name+".nl")
    p.write_text(src)
    expected = item["expected"]
    before = {str(q.relative_to(sources)) for q in sources.iterdir()}
    argv=[str(compiler),str(p.resolve())]
    run=subprocess.run(argv,cwd=sources,capture_output=True)
    after = {str(q.relative_to(sources)) for q in sources.iterdir()}
    code=run.returncode
    stderr=run.stderr
    record=dict(item,source_path=str(p.relative_to(root)),
        source_sha256=hashlib.sha256(src.encode()).hexdigest(),sites=src.count("try_allocate_one<Node>()"),
        changes=src.count("replace("),argv=argv,exit=code,
        stdout_bytes=len(run.stdout),stderr_bytes=len(stderr),
        stdout_b64=base64.b64encode(run.stdout).decode(),
        stderr_b64=base64.b64encode(stderr).decode(),
        diagnostic_tags=re.findall(r"\[([A-Z0-9-]+)\]",stderr.decode("utf-8","replace")),
        generated_C=run.stdout.startswith(b"/*") or b"#include" in run.stdout or b"int main" in run.stdout,
        output_files=sorted(after-before))
    record["observed"]= "source-admitted/no-native" if code==4 and b"V1-BACKEND-UNSUPPORTED" in stderr \
         else ("checker-rejected" if code==3 else "unexpected/status-"+str(code))
    results.append(record)
    print(name, "exit", code, "tag",record["diagnostic_tags"], "C",record["generated_C"],flush=True)
summary=dict(frozen_compiler=PIN, canonical_spec="Draft17.30",source_origin="canonical 3.2b.3 text",
             cases=len(results), counts={str(code):sum(x["exit"]==code for x in results) for code in sorted({x["exit"] for x in results})},
             unexpected=[r["name"] for r in results if r["observed"].startswith("unexpected") or r["generated_C"] or r["output_files"]],
             results=results)
(out/"manifest.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False)+"\n")
(out/"quick.tsv").write_text("name\tfamily\texpected\texit\ttags\tsha256\n"+
    "".join("\t".join(map(str,[r["name"],r["family"],r["expected"],r["exit"],",".join(r["diagnostic_tags"]),r["source_sha256"]]))+"\n" for r in results))
print(json.dumps({"cases":len(cases),"counts":summary["counts"],"unexpected":summary["unexpected"]}))
