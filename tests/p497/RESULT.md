# P497: Phase1 typed usize candidate; decision-bearing guard HOLD

Fixed compiler base e666b4d36ce6c678101bc8353af76ad13b06ef54, canonical
CURRENT_SPEC -> Draft17.30. Pre-code/pre-run freeze
be32f2e7dd115d38e0dd65dc201624627ea262da. Seven source SHA256 and immutable
oracles: frozen.json. No source or oracle rewrite after execution.
Baseline binary SHA and all14 raw CLI results: baseline.json. New runs:
phase1.json, evidence/{gcc,clang}.jsonl. Later main556aeee changes only two
documentation files (PR498); its Coverage Map§§3.1–3.3 was read as navigation,
not substituted as compiler authority. PR479/494 are separate unmerged work.

## Measured stages

| Source | Fixed baseline | Phase1 candidate | CHECK / public / C17 / native |
|---|---|---|---|
| guard_true / guard_false | CLI3 parser P5-EXPECTED-EXPRESSION3:17–3:18 | CLI3 parser P5-EXPRESSION-UNSUPPORTED5:16–5:17 at first = | not reached |
| literal3 / literal9 | CLI3 parser P5-EXPECTED-EXPRESSION1:36–1:37 | CLI0 CHECK and emission | actual source-owned usize3/9, real size_t, strict GCC/Clang O0/O2 PASS |
| overflow2^64 | CLI3 parser | CLI3 semantic USIZE-LITERAL-RANGE1:30–1:57 | no executable C |
| u8 vs usize equality | CLI3 parser4:17–4:18 | CLI3 parser5:16–5:17 | semantic safety UNTESTED |
| equality-bool as ref | CLI3 parser1:29–1:30 | CLI3 parser1:58–1:59 | semantic safety UNTESTED |

Full true/false decisions7/8, Pair old0/sibling9/root preservation are NOT
measured; both complete sources stop at equality. This is not a comparison or
conditional native PASS. The independent conditional-emission blocker is
inspected code, not an executed post-comparison diagnostic: main.c's
v0_validate_node/v0_emit_node have no IF case, and checked_c_node.c consumes
Node match proofs, not Pair bool-if effects. Existing source_if forks owned arm
worlds and widens changed post-state; copying an arm or emitting both eagerly
would not faithfully lower §27.4. That requires a separate source-arm/physical
Pair carrier consumer beyond Phase1; no guard bypass or new proof attempted.

## Implementation and trust boundary

One exact usize(DIGITS) AST/checked kind appended to preserve existing enum
numbers. Generalized the existing u8 literal accumulator using
value <= (maximum-digit)/10 before multiply/add, bounded by native SIZE_MAX.
Literal is a true core usize Copy/discardable package with source span and
scalar evidence, not a user function or host-staged constant. Existing u8 range
and diagnostics remain. Lexer/infix/bool/if/ABI/authority mechanisms unchanged.

The narrow scalar emitter follows source-issued literal/Copy binding chains
through strictly decreasing checked IDs, iteratively. It checks exact type and
known scalar consistency, then emits real size_t locals and UINTMAX_C values
from checked evidence (no source names/text privileged). Existing u8 call and
Pair/Node mutation validators are not relaxed. The current frontend has no
explicit cross-target quantity model; this finite implementation uses its native
size_t width and emits _Static_assert(SIZE_MAX == compiler_native_max).
A foreign-width C target fails rather than accepting/truncating a host-width
literal. Only64-bit Linux native measured. Core layout/FFI/sizeof-ABI unchanged.

Public probe starts from the actual .nl declaration unit and actual main()
call, without semantic seeds or host-built values. It reads owned child body
and verifies literal kind/type/source spelling/scalar3 or9, independent Copy
value ID1->2, same symbol1, dependency-free and zero Domain/Allocation/ref
rights. Span14:22 is within the retained function-body source, not the original
whole file coordinate. These are public views of trusted checked evidence;
there is no newly independent public literal validator/theorem. No Node join
or historical call certificate is invented or reused as quantity authority.

Generated C digests: literal3
5b1f19409af1c08b7af74154b73310225eec3e9f8adf93f6bf953cf46904927b;
literal9 1be28e550b474cd7398d2510106b03f531f45959ff260b6f570b1b2d1c62be50.
GCC14.2 and Clang23.1.2 each strict C17 -Wall -Wextra -Wpedantic -Werror O0/O2:
plain and passive original-local reads each exit0 (16 native successes total).
Observer only reads already initialized emitted locals. Broken emitted-carrier
mutants exit91 in all8 runs using the unchanged observer. A deliberately wrong
size-width static assertion is rejected by both C compilers; actual32-bit/cross
compilation UNTESTED. No claim that a discarded value alone was observable.

## Regression / failure evidence

Added five CTests: parser, scalar/range, real function body, OOM/rollback,
source-owned+C17/native. SIZE_MAX accepted in semantic test,2^64 and very long
decimal rejected, leading zeros accepted; no wrapping. Actual-source unit body
checks Copy propagation and retains source after original parser/tree destruction.
Semantic allocator injection covers20 source-fragment,63 registration,26 actual
main-call malloc/realloc failure positions; each error explicit OOM with NULL
artifact and public state unchanged. No new allocation path introduced.
Existing same-type gate refuses usize actual->u8 formal(P3-TYPE-MISMATCH),
u8-declared result of usize(P8-BODY-RESULT), quantity as write-ref
(P3-WRITE-REF-REQUIRED). Equality-specific wrong-type/false-rights remain
UNTESTED because parser stops first, not counted as semantic safety evidence.

Final local GCC frontend suite289/289 PASS31.01s, including P471/P490,
u8/nested-call/fixed-Pair/Node, existing borrow/ref/nonCopy/EndRoot/Option,
rollback/OOM regressions. Final Clang native source test PASS; formatting PASS.
Initial new-test harness had an invalid reference-view member and two guessed
negative diagnostic codes; corrected to the real public `place` member and
existing diagnostics, without changing frozen source/oracles. One direct format
script invocation lacked executable mode; invoking repository script with bash
passed. None was a production semantic finding.

## Reproduction / decision

Configure with -DNEWLANG_FRONTEND_ONLY=ON (local preinstalled LLVM SDK is23.1.2;
full CI retains current curated23.1.3 lock). Build; run ctest -R p497 -V, then
all CTests. Run tests/p497/frontier_test.py NEWLANGC P497_PUBLIC_PROBE CC
ABSOLUTE_TESTS_P497 for each GCC/Clang. The natural unchanged workflow FULL+
PARTIAL CI pair is submitted once on the candidate HEAD; definitive run IDs,
statuses and actual tested merge/head are recorded in PR and final Issue comment.
No workflow/lock/bootstrap changes or CI rerun/passing-head shopping permitted.

Historical Gate A–E and goal rationale: PREFLIGHT.md. Faithful §6 implementation,
no canonical/ledger rule change. Phase1 is a potentially reusable quantity
literal prerequisite only; comparisons/bounds/byte/Array/loops and ordinary-caller
UB-audit locality/C-Zig-Rust fairness are NOT achieved. The map gains exact
first refusal rather than another claimed general numeric success. Next decision
belongs to Coordination: separately scope comparison typing/public bool evidence
and finite if/Pair effect lowering; do not automatically continue here.

Final: PHASE1 FINITE TYPED-USIZE SOURCE→OWNED→C17→NATIVE CANDIDATE — UNSELECTED;
PHASE2 QUANTITY GUARD HOLD — EQUALITY SOURCE / INDEPENDENT IF EFFECT LOWERING.
No main merge, Issue close, other Track, H1/STB/Ring scoring or normative change.
