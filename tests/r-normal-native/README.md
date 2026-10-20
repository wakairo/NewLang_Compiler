# R656 independent ordinary CLI / native evidence

Scope: Issue #656, **NON-BLIND calibrated**, evidence only; DO NOT MERGE.
This branch starts directly at candidate
`bebff1213223e86edfd702129ad8b3f2deb27289`, based on main
`9da4b4ce6163592f576c5a2ecb937ff6e230e6f8`, canonical Draft17.32.
PR #652, main, canonical, and historical R #653 / PR #654 are unchanged.

## Frozen input provenance

The three files were copied byte-for-byte from R's pre-execution commit
`aac7f65e38f36e16e4c8021f48696a3fcf5898f3:tests/r653/`.
`frozen.py` authenticates both Git blob SHA1 and SHA256 before **any** CLI
invocation. These identities were verified before the first candidate output.
The entire old oracle is retained unchanged, including historical predictions
and controls that this runner does **not** execute. Only its two normal cases
provide expected values. No P fixture, observer, or test runner was copied.

| Case | Initial physical Pairs | Condition / selected | Returned old Copies | Final physical Pairs |
|---|---|---|---|---|
| TRUE | (57,209), (131,244) | 1 / 186 | 57, 131 | (186,209), (57,244) |
| FALSE | (28,183), (87,164) | 0 / 19 | 28, 87 | (19,183), (28,164) |

Source and oracle identities are in `frozen.py` and local `freeze-before-cli.log`.
The unmodified ordinary CLI C17 bytes are retained as `generated/*.c17`:

| Case | Generated C17 SHA256 |
|---|---|
| TRUE | `da4b34aa62538b9aa5d84bbd44f98483de57e2b5f36dfc11fe1df025cd0c0e5b` |
| FALSE | `5b4ce031a53167cef5600733114c41a34a350c851d1ea72a3dda58bb2920bf3c` |

The TRUE bytes match the historical R Phase4 generated SHA; that does not change
the prior verdict or substitute for the new FALSE execution.

## Runner and passive observer

`run.py` invokes only `newlangc --check SOURCE` and `newlangc SOURCE`.
CHECK must return 0 with empty stdout/stderr. Emission must return 0, empty
stderr, nonempty C17, and identical bytes on a second invocation. Each run
also compares those bytes to the retained raw output.

Every guest is compiled with
`-std=c17 -pedantic-errors -Wall -Wextra -Werror`, at both `-O0` and `-O2`.
Plain guests use unchanged CLI output and must return 0 silently. Separately,
the passive version **only inserts statements** around existing generated
statements, leaving every guest declaration, assignment and control-flow
statement intact and in order. The helper header reads const field pointers,
prints actual addresses and values, and asserts the frozen expectations.
Observer variables only hold pointers to the actual two guest records.
The observer never supplies or writes guest storage.

It observes initial and final complete carriers, non-overlapping original root
addresses, actual loan pointers into those originals, selected condition/value,
old field loads and returned Copies, first-old-to-second-write transfer, and
unchanged sibling fields (including after the first write). Addresses naturally
vary per execution. The 14 observation lines per passive run are retained.
The CMake registration uses the actual `newlangc`, current host C compiler and
current `NEWLANG_SANITIZER`; the repository's existing timeout/environment apply.
No old proof-rejection gate is added.

Reproduce after the normal repository toolchain setup:

```sh
cmake -S . -B build
cmake --build build --parallel 2
ctest --test-dir build -R '^r656_' -V
```

For a standalone already-built compiler:

```sh
python3 tests/r-normal-native/run.py /absolute/build/newlangc gcc none /absolute/output
```

Use `clang-23` instead of `gcc`, and `address` / `undefined` instead of `none`
for the corresponding sanitizer guest runs. The host compiler itself is also
built with that sanitizer in the CMake configurations used for this evidence.

## Independent local environment and mechanical repairs

Actual environment: a new managed Codex code-execution session, Linux x86_64,
Debian 13, glibc 2.41; fresh network clone at
`/workspace/r656-normal-native`, R branch `r656-independent-normal-native`.
P's `/workspace/NewLang_Compiler` worktree, builds and cached tools are not used.
Tool versions: GCC 14.2.0, curated Clang / clang-format / LLVM 23.1.3,
CMake 3.31.10, Python 3.12.14. Full version output is retained locally.

Significant setup repairs, all outside compiler/oracle/source changes:

1. CMake and Clang were initially absent. The unmodified repository bootstrap's
   Debian package route hit a 404 for pinned `libclang-cpp23` after its three
   built-in attempts. The first failure log is preserved.
2. Installed CMake 3.31.10 in R's `.venv` (zstandard 0.25.0 was also installed;
   decompression uses the existing system `zstd`). Invoked the unchanged
   bootstrap's `_sdk_main` with the committed Ubuntu SDK lock on this Debian
   host. It downloaded fresh bytes, verified the exact 126520077-byte archive's
   SHA256 `5369d292ef513d32758634f98ec7cf62c52f82a0a333eb330ecf9a325648b133`,
   inspected/extracted the archive and verified its tools. This is disclosed
   cross-distro use of the exact curated SDK, not a claim of Ubuntu execution.
3. The first full build could not link LLVM smoke's `-lxml2` because the host
   only supplied `libxml2.so.2`. Added an R-local linker symlink
   `.deps/host-lib/libxml2.so` to the installed `/usr/lib/x86_64-linux-gnu/libxml2.so.2`
   and configured `-DCMAKE_EXE_LINKER_FLAGS=-L.../.deps/host-lib`. No library
   replacement, compiler source change, or relaxed diagnostic was required.
4. Formatted the independently authored observer with clang-format-23 before
   native execution. Static review corrected the observer's expected line
   count to 14 before its first run. No semantic expectations changed.

Local evidence contains original setup failures, repaired configure/build logs,
format result, native command rc/hash reports, actual passive stdout and CTest
logs. Raw generated files retain their exact CLI formatting and use `.c17` so
the repository's C/H formatting check does not rewrite authentic evidence.
The verbose Release CTest log is gzip-compressed without changing its bytes
(its original output contains trailing whitespace).

At publication the R656 checkpoint has passed in all five local configurations
(GCC Debug, GCC Release, Clang Debug, Clang ASan, Clang UBSan): 115 recorded
commands, including 40 actual guest executions, all return 0. All plain guests
and CHECK calls are silent; stderr is empty for every recorded command.
GCC and Clang full local repository suites also passed 359/359 each. The
broader local sanitizer suites continue independently; their final outcome and
the required natural Actions pair are checked before the terminal Issue report.

## Limits and CI budget

This is finite ordinary runtime/composition evidence only. It is not a proof
integrity or universal soundness claim. Issue #655's intentionally paused
internal-evidence alterations, forged receipts/IDs/worlds/ValueFacts, malformed
checked-owned artifact rejection and mutation/restore probes are **NOT TESTED**
by this runner. No test-only inclusion of `src/main.c`, unsupported backend
coercion or external security testing is performed. Existing baseline CTests
run unchanged as repository regression checks; they do not answer #655.

Historical R first-pass BLIND INCONCLUSIVE, Phase2–4 I-INCONCLUSIVE, six red
historical Actions pairs and PR #654 remain read-only. No prior result is
relabeled. Candidate adoption is C's separate decision; this evidence never
authorizes PR #652 merge. Product GO remains paused and H1 remains on HOLD.

Publication budget: one prepared evidence commit and one natural FULL5 +
NO LLVM/PARTIAL Actions pair. At most one R-only mechanical follow-up/second
pair for an actual CI-only formatter/platform issue; no manual reruns. A wrong
guest result, normal compiler rejection or sanitizer fault requires stopping.
The terminal CI outcome and exact PR head are reported once in Issue #656,
then R stops. The PR remains DRAFT, base `p651-two-pair-frame`.
