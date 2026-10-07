"""North Star V0 Checked-C acceptance spine.

The production compiler must semantically accept before any C bytes are emitted.
This test materializes stdout only after successful acceptance, then uses the
configured host C17 compiler and executes the native program.
"""

from pathlib import Path
import subprocess
import sys
import tempfile


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        capture_output=True,
        text=True,
        timeout=30,
        check=False,
    )


def positive(root: Path, compiler: str, cc: str, name: str, source: str) -> None:
    source_path = root / f"{name}.nl"
    c_path = root / f"{name}.c"
    exe_path = root / name
    source_path.write_text(source, encoding="utf-8")

    first = run([compiler, str(source_path)])
    second = run([compiler, str(source_path)])
    if first.returncode != 0 or second.returncode != 0:
        raise SystemExit(
            f"{name}: compiler rejected positive source:\n"
            f"first={first.returncode} {first.stderr}\n"
            f"second={second.returncode} {second.stderr}"
        )
    if first.stderr or second.stderr:
        raise SystemExit(f"{name}: unexpected compiler stderr")
    if first.stdout != second.stdout:
        raise SystemExit(f"{name}: Checked-C output is not deterministic")
    if "int main(void)" not in first.stdout or "static void nl_fn_" not in first.stdout:
        raise SystemExit(f"{name}: missing bounded Checked-C wrapper/function")

    c_path.write_text(first.stdout, encoding="utf-8")
    built = run(
        [
            cc,
            "-std=c17",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            str(c_path),
            "-o",
            str(exe_path),
        ]
    )
    if built.returncode != 0:
        raise SystemExit(
            f"{name}: host C compile failed:\n{built.stdout}{built.stderr}"
        )
    executed = run([str(exe_path)])
    if executed.returncode != 0 or executed.stdout or executed.stderr:
        raise SystemExit(
            f"{name}: native execution failed: "
            f"{executed.returncode}, {executed.stdout!r}, {executed.stderr!r}"
        )


def negative(
    root: Path,
    compiler: str,
    name: str,
    source: str,
    expected_code: str,
) -> None:
    source_path = root / f"{name}.nl"
    c_path = root / f"{name}.c"
    exe_path = root / name
    source_path.write_text(source, encoding="utf-8")

    result = run([compiler, str(source_path)])
    if result.returncode == 0:
        raise SystemExit(f"{name}: invalid source was accepted")
    if result.stdout:
        raise SystemExit(f"{name}: rejected source emitted C bytes")
    if expected_code not in result.stderr:
        raise SystemExit(
            f"{name}: expected diagnostic {expected_code}, got:\n{result.stderr}"
        )
    if c_path.exists() or exe_path.exists():
        raise SystemExit(f"{name}: rejected source left C/native artifact")


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: checked_c_v0_test.py NEWLANGC HOST_CC")
    compiler, cc = sys.argv[1:]

    with tempfile.TemporaryDirectory(prefix="newlang-v0-") as temp:
        root = Path(temp)
        positive(
            root,
            compiler,
            cc,
            "p1",
            "fn main()->unit{unit}\n",
        )
        positive(
            root,
            compiler,
            cc,
            "p2",
            "fn helper()->unit{unit}\n"
            "fn main()->unit{helper();unit}\n",
        )
        negative(
            root,
            compiler,
            "n1",
            "fn main()->unit{missing();unit}\n",
            "P3-UNKNOWN-CALLEE",
        )
        negative(
            root,
            compiler,
            "n2",
            "fn main()->bool{return unit;}\n",
            "P9-RETURN-TYPE",
        )

    print("north_star_v0_checked_c: P1/P2 execute; N1/N2 reject before emission")


if __name__ == "__main__":
    main()
