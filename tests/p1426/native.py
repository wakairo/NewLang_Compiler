import pathlib
import subprocess
import sys


def run(command):
    subprocess.run(command, check=True)


compiler = pathlib.Path(sys.argv[1])
cc = sys.argv[2]
root = pathlib.Path(sys.argv[3])
output = pathlib.Path(sys.argv[4])
output.mkdir(parents=True, exist_ok=True)
observer = root / "tests/p1426/native_observer.h"
wrapper = root / "tests/p1426/native_wrap.c"
cases = (
    ("U", root / "tests/p1420/U.nl", 1, 0),
    ("Alias", root / "tests/p1420/Alias.nl", 1, 0),
    ("False", root / "tests/p1426/False.nl", 0, 1),
)

for name, source, expected, option_tag in cases:
    generated = output / f"{name}.c"
    run((str(compiler), "--emit-c17-to", str(generated), str(source)))
    profiles = ((0, "O0"), (0, "O2"), (1, "O2"), (2, "O2"))
    for fail_at, optimization in profiles:
        executable = output / f"{name}-{fail_at}-{optimization}"
        flags = (
            cc,
            "-std=c17",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            f"-{optimization}",
            f'-DNEWLANG_NODE_OBSERVER="{observer}"',
            f'-DNEWLANG_HEAP_OBSERVER="{observer}"',
            f"-DP1426_EXPECT_TRUE={expected}",
            f"-DP1426_EXPECT_OPTION_TAG={option_tag}",
            f"-DP1426_FAIL_AT={fail_at}",
            str(generated),
            str(wrapper),
            "-Wl,--wrap=malloc",
            "-Wl,--wrap=free",
            "-o",
            str(executable),
        )
        run(flags)
        run((str(executable),))

print("P1426 PASS: actual Bool/ptr/read; H0/H1/H2 cleanup; strict native")
