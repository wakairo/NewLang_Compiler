"""Portable finite ordinary-CLI -> unmodified C17 -> raw/passive native test."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

compiler_cli, host_cc, sanitizer, output_root = sys.argv[1:]
tests = Path(__file__).resolve().parent
output = Path(output_root)
output.mkdir(parents=True, exist_ok=True)
work = Path(tempfile.mkdtemp(prefix="p1412-", dir=output))
sources = {
    "S": (tests.parent / "p1408" / "S-v1-same-original-two-paths.nl",
          "52160257c3527cabf8a321211fa876e77075002e1d4d04236e0236871f549852"),
    "T": (tests / "T-v1-true-arm.nl",
          "b7f82ca575af042156fd1567b8501a2528446d711a984d399f7cbdadf6ecb244"),
}
assert sanitizer in ("none", "address", "undefined"), sanitizer
flags = [host_cc] + shlex.split(os.environ.get("CFLAGS", "")) + [
    "-std=c17", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
if sanitizer != "none":
    flags += ["-fsanitize=" + sanitizer, "-fno-omit-frame-pointer",
              "-fno-sanitize-recover=all"]
record = {"sources": {}, "sanitizer": sanitizer, "commands": [],
          "cli": 0, "guest": 0, "expectation":
          "raw S/T H2; passive S/T H2 real false/true arm, same original B, "
          "initial None Copy; common S prefix H0/H1 cleanup0/1, IF NOT_REACHED"}
manifest = work / "execution.json"


def save():
    manifest.write_text(json.dumps(record, indent=2) + "\n")


def execute(command, kind, name):
    row = {"cmd": [str(x) for x in command], "kind": kind, "state": "started"}
    record["commands"].append(row)
    if kind in ("cli", "guest"):
        record[kind] += 1
    save()
    run = subprocess.run(command, capture_output=True)
    (work / (name + ".stdout")).write_bytes(run.stdout)
    (work / (name + ".stderr")).write_bytes(run.stderr)
    row.update(state="completed", exit=run.returncode)
    save()
    print(kind, name, "exit", run.returncode, flush=True)
    if run.stdout:
        print(run.stdout.decode(errors="replace"), end="", flush=True)
    if run.returncode:
        print(run.stderr.decode(errors="replace"), file=sys.stderr, flush=True)
        raise SystemExit("P1412 failure; preserved apparatus and raw C: " + str(work))


for name, (source, expected_sha) in sources.items():
    assert hashlib.sha256(source.read_bytes()).hexdigest() == expected_sha
    raw = work / (name + ".c")
    record["sources"][name] = {"path": str(source), "sha256": expected_sha}
    execute([compiler_cli, "--emit-c17-to", str(raw), str(source)], "cli", name)
    data = raw.read_bytes()
    assert data and b"int main(void)" in data
    record["sources"][name]["C_sha256"] = hashlib.sha256(data).hexdigest()
    save()

# Two raw programs and four passive cases: six process runs, two CLI calls.
plan = [("S", 2, False), ("T", 2, False), ("S", 2, True),
        ("T", 2, True), ("S", 0, True), ("S", 1, True)]
for i, (source, world, passive) in enumerate(plan, 1):
    raw = work / (source + ".c")
    expected_c = record["sources"][source]["C_sha256"]
    assert hashlib.sha256(raw.read_bytes()).hexdigest() == expected_c
    name = f"case-{i}-{source}-H{world}"
    exe = work / name
    mode_flags = flags + (["-O2"] if passive else ["-O0"])
    if passive:
        obj = work / (name + ".o")
        branch = 1 if source == "S" else 0
        execute(mode_flags + [f"-DTEST_WORLD={world}", f"-DTEST_BRANCH={branch}",
                             "-Dmain=nl_generated_main",
                             '-DNEWLANG_HEAP_OBSERVER="' + str(tests / "passive.h") + '"',
                             "-c", str(raw), "-o", str(obj)], "compile", name + "-object")
        execute(mode_flags + [f"-DTEST_WORLD={world}", str(obj),
                             str(tests / "passive_driver.c"), "-Wl,--wrap=malloc",
                             "-Wl,--wrap=free", "-o", str(exe)], "compile", name + "-link")
    else:
        execute(mode_flags + [str(raw), "-o", str(exe)], "compile", name + "-link")
    record["commands"].append({"ELF": str(exe),
                               "sha256": hashlib.sha256(exe.read_bytes()).hexdigest()})
    execute([str(exe)], "guest", name)
    assert hashlib.sha256(raw.read_bytes()).hexdigest() == expected_c
assert record["cli"] == 2 and record["guest"] == 6
record["result"] = "PASS"
save()
print("P1412 real newlangc / strict raw+passive native PASS; receipt", manifest)
