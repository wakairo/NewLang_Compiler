# NewLang Evidence archive / Codex Cloud disk policy

> **運用正本候補 — Coordinationのreview・merge後に有効。**
> [C1134 / Issue #1505](https://github.com/wakairo/NewLang_Compiler/issues/1505)の
> 有限pilotに基づく提案。DRAFT PRの作成だけではPの保存方式を切り替えない。
> 言語仕様、Product GO、compilerの安全性・CI・独立レビュー条件は変更しない。

## 1. 目的・権限・継承

大量の開発証拠を必要時に復元可能な形でGitHubへ保存し、Codex Cloudには
現在の作業に必要なものと十分な空きbytes/inodesを残す。
保存先の存在やpush成功だけを、完全な復元可能性の証明にしない。
Gitは記録されたbytes・treeの同一性を与えるが、意味論・テスト期待値・
authorの正当性・独立性・Product価値を保証しない。

本書の運用上の起点は次の裁定であり、元の失敗・条件・来歴を書き換えない。

- [C1118](https://github.com/wakairo/NewLang_Compiler/issues/1496#issuecomment-6947690512):
  元証拠保全、完全復元後だけのP所有ローカル削除、bytes/inodes維持。
- [C1119](https://github.com/wakairo/NewLang_Compiler/issues/1496#issuecomment-6947877811):
  **Pankの開発情報は原則公開**。raw・失敗・独立結果であること自体を秘密扱いしない。
- [C1120](https://github.com/wakairo/NewLang_Compiler/issues/1496#issuecomment-6948093379):
  別Codexタスクによる34B Release pilot復元を受理した来歴。
  以後の各archiveも全実bytes・全必要memberの復元が削除条件であり、
  毎回別タスクを新設する一律gateではない。
- [P1499の実アーカイブ最終結果](https://github.com/wakairo/NewLang_Compiler/issues/1499#issuecomment-6950342944):
  Releaseの大容量転送はBLOCKED、既存Git LFS fallbackは全member復元PASS。
  小さいReleaseの成功を大きい実archiveの成功へ読み替えない。

本書が採用された場合、以下の適格な個別ファイル群については通常Gitの
`wakairo/NewLang_Evidence`を最初の選択肢とし、C1118のRelease-firstをその範囲で更新する。
既存の検証済みLFS archiveの移動・削除・一括移行は要求も認可もしない。
公開・復元・所有権・独立性・安全性についてのC1118–C1120の条件は維持する。

## 2. 保存対象・公開・保持先

PはDRAFT PR公表時、および容量需要が大きくなる前に、自分の領域を短く棚卸しする。

| 区分 | 扱い |
| --- | --- |
| 公開済みrefから到達可能なsource/test/oracle/commit | 元repoと固定SHA・tree・Issue/PRを記録。unpushed/unreachable/uncommittedを保存済みとしない |
| 唯一のFIRST/RED、元source/oracle、raw logs/status、実測値、必要な生成物・実行物・private履歴 | 元bytesと所有者・観測層・元HEAD/base/PR/tree・来歴を保存。再実行を過去の元観測の復元とみなさない |
| 再生成可能なbuild objects、downloadやpackaging重複、不要cache | 現在の修復・次の認可作業で不要かをPが判断し、安全に整理 |
| active worktree/HEAD、unpushed作業、修復入力、必要な単一pinned SDK、次作業の再利用物 | 継続保持。PRを公表したという理由だけで削除しない |

**公開原則の例外は具体的な場合だけ**とする。実credentials、秘密設定、signed session URL、
個人情報、第三者の真の非公開資料・権利制限、必要な期間に限った重大な脆弱性の協調開示等は
対象を特定して除外・安全に取り扱う。home全体、環境変数全体、`.git`の認証設定を梱包しない。
通常の公開OSS資料に新しい一律の秘密審査・ファイルごとのC承認gateは設けない。

**R/Fの初期入力隔離は秘密保持ではない。** [Process §7](NewLang_Project_Development_Process.md#7-独立red-team)
の順序・FIRST・独立source/oracleを守り、他Trackの元証拠をP/Tが改変・削除・横取りしない。
公開されていても担当タスクのBLIND順序を越えて読まない。各Trackが自分の証拠のcustodyを保つ。

Evidenceのmainは入口・案内用とし、大量のarchive dataをCompiler/mainへ混在させない。
保存用ref、cohort、所有Track/Issue、commit/tree、member数/bytes、manifest/復元receiptの所在を
Issue等の小さい索引に残す。refを保持し、履歴をappend-onlyで更新する。
通常GitもGitHubも不変・永久・無制限のbackupではない。損失の重大な元証拠には
追加の独立復元可能なcopyを検討する。remote archive refの削除は通常のlocal整理に含めない。

## 3. 通常Gitによる保存・独立復元・削除

適格性はarchiveごとに判断する。ファイル数・合計bytes・最大単体・directory幅、
Git packの見込み、upload/restore権限、必要なローカル余裕、公開上の具体的例外を確認する。
大きい非圧縮binaryや実測を超える規模までpilot PASSを一般化しない。
新しい環境・保存先・認証経路では、まず小さい合成データでpush→独立復元を確認する。

1. **隔離・事前照合。** 所有者が確認した一時領域を新設する。active Pや他Trackのworktree、
   Git objects/alternates/cacheを使わない。既存remote/default branch/refをGETし、
   使用identityと意図するEvidence refへのpush権限を秘密を出さず確認する。
   compiler用の広いtokenを無断流用せず、account/credential/security設定を拡大しない。
   空repoの初期化は明示認可かつ直前の空確認時だけ。予期しない初期化・writerはSTOP。
2. **元証拠を保全して一時stagingへcopy。** 必要な全memberについて相対path、サイズ、
   SHA-256、件数・合計bytesをpush前に記録する。ignored/untrackedやdotfileの取りこぼしを確認し、
   allowlist外のpath、symlinkによる領域外参照、特殊file、秘密値を持ち込まない。
   元FIRST/RED/NOT_RUNと後続結果を区別し、元source/oracleと固定SHA・観測層を保持する。
   manifestはcohortの範囲に限定し、自身のhashは外側receiptに記録できる。
3. **普通のcommit・push。** bounded cohortの独立した保存用branchを用意する。
   初回は必要に応じ`git init`/orphan root、以降は通常の追記commitを作り、
   `git add`/`git commit`/明示refへの`git push`を用いる。force push、既存refのreset、
   上書き・リモート削除をしない。LFSやReleaseへ暗黙に切り替えない。
4. **remote receipt。** push後にremote ref/commit/treeをGETし、ローカルの固定SHA/treeと照合する。
   upload受付だけで保存完了としない。競合・error・不一致なら元証拠とstagingを残す。
5. **真正な独立復元。** 新規の空の別一時領域へ、GitHubのHTTPS remoteから取得する。
   shared/reference clone、local path/hardlink、alternates、元データ/cacheの流用はしない。
   例: `git clone --depth=1 --single-branch --no-tags --no-local --branch <archive-branch> <https-remote> <fresh-dir>`。
   `HEAD`/treeが予定した固定SHAかを確認し、branchが進んでいたら無検証で代用しない。
   過去commitは新規repoへそのSHAを`git fetch --depth=1 <remote> <commit>`して復元できることを確認する。
   refから到達しないSHAの再取得を将来も保証できるとは仮定しない。
6. **全member照合。** 実ファイルを全件読み、事前の全path集合、件数、サイズ、SHA-256を照合する。
   tracked一覧だけでなく復元先の実member集合も照合し、余分・不足・path衝突を拒否する。
   必要な元Git履歴・patch・bundle等がある場合はその復旧前提と復旧後bytesまで検証する。
   pointer、API metadata、clone exit0、rerun成功だけでは代用しない。
7. **receiptの永続化後だけlocal削除。** owner、元HEAD/status、remote URL/ref/commit/tree、
   全件検証結果、manifest/hash、所要時間、削除対象とbytes/inodesをIssue/Gitに保存・再GETする。
   **そのarchiveの完全な実復元が成功した後だけ**、担当者が自分の不要な元copy、
   staging/restore一時clone、不要な再生成物を削除する。F/R等の他所有物は対象外。
   元データ・active worktree・必要SDKを含む広い再帰削除は禁止する。
   削除前後の`df` bytes/inodesと対象`du`を記録し、実際の回収を確認する。

`git switch`は到達可能な旧objectsを消さず、`git gc`も到達可能な履歴を削除しない。
実測された回収手段は、検証済みの**自立した一時clone全体を`.git`ごと削除すること**である。

### 3.1 shallow / partial / sparse とbranchの選択

shallow single-branchは他branchと古い履歴の取得を減らすが、tipにある全blobは通常取得する。
`--filter=blob:none`＋sparse checkoutはサブツリーだけのlazy fetchに使える。
sparseだけではobject転送量削減を保証しない。endpointのfilter対応と未取得blobを確認し、
未対応ならUNSUPPORTEDとして通常shallow cloneへ戻す。
**部分取得PASSを全archiveの削除gateに使わない。** 全件復元は別途必要である。

C1134は一つのorphan cohort branchへの2snapshotを実証しただけである。
cohort branchを限定数に保つ方式は取得単位を絞れる一方、branchが際限なく増える費用がある。
共有track branch＋cohort directoryはrefsを減らせるが、shallow取得でもHEADの全cohortを含み得る。
規模に応じCがref保持・索引・取得単位を再評価する。何千branchの性能や自動削除は未承認・未実証。

## 4. bytes/inodesの予防管理とSTOP

大きいsanitizer/full buildの前、主要な生成後、DRAFT PRの区切りで、
**空きbytesと空きinodesの両方**を確認する。必要量は現物のbuild/ELF、
staging＋pack一時領域＋独立restore、download、次工程の見積peakと安全余裕から決める。
固定の「4GiBあれば十分」は採用しない。matrixの直列実行や不要な再生成物整理はできるが、
必要なnegative/native/sanitizer/OOM検証を容量のために省略しない。
予測が収まらなければその高費用実行を止め、整理・再見積またはCへ資源/範囲の裁定を戻す。
日常の自分の範囲の整理は[Process §4.1.2](NewLang_Project_Development_Process.md)の裁量内で行い、
新しいcompiler CI/PR gate、常駐監視、世代manifest protocolを作らない。

次の場合は影響するpush/削除/高費用実行をSTOPし、元証拠を残してIssueへ事実を報告する。

- scoped authority・認証不足、予期しないrepo/ref/writer、許可範囲外のpermission変更が必要。
- upload error、単体/pack/push制限、過大な時間・rate制限、容量/inode不足。
- ref/SHA/tree、path/member/size/hashの不一致、固定commitをremoteから再取得できない。
- 元来歴・FIRST/RED欠落、custody不明、BLIND入力汚染、秘密/第三者資料の混入、危険なcleanup。

失敗・未完了はNO-GO/条件付きとして記録し、通常Git成功・採用・未実行PASSを装わない。
局所的な安全な修復は認可scope内でできるが、失敗を上書きせず別候補として追跡する。

## 5. GitHub制限・fallback・移行しないもの

[GitHub公式repository limits](https://docs.github.com/en/repositories/creating-and-managing-repositories/repository-limits)
（C1134で2026-11-18にGET確認）と、利用時点の最新条件を確認する。

| 項目 | 公表値・運用上の意味 |
| --- | --- |
| 単体Git object | 推奨最大1MB、hard100MB。実際に上限を超えるfileは通常Gitへpushしない |
| 1 push | hard2GB。境界まで使える性能保証ではない |
| `.git` on-disk | 推奨最大10GB。全履歴・packのserver health/clone費用も評価 |
| directory幅 / 深さ | 推奨3,000 entries / 50。浅いfanoutで巨大なtree再生成を抑える |
| branch数 | 推奨5,000。大量refsはfetch/保守/制限の費用を増やす |
| read operations | 推奨15ops/s/repo。clone/fetchを無制限に並列化しない |

これらの推奨値とhard制限は区別する。GitHubのdiff UIにも別の表示制限があり、
ブラウザー表示だけで多数ファイルの完全性を判断しない。
Git cloneの仕様は[Git公式文書](https://git-scm.com/docs/git-clone)を参照する。
C1134では当該URLが403だったため[公式repositoryの文書ソース](https://github.com/git/git/blob/master/Documentation/git-clone.adoc)
をGETして確認した。元403は失敗来歴として残した。

巨大な単体、大容量binary、権限や性能で通常Gitが不適格なら、影響する保存/削除を止める。
**Git LFS/Release等は、別途明示した認可・予算・権限と、その実archiveの完全復元証明を
伴うescape hatchとして残す。** 自動的な課金・契約・token拡大、実ファイルの無断分割、
大きいtar/ZIPへの暗黙切替をしない。既存LFS archiveの履歴/ref/objectsを維持し、
移行を急がない。将来の個別移行にも移行先での完全復元後だけの削除条件を適用する。

## 6. C1134の実測根拠と限界

完全receipt・manifest/hash・資源checkpointは
[Issue #1505の実測報告](https://github.com/wakairo/NewLang_Compiler/issues/1505#issuecomment-6965521070)
を参照する。Git2.52.0、独立したP相当クラスのmanaged Codex Cloud、
初期空き約64.13GB/4.19M inodes、新規一時領域で実行した。
既存Pのworktree/objects/cache、実P/F/Rデータは使っていない。

保存branch: `wakairo/NewLang_Evidence:archive/pilot-c1134-20261118`。

| | 小規模A | 多数ファイルB（A含む全HEAD） |
| --- | --- | --- |
| commit | `0b864636c66c60b00f32a1ce7af1a9f273148084` | `65cf7cbf98479e7e0524fca609d3b85bb882ecac` |
| tree | `d030591e4f2337e15904bd40f7f61a3cb27a4a4c` | `d59b830e9d520f55c52a3cefcb8d1e8b99ada55b` |
| ファイル数 / bytes | 64 / 170,260 | 23,814 / 246,481,473 |
| push秒 / fresh shallow clone秒 | 1.943 / 1.072 | 34.288 / 18.669 |
| 全件hash/member照合秒 | 0.017 | 1.783 |
| restore pack / pack .idx / Git index bytes | 72,953 / 2,948 / 5,699 | 79,278,621 / 668,648 / 2,286,554 |
| restoreのallocated bytes | 524,288 | 378,527,744 |
| 検証後clone削除による実回収bytes / inodes | 1,540,096 / 351 | 780,132,352 / 48,831（partial含む） |

Bは24 batch、最大directory1,000files、最大file3,218,588B（manifest、推奨1MBを超える）。
blob-less/sparse比較1回はclone1.302秒＋checkout1.748秒、親metadataを含む
1,005files/13,450,345Bを照合し、未取得objects22,809を確認した。
partial clone全体は23,031,808B、pack合計4,954,366B、Git index2,286,562B。
部分復元は全体証明に流用せず、別のfull shallow cloneで全23,814membersを照合済み。

Bの観測最大占有増加はbaseline比783,716,352B/48,617inodes（receipt等含む）。
command中statvfsを100ms間隔で採り、別途生成後/照合後/削除前後を観測した値であり、
サンプル間の瞬間peakとserver側disk・wire bytesは未計測。
最終cleanup直後の空きは64,127,168,512B/4,194,097inodes。
実push3回、検証用fresh full clones2つ＋partial比較1つ、元clone共有なし、全件一致後に削除した。

**結論は多数の小さい合成テキストの有限pilot PASSに限る。** 実Pの約833MB corpus、
大きいbinary/ELF、長期履歴、何千branch、継続quota/課金、別のCodexタスクによる同実験の追試、
CPU/network条件の完全一致は未検証。同じfile数だけでそれらをPASSとしない。
今回のfresh clonesは同じ独立Tタスク内であり、C1120の別タスク復元来歴とは区別する。
READY・merge・既定保存方式の採用はCoordinationが独立に判断する。
