# R1601 exact-job raw retrieval procedure V2

This is a future data-only procedure, not evidence that any run or retrieval
has occurred. Current execution target, all CI budgets and all job IDs are
NOT_RELEASED. No captured bytes from this prep are a compiler result.

## 1. Bind the released objects before interpreting logs

Record the eventual C release URL, exact compiler target SHA, R source/apparatus
commit SHA, branch, canonical, write scope and finite FAST/FULL budgets.
All six FIRST source SHA256 values and the PLAN_V2.sha256 manifest remain
explicit. The original V1 packet and its PLAN.sha256 are archived unchanged.
V2_DELTA.md explains the separately frozen changes and unexecuted reasoning
cases. A later correction needs its own immutable revision and preserves this
V2, V1, FIRST and every original/failed result.

For FAST, collect the entire dedicated branch's push-run history across pages,
not only runs at its latest head. Include branch-creation, queued, cancelled,
failed and successful runs and all attempts. Check the released budget before
any later push or PR. Only an actual completed/success run containing this R
code at its matching immutable SHA can satisfy preparation.

The own metadata ALL target must print the exact:
r1601_arithmetic_actuals.integration
and Total Tests: 1 from ctest -N with the anchored filter, in that build's log.
Also inspect the unchanged R-FAST whole-inventory uniqueness/count check.
Neither target existence nor a configure/build message about intent is enough.
Both CTest enumeration commands execute zero tests. No semantic/public/native
evidence is inferred from FAST.

## 2. Identify each actual validation job

After a separately released natural FULL/PARTIAL cycle, identify all five FULL
matrix jobs (GCC, GCC Release, Clang, ASan, UBSan) and the separate NO-LLVM
PARTIAL job. Skipped, absent, queued, cancelled or failed jobs are not a complete
FULL/PARTIAL success. Early coding GO and FAST never substitute.

Use the actual run ID and run_attempt, including the workflow identity, event,
head branch/head SHA and status/conclusion. For an attempt-specific inventory,
the documented GitHub GET route to use through the connector is:

https://api.github.com/repos/wakairo/NewLang_Compiler/actions/runs/RUN_ID/attempts/ATTEMPT/jobs?per_page=100&page=PAGE

Start PAGE at 1 and follow pagination until complete. A latest-attempt/first-
page-only wrapper is insufficient for an older attempt or a paginated job set.
Individually GET the selected job record before its logs:

https://api.github.com/repos/wakairo/NewLang_Compiler/actions/jobs/JOB_ID

Bind exact run ID, attempt, job ID, job name, conclusion, timestamps, checked
SHA, workflow path and the test step/publisher step. A matrix label alone is
not identity. Do not borrow another R's logs, branch or results.

Default pull-request checkout may use a synthetic merge SHA. Record it
separately from the R branch head and source compiler target. Validate the
approved commit/tree relationship using only the later release's read scope.
A different/unapproved tree cannot silently become the exact released target.

## 3. Individually retrieve the actual job log

For each selected completed job, call:
mcp__codex_apps__github_fetch_workflow_job_logs(
    repo_full_name="wakairo/NewLang_Compiler", job_id=EXACT_JOB_ID)

This tool returns decoded job-log text through GitHub's log-download redirect.
Keep the actual returned payload and the observed tool result status. Record
the GET time and exact selected job/run/attempt URL. Do not infer a successful
fetch from a prior stored log, a local file, an upload intention or step name.

The initial plan allows one GET for each of the six completed jobs and at most
one repeated GET per job for a documented transport/transient retrieval failure,
preserving the initial attempt. These are data reads, not reruns of experiments.
If another read scope/budget is required, defer to the later C release.

If the response is missing, oversized, truncated, auth-blocked, expired or
otherwise incomplete, record NOT_RETRIEVED. No CI retry, failure induction,
secret access, local compiler run or workflow mutation is authorized to repair
that transport problem.

## 4. Parse only the fixed ASCII transport records

Extract the log body from the actual returned tool structure rather than
assuming an undocumented connector field layout. Preserve its returned UTF-8
text hash as a transport receipt. Raw child streams are recovered from base64;
the decoded GitHub log text is not itself the original child byte stream.

Strip only the established GitHub/tool per-line timestamp prefix. Require the
remaining line to begin exactly "R1601v1 ". This is the unchanged transport
frame version; require context.json apparatus_revision V2 and the frozen V2
manifest/apparatus hashes independently. Do not find the frame substring inside
the printed shell command or arbitrary text. Publisher data is base64 encoded,
so source/stdout/stderr cannot inject an extra transport record.

Expected record sequence:

1. Exactly one BEGIN JSON object for the chosen publisher step.
2. A FILE, zero or more DATA, then END_FILE for transport.manifest.
3. The same sequence for every listed captured_runner_file.
4. Exactly one END whose metadata equals BEGIN.
5. No intervening STATUS error, duplicate framing or trailing partial record.

FILE declares name, raw byte length, SHA256, part count and origin.
DATA declares the same name, zero-based index and strict ASCII base64.
Each full chunk decodes to 3072 bytes; the last is no larger. Empty files have
zero parts. Require contiguous indexes, no duplicate/missing chunk, no duplicate
file and exact ceil(length/3072) part count. No data string is executed.

Names are fixed basenames: context.json, summary.json, SHA256SUMS and the
runner's numbered git-head/case-phase .start.json/.result.json/.stdout.bin/
.stderr.bin files. Reject paths, slashes, dot-dot, unexpected names, and a
payload count/size exceeding 104 files or 768 KiB. The V2 publisher uses
nonblocking/nofollow descriptor opens, requires regular single-link data before
reading, and checks descriptor identity/type/link count/size/mtime/ctime plus
byte count and final named-file identity after reading. FIFO, hardlink, path
replacement or instability is a publisher data failure, not a source rejection.
These are proposed barriers; this text prep has created or tried none of them.
transport.manifest is a separate publisher-derived inventory, not a file claimed
to have been fetched from the runner filesystem.

Reconstruct exact bytes in memory or a separately authorized evidence location.
Hash bytes directly; do not decode/re-encode arbitrary stdout/stderr as UTF-8,
change CRLF, strip final LF or otherwise normalize them. No compiler, native
binary, apparatus, SDK or executable payload is run by this reconstruction.

## 5. Verify the complete byte chain

For every FILE, require exact raw byte length and SHA256 after base64 decode.
Verify transport.manifest against the BEGIN/END length and SHA256; its lines
are exactly "SHA256 SPACE BYTE_LENGTH SPACE BASENAME LF" for the raw files,
excluding the derived inventory itself. Require the exact listed file set.

When runner SHA256SUMS is present, verify its original exact bytes and every
lowercase-SHA256/two-spaces/basename/LF entry. It excludes itself. Its listed
file set must equal all captured runner files except SHA256SUMS.

A missing runner manifest may leave a remotely recovered prefix, which is still
useful failure evidence. Label it INCOMPLETE_PREFIX, not complete successful
capture. The publisher-derived inventory does not replace the missing runner
completion record or manufacture a process exit/status.

Compare context.json's allowlisted environment to external job metadata and
BEGIN/END: repository, run ID, attempt, event SHA, job key and matrix label.
Compare summary.checkout_sha and the saved git-head stdout to the approved
actual checkout. Check exact original source hashes, original manifest hash,
and apparatus hashes against the frozen/authorized revision. Require the
actual context apparatus_revision V2 and plan_manifest_sha256 to equal the
separately frozen PLAN_V2.sha256 hash. A V1 transport frame prefix alone never
establishes which apparatus generated the recovered evidence.

For each child, verify the saved .start.json, .result.json, stdout and stderr
file hashes/byte counts and completeness flags before reading its classification.
The start argv, cwd, timeout and result argv must agree. Source CHECK, public
observer, emission, native compile and native execution are separate commands.
Preserve signals, timeout/cap/drain status, sanitizer markers and unclassified
exits even if a later command or job is green.

A truncated stream only proves its captured prefix. EOF=false, discarded bytes,
missing result.json, absent observer completion, or a missing planned case is
listed explicitly. summary.not_started_cases, per-layer NOT_RUN and
ATTEMPTING_OR_INCOMPLETE fields must agree with the actual reached prefix.
A layer is marked attempting before its child call, so a failed capture cannot
silently relabel a launched process NOT_RUN. The actual start/result/raw files
resolve launch versus incomplete execution. No numeric/code expectation
overrides raw.

## 6. Reconstruct the bounded classification from actual raw

The observer stderr is retained evidence, never discarded. Before grading,
inspect process normality, both stream-completeness records, signal/resource/
timeout/cap/launch/sanitizer flags and every typed public phase. No healthy
refusal grade survives an internal/resource/unclassified failure or missing raw.

Read original parse, original registration and synthetic-entry records
separately. For true-invalid c1/c2, CLI acceptance or original registration OK
is STOP. The V2 observer stops before adapter creation after original admission;
the runner independently stops on registration OK before classifying any later
adapter/refusal record. An old-looking admission plus adapter exit12 sequence
could not be credited as rejection by this V2 logic.

For code3, verify the actual original source bytes/name, input index, phase,
severity, span bytes, optional category/code, primary range and notes against
the recorded structured diagnostic. Verify a successful public renderer
receipt, then compare its entire captured stderr byte-for-byte with exactly one
CLI stream; the other CLI stream must be empty. Preserve which channel matched.
Do not normalize or infer category/code from a substring or borrow expected
spelling from producer data. Optional null fields remain null.

Only original parse OK + original register SEMANTIC_ERROR, healthy processes
and this exact original diagnostic correspondence can give c1/c2
ORIGINAL_INVALID_SOURCE_REJECTED. This means rejection of the exact frozen
invalid input. A particular arithmetic-law algorithm or diagnostic path stays
UNESTABLISHED; no unprovided typed law enum is demanded. Parser-only, unsupported,
precision, adapter-only, mismatch, unknown or unhealthy cases remain
INCONCLUSIVE/STOP for that grade.

For lawful sources, retain the actual original parser/check refusal and its
binding without asserting source invalidity or numeric/native support. CLI
success with observer refusal is STOP, with no native bypass. Known/Unknown,
observed arithmetic relations, native process completion and missing runtime/
whole-call observations remain separate even when job and CTest are green.

## 7. Report retrieval and technical results separately

For each job record:

- Exact run URL/ID, attempt, job URL/ID/name, event/ref/head and checkout SHA.
- Actual individual-GET receipt hash and time, publisher step result.
- Verified raw file count/bytes and transport/runner manifest hashes.
- COMPLETE_RETRIEVED, INCOMPLETE_PREFIX, NOT_RETRIEVED, or INTEGRITY_MISMATCH.
- Actual source/API/native outcome and all missing observations.

A successful publisher process, log availability, base64 syntax, source hash,
number/ID equality, individual arithmetic validator or native exit0 are
different evidence. They do not establish complete call/result/world custody,
internal runtime quantities/order, general soundness or Product GO.

No job/run/log exists for this text prep. This procedure itself has not been
executed or mocked. After PLAN V2 FINAL POSTGET, STOP C.
