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

**C後継ターゲット適格性（§4.3の一部）：** 大きな製品向け投資では、まず実利用者のC採用理由と、stable Rust／既存crate／局所unsafe＋safe facade／FFI等で本質的な要求を無理なく満たせるかを明記する。Rustで十分ならそのworkloadでNewLangを推すProduct GOを認めない。Cの合理性が残りNewLangで実用契約を維持できる場合に、**Cを主要baselineとして**作者＋普通safe callerのUB防止監査・再監査依存閉包の改善を判断する。Rustの存在や`unsafe`だけで棄却／採用せず、移行費用と新規開発の制約も区別する。既存4行Goal/Evidence/Alternatives/Stopに吸収し、新しいformや全Track追加承認は作らない。新規ターゲットの初期探索は価値未実証でも限定的Exploration GOになり得る。

**適用時：** Coordinationが新しい大きな checker/proof/source mechanism やsource意味論への投資を認可する前、または同じ製品目標で2種類目のprecision workaround、専用条件の増殖、製品上の改善がない技術Issueの連続などの兆候があるとき。初回の大きな機構提案も対象。回数は兆候であり機械的な拒否閾値ではない。通常のbug fix、既存scopeの実装・テスト、純粋な小規模研究では省略できる。**研究結果を製品に必要な次工程へ昇格する時点**では再確認する。

新しい審査文書やformを作らず、**対象のIssueまたは親Issueに4行程度**をCoordinationが記録する。

1. **Goal**：どのuser-facing価値・元の制約を改善するか。
2. **Evidence**：CAPABILITY（機構の成功）、SAFETY（静的拒否）、PRODUCT（実workload）、BURDEN（人間の責任）で何が実測・未測定か。
3. **Alternatives**：checked・既存API・局所 `unchecked` / library境界・source-surface gap・native検証・pivot等の**関係する選択肢**と最小の反証。無関係なら理由付きN/Aでよい。どの案も事前に正しいと仮定しない。
4. **Stop / owner**：何が出たら投資判断を戻し、誰が次の裁定をするか。

**Vの警告とCoordinationの応答：** Vはmaterialな懸念があるときだけ、報告冒頭で `VALUE-ALIGNMENT-RISK` と明示し、「元の価値／今回の実証範囲／未検討の代替案／残るhuman burden／求める判断」を**一段落**で伝える。必要に応じ `HOLD PRODUCT PRIORITY`、`GO TECHNICAL RESEARCH ONLY`、または懸念なしを区別する。Vには実装停止・設計裁定の単独権限はない。Coordinationは**次の該当する機構投資の認可前**にIssueで `ACCEPT` / `REJECT（理由）` / `DEFER（停止条件）` を一度記録し、警告を無応答で黙殺しない。

**軽量さと検証：** 全Issue/PRへのV sign-off、毎週の定例、Compass読了チェックを設けない。最初の数件の重要判断（H1以外も含む）で、比較が**実際のscope・順序・停止**を変えたかと手続き負担を短く振り返り、効果がなければ簡素化・廃止する。§7のblind R独立性と既存§4.2の履歴照合は維持する。

### 4.3.1 Exploration GO and Product GO

NewLangの不完全な実装地図を改善する**探索投資（Exploration）**と、利用者に届く機能へ育てる**製品化投資（Exploitation）**は、目的とGO条件が異なる。両方とも最終的なC後継という目標に従うが、**製品価値が未実証という理由だけで探索を止めない**。Track Pのproduction実装も、選択済みv0意味論を実際に使える形へ広げるなら探索投資になり得る。

| 判定 | GOの根拠 | 獲得・確認する成果 |
| --- | --- | --- |
| **Exploration GO** | 実装可能性・必要な解析精度・最初の不足・工数等について、結果が**次の探索経路の選択を変え得る**未解決の問いがある。費用を限定し、失敗しても有用な情報を回収できる。 | 事前の予測と実測の差、source/checker/owned evidence/backend/nativeの最初の到達・停止点、別workloadにも使える知識・能力・反例 |
| **Product GO** | 二段階で判断する。(1) 実際の要求についてGC/Rustが実用上十分ではなくC選択が合理的であることを、現実的Rust（局所unsafe／safe wrapper等を含む）と公平に確認。(2) その要求を保持するNewLang-native実用操作が、十分にfactorした同等Cに比べ作者とordinary-safe callerのL/R/P/V/C監査・再監査依存閉包、安全性、保守費用を改善することを説明できる。Rustへの監査優位そのものはGO条件ではない。Zig等は追加言語が必要かの代替確認に使える。 | sourceから実際の使用結果までの証拠、必要な独立検証、Cとの実用契約・総負担の差と製品化に必要な維持・品質条件 |

**探索を決めるときの最小記録**は既存Issueの本文・コメントでよい。(1) 予測する最初の障害と、結果でどう優先順位を更新するか、(2) 比較する低コストな代替経路、(3) 実測方法と再利用範囲、(4) 作業・変更範囲・撤退条件を短く書く。完了時は**予測→実測→地図の更新**を残す。未達の製品目標を「実験の失敗」と同一視せず、反対に既知の成功を形だけ反復してテスト数を増やすことも探索成果としない。

**安全性と昇格の境界：** CHECK / NATIVE / FORMAL / EXPERIMENT / PRODUCTは異なる証拠階層であり、探索成功・CI green・Pythonモデルの成功を新しいsource規則、soundな権限、mainへの採用、製品PASSに読み替えない。精度不足は保守的に拒否し、`unchecked`やCopy pointerからAllocation/Domain/生存証明を創出しない。既存canonical規則の忠実な有限実装は、新しいProduct GOを待たずにCoordinationが限定認可できるが、**大きな新機構・投資の拡大には本§4.3**、source/API/意味論の選択・変更には**§4.2**を適用する。成果を製品化へ昇格するときは別途Product GOを判断する。

新しい審査会、独立した申請書、全Track必須sign-offは設けない。Coordinationが探索と製品化のGOを区別し、費用上限と次の裁定を決める。仕様と安全性要求はどちらのGOでも不変とする。

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


### 7.6 独立実測中の検証ハーネス修復

Red Team の独立性を保つことと、テストハーネスの単純な誤りのたびに
Coordination 裁定を待つことは同義ではない。**事前に Issue で認可した
実測checkpoint の範囲内**なら、R は自分で書いた検証環境の明白な不備を
修正し、その区切りまで続行してよい。

- **事前凍結**：対象 compiler/canonical/commit、独立 NewLang source、
  source SHA、予測した正常・異常結果、物理 topology / 安全性 oracle、
  使用可能な公開 checked-owned 情報、専用 branch / CI、最大修正 commit 数と
  CI 実行回数、区切り・停止条件を Issue に記載する。無期限の自動リトライは認可しない。
- **自律修復できる範囲**：R 専用の API 呼出名、正規表現・ログ parser、
  assertion 到達前の検証環境、読み取り専用観測プログラムの明確な実装ミス、
  CI 祖先 SHA / checkout 設定等。**最初の失敗ログを保存し、変更前に
  既存ログ・模擬入力で offline preflight** を行う。各最小修正・commit・CI を
  順に記録し、実測を隠す上書きや都合のよい再実行はしない。
- **自律修復できない範囲**：対象 compiler / Production、独立 source、
  期待する安全性・root identity・pointer 比較・失敗診断、issuer / validator、
  canonical、observer による guest state の書換え、sanitizer / `-Werror` の
  緩和、初回判定前の P/M の結果や oracle の取り込み。これらの変更が必要なら
  修復を止めて Coordination に返す。
- **実質的な停止**：予測に反する source/check、checked-owned、C17/native
  挙動、実際の安全性違反・unsoundness の疑い、独立性の破れ、原因を R の
  ハーネスだけに帰せない不一致、予算超過、権限・依存範囲の変更は、まず
  **FINDING / INCONCLUSIVE として事実を凍結して停止**する。観測条件を
  書き換えて PASS に寄せてはならない。
- **報告 checkpoint**：通常は CI 1 回ごとの停止ではなく、事前に定めた
  source→check→owned→generated artifact→実 native 観測（必要なら
  negative/sanitizer を含む）までの区切り、または上記 STOP で終了する。
  各ハーネス修正と試行回数を audit 可能に残し、**最終 verdict は一度**記録する。
  PASS の範囲は到達した有限条件に限定し、merge / Product GO は
  Coordination が別途裁定する。

Issue が修復権限・予算を与えていない場合、R は勝手に scope を拡張しない。
この節は§7.1–7.3の BLIND 情報隔離や finding の凍結を緩めるものではなく、
**検証対象の失敗と検証機材の失敗を分離する**ための運用である。

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
