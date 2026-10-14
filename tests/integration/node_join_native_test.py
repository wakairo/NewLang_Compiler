"""Real source/public-proof/C17/native; passive observer never writes guest.

Guest C bytes are identical for plain/observed builds. Mutant C is deliberately
separate evidence, never a success substitute. Expected events come from a
public-only source artifact probe, not emitter output or injected runtime data.
"""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from node_topology_test import observer


def execute(command):
    return subprocess.run(command, capture_output=True, text=True)


def observed_header(manifest):
    transformed = []
    for line in manifest.splitlines():
        w = line.split()
        if w[0] == "B":
            private, symbol, kind, scalar, tag, target = map(int, w[1:])
            transformed.append(f"B role {symbol} {kind + private * 8} {scalar} {tag} {target}")
        else:
            transformed.append(line)
    # Two checked arm records; M represents only the actual source-selected
    # runtime expectation, not a restriction on emitted guest branch code.
    transformed += ["A 1 0", "A 2 0"]
    h = observer("\n".join(transformed))
    h = h.replace("event(kind);", "event(kind + (frame != 0 ? 8 : 0));")
    h = h.replace("frame == 0 && symbol == e->symbol", "(frame == 0 || kind == 3) && symbol == e->symbol")
    h = h.replace("option_equal(p, e->tag, e->target);\n        for", "option_equal(p, e->tag, e->target);\n        if (frame != 0) return; /* branch-local borrow ends here */\n        for")
    h = h.replace("require(root == target(e->symbol) && old != &root->f0);", "require(root == target(e->symbol) && old != &root->f0);\n    printf(\"replace root=%p old=%u/%p new=%u/%p sibling=%u\\n\", (const void *)root, old->tag, (const void *)old->ptr, root->f0.tag, (const void *)root->f0.ptr, (unsigned)root->f1);")
    h = h.replace("require(variant == e->tag + 1 && pointer == target(e->target));", "require(variant == e->tag + 1 && pointer == target(e->target));\n    printf(\"branch=%zu pointer=%p\\n\", variant, (const void *)pointer);")
    return h


def native(root, cc, name, code, header, opt, sanitizer, success=True):
    cfile, exe = root / (name + ".c"), root / name
    cfile.write_text(code)
    command = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", opt]
    if header:
        command += [f'-DNEWLANG_NODE_OBSERVER="{header}"']
    if sanitizer != "none":
        command += [f"-fsanitize={sanitizer}", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    built = execute(command + [str(cfile), "-o", str(exe)])
    if built.returncode:
        raise AssertionError((name, "compile", built.stderr))
    ran = execute([str(exe)])
    if success:
        assert ran.returncode == 0 and not ran.stderr, (name, ran)
        if header:
            assert "native topology observations complete" in ran.stdout
        else:
            assert not ran.stdout
    else:
        assert ran.returncode != 0 and "native topology assertion failed" in ran.stderr, (name, ran)
    return {"name": name, "optimization": opt, "sanitizer": sanitizer,
            "compile_exit": built.returncode, "exit": ran.returncode,
            "stdout": ran.stdout, "stderr": ran.stderr,
            "C_sha256": hashlib.sha256(code.encode()).hexdigest()}


def main():
    compiler, probe, cc, fixtures, sanitizer = sys.argv[1:]
    fixtures = Path(fixtures)
    records = []
    with tempfile.TemporaryDirectory(prefix="newlang-p436-") as d:
        root = Path(d)
        for case, variant in [("node_join_same_locator", 2), ("node_join_native_none", 1)]:
            source = fixtures / (case + ".nl")
            emitted = execute([compiler, str(source)])
            again = execute([compiler, str(source)])
            assert emitted.returncode == 0 and not emitted.stderr and emitted.stdout == again.stdout
            assert "switch (" in emitted.stdout and "case 0:" in emitted.stdout and "case 1:" in emitted.stdout
            proof = execute([probe, str(source)])
            assert proof.returncode == 0 and not proof.stderr
            header = root / (case + ".h")
            header.write_text(observed_header(proof.stdout))
            runs = []
            for opt in ["-O0", "-O2"]:
                for observed in [False, True]:
                    result = native(root, cc, case + opt + str(observed), emitted.stdout,
                                    header if observed else None, opt, sanitizer)
                    if observed:
                        assert f"branch={variant} " in result["stdout"]
                    runs.append(result)
            mutants = []
            if case == "node_join_native_none":
                c = emitted.stdout
                pos = c.index("switch (")
                end = c.index("\nnl_node_option", c.index("\n}\n", pos))
                segment = c[pos:end]
                roots = [line.split()[2] for line in proof.stdout.splitlines()
                         if line.startswith("B ") and line.split()[3] == "1"]
                assert len(roots) == 2
                changes = [
                    ("wrong_root", r"(&nl_b_0_)" + roots[0] + r"(\.f0)", r"\g<1>" + roots[1] + r"\2"),
                    ("wrong_arm", r"switch \(nl_v_\d+\.tag\)", "switch (1u)"),
                    ("wrong_tag", r"(nl_node_option nl_v_\d+ = )\{ 0, NULL \}", r"\1{ 1, NULL }"),
                    ("omitted_write", r"\*nl_ref_\d+ = (nl_v_\d+);", r"(void)\1;"),
                    ("old_copy", r"(nl_v_\d+ = )nl_old_(\d+);", r"\1*nl_ref_\2;"),
                ]
                for label, pattern, replacement in changes:
                    mutated, count = re.subn(pattern, replacement, segment, count=1)
                    assert count == 1, label
                    broken = c[:pos] + mutated + c[end:]
                    mutants.append(native(root, cc, label, broken, header, "-O2", sanitizer, False))
                broken = c.replace("NL_NODE_FINISH();", f"nl_b_0_{roots[0]}.f1 = (uint8_t)(nl_b_0_{roots[0]}.f1 + 1);\nNL_NODE_FINISH();")
                mutants.append(native(root, cc, "sibling", broken, header, "-O2", sanitizer, False))
            records.append({"source": source.name, "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
                            "CLI_exit": emitted.returncode, "C_bytes": len(emitted.stdout.encode()),
                            "C_sha256": hashlib.sha256(emitted.stdout.encode()).hexdigest(),
                            "public_probe_exit": proof.returncode, "checked_expectations": proof.stdout,
                            "runs": runs, "C_mutants": mutants})
    print(json.dumps({"host": cc, "sanitizer": sanitizer, "cases": records}, indent=2))


if __name__ == "__main__":
    main()
