"""R756 first-pass source and C17 fail-closed observations.

All source files / expected results pre-frozen in commit fb1d6ea5961705b0af79a267e53cba1b851a0f5e.
No production compiler, issuer, or public validator modifications.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "evidence/r756/independent"
INDEX = json.loads((ROOT / "evidence/r756/source_manifest.json").read_text("utf-8"))
SENTINEL = b"/* R756 preexisting output protected */\n"


def invoke(*args):
    r = subprocess.run([str(x) for x in args], capture_output=True, check=False)
    return r.returncode, r.stdout, r.stderr


def diagnostic(stderr):
    text = stderr.decode("utf-8", "replace")
    ids = list(dict.fromkeys(re.findall(r"\b[A-Z][A-Z0-9]+(?:-[A-Z0-9]+)+\b", text)))
    category = "parser/profile" if "AVS-DECL-PROFILE" in text or "PARSE" in text else "semantic/unknown"
    return category + ":" + ",".join(ids[:4]) if ids else category + ":" + text[:100]


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: audit.py /path/to/newlangc")
    binary = sys.argv[1]
    errors, rows = [], []
    for item in INDEX:
        name = item["file"]
        source = SOURCE / name
        data = source.read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        if digest != item["sha256"] or len(data) != item["bytes"]:
            raise SystemExit("FROZEN SHA256/byte count drift " + name)
        positive = not name.startswith("n_")
        ck, out, err = invoke(binary, "--check", source)
        print(f"R756 {name} SHA256={digest} CHECK={ck} stdout={len(out)} diagnostic={diagnostic(err)!r}", flush=True)
        if err:
            print("R756 CHECK STDERR", err.decode("utf-8", "replace")[:1200], flush=True)
        if positive and (ck != 0 or out or err):
            errors.append(f"{name}: positive SOURCE/CHECK mismatch (precision/grammar vs safety TBC)")
        if not positive and (ck == 0 or out or not err):
            errors.append(f"{name}: rejected input accepted or unexpected stdout")
        with tempfile.TemporaryDirectory(prefix="r756-output-") as temp:
            cpath = Path(temp) / "existing-output.c"
            cpath.write_bytes(SENTINEL)
            rc, emitted, emiterr = invoke(binary, "--emit-c17-to", cpath, source)
            print(f"R756 {name} named_C17={rc} emitted={len(emitted)} sentinel={cpath.read_bytes()==SENTINEL} diagnostic={diagnostic(emiterr)!r}", flush=True)
            if emiterr:
                print("R756 EMIT STDERR", emiterr.decode("utf-8", "replace")[:1200], flush=True)
            if rc == 0 or emitted or cpath.read_bytes() != SENTINEL:
                errors.append(f"{name}: partial/unauthorized named C17 or sentinel changed")
        if positive:
            rc0, output0, stderr0 = invoke(binary, source)
            print(f"R756 {name} stdout_C17_rc={rc0} stdout_bytes={len(output0)} diagnostic={diagnostic(stderr0)!r}", flush=True)
            if rc0 == 0 or output0:
                errors.append(f"{name}: mixed stdout backend failed-open")
        rows.append((name, ck, rc, diagnostic(err)))
    for tag, path in (
        ("legacy_H", ROOT / "tests/fixtures/live_tail_return.nl"),
        ("legacy_T", ROOT / "examples/device_settings_swap.nl"),
    ):
        rc, out, err = invoke(binary, "--check", path)
        print(f"R756 baseline={tag} CHECK={rc} stdout={len(out)} diagnostic={diagnostic(err)!r}", flush=True)
        rows.append((tag, rc, "n/a", diagnostic(err)))
        if rc != 0 or out or err:
            errors.append(f"prior bounded single-nominal {tag} regression")
    if os.getenv("GITHUB_STEP_SUMMARY"):
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a", encoding="utf-8") as f:
            f.write("\n### R756 independent BLIND Stage1 CLI audit\n\n")
            f.write("| Input | CHECK exit | named C17 exit | Stage/diagnostic |\n|---|---:|---:|---|\n")
            for name, check, emit, code in rows:
                f.write(f"| {name} | {check} | {emit} | {code.replace('|','/')} |\n")
            f.write("\nCHECK0 is not a joined public-owned certificate; mixed C17/native is deferred.\n")
    for error in errors:
        print("R756 DEVIATION:", error, flush=True)
    print(f"R756 END cases={len(rows)} deviations={len(errors)}", flush=True)
    return int(bool(errors))


if __name__ == "__main__":
    sys.exit(main())
