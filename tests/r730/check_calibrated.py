"""R725 calibrated, still independent oracle; initial BLIND bytes never rewritten."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

def run(*args):
    return subprocess.run([str(arg) for arg in args], capture_output=True)

def require(ok, reason, result=None):
    if not ok:
        suffix = ""
        if result is not None:
            suffix = "\nSTDERR=" + result.stderr.decode(errors="replace")
            suffix += "\nSTDOUT=" + result.stdout.decode(errors="replace")
        raise AssertionError(reason + suffix)

def insert_observer(source, data, v):
    """Read-only instrumented copy of actual C17; never sets guest state."""
    is_a = data["file"].startswith("A")
    terminal = data["terminal"]
    index = data["index"]
    old = 34 if is_a else 67
    post = (34 if terminal else 141) if is_a else (211 if terminal else 97)
    sibling = 23 if is_a else 12
    second_old = 51 if is_a else 44
    second_new = 89 if is_a else 200
    second_sibling = 62 if is_a else 18
    originals = ([23, 34], [51, 62]) if is_a else ([67, 12], [18, 44])
    c = source
    additions = {}

    def add(pos, txt):
        additions[pos] = additions.get(pos, "") + txt
    def one(regex, start=0, stop=None):
        hits = list(re.finditer(regex, c[start:stop], re.M))
        require(len(hits) == 1, f"read-only observer expected 1 unique {regex}; got {len(hits)}")
        m = hits[0]
        return start+m.start(), start+m.end(), m
    constructors = list(re.finditer(r"nl_type_\d+ nl_local_(\d+) = \{[^;]*;",c))
    require(len(constructors) == 2, "exact two real original constructors")
    root = int(constructors[0].group(1))
    other = int(constructors[1].group(1))
    require(root == v["base"], "genuine source-issued base != emitted physical original")
    a,b=originals
    add(constructors[0].end(),f'\n    ob_target=&nl_local_{root}.f{index}; ob_sibling=&nl_local_{root}.f{1-index};'
                             f'\n    if (*ob_target != {old} || *ob_sibling != {sibling}) exit(91);')
    add(constructors[1].end(),f'\n    ob_other=&nl_local_{other}.f{1-index}; ob_other_sibling=&nl_local_{other}.f{index};'
                             f'\n    if (*ob_other != {second_old} || *ob_other_sibling != {second_sibling}) exit(91);')
    fn=v["function"]
    formal=v["formal"]
    expected_arm=0 if terminal else 1
    require(v["normal_arm"]==1,"one normal (else) edge required")
    start,stop,_=one(rf"static void nl_fn_{fn}\(uint8_t \*nl_local_{formal}, uint8_t nl_local_\d+\)\n\{{")
    add(stop, f'\n    ++ob_calls; if (nl_local_{formal} != ob_target || *ob_target != {old} '
              f'|| *ob_sibling != {sibling}) exit(91);')
    left,right,_=one(r"if \(\(nl_less_left_\d+ < nl_less_right_\d+\)\) \{", stop)
    delimiter=c.index("    } else {",right)
    last=c.index("\n    }\n",delimiter)
    bounds=[(right,delimiter),(delimiter+len("    } else {"),last)]
    for arm,(lo,hi) in enumerate(bounds):
        add(lo,f"\n        ob_arm={arm};")
        expected_writes = 0 if is_a and arm==0 else 1
        captures=list(re.finditer(rf"const uint8_t nl_replace_old_\d+ = \*nl_local_{formal};",c[lo:hi]))
        writes=list(re.finditer(rf"\*nl_local_{formal} = [^;]+;",c[lo:hi]))
        require(len(captures)==expected_writes and len(writes)==expected_writes,
                "actual callee store/capture count "+str((arm,len(captures),len(writes))))
        for m in captures:
            add(lo+m.end(),f'\n        if (*ob_target != {old} || *ob_sibling != {sibling}) exit(91);')
        for m in writes:
            predicted = (141 if is_a else (211 if arm==0 else 97))
            add(lo+m.end(),f'\n        ++ob_stores; if (*ob_target != {predicted} '
                           f'|| *ob_sibling != {sibling}) exit(91);')
        returned=list(re.finditer(r"\breturn;",c[lo:hi]))
        require(len(returned)==(1 if arm==0 else 0),"real RETURN confined to helper terminal arm")
        for m in returned:
            add(lo+m.start(),"\n        ob_return=1; ")
    call_start,call_end,_=one(rf"nl_fn_{fn}\(nl_local_{v['caller_ref']}, [^;]+;",0,start)
    add(call_end,f'\n        if (ob_calls!=1 || ob_arm!={expected_arm} || ob_return!={1 if terminal else 0}'
                 f' || ob_stores!={0 if is_a and terminal else 1} '
                 f'|| *ob_target!={post} || *ob_sibling!={sibling} '
                 f'|| *ob_other!={second_old}) exit(91);\n        ob_caller=1;')
    # Caller continuation physically writes DIFFERENT original; archive pointer exists.
    archive_ref_match=list(re.finditer(rf"&nl_local_{other}\.f{1-index};",c))
    require(len(archive_ref_match)>=1, "actual archived field address required")
    arch_old=list(re.finditer(r"const uint8_t nl_replace_old_\d+ = \*nl_local_(\d+);",c[:start]))
    require(len(arch_old)==1, "one caller continuation replace capture")
    local=arch_old[0].group(1)
    add(arch_old[0].end(), f'\n        if (ob_caller != 1 || *ob_other != {second_old} '
                             f'|| *ob_target != {post}) exit(91);')
    following=re.search(rf"\*nl_local_{local} = [^;]+;",c[arch_old[0].end():start])
    require(following is not None, "actual caller physical write not generated")
    add(arch_old[0].end()+following.end(),f'\n        ++ob_continuation; if (*ob_other != {second_new} '
                                           f'|| *ob_other_sibling != {second_sibling} '
                                           f'|| *ob_target != {post} || *ob_sibling != {sibling}) exit(91);')
    # Check the postcall and archive lexical scalar Copy reads, not observer-synthesized values.
    for rid,field,expected,occurrence in [
         (root,index,post,1),(root,1-index,sibling,1),
         (other,1-index,second_new,0),(other,index,second_sibling,0)]:
        observations=list(re.finditer(rf"const uint8_t nl_local_\d+ = nl_local_{rid}\.f{field};", c[:start]))
        require(len(observations)>=occurrence+1, "physical Copy read exists")
        m=observations[occurrence]
        add(m.end(),f'\n    if (*ob_target != {post} || *ob_sibling != {sibling} || '
                    f'{"ob_caller != 1" if rid==root else "ob_continuation != 1"} '
                    f'|| {m.group(0).split(" = ")[0].split()[-1]} != {expected}) exit(91);')
    # Close observer-only pointer records before original locals leave scope.
    caller_func=re.search(r"static void nl_fn_\d+\(void\)\n\{",c)
    require(caller_func is not None,"actual main caller function")
    end_caller=c.index("\n}\n",caller_func.end())
    require(end_caller<start,"callee comes after caller")
    add(end_caller, f'\n    if (ob_calls != 1 || ob_caller != 1 || ob_continuation != 1'
                    f' || *ob_target != {post} || *ob_sibling != {sibling} '
                    f'|| *ob_other != {second_new}) exit(91);'
                    '\n    ob_target=0; ob_sibling=0; ob_other=0; ob_other_sibling=0;')
    for pos in sorted(additions,reverse=True):
        c=c[:pos]+additions[pos]+c[pos:]
    declarations=('#include <stdlib.h>\n'
                  'static const volatile uint8_t *ob_target, *ob_sibling, *ob_other, *ob_other_sibling;\n'
                  'static unsigned ob_calls, ob_stores, ob_arm, ob_return, ob_caller, ob_continuation;\n')
    require(c.count('#include <stdint.h>')==1,"include locus")
    c=c.replace('#include <stdint.h>','#include <stdint.h>\n'+declarations,1)
    require(c.count('    return 0;\n}')==1,"main runtime observer terminal")
    c=c.replace('    return 0;\n}',f'    return (ob_calls == 1 && ob_caller == 1 && ob_continuation == 1 && '
                f'ob_arm == {expected_arm} && ob_return == {1 if terminal else 0}) ? 0 : 91;\n}}',1)
    return c

def main():
    compiler, gate, cc, sanitizer, folder = sys.argv[1:]
    root=Path(folder)
    blind=json.loads((root/"freeze.json").read_text())["cases"]
    cases=json.loads((root/"calibrated.json").read_text())["cases"]
    with tempfile.TemporaryDirectory(prefix="r725_calibrated_") as td:
        tmp=Path(td)
        # Permanent initial pre-inspection bytes preserved; original typing refusal retested.
        for item in blind:
            source=root/item["file"]
            require(hashlib.sha256(source.read_bytes()).hexdigest()==item["sha256"],
                    "initial immutable first-comment SHA "+item["file"])
            require(len(source.read_bytes())==item["bytes"],"initial byte size")
            observed=run(compiler,"--check",source)
            if item["file"][0] in "AB":
                require(observed.returncode==3 and b"USIZE-EQUALITY-TYPE" in observed.stderr
                        and not observed.stdout,
                        "independent first-pass u8 comparison must remain rejected",observed)
                print("R725 BLIND-INITIAL",item["file"],"CHECK=3 USIZE-EQUALITY-TYPE retained")
            else:
                require(observed.returncode==3 and not observed.stdout,
                        "negative CHECK",observed)
                emission=run(compiler,source)
                require(emission.returncode==3 and not emission.stdout,"zero C17 negative",emission)
                file=tmp/"sentinel.c"
                file.write_bytes(b"r725-preexisting-output")
                guarded=run(compiler,"--emit-c17-to",file,source)
                require(guarded.returncode==3 and file.read_bytes()==b"r725-preexisting-output",
                        "negative named output unchanged",guarded)
                print("R725 BLIND-NEGATIVE",item["file"],"CHECK=3 zero C17 preserve=YES",
                      observed.stderr.decode(errors="replace").strip())
        for item in cases:
            source=root/item["file"]
            require(hashlib.sha256(source.read_bytes()).hexdigest()==item["sha256"],
                    "calibrated pre-test SHA "+item["file"])
            require(len(source.read_bytes())==item["bytes"],"calibrated byte size")
            ck=run(compiler,"--check",source)
            require(ck.returncode==0 and not ck.stdout,"calibrated positive CHECK",ck)
            validation=run(gate,source,item["index"])
            require(validation.returncode==0,"source issued public proof/corruption",validation)
            v=json.loads(validation.stdout.decode())
            require(v["index"]==item["index"] and v["rejected"]>=21,
                    "true child index and 21+ independent borrowed view invalidations")
            c17=run(compiler,source)
            require(c17.returncode==0 and c17.stdout and not c17.stderr,
                    "true normal strict C17 from ordinary CLI",c17)
            generated=c17.stdout.decode()
            named=tmp/"named.c"
            saved=run(compiler,"--emit-c17-to",named,source)
            require(saved.returncode==0 and named.read_bytes()==c17.stdout,
                    "original named output must exactly equal stdout C",saved)
            guest=tmp/"original.c"
            guest.write_bytes(c17.stdout)
            instrumented=insert_observer(generated,item,v)
            observer=tmp/"passive.c"
            observer.write_text(instrumented)
            for level in ["-O0","-O2"]:
                flags=[cc,"-std=c17","-Wall","-Wextra","-Werror","-pedantic-errors",level]
                if sanitizer not in ("none",""):
                    flags += [f"-fsanitize={sanitizer}","-fno-omit-frame-pointer",
                              "-fno-sanitize-recover=all","-fno-pie","-no-pie"]
                for label,fp in [("unchanged",guest),("passive",observer)]:
                    exe=tmp/("native_"+label)
                    compiled=run(*(flags+[fp,"-o",exe]))
                    require(compiled.returncode==0,f"strict C17 {label} {level}",compiled)
                    executed=run(exe)
                    require(executed.returncode==0 and not executed.stderr,
                            f"real native {label} {level}",executed)
            print("R725 CALIBRATED POSITIVE",item["file"],"source/CHECK/issued-validate/native/observer O0 O2=PASS",
                  "original root",v["root"],"target-child",v["child"],"public invalidations",v["rejected"])
    print("R725 FINITE CALIBRATED PASS; OOM and privately coherent tamper NOT TESTED")

if __name__=="__main__":
    main()
