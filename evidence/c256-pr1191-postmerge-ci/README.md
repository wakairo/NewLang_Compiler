# C256 PR1191 postmerge CI evidence

This archive preserves the **unchanged C postmerge CI audit ZIP**, including all six captured whole job logs, and its original external FINAL receipt. Its sole Git parent is the adopted main commit `53c125b8cd66523651a21e803827eff6d2a06428`, whose tree is `1a4fcf50e76b0d2def314fdf7720f88d52a6581c`. Only `evidence/c256-pr1191-postmerge-ci/` is added to that parent tree.

## Completed checkpoint and counting unit

The original C audit records two natural **push/main runs, attempt 1**, both successful at that exact main SHA: [FULL run 42153925727](https://github.com/wakairo/NewLang_Compiler/actions/runs/42153925727) and [PARTIAL run 42153925551](https://github.com/wakairo/NewLang_Compiler/actions/runs/42153925551).

| Job group | Jobs | Passing CTests per job | Passing executions across the group |
|---|---:|---:|---:|
| FULL | 5 | 756/756 | 3780 |
| PARTIAL | 1 | 755/755 | 755 |
| All six jobs | 6 | See the two rows above | **4535** |

Thus **5 x 756 + 755 = 4535** is the aggregate across all six postmerge jobs. R recovery's 4541 aggregate belongs to its separate checkpoint and is not the count for this main integration stage. The captured audit records matching fixed-P test-name sets, all required 12 and C254 new 10 tests passing in every job, and no failure/skip/timeout/sanitizer diagnostics. PARTIAL's actual post-CTest CLI step exits **0 with empty stdout and stderr**. The separately authorized C integration budget is **6/6 jobs, zero remaining, manual rerun/dispatch 0**. Archiving performs no additional CI or tests.

## Original bytes and inventory

The Git inventory is **five blobs**: two base64 parts, this README, an outer `manifest.json`, and the unchanged source `FINAL-RECEIPT.json`. The source ZIP has **25 payloads plus its own manifest = 26 unique members**, including six whole logs totaling **1,488,290 bytes**. Its external FINAL receipt is outside those 26 ZIP members.

| Object | Bytes | SHA-256 |
|---|---:|---|
| Original audit ZIP | 559942 | `4624fa1d688190f8ddb05417da94977f0d3234f93f73cce576f5c40d50bdbd67` |
| Manifest inside ZIP | 8864 | `9f532a56d2ac2fb9d68d0d870a11a3d11a5f94b1bc44cf8e495460dbe29c5b2b` |
| Original external FINAL receipt | 1645 | `4c9610575e72fd50372caa578cea80d53fde526f968b5cdab51bf07e2c7ca685` |
| Summary inside ZIP | 7800 | `435d88997d3f5df10b90699f99c6bd35fdd7ebc64c6e94c853e6d0e94a6d7817` |

Base64 parts are standard ASCII without whitespace or trailing newline. Per-part binary offsets, sizes, SHA-256 hashes, encoded-text hashes and Git blob identities are fixed in the outer manifest. The original ZIP is not repacked or trimmed.

## Seven baseline references

The source audit's seven baseline inputs remain references to [immutable P archive commit `6e3868f58e036f3ff744da8b32b7a854b5cb112b`](https://github.com/wakairo/NewLang_Compiler/tree/6e3868f58e036f3ff744da8b32b7a854b5cb112b/evidence/c255-pr1191-ci). Restore that archive's `ci-audit-packet.zip`; the referenced members are `audit-final.json` and six files under `final/logs/`. The outer manifest lists each exact ZIP-member locator, size and SHA-256. These seven files were compared to the already preserved local P packet; no log was fetched again, and their payloads are not duplicated in this archive.

## Verification and scope

Before anchoring, every complete remote UTF-8 blob is compared with the exact pinned local ASCII/metadata bytes. The ZIP is reconstructed from those verified-equal local parts and checked against the original bytes, global SHA-256, ZIP CRC, exact member set, every payload hash and original file bytes, the internal manifest, and the original external FINAL receipt. This proof method does not claim duplicate on-disk copies of remote responses.

This is a custody archive of the completed C integration audit. It does not run a compiler, build, guest, AST tool, observer or semantic decoder, and makes no new general soundness, R or Product GO decision. The logs preserve the connector-decoded UTF-8 bytes, including their BOM/newlines; original GitHub binary log-ZIP byte identity is not asserted. The audit's Actions inventory covers this exact adopted main SHA, not all repository history. PR auto-retarget/deleted-branch preservation remains a separate Coordinator task.

All R archives and existing refs remain untouched by this operation. After readback, exact commit/tree/sole-parent checks and confirmation that the dedicated ref is absent, the only new ref is `refs/heads/evidence/c256-pr1191-postmerge-ci`. The unchanged three workflows accept push on `main` (normal two) or `r-fast/**` (preflight); this evidence branch matches none. The C archive receipt records ref readback and observed Actions counts. There is no PR, tag, merge, Issue post, existing-ref update, dispatch or rerun.

## Restore as data only

Run from this archive directory. This verifies archive data without executing any archived program or extracting into a checkout.

```python
from pathlib import Path
import base64, hashlib, io, json, zipfile
sha = lambda b: hashlib.sha256(b).hexdigest()
m = json.loads(Path("manifest.json").read_text())
parts, offset = [], 0
for i, p in enumerate(m["parts"], 1):
    assert p["index"] == i and p["binary_offset"] == offset
    text = Path(p["path"]).read_bytes()
    assert (len(text), sha(text)) == (p["ascii_bytes"], p["ascii_sha256"])
    raw = base64.b64decode(text, validate=True)
    assert (len(raw), sha(raw)) == (p["binary_bytes"], p["binary_sha256"])
    parts.append(raw)
    offset += len(raw)
assert len(parts) == 2
packet = b"".join(parts)
assert (len(packet), sha(packet)) == (m["packet"]["bytes"], m["packet"]["sha256"])
with zipfile.ZipFile(io.BytesIO(packet)) as archive:
    assert archive.testzip() is None
    assert len(archive.namelist()) == len(set(archive.namelist())) == 26
    inner = archive.read("manifest.json")
    assert (len(inner), sha(inner)) == (m["inner_manifest"]["bytes"], m["inner_manifest"]["sha256"])
    entries = json.loads(inner)["payload_files"]
    assert len(entries) == 25
    assert set(archive.namelist()) == {e["archive_path"] for e in entries} | {"manifest.json"}
    for e in entries:
        raw = archive.read(e["archive_path"])
        assert (len(raw), sha(raw)) == (e["bytes"], e["sha256"])
r = m["source_FINAL_receipt"]
receipt = Path(r["path"]).read_bytes()
assert (len(receipt), sha(receipt)) == (r["bytes"], r["sha256"])
out = Path("restored-postmerge-ci")
out.mkdir(exist_ok=False)
with (out / "ci-audit-packet.zip").open("xb") as f:
    f.write(packet)
```
