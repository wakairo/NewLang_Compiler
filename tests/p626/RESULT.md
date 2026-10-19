# P626 USER-CLI-1 candidate — local pre-push record

Fixed main9ea45c88f71506875c2e1638043894129a947665, CURRENT_SPEC Draft17.32; actual main FULL39216188726/PARTIAL39216188691 first-attempt SUCCESS verified. Ordinary Production newlangc frontend-only build (no experimental parser/flag), isolated p626-user-cli-1 branch.

## Before/after
Original sources/oracles frozen before compiler changes. TRUE11<17→42, FALSE23<17→9, one varied Copy alias13<19→42. All three baseline --check0, source-issued public Copy/IF arm observations valid after original AST teardown, emitter4/V1-BACKEND-UNSUPPORTED/Cbytes0. Existing equality IF and README node_initial_some source/CHECK/C17 succeed.

First exact blocker: copy_if_selection scalar-accounting switch omits NL_CHECKED_USIZE_LESS_THAN, though boolean_scalar_at already authenticates that Copy bool origin with original children, types, source spans/operator/current values and zero authority. Production change is exactly that case plus a comment (2 added lines) in src/main.c. Existing full-prefix and both qualified IF arm checks retained. Direct-IF condition special case remains equality-only; no source grammar, CHECK, issuer/public API, effect, CFG/SSA, owner, target guard or C17 emitter change.

All three candidates now ordinary SOURCE/CHECK0, same source-issued/public Copy operands/condition and both retained u8 arm worlds, emitted real LTR-staged size_t comparison followed by real C ternary and Copy. Parent IF result remains unknown in checked state; no cached chosen literal substituted. Semantic/public views are observations, not a new privileged proof or public validator.

Actual strict GCC13.3.0-16 and Clang23.1.2 O0/O2: plain plus passive-observed native all0. Passive tests read actual original operands/staged temps/bool/optional alias/chosen u8/result Copy; no guest initialization/permission injection. Generated C must contain source-linked <, ternary and Copy, preventing replacement by known checker truth. Each of4 synthetic wrong-C controls per source must actually compile/exit91; synthetic controls are not semantic refusals. TRUE42, FALSE9, alias42 observed. Unit main itself exits0 and has no user printing/value-exit API; test observers are separate instrumentation.

Each positive: existing IF private gate77 corruptions refused, all worlds unchanged, driver allocations0; P626-local comparison adapter24 corruptions refused, snapshot unchanged. Original P603 gate assumes a duplicate-valued alternate quantity binding missing from this frozen user source. Initial harness failure preserved and explicitly calibrated in evidence/harness-calibration.md: same-value alternate is NOT_PRESENT here, still tested by unchanged P603 fixtures in full suite. No user source or oracle was altered for it. Test-only adapter mirrors existing controls, no production hook/new proof mechanism.

Wrong u8/usize comparison remains CLI3/USIZE-LESS-THAN-TYPE, source4:19-4:20, no C. Complex but semantic-valid arm-local Copy remains CHECK0/backend4/V1-BACKEND-UNSUPPORTED/Cbytes0, native NOT REACHED. Existing == IF and README Node source/C output hashes unchanged, strict native0. Full existing CLI source I/O/parser/semantic/backend/error cleanup and arithmetic/Node/Array regressions preserved.

Local entire frontend/C17 suites GCC14.2 Debug334/334 PASS and Clang23.1.2 Debug334/334 PASS; PARTIAL not locked-LLVM FULL proof. New actual-source OOM test registration172 and main110 failure prefixes, unchanged semantic state and failed main outputNULL. Formatting checked before push. Local GCC13 Debian13.3.0-16 retained verified P605 packages, local Clang23.1.2; CI locked LLVM/Clang23.1.3 remains separate and unchanged.

## Ordinary user reproduction
```sh
CC=gcc cmake -S . -B build-user -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-user --parallel 2 --target newlangc
build-user/newlangc --check tests/p626/fits_true.nl
build-user/newlangc tests/p626/fits_true.nl > /tmp/p626.c
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 /tmp/p626.c -o /tmp/p626
/tmp/p626
```
Use fits_false.nl for FALSE. Plain guest has no print API; actual42/9 physical observation is reproduced by registered p626_cli_copy_if.integration. Run ctest --test-dir build --output-on-failure -R '^p626' for actual-source native/negative/owned controls and OOM. Frozen SHA/exit/stdout/diagnostics and public before-state in evidence/baseline*.json; new C snapshots/source hashes/native logs and summary.json retained. Baseline executable SHA in baseline-freeze.json, original local binary retained outside Git at /workspace/p626-before-newlangc; fixed commit can rebuild it independently.

## Decision / pending CI
One DRAFT PR and one natural FULL5/PARTIAL pair at final HEAD allowed; this committed report predates those CI results. One terminal Issue626 report will supply actual URLs/outcomes without another push/rerun. New sources/expected truth immutable since freeze, no massive full-suite raw logs committed (compact summaries; detailed local logs outside branch).

Compass, current Roadmap and Process compiler-first policy followed. This is one normal-use composition with actual TRUE/FALSE, not new scalar syntax or a repeat of old fixtures. Historical audit N/A faithful Draft17.32 5.2/6/7 and pure IF semantics; KEEP existing scalar/Copy/LTR/qualified-arm rules, DEFER arbitrary direct/nested/effectful IF/CFG/heap owner. No active hypothetical Product GO scoring or new Product/H1 GO, no distinct goal deviation found. USER-CLI-1 is a finite candidate for C review, not blanket v0/soundness or final adoption. Stop after final fixed-head CI report; no other Track or merge.
