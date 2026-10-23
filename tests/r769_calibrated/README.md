# R769 calibrated independent evidence — pre-execution freeze

Status: **NON-BLIND CALIBRATED, not FIRST-PASS BLIND.** Authored after
the R765 original source/oracle freeze and the original first red. The
original R765 files, oracle SHA, corrected R765 HEAD and historical CI
remain immutable; do not conflate these new results with the first pass.

## Fixed authority and execution boundaries

- Canonical Draft17.34 §16.3.8, exact compiler candidate
  `492652101a10bb573ffdfc020771334aa1dba375`.
- Additive R branch parent `25e98a8831b32f387a94097a8c6c54c5ddfe9ff2`,
  original R freeze `d20f35ce8aaa9bb90f16bd8d8a1e60d7a01b93de`.
  R765 original 14 cases (two genuine positives, 12 negatives) and frozen
  source/expected-byte SHA remain unchanged. Never edit original R PR #768.
- Execution solely normal Chat + GitHub connector + new R branch + GitHub
  Actions. **No Work/Codex local compiler execution; no merge.**
- The `suite.py` harness verifies all 14 R765 source SHA256/oracle SHA256
  before any new test. It materializes two *independently loaded* identical
  source files per A/B provenance case. Distinct parser/registered semantic
  contexts, distinct source objects, distinct checked artifacts. Identical
  source text implies intentionally colliding context-local numeric TypeIds;
  numerical equality and equal layout do not grant foreign authority.

## New independent artifact hashes, before FIRST calibrated CI

- `tests/r769_calibrated/probe.c`: SHA256
  `20b3e6a169990e476f3bba28ec7e20e62658d8d00ed8dfd651d7d4e811e27222`.
- `tests/r769_calibrated/suite.py`: SHA256
  `0830618fea6e9f0fb6f41dd085878f7500fe6a489feb2d712216f10f99732c56`.
- The new test expectations below are frozen before the first candidate Actions
  run, NOT retroactive FIRST-PASS BLIND predictions.

## Named assertions / falsification criteria

1. `r769_foreign_A.calibrated` and `r769_foreign_B.calibrated`:
   R765 sealed H-first and T-first positive source respectively, **twice
   independently parsed and checked**, same numeric H/T layout and TypeId,
   but foreign semantic contexts and owned artifacts. Both actual source-issued
   body/parameter validators MUST return `NL_CHECK_OK`.
   Foreign body, foreign live_exit, full foreign body packet, foreign
   entry/handoff, full foreign parameter packet and foreign public event
   views MUST return non-OK when passed to the other actual caller.
   Every altered function/root/event/result and
   parent/child/index/incarnation/current-fact/old+new Copy/loan/arguments/
   formal-parameter packet MUST return non-OK. Genuine packets MUST still
   validate after these attempted substitutions.
   Separately and explicitly **PRIVATE** diagnostic tampering of genuine
   current H layout-known/size/alignment, historical live scope and child
   current fact MUST be rejected by existing public revalidation after
   each in-place mutation; exact historical values then restored.
   Private perturbation is NOT a publicly injectable attack and cannot
   substitute for different-context public packet swaps.
2. `r769_oom_A.calibrated` and `r769_oom_B.calibrated`:
   independent R765 A/B checked source, deterministic host wrappers on
   malloc/calloc/realloc. At each parser, mixed registration and three
   public validator stages first collect the actual number of wrapped
   allocation calls on a success, then fail **0–31** (when reached), plus
   middle and final three reached positions. Each genuinely triggered
   fault MUST produce the exact respective `NL_PARSE_OUT_OF_MEMORY` or
   `NL_CHECK_OUT_OF_MEMORY`; not parser errors or unsupported refusal.
   Failed parsing leaves NULL syntax tree, parser reusable; failed
   registration leaves preexisting `intact` function and complete public
   original context unchanged, including types/known layout, facts,
   places/domains/regions; preserved function's body pointer unchanged.
   Normal retry MUST succeed and registration alone must grant no
   place/domain/backing authority. Failed public body/parameter/event
   validators may not change current or handoff state; genuine retry after
   allocator restoration MUST validate. Logs MUST disclose **actual**
   position counts; no false assertion of all fault positions.
   In-place original-state layout mutation above is isolated from the OOM
   tests; failure should not leave mutated state.
3. `r769_negative_stages.calibrated`: all 12 immutable R765 negatives
   MUST return CHECK3 and C17 emit3, with zero stdout/C bytes.
   Record each **actual** CLI diagnostic code and coarse stage based on
   code prefix. Unrecognized prefix is **uncertain**, never silently
   classified safe or semantic. Diagnostic results do not replace a
   dedicated parser/checker-stage observation.

## Limits / stopping conditions

Expected baseline with inherited R765 is 448 FULL5 / 447 NO LLVM PARTIAL;
adding five cases makes expected 453 / 452, conditional on identical CTest
registries. The required FULL5 is GCC, GCC Release, Clang (format/pin intact),
ASan and UBSan, plus distinct NO LLVM PARTIAL. One first natural immutable
HEAD pair. At most one correction pair solely for objectively R-owned
formatting/registration defects, preserving the original red. Any genuine
unexpected refusal, accepted forgery, OOM rollback leak, missing test, CI
policy drift or candidate drift stops without repair/retry-to-green.

This test does **not** prove arbitrary private-world transplantation is
publicly injectable, all 0–N OOM sites, post-arm guest physical frees,
an H+T dual-carrier certificate, mixed native C17, or Product GO.
Specifically same-source-owner replay, forged Allocation/Domain/BackingRegion
and old-compiler historical comparison require separate classification
where no public packet channel exists. If those remain mandatory and
unexercised, overall outcome is **INCONCLUSIVE**, never inferred PASS.
Product GO #624 remains PAUSED, cJSON H1 #268 remains HOLD.

Original R765 A frozen oracle's `Orbit` prose typo refers to
`LedgerSpoke`; frozen bytes MUST NOT be silently rewritten.
