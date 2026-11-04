# R1157 C234 evidence custody

This branch stores the original execution evidence for independent R1157. It is a custody archive, not adopted compiler code or a new validation run.

- Authority: [C234 custody release6631707594](https://github.com/wakairo/NewLang_Compiler/issues/1157#issuecomment-6631707594).
- Final finite result: [R receipt6631709037](https://github.com/wakairo/NewLang_Compiler/issues/1157#issuecomment-6631709037).
- Sole parent: `5be4b8ef6f8ab4f109841939e75632859090a1d1`; base tree `5a0fd7c42fc2b1e2662714be14865addd6dd1492`.
- Fixed compiler: `af16bbaebcaada68ff45afdf236d1debfc90b565`; authority tree `74a1bdc728398eb85bb690a9eaff34194f316b50`.
- Archive branch: `r-archive/r1157-c234-evidence`. R-fast remains frozen at V5; PR1159 remains Draft NEVERMERGE.
- Manifest SHA256: `bcda3c73b408157cac916abb3415620ea7bc4d18bc4307684aa0a7f05f3f4ba7`.

## Contents and exact restoration

[manifest.json](manifest.json) lists all28 source run/job/attempt identities, R HEAD and actual checkout/tree identities, original conclusions, payload paths, sizes, SHA256 values and Git blob IDs.

There are28 complete decoded UTF-8 job logs (26,552,105 bytes) and24 complete original compressed capsules (9,088,833 bytes). The two index files bring the total to54 added paths. No payload is chunked. Logs retain any UTF-8 BOM and exact original line endings. The .tar.gz files are binary compressed bytes, not base64 text and not newly packed archives.

Restore by obtaining each listed path as raw bytes. Do not strip a BOM, translate line endings, parse/reformat a log, or recompress a capsule before hashing. Compare byte count and SHA256 with the manifest. Every capsule can then be inspected with an ordinary gzip/tar reader as data. Do not execute archived runners, C files or commands as part of custody.

All sources, oracles, exact executed runners, generated C, public-node output, command streams and observations are recoverable from the fixed parent tree and the original capsules. The original6059 leaves and all3 workflows are retained mode/type/blob-identically.

## Historical status is preserved

The24 normal jobs comprise18 original RED jobs and6 final PASS jobs. The4 FAST jobs are preparation-only successes. All were attempt1; manual rerun/dispatch0.

- Initial FIRST: the fourth negative actually rejected at an earlier unsupported profile, but the first runner did not classify that diagnostic. Remaining phases were NOT_REACHED; the11 inherited freeze/history failures are retained.
- V3: the three original equality-SUB positive predictions rejected with precision. Those remain failed positive predictions. Later owned/C17/native phases were NOT_REACHED.
- V4: all six CHECKs accepted and add_zero genuine owned validation plus identical stdout/named C17 were reached; the observer then rejected the missing explicit loop else. Native0. The stale raw C17 NOT_REACHED bookkeeping remains unchanged, with the actual command reach explained separately.
- V5: unchanged V4 sources and oracle passed under the repaired passive observer:432 native executions (216 original +216 observed), including72 guest-ASan and72 guest-UBSan executions. Finite values, identities and counts are reported in the linked final receipt.

Original V1/V2/V3/V4/V5 records and their expectation files remain unchanged. V1 was formed independently before producer exposure. V4 is explicitly scope-calibrated after public guard-profile disclosure; V5 records the minimal emitter-shape exposure and distinguishes source-comparison truth from the emitted negated branch. No historical failure is relabeled by later success.

PARTIAL is retained as PARTIAL. Its actual guest Clang is Ubuntu clang18.1.3; FULL uses pinned Clang23.1.3. The captured version lines and actual command arguments remain in the original logs/capsules; they are not normalized into a single toolchain claim.

## Scope and execution boundary

The finite result covers six unit-main cases and seven source rejection cases. Original u8 return/export coverage is NOT_RUN. Audit locality is UNMEASURED. General equality/range/termination, T-changing or nested/general loops, heap/aliasing beyond these sources, private proof forgery, reached R-owned OOM and general soundness are not established. Original equality-SUB precision RED remains visible.

Custody adds only evidence/r1157-c234/**. No compiler, native, CTest, formatter, Actions, rerun or dispatch is executed for this archive. No PR is created. Current normal workflows run on pull_request or push to main; FAST runs on push to r-fast/**. This archive branch matches none of those push filters and has no PR.

Final source execution stays stopped at normal24/24, FAST4/6 and manual0. This archive adds no semantic gate and makes no Ready, merge or Product GO claim. GO624 PAUSED, H1 HOLD and P1036–PR1050 STOP remain as recorded.
