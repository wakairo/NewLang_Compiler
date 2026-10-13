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
  |   └─ [P388] 同じ固定G1 compilerでSOURCE/CHECK/backendの最初の壁だけ確認
  |      └─ 不成立・2以上の広域機構が必要ならHOLD/別枝
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
- **G2? / F分岐**は[V387](https://github.com/wakairo/NewLang_Compiler/issues/387#issuecomment-6136712325)推奨の*非所有* `Node.next:Option<ptr<Node>>` へのnamed helper `clear_link(ref<write,Option<ptr<Node>>>) -> Option<ptr<Node>>` の**条件付き**経由地。old `Some(ptr)`はCopy locatorで、元rootのlive authorityを自動的に保持するnonCopy ownerではない。source grammar/semantic admission、ref formalとOption結果、C17 tagged sum loweringは**未実証**。次は[read-only P #388](https://github.com/wakairo/NewLang_Compiler/issues/388)で固定候補上の最初の拒否点を確認するだけ。実装や正式G2の選定ではない。
- **S5a**はsource-visible bodyに基づく小さな安全操作だが、製品APIやrepresentation privacyの証拠を要する。**S5b**のopaque/nonCopy安全所有API、cJSON M371/M372およびSTBの複雑なepoch/claimは依然HOLD。
- **N/B/I/Ring**は迂回可能な別枝。mainで一般加算/比較、文字列/byte span、`data[i]`、Array/Ring一般source/backendが完成したとは扱わない。

### 4.2 候補星・実際の最初の障害・再利用する能力

| 経由地 | 目標となる「実際のプログラム」 | Source/実装上の第一gap（P379・M378証拠） | 成功時に別の星へ渡せるもの / 負例 |
| --- | --- | --- | --- |
| **G0：u8 return call** | 固定body `scalar()->u8`の実calleeが7を返し、ソース9に追従 | mainはbackend4。**DRAFT PR381ではSOURCE/CHECK→C17/native限定PASS、5 CI success（未マージ）** | source-generated body/型付き返値の正しいemitter。範囲外u8や型不一致はC生成前に拒否。**Product価値判定は保留** |
| **G1：safe state transition (technical)** | 同じscoped replace helperを2独立Pairから実呼出し、旧Copy値・兄弟field・別rootを観測 | P382 SOURCE/CHECK、G0ではbackend4 → stacked **DRAFT PR384**でC17/GCC/Clang native。独立R386限定PASS、V387 **TECHNICAL ONLY / VALUE NOT DEMONSTRATED**。任意`State`は未サポート | 表現・currentness/Copy-oldを実nativeまで接続。L/P/Vの限定チェックを確認。ただし普通callerの`loan_write`やfield選択が残り、C/Zig/Rust比の製品優位は未証明 |
| **G2?：非所有Option-link named TAKE/CLEAR（条件付きF枝）** | 2つの独立lexical Nodeの`next:Option<ptr<Node>>`を同じnamed helperでNoneへ置換し、旧Copy locatorとlive-root reloanを区別 | 既存Nodeの**直接**link更新はmain nativeだが、named `ref<write,Option<ptr<Node>>>` formal／`Option`結果／C17 tagged returnは未実証。[P388](https://github.com/wakairo/NewLang_Compiler/issues/388)が**固定PR384 compilerでread-only SOURCE/CHECK/backend preflight**するまで投資判定HOLD | Option occurrence/currentness/pointer identityと依存関係を露出し、C-classの実用差分を評価する候補。copy ptrはowner権限でない。最初の拒否と実装範囲を分け、広域機構2つ以上なら別枝へ迂回 |
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

## 7. 現在の一件・投入条件（2026-10-13 Coordination更新、V387後）

**固定authority：** Compiler/main `348ca7d48f6dd5d658706f4547ab7ef9a63af33d`、`CURRENT_SPEC.md`→Draft17.30、FormalProof/main `a8de4ebb4099f4742fc161d26867740dcac48414`。正本・H1契約は変更しない。

**完了した事実と限界：**
- [P380/G0](https://github.com/wakairo/NewLang_Compiler/issues/380) の引数なしu8 returnは**DRAFT PR381 / 限定C17 native technical PASS**。main未マージ。
- [P382→P383/G1](https://github.com/wakairo/NewLang_Compiler/issues/383) は、選択済み固定Pairのordinary `update_state(field:ref<write,u8>,next:u8)->u8`を二つの独立Pairへ適用するSOURCE/CHECK→C17/nativeを**stacked DRAFT PR384**で限定実装。G0のみ/mainは同じG1 sourceをbackend4/C0で拒否。PR384の通常CIは5/5成功。
- [R #385](https://github.com/wakairo/NewLang_Compiler/issues/385) はDNS blockerで独立INCONCLUSIVE（判定維持）。専用GitHub Actions runnerで[独立R #386](https://github.com/wakairo/NewLang_Compiler/issues/386#issuecomment-6136526649)が固定PR384候補`8e80af60e8c9c8b29acb0ea6a7aad4ab5d1c8bd8`をビルドし、実NewLang source・独立C観測器・GCC/Clang strict native・4 semantic negativesから**`PASS IN TESTED BOUNDARY`**。R test-only branchは本体変更なしで未マージ。一般ref、heap/domain、owner、opaque APIや形式的soundnessの証拠ではない。
- [V #387](https://github.com/wakairo/NewLang_Compiler/issues/387#issuecomment-6136712325) は実Source/CとC・Zig・Rustの同形named helperとL/R/P/V/Cを比較し**`TECHNICAL ONLY / VALUE NOT DEMONSTRATED`**。安全性の部分改善はKEEP、作者/callerの総負担やRust safe API比の競争力は未実証。C後継の製品PASSに昇格しない。

**次の一件（user launch待ち・実装不可）：** [P #388](https://github.com/wakairo/NewLang_Compiler/issues/388)で**実装せず**既存選択済みlocal Node/Option-link操作を同一のnamed scoped helperへ移した場合の最初のSOURCE/CHECK/backend拒否点を、**固定PR384 SHA**のunchanged compilerで実測する。成功するSource/profile、意味論違反、backend exit4、環境阻害を区別し、元Copy `ptr<Node>`はあくまでlocatorでoriginal root authorityではないと確認する。read-onlyの判定後、CoordinationがGO小さなemitter研究/HOLD/迂回のいずれかを再裁定する。P388は仕様変更、新C17 lowering、形式証明、R再試験、製品PASSを**許可しない**。一つの候補に広域mechanism 2以上が必要ならHOLDし、byte/有限状態等の別枝の実現可能性を検討する。

**CI留意：** [roadmap DRAFT PR376](https://github.com/wakairo/NewLang_Compiler/pull/376)旧head `de5d833c28e2e0eb954c554485e5b922eec91df5` のCIは4/5成功、ASanのみchecksum-locked LLVM23.1.2 bootstrap HTTP404で**テスト前停止**。他PRのCIやR386 runner成功をPR376のASan成功に算入しない。pin/checksum無断変更・無制限再実行・CI停止はしない。本文の更新で新しいheadが作られた場合は、そのheadのCIを改めて照合する。

**merge/投資停止：** 本ロードマップPR376、G0 PR381、G1 PR384は全て**DRAFT / DO NOT MERGE**。current-main実装と数えず、ユーザーレビュー・merge順序裁定を別途要する。凍結cJSON H1 #268の5H/6+4+2・B.prev=B・original nonCopy custodyとSTB #374 allocator六条件はHOLD/UNDECIDEDのまま。新しい広いsource/ABI/owner/unchecked/representation機構を自動開始しない。
