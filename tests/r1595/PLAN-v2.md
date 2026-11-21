# R1595 C1306 V2 corrected-apparatus / raw-log TEXT PLAN

Status: PREP ONLY; DRAFT TEXT. NOT PARSED, NOT COMPILED, NOT FORMATTED, NOT TESTED, NOT RUN.
Current target: NOT_RELEASED. Execution, Git, branch, PR, FAST/FULL and CI budgets: 0.
Same actor: /root/r1595_nested_actual_prep. New workers/descendants: 0.
Future execution mode: GitHub dedicated branch plus Actions ONLY.
A P-equivalent local execution environment is forbidden.

## 1. Release, authority and immutable originals

This V2 is limited to C's own-apparatus repair and selected data-only logging preparation:

- V2 release: https://github.com/wakairo/NewLang_Compiler/issues/1595#issuecomment-7058345950
- V2 ACTUAL RESUME, individually POST/GET matched: https://github.com/wakairo/NewLang_Compiler/issues/1595#issuecomment-7058357316
- Narrow public CLI read addendum: https://github.com/wakairo/NewLang_Compiler/issues/1595#issuecomment-7058503746
- Original FIRST seven artifacts: https://github.com/wakairo/NewLang_Compiler/issues/1595#issuecomment-7057942326
- Original PREP FINAL: https://github.com/wakairo/NewLang_Compiler/issues/1595#issuecomment-7057951152
- V1 apparatus six artifacts: https://github.com/wakairo/NewLang_Compiler/issues/1595#issuecomment-7058278615
- V1 PLAN three artifacts: https://github.com/wakairo/NewLang_Compiler/issues/1595#issuecomment-7058285790
- V1 PLAN FINAL: https://github.com/wakairo/NewLang_Compiler/issues/1595#issuecomment-7058296866

FIRST was individually GET again and its seven complete files were re-extracted. All seven exact UTF-8 byte counts, SHA-256 values and LF/final-LF properties matched the original record. The two V1 artifact comments were individually GET again; all nine files likewise matched their frozen byte counts/hashes and line endings. Neither original nor V1 comment/file text was edited. V2 changes later apparatus only; all five complete guest units and the entire original source-law oracle remain unchanged.

Fixed reading main: 3d2c0e05a206c905d5f5cfab12659d1b90b63456.
Own-Issue-supplied tree: 77b12ba519cc7dfde1e28bc369bd4b6fa4b369a1.
Neither is an execution target. No Git-object/tree/ref operation was performed.

Own Issue latest individual GET preceded ACTUAL RESUME POST/individual GET. Compass, Value 1.1/1.3, Process 7/7.6, CURRENT_SPEC and relevant Draft17.38 clauses were reconfirmed. The source quantities, type/zero-divisor judgments, Copy/let and strict nested-actual semantics remain those frozen in FIRST. Process repair guidance supplies no permission to execute in this zero-budget phase.

Authority links at the fixed main:

- docs/NewLang_Project_Compass.md
- docs/NewLang_Product_Value_Strategy.md, sections 1.1/1.3
- docs/NewLang_Project_Development_Process.md, sections 7/7.6
- docs/reference/CURRENT_SPEC.md, selecting NewLang_v0_spec_Draft17_38.md
- docs/reference/NewLang_v0_spec_Draft17_38.md, relevant 5.2/5.4/7.1/18.1/18.2/27.4 reconfirmations

Exact returned-text digests and server-returned blob identities are recorded in public-read-addendum-v2.tsv. These are text preservation checks, not compilation or semantic validation.

## 2. Exact exposure boundary

Previously released public interface remains the same eight headers: source.h, syntax.h, parser.h, semantic.h, checked.h, usize_division.h, usize_remainder.h and diagnostic.h under include/newlang/. Their repository public-include closure remains within those eight; no extra header was added. V2 reread the public semantic.h registration contract, plus the already permitted FULL/PARTIAL workflow metadata to place the logging snippets.

The original five permitted integration metadata files remain root CMakeLists.txt and r-preflight.yml, compiler-ci.yml, compiler-frontend-ci.yml, compiler-early-ci.yml. No referenced implementation, fixture, test, hook, helper script or run log was followed. Workflow test/hook names and matrix commands are integration metadata exposure, not readings of their bodies or results.

After individually reading C addendum 7058503746, exactly README.md lines 391-402 were retrieved using start_line=391/end_line=402 at the fixed main. No remainder of README or linked history/example was read. This range includes the CLI contract heading, version/help/options opening and --check contract; that small public-contract exposure is declared here.

README server blob: c0f2e0d7a91c55951a4fcc402acc7f42be70d12e.
Exactly returned ranged UTF-8 text: 698 bytes; SHA-256 aa4784f6e118f1e855c90aa636c8e27ce4876063400962b1df3604ec95444f02.
The ranged API text has no final LF. This digest is NOT a whole-README digest and was computed without adding a newline.
Public contract URL:
https://github.com/wakairo/NewLang_Compiler/blob/3d2c0e05a206c905d5f5cfab12659d1b90b63456/README.md#L391-L402

Exposure class: canonical-FIRST retained; later expressly released public-interface/integration/CLI-contract and own-apparatus text. No P/M/F/other-R originals, candidate implementation, aggregate C material, shared scratch, worktree or actor inventory was read. C's V2 observations concern only this actor's frozen V1 draft and the public registration contract. The original canonical navigation/history-metadata exposure remains declared in FIRST; no linked producer original was followed.

## 3. Two corrected own-apparatus false-success paths

These are static defects of the V1 proposed apparatus, not observed compiler failures.

### 3.1 Nonzero CLI status is no longer enough

V1 could classify any invalid-case nonzero CLI exit as source rejection. V2 preserves raw stdout/stderr and the actual child status before classification, recording launch failure, timeout, negative signal status, positive code, elapsed time, byte lengths and SHA-256. Negative signal status, timeout/launch failure, unexplained high exit and detected sanitizer/resource/internal/abnormal indicators stop collection. The marker detector can only cause STOP; its silence is not evidence of semantic correctness.

The subsequently released README contract removes the need to invent an exit convention: --check success is 0 with empty stdout/stderr, source I/O is 1, invalid options/argument counts are 2, and parser/semantic failures are 3 with retained diagnostics. Semantic --check success does not establish backend/native support.

The invalid-case acceptance gate now requires all of the following separately:

1. The public observer ran on the unchanged original source, parsed the original unit successfully and created the semantic context successfully.
2. Authentic original-unit registration reported SEMANTIC_ERROR, not OK, resource/internal error or some earlier refusal.
3. No observation_complete event exists for the invalid unit.
4. The separate CLI process ended normally with exactly status 3 after the child-health guards.
5. Its diagnostics are nonempty and exactly match the stderr bytes rendered by the public observer for that same original registration error; CLI stdout is empty.

Point 5 is R's deliberately conservative evidence-consistency gate. README does not promise byte-identical stderr formatting or an empty failure stdout. A difference therefore yields INCONCLUSIVE/STOP, not a claim that the source or CLI violates the language. No fixed diagnostic string, C-number, schema or candidate-specific spelling is used. A normal status 3 alone never proves the particular type/zero-divisor law, absence of resource failure or general compiler soundness. The source-law oracle, public registration observation, diagnostic bytes and CLI exit remain separate evidence.

Invalid code 0 is unexpected acceptance and stops. Codes 1/2 stop as I/O or invocation failure. Other codes are unclassified and stop. Missing/different diagnostics stop as INCONCLUSIVE. All exact child outputs/status are retained before this decision. Legal cases additionally require the documented zero-byte success streams after --check code 0; a lawful-source refusal remains a coverage/precision boundary, not a revised source-law judgment.

### 3.2 Invalid original registration OK stops independently

The public semantic.h contract says registration prepares signatures/owned bodies privately and commits only after every declaration passes definition checking. A true-invalid original unit reaching unit_register=OK is unexpected admission even if the subsequent synthetic main() adapter refuses.

V2 runs the public observer first for invalid units, records the original registration status, and checks unit_register=OK before treating the observer's later adapter result or absence of observation_complete as rejection. It records unexpected_original_admission and stops with TRUE_INVALID_UNIT_REGISTERED. Later entry-parse/check refusal cannot convert original admission into a successful negative test. An earlier abnormal-process/serialization stop also never counts as source rejection; the raw records remain evidence for review.

The original-unit SEMANTIC_ERROR requirement is explicit. A parse refusal, unavailable status, internal/resource failure or unknown boundary yields INCONCLUSIVE/STOP. No trusted seed, synthetic helper re-invocation, direct summary injection or source replacement was introduced.

### 3.3 Missing apparatus-stage records

observe.c now emits explicit status for apparatus_entry_parser_create, including the public status name/number, and for observer_allocate, including OK or OUT_OF_MEMORY and requested bytes. These describe existing host/apparatus paths only; no allocation failure is claimed to have occurred.

Runner stage names are now stable labels: public-observer, cli-check, emit-c17, pristine-c17-compile, pristine-native. Invalid units use public observer then CLI; lawful units use CLI then public observer, pristine emission, strict host compilation and pristine native execution. The finite future maximum remains 19 child processes for the five original cases, with 30 seconds per child; none is currently released or run.

## 4. Retained public evidence limits

C accepts COMPLETE_GENERIC_CALL_RESULT_AUTH = NOT_OBSERVABLE within the permitted eight headers. V2 adds no interface, issuer/world authority or synthetic proof to fill that gap. V1's published-edge/context/source qualification and authentic registered-original traversal remain unchanged.

Available immutable getter observations may expose original bodies, nested child relationships, scalar knownness and retained division/remainder validation. An equal number, matching ID, copied source bytes or valid arithmetic event is not the complete caller/formal/result/world custody certificate. Unknown is not Known(0). Hypothetical IF-arm traversal is not an execution trace.

Every original main returns unit. Pristine native exit 0 can check successful execution of that complete emitted unit, but cannot by itself observe its internal scalar quantities or evaluation order. Those remain quantity_runtime=NOT_OBSERVED and order_runtime=NOT_OBSERVED. No guest assertion, return-value rewrite, generated-C instrumentation, hidden symbol or fresh helper call is used to manufacture visibility.

The final runner collection label is BOUNDED_COLLECTION_COMPLETE_WITH_EXPLICIT_LIMITS, not Product PASS. It is reachable only if all required case gates actually succeed under a later release. No present observation supports that label.

## 5. Selected raw-log route: saved bytes to the real job log

C selected a concrete future route: tests/r1595/publish_logs.py, invoked by one post-test always step in each FULL/PARTIAL job. The script is provided as unexecuted draft text.

It reads a fixed allowlist of 86 possible R-owned files under exactly one selected root:

- pipeline-report.json;
- for each c02/c03/c01/p01/p02, case-report.json and public-records.json;
- for each of the five fixed stage labels, its .status.json, .stdout.bin and .stderr.bin.

It neither enumerates arbitrary directories nor opens paths supplied by saved JSON. Source/oracle files, release configuration, emitted C, native executables, other test outputs and unrelated workspace data are not opened. No subprocess, compiler, test, eval/import of saved data, network, credential read or source mutation is performed by the publisher.

The root is anchored to GITHUB_WORKSPACE. Directory components and files are opened without following symlinks; regular single-link files, stable descriptor metadata, exact lengths and bounded reads are required. The draft bounds are 4 MiB per file and 16 MiB total. Missing, unsafe, changing or oversized data is NOT_AVAILABLE; it is never synthesized as an empty stream. A genuinely existing zero-byte stream is recorded with zero length and the empty-byte digest.

Each present file is emitted as labelled R1595_LOG_V2 JSON metadata and 4096-byte base64 chunks, with full-file and chunk SHA-256. Records have a bundle identifier and monotonic sequence; a final record carries the canonical manifest digest and counts. Sidecar checks compare recorded case/stage identity, absolute stream path, byte length and hash against the fixed-path files. They do not reinterpret return codes or change verdicts.

Only named GitHub metadata variables are read: repository, run ID, run attempt, GITHUB_SHA, refs, job and workflow, plus the explicit lane. They are labelled available environment metadata. They are not an independent proof of the checked-out compiler target or R HEAD. No secret or environment dump is read.

The script returns 0 after completing its data report even when records are unavailable or inconsistent. That means only that the reporting operation completed. It does not accept the evidence or replace the already completed CTest verdict. Unexpected inability to finish the publisher yields an incomplete log; the collector must treat that as missing evidence. No fake RED or child-verdict rewriting is introduced.

Unrun stages legitimately lack files. Consequently the total unavailable-file count alone cannot determine whether a run's required evidence is complete. The collector must reconcile the actual pipeline child list, case progression and status records; missing required child bytes or unknown execution is a STOP.

## 6. Two minimal workflow snippets, not workflow edits

The exact unexecuted snippets are workflow-full-step-v2.yml and workflow-partial-step-v2.yml under tests/r1595/. They propose only a new step with if: always(), shell bash, an explicit lane and the Python data publisher.

| Workflow metadata at fixed main | Exact insertion point | Data root | Lane |
| --- | --- | --- | --- |
| .github/workflows/compiler-ci.yml | Immediately after Configure, build, and test; before the existing cache-save step | build/r1595-records | FULL plus the existing matrix.name |
| .github/workflows/compiler-frontend-ci.yml | Immediately after Execute every registered frontend/C17 CTest (NO LLVM; PARTIAL); before the existing CLI-path verification step | build-frontend/r1595-records | PARTIAL:frontend-c17 |

Existing ctest --test-dir ... --parallel ... --output-on-failure commands, their entire registered suites, captured ctest_status and exit statements are unchanged. The always step can publish after a failed test step; it cannot turn the existing failed step into success. No continue-on-error, required-check change, retry, dispatch, privilege, secret, SDK/toolchain or hook change is proposed. R-FAST remains unchanged and runs zero tests; no publisher is added there. The early workflow is unchanged.

These files are snippets for review only. No .github file, CMake file, branch, ref or PR was written. A future C immutable-target execution release must explicitly authorize the reviewed logging-only workflow delta in the R write scope and finite budgets before it can be applied.

## 7. Concrete future retrieval and acceptance procedure

No job log was fetched in this phase. Tool metadata confirms an available GitHub fetch_workflow_job_logs operation taking repo_full_name and job_id; that operation was not invoked.

A later collector must:

1. Identify the released R branch/head, immutable compiler target, exact Actions run, run attempt, workflow and matrix/partial job using separately authorized GitHub metadata. Record the actual job ID; do not infer it from a lane string.
2. Retrieve that exact job's complete log through GitHub fetch_workflow_job_logs. Local files, the publisher's exit status, workflow success and previously printed text are not a substitute for this retrieval.
3. Select the named publisher step and exactly one coherent begin/end bundle for the expected root/lane/run/attempt/SHA. Missing/duplicate/ambiguous bundles or inconsistent metadata stop collection. GITHUB_SHA on a pull-request run may denote its tested merge revision; explicitly reconcile that revision with the released R head/target rather than silently equating them.
4. Reconstruct each present file from the expected contiguous chunk indices/offsets; verify chunk length/hash, total length/hash, monotonic record sequence and final canonical manifest digest. Reject truncation, duplicate/missing chunks, unsafe filenames or mismatching counters. Preserve the retrieved raw log too.
5. Parse the recovered pipeline/status reports as data. Compare their run/attempt/SHA metadata with the bound job, require every recorded child to have its exact status and stdout/stderr, and compare each sidecar with the corresponding pipeline child record. Reconcile case progression and stop reason; an absent unrun stage is not a passed stage.
6. Report actual child codes, signals, diagnostic/quantity observations and limits separately from data-transport completeness. A MATCH from the publisher is an assertion to recheck from retrieved bytes, not a semantic certificate.

Missing required raw data is NOT_AVAILABLE. Partial or inconsistent retrieval is INCONCLUSIVE/STOP. This plan does not promise artifact downloads: current workflows do not upload these R report files, and the selected route uses the real job log. Emitted C/native files themselves are not in this publisher's allowlist; their complete contents therefore remain unavailable through this route. Saved stage status/streams and recorded hashes support only their stated evidence boundaries.

## 8. Disabled release template and assembled future inputs

release.json remains execution_enabled=false, target_sha=null, branch=null, cases=[], max_children=0 and fast_budget=full_budget=0. It records the selected actions-job-log-v2 route and PREP/addendum URLs for provenance only. logging_workflow_delta_release and successful_raw_log_retrieval_release remain null. Runner authorization requires both separate future release fields in addition to the existing Actions-only, branch, target and finite-budget checks. Route selection is not execution permission.

Changed intended paths, with V1 snapshots retained in their original Issue comments:

- tests/r1595/observe.c
- tests/r1595/run.py
- tests/r1595/release.json

New V2 draft paths:

- tests/r1595/publish_logs.py
- tests/r1595/workflow-full-step-v2.yml
- tests/r1595/workflow-partial-step-v2.yml
- tests/r1595/PLAN-v2.md
- tests/r1595/public-read-addendum-v2.tsv
- tests/r1595/draft-manifest-v2.txt

The V1 CMakeLists.txt, seven-byte entry_expression.nl, root-cmake-include.txt and public-read-manifest.tsv are reused unchanged at their frozen hashes. V1 PLAN/manifest remain historical records; this V2 supersedes only the explicitly described later-apparatus decisions. All seven FIRST artifacts remain separately immutable.

## 9. Honest unresolved limits and STOP boundary

No public source-law question is reopened. The additional README range resolves the missing public CLI exit convention. Actual diagnostic-byte agreement between CLI and public observer remains NOT_RUN; if it fails, the stricter consistency gate stops without guessing a new format or weakening the original law. C already accepted the generic call/result NOT_OBSERVABLE limitation. The raw-log route is concretely selected but its success, capacity and exact-job retrieval are unmeasured until a separately released run.

All C/Python/YAML/CMake material remains draft text. No syntax checker, formatter, compiler, test double, mock run, local harness, native process, SDK/build, Lean or CI was used to validate it. The only calculations performed were own text extraction/comparison and UTF-8 byte/SHA-256 checks; the hash helper matched three standard text vectors. Publication integrity does not imply apparatus correctness.

If future execution is released, an unexpected admission/known-value disagreement, unsupported lawful boundary, unexpected exit, resource/internal/sanitizer event, inconsistent evidence, exhausted budget or unavailable required raw log requires the applicable STOP C. A source correlation or scalar view that the allowed public representation does not expose remains NOT_OBSERVED; it cannot support a claimed quantity observation. No silent widening, producer borrowing, wrapper instrumentation or local fallback is authorized.

The two technical faults concern the reliability of this independent experiment's proposed apparatus. They do not demonstrate or repair a compiler defect, and they do not establish author plus ordinary-safe-caller UB-prevention/audit-locality/maintenance improvement over well-factored C. That product-value question remains UNMEASURED. Product GO remains PAUSED; H1 remains HOLD.

After V2 complete-text/byte/SHA POST, individual GET and re-extraction equality, publish V2 PLAN FINAL, individually GET it and STOP C. Target release, workflow edits and execution require a separate later instruction; this checkpoint grants none.
