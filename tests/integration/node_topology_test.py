"""Actual-source checked Node topology with read-only native observations.

The independent checked-artifact probe creates expected events. It never calls
codegen. The observer receives addresses of *emitted* C carriers; it cannot
seed, mutate, repair or replace a NewLang object, pointer, Option or operation.
"""
from pathlib import Path
import re
import sys
import tempfile
from checked_c_v0_test import run


def observer(manifest: str) -> str:
    records = []
    arm_count = 0
    for line in manifest.splitlines():
        words = line.split()
        if words[0] == "B":
            name, symbol, kind, scalar, tag, target = words[1:]
            records.append([int(kind), int(symbol), int(tag), int(target), 0, 0, int(scalar)])
        elif words[0] == "W":
            symbol, old_tag, old_target, tag, target = map(int, words[1:])
            records.append([5, symbol, tag, target, old_tag, old_target, 0])
        elif words[0] == "M":
            tag, target = map(int, words[1:])
            records.append([6, 0, tag, target, 0, 0, 0])
        elif words[0] == "R":
            records.append([7, 0, 0, int(words[1]), 0, 0, 0])
        elif words[0] == "A":
            arm_count += 1
        else:
            raise SystemExit(f"unknown checked evidence: {line}")
    if arm_count != 2 or not any(r[0] == 7 for r in records):
        raise SystemExit("missing both checked arms / actual reloan evidence")
    events = ",\n".join("{" + ",".join(map(str, r)) + "}" for r in records)
    return r'''
#include <stdio.h>
#include <stdlib.h>
typedef struct { size_t kind, symbol, tag, target, old_tag, old_target, scalar; } expected_event;
static const expected_event events[] = {
''' + events + r'''
};
static size_t event_index;
static const nl_node *roots[4096];
static const nl_node_option *options[4096];
static const nl_node *const *pointers[4096];
static unsigned root_tags[4096], option_tags[4096];
static size_t root_targets[4096], option_targets[4096], pointer_targets[4096], scalars[4096];
static void require(int condition) {
    if (!condition) { fputs("native topology assertion failed\n", stderr); abort(); }
}
static const nl_node *target(size_t id) {
    require(id < 4096);
    if (id != 0) require(roots[id] != NULL);
    return roots[id];
}
static const expected_event *event(size_t kind) {
    require(event_index < sizeof(events)/sizeof(events[0]));
    const expected_event *e = &events[event_index++];
    require(e->kind == kind && e->symbol < 4096);
    return e;
}
static void option_equal(const nl_node_option *value, size_t tag, size_t id) {
    require(value != NULL && value->tag == tag && value->ptr == target(id));
}
static void observed_bind(size_t frame, size_t symbol, size_t kind, const void *address) {
    const expected_event *e = event(kind);
    require(frame == 0 && symbol == e->symbol && address != NULL);
    if (kind == 1) {
        const nl_node *p = address;
        require(roots[symbol] == NULL);
        for (size_t i = 1; i < 4096; ++i) require(roots[i] != p);
        roots[symbol] = p;
        scalars[symbol] = e->scalar;
        require(p->f1 == e->scalar);
        option_equal(&p->f0, 0, 0);
    } else if (kind == 2) {
        const nl_node *const *p = address;
        require(*p == target(e->scalar));
        pointers[symbol] = p;
        pointer_targets[symbol] = e->scalar;
    } else if (kind == 3) {
        const nl_node_option *p = address;
        option_equal(p, e->tag, e->target);
        for (size_t i = 1; i < 4096; ++i) {
            require(options[i] != p);
            if (roots[i] != NULL) require(&roots[i]->f0 != p);
        }
        options[symbol] = p;
        option_tags[symbol] = (unsigned)e->tag;
        option_targets[symbol] = e->target;
    } else {
        require(kind == 4 && *(const uint8_t *)address == e->scalar);
    }
}
static void observed_replace(const nl_node *root, const nl_node_option *old) {
    const expected_event *e = event(5);
    require(root == target(e->symbol) && old != &root->f0);
    option_equal(old, e->old_tag, e->old_target);
    option_equal(&root->f0, e->tag, e->target);
    root_tags[e->symbol] = (unsigned)e->tag;
    root_targets[e->symbol] = e->target;
}
static void observed_arm(size_t variant, const nl_node *pointer) {
    const expected_event *e = event(6);
    require(variant == e->tag + 1 && pointer == target(e->target));
}
static void observed_reloan(const nl_node *reference) {
    const expected_event *e = event(7);
    require(reference == target(e->target));
    require(reference->f1 == scalars[e->target]);
}
static void observed_finish(void) {
    require(event_index == sizeof(events)/sizeof(events[0]));
    size_t count = 0;
    for (size_t i = 1; i < 4096; ++i) {
        if (roots[i] != NULL) {
            ++count;
            require(roots[i]->f1 == scalars[i]);
            option_equal(&roots[i]->f0, root_tags[i], root_targets[i]);
        }
        if (options[i] != NULL) option_equal(options[i], option_tags[i], option_targets[i]);
        if (pointers[i] != NULL) require(*pointers[i] == target(pointer_targets[i]));
    }
    require(count == 2);
    puts("native topology observations complete");
}
#define NL_NODE_BIND(f,s,k,p) observed_bind(f,s,k,p)
#define NL_NODE_REPLACE(r,o) observed_replace(r,o)
#define NL_NODE_ARM(v,p) observed_arm(v,p)
#define NL_NODE_RELOAN(p) observed_reloan(p)
#define NL_NODE_FINISH() observed_finish()
'''


def native(root: Path, cc: str, name: str, c: str, header: Path | None,
           sanitizer: str, expect_success: bool = True) -> None:
    c_path, exe = root / f"{name}.c", root / name
    c_path.write_text(c, encoding="utf-8")
    command = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-DNDEBUG"]
    if header:
        command.append(f'-DNEWLANG_NODE_OBSERVER="{header}"')
    if sanitizer != "none":
        command += [f"-fsanitize={sanitizer}", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    built = run(command + [str(c_path), "-o", str(exe)])
    if built.returncode:
        raise SystemExit(f"{name}: strict host compile failed: {built.stderr}")
    executed = run([str(exe)])
    if expect_success:
        if executed.returncode or executed.stderr or (header and executed.stdout != "native topology observations complete\n"):
            raise SystemExit(f"{name}: native observations failed: {executed}")
    elif executed.returncode == 0 or "native topology assertion failed" not in executed.stderr:
        raise SystemExit(f"{name}: broken generated topology escaped the native oracle: {executed}")


def positive(root: Path, compiler: str, probe: str, cc: str, name: str,
             text: str, sanitizer: str, corruption_controls: bool = False) -> None:
    source = root / f"{name}.nl"
    source.write_text(text, encoding="utf-8")
    first, second = run([compiler, str(source)]), run([compiler, str(source)])
    if first.returncode or first.stderr or first.stdout != second.stdout or second.returncode:
        raise SystemExit(f"{name}: source-to-C failure/nondeterminism: {first} {second}")
    evidence = run([probe, str(source)])
    if evidence.returncode or evidence.stderr:
        raise SystemExit(f"{name}: independent checked artifact probe failed: {evidence}")
    header = root / f"{name}_observer.h"
    header.write_text(observer(evidence.stdout), encoding="utf-8")
    native(root, cc, name, first.stdout, header, sanitizer)
    native(root, cc, name + "_ordinary", first.stdout, None, sanitizer)
    if corruption_controls:
        c = first.stdout
        changes = [
            ("pointer", r"(nl_ref_\d+ = )&nl_b_0_\d+;", r"\1NULL;"),
            ("tag", r"(nl_node_option nl_v_\d+ = )\{ 1,", r"\1{ 0,"),
            ("copy", r"(nl_node_option nl_v_\d+ = )nl_b_0_\d+\.f0;", r"\1{ 0, NULL };"),
            ("arm", r"switch \(nl_v_\d+\.tag\)", "switch (0u)"),
            ("binder", r"(nl_b_2_\d+ = )nl_v_\d+\.ptr;", r"\1NULL;"),
            ("skip_mutation", r"\*nl_ref_(\d+) = nl_v_(\d+);", r"(void)nl_v_\2;"),
            ("skip_reloan", r"NL_NODE_RELOAN\(nl_ref_\d+\);", "(void)observed_reloan; ((void)0);"),
            ("reloan", r"(nl_ref_\d+ = )nl_b_2_\d+;", r"\1NULL;"),
            ("old_package", r"(nl_v_\d+ = )nl_old_(\d+);", r"\1*nl_ref_\2;"),
        ]
        for reason, pattern, replacement in changes:
            if reason == "old_package":
                matches = list(re.finditer(pattern, c))
                if len(matches) < 2:
                    raise SystemExit("missing generated old package transitions")
                m = matches[-1]
                broken = c[:m.start()] + m.expand(replacement) + c[m.end():]
            else:
                broken, count = re.subn(pattern, replacement, c, count=1)
                if count != 1:
                    raise SystemExit(f"missing generated mutation control: {reason}")
            native(root, cc, name + "_broken_" + reason, broken, header, sanitizer, False)
        roots = [int(line.split()[2]) for line in evidence.stdout.splitlines()
                 if line.startswith("B ") and line.split()[3] == "1"]
        if len(roots) != 2:
            raise SystemExit("missing checked root carriers")
        broken = c.replace("NL_NODE_FINISH();", f"nl_b_0_{roots[1]}.f1 = 123;\nNL_NODE_FINISH();")
        if broken == c:
            raise SystemExit("missing sibling corruption point")
        native(root, cc, name + "_broken_sibling", broken, header, sanitizer, False)


def main() -> None:
    compiler, probe, cc, witness, sanitizer = sys.argv[1:]
    actual = Path(witness).read_text(encoding="utf-8")
    with tempfile.TemporaryDirectory(prefix="newlang-node-topology-") as temp:
        root = Path(temp)
        positive(root, compiler, probe, cc, "primary", actual, sanitizer, True)
        renamed = actual.replace("Node", "Cell").replace("next", "link").replace("payload", "data").replace("u8(2)", "u8(42)").replace("u8(1)", "u8(19)")
        positive(root, compiler, probe, cc, "renamed", renamed, sanitizer)
        repeated = actual.replace("let observed=head@next;", "let repeated_old=loan_write(head@next){|w|replace(w,Option<ptr<Node>>::Some(tail_ptr))};\nlet observed=head@next;")
        positive(root, compiler, probe, cc, "same_variant", repeated, sanitizer)
        reordered = actual.replace("None=>{unit},\n        Some(q)=>{loan_read_ptr(q){|tail_ref|unit}},", "Some(q)=>{loan_read_ptr(q){|tail_ref|unit}},\n        None=>{unit},")
        positive(root, compiler, probe, cc, "arm_order", reordered, sanitizer)
    print("Node native topology: primary, renamed, Some-to-Some; checked-derived read-only pointer/tag/copy/arm/reloan/old-package/sibling assertions; ten broken-code controls rejected")


if __name__ == "__main__":
    main()
