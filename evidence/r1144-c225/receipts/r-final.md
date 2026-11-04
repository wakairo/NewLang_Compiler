# R1144 FINAL — bounded finite observations PASS / DRAFT・NEVERMERGE

記録時刻: **2026-11-04T14:37:47.318+09:00（JST）**。GitHub専用branch＋ActionsによるR作業を完了。修復後FULL5＋NO-LLVM PARTIALの**全6実jobがcompleted/success**、最後のASanも675/675 PASS。必要な有限観測が成立したため、ここで全push・CIを停止する。Rの結論は以下の限定範囲のPASSであり、採用・READY・mergeの裁定ではない。

## 固定対象 / 完成物

- fixed main: `acdbf91944a3bac99671d71c6ae23134b56d6c32` / canonical Draft17.34。
- fixed P parent/base: `773643d2709a5e5db24650069d67c0d9032fefb0` / P tree `3d230802ca275bd963101c9b575f1567927f5c7a`。最終照合でもmain/Pは不変。
- final R HEAD: [`5f4bde2f74ffe55c90c5c2f64220ecd634daf658`](https://github.com/wakairo/NewLang_Compiler/commit/5f4bde2f74ffe55c90c5c2f64220ecd634daf658)。tree: `809b189db90e2b4b6ddceb40eeba65b9baa7996e`。
- branch: `r-fast/r1144-loop-result-consumer`。PR [#1145](https://github.com/wakairo/NewLang_Compiler/pull/1145) はopen / **DRAFT / NEVERMERGE**、base `p-c223-loop-result-consumer`、merged=false。
- 実PR checkout: `18055bb1a64d82787d0a86118976ac97f1358f9f`、checkout treeは上記R treeと一致。全修復jobのrawにpr_base/pr_head/GITHUB_SHA/ref/event/attemptを保存し照合済み。
- Production・canonical・public API/ABI・workflow・toolchain・既存source/oracleは変更しない。変更はR所有機材と最小CMake/CTest接続のみ。local compiler/native/formatter/build/testは実行していない。

## 凍結と情報境界

[V1の実行前完全bytes/SHA凍結](https://github.com/wakairo/NewLang_Compiler/issues/1144#issuecomment-6622634318)を保持。V1は**NOT_RUN**、初期oracleを成功に合わせて変更していない。canonical legalityと固定v0 CLI/source-profileの対応を区別し、main()->unitおよび対応比較形式への構造調整を別versionにした。

[C承認](https://github.com/wakairo/NewLang_Compiler/issues/1144#issuecomment-6622697245)後の[V2実行前完全bytes/SHA凍結](https://github.com/wakairo/NewLang_Compiler/issues/1144#issuecomment-6622708609)を使用。V2は同一のinner trace / Break / Copy / actual / selected期待値を保持し、unit transport差とSUB guardの等価表現差を明記した。固定g+1（1,12,18）は表現可能。V1/V2の26凍結ファイルと24source-oracle SHAリンクを保持。

最終の限定独立性ラベル:

> pre-execution frozen independent cohort / post-freeze interface-calibrated V2 / incidental old-P formatter exposure after freeze / incidental old-P registration exposure after FIRST execution

formatter旧P断片露出は[6622971928](https://github.com/wakairo/NewLang_Compiler/issues/1144#issuecomment-6622971928)、初回実行後の旧P隣接CMake登録断片露出は[6623173239](https://github.com/wakairo/NewLang_Compiler/issues/1144#issuecomment-6623173239)でC adjudication ACCEPT。閲覧時系列を残し、全期間無限定BLINDや新しい独立cohortとは称しない。FIRST rawの当時の3節ラベルを遡及変更せず、修復rawに4節ラベルを記録した。

## 初回REDと唯一の通常機械修復

初回normal HEAD `a835122ead5285cb4496d216814180e58fde48b0` / tree `a4d4cfdbd56e5093b33bee9017b5b3dcdfb0b2e2` / 実checkout `632225766d01587d73467e7f7612086b1e3af7e5`。FULL run [41776901686](https://github.com/wakairo/NewLang_Compiler/actions/runs/41776901686)、PARTIAL run [41776901796](https://github.com/wakairo/NewLang_Compiler/actions/runs/41776901796)、いずれもattempt1。

初回6jobはすべて最終RED。FULL各675実行/666PASS/9FAIL/skip0、PARTIAL674/665PASS/9FAIL/skip0。9失敗は間接旧scriptが現在のCMakeLists.txtを元freeze digestと比較したR登録機材接続の不足。初回R12ケース自体は全6jobでPASSであり、旧native成功の後にfreezeが失敗した場合も、nativeの実成功とテスト全体REDを分けて保持する。PARTIAL後段Production CLIステップは初回NOT_RUN。

[分類と修復計画](https://github.com/wakairo/NewLang_Compiler/issues/1144#issuecomment-6623149015)、[具体的機材契約](https://github.com/wakairo/NewLang_Compiler/issues/1144#issuecomment-6623208816)、[FAST4後checkpoint / FIRST保全 / ref-less修復候補](https://github.com/wakairo/NewLang_Compiler/issues/1144#issuecomment-6623260005)を保存済み。

修復は正確な9登録コマンド（7旧script）の実行先をRの認証済みprivate projectionへ接続した。3338保護ファイルとcurrent manifestのbytes、実checkout indexのstage0/mode/blob OIDを検証し、CMakeだけを既に凍結した親bytesへ投影する。private Git indexは実indexから導出して当該CMake OIDのみ置換し、readback一致を確認する。旧validator/assertion/入力はそのまま実行し、通常の未追跡ファイルnegativeも維持する。argv/cwd/compiler/probeパスを保持し、必要なp990 source directory引数だけ投影先へ写像、対象familyはprofile0のみ。失敗をnonzeroで伝播し、hash waiver・例外bypassは設けない。

これにより全9旧登録gateが最終6jobでPASS。修復によるsource/oracle/Productionの変更はなく、Rの生成C・owned観測・positive trace・negative第一拒否層は全jobでFIRSTと一致する。

### FIRST raw custody（REDを保持）

| job | job link | executed / pass / fail / skip | envelope UTF-8 bytes | envelope SHA256 |
|---|---|---:|---:|---|
| GCC | [126399253443](https://github.com/wakairo/NewLang_Compiler/actions/runs/41776901686/job/126399253443) | 675 / 666 / 9 / 0 | 93617 | `4fd9548b5f41ee8ac9dc74c7378fe318e4cb2215501604e25c18bcaf11be9fa6` |
| GCC Release | [126399253157](https://github.com/wakairo/NewLang_Compiler/actions/runs/41776901686/job/126399253157) | 675 / 666 / 9 / 0 | 93619 | `e46b3cbc0bac294717dfbbf1a0155722deb37af1b4f1e9c03c7b0e70ffad1db0` |
| Clang | [126399253432](https://github.com/wakairo/NewLang_Compiler/actions/runs/41776901686/job/126399253432) | 675 / 666 / 9 / 0 | 98129 | `e7b2edbe896fe5f63af5940d779ffb19c64b57d789041cfa2474840734c74db3` |
| ASan | [126399253348](https://github.com/wakairo/NewLang_Compiler/actions/runs/41776901686/job/126399253348) | 675 / 666 / 9 / 0 | 99932 | `a375ab1316dccef6f519485ece6bf50360d2f90f058987530e440093af263fd0` |
| UBSan | [126399253540](https://github.com/wakairo/NewLang_Compiler/actions/runs/41776901686/job/126399253540) | 675 / 666 / 9 / 0 | 99982 | `8e0c751a5d220f3c9638cde8cead9164b5aa98157ffaff667cd054b0805bee29` |
| PARTIAL | [126399253663](https://github.com/wakairo/NewLang_Compiler/actions/runs/41776901796/job/126399253663) | 674 / 665 / 9 / 0 | 94474 | `7e6c8f11d894389cf179f191781bb24b0b7b566ca9bb7b8cc96f90d48a29bdf2` |

## 最終実測 / 全6job

FULL [41778689950](https://github.com/wakairo/NewLang_Compiler/actions/runs/41778689950) と PARTIAL [41778689935](https://github.com/wakairo/NewLang_Compiler/actions/runs/41778689935) はともに自然pull_request、attempt1、fixed R HEAD `5f4bde2f74ffe55c90c5c2f64220ecd634daf658`。

| job | job link | executed / pass / fail / skip | envelope UTF-8 bytes | envelope SHA256 |
|---|---|---:|---:|---|
| GCC | [126404947588](https://github.com/wakairo/NewLang_Compiler/actions/runs/41778689950/job/126404947588) | 675 / 675 / 0 / 0 | 93680 | `5d1ee63b57d9277337cc442f0615be732a1740ae8117727e1ae129a030a961a6` |
| GCC Release | [126404947760](https://github.com/wakairo/NewLang_Compiler/actions/runs/41778689950/job/126404947760) | 675 / 675 / 0 / 0 | 93682 | `b929e656a88c9321a6429fa1e4e3b87dc85d6f91d5b9751976f01d2544f169fb` |
| Clang | [126404947403](https://github.com/wakairo/NewLang_Compiler/actions/runs/41778689950/job/126404947403) | 675 / 675 / 0 / 0 | 98192 | `acda923fcf18af570e0588ff6384bafeedd410484118b82276cc572571b6fce4` |
| ASan | [126404947575](https://github.com/wakairo/NewLang_Compiler/actions/runs/41778689950/job/126404947575) | 675 / 675 / 0 / 0 | 99995 | `2e1898cb83d63f37cecc2d420b2f6fa9cd312e7c3c2312e522a43ea70ef60a29` |
| UBSan | [126404947717](https://github.com/wakairo/NewLang_Compiler/actions/runs/41778689950/job/126404947717) | 675 / 675 / 0 / 0 | 100045 | `3bf2eb7998f552b03ce7155393c46afb0f1da05c7b1bc69f05d142be1195a48f` |
| PARTIAL | [126404947468](https://github.com/wakairo/NewLang_Compiler/actions/runs/41778689935/job/126404947468) | 674 / 674 / 0 / 0 | 94537 | `d4d9c6bbe06d66fcd6f9b011e853d0d32a642089dbe71be43a97d166ee8c17e3` |

各jobのR envelopeをBEGIN/DATA/ENDから復元し、宣言byte数/SHA256、内部73file recordsのbytes/SHA256、81コマンド記録を照合済み。最終438 file records、FIRSTも438、合計876（重複を含む実記録数）。生成C、unchanged guest/passive C、owned結果、stdout/stderr/診断を完全envelopeに保存。上表のjob logsから回収可能。別途Cのデータ保管担当へ委譲済みであり、その完了をR実行の追加条件にはしない。

FULLはGCC13.3.0 Debug/Release、Clang23.1.3、Clang23.1.3 ASan/UBSan。PARTIALはGCC13.3.0 frontend-only / NO LLVM。PARTIAL後段の実Production source/checker CLI検証ステップも最終success（初回NOT_RUNと区別）。

### 正例の実値

source→parser/CHECK→真正なowned→C17→nativeを追跡した6例。各phase列はpassive observerが読んだ実current値。Break→外binding→ordinary Copy→callee actualは表の共通値で全て一致。

| ID | phase/current | Continue回数 | Break = outer = Copy = actual | source selected | predicate branch |
|---|---|---:|---:|---:|---|
| A0 | 31 | 0 | 31 | 29 | false |
| A1 | 13 → 17 | 1 | 17 | 83 | true |
| A3 | 5 → 9 → 13 → 17 | 3 | 17 | 83 | true |
| S0 | 11 | 0 | 11 | 29 | false |
| S1 | 20 → 17 | 1 | 17 | 83 | true |
| S3 | 29 → 25 → 21 → 17 | 3 | 17 | 83 | true |

全6jobで各正例を-O0/-O2。最終のunchanged generated guestは72実行、別passive観測は72実行、計144。FIRSTも同数で、両group合計288 native実行。各groupのASan/UBSanはそれぞれunchanged guest12＋passive12実行に到達し、全て成功。unchanged guestのexit0だけから値の一致を主張せず、値の経路は別passive traceで検証した。observerは生成guest Cの既存行を保持し、値・制御・identityを生成する処理を入れていない。

真正なownedではsource-issued、header inductive、Continue1/Break1/Return0、scalar private relation、binding unitと実typed receiver/initializer/result、ordinary Copy、actual Copyを検証。direct_break_provenanceは全例falseという観測をそのまま記録する。predicate branchは別々のsource-selected値29/83から推論したもので、branch条件の独立直接計測とは区別する。

### 生成C SHA256（FIRSTと最終、全compiler/jobで一致）

| ID | generated C SHA256 |
|---|---|
| A0 | `63e6c1d92830f5635d31215a983eb698e61db5e609df52eb3b7a35d66cbcece0` |
| A1 | `519d59510054ffe57a815bf37d200162ce2f26fc90de2ad3e3d83bd6be87dc9e` |
| A3 | `e998db38a1b12ebc63978263afb7121e53599a3895815724b993ccbe8ae7f1b1` |
| S0 | `0a3daeb0b200c1dec1cd19ea387dc42234972dcce5df11a1c2a7df8c35902997` |
| S1 | `dbeb7ff1c3dfa50986ad8e0583087b44a64f8c216c0293472020264e06aa8ecb` |
| S3 | `41341e999afc0b3fc2db12a880e0021ad36ce5131e65e55b33943ac961878364` |

### 負例の第一拒否層と未到達範囲

全6例はCHECK/registrationまで、C17/nativeは凍結方針どおりNOT_RUN。各実status/codeを保存し、拒否を一律に意味エラーやprecisionと呼び替えていない。

| ID | canonical区分 / 狙い | actual status | actual code | 有限結論 |
|---|---|---|---|---|
| N1 | invalid / guard不足・underflow | ANALYSIS_PRECISION_LIMIT | USIZE-SUBTRACTION-PRECISION | 先行precision拒否。狙ったunderflow診断への到達は未観測 |
| N2 | invalid / carry型 | SEMANTIC_ERROR | P14-CONTINUE-TYPE | carry型拒否を観測 |
| N3 | invalid / carry arity | SEMANTIC_ERROR | P14-CONTINUE-ARITY | carry arity拒否を観測 |
| N4 | invalid / u8 actualとusize parameter | ANALYSIS_PRECISION_LIMIT | USIZE-ADDITION-PRECISION | 先行precision拒否。後段actual型拒否への到達は未観測 |
| N5 | invalid / cursor scope外参照 | SEMANTIC_ERROR | P3-UNKNOWN-BINDING | scope解決拒否を観測 |
| N6 | canonical-legal [2,0] / precision拒否を許容 | ANALYSIS_PRECISION_LIMIT | USIZE-SUBTRACTION-PRECISION | 許容した有限precision拒否。invalid programとは判定しない |

N6の機材判定はactual precision status＋限定precision codeに接続済みで、任意のSEMANTIC_ERRORをprecision PASSへ変換しない。N1/N4/N6の先行拒否は後段loan/barrierやnative安全性の到達証拠ではない。

## 全期間予算台帳

| FAST actual run | run / job | R HEAD | 結果 |
|---:|---|---|---|
| 1 | [41775881087](https://github.com/wakairo/NewLang_Compiler/actions/runs/41775881087) / 126396000054 | `786d0d9ec638ffad7150f88515ed436cf979392e` | failure |
| 2 | [41776114561](https://github.com/wakairo/NewLang_Compiler/actions/runs/41776114561) / 126396743803 | `e8b51186d54c7781ff954dc4a8c20e326c3def6c` | success |
| 3 | [41776347672](https://github.com/wakairo/NewLang_Compiler/actions/runs/41776347672) / 126397505248 | `f8d700bfe98683330fb3c88e8acb6ba197594794` | failure |
| 4 | [41776609675](https://github.com/wakairo/NewLang_Compiler/actions/runs/41776609675) / 126398330845 | `a835122ead5285cb4496d216814180e58fde48b0` | success |
| 5 | [41778684244](https://github.com/wakairo/NewLang_Compiler/actions/runs/41778684244) / 126404929776 | `5f4bde2f74ffe55c90c5c2f64220ecd634daf658` | success |

FAST **5/6 actual runs**、すべて自然push、attempt1。最初の完成commitをrefなしで作り、そのcommit直指しで初回refを作成した。FAST1/3のformatter REDと初回probe HEADを保持。FAST4はPR前同HEAD、FAST5は最終同HEADについて実event/ref、pinned clang-format23.1.3、GCC frontend-only build、ctest -Nの一意なR登録（672総登録、今回R1）を確認。inventory SHA256 `938b79cefcf48d8c8bffbb8fb5c1d606182e5d76c5868c7cfd5dfccdecc01770`。FASTはsemantic/native実測0。FAST5前checkpointは6623260005。

normal **12/12 actual jobs** = FIRST FULL5＋PARTIAL1（REDを保持）＋唯一の機械修復FULL5＋PARTIAL1（全success）。manual rerun/dispatch **0**。branch1、DRAFT PR1。全branch期間9workflow runs（FAST5＋normal4）、全attempt1、queued/cancelledの未計上なし。最初の信頼できる全6job成功後、追加push/CIは停止。

## 有限評価 / VALUE-ALIGNMENT-RISK

Rは、凍結した6正例のguarded scalar carry profileについて、実際の正常Break値が外binding→ordinary Copy→pure predicate actual→source selectedへ届いたという有限実測PASSを返す。このcohortではProductionのmaterial semantic/native/sanitizer findingを観測していない。一般range解析・termination・全状態soundness・一般ownership/heap安全性の証明ではない。

**VALUE-ALIGNMENT-RISK:** R自身の登録接続には、3338保護ファイルのbytes/index認証、private Git projection、および9旧登録コマンドの接続契約の保守が追加された。これは今回実際に生じた試験機材の保守・review費用であり、compiler-first profileの有限成功からlibrary author＋ordinary-safe callerのUB監査依存閉包が縮小したとは言えない。物理architecture/policyを保ったaudit locality比較実測は今回行っていない。

**audit locality UNMEASURED / GO624 PAUSED / H1 HOLD / P1036-PR1050 STOPを維持。** T1141の別環境制約はR gateではない。RはDRAFT/NEVERMERGEで終了し、Cへ有限結果を返す。