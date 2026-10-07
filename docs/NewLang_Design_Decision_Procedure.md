# NewLang 設計判断手順（Historical Design-Intent Gate）

> Status: **プロジェクト運用手順**（言語のnormative specificationではない）
>
> 上位の運用方針: [NewLang_Project_Development_Process.md](NewLang_Project_Development_Process.md)
>
> 過去の設計候補の索引: [NewLang_Design_Intent_Ledger.md](NewLang_Design_Intent_Ledger.md)
>
> 背景: [Issue #153 — 旧設計意図の引き継ぎ監査](https://github.com/wakairo/NewLang_Compiler/issues/153)、[Issue #154 — 再発防止運用](https://github.com/wakairo/NewLang_Compiler/issues/154)

## 1. 目的と基本原則

新しいsource syntax / API / semanticsを決める際、既存の旧案を知らないまま
狭いproduct/implementation gateの便宜で別の規則を「言語全体の当然の選択」として
固定することを防ぐ。

**旧案が新案より正しいとは仮定しない。**
旧案を維持しても意図的に変更してもよいが、比較と理由を追跡可能にする。

権威の順序は従来どおり:

1. `NewLang_Compiler/main` の `docs/reference/CURRENT_SPEC.md` が指すDraftが**現時点のnormative意味論**。
2. review/mergeされた新Draftだけがnormativeを変更する。
3. Design-Intent Ledger、Issue/PR、旧Draft、prototype、chat/researchは**証拠または将来の課題の索引**であり、
   単独では現在のDraftを上書きしない。
4. userの新しい方針は次の裁定の入力となる。確認前に既存normative規則を黙って置換しない。

本手順は **設計決定を行う前のpreflight** と、**独立Coordination review時のacceptance gate** の両方に適用する。

## 2. 適用対象 / 軽量な除外

必須:

- Track M: 新しいsource grammar、API、nominal/visibility/lookup/call/loan/ptr/lifetime ruleを
  選択・変更・禁止・一般化するIssue/PR、およびtargeted reopen。
- Coordination: 上記のscopeを承認するとき、candidate Draftをreviewするとき、
  次のTrack P gateにsource ruleを固定するとき。
- Track P: canonical source language/APIを変更または再解釈する提案、既存の
  bounded profileを拡張する提案（単なるfaithful implementationと区別）。
- 変更が「Deferred」「Provisional」「実験案」だったものをnormativeへ格上げする場合。

軽量な確認でよい:

- 既存canonical ruleどおりのP/F実装・proof・テスト・diagnostic改善でlanguage designを
  変えない場合は、PRに `Historical design audit: N/A — faithful implementation of §X` 等を明記する。
- typo、純粋な文書整形やCI/toolchain変更も `N/A` と根拠を示してよい。
- Product Vのevidence収集自体には全面履歴監査を課さない。ただしVの提案を
  semantic/design decisionへ昇格させる際には適用する。

**独立R first-pass例外:** `NewLang_Project_Development_Process.md` §7のblind reviewでは
Ledger/設計経緯/M-F-P rationaleをfirst-pass前に読ませない。
Rはcanonical Draftと許可された攻撃入力だけで先に判断し、
disposition後にCoordinationが本手順との照合を行う。
独立性を守るために本手順を無効化するのではなく、**誰がいつ読むかを分ける**。

## 3. 手順: scope選定前に実行

### Gate A — 現在のauthorityを固定する

まず現在の `main` commit SHA、`CURRENT_SPEC.md`、参照Draftの関連section、
担当Issueと許可scope / stop conditionを記録する。
product上の根拠が必要ならproject-level V authorityも確認する。

canonicalからの差分が未merge candidateなのか既mergeなのかを区別する。

### Gate B — 旧案と過去の根拠を**探す**

必ず [Design-Intent Ledger](NewLang_Design_Intent_Ledger.md) を開き、
対象のsyntax、operation、semantic categoryのエントリを確認する。
ledgerだけで検索を終えず、必要に応じて以下を狭く検索する:

- canonical Draftの旧revision / revision-introduction historyと関連source section
- 過去のM Issue/PR、Coordination review、R findings / V product pressure
- non-normative Surface Draft、front-end prototype、workload test、設計実験
- userが指定する旧スレッド・ファイル（アクセス可能な範囲でのみ照合）

**チャット全文や旧実験の非アクセスを「旧案は存在しない」の証明に使わない。**
検索対象と未確認部分を明示する。
必要な旧資料が未取得でも作業停止が常に必要なわけではないが、
重要な設計衝突を否定できないときはCoordinationへ戻す。

台帳に対象が見つからなければ `ledger未登録` と記録し、
関連history/Issueを検索した範囲を記す。
`ledgerに無かった` だけで `過去に議論無し` と判断しない。

### Gate C — 旧案と現案を**分類して比較する**

少なくとも次を一つの小さな表または箇条書きとして残す:

| 項目 | 記載内容 |
|---|---|
| 現canonical | Draft/sectionと既存のexact rule |
| 旧案・代替案 | 旧Draft/Issue/レポート名またはLedger ID、当時のstatus |
| evidence class | `normative-current` / `historically-normative` / `experimental` / `proposed` / `Deferred` / `rejected` |
| 裁定 | **KEEP / INTENTIONALLY REPLACE / DEFER / N/A** |
| 理由 | 旧案のメリット・限界、新案を選ぶ具体的根拠 |
| 波及 | source compatibility、grammar/AST、P/F/R/V、ABI/lookup/ptr/privacy等のno-foreclosure |
| 未確認 | 見られなかった資料、影響が評価できない領域 |

**「既存productionがこう実装しているから」だけでは
source design変更の十分な理由にならない。**
targeted bounded profileの仮構文を将来のgeneral source/APIへの唯一の選択と
誤認させない。

### Gate D — candidateとLedgerを同期する

Design変更が本当に必要なときは、通常のtargeted Issue -> candidate Draft ->
PR -> independent reviewの流れを使う。

- 同一scopeに重要な旧案または未解決の後付けリスクがあるなら、
  Ledgerの該当行を更新する（新規IDも可）。
  設計候補の段階では `PROPOSED (unmerged)` と明記し、
  `ADOPTED` はreview済みmerge後だけに使う。
- 過去のexperimental evidenceが現在のspecと矛盾していても、
  「以前こうだった」という履歴を削除せず **SUPERSEDED** として
  何により変更されたかのIssue/PRをlinkする。
- Ledgerの本文をnormative ruleの重複コピーにしない。
  stableなsection / Issue / PRへのリンクと短い判断理由を残す。
- pure implementation-only PRでLedgerの更新が不要な場合はその理由を書けばよい。

### Gate E — Coordination review / merge gate

source/API/semantic選定を含むPRをindependently reviewするときは、
**旧案比較が完成しているかを毎回確認する**。

次のいずれかなら **Coordination: BLOCK** とし、補完後にre-reviewする:

1. 上記Gate B/Cが必要なのに、証拠を伴う履歴照合の記録がない。
2. relevantなLedger IDや旧候補を無視し、差分理由なしにsource/semanticを変更した。
3. `N/A` と主張しながら、実際にはnormativeのsource/API/semantic範囲が変わっている。
4. ledger記述をcanonicalより上位のauthorityとして扱った。
5. 既知の大きいno-foreclosure riskを検討せずbounded production shortcutを一般規則へ昇格した。

本gateが **設計案そのものの採否を自動的に決めるわけではない**。
比較が行われたあとなら旧案を意図的に破棄してよい。
reviewのACCEPT/BLOCK/dispositionは従来どおり記録する。

## 4. PR / Issue に最低限記す欄

設計を変更する場合:

```text
Track: M (or Coordination)
Canonical: <main SHA>, <Draft/section>
Historical design audit: COMPLETED
Ledger: <DI-IDs / ledger未登録 and searched sources>
Earlier options: <what, where, prior status>
Decision: KEEP / INTENTIONALLY REPLACE / DEFER
Reason: <explicit rationale for contrast>
Blast radius / no-foreclosure: <affected and deliberately excluded>
Ledger update: <changed / unnecessary with reason>
Remaining uncertainties: <known gaps>
```

設計を変更しない場合:

```text
Historical design audit: N/A — exact §X faithful implementation; no language rule changed
```

PRテンプレートもこの欄を促すが、テンプレートの有無にかかわらず
**この手順とCoordination review gateが適用される**。
機械チェックがなくても未記入をACCEPTしてはいけない。

## 5. 過去の教訓

- [#153](https://github.com/wakairo/NewLang_Compiler/issues/153):
  旧source surface実験の判断材料が、後続のbounded仕様策定に引き継がれなかった。
- [#150](https://github.com/wakairo/NewLang_Compiler/issues/150) /
  [PR #151](https://github.com/wakairo/NewLang_Compiler/pull/151):
  その問題を再評価して、canonical Draft 17.22で `@` fieldと `::` sumを採用。
- 旧実験のPASSは現在のnormative ruleでも「必ず採用」でもない。
  **存在を知り、具体的に比較し、選んだ理由を残す**ことが本手順の要件。

## 6. 運用上の軽量化

全旧チャットを毎回再読する必要はない。
対象に関連するLedger IDと主要な一次証拠を見て、
新しい衝突・根拠不足があるときだけ検索を深掘りする。

多数のmechanismにまたがる変更はledgerを網羅するのではなく、
affected categoriesを列挙してそれぞれKEEP/REPLACE/DEFERとする。
履歴台帳を無制限に肥大化させず、Issue/PRを根拠本体としてリンクする。
