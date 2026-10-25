"""R907: independently frozen sources through ordinary observer-OFF CLI."""
import pathlib
import subprocess
import sys

exe = sys.argv[1]
folder = pathlib.Path(sys.argv[2])
for name in ("r907_quartz.nl", "r907_tide.nl",
             "r907_return_incoming.nl", "r907_return_literal_old.nl"):
    source = folder / name
    check = subprocess.run([exe, "--check", str(source)], capture_output=True)
    print(f"R907 CLI {name}: exit={check.returncode} stdout={check.stdout!r} "
          f"stderr={check.stderr!r}", flush=True)
    if (check.returncode != 3 or check.stdout or
            b"P6-JOIN-PRECISION" not in check.stderr):
        raise SystemExit(f"STOP unexpected ordinary CHECK for {name}")
    target = folder / ("r907-canary-" + name + ".c")
    target.write_bytes(b"R907_NAMED_SENTINEL\n")
    try:
        emit = subprocess.run(
            [exe, "--emit-c17-to", str(target), str(source)],
            capture_output=True)
        print(f"R907 NAMED {name}: exit={emit.returncode} "
              f"stdout={emit.stdout!r} stderr={emit.stderr!r}", flush=True)
        if (emit.returncode != 3 or emit.stdout or
                b"P6-JOIN-PRECISION" not in emit.stderr or
                target.read_bytes() != b"R907_NAMED_SENTINEL\n"):
            raise SystemExit(f"STOP named output was altered for {name}")
    finally:
        target.unlink(missing_ok=True)
        for extra in folder.glob("r907-canary-" + name + ".c.tmp*"):
            raise SystemExit(f"STOP residual named output {extra}")
print("R907 CLI normal observer-OFF CHECK3/P6 and sentinel PASS")
