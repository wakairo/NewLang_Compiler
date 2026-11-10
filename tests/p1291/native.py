"""Actual source result, strict raw C17 first, then passive H0/1/2 probes."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

cli, owned, cc, sanitizer, destination = sys.argv[1:]
here = Path(__file__).resolve().parent
oracle = json.loads((here / "oracle.json").read_text())
parent = Path(destination)
parent.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix="native-", dir=parent))
rows = []


def run(args, label, expected=0):
    args = list(map(str, args))
    result = subprocess.run(args, capture_output=True)
    (out / (label + ".stdout")).write_bytes(result.stdout)
    (out / (label + ".stderr")).write_bytes(result.stderr)
    rows.append({"label": label, "argv": args, "status": result.returncode})
    (out / "commands.json").write_text(json.dumps(rows, indent=2) + "\n")
    assert result.returncode == expected, (label, result.returncode,
                                            result.stderr, out)
    return result.stdout


flags = [cc, "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
if sanitizer != "none":
    flags += ["-fsanitize=" + ("address,undefined" if sanitizer == "address"
                               else sanitizer),
              "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
              "-fno-pie", "-no-pie"]
for name, values in [
        ("A-original.nl", (53, 173, 197, 0, 229, 11, 23)),
        ("B-index-value-control.nl", (61, 131, 181, 1, 227, 7, 19))]:
    old, incoming, sibling, field, front, h0, h1 = values
    source = here / "sources" / name
    assert hashlib.sha256(source.read_bytes()).hexdigest() == \
        oracle["Stage1"]["sources"][name]
    run([cli, "--check", source], name + "-CHECK")
    facts = run([owned, source, old, incoming, sibling, field],
                name + "-owned").decode()
    bindings = {n: int(s) for n, s in re.findall(r"BIND (\w+) (\d+)", facts)}
    generated = run([cli, source], name + "-C17")
    c = out / (name + ".c")
    c.write_bytes(generated)
    named = out / (name + "-named.c")
    run([cli, "--emit-c17-to", named, source], name + "-named")
    assert named.read_bytes() == generated
    text = generated.decode()
    record = re.findall(r"nl_copy_record (nl_b_(\d+)_\d+)\s*=", text)
    assert len(record) == 1
    record_name, frame = record[0][0], int(record[0][1])

    def variable_for(name, wanted_frame=None):
        symbol = bindings[name]
        matches = re.findall(r"\buint8_t (nl_b_(\d+)_" + str(symbol) +
                             r")\s*=", text)
        if wanted_frame is not None:
            matches = [m for m in matches if int(m[1]) == wanted_frame]
        assert len(matches) == 1, (name, symbol, matches)
        return matches[0][0], int(matches[0][1])

    names = ["observed_byte", "source_copy", "old_selected", "old_copy",
             "current_byte", "sibling_byte"]
    for n in names:
        variable_for(n, frame)
    arm_var, arm_frame = variable_for("arm_copy")
    outer = [variable_for(n) for n in
             ["completed", "result_copy", "continuation_copy"]]
    outer_frame = outer[0][1]
    assert all(f == outer_frame for _, f in outer)
    assert len({frame, arm_frame, outer_frame}) == 3
    projected = re.findall(r"const uint8_t \* (nl_v_\d+) = &nl_v_\d+->f1;",
                           text)
    assert len(projected) == 1
    loads = re.findall(r"uint8_t (nl_v_\d+) = \*" + projected[0] + ";", text)
    assert len(loads) == 1
    from_h, from_old = {loads[0]}, set()
    var = r"nl_(?:v_\d+|b_\d+_\d+)"
    assignment = re.compile(r"\b(" + var + r")\s*=\s*(" + var + r");")
    call = re.compile(r"uint8_t (" + var + r") = nl_copy_state_\d+\(&" +
                      re.escape(record_name) + r"\.f([01]),(" + var + r")\);")
    calls = []
    for line in text.splitlines():
        m = assignment.search(line)
        if m:
            if m[2] in from_h:
                from_h.add(m[1])
            if m[2] in from_old:
                from_old.add(m[1])
        m = call.search(line)
        if m:
            assert int(m[2]) == field
            calls.append(m[3] in from_h)
            from_old.add(m[1])
    assert calls == [True]
    assert "uint8_t old=*target;*target=next;" in text
    assert variable_for("old_copy", frame)[0] in from_old
    assert arm_var in from_old and all(v in from_old for v, _ in outer)
    assert text.count("default: abort();") == 1
    (out / (name + ".def-use.json")).write_text(json.dumps({
        "actual_H_load_to_T_call": True, "actual_old_call_to_arm_copy": True,
        "actual_old_through_three_matches_to_continuation": True,
        "T_original": record_name, "declared_field": field,
        "frames": {"T": frame, "arm": arm_frame, "continuation": outer_frame},
        "C_sha256": hashlib.sha256(generated).hexdigest()}, indent=2) + "\n")
    for opt in ["O0", "O2"]:
        label = name + "-" + opt
        raw = out / (label + "-raw")
        run(flags + ["-" + opt, c, "-o", raw], label + "-raw-build")
        run([raw], label + "-raw")
        wrapper = out / (label + "-physical.c")
        symbols = [bindings[n] for n in names]
        results = [bindings[n] for n in
                   ["completed", "result_copy", "continuation_copy"]]
        definitions = {
            "OLD": old, "NEW": incoming, "SIBLING": sibling, "FIELD": field,
            "FRONT": front, "H0": h0, "H1": h1, "FRAME": frame,
            "ARM_FRAME": arm_frame, "ARM_SYMBOL": bindings["arm_copy"],
            "OUTER_FRAME": outer_frame}
        prefix = "".join("#define P1291_" + key + " " + str(value) + "\n"
                         for key, value in definitions.items())
        prefix += "#define P1291_SYMBOLS {" + ",".join(map(str, symbols)) + "}\n"
        prefix += "#define P1291_RESULTS {" + ",".join(map(str, results)) + "}\n"
        wrapper.write_text(prefix + "#define main p1291_guest_main\n" +
                           "#define NEWLANG_NODE_OBSERVER " +
                           json.dumps(str(here / "native_observer.h")) + "\n" +
                           "#define NEWLANG_HEAP_OBSERVER " +
                           json.dumps(str(here / "native_observer.h")) + "\n" +
                           "#include " + json.dumps(str(c)) + "\n#undef main\n" +
                           "int main(void){for(size_t q=0;q<3;++q){"
                           "memset(&p1291,0,sizeof(p1291));p1291.quota=q;"
                           "assert(p1291_guest_main()==0 && p1291.finished);"
                           "printf(\"H%zu frees=%zu writes=%zu continuation=%zu\\n\","
                           "q,p1291.frees,p1291.writes,p1291.outer);}return 0;}\n")
        physical = out / (label + "-physical")
        run(flags + ["-" + opt, wrapper, "-o", physical], label + "-physical-build")
        run([physical], label + "-physical")
print("P1291 genuine result def-use, strict raw/native and passive H0/1/2 PASS", out)
