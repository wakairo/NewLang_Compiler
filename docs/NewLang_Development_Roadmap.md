# NewLang 開発ロードマップ — 既存成果の実用化を優先する

> **Status: ADOPTED / NON-NORMATIVE — Coordination Issue #375 / 2026-10-13承認**
>
> 採用時点の履歴スナップショット：2026-10-13、Compiler/main `adc63b64b656b3d6e0501f29eaeee5181927dc14`、
> FormalProof/main `a8de4ebb4099f4742fc161d26867740dcac48414`。
> 本書は順序と判定基準についての**採用済み・非規範的な開発ガイド**であり、Issue/PRの起票だけで実装やTrackを開始しない。
> 言語の正本は [CURRENT_SPEC](reference/CURRENT_SPEC.md) → Draft 17.30、
> 製品の正本は [Product / Value Strategy](NewLang_Product_Value_Strategy.md)、
> [North Star](NewLang_North_Star_Product_Validation.md)、
> [C-Replacement Coverage Map](NewLang_C_Replacement_Coverage_Map.md)。
> 運用は [Development Process](NewLang_Project_Development_Process.md) に従う。
> この文書は正本を置換せず、将来の更新時は「観測した事実」と「新しい投資案」を分ける。

## 最新補遺 — 2026-10-16（下記2026-10-15 checkpointより優先）

**現行authority（確認済み）:** Compiler/main [`ad0604d2c766eac045273eba15170027f199d650`](https://github.com/wakairo/NewLang_Compiler/commit/ad0604d2c766eac045273eba15170027f199d650)、[CURRENT_SPEC](reference/CURRENT_SPEC.md) → **Draft 17.30 不変**。FormalProof/main `5437f92d099e228773d31e2323e8ee6228ca21bf`（F62/PR63の有限・条件付きLean補遺を統合済み。実compilerからの一般的な証明bridgeではない）。前回のmainからの新規採用はロードマップ／Coverage Mapの2文書を整合した[Docs PR #487](https://github.com/wakairo/NewLang_Compiler/pull/487)と、**テスト・観測専用**の[P490/PR #492](https://github.com/wakairo/NewLang_Compiler/pull/492)であり、一般のnamed-call checker／backend意味論をmainへ追加したものではない。

- **二正常分岐後の元Node読み直し（merged / 限定native PASS）：** [P #490](https://github.com/wakairo/NewLang_Compiler/issues/490)／[PR #492](https://github.com/wakairo/NewLang_Compiler/pull/492) の固定C1は、実sourceの非対称`Some(q)`跨りroot更新後、元`tail@next`を**後続のsource式でCopy read**。SOURCE/CHECK → source-issued公開owned/validator → 通常C17 → GCC/Clang O0/O2で実物理head/tailと`post_join_read=Some(original tail)`を有限実測した。一方、公開checkerの合流後`Option`とCopyは**保守的なUnknown（variant/provenance未確定）**のままであり、実行時にSomeだったことを静的権限に変換しない。
- **合流後の再match／再loan（未達 / CHECK precision）：** 同じ事前凍結C2は後続`match joined_tail` の時点で `CLI3 NODE-JOIN-POST-PRECISION`。Some(p)から元Nodeを再loanする段階、owned／C17／nativeには**到達していない**。C1のnative PASSをC2や一般再借用のPASSとして数えない。
- **名前付きNode helper（未統合の公開証明研究だけ）：** [P #493](https://github.com/wakairo/NewLang_Compiler/issues/493)の[PR #494](https://github.com/wakairo/NewLang_Compiler/pull/494)は、未統合[PR #479](https://github.com/wakairo/NewLang_Compiler/pull/479)を固定親とした、**分岐なし**実`set_link`呼出しのsource-issued entry/body/Change-Reset/old Copy/caller post公開certificateと別replay validatorの**default-OFF実験**。[独立R #495](https://github.com/wakairo/NewLang_Compiler/issues/495#issuecomment-6204729857)は初回情報隔離のもと同型別root、古いscope／occurrence、誤った旧戻り値等の公開viewを拒否できることを有限確認（**独立OOM注入は未実施**）。Rの有限SAFEは採用許可ではない。[C裁定](https://github.com/wakairo/NewLang_Compiler/issues/495#issuecomment-6204806851)により**PR #494もPR #479もDRAFT / DO NOT MERGE**。既存P478の**関数を二正常分岐内で呼ぶ**実sourceは引き続き`CHECK CLI3 P9-CONTINUATION-PRECISION`、名前付きhelperのC17/nativeも未実施。
- **CIと価値の分離：** PR #492の**統合前・固定HEAD** FULL 5構成×285／PARTIAL284成功、候補PR #494の**未統合・固定HEAD** FULL 5構成×291／PARTIAL290成功、R-only NO LLVM/GCC13・Clang成功を区別する。これらを**PR #492統合後mainの新規CI確認**や一般soundness・名付けた関数の実native成功には読み替えない。Product Strategy §1.1の**library author＋ordinary callerのUB予防監査・再監査依存閉包削減**は依然として未実証。約530行の実験的replayと5つの状態複製の費用もあり、[V #484](https://github.com/wakairo/NewLang_Compiler/issues/484)の価値HOLDを解除しない。代替は実測済みP471インラインchecked `replace` と、公平なC／Zig／同topology Rustのsafe wrapper。cJSON H1 #268、STB allocator bridge、RingのProduct GO／実装HOLDも不変。

**投資上の結論：** 技術探索の有限正例は保持するが、PR #494のProduction採用・一般effect/CFG／P9合流拡張・Rの単なる反復には進まない。[実利用者の作業事例受付 #470](https://github.com/wakairo/NewLang_Compiler/issues/470)等から、元C topologyを保持した具体操作と作者＋普通callerの監査負担の真の差を確認した場合にのみ、別Issueで§4.3/4.3.1の投資可否を判断する。本補遺は非規範的な事実・停止線の整合であり、新Trackや意味論選択を認可しない。

## 現行checkpoint — 2026-10-15（P471/PR #472統合済み、P478/PR #479は未統合HOLD。旧§0/§7より優先）

**実装の現在地:** Compiler/main [`d6e603e12591c2ff39e3e1b77c784681fec66033`](https://github.com/wakairo/NewLang_Compiler/commit/d6e603e12591c2ff39e3e1b77c784681fec66033)、[CURRENT_SPEC](reference/CURRENT_SPEC.md) → **canonical Draft 17.30（不変）**。従前のFormalProof有限証拠はcompiler→C17/native全体のsoundness証明ではない。本checkpointは[PR #458](https://github.com/wakairo/NewLang_Compiler/pull/458)・[独立R462](https://github.com/wakairo/NewLang_Compiler/issues/462#issuecomment-6186802683)、[P466 / merged PR #467](https://github.com/wakairo/NewLang_Compiler/pull/467)、[P471 / merged PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)までの**採用済み有限source/公開owned/C17/native**と、[P475](https://github.com/wakairo/NewLang_Compiler/issues/475#issuecomment-6190667128)・[P478](https://github.com/wakairo/NewLang_Compiler/issues/478#issuecomment-6191917231)の**別個の名前付きcall停止**を区別するnon-normative事実更新である。

| 領域 | 2026-10-15時点に実際にmainへ統合済みの限定能力／未到達 |
| --- | --- |
| **source/checker** | [PR #442](https://github.com/wakairo/NewLang_Compiler/pull/442)で実NewLangの`newlangc --check`とopt-in `NEWLANG_FRONTEND_ONLY=ON`を採用。check通過≠backend/native通過。 |
| **source-issued Node合流証明** | [PR #410](https://github.com/wakairo/NewLang_Compiler/pull/410)の二正常分岐SOURCE/OWNEDに続き、[PR #434](https://github.com/wakairo/NewLang_Compiler/pull/434)で二つの元lexical Nodeのcommon-entry/arm/post **公開certificateと独立validator**をmainへ統合。旧§0/§7の「未実装」は履歴情報。 |
| **Node changed-join C17/native** | [PR #438](https://github.com/wakairo/NewLang_Compiler/pull/438)で**元の2 lexical Nodeをそれぞれ`next:None`で初期化**し、scoped checked `replace`と2 normal unit match armsを持つ有限public-proof consumerを統合。実物理C root carrier、Option tag/ptr、old Copy、変化しないsiblingは[R454-B独立NON-BLIND calibrated](https://github.com/wakairo/NewLang_Compiler/issues/454#issuecomment-6185028392)で限定確認済み。一般CFG/heap owner、元nonCopy custody、安全な普通caller APIには一般化しない。 |
| **初期Someの有限C17/native** | [PR #458](https://github.com/wakairo/NewLang_Compiler/pull/458)をmainへ統合。第1元lexical Nodeへの**source-issued checked scoped read-loan→`ptr_from_ref`**を第2元Nodeの初期`next:Some(ptr)`へ直接使う狭い構築に限り、同一の元C carrierと照合してemit。元のNone初期化は引き続き対応。alias chain、heap/nonCopy owner、一般CFG/多rootは未対応。[独立R462](https://github.com/wakairo/NewLang_Compiler/issues/462#issuecomment-6186802683)は固定元source・公開checked-owned・実C17とGCC/Clang O0/O2／ASan+UBSanの物理アドレス、tag、ptr、sibling、Copyを**PASS TESTED FINITE BOUNDARY**。 | 
| **初期Some＋分岐の合成（P466）** | [merged PR #467](https://github.com/wakairo/NewLang_Compiler/pull/467)の固定 `tests/p466/some.nl`／`none.nl` は、二つの**元lexical Node**の初期Someとscoped field replace・二正常分岐を合成。SOURCE/CHECK → source-issued公開owned/validator → strict C17 → GCC/Clang O0/O2 nativeの原物理Node/Option観測を有限実測。heap Allocation/Domain custodyや一般CFGではない。 |
| **matched Some(q) cross-root合流（P471）** | [merged PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)の `tests/p471/symmetric.nl`／`asymmetric.nl` は、元head.nextの `Some(q)` から得た**実際のCopy ptr `q`**で別の元tail.nextを書き換える。対称／非対称の二正常分岐ともSOURCE/CHECK、arm/entry/postの公開owned証明＋独立validator、strict C17、元head/tailの物理アドレス・old Copy・不変payloadのnative観測が有限PASS。非対称の実行はSome armのみで、None armはCHECK/OWNED/C17（別途native実行を主張しない）。一般heap graph・owner・named callへの拡張ではない。 |
| **名前付きNode関数のP9合流（HOLD）** | [P475](https://github.com/wakairo/NewLang_Compiler/issues/475#issuecomment-6190667128)の同一 `set_link(ref<write,Option<ptr<Node>>>,Option<ptr<Node>>)->Option<ptr<Node>>` 実sourceは**現在mainでCHECK CLI3 / `P8-SIGNATURE-PRECISION`**。[P478](https://github.com/wakairo/NewLang_Compiler/issues/478#issuecomment-6191917231)の**未統合DRAFT [PR #479](https://github.com/wakairo/NewLang_Compiler/pull/479)**（固定HEAD `03b1751b513e7aef2d7ba193eda671a556f54b47`）では限定的な署名／独立definition checkを進めたが、同一named call-in-matchは**CHECK CLI3 / `P9-CONTINUATION-PRECISION`**で停止。actual/formal・original field/root・old Copy・Change/Resetを二正常分岐に結ぶsource-issued公開call-effect certificate/validatorは未成立。対象のOWNED、C17、nativeは**NOT REACHED**。PRのgreen CIは期待停止・回帰成功であってfeature PASS／main採用ではない。 |
| **独立Rの履歴** | [R437 fresh BLIND finding](https://github.com/wakairo/NewLang_Compiler/issues/437#issuecomment-6184559150)の初期Someは**当時の固定compilerで**CLI4/C0 fail-closedだった正しい履歴。[R459](https://github.com/wakairo/NewLang_Compiler/issues/459)・[R460](https://github.com/wakairo/NewLang_Compiler/issues/460)・[R461](https://github.com/wakairo/NewLang_Compiler/issues/461)の検証ハーネスINCONCLUSIVEを改ざんせず保持。自律修復を認可した新しい[R462](https://github.com/wakairo/NewLang_Compiler/issues/462)のみが新しい固定candidateで独立有限PASS（1修正commit・1 Actions）。 | 
| **独立Rの運用** | [PR #463](https://github.com/wakairo/NewLang_Compiler/pull/463)でProcess §7.6の**限定的な自律ハーネス修復**を採用。Issue事前凍結とCI/commit上限、観測・期待値変更禁止、真のcompiler不具合でSTOPを維持。これは安全性の審査基準を緩める変更ではない。 |
| **toolchain / integration CI** | SHA固定LLVM **23.1.3** [PR #451](https://github.com/wakairo/NewLang_Compiler/pull/451)は維持。**固定main `d6e603e...`**の[FULL #38727966527](https://github.com/wakairo/NewLang_Compiler/actions/runs/38727966527) はGCC/GCC Release/Clang/ASan/UBSan **各284/284 PASS（合計1420/1420）**、[NO LLVM/PARTIAL #38727966598](https://github.com/wakairo/NewLang_Compiler/actions/runs/38727966598) は**283/283 PASS**。P466／P471の採用済み統合テストを含む有限回帰結果。[PR #479 exact-head FULL/PARTIAL](https://github.com/wakairo/NewLang_Compiler/pull/479) は**未統合candidateの期待P9停止を含む試験成功**であり、named targetのCHECK/OWNED/C17/native成功ではない。 |

**製品・大きな機構のHOLDは維持:** [Product / Value Strategy §1.1](NewLang_Product_Value_Strategy.md#11-ub-prevention-audit-locality-value-hypothesis)の作者＋普通callerのUB予防**監査・再監査依存閉包の縮小**は価値仮説であり、有限Nodeのgreen C17/nativeはその優位性やsafe API閉包を示さない。 [H1 #268](https://github.com/wakairo/NewLang_Compiler/issues/268)は元の5 independent fallible heap roots、6 setup + 4 detach + 2 adoption、`B.prev=B`、元B＋dstの完全nonCopy owner custody/receipt/refund、0–5 worldsを満たす製品PASS/FAILが未決定。[STB F45](https://github.com/wakairo/NewLang_FormalProof/issues/45)/[M374](https://github.com/wakairo/NewLang_Compiler/issues/374)のallocator/epoch/typed-claim六条件はHOLD。[V411](https://github.com/wakairo/NewLang_Compiler/issues/411)/[V415](https://github.com/wakairo/NewLang_Compiler/issues/415)の通常callerを含むC/Zig/同一topology Rustとの作者＋caller L/R/P/V/C純負担差はまだProduct GOに届かない。Core compiler Explorationの限定的成功は**Product GOではない**。

**次の投資条件:** 既存機構を増やすことを目的化せず、実際のC-class利用者操作で、入力・source上の因果的結果・元pointer topologyとownerの維持・正常/失敗・普通callerへ移る責任を一つ指定する。その上で実mainのSOURCE/CHECK→公開owned→backend→nativeの最初の不足、同等のC/Zig/Rustや既存checked＋局所uncheckedなどの安価な代替、予測・実測・停止条件をProcess [§4.3/§4.3.1](NewLang_Project_Development_Process.md)に従いIssueに記録してから、別途限定的に認可する。新しいsource/semantic/API選択時には[§4.2](NewLang_Project_Development_Process.md)が必須。**このdocs更新自体は新Track、PR merge、機構実装を認可しない。**

以下の2026-10-13節・§7・古い技術的「未実装」記述は**当時の裁定と判断経緯を保存する履歴**であり、現在のmainの能力・作業許可について矛盾した場合は本checkpointと最新のIssue/PR/main証拠を優先する。

## 0. 2026-10-13 履歴スナップショット — 旧実験と当時のmerged能力を分離

この節と§7は**2026-10-13当時の事実・投資裁定の履歴**である。上の2026-10-15 checkpointを現行として参照する。2026-10-12を起点とした本文の実験履歴は、事後に一般能力へ昇格させず歴史的観測として残す。現在のCompiler/mainは `adc63b64b656b3d6e0501f29eaeee5181927dc14`、[マージ後CI](https://github.com/wakairo/NewLang_Compiler/actions/runs/38393450406) 5構成成功、`CURRENT_SPEC` は引き続き **Draft17.30**、FormalProof/mainは `a8de4ebb4099f4742fc161d26867740dcac48414`。

| 能力／問い | 現時点の証拠と境界 |
| --- | --- |
| **通常のCopy `u8→u8`関数呼び出し** | [PR404 MERGED](https://github.com/wakairo/NewLang_Compiler/pull/404)。SOURCE→CHECKED-OWNED→C17/nativeの有限経路。P399 owned／[R406 fresh BLIND](https://github.com/wakairo/NewLang_Compiler/issues/406) native **PASS BOUNDED**。任意のsignature、nonCopy/heap ownerや一般ABIは未対応 |
| **2 lexical original Node の2 normal-arm Option-link currentness join** | [PR410 MERGED](https://github.com/wakairo/NewLang_Compiler/pull/410)（元PR401の有限実装を忠実統合）。SOURCE/CHECK/公開checked-owned **PASS BOUNDED**。[R405 calibrated](https://github.com/wakairo/NewLang_Compiler/issues/405) はowned確認。field・親rootのValueFactとSome occurrenceを正しく再発行。新しいchanged-matchの**C17/nativeは未対応** |
| **Node nativeの最初の証拠不足** | [P412](https://github.com/wakairo/NewLang_Compiler/issues/412) **NO-CODE FEASIBILITY HOLD**。公開`normal_frame_unchanged`は変更を伴う2分岐に対して偽で、C carrierと共通entry・各arm・親postを結び付ける公開証明がない。証拠専用[PR413](https://github.com/wakairo/NewLang_Compiler/pull/413)は**DRAFT/未マージ**、CI成功はunsupportedの再現のみ |
| **Node join証明の設計案** | [M414](https://github.com/wakairo/NewLang_Compiler/issues/414) は **MINIMAL CERTIFICATE FEASIBLE (CONDITIONAL)**。既存public traceのみでは十分と示せず、source-issued entry/arm/post certificate＋再検証validatorは条件付き候補。**中～高の監査・保守費用、未実装、native未測定、追加P投資HOLD** |
| **利用者価値** | [V394](https://github.com/wakairo/NewLang_Compiler/issues/394)／[V411](https://github.com/wakairo/NewLang_Compiler/issues/411)は**NO INVESTMENT-READY / Product-P HOLD**。実`uthash/utlist`のunlinkと元owner解放は別責務だが、同一トポロジーC/Zig/Rustに対する通常callerを含む純便益と、狭いsource→native経路を未実証 |
| **実験証拠の扱い** | G0/G1 [PR381](https://github.com/wakairo/NewLang_Compiler/pull/381)／[PR384](https://github.com/wakairo/NewLang_Compiler/pull/384)、Python/O6、R-only [PR403](https://github.com/wakairo/NewLang_Compiler/pull/403)／[PR407](https://github.com/wakairo/NewLang_Compiler/pull/407)／[PR408](https://github.com/wakairo/NewLang_Compiler/pull/408)は固有の実験・独立検証の証拠であり、**mainで一般対応しているという主張ではない** |

**今回の実施／非実施判断：** [Coordination M414最終裁定](https://github.com/wakairo/NewLang_Compiler/issues/414#issuecomment-6146540308)は条件付き設計研究をACCEPT、ただしNode専用certificate実装・emitter拡張を**現時点では認可しない**。追加の中～高コスト証明機構は利用者操作の実証と最小の再利用可能経路が提示された時点で再評価する。C後継の計画自体は継続し、製品HOLDをNewLang全体のFAILと混同しない。

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
| **CHECK** | 実NewLang sourceをproduction checkerがその範囲で検査 | 他のsource形状、公開証明、C17/native |
| **OWNED** | source-issued公開checked-owned certificate/validatorを実際に検証 | 別形状への一般化、backend発行／native実行 |
| **C17** | そのsourceから通常CLIで厳格C17を発行・コンパイル | nativeでの元物理root／pointer一致や製品価値 |
| **NATIVE** | 発行された実C17を原sourceに対応する物理carrier観測とともに実行 | heap/nonCopyなど未試験形状、全言語soundness、製品価値 |
| **FORMAL** | 指定されたモデル・定理・前提の成立 | 実compiler・allocator・runtimeとの未証明refinement |
| **EXPERIMENT** | opt-in/未マージPRやPython synthetic研究の局所観測 | mainへ採用された機能や製品合格 |
| **PRODUCT** | 本来のworkload、topology、安全差分、人間L/R/P/V/C負担を比較 | 技術的テストの成功を製品価値とみなすこと |

## 3. 現時点の資産・不足（証拠の範囲付き）

| 分野 | 現にある成果と根拠 | 正直に残る課題 |
| --- | --- | --- |
| **M / 規範** | Draft17.30のroot/BackingRegion、nonCopy、Domain、persistent ptr、scoped ref、Change/Reset、known-call body-sensitive、fallible分岐。実際に選択されたsourceは**bounded**。[CURRENT_SPEC](reference/CURRENT_SPEC.md) | `unchecked`の概念的意味はあるが、汎用source構文・protected representation・dynamic owner claim/epoch transferを一般に実装したわけではない |
| **P / parser・checker** | C17製のproduction compiler、P0–P14の段階的source/semantic基盤（登録関数、条件分岐、loop、ref、sum、aggregate等）。[Compiler README](../README.md) | 多くの受理済み構文は現行native emitterの範囲外。source admissibility、semantic precision、codegen failureを別診断にすべき |
| **P / 実行** | mainに有限scalar/Pair/Option-link、5-root **pre-detach** の0–5 failure world、[PR #404](https://github.com/wakairo/NewLang_Compiler/pull/404)のCopy `u8→u8` direct call、[PR #438](https://github.com/wakairo/NewLang_Compiler/pull/438)の限定changed-join、[PR #458](https://github.com/wakairo/NewLang_Compiler/pull/458)の初期Some、[PR #467](https://github.com/wakairo/NewLang_Compiler/pull/467)の合成、[PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)の実matched `Some(q)` cross-root（対称／非対称）をsource／公開owned／C17/nativeの到達範囲を分けて実証。旧[PR #410](https://github.com/wakairo/NewLang_Compiler/pull/410)のchanged-join C17未対応は**当時の履歴**。 | 名前付き`Option<ptr<Node>>`のref<write>呼出しを二正常分岐に接続する公開effect証明／emitterは**main未対応**（P475 CHECK P8、未統合[P478/PR #479](https://github.com/wakairo/NewLang_Compiler/pull/479)でもCHECK P9）。一般ABI、heap/nonCopy ownerの普通caller API、cJSON detach/adoption、STB growも未実証。 |
| **F / 形式的安全性** | F0–F2および多数のbounded adjunctによるoccurrence・責任・scoped access・loopの証明。[FormalProof README](https://github.com/wakairo/NewLang_FormalProof/blob/main/README.md) | compiler-to-formal全体のsoundness theoremや実source/runtime等価性は未完成。F45はfinite epoch/physicalモデルを構築したが**allocator rich refinement 6条件HOLD**。[F45](https://github.com/wakairo/NewLang_FormalProof/issues/45#issuecomment-6091895139) |
| **O / 検証アルゴリズム** | O0–O5のsynthetic研究と[O6 #398](https://github.com/wakairo/NewLang_Compiler/issues/398)の2分岐Change/Reset相関研究は有限joinの設計上の反例を提供。同じCopy locatorでもfield F／Some occurrence P／親root RFは更新される | Python Class O/Hはcompiler-issued certificate、公開validator、native成功や形式証明ではない。O6知見はP397/PR410のsource-owned検査へ限定的に反映された |
| **R/Q / 信頼境界** | 独立Rの実source/validator/negative実測、Qの局所的なartifact-integrityレビュー。[R #359](https://github.com/wakairo/NewLang_Compiler/issues/359) / [Q #323](https://github.com/wakairo/NewLang_Compiler/issues/323) | finite/較正済み正例を全言語に一般化できない。新たなcritical compiler境界は適切なscopeのR評価を要する |
| **V / 製品と差別化** | V387（G1）、V391（5-root）、[V394](https://github.com/wakairo/NewLang_Compiler/issues/394)（候補比較）、[V411](https://github.com/wakairo/NewLang_Compiler/issues/411)（実uthash/utlist）でC・Zig・同じnode配置Rustの作者／caller負担を比較。utlistのunlinkと元owner解放の別責務は実ソースで確認した | **NO INVESTMENT-READY / Product-P HOLD**。独立の実利用者・因果的な呼び出し結果、低コストのsource-owned/native経路、総L/R/P/V/C負担の減少はいまだ立証されていない |

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

**Product / large mechanism investment remains HOLD unless a concrete author+caller benefit is shown.**
一方で、実装コスト・source/validator/nativeの最初の壁を実測し、別経路にも再利用できる知識を増やす
**探索投資（Exploration）を、製品化投資（Exploitation）と区別する**。
製品価値が未実証なことだけで、小さく可逆的な実測をNO-GOにしない。
この区別は安全性・canonical semantics・public evidenceを緩めるものではなく、
あくまで投資許可の判断基準の区別である。

### 4.0 不完全な地図での探索ルール（Coordination運用方針）

- **探索仮説を事前登録**：何が未確定か、予測する最初の停止層（parser / semantic / source-owned / C17 / native）、結果が変える次経路、再利用できる能力を一件につき明記。推定工数は**予測値**であり実測値と取り違えない。
- **情報利得を評価**：期待したProduct成果に届かなくても、分岐のコスト・正確な停止原因・別workloadでの再利用条件が分かれば探索として進歩。既知の2-Pair smokeを成功回数だけ積み増すことは情報利得が低い。
- **費用を制限し可逆化**：原則一度に一つの未確定境界。最初は未マージDRAFT枝／小さなテスト／固定SHA／単独CIから始め、失敗時はガード解除や特権的host証拠を加えて救済せず、測定結果を地図へ返す。次の修正投資は別判断とする。
- **安全性・価値を混同しない**：`SOURCE unsupported` ≠ language-invalid、`CI green` ≠ untested feature PASS、有限technical PASS ≠ 製品上の人間負担削減。意味論を変えるならProcess §4.2、重要機構投資なら§4.3を適用する。
- **停止後の再経路探索**：成功・失敗とも、残りの推定コストと新しく獲得した能力を比較して次の星を選び直す。実測した能力に独立根拠なしで「一般対応」を与えない。

**直近の具体例：** [V394](https://github.com/wakairo/NewLang_Compiler/issues/394#issuecomment-6140235445)の
`NO INVESTMENT-READY CANDIDATE`は**製品化投資の判定**として受理。
一方、既存DRAFT G1を変更せず、テスト1ファイルでAのsource-issued Copy-oldを別Bの次の状態へ渡す
[DRAFT PR395](https://github.com/wakairo/NewLang_Compiler/pull/395)は
[Actions #38361710223](https://github.com/wakairo/NewLang_Compiler/actions/runs/38361710223)で
GCC/Release/Clang/ASan/UBSan **5/5 PASS（初回）**。
**獲得した地図上の知識＝この固定ref/u8/Pair経路ではsource→checked-owned probe→C17/nativeの実データフローが合成可能**。
普通の独立safe caller・heap/nonCopy/Option/loopへの一般化やC/Zig/Rust比較上の製品優位は**未実証**。
PR395はDRAFT/DO NOT MERGE、元G0/G1も未マージ。

**履歴（2026-10-13に再裁定）：** 旧[PR396](https://github.com/wakairo/NewLang_Compiler/pull/396)で初めて観測した`P9-CONTINUATION-PRECISION`は、その後のP397/PR410で**限定SOURCE/CHECK/ownedについて解消**した。新しい2-normal changing matchはnative対応済みとはならず、P412で公開common-entry→arm→parent-post proof欠落（`normal_frame_unchanged=false`）が次の障害と確定。M414の設計案は条件付き実現可能だが追加実装はHOLD。**古いprecision失敗を現在mainの失敗として再掲しない。**

### 4.1 証拠がある分岐と、次の実用操作への障害

| 候補 | 現実の足場・最初の不足 | 次に価値を証明する条件・現時点の扱い |
| --- | --- | --- |
| **S0：既存の局所状態／fixed field** | mainにbounded scalar/Pairと`loan_write`＋`replace`の実source→C17/native。新規一般callやheap traversalは別 | **既存能力**。新しい成功数に加算しない。具体的な利用者の実作業に使って初めて再評価 |
| **G0/G1：named fixed-field operation** | DRAFT [PR381](https://github.com/wakairo/NewLang_Compiler/pull/381) `()->u8`、stacked DRAFT [PR384](https://github.com/wakairo/NewLang_Compiler/pull/384) Pair `ref<write,u8>`＋Copy-old結果は実C17/native。[R386](https://github.com/wakairo/NewLang_Compiler/issues/386#issuecomment-6136526649)は限定独立PASS | [V387](https://github.com/wakairo/NewLang_Compiler/issues/387#issuecomment-6136712325) **TECHNICAL ONLY**：普通callerの総負担削減は未証明。両PRは未マージ／**DO NOT MERGE**。同じhelperを増やすだけでは製品評価を再開しない |
| **F：persistent Option link＋元Domain再loan** | 既存の有限2 lexical original Node native link操作に加え、[PR410 MERGED](https://github.com/wakairo/NewLang_Compiler/pull/410)で2 normal-arm changed Option-linkのchecked-owned合流を獲得。旧named Option helperは別profileで未達 | [P412](https://github.com/wakairo/NewLang_Compiler/issues/412)は新しい合流のemitter入口で公開証拠HOLD。[M414](https://github.com/wakairo/NewLang_Compiler/issues/414)の最小certificate案は**条件付き・中～高費用・現時点でP投資HOLD**。R405 checked-owned PASS≠native PASS／Product GO |
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

**製品化投資については**条件に合う候補がまだ無いこと自体を**未選定**として扱う。
ただし上記4.0の小さな探索は、情報獲得の見込み・費用上限・停止条件が独立に成立するなら実施可能。
FibonacciやG2 helper、payload fieldなどを自動的なfirst product taskへ格上げしない。

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

## 7. Coordinationの当時の裁定とレビュー対象（2026-10-13 履歴）

- **Authority:** Compiler/main `adc63b64b656b3d6e0501f29eaeee5181927dc14`（[postmerge 5/5](https://github.com/wakairo/NewLang_Compiler/actions/runs/38393450406)）、canonical Draft17.30、FormalProof/main `a8de4ebb4099f4742fc161d26867740dcac48414`。
- **技術KEEP／mainの到達点:** [PR404](https://github.com/wakairo/NewLang_Compiler/pull/404)普通のone-u8 Copy関数のC17/nativeと、[PR410](https://github.com/wakairo/NewLang_Compiler/pull/410)二正常分岐のlexical Node Option field currentness SOURCE/CHECKED-OWNEDを採用。独立R406 native PASSは前者、R405 calibrated owned PASSは後者の各**限定**証拠。R402のBLIND判定はINCONCLUSIVEのまま。
- **次のnative境界HOLD:** [P412](https://github.com/wakairo/NewLang_Compiler/issues/412)でpublic entry/arm/post証拠不足を再現。test-only [PR413](https://github.com/wakairo/NewLang_Compiler/pull/413)はDRAFT/DO NOT MERGE。[M414](https://github.com/wakairo/NewLang_Compiler/issues/414)の`MINIMAL CERTIFICATE FEASIBLE (CONDITIONAL)`は中～高費用で新issuer/validatorもsoundness実測もないため、**専用certificate実装／Node C17投資は現時点でHOLD**。private arm IDの数値一致やCopy ptrから権限を発行しない。
- **製品HOLD／優先付け:** [V394](https://github.com/wakairo/NewLang_Compiler/issues/394)と[V411](https://github.com/wakairo/NewLang_Compiler/issues/411)の**NO INVESTMENT-READY C-class product operation**を維持。utlistのunlinkとfree分離は実際の研究対象だが、実利用者・正しい普通callerの判断・fair C/Zig/Rust比較上の総責任減・単一小さなsource→owned→native経路は不在。根拠なき類似サンプルの量産ではなく、**実ユーザーのmaterialな新証拠が生じるまで大きなproof/emitter機構投資を増やさない**。
- **開発継続と再開条件:** 開発を中止するわけではない。既存mainのバグ修正・品質維持と、具体的で新しい低コスト実測は別途Goal/Evidence/Alternatives/Stopを明記して限定判断できる。特定の普通C-class利用者操作、実source-levelの因果的結果、作者＋callerのL/R/P/V/C純差分と最初の小さな実装経路が揃えば、必要な機構を選び直す。Bのcertificate実装を自動的な次工程にはしない。
- **凍結の維持:** H1 [#268](https://github.com/wakairo/NewLang_Compiler/issues/268)の元の5 independently fallible roots／6+4+2／B.prev=B self-link／B+dst nonCopy custody／0〜5 failure worlds／B1–B7はPASS/FAIL UNDECIDED・専用追加投資DEFER。STB F45/M374の6条件HOLD。
- **ロードマップの採用:** 本文は[PR376](https://github.com/wakairo/NewLang_Compiler/pull/376)で採用した**非規範的な開発ガイド**。旧DRAFT PR381/384、R-only PR403/407/408、test-only PR413は個別の実験・検証証拠であり、**DO NOT MERGE**。旧PR401は統合PR410で取り込んだため未マージclosed。本文の古い2026-10-12実験報告はこの最新版の能力表・停止判断より優先しない。
- **CI evidence:** この文書採用前のmain [run #38393450406](https://github.com/wakairo/NewLang_Compiler/actions/runs/38393450406)は5/5 PASS。採用PRの正確なHEADでの検証結果は[PR376のChecks](https://github.com/wakairo/NewLang_Compiler/pull/376/checks)とCoordination裁定を参照する。**この文書の採用からsource/semantic/API選択、新規機構の実装やTrack連鎖は自動で承認されない。**
