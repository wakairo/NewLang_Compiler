# R1611 CONT1 exact-job raw retrieval plan

TEXT PREP ONLY. Future target, branch/ref/PR writes, CI attempts and log-read
budgets are NOT_RELEASED. There is no R1611 run/job/raw or transport result.
The original R1601 retrieval is historical evidence in its own FINAL; it is
not a successful test of this new selected continuation.

## 1. Bind the later release and every actual execution

Record the actual C execution GO, fixed compiler target SHA/tree/base branch,
canonical, complete R commit/tree/parent, allowed paths, branch and finite
FAST/FULL/PARTIAL/transport budgets. The planning artifacts preserve their
prep-time NOT_RELEASED state; the later release is a separate immutable control,
not a rewrite of FIRST or a claim that old target behavior transfers.

Verify all six FIRST sources and original manifest, the exact ordered
SELECTED.sha256/SELECTION.json, actual APPARATUS.sha256 and
PLAN_R1611_V1.sha256. R1601_PLAN_V2.sha256 is historical only.
Observe release-specific public interface/scaffold equality and collisions
without opening producer implementation/tests/oracles. No target is guessed.

Before any authorized push and PRE-DRAFT, inspect the entire dedicated branch's
push R-FAST history across all pages, including first-ref creation and every
failed/cancelled/queued/success run and attempt. Latest-head filtering is not a
branch-wide budget ledger. Match actual successful FAST SHA and branch to the
complete R tree. Require actual r1611_retained_actuals.integration once and
Total Tests:1 from anchored ctest -N plus unchanged whole-inventory count/
uniqueness. These enumerate zero tests and establish no source/public/native
result. Only then, under a separate execution release, PRE-DRAFT POSTGET and
one DRAFT NEVERMERGE evidence PR may trigger its one natural validation cycle.

After release, select the actual FULL five matrix jobs and separate NO-LLVM
PARTIAL job by exact run ID, workflow path, event, head branch/SHA, run_attempt,
job ID/name and completed conclusion. No pending/skipped/cancelled/missing job
is a successful validation. Record all actual test and publisher outcomes.

The documented attempt-specific inventory route is:
https://api.github.com/repos/wakairo/NewLang_Compiler/actions/runs/RUN_ID/attempts/ATTEMPT/jobs?per_page=100&page=PAGE
Start page1 and follow all pages. Individually bind the actual job record:
https://api.github.com/repos/wakairo/NewLang_Compiler/actions/jobs/JOB_ID

If a generic metadata endpoint is unavailable through the connector, report
that tooling limitation rather than a compiler result. Use a supported exact
run/jobs wrapper only when the later release permits it and fresh run metadata
plus complete returned job IDs establish the actual attempt, head, event,
workflow and job selection. In particular, a latest-attempt wrapper cannot
stand in for an older attempt or a truncated/paginated inventory.

PR checkout can be a synthetic merge. Record its exact commit/tree/parents
separately from R head and compiler target. Bind the allowed checkout relation
through release-scoped metadata, then compare actual git-head output and event
SHA. Different/unapproved trees are STOP, never silently retargeted.

## 2. Initial exact completed-job GET and bounded transport retry

For each released completed job use the actual connector call:
mcp__codex_apps__github_fetch_workflow_job_logs(
  repo_full_name="wakairo/NewLang_Compiler", job_id=EXACT_JOB_ID)

Propose one initial GET per six jobs, plus at most one documented transient
transport retry per job if later C releases that read budget. Record request
UTC, exact selected job/run/attempt URL, actual tool result/status and full
returned payload before interpretation. Preserve the first failed retrieval.
A transport retry is not permission for a CI rerun or failure induction.

Extract the payload from the actual observed tool structure, not a guessed
field. Preserve the returned UTF-8 decoded-log hash separately from original
child byte streams and download ZIP hashes. A prior stored/local/printed log,
publisher step name or intended upload is not an actual remote GET.

Missing/expired/auth-blocked/oversized/truncated/no-footer output is explicitly
NOT_RETRIEVED or INCOMPLETE_PREFIX. Never use a new experiment, workflow edit,
local compiler run, secret access or altered failure to repair transport.

## 3. Parse only authentic fixed framing as data

Strip only the established GitHub/tool per-line timestamp prefix. Require the
remaining line to begin exactly R1611v1 followed by its kind and JSON object.
Do not search for a substring inside shell-command echo or arbitrary output.
Base64 transport data is never executed as code.

Require exactly one BEGIN and one END with identical metadata. Between them:
- transport.manifest first: FILE, zero or more DATA, END_FILE;
- the exact sorted captured-runner file set, in the same record form;
- no STATUS failure, duplicate/missing/partial or unexplained trailing record.

FILE binds a safe fixed basename, origin, raw byte length, SHA256 and part count.
DATA binds that filename and contiguous zero-based index with strict ASCII
base64/canonical padding. Each full piece decodes to3072 bytes; the last exactly
matches the remaining extent. Empty files have zero parts. Verify exact
ceil(bytes/3072), all byte counts and SHA256. Keep arbitrary binary stdout/stderr
as bytes; do not UTF-8 round-trip, change CRLF, strip final LF or normalize.

Allow only context.json, summary.json, SHA256SUMS and numbered git-head,
c2-check/observe, or c3/c4-check/observe/emit/cc/native files with start.json,
result.json, stdout.bin or stderr.bin suffix. No p1/p2/c1 command file is in
this continuation's transport scope. Refuse slashes, dot-dot, unexpected
basenames, duplicate files, over55 captured files or over768KiB.

The new publisher retains the original stable-file read: O_NOFOLLOW |
O_NONBLOCK before fstat, bounded regular type and link count1 before reading,
post-read descriptor identity/type/nlink/size/mtime/ctime/actual bytes and final
named-file identity. FIFO/hardlink/path instability produces explicit data
failure, not source rejection. These barriers have not been exercised for
R1611. A derived transport inventory never replaces absent runner completion.

## 4. Verify all independent byte chains and scope

Recreate exact transport.manifest as:
SHA256 SPACE BYTE_LENGTH SPACE BASENAME LF
for sorted captured files, excluding the derived inventory. Require exact
file set, original inventory bytes, BEGIN/END inventory length/hash and totals.

When original SHA256SUMS exists, recreate its exact two-spaces/basename/LF
format and file set (all captured files except itself). Compare original bytes
and every hash. Absence may leave a useful recovered prefix, but not a claim
of complete capture or suite completion.

Bind context's allowlisted seven environment fields to external repo/run/
attempt/job/event/ref/merge metadata and BEGIN/END. Compare summary.checkout_sha
and actual git-head stdout to the authorized checkout. Check all six source
hashes against FIRST, selected source hashes/order against SELECTED and
SELECTION.json, and every actual core hash against APPARATUS.sha256.

Require context.apparatus_revision=R1611_CONT1. The unchanged C observer emits
observer_config revisionV2; that inherited protocol label does not identify the
new driver. Require actual selection metadata/manifest hashes, original
historical V2 manifest hash, actual apparatus-manifest hash and actual current
PLAN_R1611_V1.sha256 hash. Do not treat old baseline entries as current hashes.

BEGIN/END selected_completed must equal summary.completed; both selection
lists and completion_scope must describe exactly c2/c3/c4, not six original
cases. original_keys_excluded_from_execution must remain p1/p2/c1 and
original_six_case_completion must be NOT_CLAIMED. Verify that actual command
labels and any not_started_cases agree with the reached selected prefix.

For every child, check saved start/result agreement for index,label,argv,shell,
cwd,timeout and caps, exact both-stream filenames/bytes/SHA and EOF/truncation/
discard fields. Preserve launch errors, negative/null exits, timeout/cap/drain
status and sanitizer markers. Missing result or stream tails are incomplete
evidence even if summary/CTest is green. At most13 commands; no hidden retry.

Raw JSON object member order is not semantic field inequality: compare actual
key sets and structured values, preserving array order. This does not permit
any normalization of original bytes or renderer/CLI output. Preserve large
integer tokens exactly (for example decimal target_max may exceed JavaScript
Number precision); use arbitrary-precision integer analysis, not rounding.

## 5. Reconstruct classification from actual original-source evidence

Inspect all process/stream/public-phase health before grading. Public
OUT_OF_MEMORY/RESOURCE_LIMIT/INTERNAL_ERROR, malformed/missing completion,
signals, sanitizers, cap/truncation or unclassified exits are INCONCLUSIVE/STOP.

For selected true-invalid c2:
- CLI CHECK0 is unconditional STOP before observer/native.
- Any original registrationOK is independently unconditional STOP before
  adapter rescue, irrespective of a later refusal or own observer exit.
- Only original parseOK + original registerSEMANTIC_ERROR, healthy complete
  capture and exact original diagnostic/CLI binding can support bounded
  ORIGINAL_INVALID_SOURCE_REJECTED.
- This bounded grade is not underflow-specific algorithm/diagnostic proof.
  No unprovided typed reason enum is required. Parser-only, unsupported/
  precision, adapter-only, mismatched/unhealthy/unknown remains INCONCLUSIVE/
  STOP. Preserve the actual earlier boundary without changing source law.

For a provisional CLI3, verify original source name/bytes, role, input index,
phase, ERROR severity, span bytes, nullable category/code, optional primary
range and all notes, then a successful genuine public renderer receipt.
Its entire captured stderr must equal exactly one CLI stream with the other
empty. Do not concatenate channels, normalize whitespace/path, match a
substring or assume a producer-specific diagnostic spelling.

Lawful c3/c4 original refusals remain lawful limited observations. CLI accepted/
public refused is STOP with no native bypass. For authentic lawful success,
compare only uniquely observed Known binding values to frozen expectations.
Unknown is not0. Record missing body/result/origin relations explicitly and
keep actual arithmetic-validator/math observations distinct from generic
whole-call/result/world authority.

Check actual current-target maxima only where authentic arithmetic events were
reached, against original U>=56. An earlier prefix may have no such observation.
Record NOT_OBSERVED, never borrow R1601's measured maximum or assume host width.
The later release's target-profile binding and actual observed evidence are
separate; missing evidence does not create an extra probe or source rewrite.

Only lawful genuine CLI/public success may reach pristine C17/native. Preserve
emitted-C bytes/hash, exact compiler argv/status/streams and native completion.
Unit exit0 does not observe internal quantities or ordering. Documented backend
unsupported diagnostics and unspecified numeric refusal contracts stay separate.

## 6. Failure inventory, immutable archive and FINAL

For each actual job report:
- Exact run/job URL/ID/attempt/workflow/event/ref/head and actual checkout.
- Completed conclusion, primary CTest/publisher outcomes, failed-test inventory
  and every actual sanitizer/health divergence, without borrowing other tests'
  source/oracle/results as this experiment's evidence.
- Initial/retry GET time/status, returned log hash, actual raw count/bytes and
  original/derived manifest hashes.
- COMPLETE_RETRIEVED for the actually reached prefix, INCOMPLETE_PREFIX,
  NOT_RETRIEVED or INTEGRITY_MISMATCH, independently of test success.
- Each selected source's legality, CHECK/public/numeric/C17/native boundary,
  started-but-incomplete versus NOT_RUN, remaining observations and limits.

When preserving raw in own Issue packets, label the timestamp-stripped transport
envelope as derived archival framing, keep all original base64 bytes and
BEGIN/END, give ordered part/combined length and hashes, individually GET each
posted comment, re-extract exact fenced payloads, concatenate in order and
re-decode/rehash the full original raw/manifest chain. Do not call an incomplete
part an independently complete bundle.

At material mismatch or lost evidence send FIRST STOP promptly. Only already
running separately released lanes may finish and yield original raw; no
post-PR push, rerun/dispatch or effort to force green. A stopped c2 blocks
c3/c4 and must be reported that way. No implicit continuation is authorized.

Selected completed=true means only the three selected cases were recorded
with explicit limits, not support of every lawful source, original six-case
completion, general soundness or Product GO. Original R1601 remains unchanged.
FINAL POSTGET then STOP C. Future execution requires a separate C release;
this procedure is currently unexecuted and has produced no R1611 raw.
