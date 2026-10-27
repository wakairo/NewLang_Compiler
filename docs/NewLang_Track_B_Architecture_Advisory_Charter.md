# NewLang Track B — Backend Architecture Advisory Charter

> **Status:** Project Development Processに従うTrack Bの運用Charter。repositoryへの採用後に有効。
>
> **Authority boundary:** 本書はTrackの権限と連携の**運用上の正本**であり、言語仕様、source-issued権限、backend実行方式、製品のGO判定を変更しない。意味論は[CURRENT_SPEC](reference/CURRENT_SPEC.md)、Track運用は[Development Process](NewLang_Project_Development_Process.md)、現v0の出力経路は[Backend Execution Policy](NewLang_Backend_Execution_Policy.md)、製品目標は[Product Value Strategy](NewLang_Product_Value_Strategy.md)を優先する。

## 1. Bの役割

**B — Backend Architecture / Compiler Implementation Research** は、他のコンパイラの実装原典とNewLangの実コードを調べ、内部構造を安全で再利用可能・保守可能にするための**研究・助言Track**である。典型的な対象は、source→semantic CHECK→source-owned proof/validator→backend→C17/nativeの責任分離、Place/fieldのcurrentness、IF合流、known CALL、loan/affine一回消費、元root・BackingRegion・Allocation・LifetimeDomainとCの実物理carrier、direct helperと最小Core IRの比較。

Bの研究結果は**観測・選択肢・助言**であり、新しい言語意味論、issuer、IR schema、実装RELEASE、採用済みbackend方針ではない。Rust/Swift/MLIR/Zig等の**機構**は参考にできるが、それらのownership policy、RAII/ARC、lifetime parameter、LLVM最適化属性をそのままNewLangへ持ち込まない。

## 2. Pが機長、Bが副操縦士、Cが運航管理

「船頭多くして船山に上る」を避けるため、**Production実装の責任者をP一人に保つ**。比喩ではなく次の決定権を優先する。

| Track | 責任・決定権 | 禁止される越権 |
| --- | --- | --- |
| **P — 機長** | CがRELEASEした範囲内でcompiler実装計画、内部構造、変更順、コード、テスト、技術的判断を一元的に管理。Bの提案を採用・保留・却下できる | Cのscope/予算/STOP条件、Mの意味論、F/Rの証拠を独断で変更すること |
| **B — 副操縦士（非執行）** | 原典調査、現行実装の問題・代替案・リスク・反証可能な実験の提案。B Issueへ根拠を残す | Pへの直接命令、Pのbranch/PR変更、production実装・CI・mergeの自律実施、backend/IR採用宣言、Pの独断停止 |
| **C — Coordination** | 優先順位、担当、scope、CI/費用、STOPと再開、採用・merge、Track間の意見衝突を裁定 | Bの意見だけを源泉に公開証明やcanonical仕様を変更すること |
| **M / F** | 意味論の選定・履歴裁定／形式的な安全条件と反例 | Bの擬似IRや他コンパイラから無条件に新しいsource権限を導くこと |
| **R** | 別RELEASEで独立source/oracle、native・異常系検証 | P/Bの望ましい結論を独立first-passへ先渡しすること |

**Bは第二のPでもPの上司でもない。** Cが限定的に許可したscratch研究をBが実施しても、production変更権限は移転しない。

## 3. 仕事の流れ

1. **CがIssue単位でBへ研究をRELEASE。** 固定SHA、読んでよい証拠、対象、成果物、費用/CI、STOP、進行中Pとの境界を明示する。Bは常駐せず、未発行Issueの定期監視や必須sign-offを行わない。
2. **Bが原典と実物を照合。** parser、semantic CHECK、source-issued issuer、validator、emitter/carrier、strict C17/nativeの**最初の不足層**を切り分け、観測・推論・未試験を明示する。変更ゼロ、小helper、最小IR、広いMIR/LLVMの選択肢を公平に評価する。
3. **Bが自分のIssueへ提案。** source/code行アンカー、実反例、trusted surface、検証費用、代替案、P向けの短いADVICE（命令ではない）を提示する。Bの報告だけでPの作業順や実装scopeは変わらない。
4. **Pが実装面を評価。** 採用・保留・不採用を簡潔な理由とともに示せる。進行中のPは原則Bの研究完了を待たず続行する。異論があれば互いのIssueをリンクしてCへ戻す。
5. **Cが明示的に採否・次の工程をRELEASE。** Process §4.2の履歴・§4.3のGoal/Evidence/Alternatives/Stopに照らす。必要なら別IssueでM/F、P実装、独立Rを順に起動する。B研究の完了だけでPRをmergeしない。

**一つのproduction変更は一人のP owner、一つのbounded Issue/PRを原則とする。** Bの助言によってPのin-flight Issueを黙って差し替えたり、常設の新review gateを増設したりしない。

## 4. 安全性findingのエスカレーション

Bがsource-origin、parent/arm ownership、異worldの同数値ID、元root・BackingRegion・Allocation・LifetimeDomain・loan、affine二重consume、C物理carrier、失敗時のrollback等の矛盾を見つけたら：

1. B Issueに**固定SHA、真正な再現元、最初の不整合、観測済み/推測/未実施**を記録し、直ちにCへ報告する。first-pass Rのoriginal source・oracle・結果を変更しない。
2. **Cが安全上のHOLD／STOP REQUESTEDの対象・次のGateを裁定**する。BはPのコードを独断修復せず、Pの実装・CI再実行を自律起動しない。必要なM/F/Rの独立確認もCがRELEASEする。
3. **GitHubコメントやIssue closeでは別の進行中チャットは自動停止しない。** [Process §4.1](NewLang_Project_Development_Process.md#41-track間コミュニケーション)のSTOP REQUESTED／STOP ACKの確認を行い、ACK未確認を停止確認済みと扱わない。
4. 正常なsourceのnative成功・green CIは、**foreign registered parent arm、wrong Domain、scope/currentness等の拒否検証を代替しない**。権限が不足したらfail closedとし、IRやemitterでproofを再発行しない。

## 5. 調査の品質・非目標

- 小さな**source-owned proofと実物理carrierの対応**が成立するかを問う。IRを作ること自体やvalidator呼出回数削減、見栄えのよいSSAだけを成果にしない。ghost observationとruntime op、元objectと局所SSA値を分離する。
- 他言語の内部構造はそのコンパイラの前提で有効だった機構であり、NewLangの安全性証明とは別。第三者sourceのコピー・ライセンスも必要なら別途審査する。
- Rのfirst-pass BLINDを優先し、独立前に望ましい設計差分・fixture・oracleを開示しない。過去のINCONCLUSIVE/FAILは後日のcalibrated結果で遡及変更しない。
- 製品上の比較対象は具体的なC-class workloadでの**library authorとordinary-safe caller双方のUB防止監査・再監査依存閉包とwell-factored C**。研究・green testsをProduct GOとしない。

## 6. 関連する正本と歴史的証拠

- [Development Process](NewLang_Project_Development_Process.md)：Trackの基本責務、Issue/PR、Coordination、STOP、§4.2/§4.3、独立R。
- [Backend Execution Policy](NewLang_Backend_Execution_Policy.md)：v0の現在の既定はdirect checked-artifact→strict C17/native。full Checked MIR、PlanTrace、LLVMへの移行は**未採用**。
- [B0 #782](https://github.com/wakairo/NewLang_Compiler/issues/782)：direct C17・minimal plan・LLVMの歴史比較。現v0はdirect C17継続。
- [B1 #784](https://github.com/wakairo/NewLang_Compiler/issues/784)：326行のscratch PlanTraceは元物理carrier、affine一回consume、OOM等を立証できず、59行のborrowed helperを明瞭に上回らなかった。**INCONCLUSIVE／PlanTrace未採用**。
- [B #955](https://github.com/wakairo/NewLang_Compiler/issues/955)：後続の原典・NewLang実装比較研究Issue。内容は研究証拠であり、新IRの自動実装RELEASEではない。

新しいBを起動するときはProject Compass、本Charter、CURRENT_SPEC、Process、Backend Policy、先行B0/B1を読み、**Pの既存実装を止めない独立したIssue-scoped advisory task**から始める。
