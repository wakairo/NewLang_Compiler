# NewLang プロジェクト開発運用方針

> Status: 初版運用方針
>
> この文書は、NewLangにおける複数track横断の現在の開発運用を定める。
> 意図的に軽量な初版であり、projectの経験に応じて今後改訂してよい。
> 変更履歴はGitのcommit historyを正とし、この文書内に別のchange log / Version historyを持たない。
>

## 1. 目的

NewLangは、部分的に独立した複数trackで開発する。

- **M — 仕様策定 / 設計**
- **F — 形式証明 / 意味論の形式化**
- **P — production compiler実装**
- **R — 独立Red Team review**
- **V — Value / Product Validation**
- **O — Python意味論・解析アルゴリズム研究（暫定）**
- **T — Tooling / Infrastructure（開発基盤、暫定）**
- **Coordination — track間の裁定・同期・順序決定**

目的は、各trackを最初から同じ結論へ揃えることではない。
異なる種類の証拠で同じlanguage designを攻撃し、具体的なfindingをcanonical specificationへ戻すことを目的とする。

## 2. 正本（canonical authority）

normativeな意味論仕様は、次が指すDraftとする。

```text
NewLang_Compiler/main
docs/reference/CURRENT_SPEC.md
```

設計report、proof report、compiler report、experiment、Deep Research、conversation historyは証拠であり、単独ではcanonical Draftを上書きしない。

product / targeting / value-validationのproject-level正本は次のrepository文書とする。

```text
docs/NewLang_Product_Value_Strategy.md
docs/NewLang_North_Star_Product_Validation.md
docs/NewLang_C_Replacement_Coverage_Map.md
```

これらはproduct strategyのauthorityであり、language semanticsのnormative Draftを上書きしない。
重要なsemantic decisionがreportや会話にしか存在しない場合は、review済みDraft revisionを経てmainへmergeされた時点でnormativeとする。

旧案・未解決設計意図の参照先は [NewLang Design-Intent Ledger](NewLang_Design_Intent_Ledger.md) とする。
**Ledgerはnon-normative evidence indexであり、現在のDraftを上書きしない。**
設計判断を選択・変更する前の必須照合手順は
[NewLang 設計判断手順](NewLang_Design_Decision_Procedure.md) に定める。
暫定Track Oの運用上のCharterは[Track O研究Charter](NewLang_Track_O_Research_Charter.md)、暫定Track TのCharterは[Track T開発基盤Charter](NewLang_Track_T_Tooling_Charter.md)とする。いずれもproject processの文書であり、言語仕様のnormative Draftを上書きしない。

UTF-8で問題なく管理できる文書は日本語を基本とし、必要に応じて英語版を併記する。
一方、shell操作やtoolchain上で前面に現れるcommit message等は、互換性・可搬性を優先して英語ASCIIを基本とする。

## 3. 各trackの責務

### M — 仕様策定 / 設計

Mはlanguage semanticsとsource surfaceを検討・裁定する。
**新しいsource/semantic/API裁定の前に§4.2のhistorical design-intent gateを実施する。**
過去のnon-normative案を機械的に採用する義務はなく、維持・理由付き変更・Deferredを選択できる。
大規模な再設計より、小さく直交的なmechanismとtargeted reopenを優先する。

Mだけが実装・形式化より無制限に先行しない。
coordinationが明示的な理由を記録しない限り、実務上の目安としてP/Fより概ね1 major semantic milestoneを超えて先行しない。

### F — 形式証明

Fは選択されたsemantic claimを形式化し、矛盾、欠けたinvariant、hidden assumptionを探す。

形式化は仕様に対する証拠であって、仕様より上位のauthorityではない。
既存proofを保ちやすいという理由だけでlanguage ruleを維持しない。

### P — production compiler

Pはcanonical semanticsを現実的なcompilerで表現・診断・検査し、最終的にloweringできるかを検証する。

implementation difficultyは重要なfeedbackだが、backend convenienceだけを理由にsource semanticsを変更しない。

### V — Value / Product Validation

Vはtarget user / workload / workflowを具体化し、value hypothesis、North Star、PASS / FAIL criteria、anti-gaming rule、human obligation、topology preservation等をproduct evidenceとして管理する。
また、current beachheadを不必要に広げずにpractical C-replacement envelopeを長期監査し、Deferred capabilityが将来のC-class workloadを不必要にforecloseしないかを確認する。

重要なproduct goalとの不整合、過剰な機構拡張、未検討の低負担な代替手段を発見した場合、Vは必要なときだけ `VALUE-ALIGNMENT-RISK` を短く報告し、技術研究としてのGOと製品投資としてのGOを区別する（§4.3）。

Vはcanonical language semantics、Draft revision、implementation architecture、formal proof strategy、merge / milestone sequencingを単独では決めない。詳細は `NewLang_Product_Value_Strategy.md` を参照する。

### Q — Quality Observation & Improvement

Qは、品質指標の最大化ではなく、NewLangの実質的な品質リスクを減らすためのadvisory trackである。
通常は沈黙し、既存のP / CI / F / R / Vの証拠を観測して、具体的なリスクまたは費用対効果のある改善機会がある場合だけ、根拠付きIssueとしてCoordinationへ提案する。
Q自身はcompiler実装、テスト作成、CI構築を行わず、PR / mergeの自動的な必須gateにもならない。提案の採否、優先順位、担当trackの割り当てはCoordinationだけが決定する。
低価値になった品質手続きの簡素化・廃止も正当な提案対象とする。承認時の詳細なCharterはIssue #162を参照する。

### O — Python意味論・解析アルゴリズム研究（暫定）

Oは非normativeなPython実験で、コンパイラのsemantic-state表現、Changeの時系列検証、抽象解析の精度とコストを独立に比較し、反例と再現可能な知見をM/P/Coordinationへ返す。
実NewLangソースの受理・Allocation/Domainの発行・checked-owned検証はPの責務であり、Oの合成モデルや診断用JSONで代替しない。Oは独立R、Fの形式証明、Mの意味論選択にも代わらない。
Oの実施はCoordinationが承認した限定Issueに限る。常設化・独立リポジトリ化は未承認。詳細は[Track O研究Charter](NewLang_Track_O_Research_Charter.md)を参照する。

### T — Tooling / Infrastructure（暫定）

Tは、CoordinationがIssue単位で認可した開発基盤庶務（toolchain bootstrap、固定依存取得・検証、CI/workflow、再現可能なbuild/test環境、開発者向け起動手順）の調査・小規模実装を担当する。**常駐・自動監視はせず、仕事のないときは沈黙する。** 詳細は[Track T開発基盤Charter](NewLang_Track_T_Tooling_Charter.md)。
Pは引き続きcompiler source/semantic/backendとそのテスト契約を担当し、Tは仕様・権限issuer・compiler安全性や製品判断を変更しない。Pへ着手済みのIssueはT新設だけでは移管しない。toolchain lock、署名来歴、TLS・SHA、cache/Actionsの信頼境界はTの便宜で緩めず、変更案はCoordinationがreview・merge裁定する。Qは引き続き提案のみの独立品質Trackである。

### Coordination

Coordinationは少なくとも以下を担う。

- authority / version synchronization
- milestone sequencing
- findingのclassificationとseverity整理
- KEEP / CLOSE / TARGETED REOPENの裁定
- Draft revisionの要否判定
- 設計変更前・independent PR review時のhistorical design-intent gate (§4.2)
- Mを止めてP/Fのcatch-upを待つかの判断
- 大きな機構投資・製品目標変更前のGoal Alignment（§4.3）とV警告への応答

## 4. 標準feedback loop

仕様変更は原則として次の流れを取る。

```text
Finding
  -> classification
  -> severity
  -> minimal reopen target
  -> design / adjudication
  -> candidate Draft
  -> review / CI
  -> merge to main
  -> targeted downstream revalidation
```

reportはこのprocessの代替にはならない。

具体的な問題を局所化できる場合は **targeted reopen** を優先する。
一つのworkloadで一つのlocal surface gapが見つかっただけで、milestone全体を再openしない。


### 4.1 Track間コミュニケーション

track間の継続的・参照可能なコミュニケーションは、原則として **GitHub Issueを主経路** とする。
promptやchatは作業開始・制御のために使ってよいが、handoffの全内容を会話だけへ閉じ込めず、可能な限り対応するIssueを指す薄い起動指示にする。

通常のTrack起動指示には、次の**一文**を含める（同趣旨の簡潔な表現でよい）。

> 作業開始時に `docs/NewLang_Project_Compass.md` から関連する目標・非目標・代替手段の正本を探し、曖昧ならリンク先を確認してください。目標逸脱の懸念は、技術的な作業結果と区別してIssueに記録し、Coordinationに伝えてください。

Compassは新しいauthorityではない。**独立Rのfresh blind first-pass**では§7の入力隔離が優先され、旧案・設計者のpreferred rationale等を判定前に読ませない。

一つのboundedなtask / finding / revalidationには、原則として一つのIssueを用いる。
そのIssueには必要に応じて次を集約する。

- canonical repository / main SHA / `CURRENT_SPEC.md` 等のauthority
- source/API/semantic裁定なら§4.2の履歴照合結果（Ledger ID、旧案、KEEP/REPLACE/DEFERと理由）
- scopeとnon-goals
- receiving trackへの質問・依頼・stop condition
- finding、counterexample、CI / proof / implementation evidence
- Coordinationによるadjudication
- closure / reopen / downstream revalidationの状態

実際のrepository変更はPRでreviewし、IssueはそのPRを参照する。
IssueやPR上の議論・report・promptは証拠とcoordination recordであり、それ自体がcanonical specificationを上書きしない。

現状は複数trackが同じGitHub accountを使用するため、trackを代表して書くsubstantiveなIssue本文・コメントでは、冒頭付近に発言主体を明示する。

```text
Track: Coordination
Track: M
Track: F
Track: P
Track: R
Track: V
Track: Q
Track: T
```

後から読んだときに誰の判断・finding・質問か識別できればよく、厳密な機械可読formatは要求しない。

同じscope内の質問、回答、追加証拠、裁定はできるだけ既存Issueのcommentへ追記し、「あのときのprompt」やconversation historyだけを参照点にしない。
scopeが実質的に変わった場合は新しいIssueを作り、元Issueからlinkする。

ただしIssueを細かな発言ごとに乱立させない。
単純な実装上のやり取りや一つのbounded task内の追補は既存Issue / PRへまとめる。

Red Teamについては§7の独立性を優先する。
first-pass attack用Issueには、その時点で許可された狭い入力だけを置き、M/F/Pのrationaleや過去の結論を先回りして混ぜない。
first-pass finding後に追加情報を解禁する場合は、同じIssueへ追記するか、明示的にlinkしたfollow-up Issueで扱う。

#### 4.1.1 作業指示・報告の明確性と誤認防止

GitHub Issue、PR、Track起動prompt、作業報告などの文章は、**実際の作業目的・対象・範囲が第三者やAI実行環境にも正確に伝わるように記述する**。

特にコンパイラの異常系テスト、独立レビュー、不正な入力の検証、内部状態の整合性確認などは、セキュリティ上の不正利用を目的とする作業と誤認されないよう、次を意識する。

- 冒頭で、NewLangコンパイラの開発、仕様検証、品質改善などの正当な目的を明示する。
- 実際の作業に即して「再現テスト」「異常系テスト」「境界条件の検証」「独立検証」「内部不変条件の確認」などの具体的な表現を優先する。
- 「攻撃」「敵対的」「侵入」「悪用」など、別の意味に解釈されやすい語は不用意に使用しない。技術的に必要な場合は、その意味と検証対象を明確にしたうえで使用する。
- 検証対象のrepository、固定commit、使用するテスト環境、変更可能な範囲、実施しない作業を必要に応じて明示する。
- 作業の安全性・正当性や検証結果を誇張せず、実際の内容を正確に説明する。

**この方針は、安全対策や利用制限を回避するためのものではない。** 実際の作業内容を隠したり、検証範囲や技術的事実を書き換えたりしてはならない。

また、表現の調整によってRed Teamの独立性、検証の厳密性、findingの報告義務、canonical authority、停止条件を弱めてはならない。

AI実行環境による作業中断が発生した場合は、実際の作業範囲を再確認し、目的と対象をより明確に記述する。それでも制限が継続する場合は、実行環境の正規の手続きに従い、作業を停止または適切な環境へ移管する。

**並行Trackの停止・差替え：** Issueへの作業コメントが0件でも、別チャットで着手済みの可能性がある。Coordinationは変更時に `STOP REQUESTED` と既知/不明の稼働状況を記録し、作業者は次の大きな書込みやPR公開前に現行Issueの停止指示を確認し、停止・保全した範囲を `STOP ACK` として報告する（可能な場合）。**ACK未確認は停止確認済みではない**。GitHubのIssue closeだけで別チャットが自動停止するとは扱わず、必要なら利用者から直接伝える。

### 4.2 Historical design-intent gate（設計判断の必須照合）

**必須手順:** [NewLang_Design_Decision_Procedure.md](NewLang_Design_Decision_Procedure.md)

**旧案索引:** [NewLang_Design_Intent_Ledger.md](NewLang_Design_Intent_Ledger.md)

対象: Track Mのnew/changed source syntax・API・semantic selection、Provisional/Deferredのnormative化、
およびCoordinationによるscope authorization / candidate Draft acceptance。
Track Pの実装が既存canonical規則を変更する場合も同じgateに戻す。
faithful implementation、proofや純粋なmechanical/CI変更は理由付き `N/A` としてよい。

**起動時/preflight:** 現canonical §、関連Ledger ID、過去の設計候補・実験・旧Draft/Issueを確認し、
新規案を `KEEP / INTENTIONALLY REPLACE / DEFER` から選び、理由と互換性・no-foreclosure影響を記録する。
Ledgerに載っていないことは「旧案が無い」という根拠ではない。

**独立Coordination review:** 必要な履歴比較が無い、旧案を無説明で置換した、
または `N/A` が設計変更を隠しているsource/API/semantic candidateは **BLOCK** する。
台帳の該当entryは設計変更PRと同期する。ただしledgerはauthorityではない。
PR作成時には`.github/PULL_REQUEST_TEMPLATE.md`が照合記入を促す。
**PRテンプレートを記入したという形式だけではreview完了としない。**

**Red Team first-pass独立性との両立:** §7を優先する。
blind R reviewerにはfirst-pass disposition前に本ledgerや旧M/P rationaleを読ませない。
M/Coordinationがpreflightを行い、R first-passの独立性を損なわない順序で照合する。

### 4.3 Goal Alignment Gate（重要な投資判断に限定）

**目的：** §4.2は過去の設計・意味論との整合性を問う。このgateは別に、**その技術的手段を採ることが製品目標にとって必要・適切か**を問う。技術的にGOでも、製品投資のGOとは限らない。

**C後継ターゲット適格性（§4.3の一部）：** 大きな製品向け投資では、まず実利用者のC採用理由と、stable Rust／既存crate／局所unsafe＋safe facade／FFI等で本質的な要求を**機能だけでなく、自然な構造・API・変更・保守の総負担でも**無理なく満たせるかを明記する。**同じ動作のRust CHECK／native PASSだけでは実用上の十分性を判定しない。** 必須の物理条件と偶然のC内部表現を分け、現実的なRustの安全性上の利点もCの人間によるUB監査費用も数える。**Rustも実用的であることだけでは、実際に重要なCの採用理由を棄却しない**。Rustに明確な制約がありCが有利な用途も、両言語とも合理的だがCに重要な積極的利点がある用途も、ターゲット候補となる。Cがほぼ惰性で使われ重要な積極的利点が確認できなければ優先度を下げる。Cの合理性を認めるだけでNewLang Product GOにはしない。Cの合理性が残りNewLangで実用契約を維持できる場合に、**Cを主要baselineとして**作者＋普通safe callerのUB防止監査・再監査依存閉包の改善を判断する。Rustの存在や`unsafe`だけで棄却／採用せず、移行費用と新規開発の制約も区別する。設計変更による追加負担やAI支援後に残る不自然さは具体的なsource／API／保守変更から観察し、工数は実測しなければ未測定とする。実装方式を選ぶ前に実利用者の**operation別**計算量・storage/churn・所有／ABI・safe/unsafe責任を必要な範囲だけ固定し、緩和した要件は別workloadとして区別する（V #563のC O(1) detach→Rust O(n)を、動作一致だけで同等と判定した比較上の欠落を繰り返さない）。実測していない速度やAIの設計選択原因は断定しない。既存4行Goal/Evidence/Alternatives/Stopに吸収し、新しいformや全Track追加承認は作らない。新規ターゲットの初期探索は価値未実証でも限定的Exploration GOになり得る。

**適用時：** Coordinationが新しい大きな checker/proof/source mechanism やsource意味論への投資を認可する前、または同じ製品目標で2種類目のprecision workaround、専用条件の増殖、製品上の改善がない技術Issueの連続などの兆候があるとき。初回の大きな機構提案も対象。回数は兆候であり機械的な拒否閾値ではない。通常のbug fix、既存scopeの実装・テスト、純粋な小規模研究では省略できる。**研究結果を製品に必要な次工程へ昇格する時点**では再確認する。

新しい審査文書やformを作らず、**対象のIssueまたは親Issueに4行程度**をCoordinationが記録する。

1. **Goal**：どのuser-facing価値・元の制約を改善するか。
2. **Evidence**：CAPABILITY（機構の成功）、SAFETY（静的拒否）、PRODUCT（実workload）、BURDEN（人間の責任）で何が実測・未測定か。
3. **Alternatives**：checked・既存API・局所 `unchecked` / library境界・source-surface gap・native検証・pivot等の**関係する選択肢**と最小の反証。無関係なら理由付きN/Aでよい。どの案も事前に正しいと仮定しない。
4. **Stop / owner**：何が出たら投資判断を戻し、誰が次の裁定をするか。

**Vの警告とCoordinationの応答：** Vはmaterialな懸念があるときだけ、報告冒頭で `VALUE-ALIGNMENT-RISK` と明示し、「元の価値／今回の実証範囲／未検討の代替案／残るhuman burden／求める判断」を**一段落**で伝える。必要に応じ `HOLD PRODUCT PRIORITY`、`GO TECHNICAL RESEARCH ONLY`、または懸念なしを区別する。Vには実装停止・設計裁定の単独権限はない。Coordinationは**次の該当する機構投資の認可前**にIssueで `ACCEPT` / `REJECT（理由）` / `DEFER（停止条件）` を一度記録し、警告を無応答で黙殺しない。

**軽量さと検証：** 全Issue/PRへのV sign-off、毎週の定例、Compass読了チェックを設けない。最初の数件の重要判断（H1以外も含む）で、比較が**実際のscope・順序・停止**を変えたかと手続き負担を短く振り返り、効果がなければ簡素化・廃止する。§7のblind R独立性と既存§4.2の履歴照合は維持する。

**外部協力・公開の判断境界：** 外部プログラマーにまず技術的な価値を返すこと、自発的な相互協力、library品質と内部PoC／外部評価／製品価値の証拠区分は[Product Strategy §1.4](NewLang_Product_Value_Strategy.md#14-製品価値の長期実証と外部協力)を単一の正本として参照する。技術的採用・独立R/F検証・Product GOは既存の権限と本Processの条件で判断する。**Coordinationは個別の対外文面・時機を編集判断し、実際の公開・連絡はユーザーの明示的選択なしに行わない。** 相手の同意・公開可否・批判・無反応を尊重し、新しい全Track承認、必須KPI、公開gateを設けない。

### 4.3.1 Exploration GO and Product GO

NewLangの不完全な実装地図を改善する**探索投資（Exploration）**と、利用者に届く機能へ育てる**製品化投資（Exploitation）**は、目的とGO条件が異なる。両方とも最終的なC後継という目標に従うが、**製品価値が未実証という理由だけで探索を止めない**。Track Pのproduction実装も、選択済みv0意味論を実際に使える形へ広げるなら探索投資になり得る。

**再利用可能なcompiler能力への積極投資：** 個別North Star／狭いworkloadの製品価値が未実証でも、選択済みcanonical semanticsに忠実で、他のsource・workloadにも使える見込みが具体的で、予算・最初の拒否境界・期待負例を示せるcompiler/source/checker/公開owned/C17/native拡張は、**Exploration GOとして優先的に認可し得る**。この実装によって次に実測できる製品仮説が増えることも情報利得である。小さな基盤機能ごとに競合言語の不十分さやCに対する監査負担改善のProduct PASSを事前に証明させない。ただし選択済み仕様の名目だけで専用fixture profileや高価なpublic validatorを無制限に積まず、再利用性・保守費用を確認する。

**投資対象の区別：** (A) 既選択の意味論を忠実に実装する小さな再利用可能な縦の拡張は、soundなauthority、実source/native証拠、具体的な再利用先、限定費用と最初の障害で判断する。(B) 新たな一般source規則／ownership／effects／proof／encapsulation等の高費用な追加は§4.2の歴史照合と§4.3のGoal/Evidence/Alternatives/Stopを適用して慎重に段階投資する。(C) 実用製品への昇格は下表のProduct GOを別途満たす。**BのHOLDは無関係なAの停止でもなく、AのPASSはB/Cの自動認可でもない。** P/M/F/Rの安全な実装研究とVのC合理性・NewLang対Cの監査局所性検証を並行して進め、結果を互いに反映する。

**資源と安全性：** 再利用可能なcompiler開発を資源配分の重要な中心に据えつつ、独立R/FとVを維持する。固定の60/20/20比率、新しい承認書式、全Track必須sign-offは設けない。新しい公開証明・権限issuer／provenance／Allocation／LifetimeDomain・OOM/rollback／state bridgeは引き続き厳格に検証し、独立R §7等を適用する。**有限技術PASS、soundness、Product GOは別の証拠階層**であり、安全性を犠牲にして投資を前倒ししない。

| 判定 | GOの根拠 | 獲得・確認する成果 |
| --- | --- | --- |
| **Exploration GO** | 実装可能性・必要な解析精度・最初の不足・工数等について、結果が**次の探索経路の選択を変え得る**未解決の問いがある。費用を限定し、失敗しても有用な情報を回収できる。 | 事前の予測と実測の差、source/checker/owned evidence/backend/nativeの最初の到達・停止点、別workloadにも使える知識・能力・反例 |
| **Product GO** | 二段階で判断する。(1) 実際の利用者の要求について**Cを選ぶ具体的かつ重要な積極的理由**を示し、GC/Rustの最善の現実的代替（局所unsafe／safe wrapper／RAII等を含む）も、機能だけでなく設計・API・変更・保守の費用まで公平に評価する。**Rustも十分に実用的であることはC合理性の自動棄却条件ではない**。明確なC優位と、両言語とも合理的でCにも重要な利点がある場合を候補に含め、惰性しかないC採用は低優先度とする。Cの安全性レビュー費用やRustの安全性便益、利用者にとっての重要性も考慮する。(2) その要求を保持するNewLang-native実用操作が、十分にfactorした同等Cに比べ作者とordinary-safe callerのL/R/P/V/C監査・再監査依存閉包、安全性、保守費用を改善することを説明できる。Rustへの監査優位そのものはGO条件ではない。Zig等は追加言語が必要かの代替確認に使える。 | sourceから実際の使用結果までの証拠、必要な独立検証、**動作成功と開発／保守負担の証拠の区別**、Cとの実用契約・総負担の差と製品化に必要な維持・品質条件 |

**探索を決めるときの最小記録**は既存Issueの本文・コメントでよい。(1) 予測する最初の障害と、結果でどう優先順位を更新するか、(2) 比較する低コストな代替経路、(3) 実測方法と再利用範囲、(4) 作業・変更範囲・撤退条件を短く書く。完了時は**予測→実測→地図の更新**を残す。未達の製品目標を「実験の失敗」と同一視せず、反対に既知の成功を形だけ反復してテスト数を増やすことも探索成果としない。

**安全性と昇格の境界：** CHECK / NATIVE / FORMAL / EXPERIMENT / PRODUCTは異なる証拠階層であり、探索成功・CI green・Pythonモデルの成功を新しいsource規則、soundな権限、mainへの採用、製品PASSに読み替えない。精度不足は保守的に拒否し、`unchecked`やCopy pointerからAllocation/Domain/生存証明を創出しない。既存canonical規則の忠実な有限実装は、新しいProduct GOを待たずにCoordinationが限定認可できるが、**大きな新機構・投資の拡大には本§4.3**、source/API/意味論の選択・変更には**§4.2**を適用する。成果を製品化へ昇格するときは別途Product GOを判断する。

新しい審査会、独立した申請書、全Track必須sign-offは設けない。Coordinationが探索と製品化のGOを区別し、費用上限と次の裁定を決める。仕様と安全性要求はどちらのGOでも不変とする。

**当面の運用裁定 — v0 compiler-first / Product GO能動判定の一時保留（[Issue #624](https://github.com/wakairo/NewLang_Compiler/issues/624)、2026-10-19）：** 通常の開発優先順位はPによるproduction v0 compilerの**普通のCLI経路**（source→CHECK→必要なsource-issued公開owned→strict C17/native）と、診断・負例・失敗時のclean-up・再現可能性に置く。test-only opt-in、fixture専用の緑、未マージ機構は通常利用可能とは数えない。Mは実装中に発見した最小のsource/semantic gapを裁定し、F/Rは安全上重要な真正性・権限・負例を必要な独立性で検証する。Vを廃止せず、実際の利用・保守・代替から得られ、投資判断を変える事実を扱う。ただし**v0通常利用の次工程ごとにProduct GOの机上の再採点・新たな市場／競合調査を必須gateとすることは一時停止**する。既存の§1.1二段階判定とこの節のProduct GO表は凍結して保存し、判定保留をProduct PASS／全面的な実装GOに読み替えない。新しい高額なowner／proof／encapsulationやsource ruleは従来通り§4.2・§4.3で個別に許可し、§7 blind R／fail-closed／OOM・rollback等の証拠基準を緩めない。

**再評価の実物checkpointと早期STOP：** 最初の通常`newlangc`から、既に採用された機構を組み合わせた**新しい小規模な利用者風のsingle-semantic-unit program**についてSOURCE／semantic CHECK／必要な元source-issued owned／実C17→GCC/Clang nativeを追跡でき、意味のある負例・診断・エラー／cleanupと使い方を示せた時点で、Coordinationはその**成功・不自然さ・未実装境界**を用い、能動的Product GO評価をいつ何に対して再開すべきか裁定する。到達前でも権限偽造／根本的soundness問題、通常経路に不可欠な設計矛盾、費用・scopeの急増、強い具体的利用者反証でSTOPして再裁定する。初回CLI技術CHECKPOINTは製品価値・一般soundnessの合格ではなく、H1 #268 HOLD／UNDECIDEDはそのまま。新しい時限KPI、固定資源比率、全Track追加承認は設けない。実際の初回作業は[Roadmap](NewLang_Development_Roadmap.md)の最新checkpointに従い、個別Issueで費用と停止条件を定める。

### 4.3.2 v0 compiler completion の実装優先順位（継続的な方針）

**正本と適用範囲：** この節は、v0 production compilerを実際に作って設計の良否を学ぶ段階における、**Coordinationの長期的な優先順位判断基準**である。具体的な次の機構・Issue番号・順序・予算・到達状況はここで固定せず、[Development Roadmap](NewLang_Development_Roadmap.md)の**日付付き現行実装計画**で更新する。個々のIssueとPRは実装・検証の認可単位であって、この節やcanonical specificationを上書きしない。

**完成に向けた基本方針：** 当面の主目的は、[CURRENT_SPEC](reference/CURRENT_SPEC.md)が選ぶ**v0の実装対象となる意味論の核**を、普通の利用者がsingle semantic unitでsourceを書き、semantic CHECK、必要な真正のsource-issued evidence、backend生成、実際のnative実行・異常系確認まで使えるproduction compilerとして実装することにある。実装で見つかった不自然さ・反例・費用・実装不能性は、言語設計を改めるための**積極的な知見**であり、小さな実装のPASSを増やすこと自体をゴールとしない。現時点の有限source profileと、意味論上必要な仕組みを混同しない。**既選択のv0機能を原則実装予定とする**一方、Draftの明示的なDeferred／v0非目標を自動的に実装対象へ昇格させない。v0として「必要な能力」かどうかと、既存compilerが対応済みかどうかは別々に棚卸し・裁定する。

**優先順位の決め方（上から順）：**
1. **依存関係／完成へのcritical path：** v0の必要能力Bが能力Aに依存するなら、原則Aから進める。低費用・短期成功・CI greenだけを理由に依存先Aを迂回して周辺の便利機能を繰り返し選ばない。A/Bの必要性・真の依存関係が未確認なら、まずその事実を絞って調査する。
2. **共通性と情報利得：** 同じ前提を満たす代替実装からは、複数の独立sourceや次のv0機構に再利用でき、core/source/issuer/backendの本当の問題を反証可能にする案を優先する。fixture専用のgate、数値・field順への特例や証拠の後付けを「進捗」と数えない。
3. **費用と運用負担：** 工数、CI、保守・検証費用、競合する作業は、**実装の分割・順序・予算・代替方式の選択**に使う。**必須機構の見積費用が大きいという理由だけで、その機構を無期限に延期・回避するのは不適切**である。ただし高コストの実装を無制限に一括認可する意味でもない。必要な投資を小さなsource→proof→backendの検証可能な工程に分け、各工程の停止条件と次の裁定主体を明確にする。

**NO-GOの解釈：** 「既存機構を少量の変更で再利用する案はNO-GO」「ある外部ABIやfixtureは優先しない」は、**その案に対する判定**であって、その背後にあるv0の必須能力までNO-GOとする根拠ではない。根本の設計矛盾、真正な権限を発行・保存できない問題、必要性の反証、v0範囲の再選定などはCoordinationへ戻して、仕様・構成・開発順序を改めて裁定する。単なるLOC見積増加や予定超過だけを言語機能の撤退理由としない。

**安全性・変更境界：** 選択済み意味論の忠実実装と、source規則・公開proof issuer・Allocation/Domain・新しいtrusted evidenceの**設計／権限境界の変更**を区別する。後者は§4.2の履歴確認と§4.3 Goal Alignment、必要なF/R独立検証を経て段階的に認可する。fail-closed、no authority amplification、異常系・OOM/rollback・真のorigin一致を緩めて実装を完走したことにはしない。費用やリスクが大きければ**一時停止して方式と予算を再裁定**するが、それだけでv0 completionのcritical pathから消さない。Product GO・Cに対する監査局所性の実証は引き続き別の証拠層とする。

**更新規律：** この長期的な優先順位原則を変えるときだけ本節を明示的に改訂する。優先する具体機構・依存関係・候補順・実装範囲・評価checkpointが変わるたびに本節を変更してはならない。最新の仮説・未確認点・決定に至った実測はRoadmapへ日付と固定authority付きで置き、以前のNo-GoやCIの証拠を遡及的に書き換えない。

## 5. Finding分類

必要に応じて次のような分類を用いる。

```text
CORE-SEMANTIC-GAP
SOURCE-SURFACE-GAP
COMPILER-PRECISION
DIAGNOSTIC-QUALITY
LIBRARY-DESIGN
RUNTIME-IMPLEMENTATION
LOCALIZED-UNCHECKED-BOUNDARY
DEFERRED-BUT-ADDABLE
CONVENIENCE-ONLY
FORMAL-HOLE
FORMAL-AMBIGUITY
```

classificationとseverityは分けて扱う。
例えばcompiler precisionの不足は、自動的にlanguage-design問題を意味しない。

## 6. Semantic Sync Review

重要な区切りでは、新しいsemantic inventionを一時停止し、**Semantic Sync Review**を行う。

最低限、次を確認する。

- canonical Draftとmain SHA
- M milestoneの状態
- Pの実装coverage
- Fのproof coverage
- 未解決のcontradiction / precision gap
- 次milestoneに関係するDeferred項目
- Vのcurrent value hypothesis / product evidence / strategic coverage risk
- MがP/Fより先行しすぎていないか
- 次のcross-track sequence
- 次のsource/semantic milestoneに関係するLedgerの未決定・Deferred項目と§4.2照合状態

典型的な実施時点は、major M milestoneのclosure後、または新しいmajor semantic familyを開始する前とする。

## 7. 独立Red Team

Red Teamは、同じ人間・AI・設計上の前提がM/F/Pへ横断的に入り込むことで起こるcommon-mode failureを減らすために置く。

### 7.1 独立性の基本

Red Teamの初期入力は意図的に狭くする。

```text
canonical Draft
+
selected representative workloads / attack questions
+
必要最小限のrepository context
```

初回attackの前には、原則としてM/F/Pの結論、設計意図、closure report、過去の議論を先に読ませない。
既存trackのframingをそのまま継承する前に、独自に仕様を読んで壊すためである。

**独立性は、最初に書いたテストコードを一字も修正しないことではない。**
独自に立てた検証の問い・期待する意味論・観測の来歴を守ることが本質であり、
検証機材の不備の自律修復は§7.6に従って行う。

### 7.2 Attack phase中のcommunication

初回attack中は次を原則とする。

- Red TeamはM/F/Pとの継続的なdesign discussionへ参加しない。
- Draft自体が本当に曖昧な場合は、事実確認の質問をしてよい。
- 質問は原則としてoriginating trackへ直接投げ続けず、Coordinationを経由する。
- Coordinationは可能な限りcanonical textへの参照で答え、設計を説得するための長いrationaleは先に与えない。

これは完全な情報隔離を目的としない。
Red Teamが自分自身のmodelとfirst-pass findingを作るまで、project側のpreferred explanationへの露出を遅らせるための運用である。

### 7.3 First-pass finding後

Red Teamがfirst-pass findingsを記録した後は、必要に応じて次を許可する。

- M/F/P reportやexperiment evidenceを読む
- apparent problemが既存evidenceで既に反証されているか確認する
- M/F/Pがspec text、proof、implementation evidenceで反論する
- Coordinationが最終的に裁定する

Red Team findingも証拠であり、それ自体がnormative decisionではない。

### 7.4 Red Teamを使う時点

minor editごとにfull Red Team reviewを行う必要はない。
例えば次のようなhigh-value boundaryで使う。

- major semantic milestoneを始める前
- M8のような大きなclosure後
- substantially broader Draftへ進む前
- risky mechanismについてM/F/Pが不自然なほど容易に同意した場合
- lifetime / authority / provenance / raw memory等へ広いblast radiusを持つmechanism

### 7.5 Red Team promptの基本

例えば次のように依頼する。

> canonical Draftを独立したsystems-language reviewerとして読んでください。過去のproject decisionが正しいとは仮定しないでください。現在設計を壊す具体的workloadまたはinvariantを探してください。仕様gapとcompiler/library/runtime上の制約を分離してください。具体的failureが必要としない限り、先にlanguage redesignを始めないでください。

「現在案が良いことを確認してください」という依頼は避ける。


### 7.6 独立実測の自律修復と証拠の保持

**重要なのは、テストの独立性を守ることであって、テストコードを一度も
修正しないことではない。** Red Teamの独立性は、検証の問い、根拠となる
canonical、予測した安全性・意味論上の結果、観測の来歴、結論の形成過程にある。
最初のテスト入力・ハーネスに誤りがなかったことを意味しない。
format違反や接続ミスだけで毎回停止・新Issue起票とすると、独立検証のための
手続きが実測を妨げる。Rは事前に認可されたscope内で、**自分の検証機材の
本質的でない不備を自ら修正し、再検証まで進めることを原則**とする。

#### 守るものと、修正してよいもの

- **変えないもの**：対象compilerとcanonicalの固定authority、検証する
  safety / semantic invariant、元の入力について独立に立てた予測、発行元・
  root / field / pointer等のidentityとauthorityに関する要求、異常系の
  reject条件、初回の実測ログおよびfinding。期待した結果に合わせて
  compiler・issuer・validatorやoracleを書き換えたり、guest stateを
  observerから作ったり、sanitizer・警告・検査を緩和したりしない。
- **Rが自律修復してよいもの**：Rが所有するformatter・CMake・test登録、
  build / link、公開APIの呼出名、CLI引数、テストドライバ、ログの解析、
  読み取り専用observer、checkout / CI設定等の機械的な誤り。
  **構文・綴りだけのsource修正**も、検証したい操作・安全性条件と期待する
  guest値・物理identityを変えない場合には可能とする。
  これらは修復の例であって、列挙されない類似の不備を一律停止にしない。
- **sourceの訂正と独立性**：sourceのバイト列を修正したら新しいSHAと
  修正理由を記録し、修正前のsource・parser / CHECK結果・ログを保存する。
  元の入力が構文違反なら、それは「元の入力の拒否」であり、
  修正後に通った結果で元の入力の判定を上書きしない。
  修正後のsourceについて、実行結果を見る**前**に独立した期待値を明記する。
  sourceが合法になったことはbackend/native安全性のPASSとは別である。
  構造・操作・期待値が実質的に変わる場合は、単なる修復と呼ばず、
  新しい検証ケース・oracleとして区別し、scopeと情報隔離を再確認する。
- **BLINDとcalibratedを区別する**：BLIND first-passでは、修復中も§7.1–7.3の
  M/F/P情報隔離を維持する。固定mainの公開文法・headers、R自身の既存ログ、
  実行前に自ら用意したmock入力などで検証機材を直すことは、producerの
  正解や設計意図を取り込むことと同義ではない。
  しかしproducerのsource・oracle・期待結果を参照してcaseを直したなら、
  その後の試行をfresh BLINDと呼ばず、明示的にNON-BLIND calibratedとする。
  初回判定や以前のBLIND証拠を遡及的に書き換えない。

#### 修復から再実測までの手順

1. **まず故障層を特定する**：source syntax / semantic eligibility、
   R-owned harness / formatter / CI、公開checked-owned、compiler proof /
   backend、C17/native、sanitizer、実行環境を分け、実際に到達した段階と
   NOT TESTEDを記録する。R自身のテスト準備不備をcompilerのfindingとせず、
   逆にcompilerの予想外の振る舞いを「ハーネス不備」と決めつけない。
2. **初回証拠を保存して修復する**：失敗したsource/commit/SHA、コマンド、
   診断、CI job、観測値を残し、同じ問題を再現する小さな既知入力や
   静的確認で修正内容を点検する。R所有の変更と元の予測を区別して記録し、
   目的の観測点まで同じscopeで実際に再実行する。修復コードが存在するだけ、
   buildが通っただけでは目的の検証をPASSとしない。
3. **GitHub Actionsでは無駄な起動を避ける**：専用branchで修復が複数fileに
   及ぶ場合は、まず差分を揃えてから**一まとまりのcommit**としてpushする。
   開いたPRへのfile別逐次更新は各pushでActionsを起動し得るため、
   意図しないCI消費を防ぐ。失敗・修復・再実行を同一Issue / PRから追跡できる
   ようにし、途中のrunや失敗を隠さない。使用する実行方式
   （GitHub branch + Actionsか、認可されたlocal環境か）もIssueに明示する。
4. **予算は手段であり目的ではない**：各Issueで修復可能な範囲、実行資源、
   観測checkpointと停止条件を事前に決める。ただし**特定の修正回数を
   この正本の普遍的な上限・独立性の定義とはしない**。format等の軽微な
   不備で直ちに停止することを既定にせず、自律修復できる余地を実行計画へ
   含める。明示されたCI / scopeの予算を超えて勝手に続行はせず、
   必要ならCoordinationへ予算変更・環境変更を求める。
5. **最終判定を分離する**：最終報告は修復前の結果、修復差分、
   修復後の実測、到達しなかった層、独立に得た結果とproducerによる
   校正材料を区別する。正常系や一部jobだけの成功を全体PASSとせず、
   first-passとcalibratedの証拠階層を混同しない。通常の報告checkpointは
   CIの各回ではなく、source→CHECK→真正なowned→C17/native
   （適用時はnegative/sanitizer）までの有限の実測区切りとする。

**直ちに停止・裁定へ戻すもの**は、予測に反するsemantic CHECK、
checked-owned / proof / authority、C17/native、実際の安全性違反・
unsoundness疑い、発行元や元のroot identityの不整合、Rの検証機材だけで
説明できない差異、BLIND情報隔離の破れ、修復によるoracle・scope変更、
対象compiler / canonicalの変更が必要な場合、および明示予算の超過である。
これらを期待値変更やテスト除外でPASSに寄せてはならない。

この方針は**検証対象の失敗と検証機材の失敗を分離しながら、
Rが実測まで自律的に到達するための運用原則**である。
§7.1–7.3の独立性を緩めず、Rによる有限のPASSを一般soundnessや
Product GOへ昇格させない。最終的な受理・merge・次scopeはCoordinationが裁定する。


#### 新規RのDRAFT PR前のR-FAST準備（forward-only、単一の運用規則）

**適用とCoordination RELEASE：** 採用済みの `.github/workflows/r-preflight.yml` を
固定P候補が含む**今後の新規R**では、通常Chat＋R専用GitHub branch＋Actions
（P同等のlocal実行環境は使用しない）による**PR前R-FASTを既定の必須準備**とする。
Coordinationは対象Issueと起動promptに、immutableなP候補の**完全SHA**
（main基点も該当時に固定）、canonical、`r-fast/r<issue>-<purpose>` の専用branch名、
R独自source・negative・oracleの凍結方法とSHA、廉価FAST push試行の個別上限、
**別予算の自然なFULL5＋NO-LLVM PARTIAL各1組**、最初の安全性STOPを明記する。
Rは§7.1–7.3のBLINDならproducerの実装・fixture・oracleを見る**前**に
自身の入力・予測・SHAを凍結し、後の較正をfresh BLINDと混同しない。
Issue RELEASEはRの自動起動・新権限の授与・merge許可ではない。

**`r-fast/**` ref作成時のFAST算入と、推奨する一回目のref作成順序：**
`r-fast/**` という**branch/refの作成そのもの**がGitHubの `push` eventになり、
採用済みR-FASTを**直ちに起動し得る**。R専用sourceを追加する前に
固定P候補HEADのSHAへbranchを作った場合、その元P HEADでのFASTも
**実際の1 runとしてそのIssueのFAST予算に算入**する（準備前・baseline・
no-opと呼んで除外しない）。限られた予算を有効に使うため、標準順序は
(a) §7.1–7.3に従ってR独立source／negative／oracleとSHAを先行凍結、
(b) 承認済み**完全固定P候補SHAを親**とするR所有blob／tree／
初回完全test+最小CTest登録の**commit objectをrefなしで作成**し、
(c) その**完成済みR commitの完全SHAを直接指す**新しい
`r-fast/r<issue>-<purpose>` refを**初めて作成**する、である。
先にP HEADを指す空branchを作ってからR commitへ更新する二段階を
標準としない。これはGitオブジェクト／refの作成順序であり、
特定のCLIコマンドやP同等local実行を要求せず、BLIND中のP実装・
fixture・oracleの参照を認めない。GitHubの利用可能なUI／接続手段が
refなしcommit→そのcommit直指し新refを提供せず、
P HEADからのbranch作成しかできない場合は、**作成前**に
Coordinationがbaseline FAST 1回分を含む追加予算／明示例外と
代替手順を当該Issue・起動promptに承認するか、STOPする。
歴史的R branchのref・source・oracleはこのために変更しない。

**branch全体のActions台帳とSTOP：** branch作成の直後、追加pushの前、
さらにR DRAFT PRの前に、**その`r-fast/**` branch/refに対する
全期間のR-FAST workflow `event=push` run**を一覧・ページ跨ぎで確認し、
**最初のbranch-create P HEAD run、過去のR HEAD、queued・cancelled・
failed・successを含めて** run URL/ID、head SHA、branch、event、
run_attempt、status/conclusionを保存する。単一commitの
`fetch_commit_workflow_runs(R HEAD)`等のHEAD絞り込みは
**branch全体のFAST予算管理には使用できない**。実際に発生した全run数
（branch作成由来を含む）をIssue固有の有限FAST上限と照合し、
許可済み残枠・§7.6機械修復の規律・先行凍結を満たさなければ
次のpush／PRをSTOPする。初回P HEADでのGREENは、R所有testが
そのHEADにまだない以上、今回R sourceのpreflight成功ではない。
**採用可能なpre-PR FAST成功は、今回R所有source／登録が揃った同一の
R HEAD SHA／branchで実行した`completed/success`だけ**である。
以前のBLIND／calibratedの判定と原SHA／失敗runは不変とし、
個別Issue予算を超えた場合も事後的にrunを数え直して免責しない。
§7.6の修復回数に全project共通の2回制限は設けず、
Coordinationの明示例外、独立性および通常FULL5＋PARTIALの
別予算・実実行義務を引き続き適用する。

**PRより先に確認する実物：** Rは承認済みの固定P候補HEADを起点に
上記接頭辞の**新規**専用branchを作り、凍結したR所有source／harnessと
最小CTest登録をまとめたcommitを**DRAFT PR作成前にpush**する。
GitHub Actionsの `R-FAST self-service pre-PR preparation (NOT R validation)` について、
実際の `event=push`、`refs/heads/r-fast/**` に該当するbranch、pushした
**同一の不変R HEAD SHA**、run URL/ID／attempt、`completed/success`、
固定Clang **23.1.3** formatter、GCC **frontend-only host build**、
`ctest -N` の一意な登録一覧と**今回のR test名**を確認・記録してからPRを開く。
単なるworkflowファイルの存在、queue中／cancelled／missing／failed、
異なるHEAD/refの古いGREEN、PRのFULL CI、`ctest -N` の名前だけでは
**R-FAST成功と認定しない**。runがない・一致しない・失敗した時点で
**NOT VERIFIED／STOP**とし、§7.6の事前承認済み修復予算で扱える
機械的REDだけを元のSHA・ログ・結果を保存して独立に修正・再push・再確認できる。
それ以外や予算外はSTOP→Coordination。source修正なら別の新SHAと
**再実行前の独立した期待値**が必要で、V1の拒否や初回BLIND gradingを
V2の成功で上書きしない。修復の普遍的な「一回限り」制限は設けない
（Issueごとの有限予算と本§の自律修復・STOPが優先する）。

**証拠階層・後続：** 現行R-FASTが直接実施するのは
厳格な書式・host build・CTest名の**登録検査だけ**で、
`ctest -N` は**CTestを0件実行**する。NewLangのsource semantic
`newlangc --check`、真正なsource-issued owned/public proof、C17/native、
source negative、到達OOM、ASan/UBSanや本来のFULL5＋PARTIALを
合格と認定する機能ではない（build時にR独自targetが動作しても、
それは別途実行・ログ・証拠範囲を明示する）。
FASTの実SUCCESS後に、固定P候補をbaseとする**R DRAFT PR**を開き、
通常の**FULL5のGCC／GCC Release／Clang／ASan／UBSan 5つの実行ジョブ**
＋**NO-LLVM PARTIAL 1つの実行ジョブ**を独立の自然CIとして確認する。
6ジョブの実実行・正しいSHA／attemptと適用される
CHECK→公開owned→C17/native・負例／sanitizerの証拠を別途判定し、
skippedやFAST GREENを代用しない。新規run・manual retry・rebase・
compilerやoracleの修正・Rからのmergeをこの規則だけで許可しない。

**例外と未実装の境界：** 対象候補にworkflowが存在しない旧固定baseなどで
FASTの前提が成立しないときだけ、Coordinationが**個別Issueと起動promptの両方**で
理由・代替手段・証拠と予算・STOPを**明示承認**できる。歴史的R branchへ
workflowをbackport／rebaseして既存source・oracle・CIを変更してはならない。
[T #904](https://github.com/wakairo/NewLang_Compiler/issues/904)の軽量
source CHECKは**別件でRELEASEされた未実装・未採用のパイロット**であり、
現在のR-FASTに存在せず、必須条件でもない。Tによる成功実装とCの採用、
さらにDによる本§改訂があるまでは、source eligibilityをFASTの
成功から推定しない（例：`u8 == u8` の型診断は現行FASTでは発見しない）。

## 8. 相関したvalidationを避ける

各trackは異なる問いを持つ。

```text
V: 誰の何が改善されれば価値があるか？
M: これは最小で実用的なruleか？
F: ruleは内部整合し、期待される性質を形式化できるか？
P: production compilerで忠実に表現・診断・検査できるか？
R: projectのpreferred interpretationを知らずに読むと、何が壊れるか？
Q: 既存の品質証拠に、実質的なリスクまたは誤ったconfidenceを残す盲点があるか？
O: Pythonの独立モデルで、解析に必要な状態と時系列不変条件を最小反例・精度・コストの面から明らかにできるか？
```

全trackを通過することはconfidenceを高めるが、設計の完全性を証明するものではない。

## 9. Closure discipline

milestoneは、指定したrepresentative workload / invariantを通過し、残件がreopen threshold外へ正しく分類された時点でcloseしてよい。

Closureの意味は:

> 現在のv0 targetとevidence setに対して、そのmechanismが十分である。

であり、

> APIやsyntaxが今後一切変わらない。

ことではない。

closure後に新しい問題が出た場合は、より深い矛盾が示されない限り最小のaffected scopeだけをreopenする。

## 10. 現在の運用方針

現在のphaseでは次を優先する。

1. 新しい大きなM milestoneを始める前に、pending P/F milestoneをreview・mergeする。
2. 直近で閉じたspecificationをP/Fが十分pressure-testできる程度にcatch upさせる。
3. substantiveなM9作業の前にSemantic Sync Reviewを行う。
4. そのsync point付近、または同等のhigh-value boundaryでindependent Red Team passを行う。
5. 実際の運用経験からより良いmodelが得られたら、この文書自体を改訂する。
6. **Product GO待ちだけを理由に、選択済みv0の再利用可能なcompiler能力を停滞させない。** 安全な縦の能力拡張と独立R/F・Vを並行させるが、大きな新機構と製品昇格は別裁定する（§4.3.1）。
