"""Real production source checking, independently of C17 backend admission."""
from pathlib import Path
import subprocess
import sys
import tempfile


def run(compiler: str, args: list[str]) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run([compiler, *args], capture_output=True, timeout=30,
                          check=False)


def main() -> None:
    compiler, repo = sys.argv[1:]
    node = Path(repo) / "tests/fixtures/node_join_same_locator.nl"
    samples = {
        "u8": ("fn main()->unit{let x=u8(7);x;unit}\n", 0, ""),
        # Existing checked_c_local_root_test.py boundary: semantically valid
        # scalar-returning read loan is still outside the C17 emitter profile.
        "backend": ("fn main()->unit{let x=u8(7);"
                    "let y=loan_read(x){|r|u8(9)};y;unit}\n", 0, ""),
        "parse": ("fn main()->unit{let x=;unit}\n", 3,
                  "P5-EXPECTED-EXPRESSION"),
        "range": ("fn main()->unit{let x=u8(256);unit}\n", 3,
                  "V1-U8-LITERAL-RANGE"),
        "type": ("fn bad()->u8{unit}\nfn main()->unit{bad();unit}\n", 3,
                 "P8-BODY-RESULT"),
        "escape": ("fn main()->unit{let x=u8(7);"
                   "let bad=loan_write(x){|w|w};unit}\n", 3,
                   "P8-EXIT-DEPENDENCY"),
        "plus": ("fn main()->unit{let x=u8(7)+u8(2);unit}\n", 3,
                 "P5-EXPECTED-ITEM-END"),
        "missing_main": ("fn other()->unit{unit}\n", 3, "P3-UNKNOWN-CALLEE"),
    }
    with tempfile.TemporaryDirectory(prefix="newlang-check-") as temp:
        sources = {"node": (node, 0, "")}
        for name, (text, status, diagnostic) in samples.items():
            path = Path(temp) / f"{name}.nl"
            path.write_text(text, encoding="utf-8")
            sources[name] = (path, status, diagnostic)
        sources["missing_file"] = (Path(temp) / "absent.nl", 1, "V0-SOURCE-IO")
        for name, (path, status, diagnostic) in sources.items():
            # Repeated processes exercise success/failure cleanup under
            # sanitizers. Shared-context rollback stays in existing C tests.
            first = run(compiler, ["--check", str(path)])
            second = run(compiler, ["--check", str(path)])
            actual = (first.returncode, first.stdout, first.stderr)
            assert actual == (second.returncode, second.stdout, second.stderr), name
            assert first.returncode == status and not first.stdout, (name, actual)
            if diagnostic:
                assert f"[{diagnostic}]".encode() in first.stderr, (name, actual)
            else:
                assert not first.stderr, (name, actual)
            ordinary = run(compiler, [str(path)])
            if name == "backend":
                assert ordinary.returncode == 4 and not ordinary.stdout
                assert b"[V1-BACKEND-UNSUPPORTED]" in ordinary.stderr
            elif name in ("u8", "node"):
                assert ordinary.returncode == 0 and not ordinary.stderr
                assert b"int main(void)" in ordinary.stdout
            else:
                assert actual == (ordinary.returncode, ordinary.stdout,
                                  ordinary.stderr), name
            good = run(compiler, ["--check", str(sources["u8"][0])])
            assert (good.returncode, good.stdout, good.stderr) == (0, b"", b"")
        for args in (["--check"], ["--check", "--help"],
                     ["--check", str(node), "extra"], [str(node), "--check"],
                     ["--check", "--version"]):
            rejected = run(compiler, args)
            assert rejected.returncode == 2 and not rejected.stdout, args
            assert b"[P0-CLI-" in rejected.stderr, args
    print("--check: real source/owned success, parse/semantic/escape rejection, "
          "zero output, unchanged ordinary backend boundaries and CLI misuse")


if __name__ == "__main__":
    main()
