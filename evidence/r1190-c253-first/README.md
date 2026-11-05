# R1190 C253 completed FIRST failure packet

This archive preserves the exact completed initial-six packet and separate Coordinator custody evidence. [R's final FIRST checkpoint](https://github.com/wakairo/NewLang_Compiler/issues/1190#issuecomment-6662834282) and [C253 HOLD](https://github.com/wakairo/NewLang_Compiler/issues/1190#issuecomment-6662692408) remain the authority. This is archival custody only; it does not release execution or recovery.

## Fixed identity and observed reach

- Archive commit sole parent / R HEAD: `fe18e136197b25bad77214bc5a9aa000daee3113`.
- Parent tree: `d46cabb7125d544ddc969e840262190226c4eb82`; fixed P base: `7b7095fc5bc187a40ee557b768ae5475c81cdfe7`.
- Actual normal CI test-merge: `7eeba5bd063f2e32aaf403d8ebd6661c33d10b57`, with the same R tree.
- [FULL run 42058011677](https://github.com/wakairo/NewLang_Compiler/actions/runs/42058011677) and [PARTIAL run 42058011336](https://github.com/wakairo/NewLang_Compiler/actions/runs/42058011336) are the completed initial normal runs, not new archival runs.
- All six jobs preserve the original C0 ordinary CHECK precision refusal (rc3, no timeout). R native actual is 0 per job. C0 public-owned/C17/native/observer/comparison are NOT_RUN. The other 12 cases remain CHECK_NOT_RUN at the first-barrier STOP.
- The original source, oracle, expected values, FIRST records and failure classification are unchanged. No new Copy-boundary measurement, later gate outcome, unsafe-acceptance result or adoption PASS is inferred. Historical `execution_released=true` records remain history; current authority is **C253 HOLD**.

## Preserved representations

| Object | Bytes | SHA-256 |
| --- | ---: | --- |
| Exact archived `r1190_C253_INITIAL6_COMPLETE.tar.xz` | 6531876 | `14c6c01ae8c39ced70631c5095592432af17f3ef96e5c5910f7fcb45af82d385` |
| Decompressed tar stream | 27955200 | `1cfa11aa11683fb022bae6eb996d4c302b34cb751e83bb7fe5d59d0aef938aaf` |
| Embedded `PACKET-MANIFEST.json` | 174236 | `433d7e8c37114e382ac0fcc75f56d9457099c231e8d7352ec36c144d8e1bcb73` |
| Original gzip, retained locally unchanged | 11459825 | `1aa8329c52712fd3cc1d1703ce403febf59118b9f3c3dec4210cd5f526597e22` |

The xz and original gzip decompress to byte-identical tar streams. **The original gzip compressed bytes are not stored here, and reconstruction of those original compressed bytes is not claimed.** The exact xz bytes and exact tar bytes are preserved. The tar has 769 unique regular-file members: 768 payloads plus its manifest. Every payload's size, SHA-256 and mode is preserved; no members are trimmed or reclassified.

GitHub connector readback supports UTF-8 text. `chunks/part-0001.base64` through `part-0017.base64` therefore store reversible standard base64, without whitespace or trailing newline. Each of the first 16 parts decodes to 393216 bytes; the last decodes to 240420 bytes. Total encoded chunk bytes are 8709168. `manifest.json` records every part's ordered offset, binary/ASCII size and SHA-256, and Git blob identity.

## Restore and verify, without executing packet code

Run the following Python from this archive directory. It hashes and decodes data only, verifies all embedded payloads, and writes the exact xz after verification. It does not extract or execute archived programs.

```python
from pathlib import Path
import base64, hashlib, io, json, lzma, tarfile
sha = lambda b: hashlib.sha256(b).hexdigest()
m = json.loads(Path("manifest.json").read_text())
parts = []
offset = 0
for index, c in enumerate(m["chunks"], 1):
    assert c["index"] == index and c["binary_offset"] == offset
    text = Path(c["path"]).read_bytes()
    assert len(text) == c["ascii_bytes"] and sha(text) == c["ascii_sha256"]
    data = base64.b64decode(text, validate=True)
    assert len(data) == c["binary_bytes"] and sha(data) == c["binary_sha256"]
    parts.append(data)
    offset += len(data)
packet = b"".join(parts)
assert len(parts) == m["stored_packet"]["chunk_count"] == 17
assert len(packet) == 6531876 and sha(packet) == "14c6c01ae8c39ced70631c5095592432af17f3ef96e5c5910f7fcb45af82d385"
tar = lzma.decompress(packet)
assert len(tar) == 27955200 and sha(tar) == "1cfa11aa11683fb022bae6eb996d4c302b34cb751e83bb7fe5d59d0aef938aaf"
with tarfile.open(fileobj=io.BytesIO(tar), mode="r:") as archive:
    members = archive.getmembers()
    assert len(members) == len({x.name for x in members}) == 769
    assert all(x.isfile() for x in members)
    raw_manifest = archive.extractfile("PACKET-MANIFEST.json").read()
    assert len(raw_manifest) == 174236 and sha(raw_manifest) == "433d7e8c37114e382ac0fcc75f56d9457099c231e8d7352ec36c144d8e1bcb73"
    expected = json.loads(raw_manifest)["members"]
    assert len(expected) == 768
    assert {x.name for x in members} == {x["path"] for x in expected} | {"PACKET-MANIFEST.json"}
    for e in expected:
        entry = archive.getmember(e["path"])
        data = archive.extractfile(entry).read()
        assert len(data) == entry.size == e["bytes"] and sha(data) == e["sha256"]
        assert oct(entry.mode) == e["mode"]
with Path(m["stored_packet"]["name"]).open("xb") as output:
    output.write(packet)
```

## Separate C evidence and custody limits

`c-audit/c-audit-final.json` (9438 bytes, SHA-256 `3e4cbac2cd6b42c23872925e39ed3876fdd0c562f67954c4770f16c0a2a92035`) and `c-audit/raw-custody-audit.json` (7970 bytes, SHA-256 `085dadef4e74b74b8e937a2a0d64f63456969fef9fa3f6c3d09609275d63295f`) are separate C evidence. Their exact bytes are copied beside the packet; they are not inserted into or substituted for its original contents.

The C archive receipt records remote UTF-8 readback of every chunk and metadata blob, reconstruction of the exact xz/tar, all member checks, the immutable commit and its anchor. The only intended anchor is `refs/heads/evidence/r1190-c253-first`, after complete readback. This is an evidence archive, not a P or R implementation branch. Existing workflow push filters are `main` (two normal workflows) and `r-fast/**` (one preflight); the evidence ref matches none. No PR, tag, merge, CI/rerun/dispatch, build or native execution is part of this operation. The parent repository state and all prior archive operations are preserved.
