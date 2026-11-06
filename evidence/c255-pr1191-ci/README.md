# C255 PR1191 accepted CI evidence

[C acceptance](https://github.com/wakairo/NewLang_Compiler/issues/1188#issuecomment-6674429573) covers the final natural CI of fixed P HEAD `f1648a1382ae058e5ab5b9ccdea1e3200879f2af`, tree `1a4fcf50e76b0d2def314fdf7720f88d52a6581c`. This evidence commit has that HEAD as its sole parent and adds only `evidence/c255-pr1191-ci/`.

The exact original packet contains 29 entries, including all 12 C254 whole-job logs, their raw transport/custody receipts, audit results, authority snapshots and fixed source/evidence files. The original six RED logs and the final six PASS logs are both retained without alteration or reclassification.

## Original packet identities

| Object | Bytes | SHA-256 |
| --- | ---: | --- |
| `ci-audit-packet.zip` | 742136 | `38297aa4f13ca6d0d12ba9bcc83bc50b09812c29d393ac9281d0b4d1d5b1d4f0` |
| ZIP entry `audit-final.json` | 132782 | `c66d8773737d53e754b9a982ca5a280a27a6cb1aef84ef5af67c8e63144a6f2d` |
| ZIP entry `manifest.json` | 4690 | `c0e7e7fc25b4c5e5027f56d7bf7d1f5ba9cc766c0c45b731f44f074815cb848d` |

The original ZIP Git blob identity is `f71b5d483a3e7bffccd99b808347c97d04386c88`. It is represented by two standard-base64 ASCII files, each without whitespace or a trailing newline. Their decoded sizes are 393216 and 348920 bytes; their ASCII sizes are 524288 and 465228 bytes. The adjacent restoration `manifest.json` records both parts and all 29 original entry size/SHA-256/CRC values; it is distinct from the unchanged manifest inside the ZIP.

## Restore and verify

This Python performs data custody only. It verifies the two parts, exact ZIP identity, CRC and every entry before writing the ZIP; it does not extract or execute archived programs.

```python
from pathlib import Path
import base64, hashlib, io, json, zipfile
sha = lambda b: hashlib.sha256(b).hexdigest()
m = json.loads(Path("manifest.json").read_text())
parts, offset = [], 0
for i, c in enumerate(m["chunks"], 1):
    assert c["index"] == i and c["binary_offset"] == offset
    text = Path(c["path"]).read_bytes()
    assert len(text) == c["ascii_bytes"] and sha(text) == c["ascii_sha256"]
    data = base64.b64decode(text, validate=True)
    assert len(data) == c["binary_bytes"] and sha(data) == c["binary_sha256"]
    parts.append(data)
    offset += len(data)
packet = b"".join(parts)
assert len(parts) == 2 and len(packet) == 742136
assert sha(packet) == "38297aa4f13ca6d0d12ba9bcc83bc50b09812c29d393ac9281d0b4d1d5b1d4f0"
with zipfile.ZipFile(io.BytesIO(packet)) as z:
    assert len(z.infolist()) == len(set(z.namelist())) == 29
    assert z.testzip() is None
    assert set(z.namelist()) == {e["path"] for e in m["zip_entries"]}
    for e in m["zip_entries"]:
        data = z.read(e["path"])
        assert len(data) == e["bytes"] and sha(data) == e["sha256"]
        assert f'{z.getinfo(e["path"]).CRC:08x}' == e["crc32"]
with Path("ci-audit-packet.zip").open("xb") as output:
    output.write(packet)
```

## Preserved finite results

- Final [FULL 42140798746](https://github.com/wakairo/NewLang_Compiler/actions/runs/42140798746): five jobs, each 756/756 actual PASS.
- Final [PARTIAL 42140798935](https://github.com/wakairo/NewLang_Compiler/actions/runs/42140798935): 755/755 actual PASS, then actual Production CHECK exit0 with empty stdout/stderr.
- Original [FULL 42137732085](https://github.com/wakairo/NewLang_Compiler/actions/runs/42137732085) and [PARTIAL 42137732156](https://github.com/wakairo/NewLang_Compiler/actions/runs/42137732156): 4529 PASS plus 6 failures across 4535 completions. Each sole failure is the unchanged P1062 exact manifest-byte guard. The original PARTIAL standalone CLI remains NOT_RUN.
- All final checkouts are `67781fdc203fac04f3f01499fd56ed4a468589f2`, ordered parents main `70059189c2030f927ad3daa0044c8898848b4548` then fixed P HEAD, with the same final tree.

## Custody and authority limits

Every stored blob is fetched through supported UTF-8 and compared as a complete string with its pinned local file. Whole ZIP and member verification then uses those verified-equal local parts; a second on-disk copy of each remote response is not claimed. The original packet remains unchanged.

The logs are connector-decoded whole-job UTF-8 text with their returned BOM/newlines preserved. Original GitHub binary ZIP identity and success-suppressed per-guest worktree artifacts are not claimed. This supporting C evidence does not replace independent F/R or substantive adoption.

The intended retained ref is `refs/heads/evidence/c255-pr1191-ci`, created only after complete readback and commit verification. The three existing workflows have push filters `main` (normal two) and `r-fast/**` (preflight); the evidence ref matches none. No Production/source/oracle change, PR/tag/merge, CI/rerun/dispatch, build/compiler/native execution or Issue write is part of this archival operation. Other contributors' separate R branch activity is outside this archive's operation counts.
