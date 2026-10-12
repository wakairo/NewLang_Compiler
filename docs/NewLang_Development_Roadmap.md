# NewLang 開発ロードマップ — 既存成果の実用化を優先する

> **Status: PROPOSED / NON-NORMATIVE — Coordination Issue #375 / review待ち**
>
> 対象スナップショット：2026-10-12、Compiler/main `348ca7d48f6dd5d658706f4547ab7ef9a63af33d`、
> FormalProof/main `a8de4ebb4099f4742fc161d26867740dcac48414`。
> 本書は順序と判定基準の**提案**であり、Issue/PRの起票だけで実装やTrackを開始しない。
> 言語の正本は [CURRENT_SPEC](reference/CURRENT_SPEC.md) → Draft 17.30、
> 製品の正本は [Product / Value Strategy](NewLang_Product_Value_Strategy.md)、
> [North Star](NewLang_North_Star_Product_Validation.md)、
> [C-Replacement Coverage Map](NewLang_C_Replacement_Coverage_Map.md)。
> 運用は [Development Process](NewLang_Project_Development_Process.md) に従う。
> この文書は正本を置換せず、将来の更新時は「観測した事実」と「新しい投資案」を分ける。

## 1. まず何を継続するのか

**継続する。** NewLangの目標は、低水準C-class architectureを保持しながら、
lifetime / ownership / provenance / current stateの**機械的な確認責任を減らす**こと。
**Policy-neutral, mechanism-strict**。全内部操作の完全自動証明も、
反対にすべてのpointer hopで人手による`unchecked`を要求することも目標ではない。
安全なlibrary APIが要求する真の局所条件は実装者が監査できる必要があるが、
`unchecked`は原Allocation/Domain/typed authorityを創出しない。

**停止しているのは個別の追加投資**であり、プロジェクト全体ではない。
cJSON H1 [#268](https://github.com/wakairo/NewLang_Compiler/issues/268) は
**PASS/FAIL UNDECIDED / 追加H1専用実装はDEFER**。
STB-ARRAY-1 [#262](https://github.com/wakairo/NewLang_Compiler/issues/262) は
**Cでの挙動は実測済み、NewLangの本物のallocator-owned growは未実証 / 追加実装HOLD**。
M371/M372のsafe operation contractとM374/F45のallocator/typed-claim橋を
*解決済み*として扱わず、将来の再開条件として保存する。

## 2. エビデンスの読み方

少なくとも次を区別する。後の列が前の列から自動的に従うわけではない。

| ラベル | 何を確認したか | 何を確認していないか |
| --- | --- | --- |
| **SPEC** | `CURRENT_SPEC`が選ぶ規範的意味論 | production parser、実装、C ABI、native |
| **CHECK** | 実NewLang sourceをproduction checkerがその範囲で検査 | そのsourceの一般的なcodegen/使用感 |
| **NATIVE** | 実NewLang source → checker → emit → C17などで実際に実行 | 別形状・別platformでも動くこと、製品価値 |
| **FORMAL** | 指定されたモデル・定理・前提の成立 | 実compiler・allocator・runtimeとの未証明refinement |
| **EXPERIMENT** | opt-in/未マージPRやPython synthetic研究の局所観測 | mainへ採用された機能や製品合格 |
| **PRODUCT** | 本来のworkload、topology、安全差分、人間L/R/P/V/C負担を比較 | 技術的テストの成功を製品価値とみなすこと |

## 3. 現時点の資産・不足（証拠の範囲付き）

| 分野 | 現にある成果と根拠 | 正直に残る課題 |
| --- | --- | --- |
| **M / 規範** | Draft17.30のroot/BackingRegion、nonCopy、Domain、persistent ptr、scoped ref、Change/Reset、known-call body-sensitive、fallible分岐。実際に選択されたsourceは**bounded**。[CURRENT_SPEC](reference/CURRENT_SPEC.md) | `unchecked`の概念的意味はあるが、汎用source構文・protected representation・dynamic owner claim/epoch transferを一般に実装したわけではない |
| **P / parser・checker** | C17製のproduction compiler、P0–P14の段階的source/semantic基盤（登録関数、条件分岐、loop、ref、sum、aggregate等）。[Compiler README](../README.md) | 多くの受理済み構文は現行native emitterの範囲外。source admissibility、semantic precision、codegen failureを別診断にすべき |
| **P / 実行** | scalar/Pair/fixed link等の限定的なsource→checked→Checked-C→host実行と、original五Hの**pre-detach**六初期link・0–5 failure native。[Five-root report](FIVE_ROOT_NATIVE_C17_REPORT.md) / [contract](FIVE_ROOT_NATIVE_C17_CONTRACT.md) | 一般backend完成、libraryのordinary helper実行、cJSON B detach+adopt/native、STB NewLang growは未実証。LLVMは**主backend方針**であり、Checked-C成功だけでLLVM完成としない |
| **F / 形式的安全性** | F0–F2および多数のbounded adjunctによるoccurrence・責任・scoped access・loopの証明。[FormalProof README](https://github.com/wakairo/NewLang_FormalProof/blob/main/README.md) | compiler-to-formal全体のsoundness theoremや実source/runtime等価性は未完成。F45はfinite epoch/physicalモデルを構築したが**allocator rich refinement 6条件HOLD**。[F45](https://github.com/wakairo/NewLang_FormalProof/issues/45#issuecomment-6091895139) |
| **O / 検証アルゴリズム** | M7.5歴史的Pythonプロトタイプの再評価、O1–O5の独立synthetic Change chronology/abstract-state比較。[O0](https://github.com/wakairo/NewLang_Compiler/issues/339) / [O5](https://github.com/wakairo/NewLang_Compiler/issues/354) | source-owned authorityへ変換する証明付き橋ではない。PR #344/#357等は**DRAFT・非normative**。独立Oracle repository化は保留 |
| **R/Q / 信頼境界** | 独立Rの実source/validator/negative実測、Qの局所的なartifact-integrityレビュー。[R #359](https://github.com/wakairo/NewLang_Compiler/issues/359) / [Q #323](https://github.com/wakairo/NewLang_Compiler/issues/323) | finite/較正済み正例を全言語に一般化できない。新たなcritical compiler境界は適切なscopeのR評価を要する |
| **V / 製品と差別化** | 実cJSONと固定STB Cの具体的workload、元topology/失敗世界/人間の負担の評価。[H1](https://github.com/wakairo/NewLang_Compiler/issues/268) / [STB C #265](https://github.com/wakairo/NewLang_Compiler/issues/265) | safe library author/ordinary callerの**比較実測の優位**未証明。cJSONのoriginal B adoption 0/2、STBのNewLang source/nativeなし。C、Zig、topology-preserving Rustの実際のsafe-APIに公平に比較する |

**注意：** Compiler/mainにない実験機構のPRは見つけても自動的に「現在の実装」とは数えない。
[H1追加機構投資の裁定](https://github.com/wakairo/NewLang_Compiler/issues/373#issuecomment-6125144038)、
[STB六条件HOLD裁定](https://github.com/wakairo/NewLang_Compiler/issues/374#issuecomment-6125505436)
はtechnical evidenceとproduct investmentを分離するための拘束条件である。

## 4. 新しい優先順位（暦日ではなく通過条件で進む）

| 順序 | 段階 | 成果物・合格条件 | ここでやらないこと / STOP |
| --- | --- | --- | --- |
| **A0 — 今回** | 根拠付き棚卸しとscope固定 | 本文の能力表、実行到達範囲、保留理由と再開条件をuser/Coordinationがレビューする | Draft・P/F実装・H1 score・製品目標を変更しない |
| **A1 — 最優先** | **既存の言語構成による普通のプログラムのsource→CHECK→NATIVE** | **新しい意味論なし**で、単一unit内の通常の名前付きhelper/direct call + Copy引数/返値を1件、CLIから実行。異常系は同じCLIで真のsemantic rejectとbackend unsupportedを区別。既存のV0/V1/AVS・five-H nativeを回帰として保護 | **最初にsource admissionとemitter対応を観測して境界を固定**。backend修正が複数の新しいsource/ABI/ownership機構を要求するならSTOP・小さいスライスへ切る。名前付きhelperはP8–P11に意味論の基礎があるが**この組合せのnative実行は未確認** |
| **A2 — A1後** | 低水準state updateを伴う**小さなlibrary内部操作** | 既に規範化された`Pair`/fixed field or bounded persistent-link + scoped write/replaceのうち**実sourceで現に受理できる一つ**を、同じ実行経路へ追加。before/afterと旧occurrence終了、wrong-D/stale-access/consume misuseの実source負例を検査。正確な診断と反復loan/cleanupを記録 | cJSON graphやSTB relocationへ形だけ寄せない。compiler専用特別名前/host-owned証明が必要ならSTOP。機能数ではなくユーザーが使える操作を増やす |
| **B — 条件付き** | library API / 人間の負担の実測 | A1/A2の成果で、1つの通常の**呼出可能なsafe operation**の作者側・利用者側L/R/P/V/Cを同等のC/Zig/Rust実装と比較。静的に除去できた責任と残るtrue library policyを分類する | safetyを隠れた`requires fn`で呼出者に移転しない。receipt + encapsulation等**複数の新規大機構が必要**ならHOLDし別候補を検討 |
| **C — 再開条件付き** | 独立した実C workloadの縦断製品検証 | cJSONまたはSTBの保留条件が独立した新証拠で狭まったときだけ、元pre-registrationを**そのまま**使い、source→semantic→native + negative + product benefitを測る | H1 PASS/FAILを改変しない。STB C結果をNewLang productの成功に転記しない。技術GOを即製品GOにしない |
| **D — 長期別枠** | practical C-replacement coverage | FFI/C ABI、function pointer/context callback、external backing、in-place init、freestanding、modules/separate compilation、platform/atomicity等を[Coverage Map](NewLang_C_Replacement_Coverage_Map.md)のno-foreclosureに従い個別評価 | roadmapの次項目として全機構を今実装しない。C構文互換・一般Rust型制度の複製を目標にしない |

**優先する最初の実験はA1の一つだけ**。これは「NewLangはすでにnamed helperのnativeが動く」という主張ではない。
まず現在のsource → CHECKの受理状況、emitterが拒否するexact stageと診断を記録し、
すでに選択された規則の**faithful backend workに収まる**場合だけ最小実装へ進む。
もし通常関数自体が未サポートなら、初回の作業で見つけた**最も近いsource-positive**を採用候補として再裁定する。
テスト用host JSONやsynthetic Pythonが通常のcallerから使えるNewLang sourceを置換してはいけない。

## 5. 各Trackの作業方針と審査

- **Coordination**：A1の一件分だけ実装scopeを判断し、source・checker・nativeの拒否点と費用をIssueに記録。変更が新たなsource/semantic選択に当たるなら [§4.2](NewLang_Project_Development_Process.md)、大きな投資なら[§4.3](NewLang_Project_Development_Process.md)を適用する。無関係な全Issueへ新しいgateを追加しない。
- **P**：選択済みのsource ruleとcompiler APIのfaithful source/diagnostic/emitter。actual sourceとnative実行をprimary evidenceにする。プロトタイプflagとmainの機能を混同しない。
- **M**：A1で本当に不足する規範的source機構が生じた場合のみtargeted reopen。別途正当化された低水準design/no-foreclosure研究は継続可能だが、H1/STB productの自動GOではない。
- **F**：A1が既存の証明境界に収まるなら追加証明を強制しない。新しい安全性主張・critical allocator/authority橋が実際に提案された場合のみ独立してformalize/falsify。
- **O（暫定）**：独立アルゴリズムの再現性・精度・memory cost調査を条件付きで継続可。synthetic一致≠source-owned safety、通常のcompilerのoracleではない。永久Track化/別repo化は自動的には行わない。
- **V**：各段階で**library作者と通常の利用者を分けて**L/R/P/V/C、診断、比較対象C/Zig/Rustを判定。materialな目標逸脱には `VALUE-ALIGNMENT-RISK` を出すが、すべてのPRに事前signoffは要しない。
- **R / Q**：新しい本物のtrust boundaryや広いsource-positiveには限定的な独立検証（blind優先）、品質のための手続き拡大ではなく観測可能な欠陥・リスクを対象にする。

## 6. 既存HOLDの再開条件と非目標

**cJSON H1:** 元の5 fallible src/A/B/C/dst、6 setup＋4 middle detach＋2 adoption
（`dst.child=B`, `B.prev=B` **self-link**）、full original B+dst nonCopy owner return、
donors先行release、失敗0–5、recursive/reference区別、native、B1–B7、topology、ceremony/diagnosticsを固定。
次の投資の条件は**真の原B receipt/refund/caller poststateと安全なmembership/representationの少なくとも最初の独立したsource実証**、加えて作者/利用者負担の比較可能な改善である。
新しい一般機構を先に仮定したH1-only勝利は不可。

**STB-ARRAY-1:** cap4,len4→8,len4（元ヘッダ隣接単一Backing）、same-numericのepochとforced moveを両立。
F45の**六条件**：failure旧状態完全保存、success atomic exchange、typed rich package/invariant、
D/loan+外部生存者、実レイアウト/overflow、full raw/Storage/hidden owner保存。
特に#2/#3/#6は独立した未証明の橋。allocator内部の`unchecked`一箇所という表記だけでは証明されない。
これらが**実source表現・意味論的責任・実allocator contract**に局所化し、
費用対効果が改善した根拠が得られた場合のみ再開する。

**共通の非目標：** 全部checkedで証明するためだけのengine、per-hop unchecked、
metadataからauthorityを創造するwrapper、CLI以外でだけ動くhost fixture、
変更されたtoy topology、並行する複数の未選択広域機構、成功数だけのKPI。
H1/STBのHOLDは「NewLang開発中止」や既存規則の不健全性確定を意味しない。

## 7. 次に承認を求める一件

**Proposed next bounded Track P task（この文書で自動起動しない）：**
A1 preflight＋忠実な最小backend slice。まずmainの現実の
`fn named helper / ordinary direct call / Copy arg+return` sourceを
**1本のfixture**で実測し、parse / registration / semantic / emitter / nativeの
最初のfailure stageを報告する。既存選択済み規則とback-end coverageの範囲で
修正可能なら、正負例、emit前reject、host native result、GCC/Clang/ASan/UBSan、
CI回帰と実行手順を**同じ小scope**で実証する。

**Hard STOP:** fixed selected semanticsを変える必要がある、sourceがまったく表現できない、
正しいbackendを作るのに他の未選択ABI/closure/owner系が必須、または
診断・通常ユーザーの負担が改善しない。そこでは新機能追加に切り替えず、
Coordinationが別のsource-positive候補と価値を選び直す。
一般LLVM backendへの投資可否は、このv0実行経路で学んだ差分を使って別途裁定する。

**この提案をreviewしてから**最初のP Issueを起票する。
その他のTrackを連鎖起動せず、独立研究の既存判断を上書きしない。

