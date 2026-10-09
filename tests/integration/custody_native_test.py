"""Unchanged full source → owned evidence → emitted C17 → observed real heap.

Platform supplies only fallible malloc/free. Observer cannot repair bytes or
release memory; the generated source functions perform every owner operation.
"""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

compiler, probe, cc, fixture, sanitizer = sys.argv[1:]
support = pathlib.Path(__file__).resolve().parent
source = pathlib.Path(fixture).read_text()
assert hashlib.sha256(source.encode()).hexdigest() == "a30bbc37ff817020165d44a1436cb98ac934568f557068b378bf3118227d5644"

def run(args, **kw):
    return subprocess.run(args, capture_output=True, text=True, **kw)

def native(root, name, code, definitions, plain=False, corrupt=False, failure=None):
    cfile, exe = root / (name + ".c"), root / name
    cfile.write_text(code)
    command = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
               "-DNDEBUG", "-O2", "-fno-builtin-malloc", "-fno-builtin-free",
               str(cfile), str(support / "two_heap_platform.c"),
               "-Wl,--wrap=malloc", "-Wl,--wrap=free", "-o", str(exe)]
    command += (["-DNEWLANG_UNOBSERVED"] if plain else definitions +
                ['-DNEWLANG_HEAP_OBSERVER="' + str(support / "custody_observer.h") + '"'])
    if sanitizer != "none":
        command += [f"-fsanitize={sanitizer}", "-fno-sanitize-recover=all",
                    "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    built = run(command)
    assert built.returncode == 0, (name, built.stderr)
    for failed in ([failure] if corrupt else ["1", "2", None]):
        env = dict(os.environ)
        env.pop("NEWLANG_TEST_FAIL_CALL", None)
        if failed:
            env["NEWLANG_TEST_FAIL_CALL"] = failed
        result = run([str(exe)], env=env)
        if corrupt:
            assert result.returncode == -6 and "OBSERVER_REJECT" in result.stderr, (name, result)
            assert "AddressSanitizer" not in result.stderr and "runtime error:" not in result.stderr, (name, result)
            print(name, "observer=detected sanitizer=clean")
            continue
        assert result.returncode == 0 and not result.stderr, (name, failed, result)
        if plain:
            assert not result.stdout
            continue
        row = re.fullmatch(r"CUSTODY head=([0-9a-f]+) tail=([0-9a-f]+) trials=(\d+) free=(\d+) policy=(\d+) recipient=(\d+) stored=(\d+) returned=(\d+) extract=(\d+) terminal=(\d+) none=(\d+)/(\d+)\n", result.stdout)
        assert row, result.stdout
        h, t = (int(v, 16) for v in row.groups()[:2])
        counts = tuple(int(v) for v in row.groups()[2:])
        if failed == "1":
            assert h == t == 0 and counts == (1,0,0,0,0,0,0,0,0,0)
        elif failed == "2":
            assert h and not t and counts == (2,1,0,0,0,0,0,0,0,0)
        else:
            policy = int(next(d.split("=")[1] for d in definitions if d.startswith("-DEXPECT_POLICY=")))
            adopted = int(policy == 1)
            assert h and t and h != t and counts == (2,2,policy,adopted,adopted,adopted,1,1,adopted,1)
        print(name, failed or "both", result.stdout.strip())

def positive(root, name, text, corrupt=False):
    path = root / (name + ".nl")
    path.write_text(text)
    checked = run([probe, str(path)])
    assert checked.returncode == 0, checked.stderr
    rows = [(r.split()[0], list(map(int,r.split()[1:]))) for r in checked.stdout.splitlines()]
    initial = [v for k,v in rows if k == "INIT"]
    fields = [v for k,v in rows if k == "FIELD"]
    roots = [v for k,v in rows if k == "ROOT"]
    assert len(initial) == 2 and len(fields) == 2 and len(roots) == 4
    assert len([v for k,v in rows if k == "CUSTODY"]) == 1
    assert all(initial[0][i] != initial[1][i] for i in range(4))
    definitions = []
    for prefix, values in zip(("H","T"), initial):
        definitions += [f"-DEXPECT_{prefix}{k}={v}" for k,v in zip(("R","P","I","D","V"), values)]
    definitions += [f"-DEXPECT_{k}={v}" for k,v in zip(("NOMINAL","CHILD","CI"), fields[0][1:4])]
    policy = 2 if "refusal" in name else 1
    definitions += [f"-DEXPECT_POLICY={policy}"]
    definitions += [f"-DEXPECT_{k}={r[5]}" for k,r in zip(("HS0","HS1","TSA","TSF"), roots)]
    emitted, repeated = run([compiler,str(path)]),run([compiler,str(path)])
    assert emitted.returncode == repeated.returncode == 0 and not emitted.stderr and emitted.stdout == repeated.stdout
    code = emitted.stdout
    assert code.count("malloc(24);") == 2
    before, main = code.split("int main(void) {",1)
    recipient = re.search(r"static void nl_recipient_\d+\([\s\S]+?(?=\nstatic )", before)
    producer = re.search(r"static nl_live_tail nl_producer_\d+\([^}]+", before)
    assert recipient and producer and "NL_CUSTODY_ENTER" in recipient[0]
    assert "*nl_v_" in recipient[0] and "NL_CUSTODY_NONE(1" in recipient[0]
    assert "free(" not in recipient[0] and "malloc(" not in recipient[0]
    assert main.count("NL_CUSTODY_EXTRACT(") == 2 and main.count("NL_CUSTODY_NONE(2") == 2
    assert re.search(r"nl_live_tail nl_v_\d+ = nl_producer_\d+\(", main)
    assert re.search(r"nl_recipient_\d+\(nl_v_\d+,nl_v_\d+\);",main)
    assert main.count("NL_HEAP_HANDOFF(") == 2
    for p,s,place,inc,domain,scope,write in roots:
        assert f",{write},{place},{inc},{scope});" in code
    native(root,name,code,definitions)
    native(root,name+"_plain",code,definitions,plain=True)
    if not corrupt:
        return
    returned = re.search(r"NL_LIVE_RETURN\((nl_v_\d+)\);",code)
    store = re.search(r"NL_CUSTODY_STORED\((nl_b_\d+_\d+),",code)
    extract = re.search(r"NL_CUSTODY_EXTRACT\((nl_v_\d+),(nl_v_\d+)\);",main)
    terminal = re.search(r"nl_owner_\d+_\d+\(nl_v_\d+,nl_v_\d+,nl_v_\d+\);",main)
    frees = re.findall(r"free\((nl_v_\d+)\.handle\);",code)
    assert returned and store and extract and terminal and len(frees) == 5
    mutations = {
        "return_wrong_ptr":(code.replace(returned[0],returned[1]+".ptr=(const nl_node *)test_allocated_address(0);"+returned[0],1),None),
        "return_wrong_owner":(code.replace(returned[0],returned[1]+".allocation.handle=(void *)test_allocated_address(0);"+returned[0],1),None),
        "return_wrong_domain":(code.replace(returned[0],returned[1]+".domain.token=EXPECT_HD;"+returned[0],1),None),
        "custody_wrong_ptr":(code.replace(store[0],store[1]+"->packet.ptr=(const nl_node *)test_allocated_address(0);"+store[0],1),None),
        "custody_wrong_owner":(code.replace(store[0],store[1]+"->packet.allocation.handle=(void *)test_allocated_address(0);"+store[0],1),None),
        "custody_wrong_domain":(code.replace(store[0],store[1]+"->packet.domain.token=EXPECT_HD;"+store[0],1),None),
        "custody_missing_some":(code.replace(store[0],store[1]+"->tag=0;"+store[0],1),None),
        "extracted_wrong_ptr":(code.replace(extract[0],extract[2]+".packet.ptr=(const nl_node *)test_allocated_address(0);"+extract[0],1),None),
        "wrong_custody_local":(code.replace(store[0],f"nl_custody rogue=*{store[1]};{store[1]}=&rogue;"+store[0],1),None),
        "missing_unlink":(re.sub(r"\*nl_v_\d+ = (nl_v_\d+);(\nNL_HEAP_CHANGE\([^;]+,2\);)",r"(void)\1;\2",code,count=1),None),
        "invalid_scope":(re.sub(r"(NL_CUSTODY_LOAN\([^,]+,)\d+(,1\);)",r"\g<1>0\2",code,count=1),None),
        "wrong_none":(re.sub(r"(NL_CUSTODY_NONE\(1,)(nl_v_\d+)(\);)",r"\2.tag=1;\1\2\3",code,count=1),None),
        "skipped_terminal":(code.replace(terminal[0],"(void)"+terminal[0].split("(",1)[0]+";",1),None),
        "duplicate_free":(code.replace(f"free({frees[0]}.handle);",f"free({frees[0]}.handle);free({frees[0]}.handle);",1),None),
        "skipped_tail_free":(code.replace(f"free({frees[0]}.handle);","/* suppressed tail free */",1),None),
        "skipped_head_free":(code.replace(f"free({frees[3]}.handle);","/* suppressed head free */",1),None),
        "second_none_head_leak":(code.replace(f"free({frees[2]}.handle);","/* suppressed failure-path free */",1),"2"),
        "skipped_final_none":(re.sub(r"NL_CUSTODY_NONE\(2,nl_v_\d+\);","/* omitted explicit consume */",code),None),
    }
    for reason,(mutated,failed) in mutations.items():
        assert mutated != code, reason
        native(root,name+"_bad_"+reason,mutated,definitions,corrupt=True,failure=failed)

refusal = source.replace("let admit_flag = Option<ptr<Node>>::None;", "let admit_flag = Option<ptr<Node>>::Some(ptr_h);")
variants = {
    "primary": source,
    "refusal": refusal,
    "renamed": source.replace("Node","Cell").replace("next","link").replace("packet","owner").replace("custody","retained").replace("recipient_adopt","keep_original"),
    "refusal_renamed": refusal.replace("Node","Cell").replace("next","link").replace("packet","owner").replace("custody","retained").replace("recipient_adopt","keep_original"),
    "main_first": source[source.index("fn main()"):]+source[:source.index("fn main()")],
}
with tempfile.TemporaryDirectory(prefix="custody-native-") as directory:
    for name,text in variants.items():
        positive(pathlib.Path(directory),name,text,corrupt=name=="primary")
print("custody native: full source; 5 variants × observed/plain × 3 outcomes; 18 safe observer mutation controls")
