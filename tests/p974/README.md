# P974 / C93: authenticate the current UNIT callable and guard V0 tables

An original sealed Copy receipt is history, not permission to call a different
current function entry. The common formal parameter-entry check now requires the
original retained body/plan, result, category, effect/owner flags and signature,
with the actual callee source and world. Historical GET is unchanged. UNIT,
unit-entry and parameter validation share this current authentication; scalar
formal validation retains its stronger original checks.

V0 now checks the current functions table before its first entry pointer and the
parameter table before indexing. The actual CLI rejection path next tries Array4;
its fallback entry and event walk also need table-presence guards. None grants
new source rights or changes canonical child destruction, public API/ABI or source
eligibility. All original parent tests/oracles/first results remain unchanged.

## Authority and source before execution

Main438170098d5b98096f1ad1f1f200dc1f326c75c3 adds process docs only; compiler
implementation baseline remains d579c761a78e3c84765e35061f77936789788d82.
CURRENT_SPEC -> Draft17.34. Exact parent PR969 is
fce069982e4b4ac8aa1b0488166490e54b66ae9f, tree7703b0c64235308394c19a118a43118ecb38d3d8.
Branch p974-current-unit-entry-authenticity is a separate worktree/branch; original
P965/PR969/962/954/946 stay untouched. Process4.1.2 grants in-scope local iteration.

Each candidate/test/oracle was frozen and reported in Issue974 before execution.
Final tested candidate d238528c414bb8fa71465f8aee27bdba9856dad9, tree
57b06d065defccd8f7da663faaab5053424db66b. A subsequent evidence-only commit does not
change this source. Earlier manifests bind their own candidates/registrations;
freeze-v3 binds the final pre-run inputs. Parent F78 findings are STATIC/NOT RUN;
no knowingly unsafe unpatched-parent negative was executed.

## First results, preserved

- Candidate292a91a:8/9 direct cases PASS, then named actual CLI subprocess-11.
  Static diagnosis: after V0 refusal the Array4 fallback still indexed the NULL
  table before checking whether an Array4 carrier exists. Preserve crash/log/source
  hashes; never rerun this unsafe candidate for a sanitizer trace.
- Candidated049cc0: NULL-table direct and actual CLI invalid/output/cleanup phases
  succeeded, then the healthy charge_false expectation0 failed. The original
  parent healthy source also returns4: this was a fixture precision assumption,
  not permission to widen source semantics. Preserve the failed expectation.
- Final variant keeps charge as a precision4 control and adds the existing P631
  native-emitting source for healthy retry0. Old named.py/oracle remain frozen;
  named-v3 is a separately frozen variant. The charge negative is not deleted.

## Finite local evidence

- Full GCC frontend: original parent525 + additive10, **535/535 PASS** with actual
  CTest inventory. Historical P605/P634/P965 source/hash and pinned formatter PASS.
- Clang23.1.3/ASan+LSan/UBSan each pass new10, P633/P817/P785/P790 core5,
  selected unit neighbors20, original owner/custody43 (including C89), captured/
  five-root compatibility10, and original strict source/native integration.
- New metadata matrix:4 P-owned sources x18 faults/config. Real A/B share plan,
  source and numerical IDs but are independently CHECKed; separately registered F
  has equal signature/ID and a different retained live helper. Body/result/kind/
  effect/owner/signature/source/world/table faults preserve historical GET and
  reject current UNIT/unit-entry/parameters or scalar/body/parameters/event. Equal
  signature-array values alone remain a valid positive with the original plan.
- Four real V0 null-table probes preserve parameter/program/output sentinels and
  restore before teardown. Actual CLI named tests preserve old bytes/dev/ino,
  no partial C/temp artifact; valid healthy retry0, pre-existing precision4 retained.
- Fresh **494 original ownership trials and502 reached OOM positions per build**:
  CHECK320/definition182; full original frozen records match. Unrestored A child/
  count/base mutations free A originals once/no B, B healthy then destroys once.
  All selected failures are unpublished, registry unchanged and same-context
  retry healthy. This is finite selected coverage, not exhaustive OOM proof.
- Echo/Window normal CLI -> unchanged strict C17 GCC/Clang O0/O2:16 raw and48
  observed guest heap0/1/2 runs across none/address+leaks/undefined. Original H
  physical identity and old T Copy/TRUE-FALSE, named write-failure/retry verified.

See evidence/local-results.json, first RED logs, complete reached-OOM/native records,
source hashes and private-repair.patch. SHA256SUMS covers the retained evidence.
R963/R970 independent source/oracle/fixtures were not read or reused.

## Limits and handback

Faults assume genuinely live objects and real backing extents. Temporary world/
function/table fields restore before teardown; original owner child/count/base
trials intentionally do not. No arbitrary dead pointer/opaque-owner/forged length
safety is claimed, and no malformed generated C is native-executed. Array4 event
NULL guard is source-audited; new direct negative covers the actual fallback entry,
while existing Array4 regressions cover normal behavior.

This is the pre-PR local evidence snapshot. C93 allows one new stacked DRAFT PR,
at most2 natural FULL5+PARTIAL cycles/12 jobs, no manual rerun. Independent F/R
fixed-code validation, READY/merge and Product GO are not granted. CI run IDs and
checkout trees will be reported in Issue974 without regrading earlier failures.

VALUE-ALIGNMENT-RISK: current-entry and first-consumer checks strengthen finite
original CHECK correspondence, but Copy/V0/fallback dependencies increase compiler
audit coupling and unmeasured cost. Human library-author plus ordinary-safe-caller
UB audit/re-audit burden versus well-factored C remains UNMEASURED. KEEP Draft17.34,
REPLACE NONE; Product GO PAUSED, H1 HOLD.
