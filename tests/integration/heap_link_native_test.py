"""Actual source, independent checked expectations, read-only NDEBUG native oracle."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

compiler, probe, cc, fixture, sanitizer = sys.argv[1:]
support = pathlib.Path(__file__).resolve().parent
actual = pathlib.Path(fixture).read_text()

def run(args, **kw):
    return subprocess.run(args, capture_output=True, text=True, **kw)

def native(root, name, code, expected, broken=False, plain=False):
    source, exe = root / (name + ".c"), root / name
    source.write_text(code)
    cmd = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-DNDEBUG", str(source), "-o", str(exe)]
    if not plain:
        cmd += ["-fno-builtin-malloc", "-fno-builtin-free", str(support / "allocated_platform.c"), "-Wl,--wrap=malloc", "-Wl,--wrap=free", '-DNEWLANG_HEAP_OBSERVER="' + str(support / "heap_link_observer.h") + '"']
        cmd += [f"-DEXPECT_{k}={v}" for k, v in expected.items()]
    if sanitizer != "none":
        cmd += [f"-fsanitize={sanitizer}", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    built = run(cmd)
    assert built.returncode == 0, (name, built.stderr)
    result = run([str(exe)])
    if broken:
        assert result.returncode != 0, (name, "observer missed corruption")
    else:
        assert result.returncode == 0 and not result.stderr, (name, result)
        if not plain:
            assert "roots=3 fields=3 changes=2 copies=3 scopes=3 free=1" in result.stdout
            print(name, result.stdout.strip())
            failed = run([str(exe)], env=dict(os.environ, NEWLANG_TEST_ALLOCATION_FAIL="1"))
            assert failed.returncode == 0 and not failed.stderr, (name, "real None path", failed)
            assert "heap=0 lexical=0 roots=0 fields=0 changes=0 copies=0 scopes=0 free=0" in failed.stdout
            print(name + "_None", failed.stdout.strip())

def positive(root, name, text, corrupt=False):
    path = root / (name + ".nl")
    path.write_text(text)
    checked = run([probe, str(path)])
    assert checked.returncode == 0, checked.stderr
    rows = [line.split() for line in checked.stdout.splitlines()]
    roots = [list(map(int,r[1:])) for r in rows if r[0] == "ROOT"]
    fields = [list(map(int,r[1:])) for r in rows if r[0] == "FIELD"]
    values = next(list(map(int,r[1:])) for r in rows if r[0] == "VALUES")
    expected = dict(HEAP=values[0], LEXICAL=values[1], ROOT=roots[0][2], ROOT_INC=roots[0][3], DOMAIN=roots[0][4], NOMINAL=fields[0][1], CHILD=fields[0][2], CHILD_INC=fields[0][3])
    emitted, again = run([compiler, str(path)]), run([compiler, str(path)])
    assert emitted.returncode == 0 and not emitted.stderr and emitted.stdout == again.stdout
    code = emitted.stdout
    root_calls = re.findall(r"NL_HEAP_ROOT\(nl_v_(\d+),nl_v_(\d+)->token,(\d+),(\d+),(\d+),(\d+)\)",code)
    field_calls = re.findall(r"NL_HEAP_FIELD\(nl_v_(\d+),nl_v_(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\)",code)
    assert len(root_calls) == len(field_calls) == 3
    for index, (p,s,place,inc,d,scope) in enumerate(roots):
        r, stable, mode, rp, ri, rs = map(int,root_calls[index])
        assert (rp,ri,rs) == (place,inc,scope) and mode == (index != 1)
        pointer_copies = re.findall(r"const nl_node \* nl_v_(\d+) = nl_b_\d+_"+str(p)+r";",code)
        origin = re.search(r"nl_v_"+str(r)+r" = (?:\(nl_node \*\))?nl_v_(\d+);",code)
        assert origin and origin.group(1) in pointer_copies
        stability_copies = re.findall(r"const nl_domain \* nl_v_(\d+) = nl_b_\d+_"+str(s)+r";",code)
        assert str(stable) in stability_copies
    for index, (base,nominal,child,inc,scope) in enumerate(fields):
        r, projected, mode, n, key, c, ci, cs = map(int,field_calls[index])
        assert (n,key,c,ci,cs) == (nominal,0,child,inc,scope)
        assert re.search(r"nl_v_"+str(r)+r" = nl_b_\d+_"+str(base)+r";",code)
        assert f"nl_v_{projected} = &nl_v_{r}->f0;" in code
    native(root,name,code,expected)
    native(root,name+"_plain",code,expected,plain=True)
    if corrupt:
        writes = re.findall(r"\*nl_v_\d+ = nl_v_\d+;",code)
        assert len(writes) == 2
        release = re.search(r"free\(nl_v_\d+\.handle\);",code).group(0)
        lexical = re.search(r"nl_node (nl_b_\d+_\d+) =",code).group(1)
        first_ref = field_calls[0][1]
        mutations = {
            "wrong_tag": code.replace(" = { 1, nl_v_", " = { 0, nl_v_",1),
            "wrong_pointer": re.sub(r"(nl_node_option (nl_v_\d+) = \{ 1, nl_v_\d+ \};)",r"\1 \2.ptr=NULL;",code,count=1),
            "proxy": code.replace(writes[0],f"{lexical}.f0 = "+writes[0].split(" = ")[1]),
            "wrong_field": code.replace(f"nl_v_{first_ref} = &nl_v_{field_calls[0][0]}->f0;",f"nl_v_{first_ref} = (nl_node_option *)&nl_v_{field_calls[0][0]}->f1;"),
            "missing_write": code.replace(writes[0],"(void)"+writes[0].split(" = ")[1]),
            "missing_unlink": code.replace(writes[1],"(void)"+writes[1].split(" = ")[1]),
            "missing_free": code.replace(release,"/* no free */"),
            "double_free": code.replace(release,release+"\nfree(NULL);"),
            "wrong_free_identity": code.replace(release,"free(NULL);"),
            "early_free": code.replace("NL_HEAP_END(nl_v_","free("+re.search(r"nl_allocation (nl_b_\d+_\d+) =",code).group(1)+".handle); NL_HEAP_END(nl_v_",1),
        }
        for reason, altered in mutations.items():
            assert altered != code, reason
            native(root,name+"_broken_"+reason,altered,expected,broken=True)

with tempfile.TemporaryDirectory(prefix="heap-link-native-") as directory:
    root = pathlib.Path(directory)
    positive(root,"canonical",actual,True)
    positive(root,"renamed",actual.replace("Node","Cell").replace("next","link").replace("payload","datum"))
    positive(root,"payloads",actual.replace("u8(1)","u8(11)").replace("u8(2)","u8(37)"))
    positive(root,"selected_alias",actual.replace("let root_w = ref_from_ptr(write, heap_head, stable);","let p_alias=heap_head; let s_alias=stable; let root_w=ref_from_ptr(write,p_alias,s_alias);"))
    positive(root,"root_alias",actual.replace("replace(root_w@next,","let alias=root_w; replace(alias@next,"))
print("heap-owned link native: 5 source variants, real malloc/None, read-only identity/tag/scope/old-value observer, 10 corruption controls passed")
