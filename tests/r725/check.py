"""Independent first-pass: frozen bytes, CHECK, public borrowed proof, strict native."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

def run(*args):
    return subprocess.run([str(s) for s in args], capture_output=True)

def require(cond, description, result=None):
    if not cond:
        raise AssertionError(description + (("\\nSTDERR=" + result.stderr.decode(errors="replace") +
                           "\\nSTDOUT=" + result.stdout.decode(errors="replace")) if result else ""))

def main():
    compiler, gate, cc, sanitizer, folder = sys.argv[1:]
    root = Path(folder)
    records = json.loads((root / "freeze.json").read_text())["cases"]
    with tempfile.TemporaryDirectory(prefix="r725_") as workspace:
        tmp = Path(workspace)
        for rec in records:
            source = root / rec["file"]
            require(hashlib.sha256(source.read_bytes()).hexdigest() == rec["sha256"],
                    "immutable independent SHA for " + rec["file"])
            require(len(source.read_bytes()) == rec["bytes"], "byte-length mismatch")
            result = run(compiler, "--check", source)
            if rec["file"].startswith("N"):
                require(result.returncode == 3 and not result.stdout,
                        "negative must reject cleanly at CHECK", result)
                emitted = run(compiler, source)
                require(emitted.returncode == 3 and not emitted.stdout,
                        "negative must emit zero C17", emitted)
                named = tmp / "sentinel.c"
                named.write_bytes(b"unchanged-artifact")
                refused = run(compiler, "--emit-c17-to", named, source)
                require(refused.returncode == 3 and named.read_bytes() == b"unchanged-artifact",
                        "negative must preserve named output", refused)
                print("R725 NEG", rec["file"], "CHECK=3, zero C17, preserved named output")
                continue
            require(result.returncode == 0 and not result.stdout,
                    "positive CHECK expected for " + rec["file"], result)
            proof = run(gate, source, str(rec["index"]))
            require(proof.returncode == 0, "source-issued public owned proof", proof)
            view = json.loads(proof.stdout)
            require(view["index"] == rec["index"] and view["rejected"] >= 21,
                    "root field and public corruption count")
            emitted = run(compiler, source)
            require(emitted.returncode == 0 and emitted.stdout,
                    "strict C17 generation", emitted)
            named = tmp / "named.c"
            output = run(compiler, "--emit-c17-to", named, source)
            require(output.returncode == 0 and named.read_bytes() == emitted.stdout,
                    "named C17 emitted must equal stdout", output)
            source_c = tmp / "guest.c"
            source_c.write_bytes(emitted.stdout)
            for level in ["-O0", "-O2"]:
                flags = [cc, "-std=c17", "-Wall", "-Wextra", "-Werror",
                         "-pedantic-errors", level]
                if sanitizer not in ("", "none"):
                    flags += [f"-fsanitize={sanitizer}", "-fno-omit-frame-pointer",
                              "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
                executable = tmp / "guest"
                built = run(*(flags + [source_c, "-o", executable]))
                require(built.returncode == 0, "actual STRICT C17 compilation", built)
                native = run(executable)
                require(native.returncode == 0 and not native.stderr,
                        "actual unchanged native C program", native)
            print("R725 POS", rec["file"], "CHECK/proof/validator/C17/native O0/O2 PASS",
                  "root", view["root"], "child", view["child"], "mutations", view["rejected"])

if __name__ == "__main__":
    main()
