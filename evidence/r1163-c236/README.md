# R1163 terminal evidence — finite R receipts PASS, overall CI STOP / RED

This is the evidence-only archive for [Issue1163](https://github.com/wakairo/NewLang_Compiler/issues/1163)
and [Draft NEVERMERGE PR1165](https://github.com/wakairo/NewLang_Compiler/pull/1165).
The measured HEAD is **df73c2913d18045a7c97a90f6bf5088644868782**, tree
**d977df728458b985c3738f20b3d64af91da67097**. The archive's sole parent is that
HEAD; it adds files only under `evidence/r1163-c236/` and performs no execution.

## Disposition and unresolved boundary

**Overall R CI remains RED; normal budget is exhausted.** Each final FULL job
passed 697/698 CTests, and PARTIAL passed 696/697. Every final job failed only
`p1062_h_body_0.integration`. In the verified fixed-P snapshot, the unchanged
`tests/p1062/history_tests.py:88` -> `history_v12.py:79` -> `git_inputs:69` chain
reports **missing Git-managed inherited input**. The snapshot preserves P file
bytes/modes, but its Git-managed-input context does not satisfy this inherited
history check. This is the unresolved R transport boundary. No assertion was
skipped or weakened. No repair or extra run follows the exhausted budget.

PARTIAL's workflow step `Verify real Production source/checker CLI path (NO LLVM;
PARTIAL)` was **skipped** after the failing CTest step. The R driver's separate
actual source CHECK/stdout/named CLI receipts below did execute. These are distinct
facts. No all-six regression-green or general compiler validation is claimed.

The independent R CTest itself passed in **all six final jobs**: six positive
sources, seven negative sources, and **48 native runs per job**. The total is
**288 native runs: 144 unmodified + 144 passive**. Every native run exited zero
with empty stderr. Guest ASan and UBSan each cover 48 native runs (24 of each
form), with actual `-fsanitize=address` / `-fsanitize=undefined`,
`-fno-omit-frame-pointer`, and `-fno-sanitize-recover=all` on all guest builds.
Host sanitizer labels and guest flags are recorded separately in each capsule.

## Immutable identity and budget

| Role | Commit / tree |
|---|---|
| Canonical authority, CURRENT_SPEC Draft17.34 | `449ccf0dc567c05f3fd6dc4430b265759e22d571` / `74a1bdc728398eb85bb690a9eaff34194f316b50` |
| Fixed P compiler content | `9030055314600b804fafde64adebb03c4d6315a1` / `261a8e404a1cf1982e13d3b94e3223d5da0e0499` |
| Complete initial R object, before first ref | `735677fbc462c70fbe1af844a4542720b2263361` / `cd5b9314317ed70b8d19f712ecd8819c77986896` |
| Formatting-only forward repair / FIRST normal HEAD | `52b5b577e33b7be45419a797f6994565f2923b28` / `4deaa3daeb71d90f67855759712b1d12de96cce1` |
| M2 combined machinery repair / final measured HEAD | `df73c2913d18045a7c97a90f6bf5088644868782` / `d977df728458b985c3738f20b3d64af91da67097` |
| FIRST PR test checkout | `75ee7c3bea0e83b1b6207021cc4071dc39a9f906` |
| Final PR test checkout | `c15349a2aec6d712920961093f970086c666f61a`, parents P903 + M2df73, exact M2 tree |

The complete ledger contains **7 runs / 15 actual jobs**, every attempt 1:
FAST3/6, normal12/12, manual0. FAST1 failed the new R probe's formatting; FAST2
and FAST3 succeeded as format/build/registration preparation only. The first
normal six jobs failed; the last six are also full-regression RED as stated
above. No cancelled, retried, unrecorded or additional measurement job is used.
The original R ref remains at M2. Archive creation has no PR and no matching
workflow push trigger; the separate Issue receipt records its zero-Actions check.

## Independent inputs and exposure

Classification: **canonical-first / declared-profile-scoped / pre-execution
independently frozen cohort**. This is not an unrestricted all-period blind claim.
Canonical text, declared public profile, public grammar/headers and CLI contract
preceded the independent source/oracle freeze. No producer implementation,
fixtures/oracles or old R/F cases were used to design this cohort.

V1 and V2 remain complete **NOT_RUN** artifacts. C corrected an incomplete public
profile before execution: V3 uses direct carry expressions and parameter-left /
literal-right strict-less-than guards, while restoring the original actual
`break cursor`. The independent numerical and trace expectations were retained.
V3's oracle SHA256 is
`cfd4cd373e3b1b1e3c358b546cdd5691687bbc87ccc7de8e1eb31f83bc71fc63`.
All 42 V1/V2/V3 input/oracle files are preserved in the measured parent tree;
`frozen-inputs.json` records their bytes, SHA256 and Git blob identities.

After freezing, minimal CMake/history launch interfaces and the opaque fixed-P
tree manifest were read for the authorized adapter. P formatter suggestions and
inherited-test output appeared incidentally after the freeze. They were not used
as new source/oracle inputs. M2 observer repairs used this R's own FIRST actual
generated C and actual Clang AST. Original and passive C remain separate.

## Finite result matrix

Each row ran under GCC and Clang, O0 and O2, in unmodified and passive form in
each of the six final host jobs. Full jobs recorded GCC13.3 and curated
Clang23.1.3; PARTIAL recorded GCC13.3 and Ubuntu Clang18.1.3. Exact full version
output is in every capsule. Strict C17 includes Wall/Wextra/Wpedantic/Werror;
the already pinned Clang23 installation warning remains a warning through its
specific `-Wno-error=gcc-install-dir-libstdcxx` flag.

| Case | Actual phase trace | Result / compare / Bool | RETURN arm | Actual path | Child index | Normal old / current / sibling |
|---|---|---|---|---|---:|---|
| a ADD, 0 backedges | 4 | 4 / 4 / true | then | early RETURN | 1 | no store or postread |
| b ADD, 1 backedge | 12, 13 | 13 / 14 / false | else | early RETURN | 0 | no store or postread |
| c ADD, 3 backedges | 20, 21, 22, 23 | 23 / 24 / false | then | normal | 0 | 43 / 197 / 157 |
| d SUB, 0 backedges | 8 | 8 / 8 / true | else | normal | 1 | 131 / 223 / 71 |
| e SUB, 1 backedge | 19, 18 | 18 / 18 / true | then | early RETURN | 0 | no store or postread |
| f SUB, 3 backedges | 31, 30, 29, 28 | 28 / 28 / true | else | normal | 1 | 163 / 239 / 97 |

All 144 passive runs observed the root comparison once and only its selected
source arm. The 72 early returns observed zero normal stores/postreads/loan scope
entries and zero post-IF carrier uses. The 72 normal paths observed one store,
two original-object postreads, and one entry/exit of the actual loan block.
Old-value Copy carriers and live original root/field addresses were recorded.
After the guest returns, the wrapper reads only counters.

The public probe follows actual source-issued owned result -> Copy -> comparison
nodes and validates the RETURN normal replay and both field-event receipts with
original root/child/sibling identities. The public single-direct-Break validator
and generic ADD/SUB receipt views are **not applicable** to these guarded
header-inductive shapes (`single_direct_break_validator=0`, arithmetic
`applicable=0`); these zeroes are not reported as validator passes. Actual loop
values and arithmetic are additionally supported by the C/AST and native traces.

Archive-side pure-text checks reconstruct every passive C exactly from original
C plus its insertion map, follow actual carry -> phase -> Break result -> Copy ->
equality carrier definitions and raw values, and confirm exactly one original
two-field record local initialized before the loop. All six final jobs emitted
identical original C bytes per source. These retrospective checks execute no
compiler or guest and do not modify the frozen oracle.

## Negatives and reached-layer limits

Every final job observed each negative reject with status3 through CHECK, ordinary
emit and named emit, with no emitted C/output file. Owned/native were NOT_REACHED /
NOT_RUN for the rejected sources. The actual earliest diagnostic is preserved:

| Source | Actual earliest diagnostic |
|---|---|
| n_condition_usize | semantic `P13-IF-CONDITION` |
| n_return_payload | semantic `P9-RETURN-TYPE` |
| n_loan_escape | semantic `P8-EXIT-DEPENDENCY` |
| n_return_in_loan | unsupported `P9-RETURN-CONTEXT` |
| n_unknown_child | semantic `FIELD-UNKNOWN-FIELD` |
| n_sub_underflow | semantic `USIZE-SUBTRACTION-UNDERFLOW` |
| n_loop_fallthrough | precision `USIZE-ADDITION-PRECISION` |

The last case rejects earlier at quantity precision. It does **not** establish
later loop-fallthrough validator coverage. No negative is credited with an
unreached backend or owned-validator result.

## FIRST preservation and M2 repair

Every FIRST normal capsule preserves seven negative rejections and positive a's
CHECK/public/ordinary C17/named C17/Clang AST exits0. It then stops at
`MACHINERY_AST: comparison does not produce a named actual bool`. FIRST R native
builds/runs are zero; the other five positives are NOT_RUN. An empty positive
summary array does not erase the five reached layers for a. The initial ten
inherited history failures are also preserved unchanged.

M2 handles actual inline comparisons using an identity-returning recorder, full
balanced macro invocations, typedef-linked anonymous records, source-vs-C branch
polarity, and actual old-value Copy tracking. Original expressions/branches/stores
remain as bytes and evaluate with their own actual results; expected values never
enter the guest. Twelve exact old history launch paths use a complete 5,992-leaf
verified P snapshot with unchanged scripts/assertions and the actual R-built
compiler/CC/SAN. The remaining p1062 Git-context failure is the stated STOP.

## Raw custody and verification

`jobs.json` maps every job/run/attempt/HEAD/checkout to exact digests and reached
layers. `jobs/<id>/decoded.log.gz` contains the **complete original connector-decoded
job log**, losslessly gzip-compressed for storage: 15 logs / **14,800,076 decoded
bytes**. The twelve `capsule.zip` files are the **actual compressed bytes emitted
inside those logs**, totaling **1,617,971 bytes**; they were decoded, CRC-checked
and SHA256-checked. Each member has a byte count and SHA256 manifest. Commands,
stdout/stderr, inputs, original/named/passive C, AST, maps, traces and summaries
are preserved. No log or capsule was synthesized to fill a missing run.

The original GitHub job-log ZIP downloads and original formatter-artifact ZIPs
were **not fetched or independently verified**. The verified claim concerns
the complete decoded logs and the exact R capsule ZIP bytes described above.

`manifest.json` covers every archive file except itself; its own SHA256 is in the
Issue publication receipt. `verify_archive.py` performs an offline hash/CRC/chunk/
member/command-stream/input check and invokes no compiler, native executable,
formatter, CMake, CTest or network. `post_run_audit.py` preserves the workspace
packaging/trace-analysis recipe used for this archive. Git readback of the actual
saved bytes and preservation of all 6,048 measured-parent leaves are recorded
separately in the final Issue receipt.

These finite results do not establish general loop/range/termination, arbitrary
RETURN or identity soundness, OOM exhaustiveness, heap/H/FFI, audit locality or
Product GO. Audit locality stays UNMEASURED; GO624 PAUSED, H1 HOLD, and
P1036–PR1050 STOP remain outside this experiment. Issue disposition and any future
work belong to C. PR1165 remains DRAFT / NEVERMERGE.
