# R1179 C245 原証拠 archive

このdirectoryは、[C245 ARCHIVE RELEASE](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6653493272) に基づき、完了したR1179の原証拠を保存するものです。[ARCHIVE START](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6653519905) の後、追加compiler/native/AST生成/build/SDK/test/Actions/PRを実施していません。archiveのserver readback、確定commit/tree、ref作成後のActions0はIssue #1179のARCHIVE FINALに記録します。

## 固定identity

- archive branch: `r-archive/r1179-c245-evidence`。新PRなし。
- sole parent: `9b58f0f4bf7d09619c904adc61b4e40db6294bdb`。
- parent tree: `11f66a5e660d4d59cf3bb599aecad24fb895fa2a`。
- 既存6118 leavesと3 workflowsをmode/blob完全不変に維持し、追加は `evidence/r1179-c245/` 内だけ。
- 測定compiler: `bb8b33cefc030ed99941c9c93fef9c7ef89b0ac0` / tree `d8fd0b0a95184d3449d7e1525ba56574429dbf8f`。
- FINAL実checkout: `2193444841acdac9308cf697e45b0985900f881c`（parents bb8＋9b、tree11f）。
- FIRST通常実checkout: `ebc5ebe1a1eb7718805d6f3f73462784266ec1bb`（parents bb8＋e9、tree `c5cf0df9864e65ec6525dde209f91df133efb7cf`）。
- 元測定branch `r-fast/r1179-phase-selected-current` とPR1183 HEAD9bは変更しない。PR1183はDRAFT/NEVERMERGE。後続main採用は元測定対象を変更しない。

## 保存内容とチェックサム

`MANIFEST.json` はself以外の全追加fileについて、relative path、bytes、SHA256、Git blob SHA-1、modeを列挙します。manifest自体のSHA256とGit blobはARCHIVE FINALのreadback receiptで確定します。`INDEX.json` は全15 jobsのrun/job/attempt/event/head、原log/exportのchecksum、freeze、到達層・未到達層への入口です。

- `raw/logs/`: 15原decoded job logs。UTF-8原bytes（BOMを含むものも保持）を変更していません。
- `raw/exports/`: 12原lossless tar.gz。元base64 exportから復元したgzip bytesをそのまま保存し、再圧縮していません。各 `*.members.json` は元 `evidence-index.json` 自体も含む全memberのbytes/SHA256/tar metadataを記録します。FIRST各161、FINAL各730、合計5346members。
- `raw/formatter/`: 実取得した原ZIP1784bytes、原patch、Rだけの適用patch、GitHub artifact metadata、repair receipt、ZIP全member index。ZIP SHA256は `ec1d8dbcfe028639d03b22af6075ebc4b712737bce928275d022a950f54d0fac`。GitHub reported digestと実bytes hashを区別しています。旧P blockはopaque保存だけで、表示・適用・期待値流用なし。
- `freeze/v1/` と `freeze/v2/`: 原bundle、全17payload/版、元checksum一覧など、保存元の全files。V1 bundle49260bytes / `b1a671dad926d2ad3e57eb33f6c7fb80bb12eab3a7d18b7faf84280c862aa893`、V2 bundle46854bytes / `2dff96cd7271a2b5637c46e7a45ab798fbbf84249b447fdf82657b484a69d680`。V2 oracle27762bytes / `4fe82f7bec6315d85a1c4eb4b41634d0135e2f363c1e38640ae1fa7f9c511cff`。
- `records/`: 原Issue body/metadata（START、freeze、STOP、露出、機材freeze、FIRST、FINAL、C受領、archive release）、job ledger、FIRST/FINAL集約、readback receipt、元6086＋R32のleaf metadata、workflow trigger確認、各decode/保存データ監査receipt。
- `records/C-identity-correspondence.json`: C既監査manifest SHA256 `136f64052ea38e008e5e259cef8badb700b869132f49cafdba6836b7a2577c90` の15log/12export全27identityとR原bytesが一致した対応表。C raw audit receipt `a599099c4bf20f877f21d66eb3756cdb2a894a553b4473f34b03e01a78af32ac` も原bytesで保存。
- `replay/`: 許可された保存済みFIRST C＋既存ASTだけのlocal pure-text replay6回の36原記録。ここだけはarchive用の新しいwrapper gzipであり、原Actions exportとは別物。全member indexあり。入力C/AST自体は原FIRST exportsに保持。
- `machinery/`: 初回immutable333ace59からのobserver/driver/probe bytes。現機材はparent9bの `tests/r1179/` にそのまま継承。全3commit ancestryと機材freezeを保持。
- `tools/`: 保存済みexportを復元・監査したpure-text codeの記録。archive生成はこれらをsemantic testとして再実行しません。

原decoded個別filesは12原gzipから損失なく復元できます。原C、Actions生成AST、passive C、diff、role map、owned JSONL、raw reports、全command/argv/cwd/rc/stdout/stderr/hash、fixed-context、binary hash receiptsを含みます。巨大なdecoded出力の重複commitは行っていません。

## 復元と検証

最初にMANIFESTの全fileをbytes/SHA256で確認し、Git blobは `SHA1(b"blob " + str(size).encode("ascii") + bytes([0]) + file_bytes)` で確認します。manifest自身はIssue ARCHIVE FINALのchecksumと照合してください。

原exportは通常のgzip/tarです。展開前に対応する `job<ID>.members.json` とtarの全member集合、size、SHA256を照合します。全memberはregular fileであり、absolute pathや `..` はありません。各元export内の `evidence-index.json` はself以外を列挙し、外側のwhole-member manifestはそのindex自体も含みます。raw stdout/stderrはdecode/改行変換せずbytesで扱ってください。

V1/V2 bundleの各 `files[]` はfilename、byte_count、sha256、contentを保持します。contentをUTF-8でencodeすれば元payload bytesとなり、末尾LFを含めbyte_count/SHA256と照合できます。原payload実filesも同じdirectoryに保存しています。

これはファイルの復元・checksum確認手順であり、compiler・driver・observer・guest・CTest・Actionsの新実行指示ではありません。native ELF実体は元export外のためこのarchiveに含まず、Actionsが実binary bytesから算出したhash/sizeと実compile/execute receiptsを保存しています。取得ELFの独立再hashという主張はしません。

## FIRSTとFINAL、未到達の保持

[測定FINAL](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6653415834)の結果は変更しません。

- FAST1: strict format RED。build/inventory未到達。原suggestionのR probe3hunk/4箇所だけを修復。
- FIRST通常6: 各7 CHECK/owned/C17、8negative classification後、P01機材recognizerで停止。FULL各719/720、PARTIAL718/719。guest native0、PARTIAL後続CLI skipped。
- FINAL通常6: FULL5各720/720、PARTIAL719/719、各84guest、合計504guest（未改変252＋passive252、guest ASan84/UBSan84を含む）。7sourceを6host lanes/6guest modesで観測した回数。独立source504種類ではありません。
- 全7runs/15actual jobs、attempt1のみ。FAST3/6、通常12/12、manual rerun/dispatch0。元測定でfirst reliable all-green後の追加push/CI0。
- guest GCC13.3.0、FULL guest Clang23.1.3、PARTIAL既存Clang18.1.3。host sanitizer名からguest coverageを推定していません。
- V1は全件未実行。V2は事前scope較正済みの原17payload不変で、結果をfreeze bytesへ書き戻していません。
- whole-loop validatorはNOT_RUN/NO_APPLICABLE_PUBLIC_API。N04は実P8-EXIT-DEPENDENCY到達。N05はUSIZE-ADDITION-PRECISION先行拒否でoverflow固有検査未到達。N07/N08も同precision先行拒否で、canonical legal scope外の分類を保持。
- finite native/value/address/same-bitsを一般soundness、ghost freshness、private authority一般証明へ格上げしません。

独立性: **pre-execution frozen independent cohort / scope-calibrated V2; post-freeze incidental P CI-summary exposure**。許可された配管参照とCの新R機材だけの静的助言、原STOP、既存C/AST pure-text再生を区別し、全期間無限定fresh-BLINDを主張しません。

**audit locality UNMEASURED / GO624 PAUSED / H1 HOLD / P1036–PR1050 STOP** を維持します。このarchiveは新しいlanguage/native検証ではありません。
