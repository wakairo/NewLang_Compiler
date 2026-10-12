# Track O — 暫定Python意味論・解析アルゴリズム研究Charter

> **Status: PROVISIONAL（暫定運用）**
>
> この文書は[NewLangプロジェクト開発運用方針](NewLang_Project_Development_Process.md)を補う、Track Oの**運用上のCharter**である。mainへreview・mergeされた版を当該運用方針の正本とする。
> **言語仕様の正本ではない。** NewLangのnormative semanticsは一貫して[CURRENT_SPEC.md](reference/CURRENT_SPEC.md)が選択するDraftが決める。
> Oの継続、個別タスク、実験PR、成果の採用・停止はCoordinationがIssueで個別に裁定する。この文書だけで新しい実装・merge・Track起動を承認しない。

## 1. 目的と位置付け

Track O（**Oracle / Python Semantic Algorithm Research**）は、Pythonで意味論的状態表現・解析アルゴリズムを小さく実装して比較し、production C実装より低コストで**反例、精度限界、計算量、最小限の不変条件**を見つける暫定研究トラックである。

- 研究の対象は、neutralなtyped operation/semantic IR、concrete interpreter、独立したfinite abstract interpreter、Value/Occurrenceとfield/rootのcurrentness、変更の時系列・依存関係、分岐joinやknown call summaryの**限定的な候補アルゴリズム**など。
- Cでのimplementation feasibility、保守性やユーザーの記述負担を重視し、抽象的な完全性だけを目指さない。複数の非cJSONモチーフを使ってベンチマーク特化を見分ける。
- Track Oの証拠は**Class O/H（研究・歴史的証拠）**であり、Pythonの`ACCEPT`は実NewLang source checkerの受理、実Allocation/Domainの発行、公開checked-owned validatorによる検証、native実行を**意味しない**。
- Oは**常設トラックでも独立リポジトリでもない**。有用な成果が続く限り、限定Issue単位で実施する。

## 2. Authorityと信頼境界

| Authority / evidence | 権限の所在・扱い |
| --- | --- |
| 言語仕様 | `docs/reference/CURRENT_SPEC.md`が指すcanonical Draftのみがnormative。Oの旧モデル・研究報告はDraftを上書きしない。 |
| 設計判断 | Track MとCoordinationが[設計判断手順](NewLang_Design_Decision_Procedure.md)および[Design-Intent Ledger](NewLang_Design_Intent_Ledger.md)に基づいて判断する。 |
| 製品評価 | Track Vのproduct strategy/North Star契約を変更しない。Oの合成成功をH1のPASSに加点しない。 |
| 実ソースによる根拠 | Track Pの実NewLang parser→source checker→source-generated checked-owned artifact→C公開validatorの実測が必要。Oの模型は代替できない。 |
| Oの入力 | Pythonの`Original`、`Domain`、loan、event、snapshotは明示した**仮定・モデル入力**であり、実Cコンパイラが発行した権限ではない。 |
| Cの診断データ | JSON/ハッシュ/`validated:true`は観測・再現性の手掛かりであり、source-issued Allocation/LifetimeDomainや完全性を自動的に証明しない。 |

モデルはCopy pointerからroot/Domain/owner権限を捏造せず、現在の正本が区別する**元のR/O/incarnation、Backing/Allocation、governing LifetimeDomain、非Copy所有責任**を混同しない。C側の実際のREAD stability loan→派生WRITE ref→field refと、過去のPythonで仮定していた直接WRITE loanも区別する。Optionのold Some occurrence End、新Some occurrence Fresh（同じpointerへのSome→Someを含む）、whole Change/Reset、無関係なsibling currentness、Unknownの保守的な拒否を再現対象とする。旧プロトタイプのfield数・source文法を現在のNewLang規則と推定しない。

## 3. 他Trackとの分担

| Track | 責務とOからのhandoff |
| --- | --- |
| **O** | 独立した小規模モデル、候補アルゴリズムと状態表現、生成試験、差分解析、測定、最小反例の作成。 |
| **P** | C製production compilerの実source→owned経路、実権限、公開validator、CI/backend。Oのコードや判定は自動採用しない。 |
| **M** | 言語意味論・source surface・APIの裁定。Oで仕様の曖昧さが見つかれば証拠付きでMへ返す。 |
| **F** | 明示して選択された規則の形式化。Python differential agreementを形式証明と呼ばない。 |
| **R** | 独立Red Team。Oの生成負例は参考資料だが、Rのblind first-passの独立性を壊す入力にはしない。 |
| **V / Q** | 製品価値・人間の負担、または品質上の具体的リスクを各担当が評価。Oは単独でこれらを判定しない。 |
| **Coordination** | Issueによる限定作業の起動許可、優先順位、Pへの移管、独立reviewの要否、研究の継続・停止、Charter改訂を裁定。 |

Track OとPの**並行作業は可能**である。Oは実装の選択肢と反例を独立に作り、Pは実ソース由来の受理・権限検査を実測する。両者の差異は`SPEC-QUESTION`、`MODEL-LIMITATION`、`ANALYSIS-PRECISION`、`SOURCE-SURFACE`、`PRODUCTION-IMPLEMENTATION`等に分け、先に一方を正解と仮定しない。結果の比較・採用可否はCoordinationを経由する。

## 4. 研究の作法と再現可能な成果

1. **Issue-first・bounded。** 研究ごとに目的、固定main/正本Draft、参照するP/O PRのhead、研究対象、不対象、時間・停止条件、許される変更範囲を記す。Issueの作成だけではworkerは起動しない。
2. **Independent algorithms。** 同じ宣言的neutral IRの上に、基準となる具体遷移と別実装の抽象/増分解析を用意する。一方の判定を他方の`ACCEPT`のguardにしない。共通のIRや同じテスト仮定に起因するcommon-mode errorの可能性は必ず残す。
3. **意図的に区別する結果。** `ACCEPT`、`SEMANTIC_REJECT`、`PRECISION`、`UNSUPPORTED`を区別する。resource budget超過や未モデル化effectをunsafe sourceの証明と誤認しない。Cのparse/register/semantic/owned/public/backend/native各段階との対応がない場合は明示する。
4. **反証可能な比較。** 最低限、異なる非cJSON workload、正例・故意の負例・metamorphic/differential試験を用い、最初の不一致をoperation/状態差分/再現seedで記録する。未記録の同値スカラーChange/Resetによる新しいcurrent fact、古いSome occurrence、誤Domain、sibling違反、unknown gapなどを優先する。成果が単なる過去の同じテストの再実行なら`NO INDEPENDENT VALUE`と判断する。
5. **コストも評価する。** snapshot、persistent sharing/delta、fact dependencyなどを比較するときは状態量、複製、検査回数、precision、OOM/rollbackへの影響を測り、実測と推定を区別する。小規模サンプルの相対速度を一般的な優位性と断定しない。
6. **明確な限界。** 直線的なChangeだけの研究は、CFG/join、unknown call、callee substitution、nonCopy whole move/return、5-root fallible cleanup、nativeまで証明したことにはならない。実装しないものを研究の射程として列挙する。

成果は可能な限り`oracle/experiments/oN/`にPython >=3.12で再実行可能な小規模コード・unit test・generator/seed・実行方法・hash/結果を残す。必要に応じて`docs/experiments/`に短い報告を書く。実験PRは原則**DRAFT / DO NOT MERGE**とし、production Pのbranch、canonical Draft、CI、既存O実験や過去のsourceを勝手に変更しない。重い一時成果物をGitに置く必要はないが、再生手順と内容識別子は残す。Python結果だけのCI成功をC source-acceptance CIの成功とは呼ばない。

## 5. Pへの還元と採用gate

Oが提示する最小成果は **(a) 再現可能な入力と最小反例、(b) 2方式以上の独立比較またはその不能理由、(c) 改善すべき不変条件・情報量、(d) precision/計算コスト、(e) 次のCでの反証方法** とする。

- 単にPythonのevent9/12を`ACCEPT`にする、Cのshadow checkerを逐語移植する、診断JSONのchecksumを増やす行為だけでは、新しいproduction-level安全性の証拠としない。
- Pへの導入には別途Coordinationの**明示したP Issue**、実source-generated checked-ownedと公開validator、適切なsemantic拒否/precisionの識別、CI、必要に応じ独立Rが必要。Oのモデルが実際の権限issuerになることはない。
- OとPが食い違っても、仕様の変更を決めずにまず差異と最初の拒否層を記録する。新しい言語規則が必要ならMのhistorical design-intent gateに戻す。
- OがRの前提・攻撃問題を事前に与えてblind first-passを汚染しない。独立性の扱いは開発運用方針§7に従う。

## 6. 実施・停止・常設化

Track Oは**Coordinationの個別Issue**でのみ実施する暫定研究である。具体的な反例やモデル上の新情報が得られない場合、研究範囲がすでにPで検証済みの表層処理だけになる場合、またはsource-issued authorityなしに安全性を断定しなければ成果にならない場合は、追加連鎖を作らず**HOLD / NO INDEPENDENT VALUE**を報告する。時間枠到達時も未実行部分を明示して停止する。

Track Oの**常設化**や`NewLang_Oracle`独立リポジトリ化は別のCoordination決定を要する。判断材料は、(1) 複数の実質的に異なる解析アルゴリズム研究成果、(2) 再現可能な継続試験とmaintainer/CI費用、(3) 複数の非cJSON workloadに対する有益な独立反例、(4) Cとの型・観測情報の対応と権限を捏造しないtrust boundary、(5) 独立repoがもたらす便益が運用コストを上回ること。現時点では**分離HOLD**である。

Charter自体の更新・廃止はCoordinationがIssueに理由を記録し、開発運用方針と整合するreviewed PRで扱う。spec/source/APIのnormative選択を伴う場合は、Charter更新とは別に開発運用方針§4.2のdesign gateが必要。

## 7. 起点となった記録（歴史的証拠）

- [O0暫定charter/旧Python研究棚卸し #339](https://github.com/wakairo/NewLang_Compiler/issues/339)；旧M7.5、Unified IR/Finite Abstract Stateは歴史的なモデル資産。
- [O1/O2の独立Python実験と再生 #343](https://github.com/wakairo/NewLang_Compiler/issues/343)、[evidence PR #344](https://github.com/wakairo/NewLang_Compiler/pull/344)：合成入力に限ったexact/abstract研究。
- [O3のsource→neutral信頼境界 #345](https://github.com/wakairo/NewLang_Compiler/issues/345)、[O4の診断データ検証 #348](https://github.com/wakairo/NewLang_Compiler/issues/348)：可搬なC-issued authority証明は未確立。診断用JSON反復への追加投資は停止。
- [P350のC内部時系列再検証 #350](https://github.com/wakairo/NewLang_Compiler/issues/350)：独立primitive成立でも未記録のscalar fact更新を見逃さない新しいgap検証が有用。
- [O5独立アルゴリズム研究 #354](https://github.com/wakairo/NewLang_Compiler/issues/354)、[Coordination承認記録](https://github.com/wakairo/NewLang_Compiler/issues/268#issuecomment-6117692765)：Pの実source検証と並行してOの研究を限定再開。

これらのIssue/PRは**各時点の証拠と判断履歴**であり、このCharterやcanonical Draftより上位のauthorityではない。
