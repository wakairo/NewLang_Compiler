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
| **P / 実行** | mainではscalar/Pair/fixed Option-link等の限定source→CHECK→C17/nativeとoriginal五H **pre-detach** 六初期link・0–5 failure native。未マージ[DRAFT G0 PR381](https://github.com/wakairo/NewLang_Compiler/pull/381)のu8返却と、stacked [DRAFT G1 PR384](https://github.com/wakairo/NewLang_Compiler/pull/384)の二Pair named scoped write-ref/Copy-oldは別のbounded native技術実証。[R #386](https://github.com/wakairo/NewLang_Compiler/issues/386#issuecomment-6136526649)独立PASS（限定）。 | G0/G1は**main未マージ**。一般Option/Node named-call/backend、任意record、opaque library、cJSON B detach+adopt/native、STB NewLang growは未実証。LLVM主backend方針≠Checked-C完成 |
| **F / 形式的安全性** | F0–F2および多数のbounded adjunctによるoccurrence・責任・scoped access・loopの証明。[FormalProof README](https://github.com/wakairo/NewLang_FormalProof/blob/main/README.md) | compiler-to-formal全体のsoundness theoremや実source/runtime等価性は未完成。F45はfinite epoch/physicalモデルを構築したが**allocator rich refinement 6条件HOLD**。[F45](https://github.com/wakairo/NewLang_FormalProof/issues/45#issuecomment-6091895139) |
| **O / 検証アルゴリズム** | M7.5歴史的Pythonプロトタイプの再評価、O1–O5の独立synthetic Change chronology/abstract-state比較。[O0](https://github.com/wakairo/NewLang_Compiler/issues/339) / [O5](https://github.com/wakairo/NewLang_Compiler/issues/354) | source-owned authorityへ変換する証明付き橋ではない。PR #344/#357等は**DRAFT・非normative**。独立Oracle repository化は保留 |
| **R/Q / 信頼境界** | 独立Rの実source/validator/negative実測、Qの局所的なartifact-integrityレビュー。[R #359](https://github.com/wakairo/NewLang_Compiler/issues/359) / [Q #323](https://github.com/wakairo/NewLang_Compiler/issues/323) | finite/較正済み正例を全言語に一般化できない。新たなcritical compiler境界は適切なscopeのR評価を要する |
| **V / 製品と差別化** | 実cJSON/STB workloadに加え、実G1のsource/nativeを対象にC・Zig・Rustの同じnamed scalar field helperと**作者・caller別**L/R/P/V/Cを比較。[V #387](https://github.com/wakairo/NewLang_Compiler/issues/387#issuecomment-6136712325)は**TECHNICAL ONLY / VALUE NOT DEMONSTRATED**。 | G1は静的安全性負例の限定実証であり、減った総人間負担・通常safe library APIの優位は未実証。opaque/nonCopy/heap/domain、cJSON original B adoptionとSTB NewLang source/nativeは未達 |

**注意：** Compiler/mainにない実験機構のPRは見つけても自動的に「現在の実装」とは数えない。
[H1追加機構投資の裁定](https://github.com/wakairo/NewLang_Compiler/issues/373#issuecomment-6125144038)、
[STB六条件HOLD裁定](https://github.com/wakairo/NewLang_Compiler/issues/374#issuecomment-6125505436)
はtechnical evidenceとproduct investmentを分離するための拘束条件である。

## 4. 現在の優先順 — 実用操作を選んでから機構を選ぶ

**ロードマップの出発点は新しい機能ではなく、実際のC-class利用者が必要とする一つの操作。**
その操作のsource-level入力・出力・状態変化と、作者／通常caller別の
L（lifetime）/R（responsibility）/P（pointer access）/V（value）/C（control）
負担を先に定義する。C、Zig、**同じpointer topologyを保持するRust**
（`Box`＋`NonNull`＋局所`unsafe`の安全なwrapperやRAII）と比較する。
Rustのsafe reference / indices等へ構造を変更する比較は、それを明示して別枠で行う。

**2026-10-13時点の結論：どの新しいProduct／Implementation gateも自動GOではない。**
ユーザーの次の候補選択と投資判断を経るまでは、追加のTrack実装を起動しない。
現在の技術的成功を残しながら、製品価値の未実証を言い換えて隠さない。

### 4.1 証拠がある分岐と、次の実用操作への障害

| 候補 | 現実の足場・最初の不足 | 次に価値を証明する条件・現時点の扱い |
| --- | --- | --- |
| **S0：既存の局所状態／fixed field** | mainにbounded scalar/Pairと`loan_write`＋`replace`の実source→C17/native。新規一般callやheap traversalは別 | **既存能力**。新しい成功数に加算しない。具体的な利用者の実作業に使って初めて再評価 |
| **G0/G1：named fixed-field operation** | DRAFT [PR381](https://github.com/wakairo/NewLang_Compiler/pull/381) `()->u8`、stacked DRAFT [PR384](https://github.com/wakairo/NewLang_Compiler/pull/384) Pair `ref<write,u8>`＋Copy-old結果は実C17/native。[R386](https://github.com/wakairo/NewLang_Compiler/issues/386#issuecomment-6136526649)は限定独立PASS | [V387](https://github.com/wakairo/NewLang_Compiler/issues/387#issuecomment-6136712325) **TECHNICAL ONLY**：普通callerの総負担削減は未証明。両PRは未マージ／**DO NOT MERGE**。同じhelperを増やすだけでは製品評価を再開しない |
| **F：persistent Option link＋元Domain再loan** | mainで2 lexical Nodeの直接link変更とpost-clear旧Copy locator→まだliveな元rootへのread-reloanは有限native。[P390](https://github.com/wakairo/NewLang_Compiler/issues/390#issuecomment-6137712444)。named `Option<ptr<Node>>` helperは[P388](https://github.com/wakairo/NewLang_Compiler/issues/388#issuecomment-6136990174) `P8-SIGNATURE-PRECISION` | [V389](https://github.com/wakairo/NewLang_Compiler/issues/389#issuecomment-6137182596) **TECHNICAL BASIS ONLY**。sourceでのpayload取得と独立caller未達。**G2 named／toy二個体の追加実装HOLD** |
| **real five-root heap graph** | mainで5独立fallible original heap nodes・6本のcross links・0〜5失敗/解放世界のSOURCE/CHECK→C17/native。[V391](https://github.com/wakairo/NewLang_Compiler/issues/391#issuecomment-6137908136)は差別化の**土台**と判断 | [P392](https://github.com/wakairo/NewLang_Compiler/issues/392#issuecomment-6138094156)実2-hop consumerの`B.payload`読み取りは`HEAP-LINK-FIELD-PROFILE`（SOURCE未対応、semantic不正ではない）。[M393](https://github.com/wakairo/NewLang_Compiler/issues/393#issuecomment-6138319527)限定候補は可能でも**VALUE GATE NOT MET／source選択・checker/validator/emitter投資HOLD** |
| **N：bounded数値処理** | u8 literalはあるが、普通の算術式・比較・overflow等の完全source/check/native未実証。再帰は別precision制約 | Fibonacciやsumは機能デモだけではC後継の責務削減を示さない。現実の数値処理のconsumerが先 |
| **B：byte処理／decoder** | byte/span/index/guard/loop/結果の通常source/backendに複合gap。[P379](https://github.com/wakairo/NewLang_Compiler/issues/379) | 実際の長さ付きdecode、エラー返却をconsumerが使い、C/Zig/Rustの安全性／費用差が示せる場合のみ。複数の大きな機構が同時に必要ならHOLD |
| **I：fixed storage／search／Ring** | 通常の`Array<T,N>`要素source/`data[i]`/bounds/loop/占有状態とnativeの複合gap。§15 `head/len`はmetadata SSOTであり権限ではない | 既存配置を維持するCopy要素の使用・fail resultを先に示す。nonCopy claim、dynamic grow、STB allocator epochは別gate |
| **S5：安全なライブラリ利用** | body-visible known callなどの有限基盤はあるが、任意opaque representation・independent safe caller／元nonCopy owner転送は未成立 | **S5a**通常callerの実用操作と安全責務削減を先に検証。**S5b**元owner／representation／receipt-refundは独立の後段。H1の代理としない |

原根拠：[V377の製品経路比較](https://github.com/wakairo/NewLang_Compiler/issues/377)、
[M378の意味論・旧Python調査](https://github.com/wakairo/NewLang_Compiler/issues/378)、
[P379のsource/backend実装可能性一覧](https://github.com/wakairo/NewLang_Compiler/issues/379)。
これらは今の実装能力・製品PASSを勝手に増やすauthorityではない。

### 4.2 最初のEngineering gateを選ぶための条件

新しい候補を始める場合は、次を**一つの具体的な操作**について記録する。

1. 実際のユーザーが扱う元のC型の構造／動作と、入力・戻り値・失敗結果・状態変化。値の単なるdiscardや受動C observerだけは不合格。
2. 既存**選択済みsource semantics**に沿った最も狭いNewLang入力と、SOURCE/CHECK・公開checked-owned evidence・限定backend・nativeの**最初の未対応点**。既存のtest flagや昔のPythonで成功したことはsource-positiveではない。
3. 同等のC/Zig/トポロジー保持Rustに対して、作者と普通callerのL/R/P/V/Cのどれが**compilerへ移り**、何がmanual／局所unsafe／runtime check／library policyとして残るか。LOCだけで判定しない。
4. **停止条件：** 一つのユーザー操作を通すために二つ以上の無関係な広いsource/semantic/ABI/owner機構、権限を無から作るhostや`unchecked`、あるいは反復するper-hop proofが必要ならHOLDして他workloadへ。static negative、runtime error、policy mismatch、unsupported profileを混同しない。
5. 実装判断はCoordinationの§4.3 Goal Alignment後のみ。新しい選択source ruleが必要なら[Development Process §4.2](NewLang_Project_Development_Process.md)／Design Decision Procedure／Ledgerを経て、実装のGOと別に選択する。広い安全性主張が新たに加わる時だけ適切な独立R/Fを起動する。

この条件に合う候補がまだ無いこと自体を**未選定**として扱う。
「とにかく進める」ために、FibonacciやG2 helper、payload fieldなどを自動的なfirst taskへ格上げしない。

### 4.3 過去のPython／Oracle研究の扱い

[O0 #339](https://github.com/wakairo/NewLang_Compiler/issues/339)・
[O5 #354](https://github.com/wakairo/NewLang_Compiler/issues/354)等は
source-owned event chronologyや有限状態アルゴリズムを調べるための再現可能な研究。
Vec/arena/free-list/HashMap等の歴史的Python prototypeや多数のdifferential PASSは、
**現在のNewLang sourceで同じ操作を実行できる証拠ではない**。
正しい原Domain／Allocation照合、Value/OccurrenceとChange/Reset/Unknown、占有metadataと権限の分離、
public certificateの検証可能性という知見だけを、実際に必要な次工程へ条件付き再利用する。
別の常設Track／独立repo化はこのロードマップでは認可しない。

## 5. Track間の責任分担（新しい審査工程は増やさない）

- **Coordination：** 新しい具体的なuser operationと比較可能な製品メリットを選択し、投資GO/HOLDを裁定する。候補と証拠の境界は親Issue #375に記録。
- **V：** user/author/ordinary callerを区別して、実作業とC/Zig/Rust比較、lifetime/owner/provenanceなどの**人間の意味的責務**を評価する。技術成功は製品PASSでない。
- **P：** 認可された狭い実NewLang source→checker→**public owned**→限定C17/nativeの責任。SOURCE unsupported、semantic reject、backend unsupported、環境障害を分ける。
- **M：** 新たに選択するsourceやcore規則が本当に必要な場合だけ、過去の採用／意図変更とno-foreclosureを検討する。
- **F/O：** 未解決の重要なauthority/allocator橋や実際の解析精度リスクに絞った独立研究。Python合成結果を実source証明へ昇格しない。
- **R/Q：** 真のtrust boundaryが増えた場合のみ、独立した限定的検査（必要に応じblind）。全変更への儀礼的な義務レビューはしない。

## 6. 凍結した大きな製品ゲート

**cJSON H1 [#268](https://github.com/wakairo/NewLang_Compiler/issues/268) —
PASS/FAIL UNDECIDED、H1-only実装DEFERRED。**
元の5つの独立fallible heap roots `src,A,B,C,dst`、6 initial setup＋4 middle detach＋2 adoption
（`dst.child=B`、**`B.prev=B` self-link**）、元B＋dstの**完全なnonCopy owner custody／receipt-refund／return**、
0–5失敗世界、早期donor release、参照／owning区別、recursive delete、B1–B7 safety、link topologyと人間負担を維持。
元のownership/representation/membership責務を放棄した簡易graphやread-only observerでH1を達成した扱いにしない。
再開には本当の元B receipt/refund/caller poststateと、作者＋利用者のL/R/P/V/C差分に関する独立実証が必要。

**STB-ARRAY-1 [#262](https://github.com/wakairo/NewLang_Compiler/issues/262) —
Cのallocator操作は実測済みだがNewLang growはHOLD。**
cap4,len4→8,len4、header-adjacent original backing、same-numeric epochとforced moveの比較を維持。
[F45](https://github.com/wakairo/NewLang_FormalProof/issues/45)／
[M374](https://github.com/wakairo/NewLang_Compiler/issues/374)の六条件
（失敗時旧状態保存／success atomic exchange／typed rich package-invariant／loanと外部生存者／実layout-overflow／full raw-Storage-hidden owner）
を全て保持。局所`unchecked`という表現だけでAllocation/Domain/authorityを作れたと扱わない。

H1/STBの停止は**NewLang全体の開発中止でも既存core semanticsの不健全性でもない**。

## 7. Coordinationの現在の裁定とレビュー対象（2026-10-13）

- **Authority:** Compiler/main `348ca7d48f6dd5d658706f4547ab7ef9a63af33d`、canonical Draft17.30、FormalProof/main `a8de4ebb4099f4742fc161d26867740dcac48414`。
- **技術KEEP:** mainの五実heap roots/六初期link/0–5 native。DRAFT G0/G1の限定C17、[R386](https://github.com/wakairo/NewLang_Compiler/issues/386#issuecomment-6136526649)独立限定PASS。
- **製品HOLD:** [V387](https://github.com/wakairo/NewLang_Compiler/issues/387#issuecomment-6136712325)のG1 `TECHNICAL ONLY`、[V391](https://github.com/wakairo/NewLang_Compiler/issues/391#issuecomment-6137908136)の5-root `BOUNDED BASIS`、[P392](https://github.com/wakairo/NewLang_Compiler/issues/392#issuecomment-6138094156)のSOURCE-PROFILE障害、[M393](https://github.com/wakairo/NewLang_Compiler/issues/393#issuecomment-6138319527)の`NARROW CANDIDATE / CONDITIONAL`を統合。[Coordination裁定](https://github.com/wakairo/NewLang_Compiler/issues/393#issuecomment-6138373666)：**`VALUE GATE NOT MET — HOLD SOURCE SELECTION AND PRODUCTION INVESTMENT`**。2-hop sourceの`peek_B_payload;`は`u8`を単に捨て、利用結果を変えない。225行の手動cleanup＋25行のpeekは製品上の負担減を未証明。read-only proof certificateや限定nativeの追加は別途必要。
- **再開条件:** 元5heap/6link/0–5責任を変えずに、値が普通の利用者の**実際の戻り値／状態変化／判断**に因果的に作用するsource operationを定め、C/Zig/同トポロジーRust比較で**どの人間の義務が減るか**とbounded public owned artifact→backend可能性を示す。そうでなければ別の本物のC-class workloadへ迂回。追加のgraph専用実験や自動Track連鎖は行わない。
- **現在のPR:** [PR376](https://github.com/wakairo/NewLang_Compiler/pull/376)は**DRAFT docs-only、ユーザーレビュー待ち**。[PR381](https://github.com/wakairo/NewLang_Compiler/pull/381)・[PR384](https://github.com/wakairo/NewLang_Compiler/pull/384)はDRAFT・未マージ。**DO NOT MERGE**。新しい機構/試験の自動GOはない。
- **CI evidence:** docs PR前HEAD `16efbd592ef4e2a5c9fc94a90f56f22653bf9f15`の[Actions #38347698561](https://github.com/wakairo/NewLang_Compiler/actions/runs/38347698561)は5/5 SUCCESS。文書を再編集した新HEADは別のCIを確認するまでgreenとは書かない。
