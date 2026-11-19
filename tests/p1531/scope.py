"""C1213: direct prefix, old zero-prefix LiveTail, held non-direct prefix."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys


def exercise(compiler, consumer, live_probe, direct_probe, tests, out, run):
    out.mkdir(parents=True, exist_ok=True)
    sources = {
        "live": tests / "p1481/Z-success-zero.nl",
        "direct": tests / "p1529/D-combined-nonzero.nl",
        "held": tests / "p1531/N-live-prefix.nl",
    }
    generated = {}
    for name, source in sources.items():
        run(name + "-CHECK", [compiler, "--check", source])
        if name == "direct":
            run(name + "-owned", [direct_probe, source, "37"])
        else:
            run(name + "-owned", [live_probe, "owned", source, "0", "0"])
        target = out / (name + ".c")
        if name == "held":
            sentinel = b"P1531 C1213 untouched named output\n"
            target.write_bytes(sentinel)
            run(name + "-C17", [compiler, "--emit-c17-to", target, source],
                expected=4)
            assert target.read_bytes() == sentinel
            assert not list(out.glob(target.name + ".tmp.*"))
            run(name + "-API", [consumer, "refuse", source])
            run("pristine-named-retry", [compiler, "--emit-c17-to", target,
                                        sources["direct"]])
            assert target.read_bytes() == generated["direct"].read_bytes()
            assert not list(out.glob(target.name + ".tmp.*"))
        else:
            run(name + "-C17", [compiler, "--emit-c17-to", target, source])
            run(name + "-API", [consumer, "agree", source, target])
            generated[name] = target
    return generated


def main():
    compiler, consumer, live_probe, direct_probe, tests, output = sys.argv[1:]
    out = Path(output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    rows = []

    def run(label, argv, expected=0):
        argv = list(map(str, argv))
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        r = subprocess.run(argv, capture_output=True, env=env)
        for suffix, data in [("stdout", r.stdout), ("stderr", r.stderr)]:
            (out / (label + "." + suffix)).write_bytes(data)
        rows.append(dict(label=label, argv=argv, expected=expected,
                         exit=r.returncode))
        (out / "ledger.json").write_text(json.dumps(rows, indent=2) + "\n")
        assert r.returncode == expected, (label, r.stdout, r.stderr)
        return r

    generated = exercise(compiler, consumer, live_probe, direct_probe,
                         Path(tests).resolve(), out, run)
    print("P1531 direct/zero-prefix LiveTail preserved; held prefix "
          "refused/no-clobber/retry PASS",
          {k: hashlib.sha256(v.read_bytes()).hexdigest()
           for k, v in generated.items()})


if __name__ == "__main__":
    main()
