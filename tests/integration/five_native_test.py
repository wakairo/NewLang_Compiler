"""Real production source→C17→native, independent checked IDs and read-only oracle.
Guest NULL injection is separate from compiler OOM. Mutants stop via observer
exit 77 BEFORE invalid free/dereference; sanitizer-after-UB is never success.
"""
import hashlib
import json
import os
import pathlib
import re
import subprocess
import sys
import tempfile

compiler, probe, cc, fixture, sanitizer = sys.argv[1:]
support = pathlib.Path(__file__).resolve().parent
repo = support.parents[1]
source = pathlib.Path(fixture).read_text()
assert hashlib.sha256(source.encode()).hexdigest() == "812042833072145f761ddc21cbf774d81b64da3a34f6d466ba7db01a6d704d7d"
records = []
sources = {}

def run(args, **kw):
    return subprocess.run(args, capture_output=True, text=True, **kw)

def block_end(text, start):
    depth = 0
    for i in range(start, len(text)):
        depth += (text[i] == "{") - (text[i] == "}")
        if depth == 0:
            return i
    raise AssertionError("unclosed block")

def swap(text, site):
    start = -1
    for _ in range(site):
        start = text.index("match try_allocate_one<Node>()", start + 1)
    a = text.index("None =>", start)
    ae = text.index("\n", block_end(text, text.index("{", a))) + 1
    b = text.index("Some(bundle_", ae)
    be = text.index("\n", block_end(text, text.index("{", b))) + 1
    a = text.rfind("\n", 0, a) + 1
    b = text.rfind("\n", 0, b) + 1
    return text[:a] + text[b:be] + text[a:ae] + text[be:]

def native(root, name, code, observed=True, mutant=False, fail=None, platform=True):
    path, exe = root / (name + ".c"), root / name
    path.write_text(code)
    cmd = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-DNDEBUG",
           str(path), "-o", str(exe), "-fno-builtin-malloc", "-fno-builtin-free"]
    if platform:
        cmd += [str(support / "five_platform.c"), "-Wl,--wrap=malloc", "-Wl,--wrap=free"]
    if observed:
        cmd += ["-I" + str(root), '-DNEWLANG_FIVE_OBSERVER="' + str(support / "five_observer.h") + '"']
    else:
        cmd += ["-DNEWLANG_UNOBSERVED"]
    if sanitizer != "none":
        cmd += ["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    built = run(cmd)
    assert built.returncode == 0, (name, built.stderr)
    outcomes = [fail] if mutant or not platform else ["1", "2", "3", "4", "5", None]
    for outcome in outcomes:
        env = dict(os.environ)
        env.pop("NEWLANG_TEST_FAIL_CALL", None)
        if outcome:
            env["NEWLANG_TEST_FAIL_CALL"] = outcome
        executed = run([str(exe)], env=env)
        if mutant:
            assert executed.returncode == 77 and "FIVE OBSERVER FAIL" in executed.stderr, (name, executed)
            assert "Sanitizer" not in executed.stderr and "runtime error:" not in executed.stderr, (name, executed.stderr)
        else:
            assert executed.returncode == 0 and not executed.stderr, (name, outcome, executed)
            if observed:
                summary = executed.stdout.splitlines()[-1]
                m = re.fullmatch(r"OBSERVED trials=(\d+) allocations=(\d+) frees=(\d+) changes=(\d+) addresses=([0-9a-f,]+) order=([0-9a-f,]*)", summary)
                assert m, executed.stdout
                live = int(outcome)-1 if outcome else 5
                assert tuple(map(int,m.groups()[:4])) == (live+1 if outcome else 5, live, live, 6 if live==5 else 0)
                addresses = [int(v,16) for v in m[5].split(",")]
                order = [int(v,16) for v in m[6].split(",")] if m[6] else []
                assert len(addresses)==5 and len(set(addresses[:live]))==live
                assert all(addresses[:live]) and not any(addresses[live:]) and order==list(reversed(addresses[:live]))
                links = [row for row in executed.stdout.splitlines() if row.startswith("LINK ")]
                assert len(links)==(15 if live==5 else 0)
                print(name, outcome or "success", summary)
        records.append({"input":name,"outcome":outcome or "all-Some","observed":observed,
                        "mutant":mutant,"sanitizer":sanitizer,"c_sha256":hashlib.sha256(code.encode()).hexdigest(),
                        "binary_sha256":hashlib.sha256(exe.read_bytes()).hexdigest(),
                        "exit":executed.returncode,"stdout":executed.stdout,"stderr":executed.stderr})

def prepare(root, name, text):
    path = root / (name + ".nl")
    path.write_text(text)
    checked = run([probe,str(path)])
    assert checked.returncode==0, (name,checked.stderr)
    rows = {}
    for row in checked.stdout.splitlines():
        kind, *values = row.split()
        rows.setdefault(kind,[]).append(list(map(int,values)))
    assert {k:len(v) for k,v in rows.items()}=={"INIT":5,"ROOT":6,"FIELD":6,"CHANGE":6}
    assert all(len({v[i] for v in rows["INIT"]})==5 for i in range(4))
    header = "".join("static const size_t expected_"+k.lower()+"["+str(len(v))+"]["+str(len(v[0]))+"]={"+
                     ",".join("{"+",".join(map(str,row))+"}" for row in v)+"};\n" for k,v in rows.items())
    (root/"five_expect.h").write_text(header)
    emitted,retry = run([compiler,str(path)]),run([compiler,str(path)])
    assert emitted.returncode==retry.returncode==0 and emitted.stdout==retry.stdout and not emitted.stderr, (name,emitted.stderr)
    code=emitted.stdout
    assert code.count("=malloc(56);")==5 and len(re.findall(r"^\*nl_v_\d+=nl_v_\d+;",code,re.M))==6
    # Source operands remain separate actual pointer/domain carriers, not
    # constants synthesized by C. Checked IDs are independent probe inputs.
    calls=re.findall(r"NL_FIVE_ROOT\(nl_v_(\d+),nl_v_(\d+)->token,(\d+),(\d+),(\d+),(\d+)\)",code)
    assert len(calls)==6
    for row,call in zip(rows["ROOT"],calls):
        p,s,r,i,d,scope,write=row
        pv,sv,w,cr,ci,cs=map(int,call)
        assert (w,cr,ci,cs)==(write,r,i,scope)
        assert re.search(r"nl_v_"+str(pv)+r"=nl_b_\d+_"+str(p)+";",code)
        assert re.search(r"nl_v_"+str(sv)+r"=nl_b_\d+_"+str(s)+";",code)
    sources[name]={"source_sha256":hashlib.sha256(text.encode()).hexdigest(),
                   "c_sha256":hashlib.sha256(code.encode()).hexdigest(),"checked_expectations":rows}
    return code

draft=(repo/"docs/reference/NewLang_v0_spec_Draft17_30.md").read_text()
canonical=draft.split("### 3.2b.3 Exact five-site full source-shaped positive witness\n",1)[1].split("~~~newlang\n",1)[1].split("~~~",1)[0]
def construction_permutation(text, order):
    pattern=r"( +)(next: Option<ptr<Node>>::None,)\n\1(prev: Option<ptr<Node>>::None,)\n\1(child: Option<ptr<Node>>::None,)"
    changed,count=re.subn(pattern,lambda m:"\n".join(m[1]+m[i+2] for i in order),text)
    assert count==5
    return changed

variants={"canonical":source,
          "alpha_inner_permutation":construction_permutation(swap(source,3),(1,0,2)).replace("Node","Cell").replace("ptr_","pointer_").replace("life_","domain_"),
          "outer_permutation":construction_permutation(swap(swap(source,1),5),(2,0,1)),
          "canonical_comments":canonical}
with tempfile.TemporaryDirectory(prefix="five-native-") as directory:
    root=pathlib.Path(directory)
    codes={}
    for name,text in variants.items():
        code=prepare(root,name,text)
        codes[name]=code
        native(root,name,code)
        native(root,name+"_unobserved",code,observed=False)
    assert codes["canonical"]!=codes["alpha_inner_permutation"] and codes["canonical"]!=codes["outer_permutation"]
    code=prepare(root,"canonical",source)
    native(root,"canonical_platform_free",code,observed=False,platform=False)
    writes=re.findall(r"^\*nl_v_\d+=nl_v_\d+;",code,re.M)
    frees=re.findall(r"free\((nl_v_\d+\.handle)\);",code)
    first_field=re.search(r"(nl_v_\d+)=&(nl_v_\d+)->f2;",code)
    first_some=re.search(r"nl_option (nl_v_\d+)=\{1,(nl_v_\d+)\};",code)
    root_hook=re.search(r"NL_FIVE_ROOT\(nl_v_\d+,(nl_v_\d+)->token",code)
    end_hook=re.search(r"NL_FIVE_END\((nl_v_\d+),",code)
    trial5=re.findall(r"NL_FIVE_TRIAL\(4,(nl_heap_\d+),56,8\);",code)[0]
    first_heap=re.search(r"void \*(nl_heap_\d+)=malloc\(56\);",code)[1]
    mutations={
        "wrong_field":(code.replace(first_field[0],first_field[0].replace("->f2","->f0"),1),None),
        "missing_write":(code.replace(writes[0],"(void)"+writes[0].split("=")[1],1),None),
        "wrong_tag":(code.replace(first_some[0],first_some[0]+f" {first_some[1]}.tag=0;",1),None),
        "wrong_domain":(code.replace(root_hook[0],root_hook[0].replace(root_hook[1]+"->token","0"),1),None),
        "wrong_owner":(code.replace(f"free({frees[1]});",f"free({first_heap});",1),"3"),
        # Capture integer identity before real free, then reconstitute only an
        # invalid REQUEST. Never load an indeterminate post-free C pointer.
        "duplicate_free":(code.replace(f"free({frees[0]});",f"uintptr_t remembered=(uintptr_t){frees[0]};free({frees[0]});free((void *)remembered);",1),"2"),
        "post_free_reloan":(code.replace(f"free({frees[0]});",f"uintptr_t remembered=(uintptr_t){frees[0]};free({frees[0]});NL_FIVE_ROOT((const nl_node *)remembered,1,1,5,8,1);",1),"2"),
        "missing_free":(code.replace(f"free({frees[0]});","/* skipped free */",1),"2"),
        "early_free":(code.replace(end_hook[0],f"free((void *){end_hook[1]});"+end_hook[0],1),"2"),
        "false_fifth_outcome":(code.replace(f"NL_FIVE_TRIAL(4,{trial5},56,8);",f"NL_FIVE_TRIAL(4,NULL,56,8);",1),None),
        "hidden_cleanup":(code.replace("NL_FIVE_FINISH();","free(NULL);NL_FIVE_FINISH();",1),"1"),
    }
    for name,(altered,outcome) in mutations.items():
        assert altered!=code,name
        native(root,name,altered,mutant=True,fail=outcome)
    # Memory-safe library-policy mismatch is source-admitted and physically
    # lowered, then rejected by the separate library topology observer.
    altered=prepare(root,"changed_tail",source.replace("Some(ptr_C))","Some(ptr_B))",1))
    assert altered!=code
    native(root,"changed_tail",altered,mutant=True)
evidence=os.environ.get("NEWLANG_FIVE_EVIDENCE")
if evidence:
    pathlib.Path(evidence).write_text(json.dumps({"fixture_sha256":hashlib.sha256(source.encode()).hexdigest(),
                                               "host_compiler":run([cc,"--version"]).stdout.splitlines()[0],
                                               "sources":sources,"records":records},indent=2)+"\n")
print("five-root native: four real-source variants, six observed/unobserved worlds, unwrapped success, 11 safe C mutants and admitted changed-tail policy control")
