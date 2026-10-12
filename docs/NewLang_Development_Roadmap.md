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

## 4. Coordination裁定済みの暫定航路（改訂可能）

> **Status: PROPOSED / user review pending; no P start or normative selection.**
>
> 独立 [V #377](https://github.com/wakairo/NewLang_Compiler/issues/377#issuecomment-6134367162)・
> [M #378](https://github.com/wakairo/NewLang_Compiler/issues/378)・
> [P #379](https://github.com/wakairo/NewLang_Compiler/issues/379#issuecomment-6134477587)
> の最終報告を[Coordination #375](https://github.com/wakairo/NewLang_Compiler/issues/375)で照合した。
> **最初の製品価値の星は「2つの独立した固定レコードに使える、previous-Copy値を返す状態更新safe operation」G1**。
> 一方**最初に追加してよい実装の候補は、その前提のうち既存checker-positiveの
> 引数なしu8-return C17 loweringだけ（G0）**。G0は技術的な橋であり、星の到達やproduct PASSではない。
> 現在のDraft17.30・compiler/mainの機能を越えたSource/Result/borrowは自動採用しない。

### 4.1 合流・分岐できる星図

```text
S0: 既存の u8/Pair 固定field scoped replace + 引数なしunit helper (SOURCE/CHECK/NATIVE済み)
  |
G0: 引数なし u8 戻り値を実際のcalleeからC17へlower (技術的な橋、未実装)
  |                      ┌─ N: checked数値演算→合計/反復Fibonacci (一般演算source未実証)
  v                      ├─ B: bounded byte入力→decoder/scan (index/span等が未実証)
G1: 2つの固定record     ├─ I: safe fixed array/index→二分探索→必要ならsort
に再利用できるsafe       │       └─ S4: fixed-capacity ring/queue（occupancy/SSOT）
state update/Copy-old ───┤
(製品価値の初回判定)     └─ F: 正しいDのscoped ref + fixed Option/ptr update
  |                                        └─ G1/S5aと合流可能
S5a: 小さなbody-visible safe-call operation（本物のcaller証拠）
  |
[HOLD境界: original owner receipt/refund、membership/representation保護]
  |
S5b: 真の独立したnonCopy owning-safe library operation（別途設計・実測が必要）
  |
★: 凍結済みcJSON H1（製品PASS/FAIL未決定、専用実装投資DEFER）
STB-ARRAY-1: allocator epoch/rich claimは独立のHOLD。★の代用ではない。
```

- **S0**: current mainで実NewLang source→CHECK→限定C17/nativeを既に通したu8/Pair/fixed-field old capture/replace、0引数→unit ordinary named call、限定Node link等。*新たな星として加算しない*。
- **G0 technical rung**: P379が固定main実テストで確認した`fn scalar()->u8{u8(7)}; fn main()->unit{scalar();unit}`相当。**SOURCE/CHECK YES、emitter NO、`V1-BACKEND-UNSUPPORTED` / exit4、stdout空**。正しい関数の本体・戻り値のruntime loweringを最小のC17 subsetで追加できるかを測るだけ。**実装は別途user review/1件のP authorization後**。7や9をhostが埋める偽nativeは不採用。一般のCopy引数・算術・recursion・LLVM backend完成を示さない。
- **G1 first product star**: 2つのu8固定field（state/marker）を持つ**二つの独立レコード**を、**同じ普通のsafe named operation**が正しいscoped fixed-field writeで更新し、元の**Copy old value**をcallerへ返し、markerと他オブジェクトを保存。callerはuncheckedなしで次の操作へ進める。**概念上の契約であり、現在Source/CHECK/NATIVEで一般refパラメータ・copy-result・callee poststateの合成が成立すると宣言しない**。呼出しの引数/loan・戻り値・作用のsource受理とemissionに最初の不足がある場合、第一障害を分類する。複数の新しい大きな機構が必要ならG1をHOLDし、F枝の既存fixed-field操作/closed sum短路へ迂回。**G0達成だけでG1へ自動投資しない**。
- **S5a**は限定的な**同じsemantic unitの可視bodyに基づく安全な操作の再利用**であり、private representationやoriginal owner移転を要求しない。**S5b**のopaqueなnonCopy所有オブジェクトAPIとは明確に分ける。後者はcJSON M371/M372の別の**SOURCE/ENCAPSULATION GAP**を満たす必要がある。
- **N/B/I/S4/F枝**は順序固定ではない。現mainでは一般`+`/比較、再帰、char/string/byte sequence、`data[i]`、一般Array/span、Ring構築の一体source/nativeが**未実証/未対応**であるため、最短の経由地と誤認しない。既存の条件分岐やloop checkerを、新しい数値処理・配列のnative成功へ昇格しない。

### 4.2 候補星・実際の最初の障害・再利用する能力

| 経由地 | 目標となる「実際のプログラム」 | Source/実装上の第一gap（P379・M378証拠） | 成功時に別の星へ渡せるもの / 負例 |
| --- | --- | --- | --- |
| **G0：u8 return call** | 固定body `scalar()->u8`のC17 calleeが、実行時に本当に7を返す／sourceを9へ変えれば9が観測される | mainはCHECKは通すがbackend4/非対応。**引数なしunit-return callは既にnative成功** | source-generated body/型付き返値の正しいemitter。範囲外u8や型不一致はC生成前に拒否。**Product価値判定は保留** |
| **G1：safe state transition** | 固定Pairに一回scoped write、元u8を返す普通のnamed functionを**2個の異なるrecordに適用** | 正しい`ref`パラメータ・Copy返値・caller-visible changeを実source/nativeで組合せた結果は**未確認** | checked author/scoped dependency＋通常callerの再利用。逃げるscoped refは実semantic reject、wrong old/siblingはnative policy mismatch。作者・利用者L/R/P/V/CとC/Zig/Rustの操作分割版を比較 |
| **N：有限数値処理** | 合計・反復Fibonacci、別候補の再帰 | 一般算術式とoverflowガードのsource/parser/checker/codegen不足。再帰は別precision limit | 単なるu8 literalと区別して真正の計算、正しいover/underflow runtime/semantic分類 |
| **B：byte/char処理** | length-bounded scan、整数parse、escape decoder | byte sequence/index/span/bounds/loop source未実証。C lexer実装はNewLang libraryでない | 検査可能な入力長・失敗/Result・parserへ再利用。範囲外アクセスと不正文字のruntime errorを区別。無限NUL探索をsafeと称さない |
| **I：検索/ソート** | 固定array内のbinary search、挿入sort | `Array<T,N>`の一般実sourceと`data[i]`、比較/算術が未対応 | bounds・loop・更新/aliasを共有。空範囲/重複/overflowを検証し、ソート結果はlibrary policy oracle |
| **S4：固定Ring/queue** | Copy要素のcapacity3: 空→満→wrap→排出→再使用、2独立インスタンス | 一般Array/index/Option conditional occupancy source不足。§15の`head/len` SSOTはAllocator/typed authorityを生まない | 有限状態/部分初期化・失敗返却、長期intrusiveへの準備。**STB reallocは不要**。raw-vacantやnonCopy growの別難問を隠さない |
| **F：root/ref/Option** | correct-original-D reloanとfixed Option link更新、独立した状態fact検証 | 既存限定nativeはあるが、任意helperとの本物のsource/result/poststate結合は未確認 | ptr locatorと元Domainの区別、Option current occurrence、失効ref・wrong-Dの拒否を維持 |
| **S5b/★** | 真のsafe library owning API→凍結cJSON H1 | original B nonCopy receipt/refund、representation/membership、真の局所unchecked sourceが未成立 | H1の元の5H/6+4+2/0–5/world native/burden gateへ証拠を移すが、その条件を書き換えない |

### 4.3 旧破壊検査・Python実験の位置付け

[M378](https://github.com/wakairo/NewLang_Compiler/issues/378)が旧Draft16/17.1ノート、SurfaceDraft1、Draft17 SourceLike IR Kernel報告等の原文を照合した。歴史的source-like Pythonの8題材：**vec_grow、coalescing_allocator、arena_reset_callback、free_list_reuse、intrusive_relocation、tagged_parser、generic_hash_insert、wrapper_precision_pressure**。報告されたpytest28件・10k differential等は**当時のsynthetic/exact checkerの有限実験**であり、一般的な新Draft source受理やnative証拠ではない。Ring `head/len`/drain、Vec prefix/arena/free-listの請求・転送ルールを混同しない。過去の原source/codeをMが全件再実行したと主張せず、現実のmain語彙・安全性APIとしての採用には別途実source検証が必要。

[Compiler Testing Strategy](NewLang_Compiler_Testing_Strategy.md)にあるnumeric parsing、escape decoding、checked arithmetic、hash/equalityやsource-span testsは、まず本体CompilerのC言語側検査対象であり、NewLangのstdlib実装済みではない。過去から採用してよい考え方は、正しい原R/O/Allocation/D、strict event chronologyとold/new occurrence、`len/head`は**metadata SSOTであって権限ではない**、bounded correlated alternatives/Unknownの保守性、human obligationを過少計上しない点に限る。旧`unchecked`や`loan_span`の字面を現parserの承認済み構文と扱わない。

### 4.4 星の到達基準・停止と迂回

**区別する評価**：SPEC ≠ SOURCE CHECK ≠ source-owned artifact ≠ C17/LLVM NATIVE ≠ FORMAL ≠ historical Python ≠ PRODUCT。各星で、同一の固定実NewLang source + compiler SHA、意味論的な正例と**真の安全性負例**（semantic rejection）、native observable old/new/sibling/別インスタンス（該当するとき）、正しいruntime Err（policy failureはsemantic rejectと混同しない）、author/caller別のL/R/P/V/C、再利用する次段の能力を短く記録する。

**V377の `VALUE-ALIGNMENT-RISK`をACCEPT**：G0のただのu8返値C17成功、Fibonacciやテスト数は、C後継製品のuser WINではない。C/Zig/Rustの**同じ普通のnamed operation**と比較して、実際に移譲できたmanual proofと追加の儀式を測るまでG1 product GO/PASSと呼ばない。

**STOP/迂回**：G0が新しい意味論や一般ABI/owner機構を要する、実calleeの戻り値を独立に観測できない、G1のref引数/事後状態を現在のsourceで全く表せない、1つの星に独立した大きなsource familyが2以上必要、形式安全性を`unchecked`やhost assertionから創造してしまう場合、そこで**その星をHOLDして航路選択へ戻る**。独立F/Oは必要な反例・実証がある地点だけ、独立Rは広い安全性境界で起動し、すべての星に義務的な全Track reviewをしない。

## 5. 各Trackの作業方針と審査

- **Coordination**：V/M/P独立査定後に最初の候補星を一つ選び、その実装scopeを判断し、source・checker・nativeの拒否点と費用をIssueに記録。変更が新たなsource/semantic選択に当たるなら [§4.2](NewLang_Project_Development_Process.md)、大きな投資なら[§4.3](NewLang_Project_Development_Process.md)を適用する。無関係な全Issueへ新しいgateを追加しない。
- **P**：選択済みのsource ruleとcompiler APIのfaithful source/diagnostic/emitter。actual sourceとnative実行をprimary evidenceにする。プロトタイプflagとmainの機能を混同しない。
- **M**：選ばれた星で本当に不足する規範的source機構が生じた場合のみtargeted reopen。別途正当化された低水準design/no-foreclosure研究は継続可能だが、H1/STB productの自動GOではない。
- **F**：選ばれた星が既存の証明境界に収まるなら追加証明を強制しない。新しい安全性主張・critical allocator/authority橋が実際に提案された場合のみ独立してformalize/falsify。
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

## 7. 最初の航路と次の一件（Coordinationの暫定裁定）

**三つの独立read-only評価は完了**：
[V377](https://github.com/wakairo/NewLang_Compiler/issues/377#issuecomment-6134367162) は小さくても実際のsafe *operation*を重視し`VALUE-ALIGNMENT-RISK`、
[M378](https://github.com/wakairo/NewLang_Compiler/issues/378) は既存rule/body-visible helperと歴史的source境界、
[P379](https://github.com/wakairo/NewLang_Compiler/issues/379#issuecomment-6134477587) は**0引数unit関数とPair fixed-fieldの既存native**、
**0引数u8関数はsource CHECK-positive/backend4**と特定した。
CoordinationのGoal/Evidence/Alternatives/Stop裁定を[親Issue #375](https://github.com/wakairo/NewLang_Compiler/issues/375)に記録する。

**製品上の次の星 G1（選択した目標）**：一つの普通のnamed safe fixed-record操作を、
少なくとも**2つの独立したrecord**と通常callerで再利用し、
正しい元Copy値／更新後／無関係fieldを実際のnativeで観測する。
call側に隠れたunchecked・元D/authorityの再発行を要求せず、
author/caller双方の手作業負担と同等のC/Zig/Rust操作を比較する。
G1はまだsource CHECKにもNATIVEにも未成立であり、**実装権限のない製品目標**。

**候補となる最初のP engineering gate G0（G1のための技術的橋、まだ起動しない）**：
固定mainで既存の正例`fn scalar()->u8{u8(7)}; fn main()->unit{scalar();unit}`を使い、
source check成功→`V1-BACKEND-UNSUPPORTED`/exit4・C stdout emptyを再現/較正する。
定義済みの既存CHECKED body/returnに対応する**引数なしu8返値のC17 lowering**のみを
忠実に実装可能なら対応し、GCC/Clang nativeで実**calleeが7を返すこと**を独立observerで確認、
sourceを9に変えて9を観測。u8範囲外/返値型不一致はC生成前に拒否。
**一般引数/算術/再帰/array/新effects/owner/LLVM全面実装は扱わない**。
若干の実装が必要でも、**source/semanticsがすでに選択済み**の範囲で収まることが条件。
first blockerが新しいsource/ABI/typed owner proofなら、そのままSTOPしCoordinationへ返す。
技術的G0成功を製品価値・G1達成・H1得点として記録しない。

**G0後の再判断**：G1候補の正しいref/Copy引数、scoped write、caller currentness、
2インスタンス利用の実source受理について**最初の具体的な障害を測る**。
1つのbounded source/backend差分なら次の独立タスク候補。
複数の大きな未選択機構やhidden caller proofが必要ならG1をHOLDし、
すでにnativeのfixed-field + bounded closed-sum/F branchなど、
実証できる別の星へ分岐する。各段階で同一の小さな作者/caller負担台帳を取る。

**順序と承認**：この[PR #376](https://github.com/wakairo/NewLang_Compiler/pull/376)は
改訂した**DRAFT / DO NOT MERGE**。ユーザーの航路レビュー後、Coordinationが
**一つだけ**のbounded P G0 Issue（first compiler acceptance/diagnostic, then conditional emitter and native）を正式起票し、別Track起動Promptで渡す。
その時点まで**P実装は起動・認可しない**。Product G1の別実装を先走って承認せず、
既存cJSON H1 #268の内容・PASS/FAILとSTB #374のHOLDを固定する。
標準の元[C Replacement Coverage Map](NewLang_C_Replacement_Coverage_Map.md)のFFI/ABI/embedded/indirect-call等は長期のno-foreclosure調査対象として残す。

