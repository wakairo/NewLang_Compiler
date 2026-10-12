# NewLang Project Compass — 目的と正本への入口

> **Navigation only / non-normative.** 本書は仕様・製品評価条件を追加・変更しない。
> 言語意味論の正本は [CURRENT_SPEC](reference/CURRENT_SPEC.md) が指定する Draft、
> 製品方針の正本は下記の Product Strategy / North Star 等である。
> 見出しやリンクと正本が異なる場合は、必ず正本を優先する。

## 1. 何を目指すか / 何を目指さないか

- **C 後継**：C が合理的に選ばれる実用的な systems workload を対象にする。
- **Preserve the architecture, replace the bookkeeping**：低水準の構造や pointer topology を保ち、機械的に扱える lifetime / ownership 等の責務を compiler に移す。
- **Policy-neutral, mechanism-strict**：データ構造・アルゴリズムの方針は利用者が決め、言語の低水準 mechanism は厳密にする。
- **全操作の自動証明は必須目標ではない**：証明できる条件は checked、証明できない**真の**事前条件は明示的・局所的な `unchecked` と人間の責任で扱える。ordinary safe API は呼出者へ隠れた証明責任を転嫁しない。
- **unchecked を増やすことも目標ではない**：頻繁な pointer traversal / mutation のたびに unchecked が必要な構造や、必要な権限を unchecked で無から作ることは解決ではない。
- **North Star は長期の全用途を定義しない**。個別技術の進歩、仕様上の許容、製品価値の実証を混同しない。

上記は**案内用の要約**。正確な規則・例外・適用範囲は必ず下記の正本で確認する。

## 2. 迷ったときにたどる正本

| タスクで問うこと | 参照先 |
| --- | --- |
| C 後継の狙い、product/user value、policy-neutral、no-foreclosure | [Product / Value Strategy](NewLang_Product_Value_Strategy.md) §§1–6,9 |
| 今の North Star、topology、per-hop unchecked の例外、L/R/P/V/C、PASS/FAIL | [North Star Product Validation](NewLang_North_Star_Product_Validation.md) §§1,6–11 |
| 長期の C-class workload と現段階の coverage | [C Replacement Coverage Map](NewLang_C_Replacement_Coverage_Map.md) |
| 現在の normative semantics（Draft を決め打ちしない） | [CURRENT_SPEC](reference/CURRENT_SPEC.md) → 現行 Draft |
| checked / `unchecked` / `requires fn` / safe wrapper、正しい Domain stability | 現行 Draft §§8,10,20（[CURRENT_SPEC](reference/CURRENT_SPEC.md) から移動） |
| Ring `head/len`、Vec `len` の metadata SSOT と局所的な claim transfer | 現行 Draft §15（[CURRENT_SPEC](reference/CURRENT_SPEC.md) から移動） |
| 過去案の位置付けと source/semantic 変更前の確認 | [Design Decision Procedure](NewLang_Design_Decision_Procedure.md)、[Design-Intent Ledger](NewLang_Design_Intent_Ledger.md) |
| Track、Issue、レビュー、独立 R、Goal Alignment、停止確認 | [Development Process](NewLang_Project_Development_Process.md) §§3–4,7 |
| H1 固有の凍結条件（一般的な言語要件とは別） | [H1 Issue #268](https://github.com/wakairo/NewLang_Compiler/issues/268) |
| 今回の目標逸脱と反省点（正本ではなく evidence） | [Coordination #368](https://github.com/wakairo/NewLang_Compiler/issues/368)、[V #369](https://github.com/wakairo/NewLang_Compiler/issues/369) |

## 3. 作業開始時・方向転換時の使い方

1. **関連する行だけ選ぶ**。分からない目標・非目標・代替手段があればリンク先の正本を読む。Compass を引用しただけで理解・審査済みとはしない。
2. **手段の目的化に注意**：新しい checker/proof mechanism が作れることと、製品に必要であることは別。checked、既存 API、局所 unchecked、別 workload 等を必要に応じて比較する。ただし unchecked が source 上表現可能・sound と推定してはならない。
3. **投資判断は [Process §4.3](NewLang_Project_Development_Process.md) に従う**。小修正・既存範囲の実装・独立した研究ごとに重い審査を繰り返さない。
4. **blind Red Team の初回判定は Process §7 を優先**。設計者の rationale や過去の優先結論を先に読ませない。

この入口が古い・壊れている場合はリンク先の正本を確認して修正を提案する。Compass 自体を第2の authority にしない。
