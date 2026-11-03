# R1129 C221 immutable preparation / future observation plan v1

## Authority and scope

Repository: wakairo/NewLang_Compiler.
Canonical main: 73ef10a97f2fab7b70d094b2114359e961acefb5.
CURRENT_SPEC selects NewLang_v0_spec_Draft17_34.md.
No execution candidate HEAD has been released to this worker.

Canonical reads began with docs/NewLang_Project_Compass.md, then:
- docs/NewLang_Product_Value_Strategy.md (especially 1.1, 1.3, 2-6 and slice principles).
- docs/NewLang_Project_Development_Process.md (7 in full).
- docs/reference/CURRENT_SPEC.md.
- docs/reference/NewLang_v0_spec_Draft17_34.md: goals/non-goals 1-2; types/Copy/Discardable; usize comparison and typed literals 5.2/6; dependency/current state/Change 13.5a-b; loan result 13.8; construction 16.1-2; selected two-u8 direct fields 17.1; replace 17.4; ordinary functions/caller post-state 18; lexical sequencing and target boundaries 19.1; loop/header/Continue/BREAK 27.1-11; semantic-unit registration 28.1-2.

The normative basis for unchanged T is the absence of a T write on both valid Continue paths plus the required sound inductive header. The basis for old/result and updated memory is replace and ordinary known-call post-state, not a producer fixture. The equal-bits case deliberately changes only the semantic distinction that a source write occurred.

## Contact / independence record

Read: this Issue, pinned main branch metadata, and the canonical documents above. The branch response also contained its own commit message/metadata. Canonical documents contain embedded historical P/M/R mentions, links and source sketches; no such linked implementation, fixture, report, rationale or oracle was fetched. No P1123 implementation/source/fixture/oracle/report/checkpoint, M/F/B rationale or concrete-example material outside canonical, or old R sources/oracles/raw was opened. No prior R corpus was reused. The names, constants, control shape and oracles here were independently selected from canonical semantics. No agent was spawned. Coordination messages supplied only phase/progress information, without candidate implementation context.

START: https://github.com/wakairo/NewLang_Compiler/issues/1129#issuecomment-6614005181

All six sources are complete bytes in this Issue. The negative variants are deliberate local mutations of this worker's own baseline, not copies of P examples. They are separate semantic units; the three same-named functions are not cross-file declarations.

## Present phase ledger

Canonical/read-only GitHub requests: permitted.
Pure JavaScript string/manifest/SHA-256 preparation: permitted; not execution of NewLang source.
Issue START, full freeze and final return comments: permitted.
Local shell/executor, repository checkout, compiler/native/build/test, Actions/CI, branch/ref creation, blob/tree/commit publication, push and PR: ZERO.
No source-check result, issued proof, C translation unit, native trace, formatter result or sanitizer result exists for this freeze.

## Future release gate and mechanical registration

Stop after this freeze. C must separately pin the published candidate's full immutable HEAD, actual execution method, dedicated branch and finite FAST/natural-CI budgets. This plan grants none of those actions. Keep GitHub dedicated branch + Actions; P-equivalent execution environment is forbidden.

After that release, preserve these v1 bytes and oracle. Verify the released candidate and canonical. Read only the necessary public CLI/registration/owned API contracts for the mechanical R driver. Freeze the exact driver/observer sources, command argv, output paths, candidate SHA and registration before they are used. A missing public interface is a boundary to report, not permission to construct internal proof objects. The ordinary source main()->unit and all definitions must be registered from the frozen file; no source fragment substitute, host-injected T root or synthetic expected value.

A source syntax/spelling or driver repair must be forward-only: retain original bytes, SHA, predicted outcome and first diagnostics; publish new bytes/SHA/reason/prediction before rerunning. No compiler modification, oracle change, exclusion of a failing negative, or producer-informed calibration under a BLIND label.

## Ordered future evidence

1. Verify bytes and manifest before use. Parse/register each complete file in a fresh semantic unit using the public source path. Record registration failures separately. Normal source functions are collected under 18.1 and 28.2; field declaration order is signal index 0, checkpoint index 1, regardless of initializer source order.

2. Run semantic CHECK and capture exit status, diagnostic code/text/location and reached layer separately for every file. Legal baseline and equal-bits legality are independent of production precision/coverage. Four illegal variants must not obtain successful owned/C17/native evidence. An earlier unrelated parser/registration/precision refusal does not validate the intended negative boundary.

3. Inspect the authentic source-issued owned/public result through supported read-only public views. Require:
   - the sole loop L0, its usize parameter, typed Bool conditions, exact C0/C1/B0 sites and targets;
   - entry and BOTH Continue successor obligations, fresh iteration binding semantics and no inherited loan target across a control boundary;
   - original and bystander roots with exact nominal/declared-child identity, existing incarnation/current facts, scoped loans and actual helper-call lineage;
   - real old Copy/result identity and caller-visible postcurrent through B0 and the ordered caller writes;
   - every source Change, including equal bits when accepted. Never synthesize issuer evidence, infer a certificate from matching numbers, or require concrete numeric ghost IDs not specified by canonical.

4. Request C17 only from the authentic accepted result and supported backend. Keep emission refusal distinct from CHECK. For every negative or unsupported path, test named output no-clobber using a pre-existing sentinel file whose exact bytes are:
   R1129:<logical case filename>:KEEP\n
   where <logical case filename> is the complete manifest path and the final sequence denotes one LF byte. Snapshot its bytes/size/hash before the emission request and require exact preservation on rejection. If the named-output mode is unavailable, report no-clobber NOT REACHED; do not pretend a missing output proves preservation. Also preserve the absence of a fresh successful artifact on source/owned rejection.

5. Preserve and hash the unmodified emitted C. Compile strict C17 with the exact flags and compiler versions frozen in the future driver, keeping source checks and warning/sanitizer settings intact. Run only accepted finite sources. The ordinary main's two calls are the delayed and immediate paths in the oracle. Record observed guest execution separately from host/compiler success. A code path folded away by legitimate static proof may still be semantically correct but does not demonstrate the requested runtime Continue capability. Do not rewrite guest inputs or introduce a synthetic loop to obtain that credit.

6. If additional values/storage observations need instrumentation, retain a separate passive-observer derivative and its diff/hash. Resolve actual source-origin storage/bindings mechanically from the emitted artifact and public provenance. Observe before Continue, after actual BREAK update, after loop receiving and after each caller write. The observer may read existing guest values/addresses and maintain its own read-only-trace bookkeeping; it may not assign expected values into guest state, repair stores, replay transitions, add guest roots or replace the emitted code's behavior. Its expected data come solely from oracle.json.

   Evidence A is successful compilation/execution of unmodified generated C, with only its actual outputs/exit. Evidence B is the explicitly instrumented trace and address observations. Never attribute B's observations to A or treat instrumentation-induced address materialization as proof of uninstrumented physical placement. If actual original storage mapping is unavailable, mark native identity NOT OBSERVABLE and return that boundary. Matching addresses alone do not prove nominal/incarnation/current-fact validity; those also need authentic owned evidence. Across two nonoverlapping probe calls, stack-address reuse is allowed.

7. Proposed guest runtime bound is 2 seconds per process with a hard harness timeout, not an unbounded loop run. Expected baseline main has four body entries in total, two Continues and two BREAKs. Timeout, extra iterations, wrong values, a stale/current/target mismatch, missing edge or unsafe acceptance is recorded and returned to C. No extra fuzzing, infinite-source execution or budget borrowing is implied.

8. Apply ASan/UBSan only where the released Actions job really instruments and executes code. Host compiler/harness instrumentation covers host behavior; it does not establish guest sanitizer coverage. For guest coverage, record flags/runtime and actual execution of the generated C binary separately. Sanitizer clean is finite dynamic evidence, not UB freedom or a proof of the header relation. Applicable natural FULL5 and NO-LLVM PARTIAL job results, attempts and exact SHAs must be recorded under C's later budget; skipped jobs and ctest -N do not count as test execution. No heap allocation/OOM experiment is introduced by these lexical two-u8 sources.

## Separate value-alignment concerns

This is a small counterexample-ready compiler-capability preparation. It does not establish a useful customer workload, a C-versus-alternative advantage, representation privacy, or a safe public library. Rust/GC/other practical alternatives remain fair choices under the Value Strategy. The relevant alternative here is a smaller complete source and focused independent negatives, not a new generic CFG/owner/proof mechanism or rerunning old R evidence.

Library-author plus ordinary-safe-caller UB audit/re-audit locality versus well-factored C is UNMEASURED. Legal field writers and all safe-reachable normal/error/teardown/reentrant/FFI states are not validated by these finite sources. No timing, maintenance-cost or no-hidden-precondition benefit is claimed. GO624 PAUSED / H1 HOLD remains unchanged.

## STOP / return

Stop now at SOURCE-FREEZE COMPLETE / EXECUTION WAITING.
Do not mark semantic FIRST, Product PASS, READY, merge or Product GO.
Future authority drift, scope/identity uncertainty, independence breach, unexpected semantic acceptance/refusal, owned/C/native/sanitizer discrepancy, or budget need goes to C without adapting the oracle to P. No all-H, general induction or general termination conclusion may be inferred from finite traces.
