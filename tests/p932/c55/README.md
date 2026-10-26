# C55 calibrated one-token continuation

This directory records the separately authorized continuation of P932.
The original C54 STOP commit is
`2b4e8cc1d269085edc22ca3a44f801ee3631b29b` (1/4 PASS, 3/4 FAIL, CI0).
Its original README, freeze, manifest and raw failure are intentionally retained
unchanged as historical evidence, not regraded as a first-pass success.

Only code change: aggregate initializer fields are `NL_SYNTAX_FIELD`, not
`NL_SYNTAX_BINDING`; the existing `data.binding` payload access is unchanged.
`one-token.diff` is the exact correction against the STOP commit. No Production,
public authority, parser/checker, backend, source, expected-value or CMake edit.

Before the corrected binary was tested, `pre-run-freeze.json` recorded the
original branch HEAD, corrected file Git blob and SHA256, exact diff SHA256,
unchanged original source/oracle/first-failure hashes and predicted comparisons.
The corrected adapter blob is
`12c8f58cadad98a38d82468a942a5f1a13ce2670`, SHA256
`bb271e51f19207e64e70978b561952898d1ddb44ecb9e9c0d07489608eb9c5bb`.
The pre-run freeze SHA256 is
`df085fdb10e2795c355f0f4184dc7650a882ad833e1c19c7bb2ec65d5bb32067`.

First corrected GCC13.3 Debug frontend configure/build and focused CTest:
**4/4 PASS**, exit0. This is the sole first corrected diagnostic invocation,
not a retry of a failed C55 test. After its success, the explicitly authorized
full frontend regression passed **474/474**. Available local Clang23.1.2
ASan/LSan and UBSan checks passed **4/4 each**; these are local preflights,
not the pinned Clang23.1.3 FULL5 jobs. Raw first logs are separate files here.

Source-derived comparison reaches eight definition/actual-main captures in
each normal/negative run. New mismatch refusal counts are
`3,4,3,4,4,5,3,4` (30 total): wrong registered nominal type, initializer
permutation, member-owner ValueId used as PlaceId, copied definition used as
current caller, and prior source order with identical declared values.
Existing P930 Expected comparisons still reject26. These are **diagnostic
comparison refusals**, not a new public validator or semantic proof theorem.
Only copied packets are perturbed; original source/context is never mutated.

Both real prepare OOM sites reach hit1, emit zero observations before rollback,
preserve original context inventory and complete healthy same-context retries.
The adapter itself allocates nothing. Ordinary CLI positives stay CHECK0 /
both C17 modes EXIT4 `V1-BACKEND-UNSUPPORTED`, controls stay semantic EXIT3,
and named output sentinel/no-temp behavior is preserved. No native for this
unit-IF program shape, no new public proof or authority.

One DRAFT PR / natural FULL5+PARTIAL pair is permitted only after these local
results; final Actions results belong in Issue932 at the immutable PR HEAD.
No repair push, rerun, rebase or merge. Candidate adoption is Coordination's
decision. F72 B mathematical result is unchanged; general A/C refinement,
injective Encoding and source-event history/activeBlocks remain UNPROVED;
D rich governing lifetime/carrier/access/ending remains HARD HOLD.

VALUE-ALIGNMENT-RISK: finite source-origin diagnostics improve a missing
correspondence check, but no ordinary compiler/native capability or measured
author+safe-caller UB audit-locality benefit is added. Product GO PAUSED,
H1 HOLD and all independent R evidence remain unchanged.
