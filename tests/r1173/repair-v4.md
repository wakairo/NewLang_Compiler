# R1173 machinery repair V4

Prior R HEAD: 1ed41067d17adab7513be017c5e549203b79f532.
Independent FIRST: Issue1173 comment6650534060; initial FULL41973467513 and PARTIAL41973467722, attempt1.
No original source/oracle/expected values, fixed compiler, public probe, existing P test/assertion/timeout/inclusion, or shared workflow is changed.

## Actual failures and bounded changes

1. Passive recognizer V1 rejected the actual unique guard because its RHS begins with a size_t cast and UINTMAX_C. V2 recognizes the strict less-than operator independently of RHS spelling, preserving unique-guard and real-carry origin checks. It follows actual scalar copies from the indirect replace-old capture through the loan result into the source old binding, and reads both actual post-Break guest field-load variables. These observations implement the original frozen old/final-value comparisons; no expected values enter instrumentation. Insertions remain only read-only fprintf calls, without guest stores, branches, helper state, or authority.
2. The R registration adapter moved old P test scripts into the canonical fixed-P checkout, but left exact repository-root arguments pointing at the outer R checkout. It now relocates that exact argument too. Fixed tests/p1098/historical_forward.py was inspected only after the independent FIRST record; its identity assertion and all file bytes are unchanged.

3. FIRST custody review found a latent R control-flow defect: native errors were recorded but the loops could continue into later compiler/optimization/observer/sanitizer stages for that source. No such anomaly or unsafe continuation occurred in FIRST. V4 predeclares native rows as NOT_RUN, preserves already completed rows, and raises a source-scoped stop on compile/runtime/stdout/stderr or value/identity anomaly. Later native/observer/sanitizer work for that source does not execute; independent unaffected sources may continue. This enforces existing authority without changing the oracle or safety semantics.

Initial PARTIAL also timed out in the unchanged p990_new_native.integration aggregate at 90.02 seconds, after logged native/transitive-suite progress. Its exact cause is not established. That RED is preserved separately; no timeout or test selection is adjusted. C permits the one already-authorized natural repair cohort while preserving it. Recurrence or semantic/UB/assertion findings require affected STOP; no further cohort is authorized.

V1/FIRST logs and raw packets, all prior execution-freeze manifests and repair records are retained. The source cohort/oracle remains V1. Independence is pre-frozen source/oracle with post-freeze old-P formatter exposure and post-FIRST old-P machinery inspection, never an unqualified whole-period BLIND claim.

C pure-text audit found no material static blocker in the six actual generated-C texts and the two R machinery candidates. This is not an execution PASS.

The repair push is the single remaining normal FULL5/PARTIAL set (total maximum12 actual jobs); its natural push also consumes FAST4/6. Manual dispatch/rerun, exclusions, production changes, rebase/amend/force, Ready and merge remain zero. DRAFT / NEVERMERGE.
