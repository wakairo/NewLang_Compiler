# NewLang CI障害の一次切り分け・限定再実行

> **運用メモ（non-normative）**。Track T Issue [#428](https://github.com/wakairo/NewLang_Compiler/issues/428)／確認時点のCompiler/main `a5730d805efdc2c4825e3b73785727e70bef21da`。仕様・CIの合格条件・依存lock・bootstrapの挙動を変更しない。常時監視や新しい承認gateも設けない。

目的は、外部依存の取得障害をcompiler回帰と取り違えず、初回失敗の証拠を残したまま、**許可された場合に限り失敗ジョブだけ**安全に再実行すること。build/test手順は[README](../README.md#fresh-setup)、責務・信頼境界は[Track T Charter](NewLang_Track_T_Tooling_Charter.md)を優先する。

## 5ステップ（GitHub Actions画面）

1. **対象と不変性を確認する。** Repository → **Actions** → **Compiler validation** → 対象runを開く。run URL/ID、event、branch/PR、表示されたHEAD SHA、attempt番号、各ジョブ名と結論を控える。PRなら現在のPR HEADとの一致も確認する。**古いHEADや停止指示がある場合は再実行しない。** リトライでも元runのSHA/refを使用する（GitHubの仕様）。
2. **最初の失敗を読む。** 失敗したGCC／GCC Release／Clang／ASan／UBSanジョブを開いてstepとログの**最初の実原因**を確認する。run画面の **Latest** から過去のattemptを選択して初回ログを読む。ログの取得・検索方法は[GitHub公式手順](https://docs.github.com/en/actions/how-tos/monitor-workflows/use-workflow-run-logs)を参照。末尾の一般的な `Process completed with exit code 1` だけで分類しない。
3. **原因を区分する（下表）。** 特に `Bootstrap checksum-locked LLVM 23.1.2` の失敗か、続く `Configure, build, and test`／`Check C formatting` の失敗かを分離する。lockの期待値・TLS・SHA-256の検証を飛ばす回避策は禁止。
4. **条件を満たす場合だけ限定再実行する。** 全ジョブが終了し、**外部のlocked packageのHTTP404など、一時的な取得不能がログで確認され**、同じ固定HEADの再確認に意味があり、Rの独立検証・停止条件に抵触しない場合のみ、run右上 **Re-run jobs → Re-run failed jobs → Re-run jobs** を一度選ぶ。単一ジョブならJobs欄の対象ジョブの再実行操作を使える。**Re-run all jobsを選ばない。** Repositoryへの**write権限**が必要。権限がなければCoordinationへ報告し、credential・設定変更を求めない。
5. **結果と履歴を照合して報告する。** Latestの各ジョブを確認し、再実行したジョブの新しいattemptの実行結果と、前attemptから引き継いだ成功ジョブを**区別**する。再失敗、異種障害、HEAD不一致なら停止して担当先に渡す。「attempt 2で最終5/5成功」と「attempt 1から全5ジョブ成功」は同義でない。報告には `run URL / attempt / SHA / 失敗ジョブ・最初の失敗step・根本エラー / 分類 / 再実行の有無 / final各ジョブ / 引継ぎ先` を短く残す。

### 最初の実エラー別の処置

| 観測箇所・兆候 | 判定と扱い |
| --- | --- |
| `Bootstrap checksum-locked LLVM 23.1.2` 中のlocked package `HTTP Error 404`（CMake/CTest前） | **DEPENDENCY-HTTP404 / 環境由来**。一過性の取得障害と認められ、同一HEADが妥当なときだけ失敗ジョブの限定再実行候補。bootstrap内の3回の試行を成功と数えない。継続する404、lock URLの恒久消失疑いは**STOP → Coordination/T**。mirror・pin・取得回数の独断変更は禁止。 |
| `checksum mismatch`、lock/hash/versionの不一致、TLS/証明書検証エラー | **INTEGRITY / TRUST FAILURE**。**即STOP → Coordination/T**。SHAチェックを無効化せず、代替artifactを手動投入せず、再実行で証拠を覆い隠さない。HTTP404と混同しない。 |
| `Host prerequisites from signed Ubuntu repositories` 失敗、OS/architecture非対応、`libstdc++6 >=14`不足、`dpkg`等不足 | **HOST-PREREQUISITE**。runner/前提を特定してCoordination/Tへ。lockを書き換えたり勝手に再実行を続けたりしない。 |
| bootstrap成功後のCMake configure／C17 compile/link／CTest failure | **COMPILER-OR-TEST**。最初のcompile診断または失敗したCTest名・出力を添え**Pへ**。環境404として機械的に再実行しない。 |
| `Check C formatting`、ASan/UBSanのbuild・testでの失敗 | **FORMAT / SANITIZER**。該当step・診断・再現対象HEADを添え**Pへ**（bootstrap起因なら上の分類を優先）。検証失敗をgreenにする再実行をしない。 |
| R fresh BLINDの専用branch／初回独立判定に関係する環境失敗 | **独立性境界**。Tは入力を編集・ジョブ再起動せず、環境障害を **INCONCLUSIVE** として**R/Coordination** に判断を委ねる。Fのproof専用環境ならF/Coordinationへ。 |
| repository/branchの権限・secret・不審なrunner/credential事象 | **SECURITY-STOP**。内容をsecret抜きで**Coordination**に報告。自動再実行・権限拡大をしない。 |

### 確認済みの過去事例（2026-10-14、実際のattemptを分ける）

- [PR #424 run 38495712841](https://github.com/wakairo/NewLang_Compiler/actions/runs/38495712841)、HEAD `29745915fec21742dfcdf707144f673c18d571af`：**attempt 1**はGCCのみ `llvm-23` ダウンロードで `HTTP404 attempt 1/3`、`2/3` に続き最終404、bootstrap stepが失敗（初回**4/5**）。**attempt 2**のGCCが成功し、最終**5/5**。他4ジョブの成功は初回からの持越し。SHA不一致やCTest失敗はこの事例では観測されていない。
- [main run 38496970332](https://github.com/wakairo/NewLang_Compiler/actions/runs/38496970332)、HEAD `241ae6e872ce4e099e6fddda3f57611ce03f3f84`：**attempt 1**はGCCの `llvm-23`、ASanの `libclang-common-23-dev` がそれぞれ3回目のHTTP404でbootstrap失敗（初回**3/5**）。**attempt 2**は失敗した2ジョブが成功し、最終**5/5**。他3ジョブは初回成功の持越し。コンパイラ・CTest・SHA不一致の証拠ではない。

GitHubの[再実行公式手順](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/re-run-workflows-and-jobs?tool=webui)は、失敗ジョブのみ・単一ジョブのUI操作、write権限、元のSHA/ref維持、および過去attemptの確認方法を説明している。**この文書は再実行を事前承認するものではない**。無用な再実行を避け、lock更新・cache評価・workflow改変は各承認済みIssueへ戻す。

### clang-format-23 の修正候補を読む（Issue #648 / Track T 提案）

Clang 構成の `Check C formatting and provide exact pinned-formatter correction` が失敗した場合、
同じ SHA 固定 SDK の `clang-format-23` が生成した **unified patch** をログ末尾と
Actions の `clang-format-suggestions-<checked SHA>-attempt<N>` artifact で確認する。
artifact の `format-suggestions.patch` は提案であり、CI は元の source を**変更しない**。
チェックは従来どおり `clang-format-23 --dry-run --Werror` で失敗扱いとなる。

checkout を該当する固定 commit に合わせてから、内容と対象 C/H ファイルを確認し、
`git apply --check format-suggestions.patch`、次に `git apply format-suggestions.patch`
を実行する。適用後に `bash scripts/check-format.sh` で再確認する。
別の修正手段 `bash scripts/format.sh` は**実ファイルを変更する**ため、差分と凍結済み R の
元 source/oracle の SHA・初回 finding を保存してから、独立検証 Issue §7.6 の予算内で行う。
フォーマット以外の予測・guest 値・安全性・authority を変えてはならない。

## R-FAST：新規RのPR前準備（GitHub操作例、non-normative）

**規則の唯一の正本は[Development Process §7.6](NewLang_Project_Development_Process.md#76-独立実測の自律修復と証拠の保持)**。
以下は[Issue #903](https://github.com/wakairo/NewLang_Compiler/issues/903)の新規R向け手順メモであり、
このRunbook自体はrepair回数・独立性・CI予算・例外を新たに決定しない。
既存の上記HTTP404限定retryの説明も、Rの手動retry許可へ拡張しない。

**FAST残枠の確認（[§7.6の標準6回・個別裁定](NewLang_Project_Development_Process.md#76-独立実測の自律修復と証拠の保持)）：**
通常の新規RではCoordinationが当該Issueに明記した
**「標準6回の個別予算」**（branch-create FASTを含む自然`push`run）を
Rがbranch-wide Actions台帳で管理する。6回は普遍的な安全性基準でも
自動補充でもなく、CがIssue別に異なる有限予算を定める場合はそちらを使う。
§7.6のR所有の機械的修復は残枠内で自律的に行えるが、
未知のsemantic/proof／owner/loan／OOM/native／sanitizer異常や
BLIND／oracleの汚染は残枠に関係なく即STOPとなる。

1. **Gate A／固定SHAを先に記録する。** Coordination RELEASEから
   immutableな**P候補HEAD完全SHA**、main/canonical、対象R Issue、
   `r-fast/r<issue>-<purpose>` という**具体的branch名**、
   FASTのpush予算、別のFULL5＋PARTIAL予算、STOPを控える。
   BLINDならP実装・fixture・oracleを見る前に、R独自のsource・negative・
   oracle・期待値・SHAを凍結する。Rは通常Chat＋専用GitHub branch＋Actionsだけを使い、
   P同等local実行環境を代用しない。
2. **R所有の完成commitを先に作成し、そのSHAにbranchを直接作る。**
   すでに凍結したR source／harness／最低限のCTest登録をGitHub上の
   **blob → tree → commit object（親は完全固定P候補HEAD SHA）**として
   作り、最初は**どの`r-fast/**` refも作らない**。親が本当に固定PのSHA、
   commitのtreeにR source／CTest名があることと、完成R commitの
   **完全SHA**を確認する。その後で一度だけ
   **`r-fast/r<issue>-<purpose>` という新規branch/refを、P HEADではなく
   完成R commit SHAへ直接作成**する。GitHub API／Git object対応アプリなら
   「create blob/tree → create commit(parent=P SHA) → create branch(sha=R commit)」
   の順番にする。GitHub UIが未参照commitの作成やSHA直指定branch作成を
   提供しない場合、このUIに存在しない操作ができると仮定しない。
   **P HEADへの空branch作成を強行せず**、Coordinationからbaseline-run分を
   含む追加FAST予算・代替手段の事前承認を得るかSTOP。
   **重要：`r-fast/**` refの作成そのものが`event=push`でFASTを起動する**。
   P HEADに空branchを作ればそこで1本、R commitへ更新すれば別の1本が
   自然発生する。作成時のrunもBudgetから除外しない。
   `r899-...`など接頭辞が違うbranchや`pull_request`では現行
   `.github/workflows/r-preflight.yml`のFAST triggerを満たさない。
3. **branch全体のFAST実行を列挙してから成功を照合する。**
   **branch作成直後／次のpush直前／DRAFT PR直前**に、Repository →
   **Actions** → `R-FAST self-service pre-PR preparation (NOT R validation)` →
   **対象branch `r-fast/r<issue>-<purpose>`で絞り**、`event=push`の
   **全runをページ跨ぎで取得**する。GitHub Actionsのbranch filterまたは
   GitHub RESTのworkflow runs一覧（`branch`／`event=push` filter）を使い、
   **R HEADだけのcommit runs取得ではない**ことを確認。
   作成時のP HEADでのrun、過去HEADのRED、cancelled／queuedなども
   含む**全run ID・SHA・ref・attempt・status/conclusionの台帳**を
   当該IssueのFAST実行上限と突き合わせ、残枠がないまま次push／PRを
   起動しない。最終候補SHA上のR所有CTest登録が存在しない旧P HEADの
   GREENはpre-PR R成功ではない。個別runを調べるには Repository →
   **Actions** →
   **R-FAST self-service pre-PR preparation (NOT R validation)** →
   対象の**push run**を開き、URL/ID・attempt・branch・event・
   `head_sha` が**いまPRに出すR HEADと完全一致**するか確認する。
   `completed/success` の各stepから固定Clang**23.1.3** formatter PASS、
   GCC `NEWLANG_FRONTEND_ONLY=ON` build PASS、
   `ctest -N` のユニークな登録名前群（**今回追加したR CTest名**を含む）
   とrequired core名PASSを確認。**`ctest -N` の実CTest数は0**、
   source `newlangc --check`／public proof／C17/native／負例／OOM／ASan／UBSanは
   **現行FASTの標準検査ではNOT TESTED**。
   実例：[T862成功#40080841697](https://github.com/wakairo/NewLang_Compiler/actions/runs/40080841697)
   （`push`/HEAD `d0e4f3756907a0b784d040b3186c89018f927b6e`／attempt1）
   と[R886成功#40144734152](https://github.com/wakairo/NewLang_Compiler/actions/runs/40144734152)
   （`push`/HEAD `c6f7f66c6a316ae091c8b408e0e0890f8e5c4ed3`／attempt1）。
   **別HEADの古いGREEN、missing／queued／cancelled／failedはPASSではない**。
   formatter REDならActionsのreview-only suggestions patchを確認し、
   [Process §7.6](NewLang_Project_Development_Process.md#76-独立実測の自律修復と証拠の保持)
   に従い、**認可済みのR所有機械的修復だけ**を元source／oracle SHA・失敗runを
   保存して次の有限pushで再確認する。無許可retry／追加pushはしない。
   **予算算入の実例（固定された歴史的結果、再実行しない）：**
   [T904のbranch作成FAST #40176271506](https://github.com/wakairo/NewLang_Compiler/actions/runs/40176271506)
   はbranch `r-fast/r904-t-source-check-pilot` の**変更前base SHA
   `0981a87db7121fd357d88b964d6b0040a961f41b`**で
   `push`／attempt1／SUCCESS。その後の[T-only RED #40176341366](https://github.com/wakairo/NewLang_Compiler/actions/runs/40176341366)
   `b63e472f84a4480012964fa0bc1a04a951be36d1`（FAIL）と
   [T-only GREEN #40176542790](https://github.com/wakairo/NewLang_Compiler/actions/runs/40176542790)
   `6fbfa88edc9e6f672579600c7516cb198a201f67`（SUCCESS）を合わせ
   **branch全体3 run／認可2 run超過、T904全体OVER-BUDGET STOP**。
   synthetic source CHECK はT-only未採用実験であり、現行mainのFAST機能ではない。
   対照的に[R910は無refの完成R commitから直接branchを作成](https://github.com/wakairo/NewLang_Compiler/issues/910#issuecomment-6396034011)：
   最初の[FAST #40181431466](https://github.com/wakairo/NewLang_Compiler/actions/runs/40181431466)
   は完成R `e520342f3749b4e01d6bd98a21e0f20da5f87e90` でFORMAT RED、
   次の[FAST #40181581234](https://github.com/wakairo/NewLang_Compiler/actions/runs/40181581234)
   はR書式だけ修正した `107b172b39b558bbec1a2ebdbcc7dced292925a5`で
   format／GCC build／483件 `ctest -N` 登録PASS、**branch全体2 run／予算2**。
   ただしR910の本来のFULL5＋PARTIALは後で失敗し総合INCONCLUSIVE／STOP；
   **FAST GREENをR安全性PASSと呼ばない**。

   **4回目FAST後・5回目の前**には、§7.6に従って
   RがIssueへ**情報共有checkpoint**（branch全実run／HEAD／初回失敗層／
   残枠と修復予定）を投稿する。これは**Cの新規承認待ちではない**ため、
   すでに当該Issueで承認済みの5回目・6回目は範囲内でRが判断する。
   ただし原因不明のproof／安全性／権限／source期待値逸脱や予算超過は
   FAST修復で押し通さずSTOP→Coordinationとする。

4. **FAST成功後にだけ固定P向けDRAFT R PRを開く。**
   Baseは**同じ固定P候補HEADのbranch**、headは検証した専用R HEAD。
   別に自然起動する**FULL5（GCC／GCC Release／Clang／ASan／UBSan）と
   NO-LLVM PARTIAL**の6つの**実行**jobについて、
   run URL／ID／event／head／attempt、skippedなし、実CTest・元sourceからの
   CHECK→真正owned→C17/native・negative/sanitizer／未到達項目を記録する。
   FAST成功はFULL5/PARTIALの省略・merge・R独立性の認定ではない。
   例外はCoordinationが個別Issue**および起動prompt**に事前明記した場合のみ。
   Rは失敗・見当たらないrun・SHA不一致・予算超過ならSTOPしてCへ戻す。

**source CHECKの現況（T #904）：** [T904](https://github.com/wakairo/NewLang_Compiler/issues/904)は
軽量source-smokeの**GUARDED PILOT RELEASE**であり、
**現在の採用済み`r-preflight.yml`には存在しない／未実装・未採用**。
`u8 == u8` に対する`USIZE-EQUALITY-TYPE`のようなsource診断は
現行のformat／build／`ctest -N`だけでは検出できない。
仮に将来source-smokeを採用しても、その最初のsource診断到達は
source-issued proof／C17/native／実OOMのPASSではない。
T実装・C採用・Docs改訂の**別判断までは現在の操作・必須gateとして扱わない**。
