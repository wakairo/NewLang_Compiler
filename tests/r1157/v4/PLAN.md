# R1157 V4 — C234 scope-calibrated continuation

Authority: C234 https://github.com/wakairo/NewLang_Compiler/issues/1157#issuecomment-6630549617. Same R, branch r-fast/r1157-runtime-if, Draft NEVERMERGE PR1159; no new R/PR. Fixed compiler af16bbaebcaada68ff45afdf236d1debfc90b565, fixed tree 74a1bdc728398eb85bb690a9eaff34194f316b50; canonical Draft17.34 unchanged. The actual RESUME is comment6630571137. V4 is formed after C disclosed the public supported loop guard, not a fresh blind cohort and not a mechanical relabeling of V3.

## Independent choice from the disclosed profile

C disclosed the finite inherited profile: actual loop guard phase<L, ADD on its true arm, SUB on its false arm with L>0 and step<=L. Equality-derived arithmetic is outside that adopted precision profile. No producer source cohort, values, oracle, emitted-C example, runner or result was supplied. C also reported public checker blob identities and its scope adjudication. R did not open producer PR body/diff or P/F reports to form V4.

The three ADD sources are exact V3 bytes. The three own SUB variants change exactly one source span: `if (grain == usize(0))` becomes `if (grain < usize(1))`. True arm still breaks grain; false arm still continues grain-usize(1). L=1 and step=1 satisfy the disclosed sufficient condition. No original quantity trace, T construction or operation, post-loop Copy/comparison, field selection, replacement value or old/current/sibling read changes.

Predicted actual guard operands and truths:

| Source | Left operands across loop headers | Right operand | Actual guard truths | Backedges | Loop/Copy result |
|---|---|---|---|---|---|
| add_zero | 9 | 9 | false | 0 | 9 |
| add_one | 12,13 | 13 | true,false | 1 | 13 |
| add_four | 3,4,5,6,7 | 7 | true,true,true,true,false | 4 | 7 |
| sub_zero | 0 | 1 | true | 0 | 0 |
| sub_one | 1,0 | 1 | false,true | 1 | 0 |
| sub_four | 4,3,2,1,0 | 1 | false,false,false,false,true | 4 | 0 |

These traces and truths are the same as V1/V3. For the SUB cases the source operator and right guard literal differ; V4 oracle makes them explicit rather than silently changing V1. All post-loop comparison operands, Bool results, before/displaced/after/still bytes, original root/child/sibling identities, one actual runtime root condition, one selected arm and store, zero unselected arm/store remain the original independent predictions. The original V1 expectation SHA256 is a0b8b83bfec3a873f8b82b1036f013517bfff8a35d698930cd039623f72955ad. V4 has a distinct expectation file and SHA recording scope provenance and actual new guard operands.

## Earlier versions and actual failures remain

V1/V2/V3 source and oracle/manifest bytes are retained without edits. The initial six raw capsules and exact original runner are in first/. V3's six raw capsules, receipts and exact executed runner/transport/CMake are in v3_results/. The V3 commit 8bd8823600d81b4f823faa7668781f52aec368f9 and raw logs stay authoritative for that version.

The original equality-SUB positive predictions remain failed predictions. V3 reached all seven expected rejection checks; read_mode_write rejected at earlier unsupported LOCAL-LOAN-PROFILE, scope escape at P8-EXIT-DEPENDENCY, join mismatch at P13-IF-RESULT, unknown field at FIELD-UNKNOWN-FIELD. V3's three ADD checks accepted and three SUB checks rejected with precision USIZE-SUBTRACTION-PRECISION. All V3 owned/C17/native were NOT_REACHED. V4 successes cannot overwrite those failures or reach records. C adjudicated the SUB rejection as an existing explicit precision/coverage boundary, not an observed compiler regression, soundness violation, or requirement to implement equality inference.

## V4 execution and observations

The actual source-issued public probe invokes V4 main() and maps symbols from that exact V4 source for the emitted C. Original V1 specimen/u8 owned-return/export coverage remains NOT_RUN; V4 does not retry the former equality SUB as if it were a supported current positive. All positive CHECKs must accept before this cohort proceeds; any fresh disagreement is affected STOP. The original seven negatives are byte-identical V3 copies with actual earliest rejection layers recorded.

After genuine owned/current validation, ordinary stdout C17 and named `--emit-c17-to` C must be byte-identical. Both unchanged raw C and a separate passive-observation C file are compiled/run under strict GCC O0/O2, Clang O0/O2, guest ASan and UBSan. The passive observer evaluates actual operands and Bool values, captures the source-issued original root and child addresses, selected arm/store and final reads, and never supplies expected guest values, truth, addresses or writes. V4 additionally asserts the observed actual loop guard right operand at every header. Expected native guest values and identity/count predicates are unchanged. Runtime return coverage is unit-main only; original u8 return/export coverage is not claimed.

The independently authored fixed-tree transport is retained, selecting the current V4 manifest. Exactly eleven inherited CTests use original fixed-af16 bytes/assertions/commands with actual R-built compiler/CC/SAN; all other registrations remain unchanged. The transport verifies the entire 5959-file original tree and every tracked R payload. Current root CMake is byte-identical to V3. No production/canonical/workflow/toolchain modification and no old test removal or assertion weakening.

## Budget and STOP

Original normal12 (initial6+V3repair6) remain counted. C234 permits one new natural FULL5+PARTIAL6 and at most one subsequent R-machinery-only repair6: total normal24 maximum. FAST remains total6 maximum, currently2 before V4; manual rerun/dispatch0. V4's next single forward push occurs only after every already-dispatched V3 job finishes. First reliable all-six green stops further push/CI and leaves unused repair budget unused. No old completed R/F/archive rerun. Mechanical faults may use the one remaining repair set only after freezing the correction and preserving first RED. Any new material semantic/owned/C17/identity/count/sanitizer, dangerous acceptance, authority/compiler drift, oracle substance change, independence/scope or budget finding is affected STOP to C.

All execution stays GitHub branch plus Actions; local work is text/data/hash/archive processing and own Python AST parsing only. Audit locality UNMEASURED; general equality/range/termination/heap, T-changing or general nested loops, private forgery, reached OOM and general soundness remain outside the finite observation. GO624 PAUSED; H1 HOLD; P1036–PR1050 STOP. No Ready, merge or Product GO.
