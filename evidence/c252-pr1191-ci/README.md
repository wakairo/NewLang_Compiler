# C252 — PR1191 修復HEADの自然CI監査証拠

[C受理（Issue #1188 / C252）](https://github.com/wakairo/NewLang_Compiler/issues/1188#issuecomment-6662227553)に対応する、C補助者によるread-only監査の保存です。元ZIPのbytesは変更していません。

## 固定identity

- CI候補: `7b7095fc5bc187a40ee557b768ae5475c81cdfe7`
- 候補tree: `f0aebb423a1c38908fde83f219886cf387b3705e`
- 全6jobの実checkout: `2fe005369249d666c8a1cbef7e7b83005f21f539`
- test-mergeの順序付きparents: `70059189c2030f927ad3daa0044c8898848b4548` + `7b7095fc5bc187a40ee557b768ae5475c81cdfe7`
- test-merge treeは候補treeと一致。
- 本証拠commitのsole parentは上記候補7b7095。追加はこのREADMEと `ci-audit-packet.zip.base64` の2 filesだけ。Production、既存証拠、workflowsを変更しない。

## 保存物

| 対象 | bytes | SHA256 |
| --- | ---: | --- |
| 復元される `ci-audit-packet.zip` | 384015 | `4daadbaf9996e4c1729d4e03c2aec2ecbb8c9848b6976ee34a8afdb3bb7f5bab` |
| 保存する `ci-audit-packet.zip.base64` | 512021 | `06a90b98f13ed97a428583a2297e960d54e3d02727530ac000e18f36cc24bc2b` |
| ZIP内 `manifest.json` | 3090 | `594b40bdaa004b9197105d17fa4056ab0dab0e9b1910bb6604a978292b9cf54f` |
| ZIP内 `audit-final.json` | 221099 | `6ead4500bb28acae79552c50092fc13de9b91480c42dfe685ef0c3d999a01d93` |

ZIPのGit blob identity: `5baec069d6b7f7d0bc0d52e0800389fbeb8b5f65`。

ZIPは19 entries。6 decoded whole-job logs、固定CMake/runner/oracle/LOCAL-COMPLETION、GitHub初回・予算・最終read receipts、ASan未完時のreceipt、最終audit、再解析用Python、manifestを含む。manifestは自身とZIPを除く18 filesのsize/SHA256を列挙する。展開内容をGitへ重複保存しない。


## ASCII保存と復元

保存ファイルは標準base64を1行で表した512020文字と末尾LF1 byte。encoded-text Git blobは `fc1b6322da38bd783dd450850f874a509194fd16`。ASCII blobをGitHub connectorのUTF-8経路で読み戻し、保存文字列の完全一致、復元ZIPの元bytes完全一致、上記SHA256/Git blob、19 unique entries、CRC、manifest全18件のsize/SHA256を確認した。

保存ファイルのディレクトリで、次のcustody操作だけで元ZIPを復元できる（compiler・test・CIは実行しない）。

```python
from pathlib import Path
import base64, hashlib
text = Path("ci-audit-packet.zip.base64").read_bytes()
assert len(text) == 512021 and text.endswith(b"\n") and text.count(b"\n") == 1
assert hashlib.sha256(text).hexdigest() == "06a90b98f13ed97a428583a2297e960d54e3d02727530ac000e18f36cc24bc2b"
packet = base64.b64decode(text[:-1], validate=True)
assert len(packet) == 384015
assert hashlib.sha256(packet).hexdigest() == "4daadbaf9996e4c1729d4e03c2aec2ecbb8c9848b6976ee34a8afdb3bb7f5bab"
with Path("ci-audit-packet.zip").open("xb") as output:
    output.write(packet)
```

### 初回binary readback STOPと解消範囲

最初のbinary ZIP blob作成は成功し、返されたSHAは上記 `5baec069d6b7f7d0bc0d52e0800389fbeb8b5f65` と一致した。しかしbinaryを読む `github_fetch` は `HTTPError: 400: GitHub Fetch only accepts UTF-8 text. (Response: None)`、`github_fetch_blob` は `UnicodeDecodeError: 'utf-8' codec can't decode byte 0xca in position 10: invalid continuation byte` で失敗し、その時点ではtree/commitを作成せず停止した（2026-11-05 15:44:45 UTC）。

その初回binary blobを直接読み戻したとは主張しない。承認されたASCII表現への変更後に、remote ASCII readbackから元ZIPを完全復元した。元ZIPと初回 `STOP-RECEIPT.json` は変更せずローカルに保持している。初回STOP receiptは3340 bytes、SHA256 `b75649f29b57b759d63f3f83005471faad778d163efd1b9d93c663c54129a731`。この変更は表現・custodyのみで、CI証拠内容の変更や新CIではない。

## 元runと有限結果

- [FULL 42053160323](https://github.com/wakairo/NewLang_Compiler/actions/runs/42053160323)
- [NO-LLVM PARTIAL 42053160483](https://github.com/wakairo/NewLang_Compiler/actions/runs/42053160483)

両runはpull_request / attempt1 / completed-successで、候補HEADが一致する。

| job | job ID | 実CTest PASS |
| --- | ---: | ---: |
| GCC Release | 127334342242 | 746/746 |
| GCC | 127334342261 | 746/746 |
| UBSan | 127334342312 | 746/746 |
| Clang | 127334342361 | 746/746 |
| ASan | 127334342386 | 746/746 |
| NO-LLVM PARTIAL | 127334342580 | 745/745 |

全jobでID/name/ordinal一意、fail/timeout/NotRun/skip/sanitizer診断0。FULL5の名前集合は一致し、PARTIALとの差はllvm.c_apiのみ。required12、新C250a2件、signature-origin2件は全jobで実PASS。PARTIAL後段の実Production CLIはexit0、stdout/stderr空。最終ASanは2026-11-05 15:36:28 UTC（11/6 00:36:28 JST）に完了した。

予算は旧8585の自然6 + 修復7b7095の自然6 = 12/12 actual jobs、全branch run4、各attempt1、observed manual0。最初の信頼できる修復all-greenで追加P push/CI停止。

## 境界と保持方法

- 取得したのはconnectorが返したdecoded UTF-8 whole-job text。元GitHub ZIPのbyte-identical保存とは称さない。
- CTestが成功時に省略する個別guest観測・binary・trial stdoutはCI作業領域から取得していない。runnerが要求する契約と実CTest PASSを照合したもの。
- Pのlocal測定はP証拠として区別する。独立R/F、一般soundness、Product GO、main採用の承認を代替しない。
- 今回の保存はGit blob/tree/commit objectsのみ。ref/branch/tag、push、PR、Actions、compiler/native再実行、Issue投稿を行わない。
- refによる到達可能性・保持は設定していない。commit SHAによる読取り照合を記録し、長期retentionはこのobject-only操作だけでは保証しない。
