"""Exploratory map probe: matched Copy locator drives a DIFFERENT live Node's link.

This is a test-only cost/first-barrier measurement, not an implementation,
a language rejection test, or evidence of general safe heap graph support.
Do not count CTest green as the experimental construct being supported.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile


def run(argv):
    return subprocess.run(argv, check=False, capture_output=True,
                          text=True, timeout=45)


def main():
    compiler, cc, fixture, sanitizer = sys.argv[1:]
    original = Path(fixture).read_text(encoding="utf-8")
    anchor = "Some(q)=>{loan_read_ptr(q){|tail_ref|unit}},"
    if original.count(anchor) != 1:
        raise SystemExit("frozen fixture anchor changed; no credible comparison")
    if "let observed=head@next;" not in original or original.count(
            "let old_some=loan_write(head@next)") != 1:
        raise SystemExit("baseline two-original-Node topology changed")

    # Prediction before executing: node/link source checker MAY be too bounded
    # for a write to tail inside the matched Some arm; if source is admitted,
    # the closed C17 node emitter or post-join field currentness may stop.
    changed = original.replace(
        anchor,
        "Some(q)=>{let old_tail=loan_write(tail@next){|w|"
        "replace(w,Option<ptr<Node>>::Some(q))};old_tail;unit},")
    joined = changed.replace(
        "let old_some=loan_write(head@next)",
        "let tail_after=tail@next;tail_after;"
        "let old_some=loan_write(head@next)")

    flags = ["-std=c17", "-Wall", "-Wextra", "-Wpedantic",
             "-Werror", "-O0"]
    if sanitizer != "none":
        flags += ["-fsanitize=" + sanitizer,
                  "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
                  "-fno-pie", "-no-pie"]

    with tempfile.TemporaryDirectory(prefix="newlang-node-frontier-") as d:
        root = Path(d)

        def probe(name, source):
            path = root / (name + ".nl")
            path.write_text(source, encoding="utf-8")
            result = run([compiler, str(path)])
            if result.returncode not in (0, 2, 3, 4, 5):
                raise SystemExit(
                    f"{name}: compiler crash / unclassified exit "
                    f"{result.returncode}: {result.stderr[:700]}")
            detail = {
                "case": name,
                "source_sha256": hashlib.sha256(source.encode()).hexdigest(),
                "compiler_exit": result.returncode,
                "c_bytes": len(result.stdout),
            }
            if result.returncode:
                if result.stdout or not result.stderr:
                    raise SystemExit(
                        f"{name}: partial C output / missing failure diagnostic")
                # A source-profile or backend stop is an experimental FINDING,
                # never a NewLang semantic-invalid or product failure claim.
                match = re.search(r"\[([A-Z0-9-]+)\]", result.stderr)
                detail["first_diagnostic"] = (
                    match.group(1) if match else result.stderr.strip()[:240])
                detail["evidence"] = "FIRST-BARRIER-ONLY"
            else:
                if not result.stdout or result.stderr:
                    raise SystemExit(f"{name}: absent/nonclean compiler output")
                c_file, exe = root / (name + ".c"), root / name
                c_file.write_text(result.stdout, encoding="utf-8")
                build = run([cc, *flags, str(c_file), "-o", str(exe)])
                if build.returncode or build.stderr:
                    raise SystemExit(
                        f"{name}: generated strict C17 compilation failed: "
                        f"{build.stderr[:900]}")
                native = run([str(exe)])
                if native.returncode or native.stderr or native.stdout:
                    raise SystemExit(
                        f"{name}: native exit/nonempty I/O: "
                        f"{native.returncode} {native.stderr[:700]}")
                detail["evidence"] = (
                    "CLI-SOURCE-C17-NATIVE-EXECUTION-ONLY;"
                    "NO-POINTER-POSTSTATE-OBSERVER")
            return detail

        base = probe("unchanged-main-baseline", original)
        if base["compiler_exit"] != 0:
            raise SystemExit(f"existing native baseline regressed: {base}")
        branch = probe("matched-cross-root-relink", changed)
        rows = [base, branch]
        if branch["compiler_exit"] == 0:
            rows.append(probe("post-match-currentness-read", joined))
        else:
            rows.append({
                "case": "post-match-currentness-read",
                "evidence": "NOT-REACHED-FIRST-BARRIER"
            })
        print(json.dumps({
            "contract": "PREDICT/MEASURE FIRST BARRIER; NO PRODUCT PASS",
            "authority": "Compiler/main 348ca7d48f6dd5d658706f4547ab7ef9a63af33d",
            "cases": rows
        }, sort_keys=True))
        # Green CTest means the measurement was deterministic and bounded.
        # It does NOT imply the experimental branch was source/native admitted.


if __name__ == "__main__":
    main()
