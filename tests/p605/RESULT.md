# P605 HOLD — GCC13 also refuses a truth-changing synthetic mutant

## Authority and completed entry gate

Fixed main2cca0c6118cdf725ee68b48e07fc566b29622230, canonical Draft17.32.
Original PR604/bdb505d5350fd5ad9e74affe93397a8b2af4997a is immutable.
Latest Coordination6256757167 and live main were checked; postmerge PUSH
FULL39178092674 and PARTIAL39178092814 completed SUCCESS/attempt1/exactSHA.
Real local main and old-head checkouts exist separately, and main was freshly
built and all325 frontend CTests passed. No CI or moving-main retarget.

This new result does not replace P603's final or original failed CI. All61
original files except the explicitly authorized integration_test.py repair are
hash-preserved: source, frozen.json, original baseline/candidate/native logs,
PREFLIGHT/RESULT and probes. No old successful/failed evidence was overwritten.

## Local integration and exact proof preservation

First follow-up is merge1db3648174a8912584550b8a2d013142b80a3287,
parents originalbdb505d and fixedmain2cca0c6. Five anticipated conflicts were
resolved in syntax.h/checked.h/parser.c/semantic_check.c/main.c only. Selected
subtraction enum IDs remain fixed; the unmerged DRAFT < tag follows subtraction.
No serialized/cross-version artifact compatibility is claimed for that DRAFT tag.
No proof receipt format/issuer/validator modification. Byte checks preserve:
- src/usize_addition.c
- include/newlang/usize_addition.h
- include/newlang/usize_subtraction.h
- src/semantic_internal.h
- the exact usize_arithmetic checker/capture function body from selected main.

Only the shared flat operator choice, boolean comparison selection and original
C17 operator dispatch combine + / - / == / <. No precedence, CFG, dynamic proof,
IF consumer, alias/owner or new language mechanism. Merge-focused23/23 passed on
GCC14.2 including P596 public/native/negative and Array opt-in/old equality/plus.
A guarded local merge-script error was fixed before committing/validation; no
remote attempt was spent on it.

## Phase-aware partial harness repair

Original positive strict compilation/guest observer assertions remain mandatory.
TRUTH_CHANGING must strictly compile and execute with observer exit91.
SAME_TRUTH, when compiled, executes with exit0 and is not a wrong-result falsifier.
Its compilation failure records categorySAME_TRUTH/outcomeMUTANT_COMPILE_REFUSED,
compile exit/diagnostic and native_exitNULL. It is never a CHECK/native PASS.
-Werror and original NewLang sources/oracles stay unchanged. A harness-only #error
unit checks refusal cannot pass original/changed categories; native wrong exit0
cannot be called a changed-mutant refusal. It is not guest/compiler evidence.

GCC14.2 focused7/7 passed: P603 parser/value/rollback/OOM/native, phase unit and
supplemental arithmetic/comparison integration. All9 original native sources run
here; the new JSON separates real original native from synthetic mutation phases.

## Actual GCC13 first new blocker (STOP, before remote CI)

For a real local diagnostic compiler, official Debian HTTPS Packages index and
SHA256-matching .deb payloads were extracted under /workspace/p605-tools, without
system install, repository/CI lock change, or maintainer script execution.
GCC is Debian13.3.0-16, distinct from Ubuntu13.3.0 CI. This provenance provides TLS
and index digest checks, NOT a claim of independent OpenPGP verification.
See local-gcc13-provenance.json; native development files use the existing host.

The original known case is reproduced and correctly phase-recorded twice:
0<=SIZE_MAX, SAME_TRUTH, strict compile exit1, MUTANT_COMPILE_REFUSED,
native_exitNULL at O0/O2. Its source SHA and original/mutant C SHA are retained.

After this repair, the SAME actual compiler reaches the next > mutant:

    const _Bool nl_local_1 =
        (((size_t)UINTMAX_C(0)) > ((size_t)UINTMAX_C(18446744073709551615)));

At max_true.nlgt.c:15:54:

    error: comparison of unsigned expression in '< 0' is always false
           [-Werror=type-limits]

This is TRUTH_CHANGING: original0<MAX is true, mutant0>MAX is false. Compilation
stops at O0, so no executable or observer exit91 exists. It is NOT a Production
< CHECK/backend failure, not an observed unsafe acceptance and not native PASS.
The local fixed integration script FAILS as required by the exact changed-mutant
contract; gcc13-new-blocker.log preserves it. First four source rows completed;
later GCC13 rows after max_true remain NOT COMPLETED. No skip/flag relaxation/
guest injection, changed-truth compile-refusal pass, or unauthorized CI retry.

The first meaningful new test-policy blocker is therefore established. Per C's
stop instruction, neither the new FULL5 nor PARTIAL was launched. The existing
remote PR604 branch remains bdb505d and DRAFT/conflicted; no partial repair was
pushed, because that would automatically spend CI before this local gate passes.
The two local follow-up commits and a git bundle preserve concrete partial work.

## Supplementary already-frozen local integration evidence

Two new P605 sources, frozen before execution, use separate completed flat
expressions in one real artifact: 0+0,9-2,0<7 true; and0+0,7-7,0<0 false.
Existing public +/- view+validator APIs revalidate both genuine source-issued
receipts and reject the other operation; comparison is only an ordinary copied
bool with no memory authority, not a new public proof. Public observations are
after original input/parser/AST teardown. Strict GCC14.2 O0/O2 passive native
observers read all original arithmetic, bool and Copy carriers. No host-seeded
world or altered guest quantities. Four frozen mixed -/< and ==/< expressions
remain SOURCE/CLI3/Cbytes0. This is finite integration evidence, not a new API.

Full final candidate/Clang/ASan/UBSan CI NOT RUN in this task; original P603
Clang/ASan/UBSan PASS and original GCC failures are historical, not recertified.
R601's independent calibrated LIMITED ACCEPT/NOT RUN allocator/FILE* OOM remains
unchanged. P-side tests are not new independent R validation. No new Product GO.

## Next decision / value / stop

C must decide synthetic-test evidence policy before another implementation/CI
allocation: whether a truth-changing mutant may be explicitly compile-refused
(with no native claim and other runtime falsifiers), or an explicitly approved
alternative mutation must strictly execute. P did not silently make that choice,
remove a negative, rewrite original oracles, suppress -Werror or alter semantics.
No privileged proof change/general mechanism appears needed by this result.

Compass→Strategy1.1/1.3/Process4.2/4.3.1/7 were read. Historical gate N/A faithful
integration/test repair; KEEP exact scalar types, ordered Copy, guarded +/- and
source-vs-memory authority, DEFER broader language/proof/owner mechanisms.
Goal is a usable total comparator and trustworthy regression evidence; measured
compiler capability and the synthetic-test limitation are separate from actual
UB-prevention author+ordinary-safe-caller audit/re-audit locality versus factored
C, which remains unmeasured. No Product/H1 GO or goal deviation.

STOP after one final Issue605 report. No main merge, PR599/public-proof/canonical/
workflow/toolchain edit, remote branch update/new CI or other Track startup.
