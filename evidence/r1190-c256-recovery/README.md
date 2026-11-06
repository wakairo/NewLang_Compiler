# R1190 C256 recovery evidence archive

This archive preserves **the exact R-authored `r1190_C256_RECOVERY6_COMPLETE.tar.xz` bytes**, without repacking, trimming or changing the source packet. It adds a separate C supporting audit and the original R packaging receipt. The initial archive commit `77b5340673045793e65b6f1f38dbb0a1a5f033fa` has sole Git parent `52dbe24af4fb660930457c741f17bdd87052622b`; the only added tree path is `evidence/r1190-c256-recovery/`.

## Recorded checkpoint and limits

The [R final checkpoint](https://github.com/wakairo/NewLang_Compiler/issues/1190#issuecomment-6675624858) describes **incident-disclosed calibrated mechanical recovery of the independently frozen original inputs, not fresh BLIND**. Its fixed compiler is `f1648a1382ae058e5ab5b9ccdea1e3200879f2af`. The source/oracle remain unchanged; the observer repair and its provenance are recorded by R and C. This archive performs no independent technical adjudication or new semantic execution.

The final R checkpoint and separate C audit record all six recovery jobs as complete/success. The five FULL jobs each pass **757/757 CTests**, and PARTIAL passes **756/756**. Across all six jobs, the aggregate is **4541 passing CTests: 4535 existing P test executions plus 6 R integration executions**. Across six jobs they record **432 native invocations and 216 completed original-oracle comparisons**, with no incomplete positive comparisons or comparison differences. The previously incomplete P3/P5 comparisons are reached in the recovery evidence. FAST is a format/build/registration checkpoint, not a semantic run. Whole-loop validation remains `NOT_RUN / NO_APPLICABLE_PUBLIC_API`; machine CALL/STORE survival is `NOT_MEASURED`; internal helper RETURN_VALUE is not directly observed. Public IDs and runtime addresses retain the artifact/world and in-run qualifications stated in the original receipts. The finite original cohort does not establish general soundness or an archive-owned adoption decision.

The [initial failure archive](https://github.com/wakairo/NewLang_Compiler/tree/8bcd15b69f6d15931e5f528ac3da826ac1330a74/evidence/r1190-c256-initial) remains separate and unchanged. Its initial failures, incomplete decoder results and NOT_RUN comparisons are not replaced by these recovery results. The original FIRST 26 files are preserved and byte-compared across prepared/released/recovery copies, all six recovery raw exports and the initial archive. The older complete C253 packet remains at [immutable commit `e7ab5e9d8c10a6929d2e309a993faa5c3f5f4116`](https://github.com/wakairo/NewLang_Compiler/tree/e7ab5e9d8c10a6929d2e309a993faa5c3f5f4116/evidence/r1190-c253-first).

## Files and custody boundaries

There are **41 Git blobs**: 36 standard-base64 R packet parts, `manifest.json`, this README, `R-PACKET-RECEIPT.json`, and two files under `c-support/`. Base64 text has no whitespace or trailing newline. Decode each part separately in manifest order and concatenate the binary results.

The unchanged R packet contains **7661 payload files / 98,044,971 bytes**, plus `PACKET-MANIFEST.json` and `PACKET-README.txt`, for **7663 unique regular tar members**. It includes recovery whole logs, all six exact raw archives and their expanded members, FAST, API/custody metadata, frozen inputs/equipment and final receipts. Each raw archive contains 1237 regular members, including its index. No exported raw member is stripped. Transient native binaries were not exported by the original driver; their recorded hash receipts are retained. This archive does not reconstruct those binaries.

`R-PACKET-RECEIPT.json` is an exact copy of R's external lossless-packaging receipt. It is **outside** the 7663-member R packet. The R manifest is already preserved inside the packet and is not duplicated as a separate Git blob.

`c-support/ci-audit-packet.zip.base64` reconstructs the separate C audit ZIP, and `c-support/FINAL-RECEIPT.json` preserves its final receipt. That ZIP has 13 audit payloads plus its manifest, including retained audit apparatus/draft material. It does not contain R raw exports. The C audit's **102 referenced inputs** have these explicit custody locations, with individual paths, bytes and SHA-256 values in the outer manifest:

| Inputs | Custody location |
|---:|---|
| 73 | Members of this exact recovery packet |
| 18 | Initial R whole logs/raw/custody under `r1190_c255_successor/runs/` in the initial archive |
| 4 | Initial archive C support: its FINAL-RECEIPT, ZIP, and the ZIP's nested audit-final and manifest |
| 7 | [Immutable C255 P audit archive](https://github.com/wakairo/NewLang_Compiler/tree/6e3868f58e036f3ff744da8b32b7a854b5cb112b/evidence/c255-pr1191-ci): audit-final and six final logs inside its reconstructed ZIP |

The final seven are C comparison-baseline references. They are not R-authored inputs, an R oracle, or evidence of R prior knowledge. The 22 initial and 7 P baseline inputs remain references to their existing immutable archives; they are not silently duplicated or classified as missing recovery payloads.

## Exact identities

| Object | Bytes | SHA-256 |
|---|---:|---|
| Original R XZ packet | 14116724 | `fc7d3551635a3ce844d4d591dc3746cdfff1d2f4e69c445360c58f4374eb88ae` |
| Decompressed original R tar | 109056000 | `1b76060e106cd7f88005e84ebe8c60a6d87d337a3b50fec36ea8282d926009bc` |
| Embedded R manifest | 1608801 | `ee55f9245e9e54e2052392a0c7948c1291d5b44143a9939c0e8f61072d00f8a9` |
| External R packaging receipt | 1228 | `3dcc8497253ac9eca6c30f45acc9279b590ae3db561064139d96e1196897496b` |
| Embedded final R checkpoint | 92189 | `37e7e0df96d513f27db2982b35ce646a65a62a9379309c76ea13b6c941b0e174` |
| Separate C ZIP | 367738 | `df2565f42ad57db5c0c8e77a4c0f926ccbfab02bf83b51065f1c936c5bc90f36` |
| Separate C final receipt | 1876 | `7f9343ceae723f94bd6d9a53efa8fc099b9ba14a84eefcf692b2115ca60024fa` |

The original binary XZ has Git-blob identity `5403b4d4459b4afdf2108fdafc277796ee197c5e`; its binary blob is not claimed as remotely readable. Reversible ASCII storage supports the available UTF-8 readback path. Exact per-part ASCII/binary sizes, hashes and Git blob identities are in `manifest.json`.

## Restore and verify as data only

Run from this archive directory. This verifies decoded bytes and archive manifests and writes a new output directory. It does not execute any archived program, compiler, guest, AST tool, observer or semantic decoder, and does not extract into a checkout.

```python
from pathlib import Path
import base64, hashlib, io, json, lzma, tarfile, zipfile
sha = lambda b: hashlib.sha256(b).hexdigest()
m = json.loads(Path("manifest.json").read_text())
parts, offset = [], 0
for i, p in enumerate(m["chunks"], 1):
    assert p["index"] == i and p["binary_offset"] == offset
    text = Path(p["path"]).read_bytes()
    assert (len(text), sha(text)) == (p["ascii_bytes"], p["ascii_sha256"])
    raw = base64.b64decode(text, validate=True)
    assert (len(raw), sha(raw)) == (p["binary_bytes"], p["binary_sha256"])
    parts.append(raw)
    offset += len(raw)
assert len(parts) == 36
packet = b"".join(parts)
assert (len(packet), sha(packet)) == (m["packet"]["bytes"], m["packet"]["sha256"])
tar = lzma.decompress(packet)
assert (len(tar), sha(tar)) == (m["decompressed_tar"]["bytes"], m["decompressed_tar"]["sha256"])
with tarfile.open(fileobj=io.BytesIO(tar), mode="r:") as archive:
    members = archive.getmembers()
    assert len(members) == len({x.name for x in members}) == 7663
    assert all(x.isfile() for x in members)
    rm = archive.extractfile("PACKET-MANIFEST.json").read()
    assert (len(rm), sha(rm)) == (m["inner_manifest"]["bytes"], m["inner_manifest"]["sha256"])
    entries = json.loads(rm)["entries"]
    assert len(entries) == 7661
    expected = {"r1190_c255_successor/" + e["path"] for e in entries}
    assert {x.name for x in members} == expected | {"PACKET-MANIFEST.json", "PACKET-README.txt"}
    for e in entries:
        raw = archive.extractfile("r1190_c255_successor/" + e["path"]).read()
        assert (len(raw), sha(raw)) == (e["bytes"], e["sha256"])
    rr = archive.extractfile("PACKET-README.txt").read()
    assert (len(rr), sha(rr)) == (m["inner_README"]["bytes"], m["inner_README"]["sha256"])
c = m["separate_C_support"]
encoded = Path(c["encoded_path"]).read_bytes()
assert (len(encoded), sha(encoded)) == (c["encoded_bytes"], c["encoded_sha256"])
c_zip = base64.b64decode(encoded, validate=True)
assert (len(c_zip), sha(c_zip)) == (c["original_zip_bytes"], c["original_zip_sha256"])
with zipfile.ZipFile(io.BytesIO(c_zip)) as archive:
    assert archive.testzip() is None and len(archive.namelist()) == 14
    cm = archive.read("manifest.json")
    assert (len(cm), sha(cm)) == (c["manifest_bytes"], c["manifest_sha256"])
    for e in json.loads(cm)["audit_files"]:
        name = e["path"].split("/c256-r1190-recovery-ci-audit/", 1)[1]
        raw = archive.read(name)
        assert (len(raw), sha(raw)) == (e["bytes"], e["sha256"])
for x in [m["R_packaging_receipt"], {"path": c["receipt_path"], "bytes": c["receipt_bytes"], "sha256": c["receipt_sha256"]}]:
    raw = Path(x["path"]).read_bytes()
    assert (len(raw), sha(raw)) == (x["bytes"], x["sha256"])
out = Path("restored-recovery-packet")
out.mkdir(exist_ok=False)
for name, raw in [(m["packet"]["name"], packet), (m["decompressed_tar"]["name"], tar), ("C-support-ci-audit-packet.zip", c_zip)]:
    with (out / name).open("xb") as f:
        f.write(raw)
```

## Initial archive readback and ref boundary

Each complete remote UTF-8 blob is compared to its exact pinned local ASCII/metadata bytes. Restoration then uses these verified-equal local parts, with whole original XZ/tar equality and all original payload bytes checked. Duplicate on-disk remote-response files are not claimed. Original source files, prior FIRST and the earlier initial-archive temporary-tar stop are left unchanged; recovery restoration uses a new C scratch directory and new output names.

The intended sole new ref is `refs/heads/evidence/r1190-c256-recovery`, created only after full readback and verification. The unchanged three workflows accept push on `main` (normal two) or `r-fast/**` (one preflight); this evidence branch matches none. The C final receipt records the actual commit/tree/ref readbacks and observed Actions counts. Nonarchive paths are compared to fixed parent `52dbe24a`, while main/P/current R may legitimately change under separate Coordinator operations. This archive operation updates no existing ref and creates no PR, tag, merge, Issue post, Actions dispatch/rerun or new semantic run. Root C publishes the canonical archival receipt.

## README aggregate-count correction

The initial archive account above describes the one ref creation at `77b5340673045793e65b6f1f38dbb0a1a5f033fa`, with no update to any existing ref. This README-only follow-up has that immutable archive commit as its sole parent and corrects the CTest aggregate wording: 5 x 757 + 756 = 4541 across the six jobs. The original R packet, C support, original receipts, and all other Git entries remain unchanged. The restoration manifest records the original archive basis and does not pin this outer README, so it is preserved byte-for-byte.

This separate correction authorizes exactly one non-force fast-forward of `refs/heads/evidence/r1190-c256-recovery`, guarded by expected SHA `77b5340673045793e65b6f1f38dbb0a1a5f033fa`. The original `FINAL-ARCHIVE-RECEIPT.json` remains the historical initial-archive receipt; a separate `CORRECTION-RECEIPT.json` records the corrected aggregate, actual update count, changed-blob readback, unchanged-tree checks and observed Actions counts. Neither the original packet nor the old receipt is rewritten.
