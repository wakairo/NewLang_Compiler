"""Finite real-CHECK/native observations of the independently frozen C247 cohort."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile

here = Path(__file__).resolve().parent
root = here.parents[1]
compiler, probe, gcc, clang, sanitizer, destination = sys.argv[1:]
base = Path(destination)
base.mkdir(parents=True, exist_ok=True)
raw = Path(tempfile.mkdtemp(prefix="run-", dir=base))
freeze = json.loads((here / "equipment-freeze-v1.json").read_text())
for path, expected in freeze["files"].items():
    assert hashlib.sha256((root / path).read_bytes()).hexdigest() == expected, path
oracle = json.loads((here / "oracle-v1.json").read_text())
native = json.loads((here / "native-oracle-v1.json").read_text())
rows = []


def run(command, label, expected=0):
    command = list(map(str, command))
    result = subprocess.run(command, capture_output=True, timeout=120)
    (raw / (label + ".out")).write_bytes(result.stdout)
    (raw / (label + ".err")).write_bytes(result.stderr)
    row = dict(command=command, label=label, status=result.returncode, expected=expected)
    rows.append(row)
    (raw / "observations.json").write_text(json.dumps(rows, indent=2) + "\n")
    if result.returncode != expected:
        (raw / "FIRST.json").write_text(json.dumps(row, indent=2) + "\n")
        raise AssertionError(row)
    return result


def observer(meta, symbols):
    def array(values):
        return ",".join(str(v) for v in values) or "0"
    fields = [meta["initial"], meta["sibling"]]
    if meta["index"]:
        fields.reverse()
    incoming = meta["incoming"]
    olds = [meta["initial"], *incoming[:-1]]
    expected = {"old": "olds[phases-1]", "old_copy": "olds[phases-1]",
                "incoming": "incoming[phases-1]", "during": "incoming[phases-1]",
                "beside": str(meta["sibling"])}
    for name in symbols:
        if name.startswith("actual"):
            expected[name] = "headers[phases-1]"
    cases = "\n".join(
        f"case {symbol}: assert(value=={expected[name]}); ++locals[{i}]; break;"
        for i, (name, symbol) in enumerate(symbols.items())
    )
    return f'''#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t *original[2];
static size_t phases,calls,returns,results,reads,locals[{len(symbols)}];
static const size_t headers[]={{ {array(meta['headers'])} }};
static const size_t incoming[]={{ {array(incoming)} }};
static const size_t olds[]={{ {array(olds)} }};
static const size_t child[]={{ {array([meta['initial'], *incoming])} }};
static void finish(void){{
 assert(phases=={len(meta['headers'])} && calls=={len(incoming)} && returns==calls);
 assert(results==1 && reads==2);
 for(size_t i=0;i<sizeof locals/sizeof locals[0];++i)assert(locals[i]==calls);
 puts("C247 actual physical-original/old/current/sibling/phase/result PASS");
}}
static void bind_original(uint8_t *a,uint8_t *b){{
 assert(!original[0] && !original[1] && a!=b && *a=={fields[0]} && *b=={fields[1]});
 original[0]=a;original[1]=b;assert(atexit(finish)==0);
}}
static void phase(size_t n,uint8_t *a,uint8_t *b){{
 assert(a==original[0] && b==original[1] && phases<{len(meta['headers'])});
 assert(n==headers[phases] && *original[{meta['index']}]==child[phases]);
 assert(*original[{1-meta['index']}]=={meta['sibling']});++phases;
}}
static void local(size_t symbol,size_t value){{
 assert(phases && phases<={len(incoming)});
 switch(symbol){{ {cases} default:assert(0); }}
}}
static void helper_input(size_t n){{
 assert(calls==returns && calls+1==phases && calls<{len(incoming)});
 assert(n==headers[calls] && *original[{meta['index']}]==child[calls]);++calls;
}}
static uint8_t helper_return(uint8_t value){{
 assert(returns+1==calls && value==incoming[returns]);
 assert(*original[{meta['index']}]==child[returns]);++returns;return value;
}}
static void result(size_t value){{
 assert(value=={meta['headers'][-1]} && *original[{meta['index']}]=={incoming[-1]});
 assert(*original[{1-meta['index']}]=={meta['sibling']});++results;
}}
static void read_original(uint8_t *p,uint8_t value){{
 assert((p==original[0] || p==original[1]) && value==*p);
 assert(value==(p==original[{meta['index']}]?{incoming[-1]}:{meta['sibling']}));++reads;
}}
#define NL_LOOP_BIND(a,b) bind_original(a,b)
#define NL_CURRENT_PHASE(n,a,b) phase(n,a,b)
#define NL_CURRENT_LOCAL(s,v) local(s,v)
#define NL_CURRENT_RESULT(n) result(n)
#define NL_LOOP_READ(a,v) read_original(a,v)
'''


native_runs = 0
for name, meta in oracle["cases"].items():
    source = here / "sources" / (name + ".nl")
    run([compiler, "--check", source], name + "-check", meta["target_check"])
    emitted = run([compiler, source], name + "-emit", meta["target_emit"])
    target = raw / (name + "-named.c")
    sentinel = b"C247 original named-output sentinel\n"
    target.write_bytes(sentinel)
    before = target.stat()
    run([compiler, "--emit-c17-to", target, source], name + "-named", meta["target_emit"])
    if meta["target_emit"]:
        assert not emitted.stdout and target.read_bytes() == sentinel
        after = target.stat()
        assert (before.st_ino, before.st_size, before.st_mtime_ns) == (
            after.st_ino, after.st_size, after.st_mtime_ns)
        continue
    assert target.read_bytes() == emitted.stdout
    if meta["category"] == "control":
        assert emitted.stdout == (here / "evidence" / (name + ".c17")).read_bytes()
        continue
    proof = run([probe, source], name + "-owned")
    assert b"public-owned=YES private-current=0" in proof.stdout
    symbols = {n: int(i) for n, i in re.findall(
        r"C245 LOCAL name=(\w+) symbol=(\d+)", proof.stdout.decode())}
    assert {"old", "old_copy", "incoming", "during", "beside"} <= symbols.keys()
    header = raw / (name + "-observer.h")
    header.write_text(observer(native[name], symbols))
    plain = raw / (name + ".c")
    plain.write_bytes(emitted.stdout)
    pattern = r"(static uint8_t nl_fn_\d+\((?:const )?size_t (nl_local_\d+)\)\n\{\n)(.*?)(^\})"

    def passive(match):
        body, count = re.subn(r"\breturn ([^;\n]+);", r"return helper_return(\1);", match[3])
        assert count == 1
        return match[1] + "    helper_input(" + match[2] + ");\n" + body + match[4]

    observed, count = re.subn(pattern, passive, emitted.stdout.decode(), flags=re.S | re.M)
    assert count == 1
    observed_path = raw / (name + "-observed.c")
    observed_path.write_text(observed)
    compilers = [("gcc", gcc)] + ([] if gcc == clang else [("clang", clang)])
    for tag, cc in compilers:
        for opt in ["O0", "O2"]:
            for active in [False, True]:
                label = name + "-" + tag + "-" + opt + ("-passive" if active else "-raw")
                exe = raw / label
                flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-UNDEBUG", "-" + opt]
                if active:
                    flags += ["-include", header]
                if sanitizer != "none":
                    flags += ["-fsanitize=" + sanitizer, "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
                run([cc, *flags, observed_path if active else plain, "-o", exe], label + "-build")
                result = run([exe], label + "-native")
                assert not result.stderr
                assert (b"C247 actual physical-original" in result.stdout) == active
                native_runs += 1
                exe.unlink()
print("C247 finite CHECK/current/C17/named/negative/control/native PASS runs", native_runs)
print("C247 raw", raw)
