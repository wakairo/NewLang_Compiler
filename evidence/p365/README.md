# P365 partial archive — STOP / DO NOT MERGE

**`P G4 OWNED8 HOLDER D6→C1: HOLD — FIRST CHECKED-IR / CUSTODY / PUBLIC REPLAY BARRIER`**

This is a stopped, incomplete test-only prototype, not an admitted feature or a
clean custody certificate. Coordination's
[STOP / SUPERSEDED decision](https://github.com/wakairo/NewLang_Compiler/issues/365#issuecomment-6120768428)
changed the next product step to the separately user-launched V366 review.
The user had already launched P365 before that decision. This archive preserves
that partial work under the user's subsequent instruction to retain useful
records; it does not continue the pilot or start V366.

The experiment's local HOLD and Coordination's sequencing decision are distinct.
Neither establishes that full-checked H1 is impossible or that H1 fails.
Localized, justified unchecked remains a separate product/source-surface question;
this experiment introduces no unchecked and makes no decision about that question.

## Authority and immutable positive

- Parent: PR353 `82578ff6eb23970e3ae934c98a38383ca01468f7`, DRAFT / DO NOT MERGE.
- Semantic main: `a5343bf70004b30a8b026e52aa2fde93b5920be0`; Issue365's current
  main `6e8c07ebbfac01d04acd76931f285e66936ee7b1` is docs-only relative to it.
- Canonical: Draft17.30 selected by `docs/reference/CURRENT_SPEC.md`; normative
  spec blob `db8fe08f811acc8286f2d37e9d3229b5d7a67067`.
- FormalProof main `a8de4ebb4099f4742fc161d26867740dcac48414` is proof-only.
- Source is exactly PR331 `46a02e60cabce1b63cc3db4bc9035fe06f5f8009`:
  `evidence/p329/returned_owner.nl`, copied here without edits.
- Source SHA256:
  `bd7692f8118c8e4de2b7662b0f90fcf503a548d9948bfded27bd430ad127716d`.
- Existing ten opt-ins ON, `NEWLANG_EXPERIMENTAL_FINITE_CHANGE_REPLAY=OFF`.
  The runner derives and asserts the ten switches from the immutable parent's
  P338 build script, rather than inventing an admission flag.

Design inputs were M364's
[finite source-node replay report](https://github.com/wakairo/NewLang_Compiler/issues/364#issuecomment-6120079496)
and [Coordination acceptance](https://github.com/wakairo/NewLang_Compiler/issues/364#issuecomment-6120114324),
plus P362's already recorded composition HOLD. This archive is KEEP existing
canonical semantics and DEFER new source admission/feature selection. No new
source syntax/profile, spec exception, Ledger rule, or normative substitution is
proposed. Process/Decision Procedure/Ledger authority checks do not imply a
completed new-admission history review or permission to resume the pilot.

## Measurements made before the stop

`measurement.json` separates host exits, checker statuses and untested work.
`stage0-owned.jsonl` records **unchanged** compiler calibration before private
replay work: observer exit 0; parser, registration and main-check statuses 0;
actual source-generated owned artifact; five captured validators, two scoped
call validators and the returned two-root terminal validator succeed. CLI exit
was not separately measured. The initial unchanged executable SHA256 was
`85a65077b56f8ce5b9f4e2c0145c8c580d4def4fb5af425e6f3d0e0eedf6f2fd`.
A fresh local GCC Debug build of the exact parent also ran the baseline observer
and graph observer successfully. These are source-checking experiments, not
native execution or runtime allocation-failure injection.

The genuine source retains five independently fallible roots, six setup links,
two named B helper calls, direct A/C detach, a whole Holder move, complete returned
B+dst TreeBOwner and donor-first/receiver-late release. Direct caller Changes=8;
each of the two independently checked callee bodies has one Change. Static
release-world counts for 0–5 are `[1,1,1,1,1,1]`. No ninth adoption is present.

`graph.jsonl` uses the public checked-node/semantic observation APIs on that actual
artifact. It records source dependency links (including unit parent bindings and
Copy wrappers), not a reordered host-generated semantic world. It confirms all
five original roots and all twenty fixed fields unchanged across D6→M→C1;
original B Allocation value29 and Domain value34 move carriers 30/33→95/96,
DomainId3/root31/incarnation34. The actual destination Domain place96 has
`implicit_local=false`. These IDs are observations of this source, not authority
created by JSON. The graph records the relevant slice, not a proof that every
custody dependency has been replayed correctly.

## First incorrect replay — retained, not repaired

`cursor.inc` is an incomplete guarded private replay. `isolate.py` compiles a
**copied** `captured_closure.c` translation unit with
`NEWLANG_TEST_P365_OWNED8_CUSTODY=1` into a copied archive. The repository's
`src/`, `include/`, CMake, public API and production archive remain unchanged.
It gates the normally named public `nl_checked_captured_closure_validate` after
its legacy validation, rather than claiming a standalone host boolean is public
validation. This private archive deliberately changes test-only validator
behavior; it must never be selected as a production checker.

The cursor clones genuine D6.after, follows source parent/operand links, consumes
and receives existing nonCopy values, reconstructs Holder/record bindings,
ends the old source scope and follows the fresh original-Domain loan into the
first named helper's entry. It has no new Allocation/Domain/original-root issuer.
It does not execute the helper's Change or extend C2/J/T replay. Source IDs,
nominal/type assumptions and incomplete replay logic make it a research sketch,
not a general verifier.

The first execution produced:

```text
P365_ANCHOR D6=110 block=104 loan=103 binding=115 values=124 bindings=57 scopes=11
P365_REPLAY M bindings=65 values=127 next_statement=155 loan=142
P365_ENDPOINT place=96
P365_HOLD node=149 reason=ENDPOINT_PLACE
```

Observer exit **1** is the existing positive observer assertion failing because
the guarded normally named public validator returned
`NL_CHECK_ANALYSIS_PRECISION_LIMIT` (**3**). Parser/registration/main still
succeeded and generated the owned artifact (`guarded-partial-owned.jsonl`).
The trace is test-only stderr, not a production compiler semantic diagnostic.
Node149 byte span `[12007,12053)` begins at source line201 column24.

Snapshot counts matched the actual C1.before (types21, bindings69, values136,
places100, scopes12, last-fact112). Endpoint place comparison passed places1–95
then stopped at place96. Later complete value/binding/scope comparisons were
**not reached**, so matching counts are not a complete custody certificate.

Read-only cause inspection found a prototype porting error:
`p365_receive` unconditionally marks type2 receiver places `implicit_local=true`.
The existing source checker `src/semantic_check.c` only sets that property for
simple bindings (`!multi && !destructure`). Source receiver141 belongs to
destructure parent138, so the actual Domain receiver place96 correctly has it
false. Thus this is a **false rejection in the incomplete replay**, not evidence
of unsafe source admission, an invalid source program, a missing Domain issuer,
or a canonical semantics defect. The required source parent information exists.
Per the first-incorrect-replay stop condition, that discrepancy was not fixed;
subsequent replay assumptions and memory/ownership correctness remain unaudited.

## Explicitly untested

- Genuine guarded public positive: **NOT ESTABLISHED**.
- Omission of consumed node131 or transferred node134: **UNTESTED**. No corrupt
  artifact was accepted or rejected under this incomplete guarded proof.
- Prototype OOM, rollback/snapshot nonmutation and leak/sanitizer checks:
  **UNTESTED**. The fresh source build is not an OOM or rollback test.
- Prototype GCC Release, Clang, ASan/LSan, UBSan and dedicated CI: **NOT RUN**
  after the superseding STOP. Existing parent CI is not this prototype's CI.
- Archive PR ordinary CI may run automatically; its state is not prototype proof.
- No ninth dst.child, further helper/boundary, adoption, join, backend, native
  execution, new Track, merge, issue closure or H1 PASS/FAIL claim.

## Reproduction for a separately authorized future investigation

These commands are documentation, not authorization to restart the stopped task.
Use the repository's locked toolchain and its supported CMake version. From the
repository root (build directory must be absolute):

```sh
bash evidence/p365/run.sh /tmp/p365-reproduction
# This should succeed with the normal exact-parent compiler:
cat /tmp/p365-reproduction/stage0-owned.jsonl
cat /tmp/p365-reproduction/graph.jsonl
# This reproduces the known first FALSE REJECTION, not a proof success:
bash evidence/p365/reproduce-refusal.sh /tmp/p365-reproduction
```

The reproduction script requires observer exit1 and the place96/node149 trace.
It intentionally retains the bad flag behavior to preserve the first failure.
No new acceptance test, mutation generator or CI workflow is installed.
`SHA256SUMS` checks the archived files; logs are diagnostic data, not serialized
ownership authority. If separately authorized after the product-surface review,
first reconcile ordinary vs destructuring receiver metadata with existing source
semantics, then establish a genuine guarded positive before any omission claim,
OOM/rollback checks or extension to another boundary.
