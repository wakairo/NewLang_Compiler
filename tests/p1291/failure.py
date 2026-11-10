"""Fresh real CHECK/emitter census; finite OOM, rollback and named no-clobber."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

mode, probe, cli, destination = sys.argv[1:]
here = Path(__file__).resolve().parent
oracle = json.loads((here / "oracle.json").read_text())
parent = Path(destination)
parent.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix=mode + "-", dir=parent))
rows = []


def run(args, label, expected=0):
    args = list(map(str, args))
    p = subprocess.run(args, capture_output=True)
    (out / (label + ".stdout")).write_bytes(p.stdout)
    (out / (label + ".stderr")).write_bytes(p.stderr)
    rows.append({"label": label, "argv": args, "status": p.returncode})
    (out / "commands.json").write_text(json.dumps(rows, indent=2) + "\n")
    assert p.returncode == expected, (label, p.returncode, p.stderr, out)
    return p


def same_file(before, path, sentinel):
    after = path.stat()
    assert path.read_bytes() == sentinel
    assert (before.st_ino, before.st_size, before.st_mtime_ns) == \
        (after.st_ino, after.st_size, after.st_mtime_ns)
    assert not list(out.glob("*.tmp*"))


if mode == "negative":
    extra = json.loads((here / "typed-result-negative.json").read_text())
    cases = {n: h for n, h in oracle["Stage1"]["sources"].items()
             if n.startswith("N-")}
    cases[extra["source"]] = extra["sha256"]
    for name, sha in cases.items():
        source = here / "sources" / name
        assert hashlib.sha256(source.read_bytes()).hexdigest() == sha
        assert not run([cli, source], name + "-C17", 3).stdout
        target = out / (name + ".preserved.c")
        sentinel = b"P1291 original output survives invalid source\n"
        target.write_bytes(sentinel)
        before = target.stat()
        run([cli, "--emit-c17-to", target, source], name + "-named", 3)
        same_file(before, target, sentinel)
    print("P1291 negative zero-C/named no-clobber PASS", out)
    sys.exit(0)

for name in oracle["Stage1"]["positive"]:
    source = here / "sources" / name
    assert hashlib.sha256(source.read_bytes()).hexdigest() == \
        oracle["Stage1"]["sources"][name]
    for phase in ["check", "emit"]:
        label = name + "-" + phase
        target = out / "preserved.c"
        if mode == "api":
            census = run([probe, source, phase, "census"], label + "-census")
            m = re.search(rb"CENSUS phase=\w+ total=(\d+) sites=(\d+)",
                          census.stdout)
            assert m
            count, sites = map(int, m.groups())
            ordinals = [int(x) for x in re.findall(rb"SELECT (\d+)",
                                                  census.stdout)]
            assert count <= (4096 if phase == "check" else 512)
            assert len(ordinals) <= (384 if phase == "check" else 512)
            assert ordinals and sites
        else:
            assert mode == "named"
            baseline = run([cli, source], label + "-baseline").stdout
            census = run([probe, source, target, phase, 0], label + "-census")
            assert target.read_bytes() == baseline
            count = int(re.search(rb"allocations=(\d+)", census.stdout)[1])
            assert 0 < count <= (4096 if phase == "check" else 512)
            ordinals = (sorted(set(range(1, min(count, 8) + 1)) |
                               set(range(max(1, count - 7), count + 1)) |
                               {max(1, count // 2)}) if phase == "check"
                        else list(range(1, count + 1)))
        plan = {"source": name, "phase": phase, "census": count,
                "ordinals": ordinals, "injections_executed": 0,
                "probe_sha256": hashlib.sha256(Path(probe).read_bytes()).hexdigest()}
        p = out / (label + ".plan.json")
        p.write_text(json.dumps(plan, indent=2) + "\n")
        p.chmod(0o444)
        if mode == "api":
            result = run([probe, source, phase], label + "-faults")
            selected = int(re.search(rb"selected=(\d+)", result.stdout)[1])
            assert selected == len(ordinals)
            print(name, phase, "API OOM/rollback/retry", selected, count)
            continue
        sentinel = b"P1291 output must survive real core allocation failure\n"
        for ordinal in ordinals:
            target.write_bytes(sentinel)
            before = target.stat()
            result = run([probe, source, target, phase, ordinal],
                         label + "-" + str(ordinal))
            assert b"fired=1" in result.stdout
            if phase == "check":
                assert b"error(host)" in result.stderr and \
                    b"OUT-OF-MEMORY" in result.stderr
            else:
                assert b"error(cli)[NODE-C-RESOURCE]" in result.stderr
            same_file(before, target, sentinel)
            run([cli, "--emit-c17-to", target, source],
                label + "-" + str(ordinal) + "-retry")
            assert target.read_bytes() == baseline
        print(name, phase, "named OOM/no-clobber/retry", len(ordinals), count)
print("P1291", mode, "failure gate PASS", out)
