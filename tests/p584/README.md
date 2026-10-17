# P584 — emitted C17 exact-u8 target precondition

Fixed authority: af2086216d27acb5fc8f7f745d2d93ee1171973a,
CURRENT_SPEC Draft17.32. DRAFT / DO NOT MERGE. Related PR576 is untouched.
See PREFLIGHT.md and frozen.json for the pre-edit decision and source SHA256s.

## Meaning and scope

One shared private string, src/checked_c_target.h, is emitted before guest
code by main.c, array4.c, checked_c_node.c and checked_c_five.c. It requires:

- CHAR_BIT == 8;
- UINT8_MAX == 255, sizeof(uint8_t) == 1, and (uint8_t)-1 == 255.

C17 6.7.10 makes a false _Static_assert a constraint violation requiring a
diagnostic (5.1.1.3). CHAR_BIT is bits per C byte, not sizeof(char); 16 != 8
regardless of what an SDK calls uint8_t. The second assertion also rejects a
wide or signed misleading alias. Missing standard exact-width typedef/macro
fails compilation. These are integer constant expressions using C17 mechanisms;
no arithmetic/runtime conversion/storage/guest semantics is introduced.

This is a fail-closed requirement on the whole bounded C17 backend, including
outputs without an actual u8 use. It is NOT a full ABI portability certificate,
new target model or C28x support. A C compiler ignoring required diagnostics is
not qualified. Existing SIZE_MAX and heap layout/type guards remain unchanged.
Compiler source/check/public-owned admission and failure/output management are
unchanged. Unsupported C targets are rejected at GENERATED C compilation,
not at NewLang CHECK: the source checker does not discover the downstream ABI.

## Evidence and reproduction

Six actual registered-source outputs cover scalar bounds, Pair known-call +
scoped write, one/two original opt-in Arrays, lexical Node join, and five-root
heap representation. Each source CHECK and applicable existing public probe
succeeds. Before-code hashes and synthetic baseline outcomes are retained in
evidence/baseline.json; selected actual before/after C is also retained. Deleting
only the new guard reproduces each entire baseline C hash, including all
functions, values, field/Option/array carriers and existing target guards.
No source/expected result or previous test fixture was modified.

The focused c17_target_model.integration test compiles all six strict C17 O0/O2
with the selected C compiler. Five are also executed; the five-root program's
existing separate native test supplies its required allocation adapter.
A passive scalar observer checks actual generated callee results 0 and255,
without changing guest values or supplying a host result. public-bounds.txt
retains actual source-issued returned Copy packages. Existing p521, p534, p541,
Node and five-root native tests separately check physical fields/options,
original addresses/strides, old Copy and failure worlds.

For each of six outputs and each GCC/Clang compiler, six preincluded synthetic
headers separately model CHAR_BIT16, a wide alias, a signed alias, wrong/missing
UINT8_MAX, and combined16. All must fail compilation, with the guard diagnostic
and no object. The normal host still has CHAR_BIT8: these macro/typedef-spelling
probes test guard logic, NOT a 16-bit physical C implementation, TI CGT, firmware,
board, flash or runtime. The unchanged baseline accepts the combined probe.
Raw combined diagnostics and GCC/Clang result JSONL are retained.

Semantic negatives u8(256), an existing semantic operand error, and a valid
but backend-unsupported two-argument call preserve status3/3/4 and zero C stdout.
The last remains CHECK0. /dev/full still reports the existing V0-C-OUTPUT error.
Existing proof/validator corruption and OOM/rollback tests run unchanged.

Configure/build with the repository pinned toolchain (normal LLVM build, or
-DNEWLANG_FRONTEND_ONLY=ON for the explicitly separate PARTIAL configuration),
then run:

    ctest --test-dir build -R c17_target_model --output-on-failure
    ctest --test-dir build --output-on-failure

Manual GCC/Clang evidence uses the same test command registered in CMake.
Local toolchain: GCC14.2, Clang23.1.2. Locked CI evidence is reported separately
in the PR/Issue. No workflow/bootstrap/lock/Canonical changes.

## Test-development transparency / stop

An early local evidence collector overlapped relinking newlangc and saw EACCES;
a separate local full run overlapped pruning duplicate evidence files and its
already-loaded test saw a removed old golden file. These are P orchestration
errors, not source/compiler findings. After the build and evidence layout were
fixed, rerun focused/full tests on the stable tree. Raw candidate logs retain
the actual failures; no frozen source/oracle or C guest body was changed to pass.
The final test keeps hashes instead of duplicating the long five-root C output.
No such overlap or manual rerun is introduced into Actions.

Historical gate N/A: faithful canonical5.1/6/V1 exact-width u8 implementation,
not a source-rule choice. Product Strategy1.3 Exploration GO: reusable backend
quality; no C28x ABI, general target support or Product GO. Goal/Evidence/
Alternatives/Stop and remaining unsupported environments are in PREFLIGHT.md.
Coordination owns adoption; no merge, other Track or H1 restart.
