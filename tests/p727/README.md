# P727: post-finding diagnosis, NO PRODUCTION REPAIR

DRAFT / DO NOT MERGE evidence only. Base P724
`13532ab30c55b74a6685be278fa2669b8300d6a1`, tree
`ec18bd9a5f44c1becc993910b363a02c296728c8`; R726 HEAD
`e33d903738dcba43b45ddc75a978c63f5674c84d`; canonical Draft17.33.
Current docs-only main `ddc6bc478583deff13da19a0f6426d19c8aeb7f8`
does not change the frozen compiler input. This branch changes only tests/p727.

This is P's post-finding forensic work, not a new R run, fresh BLIND source,
R calibration or R PASS. The original first BLIND u8-equality inputs were
source-ineligible; the R v2 usize comparison family was explicitly NON-BLIND.
R's original sources/oracles, commits, failed Actions, PR726 and producer
PR724 remain unchanged. `repro/` holds byte-for-byte copies with provenance
and SHA in `evidence/r-evidence-hashes.json`; those copies are never formatted.

## Reproduced unchanged candidate

Exact A0v2 source SHA
`b00d4c58cb77c9180c9a3e17bc3396ea72888fc71a8deff21c8e2f8588018180`,
832 bytes. Ordinary CHECK=0. R's original public gate fails at line19 with
exit1. A P-owned public-status harness records **3 =
NL_CHECK_ANALYSIS_PRECISION_LIMIT**, for unit/IF/parameter validators.
Ordinary C17 admission is **CLI4 V1-BACKEND-UNSUPPORTED, zero C bytes**.
No R-original positive native result is claimed.

P-only instrumentation logs immediately before existing `false`/`INVALID`
returns in a separate compiled copy of the exact candidate's copy_body.c.
It changes no predicate, semantic state or status and never replaces the
Production library. Uninstrumented and instrumented statuses agree.

The first rejected invariant is `parameter_entry`, copy_body.c:923–930:

```
block->tail != p->call
```

For the original input: call=24, function=10, original root=1, child=3,
declared field index=1, scope=1, normal_arm=1. Caller loan block=23 has
**tail=0, item_count=1, first_item=27**; item27 is a checked STATEMENT whose
initializer is the genuine call24. Its result is unit and it does not
terminate. Every other predicate in that block/type/result guard is true.
The three public APIs reach the same parameter-entry refusal before IF
operation/return replay. This is not a wrong Domain, old Copy, field or
forged issuer discovered at that first gate.

The direct-call-tail restriction and its explicit finite-profile comment
are inherited from M2 (`git blame` attributes lines921–923 and930 to
`e917906f`). M5 adapted unit result counts without removing that restriction:
"The finite caller linkage is a direct call tail in this scoped body.
Richer callers remain checked but need their own C lowering contract."

## One-difference controls, frozen before execution

All five P-derived sources are separately hashed in variant-freeze.json.
They are not edits or replacements of R's source/oracle.

| P-only difference from exact A0v2 | CHECK | Public validator | C17 |
| --- | --- | --- | --- |
| Delete one semicolon from the call within caller loan | 0 | 0 | 0, 2272 bytes |
| Add explicit unit tail in callee normal arm | 0 | 3, same first guard | 4, zero bytes |
| Remove precall target Copy read | 0 | 3, same first guard | 4, zero bytes |
| Remove precall sibling read | 0 | 3, same first guard | 4, zero bytes |
| Remove independent receiver write | 0 | 3, same first guard | 4, zero bytes |

The tail-call-only control retains precall target/sibling copies, normal-arm
implicit unit, caller continuation and second original Change. Its actual
loan block is tail=24, item_count=0. Unit/IF/parameter validators all return0;
no instrumented rejecting return is logged. No compiler patch is required
to distinguish this already supported shape. Native was not run on the
original rejected source or these diagnostic controls: do not convert a
source/validator/C17 observation into a physical/native PASS.

## Classification and stop

**Sound unsupported finite caller-linkage profile / NO-CODE.** The original
source is language-legal in the admitted source/checker and CHECK should
continue to accept it. Under the frozen *tail-call-only* evidence/admission
contract its validator must return precision, and C17/native admission must
fail closed. `view == true` is historical observation, not a claim that all
validator/profile obligations have passed. R correctly called the public
API; there is no demonstrated harness misuse or forged-authority acceptance.
The CHECK/view/validator discrepancy remains an accurately reproduced R fact.
It is not established issuer corruption or runtime unsoundness.

The public unit-view documentation does not clearly spell out this inherited
caller syntax boundary; the generic precision/CLI4 diagnostic is not very
helpful. This is a real usability/coverage/documentation limitation, not a
reason to silently redefine canonical legality or relabel the R gate PASS.
P722's finite success must not be read as support for all unit-call statement
forms. Coordination retains the adoption HOLD.

Issue727 Phase B explicitly says STOP without a compiler fix when the input
is soundly outside the intentionally finite profile. Therefore no guard is
removed, no new issuer/API/driver behavior is added, and no FULL5/PARTIAL or
repair PR is started here. In particular, do not merely replace the tail test
with an unconditional allowance for statements: a supported statement form
would need exact call-wrapper/source order, unit result, single occurrence,
normal/terminal control, postcall/root/field/scope and tamper/OOM validation.

C alternatives (not executed):
1. Authorize an R-owned NON-BLIND calibrated continuation using the existing
   tail-expression shape. New source SHA and before-run oracle must preserve
   original R bytes/failures; it is not fresh BLIND or proof of statement-call
   support.
2. Authorize a separate finite Production caller-linkage extension for one
   unit call statement in a loan block, with explicit graph coverage and
   return/scope/unit rules and faithful negative/OOM/native regressions. This
   likely reuses current semantic/issuer primitives; estimated 50–120 lines
   plus tests, 60–120 minutes and one natural FULL5/PARTIAL pair, subject to
   real implementation audit. It does not require new language semantics on
   present evidence. No cost estimate substitutes for authorization/safety.
3. Separately improve public profile documentation / stage-specific
   diagnostics. Do not fake authority or change the validator to match a test.

## Reproduce

Build the frozen P candidate (frontend-only is sufficient for these stages),
then from this worktree run:

```
python3 tests/p727/reproduce.py /path/to/candidate-build gcc /tmp/p727-results
```

The script asserts unchanged Production/include/CMake input, exact frozen
copy_body.c and R reproduction hashes; compiles R's unchanged gate, P status
harness and separate predicate-preserving instrumented copy; then records
original/variant stage results. `evidence/phase-a-original.json` and
variant-results.json retain first observation; reproduced-results.json and
instrumented-copy-body.c.txt retain the scripted rerun. Instrumentation is
diagnostic test-owned C text, not a build target or issued authority. No CMake
registration, CI or Production changes are included.

Goal Alignment: Compass -> Product Strategy §§1.1/1.3 and Process
§§4.2–4.3.2/7/7.6 were consulted. This maps an ordinary helper usability gap;
it demonstrates no author+ordinary-safe caller audit-locality improvement
or Product GO. PAUSED624 and H1 HOLD268 remain. No canonical/Process/Roadmap
edit, main merge, Issue close or other Track launch. P stops for C's decision.
