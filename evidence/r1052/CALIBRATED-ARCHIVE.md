# R1052 — Original R1051 CALIBRATED PR1048 attempt1 raw evidence custody

**Status:** user-uploaded original ZIP was **independently re-read from the GitHub blob and accepted as byte-identical** by Coordination. **Archive only; not a compiler/native/safety pass.**

## Immutable source identity and archive

| Field | Exact value |
|---|---|
| Original GitHub Actions run / attempt | [40984457279 / attempt1](https://github.com/wakairo/NewLang_Compiler/actions/runs/40984457279) |
| Original artifact | [#12946890022](https://github.com/wakairo/NewLang_Compiler/actions/runs/40984457279/artifacts/12946890022) |
| Artifact creation / expiry | 2026-10-30T01:13:06Z / 2026-11-13T01:13:05Z |
| Original R executor HEAD | `68f7fe6a0f0ff934a827fd9ad0459bc9d67caeaa` |
| Original executor tree | `77ac35417e52ff29274ec5008806382401c6660a` |
| Original executor sole parent | `e161666b611a9fe6b501cb132b7c7e916c5f5d2b` |
| Original workflow ref | `r-calibrated/r1016-pr1048` (do not re-push/re-run) |
| Fixed compiler candidate | PR1048 HEAD `ade11ebec808f5a95e8831a7ae6ee6f6da2b0dbe`, tree `70305183f8951038d296ac9a9ecae1f8e8750e4f` |
| Original GitHub ZIP size | **48639 bytes** |
| Original GitHub ZIP SHA256 | **`b79662261d7bbb23626cf57e9852f13b2f69e5dd192893214feb36bd878a075a`** |
| Exact ZIP Git blob SHA-1 | **`eb680f99bfd30fd4e27a3e5f8c562e811a59a70e`** |
| Immutable ZIP location | [`evidence/r1052/calibrated-pr1048-40984457279-attempt1.zip`](calibrated-pr1048-40984457279-attempt1.zip) |
| User's one-file archival commit | [`136f584a9e2fbcc55cdaf4b568a3a9a2fe158838`](https://github.com/wakairo/NewLang_Compiler/commit/136f584a9e2fbcc55cdaf4b568a3a9a2fe158838) |
| Archive branch | `r-archive/r1052-calibrated-pr1048-custody` (nontriggering, never merged to `main`) |

### Independent readback checks

Coordination directly fetched the GitHub repository ZIP as **base64-encoded Git file bytes**, decoded the response inside the connected read-only verification context, and **computed SHA256 on the reconstructed exact 48639 bytes**. The digest matches the GitHub original artifact's reported SHA256 and Track R's earlier direct-artifact SHA256. The returned **Git blob OID is exactly `eb680f99bfd30fd4e27a3e5f8c562e811a59a70e`**.

The read-back ZIP central directory has **66 entries, 66 distinct filenames**, correct central offset/length and terminating end-of-central-directory structure; `CALIBRATED-RAW.jsonl` is present. This byte-for-byte SHA256 identity means the original R/C artifact-level observations carry over to the archived bytes without expansion/recompression. Earlier Track R's direct ZIP verification recorded **CRC 66/66**, **98/98 JSONL rows**, **23 setup + 30 stage stream hash/length matches = 53/53** and 15 END stages; a separate earlier C local copy verification recorded ZIP 66/CRC clean/JSONL98. **The new GitHub-only readback independently checked SHA256, length, directory, filename uniqueness and Git blob OID; it did not separately inflate all ZIP members or recompute the 53 per-stream hashes at this readback stage.** Those latter claims rely on the already matched exact original bytes and clearly attributed earlier checks.

Git history provenance: the archive branch had one new user-authored ZIP-only commit `136f584…` whose **sole parent is exact R executor `68f7fe6…`**. Git comparison from the executor to that commit found **only the ZIP above added**. The existing original-FIRST ZIP Git blob `7467df2023c84fe86409f815136df779de728bae`, calibrated-v2 Git blob `b85e5e794b03d1d623e71d3a5fa6d36cccf18060`, six frozen R original source files and seventh oracle are inherited unchanged from the executor. This document is a **second permitted documentation-only commit** on the same dedicated archive branch under [C142 conditional human-binary-transport approval](https://github.com/wakairo/NewLang_Compiler/issues/1052#issuecomment-6514190874). No `main`, Production, P/T fixtures/runners/oracles, original R1016 or original other archive data are modified.

### What this evidence proves and does not prove

The original R1051 result is **authentic CALIBRATED C17 RED** for fixed PR1048 on both frozen original independent positive programs: source CHECK exit0 and narrowly observed public current/event stages, but C17 emission exit4 `V1-BACKEND-UNSUPPORTED` with **no emitted C/native O0/O2, actual H/T physical equivalence, host OOM, sanitizer or all failure-world observation**. Four frozen original negatives refused at CHECK exit3 and recorded no-clobber output sentinel; that does not promote every deeper intended negative guard to proven. The workflow run's `success` means raw evidence collection succeeded, **not** semantic/native verification success. Earlier original FIRST run `40849018312/attempt1` remains a distinct preserved original RED; v1 `40869364041/attempt1` raw loss and v2 `40874742688/attempt1` artifact are separate historical records, not replacements.

**No new R run or attempt** occurred as a result of this archive branch. An Actions run query against the archive branch returned **0 workflow runs** at initial verification; after documentation publication this remains subject to a fresh check. No compilation, local P-equivalent environment, test, CI or oracle revision is authorized by archive custody. No READY, merge, Product GO or compiler safety PASS. R1051 raw source/oracle/current/negative expectations remain immutable.

### Goal alignment and limits

[Project Compass](../../docs/NewLang_Project_Compass.md) is only a navigation entry; [Product / Value Strategy §1.1](../../docs/NewLang_Product_Value_Strategy.md) defines the actual value hypothesis: preserve C-class pointer topology/storage/allocation/lifetime/failure architecture while reducing the **human UB-prevention audit and re-audit dependency closure of library authors and ordinary-safe callers** relative to well-factored C. This ZIP establishes provenance/custody for a negative independent compiler-capability observation, **not** measured reduction of this human burden. The extra archive/document-maintenance cost is acknowledged; no general manifest, new checker, broader MIR/LLVM or repeated run is justified by this act. [Development Process §4.1.2](../../docs/NewLang_Project_Development_Process.md) autonomy remains subordinate to Issue stop constraints. [CURRENT_SPEC](../../docs/reference/CURRENT_SPEC.md) selects canonical Draft17.34; Product GO #624 PAUSED, H1 #268 HOLD. KEEP existing source meaning/REPLACE NONE.

**Custody acceptance is deliberately narrower than compiler acceptance.**
