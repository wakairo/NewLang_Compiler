"""R708 independent finite CHECK -> authentic source-issued public history -> C17/native.
Instrumented build is read-only observation of actual generated guest memory.
The byte-for-byte original generated C17 is always built and run separately.
"""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

CASES = {
    "A_caller_original.nl": {
        "sha": "076142fd4555bae19933257517eab65fb818b2acb67694bf00b65bb994c6115f",
        "initial": ((7, 31), (40, 8)),
        "index": (1,),
        "incoming": (93,),
        "old": (31,),
        "post": ((7, 93), (40, 8)),
        "direct": {"index": 0, "old": 40, "post": ((7, 93), (31, 8))},
    },
    "B_two_invocations.nl": {
        "sha": "531bc4557b800c24940ccddca6d80a23eab16fbbed68ce1f1784db40eff786b3",
        "initial": ((77, 16), (21, 66)),
        "index": (0, 1),
        "incoming": (98, 77),
        "old": (77, 66),
        "post": ((98, 16), (21, 77)),
    },
}
NEGATIVE_SHA = "d998bd869873006e4f0b0c2a96ee1f44032b6f6d879a0df6f0f00b63f88b7a0b"


def execute(argv):
    return subprocess.run([str(x) for x in argv], capture_output=True)


def require(condition, message):
    if not condition:
        raise AssertionError(str(message))


def only(pattern, text):
    matches = list(re.finditer(pattern, text))
    require(len(matches) == 1, "expected unique emitted C pattern: " + pattern
            + " (found " + str(len(matches)) + ")")
    return matches[0]


def witness(code, facts, oracle):
    calls = facts["calls"]
    direct = facts["direct"]
    expected = len(oracle["index"])
    require(len(calls) == expected and len(calls) in (1, 2), "call coverage")
    require([x["index"] for x in calls] == list(oracle["index"]),
            "original field index from public issue")
    require(all(x["parent_before"] != x["parent_after"] and
                x["child_before"] != x["child_after"] for x in calls),
            "source-issued before/after facts")
    if expected == 1:
        require(len(direct) == 1 and direct[0]["index"] == oracle["direct"]["index"],
                "independent secondary original direct-write selection")
        require(calls[0]["root"] != direct[0]["root"],
                "two original physical root identifiers")
        roots = (calls[0]["symbol"], direct[0]["symbol"])
    else:
        require(len(direct) == 0 and calls[0]["root"] != calls[1]["root"]
                and calls[0]["function"] == calls[1]["function"],
                "two distinct original call sites, same helper")
        roots = tuple(x["symbol"] for x in calls)
    require(roots[0] != roots[1], "two independent lexical original symbols")
    state = {roots[0]: list(oracle["initial"][0]),
             roots[1]: list(oracle["initial"][1])}
    inserts = {}

    def append(match, code_fragment):
        inserts[match.end()] = inserts.get(match.end(), "") + "\n" + code_fragment + "\n"

    def bad(values):
        return " || ".join(
            "*r708_ob_%s_%s != %s" % (symbol, idx, v)
            for symbol, vector in values.items() for idx, v in enumerate(vector)
        )

    for symbol, vector in state.items():
        declaration = only(
            rf"(?:const )?nl_type_\d+ nl_local_{symbol} = \{{[^;]*;",
            code,
        )
        lines = [f"r708_ob_{symbol}_{i} = &nl_local_{symbol}.f{i};"
                 for i in range(2)]
        lines.append(f"if ({bad({symbol: vector})}) abort();")
        append(declaration, "\n    ".join(lines))
    representative = calls[0]
    fn = representative["function"]
    formal_ref = f'nl_local_{representative["ref"]}'
    # 'ref' is the caller actual. Formal reference id is independently
    # recovered from the generated callee signature rather than guessed.
    arg_symbol = representative["value_formal"]
    callee = only(
        rf"static uint8_t nl_fn_{fn}\(uint8_t \*nl_local_(\d+), uint8_t nl_local_{arg_symbol}\)\n\{{",
        code,
    )
    callee_ref = f"nl_local_{callee.group(1)}"
    body_old_node = representative["change"]
    expected_pre = {key: value[:] for key, value in state.items()}
    checks_entry, checks_old, checks_post = [], [], []
    for index, call in enumerate(calls):
        require(call["function"] == fn and call["change"] == body_old_node,
                "same independently observed callee and event node")
        sym = call["symbol"]
        field = oracle["index"][index]
        require(expected_pre[sym][field] == oracle["old"][index],
                "frozen old Copy oracle")
        checks_entry.append(
            f"if (r708_calls == {index + 1} && "
            f"({callee_ref} != r708_ob_{sym}_{field} || "
            f"nl_local_{arg_symbol} != {oracle['incoming'][index]} || "
            f"{bad(expected_pre)})) abort();"
        )
        checks_old.append(
            f"if (r708_calls == {index + 1} && "
            f"nl_replace_old_{body_old_node} != {oracle['old'][index]}) abort();"
        )
        expected_pre[sym][field] = oracle["incoming"][index]
        require(tuple(tuple(expected_pre[s]) for s in roots) ==
                tuple(tuple(pair) for pair in oracle["post"]) or index < expected - 1,
                "intermediate state only after final invocation")
        checks_post.append(
            f"if (r708_calls == {index + 1} && ({bad(expected_pre)})) abort();"
        )
        callstmt = only(
            rf"const uint8_t nl_call_{call['call']} = "
            rf"nl_fn_{fn}\(nl_local_{call['ref']}, [^;]+;",
            code,
        )
        append(callstmt,
               f"if (r708_calls != {index + 1} || "
               f"nl_call_{call['call']} != {oracle['old'][index]} || "
               f"{bad(expected_pre)}) abort();")
    append(callee, "++r708_calls;\n    " + "\n    ".join(checks_entry))
    old = only(
        rf"const uint8_t nl_replace_old_{body_old_node} = \*{callee_ref};",
        code,
    )
    append(old, "\n    ".join(checks_old))
    changed = only(rf"\*{callee_ref} = nl_local_{arg_symbol};", code)
    append(changed, "\n    ".join(checks_post))
    if "direct" in oracle:
        item = direct[0]
        require(expected_pre[roots[1]][item["index"]] == item["old"],
                "second original old byte")
        old2 = only(
            rf"const uint8_t nl_replace_old_{item['node']} = "
            rf"\*nl_local_{item['ref']};", code,
        )
        append(old2,
               f"if (nl_local_{item['ref']} != "
               f"r708_ob_{roots[1]}_{item['index']} || "
               f"nl_replace_old_{item['node']} != {item['old']}) abort();")
        store2 = only(rf"\*nl_local_{item['ref']} = [^;]+;", code)
        expected_pre[roots[1]][item["index"]] = oracle["old"][0]
        require(tuple(tuple(expected_pre[s]) for s in roots) ==
                tuple(oracle["direct"]["post"]), "final second-root post-state")
        append(store2, f"if ({bad(expected_pre)}) abort();")
    final = only(r"\(void\)0;", code)
    append(final, " ".join(f"r708_ob_{s}_{i} = NULL;"
                           for s in roots for i in range(2)))
    for pos in sorted(inserts, reverse=True):
        code = code[:pos] + inserts[pos] + code[pos:]
    prelude = "#include <stdlib.h>\n#include <stdint.h>\nstatic unsigned r708_calls;\n"
    prelude += "\n".join(
        f"static const volatile uint8_t *r708_ob_{s}_{i};"
        for s in roots for i in range(2)
    ) + "\n"
    return prelude + code


def main(compiler, probe, cc, sanitizer, folder):
    folder = Path(folder)
    results = {}
    with tempfile.TemporaryDirectory() as directory:
        temp = Path(directory)
        for filename, oracle in CASES.items():
            source = folder / filename
            require(hashlib.sha256(source.read_bytes()).hexdigest() == oracle["sha"],
                    "first blind SHA mismatch " + filename)
            checked = execute([compiler, "--check", source])
            require(checked.returncode == 0 and not checked.stdout and not checked.stderr,
                    f"CHECK {filename}: {checked}")
            result = execute([probe, source, len(oracle["index"])])
            require(result.returncode == 0 and not result.stderr,
                    f"source-issued proof {filename}: {result}")
            facts = json.loads(result.stdout)
            generated = execute([compiler, source])
            require(generated.returncode == 0 and generated.stdout and not generated.stderr,
                    f"C17 {filename}: {generated}")
            untouched = generated.stdout.decode("utf-8")
            require("#include <stdint.h>" in untouched, "actual C17")
            passive = witness(untouched, facts, oracle)
            # Dropping the real named call is an instrumentation rejection,
            # not a valid substitute for its old byte / caller mutation.
            require(len(re.findall(r"nl_call_\d+ = nl_fn_\d+\(", untouched)) ==
                    len(oracle["index"]), "real helper call count")
            rows = []
            for code_label, source_text in (("unchanged", untouched), ("observer", passive)):
                c_path = temp / (filename + "-" + code_label + ".c")
                c_path.write_text(source_text)
                for opt in ("-O0", "-O2"):
                    exe = temp / (filename + "-" + code_label + opt)
                    command = [cc, "-std=c17", "-Wall", "-Wextra",
                               "-Wpedantic", "-Werror", opt, c_path, "-o", exe]
                    if sanitizer != "none":
                        command += ["-fsanitize=" + sanitizer,
                                    "-fno-sanitize-recover=all",
                                    "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
                    built = execute(command)
                    require(built.returncode == 0, f"{code_label} {opt}: {built.stderr.decode()}")
                    guest = execute([exe])
                    require(guest.returncode == 0 and not guest.stdout and not guest.stderr,
                            f"{code_label} {opt}: {guest}")
                    rows.append(code_label + opt)
            named = temp / "verified.c"
            named.write_bytes(b"KEEP\n")
            output = execute([compiler, "--emit-c17-to", named, source])
            require(output.returncode == 0 and not output.stdout and
                    named.read_bytes() == generated.stdout,
                    f"named output differs: {output}")
            results[filename] = {
                "check": "PASS", "source_issued": "PASS",
                "copied_view_rejections": facts["rejections"],
                "native": rows, "C17_sha256": hashlib.sha256(generated.stdout).hexdigest(),
                "issued": facts,
            }
        negative = folder / "N_read_amplification.nl"
        require(hashlib.sha256(negative.read_bytes()).hexdigest() == NEGATIVE_SHA,
                "first BLIND negative SHA")
        denied = execute([compiler, "--check", negative])
        require(denied.returncode == 3 and not denied.stdout and
                b"TYPE-MISMATCH" in denied.stderr,
                f"negative semantic-rejection requirement: {denied}")
        reject_output = execute([compiler, negative])
        require(reject_output.returncode == 3 and not reject_output.stdout,
                f"negative C17 fail-closed: {reject_output}")
        named = temp / "negative.c"
        named.write_bytes(b"KEEP\n")
        rejected_named = execute([compiler, "--emit-c17-to", named, negative])
        require(rejected_named.returncode == 3 and not rejected_named.stdout and
                named.read_bytes() == b"KEEP\n", f"negative output modified: {rejected_named}")
        results[negative.name] = {
            "check": "REJECT", "diagnostic": denied.stderr.decode("utf-8").strip(),
            "C17": "REJECT", "old_output_preserved": True,
        }
    print("R708 INDEPENDENT FINITE RESULT " + json.dumps(results, sort_keys=True))


if __name__ == "__main__":
    main(*sys.argv[1:])
