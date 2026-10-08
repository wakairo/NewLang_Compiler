"""Checked-only data-flow evidence plus independent read-only native oracle."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

compiler, probe, cc, fixture, sanitizer = sys.argv[1:]
support = pathlib.Path(__file__).resolve().parent
actual = pathlib.Path(fixture).read_text()

def run(args, **kwargs):
    return subprocess.run(args, capture_output=True, text=True, **kwargs)

def native(root, name, code, expectations, broken=False, plain=False):
    source, exe = root / (name + ".c"), root / name
    source.write_text(code)
    head, tail, _, _ = expectations
    command = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-DNDEBUG", str(source), "-o", str(exe)]
    if not plain:
        command += ["-fno-builtin-malloc", "-fno-builtin-free", str(support / "allocated_platform.c"), "-Wl,--wrap=malloc", "-Wl,--wrap=free", f"-DEXPECT_HEAD={head}", f"-DEXPECT_TAIL={tail}", '-DNEWLANG_HEAP_OBSERVER="' + str(support / "allocated_observer.h") + '"']
    if sanitizer != "none":
        command += [f"-fsanitize={sanitizer}", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    built = run(command)
    assert built.returncode == 0, (name, built.stderr)
    executed = run([str(exe)])
    if broken:
        assert executed.returncode != 0, (name, "corrupted native lifecycle escaped oracle")
    else:
        assert executed.returncode == 0 and not executed.stderr, (name, executed)
        if not plain:
            env = dict(os.environ, NEWLANG_TEST_ALLOCATION_FAIL="1")
            failed = run([str(exe)], env=env)
            assert failed.returncode == 0 and not failed.stderr, (name, "None path failed", failed)

def positive(root, name, text, corrupt=False):
    source = root / (name + ".nl")
    source.write_text(text)
    checked = run([probe, str(source)])
    assert checked.returncode == 0, checked.stderr
    expected = tuple(map(int, checked.stdout.split()))
    emitted = run([compiler, str(source)])
    again = run([compiler, str(source)])
    assert emitted.returncode == 0 and emitted.stdout == again.stdout and not emitted.stderr
    code = emitted.stdout
    # E2: follow the emitted Copy from the exact checked symbol to reloan.
    # The value expectations/symbol IDs above come from checked artifact views,
    # never a source-text extraction or emitter-produced oracle.
    p, stable = expected[2:]
    copies = re.findall(r"const nl_node \* nl_v_(\d+) = nl_b_\d+_" + str(p) + r";", code)
    reloan = re.search(r"const nl_node \*nl_v_(\d+) = nl_v_(\d+);\nNL_HEAP_RELOAN", code)
    assert reloan and reloan.group(2) in copies, "reloan did not use selected checked ptr"
    sc = re.findall(r"const nl_domain \* nl_v_(\d+) = nl_b_\d+_" + str(stable) + r";", code)
    assert re.search(r"NL_HEAP_RELOAN\(nl_v_" + reloan.group(1) + r",nl_v_(\d+)->token\)", code).group(1) in sc
    assert "sizeof(nl_node)==24" in code and "_Alignof(nl_node)==8" in code
    native(root, name, code, expected)
    native(root, name + "_plain", code, expected, plain=True)
    if corrupt:
        replacements = re.findall(r"\*nl_ref_(\d+) = nl_v_(\d+);", code)
        assert len(replacements) == 2
        free = re.search(r"free\(nl_v_\d+\.handle\);", code).group(0)
        mutations = {
            "missing_release": code.replace(free, "/* omitted platform release */"),
            "extra_release": code.replace(free, free + "\nfree(NULL);"),
            "no_unlink": code.replace("*nl_ref_" + replacements[1][0] + " = nl_v_" + replacements[1][1] + ";", "(void)nl_v_" + replacements[1][1] + "; /* unlink omitted */"),
            "wrong_tag": code.replace(" = { 1, nl_v_", " = { 0, nl_v_", 1),
            "wrong_pointer": re.sub(r"(nl_node_option (nl_v_\d+) = \{ 1, nl_v_\d+ \};)", r"\1 \2.ptr=NULL;", code, count=1),
            "wrong_payload": code.replace("uint8_t nl_v_9 = 2;", "uint8_t nl_v_9 = 3;"),
            "release_before_end": code.replace("NL_HEAP_END(nl_v_", "free(" + re.search(r"nl_allocation (nl_b_\d+_\d+) =", code).group(1) + ".handle); NL_HEAP_END(nl_v_", 1),
        }
        for reason, altered in mutations.items():
            assert altered != code, reason
            native(root, name + "_broken_" + reason, altered, expected, broken=True)

with tempfile.TemporaryDirectory(prefix="allocated-native-") as directory:
    root = pathlib.Path(directory)
    positive(root, "primary", actual, True)
    positive(root, "renamed", actual.replace("Node", "Cell").replace("next", "link").replace("payload", "datum"))
    positive(root, "values", actual.replace("u8(1)", "u8(11)").replace("u8(2)", "u8(37)"))
    positive(root, "direct_tail", actual.replace("ref_from_ptr(read, q, stable)", "ref_from_ptr(read, tail, stable)"))
    positive(root, "stability_alias", actual.replace("let access = ref_from_ptr(read, q, stable);", "let borrowed=stable; let access=ref_from_ptr(read,q,borrowed);"))
    # Finite copied-link match arm source order is immaterial to tag selection.
    positive(root, "arm_order", actual.replace("            None => { unit },\n", "").replace("        };\n\n        let old_some", "            None => {unit},\n        };\n\n        let old_some", 1))
print("allocated native lifecycle: real success/None, checked operand lowering, read-only identity/value oracle and corruption controls passed")
