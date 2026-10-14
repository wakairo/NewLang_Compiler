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
