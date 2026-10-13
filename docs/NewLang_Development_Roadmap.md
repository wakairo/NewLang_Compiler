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

## 4. Coordination裁定済みの暫定航路（改訂可能）

> **Status: PROPOSED / NON-NORMATIVE / user review pending — G1独立技術PASS後もPRODUCT未実証。**
>
> [V #377](https://github.com/wakairo/NewLang_Compiler/issues/377)・[M #378](https://github.com/wakairo/NewLang_Compiler/issues/378)・[P #379](https://github.com/wakairo/NewLang_Compiler/issues/379)で経由地を調べ、
> [P #380](https://github.com/wakairo/NewLang_Compiler/issues/380)→[P #382](https://github.com/wakairo/NewLang_Compiler/issues/382)→[P #383](https://github.com/wakairo/NewLang_Compiler/issues/383)でG0/G1の技術的経路を進めた。
> **G0：未マージDRAFT [PR381](https://github.com/wakairo/NewLang_Compiler/pull/381)、限定native技術PASS。**
> **G1：G0にstackした未マージDRAFT [PR384](https://github.com/wakairo/NewLang_Compiler/pull/384)でPairだけをSOURCE/CHECK→C17/native、[R #386](https://github.com/wakairo/NewLang_Compiler/issues/386#issuecomment-6136526649)がGitHub ActionsでBLIND独立 `PASS IN TESTED BOUNDARY`、[V #387](https://github.com/wakairo/NewLang_Compiler/issues/387#issuecomment-6136712325)は `TECHNICAL ONLY / VALUE NOT DEMONSTRATED`。**
> よって元の「G1が最初の製品価値達成」という仮説は実測後**未立証**。安全性の部分的検査・native成功はKEEPするが、C後継としての優位や元のNorth Star PASSへ昇格させない。

### 4.1 合流・分岐できる星図

```text
S0: mainで成立した限定scalar/Pair・fixed Option-link/Node直操作（SOURCE/CHECK/NATIVE）
  |
G0: 引数なしu8返却をC17実行（DRAFT PR381、限定technical PASS）
  |
G1: body-visible named ref<write,u8> + u8 → old Copy u8
  |  Pair 2 lexical roots、GCC/Clang、独立R PASS（DRAFT PR384）
  |  V387: TECHNICAL ONLY / PRODUCT VALUE NOT DEMONSTRATED
  |
  +── G2? F枝: Node.next Option<ptr<Node>> の非所有 named TAKE/CLEAR
  |   └─ P388: named Option signatureはP8-SIGNATURE-PRECISION (CLI3/C0)
  |      └─ V389: TECHNICAL BASIS ONLY。G2 named実装HOLD継続
  |          └─ P390: 既存direct Nodeのpost-clear old Copy→live元Node reloan実測だけ
  +── N枝: checked数値計算→sum/Fibonacci（一般演算source未実証）
  +── B枝: 有界byte処理→parse/decoder（span/index等未実証）
  +── I枝: fixed array/index→search/sort→Ring/queue（別の複合gap）
  |
S5a: 低水準安全責務の軽減を伴う実用API（価値比較まで要る）
  |
[HOLD: 真のopaque representation・original nonCopy owner receipt/refund]
  |
S5b: 所有オブジェクトの安全な公開API（未実証）
  |
★ 凍結cJSON H1（PASS/FAIL UNDECIDED; H1専用投資DEFERRED）
STB: allocator epoch/typed claim six refinements（別のHOLD）
```

- **S0**はmainの実証済み限定機能で、新機能数には加算しない。既存[Node direct-link SOURCE/native report](DRAFT_17_23_NODE_EXECUTABLE_REPORT.md)はG2 named-callの成功根拠**ではない**。
- **G0**は[PR381](https://github.com/wakairo/NewLang_Compiler/pull/381)の実calleeのu8結果（7→9等）がC17/nativeに忠実に出た技術的橋。パラメータ、算術、再帰や一般backendを含まない。
- **G1**は[PR384](https://github.com/wakairo/NewLang_Compiler/pull/384)の選択済み`Pair{left:u8,right:u8}`と`update_state(ref<write,u8>,u8)->u8`に限る。独立Rの二つのLexical Pair sourceと4 semantic negatives、GCC13/Clang native+observer [Actions #38334944637](https://github.com/wakairo/NewLang_Compiler/actions/runs/38334944637)でbounded信頼境界を確認した。main/PR381のみでは同じhelperのbackendはexit4。呼出者の`loan_write`×2、field選択、old受取は残り、Rust safe `&mut`比較で優位未実証（V387）。**「G1最初の製品価値の星」は候補名だけであり、PRODUCT達成ではない**。
- **G2? / F分岐（named実装HOLD）**：[P #388](https://github.com/wakairo/NewLang_Compiler/issues/388#issuecomment-6136990174)は固定G1 candidateで、名前付き非所有`clear_link(ref<write,Option<ptr<Node>>>) -> Option<ptr<Node>>`の構文解析に成功したが、関数登録で**`P8-SIGNATURE-PRECISION`（CLI3/C0）**と実測。戻り値`Option<ptr<Node>>`・引数ref-to-Optionが個別にsignature precision limitであり、callee/actualのchecked artifactもC17も未到達。安全性違反のsemantic rejectではない。checker定義解析と名前付きNode helper/codegenの**少なくとも二つの実装領域**が未完成だが、二つの広汎な新言語機構が必須とまでは判定されていない。old Some(ptr)はCopy locatorでowner/Domain権限ではない。**追加機構投資はHOLD**。[V #389](https://github.com/wakairo/NewLang_Compiler/issues/389#issuecomment-6137182596)は既存mainの*直接*Node/OptionリンクをC・Zig・safe-reference Rustと比較し**`V NODE: TECHNICAL BASIS ONLY`**：raw-pointer C/Zig比でliveness/permissionを有限に静的検査する技術的利点があるが、作者と通常利用者が分離したAPI・総L/R/P/V/C負担の改善は未証明。**次は[P #390](https://github.com/wakairo/NewLang_Compiler/issues/390)で追加コンパイラ機構ゼロのpost-clear old Copy locatorから原生存Nodeへの再loanを新たにnative観測する限定テストだけ**。
- **S5a**はsource-visible bodyに基づく小さな安全操作だが、製品APIやrepresentation privacyの証拠を要する。**S5b**のopaque/nonCopy安全所有API、cJSON M371/M372およびSTBの複雑なepoch/claimは依然HOLD。
- **N/B/I/Ring**は迂回可能な別枝。mainで一般加算/比較、文字列/byte span、`data[i]`、Array/Ring一般source/backendが完成したとは扱わない。

### 4.2 候補星・実際の最初の障害・再利用する能力

| 経由地 | 目標となる「実際のプログラム」 | Source/実装上の第一gap（P379・M378証拠） | 成功時に別の星へ渡せるもの / 負例 |
| --- | --- | --- | --- |
| **G0：u8 return call** | 固定body `scalar()->u8`の実calleeが7を返し、ソース9に追従 | mainはbackend4。**DRAFT PR381ではSOURCE/CHECK→C17/native限定PASS、5 CI success（未マージ）** | source-generated body/型付き返値の正しいemitter。範囲外u8や型不一致はC生成前に拒否。**Product価値判定は保留** |
| **G1：safe state transition (technical)** | 同じscoped replace helperを2独立Pairから実呼出し、旧Copy値・兄弟field・別rootを観測 | P382 SOURCE/CHECK、G0ではbackend4 → stacked **DRAFT PR384**でC17/GCC/Clang native。独立R386限定PASS、V387 **TECHNICAL ONLY / VALUE NOT DEMONSTRATED**。任意`State`は未サポート | 表現・currentness/Copy-oldを実nativeまで接続。L/P/Vの限定チェックを確認。ただし普通callerの`loan_write`やfield選択が残り、C/Zig/Rust比の製品優位は未証明 |
| **G2?：非所有Option-link named helper（HOLD）** | 2独立lexical Nodeの`next:Option<ptr<Node>>`を一つのhelperでNoneへ置換、旧Copy locatorを使用 | [P388](https://github.com/wakairo/NewLang_Compiler/issues/388#issuecomment-6136990174)：parse0、registration precision limit `P8-SIGNATURE-PRECISION`、CLI3/C0。Option resultとref-to-Option formalの両方が独立障害。mainの直接Node fixtureと固定candidate上のdirect-prepared正例はnative可、**namedはsource/check/backend未到達**。checkerとemitterの2領域、実装規模未確定 | G2 named実装HOLD。[V389](https://github.com/wakairo/NewLang_Compiler/issues/389#issuecomment-6137182596)はTECHNICAL BASIS ONLY。現在mainのdirect sourceに限定し[P390](https://github.com/wakairo/NewLang_Compiler/issues/390)でpost-clear old Someのoriginal-live reloanがnativeの実アドレス/値で観測できるかを一度だけ実測。実測しても通常safe library API/product PASSではない |
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

**V377の `VALUE-ALIGNMENT-RISK`とV387の `TECHNICAL ONLY / VALUE NOT DEMONSTRATED`をACCEPT**：G0/G1の実C17、独立Rの成功はbounded technical。C/Zig/Rustの**同じoperation**とauthor/caller負担を比べてもG1の製品優位は示せない。今後のG2?では新しいpointer/Optionのmanual proof負担を減らせるか、同じ安全性能のRust比較も含め、実現してから判定する。

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

## 7. 現在の一件・投入条件（2026-10-13 Coordination更新、V389後）

**固定authority・未マージ境界：** Compiler/main `348ca7d48f6dd5d658706f4547ab7ef9a63af33d`、`docs/reference/CURRENT_SPEC.md`→Draft17.30、FormalProof/main `a8de4ebb4099f4742fc161d26867740dcac48414`。G0 DRAFT [PR381](https://github.com/wakairo/NewLang_Compiler/pull/381)・stacked G1 DRAFT [PR384](https://github.com/wakairo/NewLang_Compiler/pull/384)・当ロードマップ DRAFT [PR376](https://github.com/wakairo/NewLang_Compiler/pull/376)は全て **DO NOT MERGE / mainにない実験機構**。本書は非normative暫定航路。

**KEEP 技術 / NOT PRODUCT：** G0の引数なしu8 C17-return、G1の選択済みPair scoped named u8 write/Copy-old C17 native、[R386](https://github.com/wakairo/NewLang_Compiler/issues/386#issuecomment-6136526649)の独立限定PASSを保持。[V387](https://github.com/wakairo/NewLang_Compiler/issues/387#issuecomment-6136712325)は同形のC・Zig・Rustとの差を検証して**`TECHNICAL ONLY / VALUE NOT DEMONSTRATED`**。静的検査・native成功を作者/caller負担の製品優位に昇格しない。

**G2 named HOLD：** [P388](https://github.com/wakairo/NewLang_Compiler/issues/388#issuecomment-6136990174)は選択済み2 Nodeの `clear_link(ref<write,Option<ptr<Node>>>) -> Option<ptr<Node>>` のparser PASSに続き、関数定義登録で **`P8-SIGNATURE-PRECISION`, CLI3/C0**。result/parameter双方のsignature制限があり、caller-owned artifact/Node named backend/native未到達。checker definitionとnative helper呼出しが少なくとも二つの開発surface。semantic-invalid拒否ではなく、二つの広域新機構が必須とまでは未確定。[Coordination](https://github.com/wakairo/NewLang_Compiler/issues/388#issuecomment-6137055271)は**G2追加実装HOLD**。

**V389 VALUE判定：** [V #389](https://github.com/wakairo/NewLang_Compiler/issues/389#issuecomment-6137182596) **`V NODE: TECHNICAL BASIS ONLY`**を[Coordination](https://github.com/wakairo/NewLang_Compiler/issues/389#issuecomment-6137219613)が受理。mainのdirect Node `None→Some→None`、old Copy locator、元生存rootでのsafe read再loanは、通常raw-pointer C/Zigより限定静的安全性検証を示すが**作者と利用者が同一lexicalプログラマー**で、opaque independently consumed library APIは未成立。二つのNodeが一方向に結ぶこの形は**safe-reference Rustでも比較的容易**であり、総人的義務削減・製品PASSは依然未実証。元fixtureはold Someの再loanが最後のclear**前**。[P388](https://github.com/wakairo/NewLang_Compiler/issues/388#issuecomment-6136990174)のdirect-preparedはclear**後**のsource/check/native exit0を得たが、post-clearの独立native pointer identity observerは未実施。この未観測の一点を次の検査対象とし、同じ観測を重複して新たな成果と呼ばない。

**次の唯一の作業（user launch待ち / test-only）：** [Track P Issue #390](https://github.com/wakairo/NewLang_Compiler/issues/390)は**変更しないmain compiler**を使い、二つのlexical Nodeの直接 `None→Some→None`後に**clearで返ったold Some Copy**（pre-clear cached `observed`とは区別）をmatchし、まだ生存する原tailのsafe `loan_read_ptr`を行う。元tailの**実nativeアドレスとpayload・headのNone・兄弟保持**を、元生成Cを改変しない観測コピーの受動的observerで新規実証する。真のsource-invalid負例はoriginal tailが死んだあとのsafe reloanで `P3-STALE-POINTER`/C0が期待される場合に限定。初回のSOURCE/PROFILE or backend拒否と区別。現実のlexical作者が手動で選ぶ対象/loanの儀式、C/Zigのliveness契約、Rust safe-referenceとの比較を記録する。**元fixture複製・native exit0のみ・改変されたguest call/host-seeded authorityでは合格しない**。新しいAPI/所有権/source/semantic/ABI/LLVM/ライブラリ実装は認可しない。

**§4.3 Stop / Owner：** Goal=低水準pointer topologyを保存しつつ人のL/P/V義務を減らす。Evidence=既存のstatic negative/Node source→nativeは有限に確実、post-clear returned-oldの実アドレスは未観測、ordinary APIとRustに対する差別化は未証明。Alternatives=今のdirect source上の未観測一点を最低費用で実測か、早期に別のC-class経由地へ迂回（一般G2 helper backendはHOLD）。Stop=正確なmain source profile/Node emitterの範囲を越える、広域mechanism二つ以上を要する、追加の人間責務削減を示さない場合は**それ以上のNode投資をSTOP**しCoordinationで別枝比較へ戻る。P390実測後の製品裁定と次の工程選択はCoordinationのみ。

**凍結対象：** cJSON H1 #268の元5 fallible roots・6+4+2・B.prev=B・full original nonCopy owner・失敗0–5世界・topology・burden gateを変更せず PASS/FAIL UNDECIDED、H1専用実装投資DEFERRED。STB allocator F45/M374六条件HOLD。正式言語変更時のみProcess §4.2、広域投資再開時のみ§4.3。

**CI区別：** [前head `e60d6e54…` run #38336253202](https://github.com/wakairo/NewLang_Compiler/actions/runs/38336253202)は5/5 SUCCESS。一つ前の[head `58dce6cf…` run #38338484099](https://github.com/wakairo/NewLang_Compiler/actions/runs/38338484099)はGCC/GCC Release/ASan/UBSan **4/5 SUCCESS**、Clangはlocked LLVM23.1.2 downloadのHTTP404で**テスト前失敗**。language/compiler findingではない。今回の文書更新で新head/runができた場合はその実測を別途確認する。checksum/pin/CIを変更・無制限再実行しない。
