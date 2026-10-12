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

## 4. 暫定航路 — 経由する星を検証してから北極星へ

> **航路は候補。順序・選定・到達状態は未裁定。**
> 元の「A1 named helper単独最優先」を取り消して決め打ちせず、
> 独立[Track V #377](https://github.com/wakairo/NewLang_Compiler/issues/377)（製品価値）、
> [Track M #378](https://github.com/wakairo/NewLang_Compiler/issues/378)（歴史的破壊検査・意味論）、
> [Track P #379](https://github.com/wakairo/NewLang_Compiler/issues/379)（実コンパイラ到達可能性）
> の評価を[Coordination #375](https://github.com/wakairo/NewLang_Compiler/issues/375)に集めて後で航路を裁定する。
> この図はまだP実装Issueや言語機構の着手承認ではない。

### 4.1 星図（一本道・必須通過順ではない）

```text
             ┌─ S1: 関数・値・小さな数値処理 ─ S2: 文字/バイト処理 ─┐
S0: 既存基盤 ┤                                                ├─ S5: 安全なlibrary操作 ─ ★ 北極星
             ├─ S3: 固定境界の探索/ソート ─ S4: ring/queue ─────┘
             └─ scoped ptr/ref・fixed-field mutation ──────────┘
                         └──────── 実装済みの足場を再利用
```

- **S0** = Draft17.30と現在mainで確認済みの*範囲限定*source/check/nativeを利用する出発点。
- **S1〜S5** = 新しい到達目標**候補**。構文・arithmetic・array/slice・文字列・安全な公開APIなどの実装有無を先取りしない。
- **★** = 凍結済みcJSON H1 #268（別途PASS/FAIL UNDECIDED）。STB-ARRAY-1は**独立した隣接能力のHOLD**であり、★の代用品ではない。
- 探索順は途中で変更可能。例えばarray-index sourceがないならS3/S4を後回しにし、scoped ref/fixed-fieldの別経路を先行してよい。
- 成功した中間星の結果を★の得点として加算しない。既存機構のreuseとコストを次の星へ移すことが成果である。

### 4.2 候補星と継承する能力

| 星 | 最小のプログラム/操作候補 | 正例＋**その題材に固有の誤り** | 最初に確認すべき依存 / 身に付く能力 | 現在の証拠分類 |
| --- | --- | --- | --- | --- |
| **S1 基本計算・関数** | 名前付きhelper、合計、反復Fibonacci（再帰版は**別候補**） | 値の受渡し・境界値／型・overflow・未定義名などの拒否 | nonCopy以外の普通のAPI、戻り値・分岐・反復・整数演算；**一般intや再帰が選択済みとは仮定しない** | P8–P14にはbounded source/semanticの基盤、CLI/nativeの組合せは**要P確認**；新題材自体は**候補** |
| **S2 文字・バイト** | length-bounded `memchr`/ `strnlen`相当、10進整数parse、escape decoder | 長さ0、境界直前、無効文字・桁あふれ・終端の扱いを明示 | byte representation、nonescaping bounded span、checked traversal、`Result`/失敗返却；glibcとの**意味の互換性を決め打ちしない** | Testing Strategyにnumeric parsing/escape decoding等の検査候補あり；**NewLang source/nativeは未確認** |
| **S3 基本アルゴリズム** | bounded binary search、挿入ソート | 範囲外・重複・empty input・order preservation/更新時の不正参照 | index/span、比較、loop、array elementへの安全なアクセス；**一般array/index構文は要確認** | **候補／既存のDraft意味論との対応未検証** |
| **S4 固定容量状態構造** | fixed ring buffer、stack、FIFO | empty/full、wraparound、部分初期化、stale ref、ownership refund | Ringのhead/len SSOT、raw-vacant claimとtyped live occupancy、要素移動／初期化；可変容量`realloc`を必須にしない | Draft §15 Ring/Vec設計・過去の破壊検査に**研究基盤**、main実source/native成立は**未確認** |
| **S5 小ライブラリAPI** | lexer/token stream、小さなstateful container、公開safe操作 | 不正入力・誤owner/Domain・失敗時返却・clientによる不変条件破壊を拒否 | library作者のtrue precondition局所化、call側poststate、representational protection、次のoperationでの合成 | cJSON M371/M372から欠落契約は判明、実用safe APIは**未成立**。限定的候補を再評価 |
| **★ 元の北極星** | 凍結cJSON B middle detach/adoptと元owner返却、native／負例 | [H1 #268](https://github.com/wakairo/NewLang_Compiler/issues/268)の事前登録条件を**一切変更しない** | 複数root・original B nonCopy receipt・membership・reference/delete・安全な操作合成 | 原B adoption 0/2、full native 0、**HOLD実装 / UNDECIDED製品** |

### 4.3 過去の実験から候補を引き出す（成功範囲を誤認しない）

- [Compiler Testing Strategy](NewLang_Compiler_Testing_Strategy.md)には**numeric parsing、escape decoding、source-span arithmetic、hash/equality、checked arithmetic**等をcomponent-levelテスト候補とする記述がある。これは実装済みのNewLang標準ライブラリを意味しない。
- Draft17.30 §15はRingの`head/len`、Vecの`len`をmetadata **SSOT**とする議論・局所claim責任を含む。**原理/機構の検討と一般のsource実装は異なる**。
- 過去のPython source-like/破壊検査にはVec growth、coalescing allocator、arena/free-list、intrusive relocation、parser/hash insert等を扱った報告がある可能性がある。ただしここで**原本を未再検証**の報告を実証済みcurrent Draft sourceとして列挙しない。[M #378](https://github.com/wakairo/NewLang_Compiler/issues/378)で正本・古いreport・最初の反例を照合して、星ごとの`HISTORICAL TESTED`、`CURRENT CHECK`、`CURRENT NATIVE`を分けて補強する。
- lexer/parser等NewLangコンパイラ自身の**C実装**を観察することと、それらの機能が**NewLangソースとして実装可能**なことは別。
- 「C標準ライブラリ関数の星」は、glibcの最適化assembly・C ABI・C仕様のUBまで複製する意味ではなく、NewLang利用者にも有用な**安全な同等操作**を提供すること。裸のNUL終端ポインタの長さ不明な読取りをordinary safe APIだと称さない。

### 4.4 到達判定・航路変更の基準

星ごとに、**(1)** 実sourceがparse/CHECK/NATIVEのどこまで到達、**(2)** 具体的な本物の負例と正しい診断、**(3)** 何が手作業からcompilerへ移りlibrary作者／callerのL/R/P/V/Cがどう減るか、**(4)** 次の星が再利用できる機構と未達を記録する。SPEC・FORMAL・Python synthetic・C元実装とproduct PASSを混ぜない。

同じ星で独立した大きなsource/semantic/encapsulation機構が複数必要なら**その星をHOLDして迂回または順序を変更**する。テストだけ増えて普通の利用者が書けるNewLangが増えなければ目的との整合を再評価する。Rは安全性境界のhigh-value地点で独立blind検証、F/Oは実際の疑問がある地点だけ参加する。すべての星で全Trackの儀式を設けない。

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

## 7. 直近の実行順序（暫定航路の独立査定を先に行う）

**本PRはDRAFT。航路の候補と棚卸しは公開するが、現時点の実装GOではない。**
[Coordination #375](https://github.com/wakairo/NewLang_Compiler/issues/375)の下で、以下の**一回限りの独立read-only査定**を実施する：

- [V #377](https://github.com/wakairo/NewLang_Compiler/issues/377)：本当に利用者の価値が積み上がる星の優先順位、C/Zig/Rustと比較した作者・caller側の負担、枝分かれ／迂回。
- [M #378](https://github.com/wakairo/NewLang_Compiler/issues/378)：過去の破壊検査と現行Draft17.30の対応、現sourceの要否、星の意味論的依存と過剰一般化リスク。
- [P #379](https://github.com/wakairo/NewLang_Compiler/issues/379)：**mainの実ソースとchecker/backend**を読んで各星のsource→CHECK→NATIVE可否、最初の詰まり、最も安い実行経路。

各Trackは自身のIssueへ一度だけ評価を記録してSTOP。他Trackの答えに合わせる義務はない。
Coordinationは報告がそろったら相違点・USER価値と最初の実行可能な星を[Issue #375](https://github.com/wakairo/NewLang_Compiler/issues/375)に裁定して、**このPRを追加改稿しユーザーのレビューを受ける**。
そこで初めて、必要な**一つだけの**bounded P実測/実装Issueを別途認可する。

**当面の候補A1 named helper + Copy return**もS1という一つの星に後退し、既存の意味論／backend実装可能性の有無によりS2やfixed-field等と入れ替えうる。
実装first stepで「一般int、再帰、array indexing、文字列、`unchecked`、allocator/ABIがすでにある」という前提を使わない。
SOURCE gapが単一既存機構の欠落ならその星の安価な候補を示し、
複数の広い新機構ならSTOPし他経路を選ぶ。Normativeなsource/semantic変更には
[Process §4.2](NewLang_Project_Development_Process.md)、
大きな投資には§4.3を適用するが、独立研究や小修正へ追加の定例承認は課さない。

北極星#268の元データ・PASS/FAILは固定のまま。STB #374の投資HOLDを解除しない。
一般LLVM backendの採否とFFI等長期envelopeは、この航路上で得た実行可能性に基づいて別途決める。

