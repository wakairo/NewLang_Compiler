"""Actual-source receiver, independent checked probe, real platform + read-only observer."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

compiler, probe, cc, fixture, sanitizer = sys.argv[1:]
support = pathlib.Path(__file__).resolve().parent
source = pathlib.Path(fixture).read_text()

def run(args, **kwargs):
    return subprocess.run(args, capture_output=True, text=True, **kwargs)

def native(root, name, code, definitions, plain=False, corrupt=False, fail=None):
    path, exe = root / (name + ".c"), root / name
    path.write_text(code)
    cmd = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-DNDEBUG", str(path), "-o", str(exe),
           "-fno-builtin-malloc", "-fno-builtin-free", str(support / "two_heap_platform.c"), "-Wl,--wrap=malloc", "-Wl,--wrap=free"]
    cmd += ["-DNEWLANG_UNOBSERVED"] if plain else definitions + ['-DNEWLANG_HEAP_OBSERVER="' + str(support / "handoff_observer.h") + '"']
    if sanitizer != "none":
        cmd += [f"-fsanitize={sanitizer}", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    built = run(cmd)
    assert built.returncode == 0, (name, built.stderr)
    for outcome in ([fail] if corrupt else ["1", "2", None]):
        env = dict(os.environ)
        env.pop("NEWLANG_TEST_FAIL_CALL", None)
        if outcome: env["NEWLANG_TEST_FAIL_CALL"] = outcome
        executed = run([str(exe)], env=env)
        if corrupt:
            # The independent observer must fire, including under sanitizers;
            # a random crash or sanitizer finding alone is insufficient.
            assert executed.returncode == -6, (name, executed)
        else:
            assert executed.returncode == 0 and not executed.stderr, (name, outcome, executed)
            if not plain:
                m = re.fullmatch(r"OBSERVED head=([0-9a-f]+) tail=([0-9a-f]+) trials=(\d+) free=(\d+) roots=(\d+) changes=(\d+) copy=(\d+) scopes=(\d+) receiver=(\d+) receiver_free=(\d+) donor_free=(\d+)\n", executed.stdout)
                assert m, executed.stdout
                h, t = map(lambda v: int(v, 16), m.groups()[:2])
                counts = tuple(map(int, m.groups()[2:]))
                read = int(next(v.split("=")[1] for v in definitions if v.startswith("-DEXPECT_RECEIVER_READ=")))
                if outcome == "1":
                    assert h == t == 0 and counts == (1, 0, 0, 0, 0, 0, 0, 0, 0)
                elif outcome == "2":
                    assert h and not t and counts == (2, 1, 0, 0, 0, 0, 0, 0, 1)
                else:
                    assert h and t and h != t and counts == (2, 2, 4+read, 2, 1, 4+read, 1, 1, 1)
                print(name, outcome or "both", executed.stdout.strip())

def positive(root, name, text, corruption=False):
    path = root / (name + ".nl")
    path.write_text(text)
    checked = run([probe, str(path)])
    assert checked.returncode == 0, checked.stderr
    rows = [(row.split()[0], list(map(int, row.split()[1:]))) for row in checked.stdout.splitlines()]
    initial = [v for k,v in rows if k == "INIT"]
    roots = [v for k,v in rows if k == "ROOT"]
    fields = [v for k,v in rows if k == "FIELD"]
    calls = [v for k,v in rows if k == "CALL"]
    assert len(initial) == 2 and len(fields) == 3 and len(calls) == 1
    function, pp, pa, pd, read = calls[0]
    assert len(roots) == 4 + read and all(initial[0][i] != initial[1][i] for i in range(4))
    definitions = []
    for prefix, row in zip(("H","T"),initial):
        definitions += [f"-DEXPECT_{prefix}{key}={value}" for key,value in zip(("R","P","I","D","V"),row)]
    definitions += [f"-DEXPECT_S{i}={r[5]}" for i,r in enumerate(roots)]
    if not read: definitions += ["-DEXPECT_S4=0"]
    definitions += [f"-DEXPECT_{k}={v}" for k,v in zip(("PP","PA","PD","RECEIVER_READ"),(pp,pa,pd,read))]
    definitions += [f"-DEXPECT_NOMINAL={fields[0][1]}",f"-DEXPECT_CHILD={fields[0][2]}",f"-DEXPECT_CI={fields[0][3]}"]
    emitted, retry = run([compiler,str(path)]),run([compiler,str(path)])
    assert emitted.returncode == retry.returncode == 0 and emitted.stdout == retry.stdout and not emitted.stderr
    code = emitted.stdout
    declaration = f"static void nl_owner_{function}("
    assert code.count(declaration) == 1 and code.count("malloc(24);") == 2
    pre_main, main = code.split("int main(void) {",1)
    receiver = pre_main.split(declaration,1)[1]
    assert receiver.count("free(nl_v_") == 1 and main.count("free(nl_v_") == 2
    assert "malloc(" not in receiver and "NL_HEAP_END(" in receiver and "NL_HEAP_FINALIZE(" in receiver
    assert re.search(r"const nl_node \*nl_b_\d+_"+str(pp)+r",nl_allocation nl_b_\d+_"+str(pa)+r",nl_domain nl_b_\d+_"+str(pd),receiver)
    call = re.search(r"nl_owner_"+str(function)+r"\((nl_v_\d+),(nl_v_\d+),(nl_v_\d+)\);",main)
    assert call and main.count("NL_HEAP_HANDOFF(") == main.count("NL_HEAP_RETURNED(") == 1
    root_calls = re.findall(r"NL_HEAP_ROOT\(nl_v_(\d+),nl_v_(\d+)->token,(\d+),(\d+),(\d+),(\d+)\)", code)
    assert len(root_calls) == len(roots)
    for p,s,place,inc,domain,scope,write in roots:
        matches = [r for r in root_calls if tuple(map(int,r[2:])) == (write,place,inc,scope)]
        assert len(matches) == 1
        target,stable = matches[0][:2]
        origin = re.search(r"nl_v_"+target+r" = (?:\(nl_node \*\))?nl_v_(\d+);",code)
        pointer = re.findall(r"const nl_node \* nl_v_(\d+) = nl_b_\d+_"+str(p)+r";",code)
        stability = re.findall(r"const nl_domain \* nl_v_(\d+) = nl_b_\d+_"+str(s)+r";",code)
        assert origin and origin[1] in pointer and stable in stability
    native(root,name,code,definitions)
    native(root,name+"_plain",code,definitions,plain=True)
    if corruption:
        heaps = re.findall(r"void \*(nl_heap_\d+) = malloc\(24\);",code)
        frees = re.findall(r"free\((nl_v_\d+)\.handle\);",code)
        writes = re.findall(r"\*nl_v_\d+ = nl_v_\d+;",code)
        some = re.search(r"nl_node_option (nl_v_\d+) = \{ 1, (nl_v_\d+) \};",code)
        assert len(heaps)==2 and len(frees)==3 and len(writes)==2 and some
        enter = re.search(r"NL_HEAP_RECEIVER_ENTER\((nl_b_\d+_\d+),",code)
        end = re.search(r"NL_HEAP_END\((nl_v_\d+),",receiver)
        # Patch receiver's real pointer after entry so the EndRoot observer,
        # rather than the handoff admission hook alone, attacks callee use.
        end_full = re.search(r"NL_HEAP_END\(nl_v_\d+,[^;]+;",code)
        wrong_pointer = code.replace(end_full[0], f"{end[1]}=(const nl_node *)test_allocated_address(0);"+end_full[0],1)
        mutations = {
            "wrong_callee_tail": (wrong_pointer,None),
            "wrong_call_tail": (code.replace(call[0],call[0].replace(call[1],f"(const nl_node *){heaps[0]}")),None),
            "forged_some": (code.replace(some[0],some[0]+f"{some[1]}.ptr=(const nl_node *){heaps[0]};",1),None),
            "missing_unlink": (code.replace(writes[1],"(void)"+writes[1].split(" = ")[1],1),None),
            "cross_free": (code.replace(f"free({frees[0]}.handle);", "free((void *)test_allocated_address(0));",1),None),
            "donor_tail_free": (code.replace(call[0],f"free({call[2]}.handle);"+call[0],1),None),
            "double_free": (code.replace(f"free({frees[0]}.handle);", f"free({frees[0]}.handle);free({frees[0]}.handle);",1),None),
            "premature_free": (code.replace(enter[0],"free((void *)test_allocated_address(1));"+enter[0],1),None),
            "second_null_leak": (code.replace(f"free({frees[1]}.handle);","/* omitted head free */",1),"2"),
            "skipped_receiver_free": (code.replace(f"free({frees[0]}.handle);","/* omitted receiver free */",1),None),
        }
        for reason,(altered,fail) in mutations.items():
            assert altered != code,reason
            native(root,name+"_bad_"+reason,altered,definitions,corrupt=True,fail=fail)

read_loan = """    loan_read(tail_life) { |stable_t|
        let root_r = ref_from_ptr(read, tail_ptr, stable_t);
        unit
    };
"""
variants = {
    "primary":source,
    "renamed":source.replace("Node","Cell").replace("next","link").replace("payload","datum").replace("tail_ptr","p_arg").replace("tail_allocation","a_arg").replace("tail_life","d_arg").replace("receive_and_release_tail","release_cell"),
    "without_optional_read":source.replace(read_loan,""),
}
with tempfile.TemporaryDirectory(prefix="handoff-native-") as directory:
    for name,text in variants.items(): positive(pathlib.Path(directory),name,text,corruption=name=="primary")
print("live-tail native: 3 actual-source variants, 3 NULL/both outcomes observed/plain NDEBUG, 10 observer-detected corruption controls")
