# R1608 C1308 public-interface TEXT PREP v2

## Authority, preservation and present STOP

This is a proposed apparatus and recovery plan, not executed code. V2 control:
https://github.com/wakairo/NewLang_Compiler/issues/1608#issuecomment-7060313809
ACTUAL TEXT RESUME was POSTed and individually GET/body-equality verified:
https://github.com/wakairo/NewLang_Compiler/issues/1608#issuecomment-7060322061

V1 control/resume remain immutable:
https://github.com/wakairo/NewLang_Compiler/issues/1608#issuecomment-7059799139
https://github.com/wakairo/NewLang_Compiler/issues/1608#issuecomment-7059957192
V1 PLAN FINAL and its full manifest/index remain immutable:
https://github.com/wakairo/NewLang_Compiler/issues/1608#issuecomment-7060234663
https://github.com/wakairo/NewLang_Compiler/issues/1608#issuecomment-7060225460
V1 manifest SHA256:
8f5a9261d4a1affd6daf91cc93bb7a7ab54a32b9457981ee2e4dc1811b583364

V2 changes only the own CTest standard runtime environment, terminal publisher
deadline, exceptional driver STOP/finalization state, and associated PLAN
wording. The 21-command accounting below is clarified without adding work.
C's auxiliary static apparatus findings are exposed; they are not producer
outcomes, new blind evidence, observed compiler failures or transport trials.
All 17 other proposed files retain their exact V1 bytes and immutable locators.

The immutable FIRST is:
https://github.com/wakairo/NewLang_Compiler/issues/1608#issuecomment-7059734025
Its source-law anchor remains 6287fcb4cb94df3da1c67f57d7d717b29c25ecf9,
CURRENT_SPEC -> Draft17.38, with FIRST manifest SHA256:
91f79b36bc923673a2291a23195b285b19e6857950629d36d0120b771d836032

All six original source byte strings, their complete oracle and the original
SHA256SUMS are reproduced unchanged under tests/r1608/first/. Relocation of a
file path does not change its bytes or the semantic unit. The driver records
the mapping to each immutable raw input copy. No source is deleted or repaired.

The ONLY interface-reading base is
088ccdef7d18620f1d5dd698e46c1aae0613bf52, verified tree
d6faf52a55e1242df7b4e1bbbd8218103665a216.
Its Compass/Value/Process/CURRENT_SPEC/Draft blobs equal the FIRST canonical
blobs. READ_LEDGER.json records every allowed path/blob/range and incidental
metadata. All local public-header includes remain inside the eight allowed
headers. No implementation/test/hook body, other-track original, aggregate C,
shared scratch, actor inventory or descendant was read.

Future exact execution target, code writes, branch/ref/PR, raw transport
execution and finite FAST/FULL budgets are NOT_RELEASED. Current compiler,
native, Lean, build, test, syntax, formatter, mock, apparatus, SDK, Git and CI
execution/mutation consumption is ZERO. Text authoring, extraction and hash
integrity checks alone have occurred. None of the proposed C, Python, CMake,
YAML or transport has been parsed, compiled, formatted, mocked or run.

The mode remains GitHub dedicated branch plus Actions ONLY. A P-equivalent
local execution environment is prohibited. The proposed driver additionally
fails closed outside GITHUB_ACTIONS or without a real C-released 40-hex target.
The target string alone is not proof of the tested checkout: remote commit,
run/attempt/job and checkout bindings below remain mandatory.

## Files and the only proposed integration changes

All new files are under tests/r1608/. observe.c uses production public headers,
normal source ingestion/parse/register/check, then read-only getters and
validators. It never seeds a value, changes layout, registers a fake signature,
binds a fixture result, edits a checked object or uses a private setter.

entry_adapter.nl is exactly "main()" followed by LF. It is a separate minimal
apparatus expression and is never substituted for the original unit. No helper
call, guest output, assertion, ABI or wrapper source is added.

Exactly one root CMake line is proposed: include(tests/r1608/CMakeLists.txt).
Append it at the end of root CMakeLists.txt, after the existing global test
property loops, so the own 600-second limit is not overwritten by a default.
The own include is globally guarded and conditional on BUILD_TESTING.
Because the earlier root test capture cannot set properties on this later
test, the own set_tests_properties explicitly carries the existing standard
ENVIRONMENT string:
PYTHONDONTWRITEBYTECODE=1;PYTHONHASHSEED=0;LC_ALL=C;ASAN_OPTIONS=detect_leaks=1:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
The own TIMEOUT remains 600. V2 reread only fixed088 root CMakeLists.txt
2304-2306 for this setting, blob 0a23e38d8790de9ed781863c1ad49aadfaca6455;
the V1 READ_LEDGER remains immutable. No root-wide property is weakened.
Existing newlang_semantic/newlang_parser libraries supply public dependencies;
newlang_options and the existing compiler/sanitizer settings propagate normally.
Existing Python3 discovery is reused. No other R apparatus or test is linked.

The own ALL target depends on r1608_observer and ordinary newlangc. Its separate
registration path invokes cmake -P registration.cmake, which invokes anchored
ctest -N -R ^r1608_arm_quantity[.]integration$. It demands exactly one
registration and executes zero tests. Those CMake/CTest metadata invocations
are outside the per-own-CTest 21 direct driver command limit; neither they nor
host compiler descendants are silently counted as direct driver commands.
This is build/registration evidence only, including in R-FAST. The EARLY
workflow's existing fixed selection remains unchanged and does not execute
this new semantic/native test. No R-FAST or EARLY workflow edit is proposed.

full_publisher.yml.txt and partial_publisher.yml.txt are the ONLY two workflow
snippets. Append the former as one step in compiler-ci.yml's validate job and
the latter as one step in compiler-frontend-ci.yml's frontend-c17-partial job.
Both are always() data-only steps. continue-on-error applies only to the
publisher step so it cannot change an already recorded primary test exit.
Publisher outcome/bytes are still independently graded by recovery; this
setting cannot turn failed retrieval into success. Existing triggers, trust,
credentials, cache/SDK hashes, primary commands/results, toolchain/sanitizer
flags and test selection are untouched.

R1608_TARGET_SHA remains NOT_RELEASED in this text. A later C release must
bind it to the real execution target and record any apparatus-text revision
and hashes; it must not alter FIRST or invent an accepted compiler result.

## Actual public pipeline and the meaning of each observation

| Layer | Actual interface or command | Meaning and limit |
|---|---|---|
| Original bytes | nl_source_load; nl_source_view | Own immutable input object, source bytes and display name |
| Original parse | nl_parser_parse_function_unit | Original unit parse status, structured diagnostic/span |
| Original registration | nl_semantic_register_function_unit | Definition-time admission/refusal of the original unit |
| Entry apparatus | Separate main() expression; nl_parser_parse_expression_fragment; nl_semantic_check_expression | Independent adapter/check status; does not rewrite original registration |
| Call body | nl_checked_call_body; nl_checked_context; nl_checked_source | Genuine borrowed actual body/retained plan; own context and source |
| IF arms | nl_checked_if_arm(...,0/1) | Both genuine source-order hypothetical snapshots; not two runtime executions |
| Arithmetic | nl_checked_usize_division_view/valid and nl_checked_usize_remainder_view/valid | Isolated registered-source event/package/fact authenticity only |
| CLI check | newlangc --check ORIGINAL | Original unit plus ordinary main entry; documented zero streams on success |
| C17 | newlangc --emit-c17-to OUTPUT.c ORIGINAL | Ordinary named-output path; output bytes retained unchanged |
| Native | Existing host GCC/Clang, current CMake flags and sanitizer flags, then pristine executable | Process completion and streams only; no internal scalar/order observation |

The observer prints JSONL using bounded source hex and numeric/context-qualified
observations. Fragment/world/source serial numbers are observer-local labels
for equality of actual live borrowed pointers. They are not invented semantic
IDs, physical IDs, guest state or authority. Public semantic IDs remain
qualified by their genuine fragment/context; IF snapshot IDs are never compared
as public-context identities. Known-call bodies are checked against the
header's same-context contract. Source bytes and original declaration body
spans are recorded; equal bytes alone are not claimed as origin authenticity.

The root actual entry artifact retains its own registered main body. Any
nested helper body is obtained only through that actual checked call. Both
genuine IF arm fragments are inspected read-only. Reachable-node traversal uses
published IDs/edges and deduplicates actual borrowed objects; it does not assume
dense numeric node IDs or equate traversal order/count with runtime evaluation.

Each exposed arithmetic view is independently checked by its actual NULL
validator call and by validation of an unchanged copied view. The copies alone
are then used for two bounded authenticity questions: replace the divisor
number field with zero; replace division.origin with the genuine foreign entry
fragment, or remainder.source with the genuine separate adapter source.
The changed candidate is supplied only to the public read-only validator;
guest state, retained checked fragments and issuers are never changed. Rejection
supports that exact candidate check only. Missing foreign objects cause an
explicit unperformed check, never a fabricated origin.

Generic unknown scalar quantities are serialized as null. Arithmetic headers'
actual canonical numeric placeholders are retained together with their known
flags. A false known flag is never converted into the frozen mathematical
answer. When a genuine known numerator/result exists, the driver compares it
with FIRST (0/7 = 0; 53/7 = 7; 53%7 = 4). A target_max that cannot represent
the frozen literals is an applicability STOP, not a source rewrite.

There is no generic scalar whole-call/result/world theorem in the allowed
public contracts. Call-body/IF structure and isolated arithmetic validation
therefore remain limited observations. A real primitive event validator is
not a proof of the complete enclosing call, final caller result or runtime
world. The driver always records those theorem/internal runtime quantities
and order as NOT_OBSERVED. All original main functions return unit. Native
exit 0 cannot establish 0 or 4 inside the guest, selected-arm count/order or
original call/result correspondence. No guest output or hidden invocation
is added to close this gap.

## Original rejection, adapter boundary and STOP discipline

The original parse and registration statuses are emitted before any adapter
work. The observer and driver both implement an unconditional STOP for a
true-invalid original with NL_CHECK_OK registration. This STOP precedes
adapter creation/check and CLI rescue, even if a later record is absent or
the observer process is incomplete. Successful invalid registration cannot
be downgraded to an adapter refusal.

For original parse or semantic refusal, the observer retains the real
NLParseDiagnostic or NLFunctionUnitDiagnostic, actual source/span, input index
and renderer output. The driver separately captures --check on the identical
immutable raw source path. It establishes narrow original-input rejection
only when the public status is healthy, the source span is valid, the renderer
succeeds, CLI exits the documented 3 with empty stdout, and CLI stderr equals
the complete saved renderer bytes. It invents neither diagnostic spelling nor
a typed law-reason enum. Semantic error can support only original-input semantic
rejection; the intended nonzero-divisor/type-join reason is not inferred from
an untyped diagnostic. Parser unsupported/syntax and semantic unsupported/
precision are kept distinct and never promoted to the intended law check.

A lawful source receiving semantic-error/syntax-error status is preserved and
returned to C after the diagnostic/CLI correspondence attempt; it is not
reclassified as source-invalid. Healthy explicit unsupported/precision
boundaries can be recorded without forcing native execution or changing FIRST.
An admitted original plus an adapter boundary is an original-admitted adapter
boundary, followed by STOP C; arbitrary CLI stderr cannot repair that evidence.

Missing status, OOM/resource/internal status, incomplete capture, signal,
timeout, sanitizer/apparatus failure, source/issuer/context inconsistency,
invalid copied-candidate acceptance, unexpected known quantity or source-byte
change is STOP C. Nonzero C17 emission is saved as unclassified failure and
STOP, not inferred healthy backend refusal from arbitrary stderr. No target
or oracle is repaired by this plan.

Only lawful original registration plus adapter CHECK, matching successful CLI
--check and authentic available public arithmetic/structure observations may
reach C17/native. Public observation limits preserve original admission and
mark later stages NOT_RUN. Emitted C is not edited, instrumented or substituted.
Existing host flags, Debug/Release flags, linker flags and sanitizer flags are
retained; no sanitizer disable/recovery or warning relaxation is introduced.

The driver begins with a non-null INCOMPLETE terminal state. It clears that
state only after the intended six-case loop and every reached per-case decision
save complete. BaseException from the protected procedure, including an
unexpected malformed-public-record IndexError, records the original exception
type/message as STOP before suite finalization. The original offending raw
bytes and reached/unreached case states are retained; public checks are not
relaxed. An exception before a durable start or during final-record I/O can
leave missing/incomplete records. A nonzero or missing driver/CTest result,
or a failed finalization, remains incomplete even if an earlier record has
stop:null; it cannot establish a completed procedure or PASS.

The driver's exit 0 means that this finite observation procedure completed
without a STOP, not that all lawful sources were accepted or that full
compiler capability passed. Per-case unsupported/precision/observation limits
remain explicit. Missing native work cannot be promoted by a green CTest,
R-FAST inventory, publisher success or another case's output.

## Finite resource and artifact envelope

There is one serial source pass, in order P1, P2, C1, C2, C3, C4. No retry,
rerun, alternate input, secondary helper call or failing probe is added.
At most 21 direct driver command launch attempts per own CTest: each of six
gets at most observer + CLI check; only the three lawful sources can additionally
get emission + host compile + native. MAX_CHILDREN/children_attempted count
these direct Run.child attempts, including a recorded attempt that fails before
launch. They do not count every OS descendant of the host compiler and do not
include the separate ALL registration metadata path (cmake -P plus ctest -N).
A STOP prevents later direct commands and records untouched cases as NOT_RUN.
This accounting grants no Git/CI budget and changes no proposed resource cap.

| Bound | Proposed maximum |
|---|---:|
| Observer / CLI check / emission child wall time | 20 seconds each |
| Host C compiler child wall time | 45 seconds |
| Native child wall time | 10 seconds |
| Per-child CPU | 50 seconds |
| Kill/drain and wait grace | 2 seconds each |
| Cohort wall budget / CTest timeout | 540 / 600 seconds |
| Child stdout / stderr stored prefix | 65,536 bytes each |
| Per-child regular-file size limit | 32 MiB |
| Source/retained-source bytes | 4,096 |
| Original function declarations | 8 |
| Borrowed fragments / contexts / sources | 32 each |
| Fragment depth / reachable nodes per fragment / total nodes | 12 / 512 / 2,048 |
| Emitted C / native binary | 256 KiB / 32 MiB |
| Data publisher raw files / aggregate bytes / chunk bytes | 154 / 8 MiB / 2,048 |
| Publisher wall limit / decoded job log input | 30 seconds / 32 MiB |

No RLIMIT_AS is imposed because it would alter sanitizer virtual-address-space
behavior. Existing sanitizer settings are inherited. Pure unit native programs
are required to complete with zero status and no unexpected streams; any
unexpected output is retained and STOPped, not silently discarded.

A fresh fixed raw directory must be created exclusively. Existing raw/work
directories cause STOP and are never deleted or overwritten. Child start
records, argv, cwd, timeout and relevant non-secret environment observations
are committed before launch. stdout/stderr files are created exclusively and
streamed as bytes while the child runs. Start-without-result remains an
attempted incomplete child, never NOT_RUN. Failure, timeout and over-limit
prefixes are preserved; their result says incomplete, with status, counts,
digests and observed prefix lengths. Classification happens after saving the
child result and both streams. Successful children are saved exactly like
failing ones. No failure is induced to make success output visible.

Raw roots are exactly $GITHUB_WORKSPACE/build/r1608-raw-v1 for FULL and
$GITHUB_WORKSPACE/build-frontend/r1608-raw-v1 for PARTIAL. Native executables
remain in the separate r1608-work-v1 sibling, with recorded size/digest;
binary transport is explicitly excluded. Emitted C, all source input bytes,
all child records/streams, public renderer output and decisions are transported.

## Data-only publisher and exact remote recovery

publish.py performs no subprocess, exec/eval, source interpretation or test.
Its only file universe is the fixed 154-name allowlist from rawio.py at one of
the two fixed raw roots. It does not walk arbitrary directories, follow a
captured path/manifest, glob files, inspect other tracks or read the work root.

Every directory component from the workspace root and every file is opened
with O_NOFOLLOW/O_NONBLOCK (and O_DIRECTORY for directories). Each data file
must be regular, single-link and within its per-file cap. The open descriptor's
device/inode/mode/link count/size/mtime/ctime are compared before/after reading
and against a fresh non-following name stat. Exact byte count must equal the
stable size. A changed, special, linked or oversized file produces an error,
not a successful empty payload. Missing files are explicit missing entries;
they are not zero-byte success streams.

The publisher emits a metadata header, numbered base64 chunks and per-file
byte counts/SHA256, then a complete canonical transport manifest and final
manifest digest. Existing saved bytes are data only. Root/missing/error states
and incomplete prefixes remain visible. A transport end marker does not assert
measurement or capability success. A publisher crash, limit or missing footer
is incomplete even when preceding chunks were valid.

The first SIGALRM deadline or explicit elapsed-deadline check calls os._exit(124).
This terminal path raises no catchable file error and performs no Python
cleanup, manifest completion, final output or buffered interpreter flushing.
The 30-second alarm stays armed through normal output and finalization; it is
not consumed by an ordinary error handler and is not cancelled before a flush.
A deadline may leave a partial frame or no final manifest/footer. That prefix
is incomplete evidence and must not be repaired or labelled recovered.
Ordinary file errors still retain their original exception type/message.
No alarm, timeout, transport or flush experiment has been run.

A later release must permit this actual retrieval route; it has NOT been tried:

1. Record C's exact execution target, own R payload commit, dedicated branch,
   expected natural workflow IDs and authorized budgets before any Git/CI
   mutation. Verify the R commit's parent/allowed diff against the target.
   For a PR checkout, separately bind the actual tested synthetic merge
   checkout/event SHA and its parents; do not assume it equals run.head_sha.
   Unexpected main movement/merge content requires STOP C, not a new target.
2. For each actual natural FULL5 or PARTIAL run, individually GET
   /repos/wakairo/NewLang_Compiler/actions/runs/{run_id}/attempts/{attempt}.
   Record exact workflow path, event, head SHA, head branch, attempt, status
   and conclusion. There must be five actual FULL jobs (GCC, GCC Release,
   Clang, ASan, UBSan) and one separate NO-LLVM PARTIAL job at the authorized
   tested source. Skipped/queued/cancelled or other-attempt jobs are not PASS.
3. Individually paginate
   /repos/wakairo/NewLang_Compiler/actions/runs/{run_id}/attempts/{attempt}/jobs?per_page=100&page=N
   through every page. Bind exact job ID/name/run ID/head SHA/status/conclusion,
   including the primary and publisher steps. Do not use the connector's
   latest-attempt, first-page-only jobs wrapper as an attempt-complete inventory.
4. GET /repos/wakairo/NewLang_Compiler/actions/jobs/{job_id} for the selected
   job and call the discovered GitHub fetch_workflow_job_logs tool with
   repo_full_name="wakairo/NewLang_Compiler" and that exact job_id. It follows
   GitHub's temporary redirect and returns decoded job logs. Retain the full
   actual tool response/decoded log, not a displayed excerpt or another job's
   similar output. Record retrieval time, exact API/job URL, decoded-log byte
   count/digest and tool completion/truncation information.
5. Create retrieval metadata with repository, run_id, run_attempt, job_id,
   job_key, matrix, event_sha, mode, event_name and decoded_log_sha256, taken
   from the individually verified remote records and publisher job identity.
   Also retain workflow/head/checkout binding and primary/publisher statuses.
   The FULL publisher job_key is validate and matrix is the actual matrix
   name; PARTIAL is frontend-c17-partial and NO-LLVM-PARTIAL.
6. recover.py is proposed data-only reconstruction, not run now. It checks
   exact metadata binding, single header/footer, fixed allowlist, chunk order,
   count/size, strict base64, every file SHA256 and the final manifest digest.
   A fresh output directory prevents overwrites. Completed verified files and
   an interrupted decoded prefix are preserved separately; incomplete recovery
   exits nonzero and never becomes a complete file or successful measurement.
   The outer decoded-log digest describes received decoded text, not claimed
   byte identity of GitHub's original compressed/raw log. Embedded payload
   digests establish exact recovered artifact bytes.
7. Only after remote reconstruction succeeds, independently inspect suite and
   child start/result pairs, terminal driver state, actual driver/CTest exit and
   finalization completeness, all original statuses, CLI renderer correspondence,
   authentic public views and their limitations, emitted C/native flags/streams,
   source/SHA consistency and primary job conclusions. Publication/local
   storage or a recovery exit 0 alone is not evidence of semantic/native
   success. A successful payload transport can faithfully carry a failed or
   incomplete observation. Missing/truncated/oversized tool results or no
   footer require TRANSPORT_NOT_RECOVERED and STOP C; no unapproved upload,
   alternate environment, retry or producer evidence is substituted.

FAST remains a separate preparation budget. This TEXT PREP grants neither
FAST nor the future natural FULL5 plus PARTIAL set. Branch-creation FAST runs
and all attempts must be counted under the eventual explicit C release.

## Goal concern and terminal handoff

This supports bounded reusable observation of ordinary source/API composition.
It does not add source laws, widen the public API, invent proof/world machinery
or authorize a per-case trusted issuer. A lack of whole-call observation stays
a limit; it is not repaired by hidden author/caller proof obligations.

Value 1.3 capability work and Value 1.1 product evidence remain distinct.
Author plus ordinary-safe-caller UB-prevention audit/re-audit dependency closure
versus well-factored C is UNMEASURED. Product GO #624 PAUSED and H1 HOLD remain.
No predecessor gate is added.

Post all complete proposed texts, the read ledger and manifests; individually
GET every payload comment, re-extract exact bytes and verify SHA256; post and
individually GET PLAN V2 FINAL; STOP C. Syntax, formatting, build, API behavior,
runtime, workflow behavior and remote transport are all UNTESTED at this stage.
