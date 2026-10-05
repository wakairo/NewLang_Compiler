# NewLang プロジェクト開発運用方針

> Status: 初版運用方針
>
> この文書は、NewLangにおける複数track横断の現在の開発運用を定める。
> 意図的に軽量な初版であり、projectの経験に応じて今後改訂してよい。
> 変更履歴はGitのcommit historyを正とし、この文書内に別のchange log / Version historyを持たない。
>
> English companion: `NewLang_Project_Development_Process_en.md`

## 1. 目的

NewLangは、部分的に独立した複数trackで開発する。

- **M — 仕様策定 / 設計**
- **F — 形式証明 / 意味論の形式化**
- **P — production compiler実装**
- **R — 独立Red Team review**
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

重要なdecisionがreportや会話にしか存在しない場合は、review済みDraft revisionを経てmainへmergeされた時点でnormativeとする。

UTF-8で問題なく管理できる文書は日本語を基本とし、必要に応じて英語版を併記する。
一方、shell操作やtoolchain上で前面に現れるcommit message等は、互換性・可搬性を優先して英語ASCIIを基本とする。

## 3. 各trackの責務

### M — 仕様策定 / 設計

Mはlanguage semanticsとsource surfaceを検討・裁定する。
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

### Coordination

Coordinationは少なくとも以下を担う。

- authority / version synchronization
- milestone sequencing
- findingのclassificationとseverity整理
- KEEP / CLOSE / TARGETED REOPENの裁定
- Draft revisionの要否判定
- Mを止めてP/Fのcatch-upを待つかの判断

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

一つのboundedなtask / finding / revalidationには、原則として一つのIssueを用いる。
そのIssueには必要に応じて次を集約する。

- canonical repository / main SHA / `CURRENT_SPEC.md` 等のauthority
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
```

後から読んだときに誰の判断・finding・質問か識別できればよく、厳密な機械可読formatは要求しない。

同じscope内の質問、回答、追加証拠、裁定はできるだけ既存Issueのcommentへ追記し、「あのときのprompt」やconversation historyだけを参照点にしない。
scopeが実質的に変わった場合は新しいIssueを作り、元Issueからlinkする。

ただしIssueを細かな発言ごとに乱立させない。
単純な実装上のやり取りや一つのbounded task内の追補は既存Issue / PRへまとめる。

Red Teamについては§7の独立性を優先する。
first-pass attack用Issueには、その時点で許可された狭い入力だけを置き、M/F/Pのrationaleや過去の結論を先回りして混ぜない。
first-pass finding後に追加情報を解禁する場合は、同じIssueへ追記するか、明示的にlinkしたfollow-up Issueで扱う。

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
- MがP/Fより先行しすぎていないか
- 次のcross-track sequence

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

## 8. 相関したvalidationを避ける

各trackは異なる問いを持つ。

```text
M: これは最小で実用的なruleか？
F: ruleは内部整合し、期待される性質を形式化できるか？
P: production compilerで忠実に表現・診断・検査できるか？
R: projectのpreferred interpretationを知らずに読むと、何が壊れるか？
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
