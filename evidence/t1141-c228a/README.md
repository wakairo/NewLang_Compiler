# T1141 C228a evidence custody

This directory preserves the original artifacts from [Issue #1141](https://github.com/wakairo/NewLang_Compiler/issues/1141) and [PR #1154](https://github.com/wakairo/NewLang_Compiler/pull/1154), under the [C229a four-file custody release](https://github.com/wakairo/NewLang_Compiler/issues/1141#issuecomment-6627583475). The archives were copied byte for byte; neither archive was regenerated. `manifest.json` records their exact sizes and SHA256 values.

The custody commit has the fixed implementation HEAD `06d06fc72d031102632c379bc94f10cd60303d81` as its sole parent (original tree `c6202e23b19df7ce45016647087810f1efbd70ad`). It adds only these four files on `t-archive/t1141-c228a-evidence`. The implementation branch and its existing files/workflows stay unchanged. This archive branch has no PR and is outside all three existing push workflow triggers. Custody performs no formatter/compiler/native/CTest execution or Actions dispatch.

## Original environment and FIRST measurements

The original T task used Debian 13.6/x86_64 and its own checkout, downloads and SDK prefix. It did not use P's active task, worktree, unpublished work or mutable build/cache. The exact curated SDK archive was measured as **126520077 bytes**, SHA256 `5369d292ef513d32758634f98ec7cf62c52f82a0a333eb330ecf9a325648b133`, then safely decoded to the bounded 448040960-byte tar and extracted with unchanged bootstrap helpers. SDK provenance and licenses were retained.

The actual `CLANG_FORMAT` was `/workspace/t1141-c228a/sdk-23.1.3/newlang-llvm-23.1.3-linux-x64/bin/clang-format`, binary SHA256 `16691dbe5bd3462af8acf565b20172db2a9ec683b9db9c86b740e9681947cfa3`. Its version was 23.1.3, upstream commit `0d261d1ca552c95a8f007e061c787ac7132fbcbc`; `ldd` and `--version` exited 0. The curated archive is SHA-locked and unsigned; the original upstream signature does not sign the curated bytes.

| Original FIRST execution | Exit | Preserved observation |
| --- | --- | --- |
| Old script, once | 1 | Reproduced ignored synthetic sentinel deletion plus the live correction; applicable two-file patch. |
| Patched RED, once | 1 | Preserved strict failure; applicable live-only patch, no ignored sentinel/body exposure or empty-file hunk. |
| Patched live correction applied, once | 0 | Strict success, no new patch, empty stdout/stderr. |
| Patched actual invalid-style failure, once | 1 | Real formatter diagnostic and canonical-generation failure; no published patch/BEGIN. |

All four executions preserved source/config/script bytes. The oracle was frozen before execution; harness repairs were 0. The later success does not replace the original RED or formatter failure. The FIRST archive contains **114 regular files**, including `evidence/SHA256SUMS.json`, original harness/staging procedure, source snapshots, oracle, raw status/stdout/stderr, patches and SDK verification records. It contains synthetic inputs, not P/R fixtures or the real old FIRST body. See the [original T result](https://github.com/wakairo/NewLang_Compiler/issues/1141#issuecomment-6627522407).

## Original CI logs

The CI archive preserves the six decoded UTF-8 job logs and their run/attempt/job/checkout metadata and SHA256 manifests. [FULL #41806027477](https://github.com/wakairo/NewLang_Compiler/actions/runs/41806027477) passed 674/674 in each of five jobs; [PARTIAL #41806027339](https://github.com/wakairo/NewLang_Compiler/actions/runs/41806027339) passed 673/673, then its Production CLI exited 0 with empty stdout/stderr. Both were natural `pull_request` attempt 1. The actual CI checkout `1724fdd09bda00e312c0553333d9a9de2d479858` has the same tree as the implementation HEAD. These decoded-log hashes are not hashes of GitHub's original transport ZIP bytes. Custody does not rerun these jobs.

## Restore and limits

First compare both original archives with `manifest.json`. From this directory, Python 3 with `tarfile.data_filter` can restore them into separate new directories:

```python
import hashlib, json, tarfile
from pathlib import Path

manifest = json.loads(Path("manifest.json").read_text())
for item in manifest["archives"]:
    archive = Path(item["filename"])
    assert archive.stat().st_size == item["size_bytes"]
    assert hashlib.sha256(archive.read_bytes()).hexdigest() == item["sha256"]
    destination = Path("restored") / archive.name.removesuffix(".tar.gz")
    destination.mkdir(parents=True, exist_ok=False)
    with tarfile.open(archive, "r:gz") as contents:
        contents.extractall(destination, filter="data")
```

Each restored archive has its own SHA256 manifest. Restoration does not authorize rerunning the harness. SDK binaries and the whole execution environment are not included; the SDK identity is bound to the public locked asset. This is finite formatter evidence and custody, not a language soundness proof, a Debian full-LLVM support claim, or a measured product audit-locality improvement. Audit locality remains UNMEASURED; GO624 PAUSED, H1 HOLD and P1036–PR1050 STOP remain unchanged.

The earlier automatic-review refusal of a large inline Issue payload remains in the historical record. This separately authorized Git branch publication does not retry that inline posting. PR #1154 was moved to Ready by C; T custody does not alter Ready/merge state or the PR HEAD.
