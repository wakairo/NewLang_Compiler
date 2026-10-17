# NewLang Project Compass — 目的と正本への入口

> **Navigation only / non-normative.** 本書は仕様・製品評価条件を追加・変更しない。
> 言語意味論の正本は [CURRENT_SPEC](reference/CURRENT_SPEC.md) が指定する Draft、
> 製品方針の正本は下記の Product Strategy / North Star 等である。
> 見出しやリンクと正本が異なる場合は、必ず正本を優先する。

## 1. 何を目指すか / 何を目指さないか

- **多言語主義／C 後継**：GC言語・Rust・Cは、同じ用途でも複数が合理的な選択肢になり得る。NewLangは**Cを選ぶ具体的かつ重要な積極的理由**がある用途で、その合法な構造・失敗処理・配備の自由を保ち、UB防止責務を減らすC後継候補（[Product Strategy §§1–3](NewLang_Product_Value_Strategy.md)）。Rustも実用的な場合を自動除外せず、Cの採用が惰性だけなら優先度を下げる。Rustの一般的不可能性を示す必要はない。
- **二段階の製品判断**：最初に**Cを選ぶ積極的合理性**を、実際の要求・構造・API・開発／保守・配備費用から判断し、実用的なRustなどの代替も公平に扱う。**RustとCの両方が合理的でも第1関門の候補となる**。次に**NewLangと十分にfactorしたC**を、同じ実用契約の下でUB防止の作者＋通常safe caller監査・再監査責務によって比較する。ターゲット適格性だけでProduct GOにしない。詳細の正本はProduct Strategy §1.1。
- **Preserve the architecture, replace the bookkeeping**：低水準の構造や pointer topology を保ち、機械的に扱える lifetime / ownership 等の責務を compiler に移す。
- **Policy-neutral, mechanism-strict**：データ構造・アルゴリズムの方針は利用者が決め、言語の低水準 mechanism は厳密にする。
- **全操作の自動証明は必須目標ではない**：証明できる条件は checked、証明できない**真の**事前条件は明示的・局所的な `unchecked` と人間の責任で扱える。ordinary safe API は呼出者へ隠れた証明責任を転嫁しない。
- **製品価値はunchecked件数ではなく監査の局所性**：library authorとordinary-safe callerのUB防止監査・再監査範囲、安全な変更が内部前提を破壊できないことを検証する（詳細は[Product Strategy §1.1](NewLang_Product_Value_Strategy.md#11-ub-prevention-audit-locality-value-hypothesis)）。per-hop uncheckedの多発や権限を無から作る操作は解決ではない。
- **North Star は長期の全用途を定義しない**。個別技術の進歩、仕様上の許容、製品価値の実証を混同しない。

上記は**案内用の要約**。正確な規則・例外・適用範囲は必ず下記の正本で確認する。

## 2. 迷ったときにたどる正本

| タスクで問うこと | 参照先 |
| --- | --- |
| C 後継の狙い、GC→Rust→NewLangの多言語主義、二段階のtarget選定・製品価値、no-foreclosure | [Product / Value Strategy](NewLang_Product_Value_Strategy.md) §§1–6,9,11 |
| CがRustでなく選ばれる可能性の文献証拠・13事例・限界（非規範、未独立検証） | [C/Rust target discovery 2026-10-17](research/C_Rust_Target_Discovery_2026-10-17.md) |
| 今の North Star、topology、per-hop unchecked の例外、L/R/P/V/C、PASS/FAIL | [North Star Product Validation](NewLang_North_Star_Product_Validation.md) §§1,6–11 |
| 長期の C-class workload と現段階の coverage | [C Replacement Coverage Map](NewLang_C_Replacement_Coverage_Map.md) |
| 現在の normative semantics（Draft を決め打ちしない） | [CURRENT_SPEC](reference/CURRENT_SPEC.md) → 現行 Draft |
| checked / `unchecked` / `requires fn` / safe wrapper、正しい Domain stability | 現行 Draft §§8,10,20（[CURRENT_SPEC](reference/CURRENT_SPEC.md) から移動） |
| Ring `head/len`、Vec `len` の metadata SSOT と局所的な claim transfer | 現行 Draft §15（[CURRENT_SPEC](reference/CURRENT_SPEC.md) から移動） |
| 過去案の位置付けと source/semantic 変更前の確認 | [Design Decision Procedure](NewLang_Design_Decision_Procedure.md)、[Design-Intent Ledger](NewLang_Design_Intent_Ledger.md) |
| Track、Issue、レビュー、独立 R、Goal Alignment、停止確認 | [Development Process](NewLang_Project_Development_Process.md) §§3–4,7 |
| 探索投資と製品化投資、Exploration GO / Product GO、予測→実測→地図の更新 | [Development Process §4.3.1](NewLang_Project_Development_Process.md#431-exploration-go-and-product-go) |
| H1 固有の凍結条件（一般的な言語要件とは別） | [H1 Issue #268](https://github.com/wakairo/NewLang_Compiler/issues/268) |
| 今回の目標逸脱と反省点（正本ではなく evidence） | [Coordination #368](https://github.com/wakairo/NewLang_Compiler/issues/368)、[V #369](https://github.com/wakairo/NewLang_Compiler/issues/369) |

## 3. 作業開始時・方向転換時の使い方

1. **関連する行だけ選ぶ**。分からない目標・非目標・代替手段があればリンク先の正本を読む。Compass を引用しただけで理解・審査済みとはしない。
2. **手段の目的化に注意**：新しい checker/proof mechanism が作れることと、製品に必要であることは別。checked、既存 API、局所 unchecked、別 workload 等を必要に応じて比較する。ただし unchecked が source 上表現可能・sound と推定してはならない。
3. **投資判断は [Process §4.3](NewLang_Project_Development_Process.md) に従う**。特に[§4.3.1の探索GO／製品化GO](NewLang_Project_Development_Process.md#431-exploration-go-and-product-go)を区別する。製品価値がまだ未実証でも、次の探索経路の判断を改善する小さな実測や選択済みv0の忠実な実装を否定しない。ただし、探索成功をProduct PASSや仕様・安全性の承認と取り違えない。
4. **blind Red Team の初回判定は Process §7 を優先**。設計者の rationale や過去の優先結論を先に読ませない。

この入口が古い・壊れている場合はリンク先の正本を確認して修正を提案する。Compass 自体を第2の authority にしない。
