# R1190 C256 immutable initial-six evidence

This is a C custody archive of the completed initial R1190 run on fixed P `f1648a1382ae058e5ab5b9ccdea1e3200879f2af`. The archive commit has **sole parent `1aec4fa49dfb8b2aba83daf503254dd37e687b5d`**, initial R tree `5f6af7cb0c6d8a6b2d8ecad8e29f13e8e17dacae`, and adds only `evidence/r1190-c256-initial/`. It does not add independent R work or change an implementation.

## Frozen checkpoint and limits

The R checkpoint and separate C supporting audit record the initial [FULL 42146445591](https://github.com/wakairo/NewLang_Compiler/actions/runs/42146445591) and [PARTIAL 42146445476](https://github.com/wakairo/NewLang_Compiler/actions/runs/42146445476) as completed/failure, with **4541 CTest completions = 4535 existing P PASS + 6 added R failures**. The initial [FAST 42146092564](https://github.com/wakairo/NewLang_Compiler/actions/runs/42146092564) is included separately.

Every initial normal job records 72 actual native invocations with rc0, 432 total. That does not complete the missing value/identity comparisons: P3 and P5 each have six incomplete passive decodes per job, **72 comparisons NOT_RUN** across the six jobs. The checkpoint records 144 completed original comparisons on the other positive cases. Whole-loop public validation is `NO_APPLICABLE_PUBLIC_API / NOT_RUN`; machine CALL/STORE survival is `NOT_MEASURED`. The PARTIAL standalone Production CLI step after CTest failure is NOT_RUN. No whole-cohort PASS or compiler adoption is inferred here.

This is a **post-finding retarget of independently frozen original inputs and expectations**, not a fresh BLIND cohort. [C's incident adjudication](https://github.com/wakairo/NewLang_Compiler/issues/1190#issuecomment-6674885398) and [Coordinator clarification](https://github.com/wakairo/NewLang_Compiler/issues/1044#issuecomment-6674922903) record incidental P CI archive completion-payload exposure after the frozen inputs and new initial execution. The later clarification confirms **no F completion payload** was displayed. Subsequent work is incident-disclosed, post-finding calibrated continuation. The initial raw and FIRST retain their original timing and bytes.

The current R branch may advance under the separately authorized observer-only recovery. This archive remains fixed at the initial R commit. It contains no live/recovery directory or recovery run results, and adds no diagnosis or technical adjudication beyond the archived R/C records.

## Exact contents

R's immutable selection lists **7244 payload paths / 93223957 bytes**, including prepared-c255 and released-c255 (45 payloads each), authority/allowed/freeze records, the initial FAST, and all six initial raw archives, expanded raw payloads, complete logs, API metadata and custody receipts. Its selection manifest is included unchanged as an additional metadata file. The R checkpoint was authored after the findings; it summarizes the initial raw without changing it. Its static-proof/incident-document hash references are reference-only and those separate post-finding files are not silently included.

Two additional files are the unchanged C supporting ZIP and FINAL-RECEIPT. Two C canonical incident-comment snapshots are serialized separately. The packet therefore has **7249 data members plus PACKET-MANIFEST.json = 7250 regular tar members**. Every payload size/SHA-256/mode is listed and verified.

The C support ZIP is 365255 bytes, SHA-256 `5e6ef61064fc7ec62d1defa4d2a5af80b8a0e7fb988a6cda08ab8fc7827e7995`. It is separate C evidence and does not contain the R raw exports. Its 17 entries include retained first apparatus/draft materials and the final supporting audit. All 64 referenced R inputs are present in this packet. Its seven P-baseline dependencies remain reference-only at the [immutable C255 archive](https://github.com/wakairo/NewLang_Compiler/tree/6e3868f58e036f3ff744da8b32b7a854b5cb112b/evidence/c255-pr1191-ci); producer payloads are not copied into the R selection or treated as R's prior knowledge.

The original FIRST26 is byte-identical in prepared/released copies, every initial raw export, and the original FIRST in the [immutable C253 archive](https://github.com/wakairo/NewLang_Compiler/tree/e7ab5e9d8c10a6929d2e309a993faa5c3f5f4116/evidence/r1190-c253-first). That complete predecessor bundle is not duplicated here. Its original finding is not relabeled. All six raw archives retain 1178 members each, including their 1177-entry MEMBER-INDEX; all nested size/hash values and expanded-copy bytes were checked.

## Packet identities

| Object | Bytes | SHA-256 |
| --- | ---: | --- |
| Exact new custody packet, XZ | 13997520 | `d6474b2aaba8fff8cf82b747f43cbf0c98cd7b08bf4f536299cedf7b76943e9b` |
| Decompressed tar | 106004480 | `69aab68ceb55718cfe4c69fcbe00638d5cd3cd5ca9bd42f6139aa77c8f3baf99` |
| Embedded PACKET-MANIFEST.json | 2121108 | `1893e49bf9dc1a25fdd8bad4562699f0af0844963ebe2e4d4610065533f9ab84` |
| R selection manifest | 1519372 | `fabbe6f0e069ca4256bd3f0d697e9700de806e6a717d9d87541ebf571e66cac4` |

The new outer tar uses sorted regular-file entries and normalized uid/gid/mtime zero; source file modes are recorded. Original selected file bytes and nested original archive bytes are unchanged. XZ uses LZMA2 preset6, dictionary64MiB and CRC64. Git stores 36 standard-base64 parts without whitespace/newlines, totaling 18663360 ASCII bytes. Part offsets and both binary/ASCII size/hash identities are in the adjacent restoration manifest.

## Restore and verify as data only

From this archive directory, the following uses only byte decoding, decompression and manifest hashing. It does not execute an observer, decoder, AST tool, compiler or guest, and does not extract files into a live checkout.

```python
from pathlib import Path
import base64, hashlib, io, json, lzma, tarfile
sha = lambda b: hashlib.sha256(b).hexdigest()
m = json.loads(Path("manifest.json").read_text())
parts, offset = [], 0
for i, p in enumerate(m["chunks"], 1):
    assert p["index"] == i and p["binary_offset"] == offset
    text = Path(p["path"]).read_bytes()
    assert len(text) == p["ascii_bytes"] and sha(text) == p["ascii_sha256"]
    raw = base64.b64decode(text, validate=True)
    assert len(raw) == p["binary_bytes"] and sha(raw) == p["binary_sha256"]
    parts.append(raw)
    offset += len(raw)
packet = b"".join(parts)
assert len(parts) == 36
assert len(packet) == m["packet"]["bytes"] and sha(packet) == m["packet"]["sha256"]
tar = lzma.decompress(packet)
assert len(tar) == m["decompressed_tar"]["bytes"] and sha(tar) == m["decompressed_tar"]["sha256"]
with tarfile.open(fileobj=io.BytesIO(tar), mode="r:") as archive:
    members = archive.getmembers()
    assert len(members) == len({x.name for x in members}) == 7250
    assert all(x.isfile() for x in members)
    raw_manifest = archive.extractfile("PACKET-MANIFEST.json").read()
    assert len(raw_manifest) == m["inner_manifest"]["bytes"]
    assert sha(raw_manifest) == m["inner_manifest"]["sha256"]
    entries = json.loads(raw_manifest)["members"]
    assert len(entries) == 7249
    assert {x.name for x in members} == {x["path"] for x in entries} | {"PACKET-MANIFEST.json"}
    for e in entries:
        member = archive.getmember(e["path"])
        raw = archive.extractfile(member).read()
        assert len(raw) == e["bytes"] and sha(raw) == e["sha256"]
        assert oct(member.mode) == e["mode"]
with Path(m["packet"]["name"]).open("xb") as out:
    out.write(packet)
with Path(m["decompressed_tar"]["name"]).open("xb") as out:
    out.write(tar)
```

## Remote custody and ref boundary

Each complete remote UTF-8 blob is compared to its exact pinned local ASCII/metadata file. Reconstruction then uses those verified-equal local parts and checks the whole XZ/tar and every payload against the original fixed snapshot. Duplicate on-disk remote-response files are not claimed. The C final archive receipt records the complete proof chain, immutable commit, tree and anchor.

The only intended new ref is `refs/heads/evidence/r1190-c256-initial`, after full readback. Existing workflow push filters are main (normal two) and r-fast/** (one preflight); this archive ref matches none. All nonarchive tree entries are compared against the initial R tree, not the advancing recovery branch. Main/P/old R1194/old archive refs are not updated. This operation performs no PR/tag/Actions/rerun/dispatch or compiler/guest/build/AST/observer/semantic-decoder execution and does not intervene in R repair. Root C publishes the receipt.
