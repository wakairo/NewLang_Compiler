# R881 CALIBRATED GATE A — frozen before PR/CI

Track R only; NEW NON-BLIND calibrated on fixed P878, not a rerun/regrade of original R875 FIRST-PASS BLIND INCONCLUSIVE. Canonical Compiler/main CURRENT_SPEC -> Draft17.34. Relevant authority read: Compass -> Product Value Strategy 1.1/1.3 -> Development Process 4.2/4.3/4.3.2/7/7.6 -> Draft17.34 4.9/13.5c/17.4/27.3–27.4 -> DI-017/018/Design Decision Procedure.

Immutable compiler HEAD 670f976d51cff52efe182d32fcb37aed3394063b, tree 18b547d99328ca714c6d2e1c52f046f2a2e07a8b, fixed P878 branch p878-unit-loan-binding (PR879), exact parent P872 a8057d1f82e1c6ca498d1ff43860fdf9822b97a0. NO rebase/cherry-pick/merge/main. Historical R875 HEAD 479d6b2f31c1294f0cbccd038d51da347e0208c0 and first pre-P freeze c1a88a37f5cb35f15f7bd562f83001e9fcca3bde untouched; historical FULL5 40091593118 FAIL and PARTIAL 40091593144 FAIL remain, grade INCONCLUSIVE.

The following files at tests/r881/ are byte-identical copies of existing frozen R875 source/negative inputs, not adapted sources or P fixtures. SHA256:
- sources/true_then_normal.nl = 5b135246445caa4fa33c38ffa470396ebe15e6d25a29db08bc9abbba6cd1e57e
- sources/false_else_normal.nl = 072bdffbf07e00a493c5a13f72261b5e1a5d13e378766e18acf062ae69c4db29
- negative/wrong_argument_type.nl = ea1db69fd3bf4a9e45005e27dc410f8041f0d97d14b146669a86b6afa77b64b1
- negative/third_argument.nl = 214c726e138c28a12a356314ec9084d4c57de49586d49c9108f9f1450629de11
- negative/wrong_unit_return.nl = 22db198cca96bd6748063545e3a45ec05c6f5ba768be34f11b4d5ea0c2060c17
- negative/unknown_original_field.nl = 55f7e36cba04fb4a9602f9b165ff2d1027640728b1574b78ff1af153b2116f2f
- original R875 verify.py numerical/oracle SHA256 = 9231af561974594d9188a41893b1dd283e56d55c609a5bd1930ab471450ec254 (original R875 oracle; no retuning; original R875 verify.py is not edited).

Frozen predictions:
A (TRUE/normal THEN, ELSE terminal return) original Gauge (31,211)->(72,211); receiver (147,8)->(147,72); original old Copy 31, incoming 72, original sibling unchanged; public child postcall ValueFact remains conservatively Unknown.
B (FALSE/return THEN, ELSE normal) original Axis (57,193)->(94,193); receiver (153,22)->(94,22); original old Copy 57, incoming 94, sibling unchanged; public child postcall Unknown.
Both are unit-return direct callees with EXACTLY TWO formals; original caller loan closes and later writes distinct receiver. Public NLCopyUnitView and specialized NLIfReturnView, source-issued event/Change, stable original root/field/sibling/incarnation; negative foreign owner/loan/actual/field/event and four malformed source inputs must fail closed. Never infer known postcall scalar from physical runtime.

Original two frozen sources select only NORMAL arms at runtime. Actual terminal RETURN branch execution: NOT TESTED unless a separately source-valid, frozen R881 complementary program is added before CI; no such program is asserted in this freeze. Proof that RETURN is present/validated is separate from runtime RETURN coverage. Real injected OOM: NOT TESTED unless an actual reached failure index is logged. P880 physical/sanitizer forecasts are diagnostic, NOT R oracle.

New calibrated harness must:
1. copy old R875 owned_probe.c only into tests/r881/ and apply exact pinned Clang23.1.3 formatting suggestion without changing any original R875 file/predicate; log original blob 644cea25910a3859249142f34046d104fb23ff51 and suggestion artifact 12559655119 before first PR/FULL5;
2. check original source sha256 in verifier; source-owned public probe and CHECK both; emit ordinary raw C17 and print its byte length/SHA256 and full base64 to Action logs BEFORE any observer transform; exact named-output byte equality;
3. produce separately sha256-labeled passive live-original-in-scope C17 without additional guest rights or state; strict GCC/Clang -std=c17 -Wall -Wextra -Werror -pedantic-errors -O0/-O2 for both plain/passive; report physical expectations without turning unit exit0 into payload proof;
4. reject original four negatives with untouched sentinel named output, no false fault/OOM PASS. Host CI ASan/UBSan and guest sanitizer compilation/runs only in actually reached sanitizer lanes, distinguish both.
5. historical R875 old verifier expected values are unchanged. Any incompatibility must STOP, not soften oracle.

CI gate: new R-only tests/r881/** + minimum additive CMake; exactly ONE new CTest `r881_calibrated_replay.integration`. P878 baseline FULL5 five jobs each 462 tests; expected 463/463 each. P878 PARTIAL 461 tests; expected 462/462. One natural FULL5 plus PARTIAL, attempt1 only, ZERO reruns/extra dispatch/after-PR pushes. DRAFT R PR against exact branch `p878-unit-loan-binding` (base SHA must match), not main/old R876. Stop at first drift, format nonmechanical change, checker/proof/original oracle/native/negative/sanitizer failure or CI budget exhaustion. Technical evidence != author+safe-caller UB audit-locality Product PASS (unmeasured); #624 PAUSED / H1 #268 HOLD.
