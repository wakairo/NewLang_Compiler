"""Finite P1412 raw/passive native observations; generated C is never edited."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

packet = Path(sys.argv[1])
tests = Path(__file__).resolve().parent
gcc = "/usr/bin/gcc"
clang = "/workspace/p1295-c439/tools/clang19/clang"
plan = [
    ("S", gcc, "O0", 2, False), ("S", gcc, "O2", 0, True),
    ("S", clang, "O0", 1, True), ("S", clang, "O2", 2, True),
    ("T", gcc, "O0", 2, True), ("T", gcc, "O2", 1, True),
    ("T", clang, "O0", 0, True), ("T", clang, "O2", 2, True),
]
ledger = packet / "Stage2-native-ledger.json"
if ledger.exists():
    raise SystemExit("Existing ledger: inspect it and never repeat guest runs")
rows = []
env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
for i, (source, compiler, opt, world, passive) in enumerate(plan, 1):
    raw = packet / f"Stage2-v1-{source}.raw.c"
    name = packet / f"native-{i}-{source}-{opt}-H{world}"
    flags = [compiler, "-std=c17", "-Wall", "-Wextra", "-Wpedantic",
             "-Werror", "-" + opt]
    if compiler == clang:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    row = dict(source=source, compiler=compiler, optimization=opt, world=world,
               passive=passive, C_sha256=hashlib.sha256(raw.read_bytes()).hexdigest())
    rows.append(row)
    if passive:
        obj = name.with_suffix(".o")
        branch = 1 if source == "S" else 0
        cmds = [
            flags + [f"-DTEST_WORLD={world}", f"-DTEST_BRANCH={branch}",
                     "-Dmain=nl_generated_main",
                     '-DNEWLANG_HEAP_OBSERVER="' + str(tests / "passive.h") + '"',
                     "-c", str(raw), "-o", str(obj)],
            flags + [f"-DTEST_WORLD={world}", str(obj),
                     str(tests / "passive_driver.c"), "-Wl,--wrap=malloc",
                     "-Wl,--wrap=free", "-o", str(name)]
        ]
    else:
        cmds = [flags + [str(raw), "-o", str(name)]]
    row["compile"] = []
    for n, cmd in enumerate(cmds):
        r = subprocess.run(cmd, capture_output=True, env=env)
        (packet / f"native-{i}-compile-{n}.stdout").write_bytes(r.stdout)
        (packet / f"native-{i}-compile-{n}.stderr").write_bytes(r.stderr)
        row["compile"].append(dict(cmd=cmd, exit=r.returncode))
        ledger.write_text(json.dumps(rows, indent=2) + "\n")
        if r.returncode:
            print(r.stderr.decode(), flush=True)
            raise SystemExit(r.returncode)
    row["guest"] = "started"
    row["ELF_sha256"] = hashlib.sha256(name.read_bytes()).hexdigest()
    ledger.write_text(json.dumps(rows, indent=2) + "\n")
    r = subprocess.run([str(name)], capture_output=True, env=env)
    (packet / f"native-{i}.stdout").write_bytes(r.stdout)
    (packet / f"native-{i}.stderr").write_bytes(r.stderr)
    row["guest"] = "completed"
    row["exit"] = r.returncode
    ledger.write_text(json.dumps(rows, indent=2) + "\n")
    print(i, source, opt, "H" + str(world), r.returncode,
          r.stdout.decode().strip(), flush=True)
    if r.returncode:
        print(r.stderr.decode(), flush=True)
        raise SystemExit(r.returncode)
