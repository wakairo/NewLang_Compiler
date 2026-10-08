"""Full actual source, independent checked expectations, read-only native oracle.
No source-value extraction, seeded topology or expected C substituted as output.
"""
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

def native(root, name, code, definitions, broken=False, fail=None, plain=False):
    path, exe = root / (name + ".c"), root / name
    path.write_text(code)
    cmd = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-DNDEBUG", str(path), "-o", str(exe),
           "-fno-builtin-malloc", "-fno-builtin-free", str(support / "two_heap_platform.c"), "-Wl,--wrap=malloc", "-Wl,--wrap=free"]
    if plain:
        cmd += ["-DNEWLANG_UNOBSERVED"]
    else:
        cmd += definitions + ['-DNEWLANG_HEAP_OBSERVER="' + str(support / "two_heap_observer.h") + '"']
    if sanitizer != "none":
        cmd += [f"-fsanitize={sanitizer}", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    built = run(cmd)
    assert built.returncode == 0, (name, built.stderr)
    outcomes = [fail] if broken else [None, "1", "2"]
    for outcome in outcomes:
        env = dict(os.environ)
        env.pop("NEWLANG_TEST_FAIL_CALL", None)
        if outcome:
            env["NEWLANG_TEST_FAIL_CALL"] = outcome
        executed = run([str(exe)], env=env)
        if broken:
            assert executed.returncode != 0, (name, outcome, "corruption escaped observer")
        else:
            assert executed.returncode == 0 and not executed.stderr, (name, outcome, executed)
            if not plain:
                m = re.fullmatch(r"OBSERVED head=([0-9a-f]+) tail=([0-9a-f]+) trials=(\d+) free=(\d+) roots=(\d+) changes=(\d+) copy=(\d+) scopes=(\d+)\n", executed.stdout)
                assert m, executed.stdout
                h, t = (int(v, 16) for v in m.groups()[:2])
                counts = tuple(map(int, m.groups()[2:]))
                if outcome == "1":
                    assert h == t == 0 and counts == (1, 0, 0, 0, 0, 0)
                elif outcome == "2":
                    assert h != 0 and t == 0 and counts == (2, 1, 0, 0, 0, 0)
                else:
                    assert h and t and h != t and counts == (2, 2, 4, 2, 1, 4)
                print(name, outcome or "success", executed.stdout.strip())

def positive(root, name, text, corrupt=False):
    path = root / (name + ".nl")
    path.write_text(text)
    checked = run([probe, str(path)])
    assert checked.returncode == 0, checked.stderr
    records = [(row.split()[0], list(map(int, row.split()[1:]))) for row in checked.stdout.splitlines()]
    initial = [v for k, v in records if k == "INIT"]
    roots = [v for k, v in records if k == "ROOT"]
    fields = [v for k, v in records if k == "FIELD"]
    assert len(initial) == 2 and len(roots) == 4 and len(fields) == 3
    assert all(initial[0][i] != initial[1][i] for i in range(4))
    definitions = []
    for prefix, row in zip(("H", "T"), initial):
        for key, val in zip(("R", "P", "I", "D", "V"), row):
            definitions.append(f"-DEXPECT_{prefix}{key}={val}")
    definitions += [f"-DEXPECT_S{i}={r[5]}" for i, r in enumerate(roots)]
    definitions += [f"-DEXPECT_NOMINAL={fields[0][1]}", f"-DEXPECT_CHILD={fields[0][2]}", f"-DEXPECT_CI={fields[0][3]}"]
    emitted, retry = run([compiler, str(path)]), run([compiler, str(path)])
    assert emitted.returncode == retry.returncode == 0 and emitted.stdout == retry.stdout and not emitted.stderr
    code = emitted.stdout
    assert code.count("malloc(24);") == 2 and code.count("free(nl_v_") == 3
    root_calls = re.findall(r"NL_HEAP_ROOT\(nl_v_(\d+),nl_v_(\d+)->token,(\d+),(\d+),(\d+),(\d+)\)", code)
    field_calls = re.findall(r"NL_HEAP_FIELD\(nl_v_(\d+),nl_v_(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\)", code)
    assert len(root_calls) == 4 and len(field_calls) == 3
    for row, call in zip(roots, root_calls):
        p, s, place, inc, domain, scope, write = row
        target, stable, mode, rp, ri, rs = map(int, call)
        assert (mode, rp, ri, rs) == (write, place, inc, scope)
        origin = re.search(r"nl_v_" + str(target) + r" = (?:\(nl_node \*\))?nl_v_(\d+);", code)
        pointer_copies = re.findall(r"const nl_node \* nl_v_(\d+) = nl_b_\d+_" + str(p) + r";", code)
        stability_copies = re.findall(r"const nl_domain \* nl_v_(\d+) = nl_b_\d+_" + str(s) + r";", code)
        assert origin and origin.group(1) in pointer_copies and str(stable) in stability_copies
    for row, call in zip(fields, field_calls):
        base, nominal, child, inc, scope = row
        parent, projected, mode, n, index, c, ci, cs = map(int, call)
        assert (n, index, c, ci, cs) == (nominal, 0, child, inc, scope)
        assert re.search(r"nl_v_" + str(parent) + r" = nl_b_\d+_" + str(base) + r";", code)
        assert f"nl_v_{projected} = &nl_v_{parent}->f0;" in code
    native(root, name, code, definitions)
    native(root, name + "_plain", code, definitions, plain=True)
    if corrupt:
        writes = re.findall(r"\*nl_v_\d+ = nl_v_\d+;", code)
        assert len(writes) == 2
        frees = re.findall(r"free\(nl_v_(\d+)\.handle\);", code)
        releases = re.findall(r"NL_HEAP_RELEASE\(nl_v_\d+\.handle,nl_v_\d+\.bytes,nl_v_\d+\.length\);", code)
        assert len(frees) == len(releases) == 3
        heaps = re.findall(r"void \*(nl_heap_\d+) = malloc\(24\);", code)
        assert len(heaps) == 2
        some = re.search(r"nl_node_option (nl_v_\d+) = \{ 1, (nl_v_\d+) \};", code)
        first_parent, first_field = field_calls[0][:2]
        tail_ref = root_calls[2][0]
        copied = re.search(r"nl_node_option (nl_v_\d+) = \*(nl_v_\d+);\nNL_HEAP_COPY", code)
        end = re.search(r"NL_HEAP_END\((nl_v_\d+),", code)
        mutations = {
            "wrong_tail": (code.replace(some.group(0), some.group(0) + f" {some[1]}.ptr=(const nl_node *){heaps[0]};", 1), None),
            "wrong_head": (code.replace(f"nl_v_{first_field} = &nl_v_{first_parent}->f0;", f"nl_v_{first_field} = &((nl_node *){heaps[1]})->f0;"), None),
            "lexical_proxy": (code.replace("int main(void) {", "int main(void) { nl_node proxy={0};").replace(f"nl_v_{first_field} = &nl_v_{first_parent}->f0;", f"nl_v_{first_field} = &proxy.f0;"), None),
            "wrong_field": (code.replace(f"nl_v_{first_field} = &nl_v_{first_parent}->f0;", f"nl_v_{first_field} = (nl_node_option *)&nl_v_{first_parent}->f1;"), None),
            "forced_tag": (code.replace(some.group(0), some.group(0) + f" {some[1]}.tag=0;", 1), None),
            "forced_copy": (code.replace(copied.group(0), copied.group(0).split("\n")[0] + f" {copied[1]}.ptr=(const nl_node *){heaps[0]};\nNL_HEAP_COPY", 1), None),
            "missing_write": (code.replace(writes[0], "(void)" + writes[0].split(" = ")[1], 1), None),
            "missing_unlink": (code.replace(writes[1], "(void)" + writes[1].split(" = ")[1], 1), None),
            "cross_free": (code.replace(f"free(nl_v_{frees[1]}.handle);", f"free({heaps[0]});"), None),
            "swapped_release": (code.replace(releases[1], f"NL_HEAP_RELEASE({heaps[0]},{heaps[0]},24);", 1), None),
            "double_free": (code.replace(f"free(nl_v_{frees[2]}.handle);", f"free(nl_v_{frees[2]}.handle); free(NULL);"), None),
            "free_early": (code.replace(end.group(0), f"free((void *){end[1]}); " + end.group(0), 1), "2"),
            "second_null_head_leak": (code.replace(f"free(nl_v_{frees[0]}.handle);", "/* omitted head free */", 1), "2"),
            "hidden_cleanup": (code.replace("NL_HEAP_FINISH();", "free(NULL); NL_HEAP_FINISH();"), "1"),
            "wrong_domain": (re.sub(r"(NL_HEAP_ROOT\(nl_v_\d+,)nl_v_\d+->token", r"\g<1>0", code, count=1), None),
            "wrong_region": (code.replace(releases[1], releases[1].replace(".bytes", ".bytes+1"), 1), None),
            "wrong_reloan": (re.sub(r"(const nl_node \* nl_v_" + tail_ref + r" = )(nl_v_\d+);", r"\g<1>(const nl_node *)" + heaps[0] + r"; (void)\2;", code), None),
        }
        for reason, (altered, fail) in mutations.items():
            assert altered != code, reason
            native(root, name + "_broken_" + reason, altered, definitions, broken=True, fail=fail)

none_start = source.index("                None =>")
some_start = source.index("                Some(tail_bundle)", none_start)
inner_end = source.index("            }\n        },", some_start)
reversed_arms = source[:none_start] + source[some_start:inner_end] + source[none_start:some_start] + source[inner_end:]
variants = {
    "canonical": source,
    "renamed": source.replace("Node", "Cell").replace("next", "link").replace("payload", "datum"),
    "values": source.replace("u8(1)", "u8(11)").replace("u8(2)", "u8(37)"),
    "aliases": source.replace("let head_w = ref_from_ptr(write, ptr_h, stable_h);", "let p=ptr_h;let s=stable_h;let head_w=ref_from_ptr(write,p,s);").replace("head_w@next", "alias@next").replace("replace(alias@next", "let alias=head_w;replace(alias@next"),
    "owner_alias": source.replace("let allocation_h = allocation;", "let owned=allocation;let allocation_h=owned;"),
    "tail_operands": source.replace("let tail_r = ref_from_ptr(read, q, stable_t);", "let p=q;let s=stable_t;let tail_r=ref_from_ptr(read,p,s);"),
    "reverse_inner": reversed_arms,
}
with tempfile.TemporaryDirectory(prefix="two-heap-native-") as directory:
    root = pathlib.Path(directory)
    for name, text in variants.items():
        positive(root, name, text, corrupt=name == "canonical")
print("two-heap native: 7 source variants, 3 outcomes observed/plain under NDEBUG, 17 generated-C corruption controls passed")
