# R1144 / C225 evidence custody

This data-only archive preserves the completed bounded R1144 observations for fixed P1139 / PR1142. It is added on `r-archive/r1144-c225-evidence`, with final tested R commit **5f4bde2f74ffe55c90c5c2f64220ecd634daf658** as its sole parent. The execution branch and PR1145 are unchanged. **PR1145 remains DRAFT / NEVERMERGE.**

Authority: [R1144](https://github.com/wakairo/NewLang_Compiler/issues/1144), [data-only release](https://github.com/wakairo/NewLang_Compiler/issues/1144#issuecomment-6623223007), and [actual custody START](https://github.com/wakairo/NewLang_Compiler/issues/1144#issuecomment-6623489784), created **2026-11-04 14:36:06 JST**. This archive adds no semantic/native execution, Actions run, workflow change or PR.

## Fixed identities

| Object | SHA / tree |
|---|---|
| Canonical main at execution release | `acdbf91944a3bac99671d71c6ae23134b56d6c32`; CURRENT_SPEC → Draft17.34 |
| Fixed P HEAD | `773643d2709a5e5db24650069d67c0d9032fefb0` |
| Fixed P tree | `3d230802ca275bd963101c9b575f1567927f5c7a` |
| FIRST R HEAD | `a835122ead5285cb4496d216814180e58fde48b0` |
| FIRST test merge / tree | `632225766d01587d73467e7f7612086b1e3af7e5` / `a4d4cfdbd56e5093b33bee9017b5b3dcdfb0b2e2` |
| FINAL R HEAD | `5f4bde2f74ffe55c90c5c2f64220ecd634daf658` |
| FINAL R tree | `809b189db90e2b4b6ddceb40eeba65b9baa7996e` |
| FINAL actual test merge | `18055bb1a64d82787d0a86118976ac97f1358f9f`; same tree as FINAL R |

Both test merges have the fixed P and their corresponding R HEAD as parents. Every normal job's actual checkout and tree were checked against these identities. Commit objects, PR snapshots and the **5,918 existing leaves** of the final R tree are retained in [metadata/refs.json](metadata/refs.json) and [metadata/base-leaves.json](metadata/base-leaves.json). Archive completion verifies that every original leaf's path, mode, type and Git object ID is unchanged.

## Actual outcomes and budget

| Phase | Runs | Actual jobs | Outcome |
|---|---|---:|---|
| FAST preparation | 41775881087, 41776114561, 41776347672, 41776609675, 41778684244 | 5 | RED / GREEN / RED / GREEN / GREEN; format/build/registration only |
| FIRST normal | FULL 41776901686; PARTIAL 41776901796 | 6 | FULL5 each 666 PASS + 9 FAIL out of 675; PARTIAL 665 PASS + 9 FAIL out of 674 |
| FINAL normal | FULL 41778689950; PARTIAL 41778689935 | 6 | FULL5 each 675/675; PARTIAL 674/674; zero skips |

All nine runs are attempt 1. **FAST 5/6; normal 12/12; manual rerun/dispatch 0.** The unused FAST allowance is not permission for further execution after completed success. See the actual per-job [budget ledger](budget-ledger.json).

FIRST's nine failures were indirect original freeze/history callers seeing the R-modified root CMake. Direct baseline/protocol and R1144's own cohort passed even in FIRST. The one mechanical repair preserved original scripts, assertions, expected bytes and arguments, authenticated all 3,338 protected inputs, and projected only the known parent CMake through a private Git index derived from actual tracked records. Only p990's explicit protected-source-directory argument is mapped to equal bytes in that snapshot; actual compiler/probe/build/toolchain paths remain actual.

FINAL actually executes and passes all nine original callers, including the original p1062 history path with its ambient-untracked-file rejection, and both original baseline/protocol tests. Successful child CTest output is suppressed by the unchanged workflows; the original scripts and their fail-closed bridge establish the internal control path. No separate printed ambient-control transcript is invented.

The FIRST PARTIAL downstream Production CLI step was **NOT_RUN** after CTest failure. FINAL's downstream CLI step actually ran with exit 0 and empty stdout/stderr. FIRST is not rewritten as green.

Each normal job has 12 R cases and 24 successful R native executions: 12 unmodified guest plus 12 passive observer. **FINAL native 144 = 72 unmodified + 72 passive.** FIRST separately executed the same cohort 144 times. Across both groups, **288 = 144 unmodified + 144 passive**, with guest ASan24 / UBSan24 and matching passive counts; FINAL alone has guest ASan12 / UBSan12 and matching passive counts. These are repeated runs of the same frozen cohort, not additional independent inputs. All native compile/run statuses and stdout/stderr are retained.

FIRST failed-test output also preserves 1,008 successful old native observations (168/job) before its eventual freeze/history failure, plus 12 planned wrong-return calibration detections in the four non-sanitized jobs. This is the visible subset in failed-test output, not a count of every old CTest's native work. Those nine CTests remain FAIL in FIRST.

The last FINAL job, ASan, completed **2026-11-04 14:34:26 JST**. [R FINAL](receipts/r-final.md) and [C finite R acceptance](receipts/c-r-accept.md) are preserved. [C substantive P PASS](receipts/c-p1142-pass.md), the [actual Ready receipt](receipts/c-p1142-ready.md), and [PR1142 Ready snapshot](receipts/p1142-ready.json) record the later technical handoff. Ready occurred at **14:40:16 JST**; the captured P state is open, Ready, mergeable and unmerged. **Merge remains user-only.** Custody does not add a P adoption gate.

## Contents and reconstruction

- `logs/fast/`, `logs/first/`, `logs/final/`: all **17 complete decoded job logs**, preserved as UTF-8 text exactly as returned by the GitHub log tool, including original timestamps and control characters.
- `envelopes/first/` and `envelopes/final/`: each complete timestamped BEGIN/DATA/END block extracted from those logs.
- `raw/first/` and `raw/final/`: **12 exact decoded envelope JSON byte sequences**, without adding a newline. Each has `summary`, all 81 `commands`, and 73 `files` records.
- `results/`: the actual `R1144_RESULT` JSON payload from each raw log. It matches its envelope summary.
- `metadata/`: complete run/job/artifact metadata, Git identities, original leaf inventory, and all three original workflow texts/trigger assessment.
- `receipts/`: Issue/PR snapshots, exact V1/V2 freeze bodies, exposure/calibration/repair/C judgments and completed technical handoff receipts. [timeline.md](timeline.md) orders them by actual JST timestamps.
- [audit-summary.json](audit-summary.json): C helper's parsed evidence audit, with FIRST and FINAL separated.
- [manifest.json](manifest.json): exact UTF-8 byte lengths, LF counts, SHA256 and Git blob IDs for every other archive file. The manifest excludes itself; its independently computed hash is recorded in the live custody-completion receipt.

The 73 file records per envelope retain `base64`, `bytes` and `sha256`, so **all 876 records** can be recovered without duplicating generated C and outputs across hundreds of extra paths. For data-only verification of one envelope:

```python
import base64, hashlib, json
from pathlib import Path

raw = Path("raw/final/126404947575.json").read_bytes()
payload = json.loads(raw)
for name, record in payload["files"].items():
    value = base64.b64decode(record["base64"], validate=True)
    assert len(value) == record["bytes"], name
    assert hashlib.sha256(value).hexdigest() == record["sha256"], name
```

The envelope BEGIN record supplies the hash and byte length of the complete JSON above. Reconstruction is byte decoding and hashing; it does not replay any command or execute generated C. Original log bytes, all envelope/file bytes and the manifest are independently rehashed before publication, then every archive path is read back from Git and compared. The live completion receipt supplies the final archive commit/tree and completed readback result.

## Formatter FIRST evidence and ZIP limitation

Both RED FAST jobs preserve the complete decoded suggestion block, including the incidental old-P `tests/p1117/lineage.c` deletion hunk. No archived suggestion is applied.

`formatter/*-suggestion.decoded.log` retains the exact timestamped block. `*.from-log.patch` removes only each standard timestamp prefix from the block's payload lines. It is explicitly a decoded-log extraction, not a claim to possess the original artifact ZIP bytes.

| Artifact | GitHub-reported size | GitHub-reported ZIP digest |
|---|---:|---|
| 13296503903 / FIRST FAST | 1,751 bytes | `623e50958786dc0456d92e696f217fbac0a906c8fc36459efb7d0f5ccb3c7445` |
| 13297371515 / FAST3 | 1,488 bytes | `e160dbe78cadb404b870595d96c399222b0fa2b1b34342a47b95fe800d759269` |

**The two original formatter ZIP binaries were not retrieved or independently hashed.** Their reported metadata is preserved, and is not substituted for a verified binary hash. Complete decoded logs and original suggestion text are preserved. No other unread original bytes are claimed to have been obtained.

## Freeze, independence and observation limits

The V1 and V2 source/oracle files remain unchanged in the parent tree, alongside the exact frozen Issue bodies. V1 oracle SHA256 is `cb7d9db213332a6ccb7dd46ac3063039cab74fbfb236cc2bebebc6f700a637bd`; V2 oracle SHA256 is `2e29cceb34c9e6fa323bbcb45629a076253aeb48241b0fad3d5259ef3f0e8176`. All 26 source/oracle files were independently matched to their declared bytes/LFs/hashes.

**V1 is NOT_RUN.** The final independence label is:

> pre-execution frozen independent cohort / post-freeze interface-calibrated V2 / incidental old-P formatter exposure after freeze / incidental old-P registration exposure after FIRST execution

FIRST envelopes retain their as-issued three-clause label. The fourth clause records later exposure after FIRST execution and appears in FINAL. Neither exposure is erased, retroactively preapproved or described as unrestricted blindness.

Positive observations retain genuine source-issued owned metadata, actual Break → outer binding → ordinary Copy → predicate actual → selected value, and complete phase/Continue ordering for 0/1/3 backedges. All six generated-C identities are identical across all 12 jobs. Removing only the read-only observer additions recovers each original C exactly. Unmodified guest exit 0 makes no selected-value claim; passive value observations are reported separately.

N1 stops at registration subtraction precision and N4 at registration addition precision; neither demonstrates the intended later diagnostic. N6 is canonical-legal [2,0] with a conservative subtraction precision refusal, not an invalid-program result. N2/N3/N5 retain their actual continue-type / continue-arity / unknown-binding diagnostics. Negatives execute no native guest.

Owned `direct_break_provenance=false` is preserved; the private cyclic relation has a separate bounded role. Predicate branch is inferred from the distinct source-selected return tag, with no separate branch marker claimed. Finite traces do not establish general range analysis, termination, ownership, universal soundness or general private authority.

## Goal alignment and ongoing stops

The nine old-gate connections, 3,338 protected files and private-index projection represent actual verification-maintenance work. C records this as **VALUE-ALIGNMENT-RISK**, separately from the bounded technical PASS. This archive does not establish that the library-author plus ordinary-safe-caller UB audit/re-audit dependency closure has become smaller.

**Audit locality UNMEASURED; GO624 PAUSED; H1 HOLD; P1036 / PR1050 STOP.** Unexpected canonical/semantic/public source/API/ABI/authority changes, unresolved soundness concerns, scope/environment/independence changes or budget excess remain affected STOP → C.

The original three workflows remain unchanged: ordinary jobs trigger on PR events or pushes to main; FAST triggers on r-fast/** pushes. The archive branch matches none and receives no PR. The archive-completion receipt verifies additional Actions/PR/semantic executions = 0 and all 5,918 original leaves unchanged. Custody acceptance is recorded separately after C's independent readback.
