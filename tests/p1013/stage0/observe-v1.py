"""P1013 Stage0: run only frozen own inputs against exact unmodified parent."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

source_dir, cli, probe, native_cc, clang, destination, reject_pc = sys.argv[1:]
source_dir, cli, probe, destination = map(Path, (source_dir, cli, probe, destination))
destination.mkdir(parents=True, exist_ok=False)
oracle = json.loads((source_dir / "oracle-v1.json").read_bytes())
sha = lambda b: hashlib.sha256(b).hexdigest()
commands, results = [], []
def run(label, command):
    proc = subprocess.run([str(p) for p in command], capture_output=True, timeout=90)
    row = {"label": label, "command": [str(p) for p in command], "exit": proc.returncode}
    for key, data in [("stdout", proc.stdout), ("stderr", proc.stderr)]:
        file = destination / (label + "." + key)
        file.write_bytes(data)
        row[key + "_sha256"] = sha(data)
    commands.append(row)
    (destination / "commands-FIRST.json").write_text(json.dumps(commands, indent=2) + "\n")
    print(json.dumps(row), flush=True)
    return proc

for name in oracle["cases"]:
    case = name.removesuffix(".nl")
    source = source_dir / name
    record = {"case": case, "source_sha256": sha(source.read_bytes())}
    checked = run(case + "-CLI-check", [cli, "--check", source])
    record["CLI_check"] = checked.returncode
    if checked.returncode:
        record["first_layer"] = "parse/register/CHECK, see original CLI diagnostic"
        results.append(record)
        continue
    observed = run(case + "-public", [probe, source, "observe"])
    rows = [json.loads(l) for l in observed.stdout.decode().splitlines() if l.startswith("{")]
    record["public_receipts"] = rows
    record["public_exit"] = observed.returncode
    if observed.returncode:
        record["first_layer"] = "public GET/current validator or observation prerequisite; STOP Stage1"
        results.append(record)
        continue
    public = [r for r in rows if r["stage"] == "public"]
    values = [r for r in rows if r["stage"] == "values"]
    assert len(public) == len(values) == 2
    assert public[0]["function"] == public[1]["function"] if case.endswith("_same") else public[0]["function"] != public[1]["function"]
    output = destination / (case + ".c")
    emitted = run(case + "-CLI-emit-FIRST", [cli, "--emit-c17-to", output, source])
    record["CLI_emit"] = emitted.returncode
    record["C_published"] = output.exists()
    if emitted.returncode:
        assert not output.exists()
        record["native_T_and_H_physical"] = "INCONCLUSIVE: no C published"
        record["first_layer"] = "backend unsupported/resource; exact raw diagnostic retained"
        if case.startswith("mixed"):
            diagnostic = run(case + "-private-trace", [probe, source, "mixed", reject_pc])
            record["trace_exit"] = diagnostic.returncode
            trace = [json.loads(l) for l in diagnostic.stdout.decode().splitlines() if l.startswith("{")]
            record["trace_receipts"] = trace
            for item in trace:
                if item["stage"] == "first_private_reject":
                    pc = hex(int(item["return_address"], 16) - 1)
                    loc = run(case + "-first-reject-location", ["addr2line", "-e", probe, "-f", "-C", pc])
                    record["first_reject_location"] = loc.stdout.decode().splitlines()
    else:
        assert output.is_file() and output.stat().st_size
        record["C_sha256"] = sha(output.read_bytes())
        native = []
        for compiler in [native_cc, clang]:
            for optimization in ["-O0", "-O2"]:
                tag = ("gcc" if compiler == native_cc else "clang") + optimization
                executable = destination / (case + "-" + tag)
                build = run(case + "-" + tag + "-build", [compiler, "-std=c17", "-Wall", "-Wextra",
                            "-Werror", "-pedantic-errors", optimization, output, "-o", executable])
                assert build.returncode == 0
                guest = run(case + "-" + tag + "-native", [executable])
                assert guest.returncode == 0
                native.append({"compiler": compiler, "optimization": optimization, "unit_exit": guest.returncode})
        record["strict_native"] = native
        record["native_T_and_H_physical"] = "INCONCLUSIVE: process unit exit is observed; no physical hook observation in this Stage0"
    named = destination / (case + "-existing.c")
    original = b"P1013 KEEP ORIGINAL NAMED OUTPUT\n"
    named.write_bytes(original)
    before = named.stat()
    named_result = run(case + "-CLI-named", [cli, "--emit-c17-to", named, source])
    if named_result.returncode:
        after = named.stat()
        assert named.read_bytes() == original and (before.st_dev, before.st_ino) == (after.st_dev, after.st_ino)
    record["named_failure_no_clobber"] = named_result.returncode != 0
    results.append(record)
    (destination / "case-results-FIRST.json").write_text(json.dumps(results, indent=2) + "\n")

negative = source_dir / "negative_loan_escape.nl"
bad = run("negative-loan-escape-CLI-check", [cli, "--check", negative])
assert bad.returncode != 0
out = destination / "negative-never-published.c"
bad_c = run("negative-loan-escape-CLI-emit", [cli, "--emit-c17-to", out, negative])
assert bad_c.returncode != 0 and not out.exists()
result = {"oracle_sha256": sha((source_dir / "oracle-v1.json").read_bytes()),
          "CLI_binary_sha256": sha(cli.read_bytes()), "probe_binary_sha256": sha(probe.read_bytes()),
          "cases": results, "source_negative_check": bad.returncode,
          "source_negative_C": bad_c.returncode, "GitHub_CI_jobs": 0,
          "implementation_gate": "NOT AUTOMATIC: evaluate actual source/public receipts and first refusal"}
(destination / "stage0-FIRST.json").write_text(json.dumps(result, indent=2) + "\n")
print("P1013 Stage0 observations complete; classify evidence before any implementation", flush=True)
