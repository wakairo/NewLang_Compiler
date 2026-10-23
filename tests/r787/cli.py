#!/usr/bin/env python3
"""R787 first-pass CLI and byte/hash check, legacy R-A native ONLY; no B native."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

compiler, cc, sanitizer, repository = sys.argv[1:]
root = Path(repository)
frozen = root / "tests/r787/frozen"
old = root / "tests/p778/external_r776"
oracle = json.loads((frozen / "manifest.json").read_text())

def require(test, note):
    if not test:
        raise AssertionError(note)

def run(args, **kw):
    return subprocess.run([str(a) for a in args], capture_output=True,
                          timeout=90, **kw)

def checkhash(path, digest):
    require(hashlib.sha256(path.read_bytes()).hexdigest() == digest,
            "source hash mismatch " + str(path))

for name, info in oracle["inputs"].items():
    checkhash(frozen/name, info["sha256"])
with tempfile.TemporaryDirectory(prefix="r787-cli-") as folder:
    saved = Path(folder)/"original.c"
    sentinel = b"r787-existing-valid-c17-KEEP"
    for source in [frozen/"positive_two_calls.nl",
                   frozen/"positive_second_field.nl",
                   old/"positive_t_interposed.nl"]:
        if source.parent == old:
            checkhash(source, oracle["historical"]["R-B"]["sha256"])
        tested = run([compiler, "--check", source])
        require(tested.returncode == 0 and not tested.stdout and not tested.stderr,
                "positive source CHECK failed " + str(source) + " " +
                tested.stderr.decode(errors="replace")[-800:])
        c = run([compiler, source])
        require(c.returncode == 4 and not c.stdout and
                b"V1-BACKEND-UNSUPPORTED" in c.stderr,
                "B C17 failure boundary moved " + str(source))
        saved.write_bytes(sentinel)
        named = run([compiler, "--emit-c17-to", saved, source])
        require(named.returncode == 4 and not named.stdout and
                saved.read_bytes() == sentinel,
                "B CLI4 must preserve named output " + str(source))
        print("R787_CLI", source.name, "CHECK0 / C174 / zeroC / sentinel-preserved",
              flush=True)
    for source in [frozen/"negative_double_release.nl",
                   frozen/"negative_wrong_original.nl",
                   frozen/"negative_read_loan.nl"]:
        checked = run([compiler, "--check", source])
        emitted = run([compiler, source])
        saved.write_bytes(sentinel)
        named = run([compiler, "--emit-c17-to", saved, source])
        require(checked.returncode == emitted.returncode ==
                named.returncode == 3 and not checked.stdout and
                not emitted.stdout and saved.read_bytes() == sentinel,
                "negative source unexpectedly admitted " + str(source) +
                " CHECK " + checked.stderr.decode(errors="replace")[-600:])
        print("R787_NEG_SOURCE", source.name, "rejected at source CHECK",
              flush=True)

    ra = old/"positive_h_first.nl"
    checkhash(ra, oracle["historical"]["R-A"]["sha256"])
    checked = run([compiler, "--check", ra])
    require(checked.returncode == 0, "historical R-A CHECK")
    output = run([compiler, ra])
    require(output.returncode == 0 and
            b"int main(void)" in output.stdout and not output.stderr,
            "historical R-A C17 emission")
    cfile = Path(folder)/"legacy_original.c"
    cfile.write_bytes(output.stdout)
    saved.write_bytes(sentinel)
    named = run([compiler, "--emit-c17-to", saved, ra])
    require(named.returncode == 0 and saved.read_bytes() == output.stdout,
            "R-A named C17 byte match")
    options = ["-std=c17", "-pedantic-errors", "-Wall", "-Wextra", "-Werror"]
    if sanitizer != "none":
        options += ["-fsanitize="+sanitizer, "-fno-omit-frame-pointer",
                    "-fno-sanitize-recover=all"]
    for opt in ("-O0", "-O2"):
        executable = Path(folder)/("r787-old-ra-"+opt[1:])
        built = run([cc, *options, opt, cfile,
                     root/"tests/integration/two_heap_platform.c",
                     "-Wl,--wrap=malloc", "-Wl,--wrap=free",
                     "-DNEWLANG_UNOBSERVED",
                     '-DNEWLANG_HEAP_OBSERVER="'+str(root/"tests/r787/native_observer.h")+'"',
                     "-o", executable])
        require(built.returncode == 0,
                "old A native strict C17 build " + built.stderr.decode(errors="replace")[-1200:])
        for fault, count in (("1",0), ("2",1), ("0",2)):
            env = dict(os.environ, NEWLANG_TEST_FAIL_CALL=fault,
                       ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
            result = run([executable],env=env)
            require(result.returncode == 0 and
                    ("frees="+str(count)).encode() in result.stdout and
                    not result.stderr,
                    "old A native fail world "+fault+" "+str(result.returncode)+
                    result.stderr.decode(errors="replace")[-600:])
            print("R787_RA_NATIVE",opt, sanitizer, "fault",fault,
                  "real_allocated_free",count, flush=True)
    print("R787_CLI frozen CHECK/semantic/C17 and legacy R-A native complete",
          flush=True)
