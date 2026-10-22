"""R720 standalone ordinary CLI / public owned / strict C17 and passive native."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

FROZEN = {
    "a_true": ("371d7669738523b27a0771b7fe5a4c61b42c0d15c51347ed38c918e3efc471f2", "A", True, [73,142], [73,142], 0, 0, 1),
    "a_false": ("4527029967daabde6ba6fd0183217149d0f810271297c467c4862945ab75312c", "A", False, [73,142], [73,58], 1, 1, 0),
    "b_true": ("e789c5cefb50ec085221d6a03ccffccec70267f53fdf63dcbe7a8d82e694747d", "B", True, [201,29], [166,29], 1, 1, 0),
    "b_false": ("aa45d052f931b4e29d33848b55474584a0940982bf5aa61ac3aade7f21cdc1c0", "B", False, [201,29], [93,29], 1, 0, 1),
}
NEGATIVE = "65695a9b2ea0581d87ec7927c8b5854f4a3fe344fea2ddacf7aec4553647f916"


def cmd(*args):
    return subprocess.run([str(x) for x in args], capture_output=True)


def require(test, reason):
    if not test:
        raise AssertionError(reason)


def find_one(text, regex, low=0, high=None):
    found = list(re.finditer(regex, text[low:high]))
    require(len(found) == 1, f"wanted one {regex!r}: found {len(found)}")
    return found[0].start()+low, found[0].end()+low, found[0]


def passive(code, proof, family, chosen, start, end, writes, post_flag, terminal):
    base = proof["base"]
    index = proof["index"]
    expected_index = 1 if family == "A" else 0
    require(index == expected_index, "public projection index")
    ctor_start, ctor_end, _ = find_one(code, rf"(?:const )?nl_type_\d+ nl_local_{base} = \{{[^;]+;")
    added = {}
    def insert(where, text):
        added[where] = added.get(where, "")+text
    # atexit observer deliberately retains only counters. It does NOT read
    # function-local memory after main returns or change any guest values.
    init = "".join(f"\n    const volatile uint8_t *const r720_field_{i} = &nl_local_{base}.f{i};"
                   for i in range(2))
    init += "\n    if ("+" || ".join(f"*r720_field_{i} != {v}" for i,v in enumerate(start))+") exit(91);"
    init += "\n    if (atexit(r720_end)) exit(91);"
    insert(ctor_end, init)
    h1, h2, _ = find_one(code, r"if \(\(nl_less_left_\d+ < nl_less_right_\d+\)\) \{")
    split = code.find("    } else {", h2)
    require(split >= 0, "real ELSE")
    finish = code.find("\n    }\n", split)
    require(finish >= 0, "real IF end")
    arms = [(h2, split), (split + len("    } else {"), finish)]
    selected = 0 if chosen else 1
    for arm, (lo, hi) in enumerate(arms):
        insert(lo, f"\n        r720_arm={arm};")
        capture = list(re.finditer(r"const uint8_t nl_replace_old_(\d+) = \*(nl_local_\d+);", code[lo:hi]))
        expected_count = 0 if family == "A" and arm == 0 else 1
        require(len(capture) == expected_count, "source event/old count")
        current = list(start)
        for m in capture:
            cend = lo+m.end()
            ref = m.group(2)
            stmt_lo = cend
            sm = re.search(rf"\*{ref} = ([^;]+);", code[stmt_lo:hi])
            require(sm is not None, "actual ref store")
            old = start[index]
            incoming = 58 if family == "A" else (166 if arm == 0 else 93)
            state_before = " || ".join(f"*r720_field_{i} != {v}" for i,v in enumerate(current))
            insert(cend, f"\n        if ({ref} != r720_field_{index} || nl_replace_old_{m.group(1)} != {old} || {state_before}) exit(91);")
            current[index] = incoming
            state_after = " || ".join(f"*r720_field_{i} != {v}" for i,v in enumerate(current))
            insert(stmt_lo+sm.end(), f"\n        ++r720_changes; if ({state_after}) exit(91);")
        if arm == (0 if family == "A" else 1):
            returns = list(re.finditer(r"\breturn\s*;", code[lo:hi]))
            require(len(returns) == 1, "one actual terminal return")
            insert(lo+returns[0].start(),
                   "\n        r720_return=1;\n        if (r720_arm != "+str(arm)+") exit(91);\n        ")
    # Normal branch must reach the original source's old-Copy binding/read;
    # terminal must bypass it, verified through an atexit flag.
    out = code.find(f"const uint8_t nl_local_", finish)
    require(out >= 0, "post-IF old Copy binding")
    result_re = rf"const uint8_t nl_local_\d+ = nl_if_result_{proof['if']};"
    p1,p2,_ = find_one(code, result_re, finish)
    initial_old = start[index]
    insert(p2, f"\n    r720_post=1; if (r720_arm != {selected} || r720_changes != {writes} || *r720_field_{index} != {end[index]} || *r720_field_{1-index} != {end[1-index]} || nl_if_result_{proof['if']} != {initial_old}) exit(91);")
    header = "#include <stdlib.h>\nstatic unsigned r720_arm=99,r720_changes,r720_post,r720_return;\n"
    header += ("static void r720_end(void){if(r720_arm!="+str(selected)+
               " || r720_changes!="+str(writes)+" || r720_post!="+str(0 if terminal else 1)+
               " || r720_return!="+str(terminal)+") _Exit(91);}\n")
    for position in sorted(added, reverse=True):
        code = code[:position]+added[position]+code[position:]
    return header+code


def exercise(compiler, probe, compiler_c, sanitizer, source_dir):
    src = Path(source_dir)
    report = {}
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        for name, (sha, family, selected_true, initial, post, writes, post_flag, terminal) in FROZEN.items():
            file = src / f"{name}.nl"
            require(hashlib.sha256(file.read_bytes()).hexdigest() == sha, "SHA drift "+name)
            check = cmd(compiler, "--check", file)
            require(check.returncode == 0 and not check.stdout and not check.stderr,
                    f"CHECK {name}: {check.returncode} {check.stderr!r}")
            p = cmd(probe, file, family)
            require(p.returncode == 0 and not p.stderr, f"PROOF {name}: {p.returncode} {p.stderr!r}")
            proof = json.loads(p.stdout)
            code_run = cmd(compiler, file)
            require(code_run.returncode == 0 and code_run.stdout and not code_run.stderr,
                    f"EMIT {name}: {code_run.returncode} {code_run.stderr!r}")
            raw = code_run.stdout.decode("utf-8")
            observed = passive(raw, proof, family, selected_true, initial, post,
                               writes, post_flag, terminal)
            for label, payload in [("untouched",raw),("passive",observed)]:
                c = tmp / f"{name}-{label}.c"
                c.write_text(payload)
                for opt in ["-O0","-O2"]:
                    exe = tmp / f"{name}-{label}-{opt[1:]}"
                    flags = [compiler_c,"-std=c17","-Wall","-Wextra","-Wpedantic",
                             "-Werror",opt,c,"-o",exe]
                    if sanitizer != "none":
                        flags += ["-fsanitize="+sanitizer,"-fno-sanitize-recover=all",
                                  "-fno-omit-frame-pointer","-fno-pie","-no-pie"]
                    cc = cmd(*flags)
                    require(cc.returncode == 0,
                            f"C17 {name} {label} {opt}: {cc.stderr.decode(errors='replace')}")
                    run = cmd(exe)
                    require(run.returncode == 0 and not run.stdout and not run.stderr,
                            f"NATIVE {name} {label} {opt}: {run.returncode}, {run.stderr!r}")
            report[name] = dict(check="PASS",owned="PASS",C17="PASS",
                                native="untouched+passive O0/O2 PASS",
                                sha256_C17=hashlib.sha256(code_run.stdout).hexdigest())
        bad = src/"return_type_bad.nl"
        require(hashlib.sha256(bad.read_bytes()).hexdigest() == NEGATIVE,
                "negative SHA drift")
        for args in [["--check"],[]]:
            fail = cmd(compiler, *args, bad)
            require(fail.returncode == 3 and not fail.stdout and fail.stderr,
                    f"wrong-return type: {fail}")
        output = tmp/"preserve.c"
        output.write_bytes(b"R720_EXISTING_OUTPUT\n")
        rejected = cmd(compiler, "--emit-c17-to",output,bad)
        require(rejected.returncode == 3 and not rejected.stdout
                and output.read_bytes() == b"R720_EXISTING_OUTPUT\n",
                "negative named-output rollback")
        report["return_type_bad"] = "SEMANTIC_REJECT_NO_C_ROLLBACK"
        scope = src/"scope_escape.nl"
        require(hashlib.sha256(scope.read_bytes()).hexdigest() ==
                "b536b80549881a49dc566746082b8aabe170635c0d168de3ef19d9fb38a6c6c2",
                "supplementary scope negative SHA drift")
        for args in [["--check"], []]:
            failure = cmd(compiler, *args, scope)
            require(failure.returncode == 3 and not failure.stdout and failure.stderr,
                    f"nonescaping scoped ref source expected semantic reject: {failure}")
        output.write_bytes(b"R720_SCOPE_KEEP\\n")
        failure = cmd(compiler, "--emit-c17-to", output, scope)
        require(failure.returncode == 3 and not failure.stdout and
                output.read_bytes() == b"R720_SCOPE_KEEP\\n",
                "escaped scoped ref emitted C or overwrote output")
        report["scope_escape"] = "SEMANTIC_REJECT_NO_C_ROLLBACK"
    print(json.dumps(report,sort_keys=True))


if __name__ == "__main__":
    exercise(*sys.argv[1:])
