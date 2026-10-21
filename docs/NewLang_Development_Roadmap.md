# NewLang 開発ロードマップ — 既存成果の実用化を優先する

> **Status: ADOPTED / NON-NORMATIVE — Coordination Issue #375 / 2026-10-13承認**
>
> 採用時点の履歴スナップショット：2026-10-13、Compiler/main `adc63b64b656b3d6e0501f29eaeee5181927dc14`、
> FormalProof/main `a8de4ebb4099f4742fc161d26867740dcac48414`。
> 本書は順序と判定基準についての**採用済み・非規範的な開発ガイド**であり、Issue/PRの起票だけで実装やTrackを開始しない。
> 言語の正本は [CURRENT_SPEC](reference/CURRENT_SPEC.md) → **Draft 17.32**（本checkpointの固定main）、
> 製品の正本は [Product / Value Strategy](NewLang_Product_Value_Strategy.md)、
> [North Star](NewLang_North_Star_Product_Validation.md)、
> [C-Replacement Coverage Map](NewLang_C_Replacement_Coverage_Map.md)。
> 運用は [Development Process](NewLang_Project_Development_Process.md) に従う。
> この文書は正本を置換せず、将来の更新時は「観測した事実」と「新しい投資案」を分ける。

## 現行実装計画 — 2026-10-21（v0-M1/M2 main統合 → v0-M3 #713 実装認可）

> **実装順・依存・到達状況に関する日付付きcheckpoint（非規範）。** 継続的な優先順位基準は[Development Process §4.3.2](NewLang_Project_Development_Process.md)、仕様の正本は[CURRENT_SPEC](reference/CURRENT_SPEC.md)→**Draft17.33**。前回のM1未実装計画と以前のcheckpointは、下記の履歴節として原文を保持する。個別Issueの起票・認可だけでTrackの実作業は起動しない。Process、canonical、Product GO条件は本更新で変更しない。

**確定したmainとCI:** Compiler/main **`e917906f0416bf6132af3b5649a4f7fc5bdf47ba`**、親 `45aa20cc1584a24c9d1a79ec99e034c7fb13c50e`、tree `ebb1eb22c028111e57f1f036f8feb43f6f157e46`。統合後の実main `push`／attempt1で[FULL5 #39607720920](https://github.com/wakairo/NewLang_Compiler/actions/runs/39607720920) GCC/GCC Release/Clang/ASan/UBSan **5/5 COMPLETED SUCCESS**、[NO LLVM PARTIAL #39607720889](https://github.com/wakairo/NewLang_Compiler/actions/runs/39607720889) **COMPLETED SUCCESS**。Candidate CI、独立R CIと統合後CIは別の証拠として扱う。

**v0-M1—採用済み・有限:** [P #701／PR #703](https://github.com/wakairo/NewLang_Compiler/issues/701)で、callee自身の2-u8 lexical originalに対するsource-issued live checkpointとordered Change/read、old/current Copy→typed return→actual callerの別originalのfield writeを、通常CHECK→真正なvalidator→C17/nativeへ接続。新しいproof境界を[R #704 fresh BLIND](https://github.com/wakairo/NewLang_Compiler/issues/704#issuecomment-6314925570)の**有限LIMITED PASS**で検証して採用。統合後main `45aa20cc1584a24c9d1a79ec99e034c7fb13c50e`の[FULL5 #39593209910](https://github.com/wakairo/NewLang_Compiler/actions/runs/39593209910) 5/5と[PARTIAL #39593210017](https://github.com/wakairo/NewLang_Compiler/actions/runs/39593210017) SUCCESS。汎用関数呼出し、foreign export、heap ownershipやgeneral soundnessを実装したとは見なさない。

**v0-M2—採用済み・有限:** [P #706／PR #707](https://github.com/wakairo/NewLang_Compiler/issues/706#issuecomment-6317379336)で、callerが渡したscoped `ref<write,u8>` formalのactual→fresh formal→元original field Change→old Copy返却→caller post-stateと別originalへの利用を実C17/native化。正しいworld/field/ValueFact、loan scope、発行元時系列をissuer-owned entry/handoffで検証する。[R #708 fresh BLIND](https://github.com/wakairo/NewLang_Compiler/issues/708#issuecomment-6317138914)もoriginal source/oracle事前凍結後、独立した2正例＋authority負例、公開証拠改変拒否、実native O0/O2を**FINITE PASS**。R自身によるprivate issuer-history任意改変・fail-at-N OOMは**NOT TESTED**であり、P所有のOOM試験と混同しない。R証拠専用DRAFT PR #705/#712はmainに混ぜない。M2統合後mainとCIは上記の確定値。

**次のproduction実装—v0-M3（着手認可済、まだ実装報告なし）:** [P #713](https://github.com/wakairo/NewLang_Compiler/issues/713#issuecomment-6317396751)を現行critical pathとする。[M #709調査とC採用](https://github.com/wakairo/NewLang_Compiler/issues/709#issuecomment-6317037846)により、Pairのtwo-normal-arm scoped field Change後の合流は**正本上合法**だが、現checkerの構造的`arm_frame`/flat-Copy制限で**CHECK3 P6-JOIN-PRECISION**が最初の障害（C17以前）。Pは2つの独立source/oracleを事前凍結し、(a)両arm更新、(b)一方だけ更新を対象として、source-issued common-entry／arm-qualified original/child facts／正常edgeのstructural join→真正なowned validator→実C17の両runtime pathと物理root/sibling→negative/OOM・FULL5/PARTIALを段階的に実装する。過去の未採用PR #510やNode MATCHの有限proofは直接のauthorityにしない。Costは工程分割に使い、必要なv0分岐機構を回避しない。候補に新しいtrusted boundaryが生じれば、main採用前に独立Rを別途認可する。**IssueのRELEASEはP実作業の自動開始ではない。**

**並行成果とProof gap:** [F #710対応監査](https://github.com/wakairo/NewLang_Compiler/issues/710#issuecomment-6317475684)は既存F1がroot/fixed child、fresh Change facts、sibling保存、EndRoot後stale-ref拒否を条件付きで扱う一方、**source-issued typed Copy origin／ordinary call→caller use**のLean refinementは未接続、と確認（core contradictionなし）。Fの新定理完了やM2 proof claimにはしない。[O #711独立Python研究](https://github.com/wakairo/NewLang_Compiler/issues/711#issuecomment-6317010900)ではfull snapshot対persistent deltaの合成比較から、**元宣言から発行Original registryの完全性**というモデル上の追加注意点を得た。ただしClass O/Hのみで実C性能・issuerの実物・OOMは未測定。**M1/M2 snapshotのproduction delta書換えはHOLD**し、実Cで有意な費用が測られるまでは次critical pathから逸れない。

**残るv0実装の独立レーン（順序を固定しない）:** fallible heap originalとnonCopy owner handoff/return・cleanup、IF return/exitと非Copy/Occurrence join、通常CLIのArray4構築とtest-only gate差、canonical v0核の`unchecked`／minimal generics等は消滅していない。個別採否・先行依存は実source→CHECK→元source-issued証拠→C17/nativeの結果で改めて裁定する。一般FFI、separate compilation、traits、concurrency、source-visible lifetime/effectsなどの明示非目標をv0必要条件に昇格させない。Product GO能動評価は[#624](https://github.com/wakairo/NewLang_Compiler/issues/624)によりPAUSED、H1 [#268](https://github.com/wakairo/NewLang_Compiler/issues/268) HOLD。bounded native成功と著者＋ordinary-safe callerのUB監査局所性Product PASSを混同しない。

## 前回の実装計画（履歴） — 2026-10-21（#700完了 → v0-M1着手前）

> **変わり得る実装計画・エビデンス地図に限る。** v0完成までの継続的な優先順位戦略は[Development Process §4.3.2](NewLang_Project_Development_Process.md#432-v0-compiler-completion-の実装優先順位継続的な方針)だけを正本とする。個別機構の採否・依存関係・予算・実行順は新たなsource/native/反例に応じて本Roadmapを日付つきで更新し、旧checkpointは履歴として保存する。言語正本は[CURRENT_SPEC](reference/CURRENT_SPEC.md)→**Draft17.33**（本checkpoint）であり、Roadmapの記述は新しいsource/proof/ABI ruleを作らない。

**基準main・完了した周辺作業：** Compiler/main `fe3e0b81d64246df14e2ec4bb76a137630582f17`、親 `493a01fab35b915b5d8ebc1195c9128d595ac195`、[統合後FULL5 #39574991400](https://github.com/wakairo/NewLang_Compiler/actions/runs/39574991400) GCC/GCC Release/Clang/ASan/UBSan **5/5 SUCCESS**と[NO LLVM PARTIAL #39574991381](https://github.com/wakairo/NewLang_Compiler/actions/runs/39574991381) **SUCCESS**（いずれもmain-push、attempt1、同じSHA）。通常CLIの名前付きC17出力[PR #695](https://github.com/wakairo/NewLang_Compiler/pull/695)・help[PR #697](https://github.com/wakairo/NewLang_Compiler/pull/697)は既に統合され、[C #696完了](https://github.com/wakairo/NewLang_Compiler/issues/696#issuecomment-6312938809)。これ以上の周辺的なhelp／sink改善を、必要なv0機構より優先しない。

**棚卸し実測→最初の真の依存：** [P #700一回限りの最終報告](https://github.com/wakairo/NewLang_Compiler/issues/700#issuecomment-6312705230)／[C採用裁定](https://github.com/wakairo/NewLang_Compiler/issues/700#issuecomment-6312988653)は、2件の**本当に呼び出される**普通のstateful関数source（Fan 1 scoped Change→Copy86→別元caller field、Meter 2 Changes＋old Copy転送→Copy137→別元caller field）が**SOURCE/CHECK0だが普通のC17はCLI4／0 bytes**で止まると実測。最初のdriver admissionはscalar-only `v1_copy_body`だが、そのguardだけ外してはlexical EndRoot後の元parent/child current ValueFact・Copy origin／typed returnが真正に確認できない。[P #692](https://github.com/wakairo/NewLang_Compiler/issues/692)の古いfact／field／Copy変更を許す共通predicate実測と[M #693](https://github.com/wakairo/NewLang_Compiler/issues/693)の安価な既存predicate転用＋**外部**stateful u8 export **NO-GO**は保存する。**内部NewLang関数合成の必要性**を、その外部ABI案のNO-GOで棄却しない。

**次の優先実装 — M1（現在の先頭・実装認可済、成果は未実装）：** [Track P Issue #701](https://github.com/wakairo/NewLang_Compiler/issues/701)で、ordinary calleeの元lexical 2-u8 recordへのscope付き1/2回更新、古い値のCopy/現在field読取り、return→本当のcall result→callerの別元field writeまでを、元source発行のcurrentness／時系列Copy由来を**検証してから**通常C17/native化する共通経路を実装する。第一段階でchecker live時点のtrustworthy snapshot／ordered operation＋Copy origin対応の最小形を選び、必要な issuer-owned historical receipt は正しい権限境界・lifecycle／OOM rollback付きで実装する。値／field名／write-count固定の専用predicateや古いfactを信じるbackend guard緩和は不可。**費用が大きいだけではこの必須機構を置換しない**。1件のP-owned DRAFT・真正source/native・負例／障害注入・固定CIの後、Cと独立Rが新trust境界を裁定するまでmainへ昇格しない。大きな意味論や権限変更が不可避なら実装途中でC/Mに戻すが、M1を必須候補から消さない。現時点では実装成功・新proof soundness・native PASS未確認。

**M1後の依存候補／並行可能性（固定した一本道ではない）：** (a) 同じfield/Copy検証核をcaller-provided `ref<write,u8>` helperとpost-state formal substitutionへ再利用するM2、(b) Pairの2-arm更新後currentness／join precisionとreturn/failure exitのM3、(c) **独立レーン**で既存 §18.1b/c LiveTail producer/recipient/terminalの**結果local経由の合法sourceがbackend4**となる一般nonCopy owner flow、(d) 現canonicalで選択されたArray<Option<u8>,4>の直接None構築が通常sourceではparser3・test-only opt-inではnative可というadmission差、(e) v0 nucleusの`unchecked`／minimal generics（未実装だからDeferredと呼ばない）。本棚卸しは**A traceがheap custodyの先行条件**だとは証明していない。heapにはAllocation/Storage/LifetimeDomain責任・失敗時cleanupが別途必要。branch joinは別arm world-qualified事実が必要。各レーンの依存辺は実装結果で再評価する。未選択のgeneral FFI、modules／separate compilation、concurrency、source-visible lifetimes/effectsやH1全操作を自動的にv0完成条件に足さない。

**次の変更条件と証拠境界：** M1の source→CHECK→authentic owned（必要な場合）→C17→GCC/Clang O0/O2 nativeで本物のcallee field・old/current Copy・caller別元field carrierを受動観測。wrong root/index/world、stale parent/child ValueFact、old Copyをcurrent readへ偽装、変異／順序差、source/owner替え、scope逃避、OOM／rollback等の意味のある拒否と独立Rのfirst-passを分ける。CI green・checker0・test-only tool・helper未呼出しのmain native0から能力を水増ししない。新しいdependency・仕様設計矛盾が見つかれば**このRoadmapのみ**を更新し、変更不要なProcess §4.3.2に個別task名／予算／順序を持ち込まない。H1 #268 HOLD、Product GO能動判定PAUSED（#624）、著者＋ordinary-safe callerのUB防止監査局所性のProduct PASSは未実証。

## 前回の実装計画（履歴） — 2026-10-21（#700棚卸し前の仮説）

> **この節は時点依存の計画・coverage地図であり、固定の実装戦略・新しい言語仕様・一括の実装認可ではない。** 優先順位の継続的な判断原則は[Development Process §4.3.2](NewLang_Project_Development_Process.md#432-v0-compiler-completion-の実装優先順位継続的な方針)、v0 source/semanticsの正本は[CURRENT_SPEC](reference/CURRENT_SPEC.md)とDraft17.33。以下の候補順は実証された依存関係に従って随時修正する。旧checkpoint／過去のNO-GO・CI verdictは保存する。

**現在地・実行中:** Compiler/main `4e7ee1f2a6426618a719cf9bb047d3b2e4636047`（[PR #695](https://github.com/wakairo/NewLang_Compiler/pull/695)の名前付きC17出力統合後）、`CURRENT_SPEC`→**Draft17.33**。[main-push FULL5 #39566806877](https://github.com/wakairo/NewLang_Compiler/actions/runs/39566806877)全5 SUCCESS、[NO LLVM PARTIAL #39566806850](https://github.com/wakairo/NewLang_Compiler/actions/runs/39566806850) SUCCESS。普通のCLIでは有限のlexical Node・2-u8 nominalのscoped field Change／old Copy、Copy返却を持つ小さな普通の関数、target-only pure scalar C17 export等は**それぞれ別の有限coverage**として確認済み。ただし任意合成、heap owner一般化、ユーザー向けの安全な公開libraryや一般FFIを達成したわけではない。[CLI help #696](https://github.com/wakairo/NewLang_Compiler/issues/696)／[P DRAFT PR #697](https://github.com/wakairo/NewLang_Compiler/pull/697)は**既に着手済みの小修正として継続**。本計画は#696を停止・差し替えず、その後の主投資の選択を改める。

**次の大きな一歩を決める最初の作業 — v0実装coverage／依存関係の棚卸し:** 大規模な新コードに着手する前に、canonicalで選択済みの意味論／既存normal CLIで実装済みの能力／限定profileやtest-only opt-in／明示Deferred／独立proofの未確認点を**区別**した簡潔なmatrixを作る。各v0必須候補について、最初のparser・checker・真正issuer・backend・native障害と、何に依存するか・別の実装法はあるかを確認する。これにより必要機構のcritical pathの**先頭**を選定する。一般のProduct GO再採点ではなくv0実装順の決定である。この棚卸しの実行者・範囲・費用は別Issueで認可する。

**第一候補（棚卸し後に採否裁定）：普通の状態変更関数を値の返却／利用まで合成する。** [P #692](https://github.com/wakairo/NewLang_Compiler/issues/692#issuecomment-6309724068)では、独立した1回／2回のscoped write後のCopy `fn()->u8`がSOURCE/CHECK・真のbody/call resultまで到達する一方、**選択済み実験的C17外部exportはCLI4**で、以前の汎用node predicateはstale parent/child fact・wrong field index・old Copy resultを**独立した時系列証明としては拒否できなかった**。[M #693](https://github.com/wakairo/NewLang_Compiler/issues/693#issuecomment-6310429018)の**NO-GO-DEFER**は「既存証拠だけの安価なshared validatorとstateful外部u8 export」という**特定の案・範囲**に対するもので、普通のv0関数合成の必要性を否定したものではない。まず**NewLang内部のordinary call→stateful body→typed result→caller側の実使用**がv0完成にどこまで必要かを確かめる。必須であり真正なsource-issued時系列receiptが必要なら、費用の大小ではなく正しい依存関係と安全性を基準にM/P/F/Rの段階的設計・実装を別途検討する。**実験的foreign C17 u8 exportの拡張と混同しない**。現在の記述は新issuerの認可ではない。

**続く重要な候補（厳密な直列順序は未確定）：** fallible heap-originalのAllocation/LifetimeDomainとnonCopy ownerの関数間handoff・終了／refund／teardownを、既存の**限定されたheap／LiveTail源・C17/native証拠**からどこまで一般化すべきか評価する。分岐／失敗時／複数元rootと実値のcurrentness・依存関係を、少数の既存fixture専用gateでなく適切なcompiler経路として合成する。Array4等の**test-only opt-in**と普通のCLI対応の差も、選択済みsource構築に対して明示的に扱う。どれが先行条件かは棚卸しで確認し、未選択のsource-visible lifetime・汎用FFI・concurrency・separate compilation等は自動的にv0必須へ追加しない。実装コストの大きさそれ自体で必須機構を永久に後回しにせず、検証可能な小工程に分ける。

**到達判定と変更条件：** 重要な段階ごとに**二つ以上の独立したsource／期待値のある正常系・負例／正確なissuer origin（必要な場合）／C17/nativeの実結果／OOM・rollback・本当の権限境界の拒否**を区別して実測する。適用する独立R/Fは危険なproof/issuer/owner境界の実態に従う。checker成功・PR CI green・test-only opt-in・C consumerのstatus0だけで一般実装やProduct GOを宣言しない。実際の依存関係、言語矛盾、信頼できるissuer不成立、別のより簡潔な設計が分かったときに**この計画順序を更新**する。必要機構の見積費用増加は工程分割・再裁定の理由だが、無関係な周辺修正へ逃れる理由にはしない。完成criteriaと必要機構のmatrixは棚卸しで明記するまで**未確定**。H1 #268 HOLD、Product GO能動採点PAUSED（#624）は維持する。

**旧checkpointの扱い：** 次節の2026-10-19版は当時の判断・成果の履歴であり、この現行計画で上書きしない。旧節のDraft17.32や旧SHAを現mainのauthorityと誤読しない。

## 前回checkpoint（履歴） — 2026-10-19（#624採用：v0 compiler-first、Product GO能動判定を一時保留）

**Coordinationによる投資段階の変更：** [Issue #624の裁定](https://github.com/wakairo/NewLang_Compiler/issues/624)は、固定Compiler/main [`35fa23efaa641b96c610324735b714891be2bbac`](https://github.com/wakairo/NewLang_Compiler/commit/35fa23efaa641b96c610324735b714891be2bbac)、[CURRENT_SPEC](reference/CURRENT_SPEC.md)→**Draft17.32**を出発点とし、**v0 compilerを普通に使えるところまで実装する作業を最優先**にする。Product GO二段階条件は変えず、通常compilerの次工程を決めるための仮想的なProduct GO採点・追加市場調査を**初回実利用checkpointまで能動的に一時保留**する。H1 #268はHOLD／UNDECIDED、L3 Product GOは未達。これは製品化のGOでも一般的なsoundness PASSでもなく、v0の実装と実使用から正負の情報を得る限定Explorationである。Product Strategy §§1.1–1.4と[Process §4.3.1](NewLang_Project_Development_Process.md#431-exploration-go-and-product-go)が正本であり、本checkpointは実装の優先順だけを示す。

**既にある通常CLIの足場を基準にする：** [READMEの`newlangc --check SOURCE`／`newlangc SOURCE`からのC17生成手順](../README.md)と、既に採用済みの限定的な`usize +/-/<`、lexical Node link操作等をまず整理する。前者は実行手順の既存例であり、後者は別々の有限source profileで実測された機構であって、**互いに任意に合成できると推定しない**。Array4はtest-only opt-in（通常CLIではdefault OFF）。旧未マージ実験とtest-only profile、証拠だけのvalidatorを通常ユーザー向けの能力と数えない。新たな一律source rule、一般heap owner返却、safe library facade、separate compilation、full H1は今回の実装認可外。

**初回実利用checkpoint（USER-CLI-1、到達前の目標でありPASS宣言ではない）：**
1. 既存READMEの通常`newlangc`機能を出発点に、**新しい独立した小規模なsingle-semantic-unitの実利用者風source**を固定する。選択済みv0の範囲だけを用い、実際に入力・値・状態変化を確認できる最小経路を選ぶ。既存fixtureに条件分岐で特例を足すことを成果としない。
2. 通常CLIの登録済み**SOURCE→semantic CHECK→必要な真正なsource-issued公開checked-owned→strict C17→GCC／Clang native**を同じoriginal programで接続し、元の値／physical carrierを該当範囲で観測する。公開ownedが必要ないCopy操作に無理にprivileged issuerを足さない。入力／出口コードと再現手順を保存する。
3. 少なくとも対応する**正当な意味論上の負例**と、まだ受理されないが仕様上合法なsource／emitter不足を区別し、診断・非ゼロ終了・部分的なC17出力や資源cleanupを実際の失敗経路で確認する。未対応は**fail closed**し、source ruleの黙示的拡大・元authorityの偽造・warning suppressionで突破しない。
4. 最初にぶつかる**parser／semantic／source-issued proof／C17／nativeの実障害**と作業・CI費用を記録し、別の小さな利用者sourceでも再利用できるか判断して次のbounded P taskを選ぶ。いきなり全Draft機能・一般owner・compiler→formal全面対応を要求しない。

**順序・資源・STOP：** Coordinationは既存通常CLI、テストと未解決のsource/backend境界の短いread-only調査後、最初の一つの小さなP作業を別Issueで認可する。Pが主担当、Mは生じた最小意味論問題、F/Rは新しい権限や高リスク機構の独立安全性確認、Tは再現性障害を必要な場合のみ扱う。新Trackの形式的な常時稼働を義務づけず、v0実用性へ接続しない専用profileや既知の成功数の増殖を避ける。初回checkpoint到達、根本的unsoundness、通常使用を阻む設計の不自然さ、予算・scopeの急拡大、具体的で強い製品反例があれば**Coordinationで優先度とProduct GO評価再開の要否を再裁定**する。独立R/F、§4.2意味論歴史照合、§4.3重要投資gateは不変。内部の[技術記事 #619](https://github.com/wakairo/NewLang_Compiler/pull/619)はユーザーの編集レビューを待ち、変更・外部公開しない。

以下の2026-10-19の**#610能力checkpointは採用済みの有限能力を記録した履歴**としてそのまま保存し、この方針転換によって当時のテスト結果・独立R判定を遡及的に変更しない。

## 履歴checkpoint — 2026-10-19（#610当時：採用済みflat usize + / - / <・独立R／PoC-firstの到達範囲）

**authority／postmerge確認：** [Issue #610](https://github.com/wakairo/NewLang_Compiler/issues/610)の固定Compiler/mainは[`10f9efad73de7a28e39a80a3a95ae24cdefbda90`](https://github.com/wakairo/NewLang_Compiler/commit/10f9efad73de7a28e39a80a3a95ae24cdefbda90)（親`2cca0c6118cdf725ee68b48e07fc566b29622230`）、[CURRENT_SPEC](reference/CURRENT_SPEC.md)→**canonical Draft17.32**。この**統合後mainそのもののpush**で[FULL5 #39187181421](https://github.com/wakairo/NewLang_Compiler/actions/runs/39187181421)がGCC Debug／GCC Release／Clang／ASan／UBSan **全5 job COMPLETED SUCCESS attempt1**、[PARTIAL #39187181424](https://github.com/wakairo/NewLang_Compiler/actions/runs/39187181424)も**COMPLETED SUCCESS attempt1**。各候補HEADの先行CIとは分離して記録し、単なるCI greenをgeneral soundness／Product GOへ昇格しない。

**真正sourceから実行までの有限数値機構：** [adopted PR #590](https://github.com/wakairo/NewLang_Compiler/pull/590)は既知・依存なし・正確なcore `usize` Copy operandの**単一flat `usize + usize`**を数学的nonoverflowの事前証明つきでSOURCE→semantic CHECK→元登録source-issued公開checked-owned view/validator→C17の元`size_t` guest carrier加算/nativeまで接続（[R #592 fresh BLIND LIMITED ACCEPT](https://github.com/wakairo/NewLang_Compiler/issues/592#issuecomment-6248199360)、後続のNON-BLIND calibrated 37+59=96とは別証拠）。[adopted PR #599](https://github.com/wakairo/NewLang_Compiler/pull/599)は同じ有限profileの**`usize - usize`**（`lhs>=rhs`を機械算術前に証明しunderflowを拒否）を、既存PLUS公開APIを維持しながら**private operation tag付き共有PLUS/MINUS source-issued view/validator**で採用。実際の`+/-`を元C17 guest値で計算するが、operator chain／nested／不明dynamic range／一般unsigned modulo／新たなptr・Domain・Allocation権限は認めない。

**共有`+/-`公開信頼境界に対する独立R：** [R #598 INCONCLUSIVE](https://github.com/wakairo/NewLang_Compiler/issues/598#issuecomment-6248797333)、[R #600 fresh BLIND INCONCLUSIVEと後続bounded smoke](https://github.com/wakairo/NewLang_Compiler/issues/600#issuecomment-6248924689)、[R #601 Phase 1 NON-BLIND INCONCLUSIVE](https://github.com/wakairo/NewLang_Compiler/issues/601#issuecomment-6249155511)を保存し、[R #601 Phase 2 **NON-BLIND calibrated LIMITED ACCEPT**](https://github.com/wakairo/NewLang_Compiler/issues/601#issuecomment-6256629853)を別の後続結果とする。Phase 2は登録sourceでの本物の発行＋view/validator、**同じ数値IDを持つ別関数contextの跨ぎ証拠拒否**、`+`と`-`のcross-op／ゼロ結果取り違え拒否、両演算各9種の**真正なissued-receipt改変拒否**、backend validation gate、GCC13/Clang O0/O2の受動元guest値（`+,-,+`実例18/11/14）を限定確認。[Cの限定採用](https://github.com/wakairo/NewLang_Compiler/issues/601#issuecomment-6256681942)と[統合後main #2cca0c6](https://github.com/wakairo/NewLang_Compiler/issues/601#issuecomment-6256707220)のCIは別層。**R側の実allocator fail-at-N／issuerとevent-listのOOM/rollback、`FILE*` transactional failure、破損proofのbyte-stream出力rollbackはNOT RUN**であり、P所有のfault試験を独立R PASSに変換しない。BLIND・calibrated・candidate CI・postmerge CIを取り違えない。

**新たなordinary Copy比較：** [adopted PR #604](https://github.com/wakairo/NewLang_Compiler/pull/604)／[P #609最終](https://github.com/wakairo/NewLang_Compiler/issues/609#issuecomment-6257867840)は同じ**単一flat／正確な既知・依存なしのcore `usize < usize`**に限り、元sourceの左→右operandを実際のC17 `size_t` carrierへ順にstageし、その**実値**を比較して登録済みsource由来のCopy `bool`（root 0、memory authorityなし）を返す。旧GCC13 strict `-Werror=type-limits`実失敗は履歴に保持し、新しいCでMAX<0等をwarning抑制・checker-known真偽置換なしに解消。**9正例×GCC13/Clang×O0/O2×plain/passive＝72 native観測と18 SOURCE/CHECK負例**（P証拠）、候補`7157413901ee3235abf8ed28f71282e8feec93a7`の[FULL5](https://github.com/wakairo/NewLang_Compiler/actions/runs/39186163584)・[PARTIAL](https://github.com/wakairo/NewLang_Compiler/actions/runs/39186163517)はSUCCESS。**`<`の新しいprivileged public issuerや独立R native resultは存在しない**。元operand currentness・Copy観測と公開`+/-`権限の信頼境界は別。一般operator precedence／dynamic range／Array4通常CLIは未選択（Array4はtest-only opt-in／default-OFF）、C28xの16-bit byte target backendも未対応。

**PoC-firstの価値検証と次の投資限界：** [V #606のPoC-first採用](https://github.com/wakairo/NewLang_Compiler/issues/606#issuecomment-6257229012)／[V #607 L1最終packet](https://github.com/wakairo/NewLang_Compiler/issues/607#issuecomment-6257583407)／[C編集裁定](https://github.com/wakairo/NewLang_Compiler/issues/607#issuecomment-6257721074)は、既に採用された**3つの本物の元LEXICAL Nodeに対する2回の順序付きscoped `next`更新**を、固定P572 source→CHECK→本物の公開owned→strict C17/nativeの**過去の有限実測**と、**別sourceによるR588 calibrated**証拠に基づくL1紹介資料にまとめた（V607が現mainで再実行した証拠ではない）。fairにfactorしたC helper、有限Rust `Cell`例は**例示／V自身未コンパイル**、成熟したRust intrusive safe O(1) owned-node手段も現実的代替。外部掲載は**未承認・ゼロ投稿**、L2一般programmerからの自発的反応**ゼロ**、[#470](https://github.com/wakairo/NewLang_Compiler/issues/470)は既存受付のみ。返却borrow／lending decoderは**Deferredな構想**で、実行可能なPoCではない。現行NewLangは**実fallible heap-original nonCopy owner／detach・adopt・refund、membershipを隠す再利用可能なordinary-safe library API／一般O(1) list・ABI**をまだ提供せず、[H1 #268](https://github.com/wakairo/NewLang_Compiler/issues/268) HOLD／UNDECIDED。次は実践者にとっての具体的な反証・要求を確認してから、[Strategy §1.1](NewLang_Product_Value_Strategy.md)の**well-factored Cと比較したlibrary author＋ordinary-safe callerのUB防止L/R/P/V/C監査・再監査依存閉包**を測る。§1.3／[Process §4.3.1](NewLang_Project_Development_Process.md)では費用限定の再利用compiler能力をExplorationとして並行可能だが、大きな新proof／owner／encapsulationは別裁定、**L3 Product GOは未実証／HOLD**。

以下の**2026-10-18「現行」checkpointは当時の採用状況（減算実装前）として正確な歴史**であり、本日のmain能力に書き換えない。

## 履歴checkpoint — 2026-10-18（PR #576／#586／#590採用、再利用可能compiler能力と製品HOLD）

**固定authorityとmain更新の区別：** Issue [#595](https://github.com/wakairo/NewLang_Compiler/issues/595)の登録時Compiler/mainは[`12c2c8e3338201ec3bda00a63ddac841ed80e574`](https://github.com/wakairo/NewLang_Compiler/commit/12c2c8e3338201ec3bda00a63ddac841ed80e574)、[Coordinationの追記](https://github.com/wakairo/NewLang_Compiler/issues/595#issuecomment-6248376239)に従う本checkpointの**現在mainは[`fdae44e62e39bc521fc4bcab70909d4d7e1d6155`](https://github.com/wakairo/NewLang_Compiler/commit/fdae44e62e39bc521fc4bcab70909d4d7e1d6155)**。両mainで[CURRENT_SPEC](reference/CURRENT_SPEC.md)→**canonical Draft17.32**、Compass／Strategy／Processと旧2文書は同一。**SPEC／真正source CHECK／公開owned／C17 emission／物理native／Product GO・人間BURDEN**を別証拠として記録する。

**採用済みNode有限能力（固定旧main時点から）：** [P #572／merged PR #576](https://github.com/wakairo/NewLang_Compiler/pull/576)、固定候補`87fc878b52763858f69678a8350d8c1b66a061bb`は、登録済み関数sourceからの**3 distinct original LEXICAL Node**、宣言順`c,a,b`、二つの順次scoped `next:Option<ptr<Node>>` field Change（`a.next:=Some(b)`→`b.next:=Some(c)`）の**真正な公開checked-owned issuer／validator**からGCC／Clang O0/O2のstrict C17／同一元物理carrier nativeまで有限実測。[R #580 fresh BLIND](https://github.com/wakairo/NewLang_Compiler/issues/580#issuecomment-6235409058)**INCONCLUSIVE**と[R #583 NON-BLIND calibrated](https://github.com/wakairo/NewLang_Compiler/issues/583#issuecomment-6241402075)**INCONCLUSIVE**は保存。[R #588 NON-BLIND calibrated LIMITED ACCEPT](https://github.com/wakairo/NewLang_Compiler/issues/588#issuecomment-6242719977)で正しい受動observerによる元アドレス・validator・選択hostile負例を限定確認し、[Coordination](https://github.com/wakairo/NewLang_Compiler/issues/588#issuecomment-6243377028)が有限採用。**後続のLIMITED ACCEPTをfresh BLIND PASSへ遡及させない**。元`c,b,a`はCHECK0でも公開issuer precision拒否、4th root／3rd ChangeはCHECK0でもC17 CLI4／0 bytesでfail closed。candidate[FULL5](https://github.com/wakairo/NewLang_Compiler/actions/runs/39038332005)＋[PARTIAL](https://github.com/wakairo/NewLang_Compiler/actions/runs/39038332020) PASSと、P側のOOM／攻撃検証は、網羅的な**独立Rのhostile C consumer／OOM/rollback**を保証しない。

**C17 target安全性とC優位の限界：** [P #584／merged PR #586](https://github.com/wakairo/NewLang_Compiler/pull/586)は選択済みemitterに共通の`CHAR_BIT==8`・真正なunsigned `uint8_t`（`UINT8_MAX==255`・size／値域）の**C17 compile-time guard**を統合。host GCC/Clang native／synthetic wide-byte target negativeとcandidate[FULL5](https://github.com/wakairo/NewLang_Compiler/actions/runs/39070652075)・[PARTIAL](https://github.com/wakairo/NewLang_Compiler/actions/runs/39070652123)成功は**非対応targetを拒否する証拠**で、TI C28xの移植・本物のCGT／デバイス／全ABIではない。[V #579](https://github.com/wakairo/NewLang_Compiler/issues/579#issuecomment-6235402221)／[C裁定](https://github.com/wakairo/NewLang_Compiler/issues/579#issuecomment-6241021539)はTI **F280039C／C28x**の実vendor C compiler／SDK／ADC firmware工程を、現に**強いCツールチェーン優位**と評価（維持されたRust C28x経路は未実証）。しかしC28xは`CHAR_BIT=16`、SDKの`uint8_t`互換aliasも16-bitであり、**現行NewLangのexact-u8 C17 backendはこの対象で使用可能と実証されていない**。現物TI実行とNewLang対factored TI Cの監査負担差は未測定で、C有利＝NewLang製品GOではない。

**最新mainの小さなquantity拡張（旧固定SHAから後に採用）：** [P #589／merged PR #590](https://github.com/wakairo/NewLang_Compiler/pull/590)、固定HEAD`54986f2b227fc4999204027ee54fc50c94b546e9`を上記mainへsquash採用。**既選択の1個のflat `usize + usize`**のみ、依存なしの既知・正確なcore Copy quantity operand、事前に数学的非overflowを証明し、**本物の登録済みsource-issued公開checked-owned receipt／validator→C17で元guest carrierを加算→GCC/Clang native**へ接続。[R #592 fresh BLIND LIMITED ACCEPT](https://github.com/wakairo/NewLang_Compiler/issues/592#issuecomment-6248199360)は13/13事前凍結source outcome、コピーした公開viewの13/13改変拒否、厳格GCC/Clang O0/O2の20/20 native compile/executeを確認し、**BLIND verdict凍結後**のNON-BLIND calibrated observerは実値**37＋59＝96**を4構成で元carrierから確認。[Coordination採用裁定](https://github.com/wakairo/NewLang_Compiler/issues/592#issuecomment-6248370657)は限定的な残存リスクを受理、**一般四則・nested/chained/未知range／uintptr演算／owner authorityは未実装**。Rは**独立foreign-context同値ID攻撃／real owned proof改変→backend／発行・出力OOM/rollbackを未実施**（Pの別OOM・原proof tamper自前テストをR PASSと混同しない）。候補固定HEADの[FULL5](https://github.com/wakairo/NewLang_Compiler/actions/runs/39086193507)／[PARTIAL](https://github.com/wakairo/NewLang_Compiler/actions/runs/39086193527)成功は**統合後mainの新しい総合CI PASSではない**。[M #593／C source裁定](https://github.com/wakairo/NewLang_Compiler/issues/593#issuecomment-6248105142)は将来の**単一flat `usize - usize` SOURCEの最小設計**（`lhs>=rhs`下限条件）だけ選択；**減算SOURCE→公開proof→C17/native実装・PASSは未到達**、新しいP開始を意味しない。

**既採用投資方針／製品境界：** [Product Strategy §§1.1・1.3](NewLang_Product_Value_Strategy.md)、[Process §4.3.1](NewLang_Project_Development_Process.md)と[merged PR #582](https://github.com/wakairo/NewLang_Compiler/pull/582)に従い、(A) 既選択canonicalに忠実で**異なるsourceにも再利用可能な低費用のSOURCE→CHECK→authentic proof→C17/native**は、個々のworkload Product GOを待たず**Explorationで並行推進可能**。(B) 大きな新ownership／一般proof／effects／encapsulation／target ABIには§4.2・§4.3の別判断と独立R/F・negative/OOMの安全性検証。(C) Vによる実際のC採用理由と、**NewLang対十分にfactorしたC**のlibrary author＋ordinary-safe callerのL/R/P/V/C UB予防監査・再監査依存閉包を同時に調査する。Rustが実用的でもCに重要な合理性がある領域は候補から一律除外しない。**test-only Array4 `newlangc_p534`／通常CLI default-OFF、fallible heap-original nonCopy所有者の返却・refund、protected membership／ordinary-safe API、O(1) full detach/attach、H1[#268](https://github.com/wakairo/NewLang_Compiler/issues/268) HOLD／UNDECIDED**は不変。新機構・製品GOへの自動昇格や固定資源quotaは認めない。

以下の**2026-10-17夜checkpointは当時の主張・独立検証・未起動Issueを保存する歴史**であり、現在の統合済みPRや最新のR判定に置き換えて読まない。

## 履歴checkpoint — 2026-10-17 夜（O(1)操作と二段階製品判断／Coordination #570）

**Authority / 技術的な現在地：** Compiler/main [`a310c9b9dae5b507b2388fcc6cb39597267d1448`](https://github.com/wakairo/NewLang_Compiler/commit/a310c9b9dae5b507b2388fcc6cb39597267d1448)、[CURRENT_SPEC](reference/CURRENT_SPEC.md)→**canonical Draft17.32**。通常`newlangc`で採用済みの有限例は、元**lexical** Nodeの`Option<ptr<Node>>`、scoped field `loan_write`／`replace`、二正常分岐での別元root更新・Copy再読（[P471実source](../tests/p471/asymmetric.nl)、[merged PR472](https://github.com/wakairo/NewLang_Compiler/pull/472)、[PR492](https://github.com/wakairo/NewLang_Compiler/pull/492)）と、元lexical Pairの条件選択後の単一field更新・有限Copy関数relay（[merged PR515](https://github.com/wakairo/NewLang_Compiler/pull/515)／[PR523](https://github.com/wakairo/NewLang_Compiler/pull/523)）。対応する**SOURCE→CHECK→真正な公開checked-owned→strict C17→元物理carrierのnative** は各固定profileで到達しているが、一般source/heap graph/製品APIではない。Draft17.32の直接4×`Option<u8>::None`による`Array<Option<u8>,4>`は[merged PR543](https://github.com/wakairo/NewLang_Compiler/pull/543)の**test-only `newlangc_p534` opt-in**で登録済み1/2元rootの公開owned・C17/nativeを有限確認（[R545 calibrated](https://github.com/wakairo/NewLang_Compiler/issues/545#issuecomment-6227096033)、[R544 BLIND finding](https://github.com/wakairo/NewLang_Compiler/issues/544#issuecomment-6226699593)は履歴維持）。**通常CLIはArray4 SOURCE default-OFF**で、[P546](https://github.com/wakairo/NewLang_Compiler/issues/546#issuecomment-6227459153)のCopy issuer/C17と[M547](https://github.com/wakairo/NewLang_Compiler/issues/547#issuecomment-6229831374)のsource適格性未裁定を理由に一般化しない。

**目標を変えずに製品候補を選ぶ第1関門（[Strategy §§1.1–1.2](NewLang_Product_Value_Strategy.md)、[Process §4.3](NewLang_Project_Development_Process.md)）：** Cの実際の**既知・現在所属する元NodeポインタによるO(1) detach**は生存／所属が正しいことを人間が保証する前提であり、index検索から始まるC detachはO(n)になり得る。[V526](https://github.com/wakairo/NewLang_Compiler/issues/526#issuecomment-6214336649)の実cJSON 2020年last-item修正は`head.prev`の古いtailが削除済み元Nodeを指し、後続の正当なappendでUAF書込みに至り得る**作者側のwriter→reader→delete再監査依存閉包**を特定（sanitizer crash実測ではない）。[V552](https://github.com/wakairo/NewLang_Compiler/issues/552#issuecomment-6230980170)は実Rust 1.99.0の元heap Node／private `unsafe`・safe facadeと非Copy lease拒否を有限native確認。[V563](https://github.com/wakairo/NewLang_Compiler/issues/563#issuecomment-6233693474)でAIが選んだ**std-only `Vec<Box<Node>>`**は元Boxアドレスと値・返却を保っても`position`＋`Vec::remove`で**O(n)**：**O(1)同等性は未達**。元の比較IssueはO(1)を必須条件に固定しておらず、この実測から「RustがO(n)を強制する」「AI一般が必ず退化させる」とは言えない。

**強い反証のある同じ第1関門：** [V567](https://github.com/wakairo/NewLang_Compiler/issues/567#issuecomment-6233871201)／[C裁定](https://github.com/wakairo/NewLang_Compiler/issues/567#issuecomment-6233968029)は**実`rustc 1.99.0`＋`intrusive-collections 0.10.3`**で元heap `Box<Node>`の物理intrusive linkを維持し、private indexed slot／generation registryとopaque safe Handleの事前検証→真のmemberのunlink→同じ元Box返却という**valid known Handleに対する操作全体のworst-case O(1)（list node数に対して）**をsource/APIとnative 4/4で確認。stale／foreign拒否も有限確認。反面、公開APIはraw `cJSON*`とは別物で、追加membership registry・handle/issuer・private `unsafe`のauthor証明を要し、slot非再使用により**retained metadataはO(total historical appends)**。appendのslot伸長は償却O(1)、OOM・exact C ABI・長期churn・実利用者の受容／保守工数は未実証。Rustの安全な普通callerに対する保護と、Cの手動の元ptr安全責務の両方を公正に数える。**Rustの機能的O(1)実現可能性はPASS、自然な設計／API／長期保守の十分性はOPEN、C優位ターゲット選定もOPEN**。[F-1 #559](https://github.com/wakairo/NewLang_Compiler/issues/559#issuecomment-6233218690)のSQLite builder recoverable failureは指定範囲で**Rust十分／NO TARGET**、[P2 #561](https://github.com/wakairo/NewLang_Compiler/issues/561#issuecomment-6233302504)のClay C-source-only義務は**未証明／HOLD**。別workloadへ一般化しない。

**第2関門の目的地（未到達）：** Cを使う合理的理由が実ユーザー契約で残る場合、**同一original heap Node／Allocation・LifetimeDomainとO(1) detach/attach、元のownerを正しく返却・refund／失敗時責任・teardownまで守るNewLang-native低水準library**を目指す。必要なのは(1)実際のfallible **heap-original nonCopy owner／root/Allocation/Domainのsource発行・安全な返却／refund**、(2)**membership・alias・原リンク不変条件を保護するrepresentation hiding／ordinary-safe API**、(3)general source `unchecked`の真の前提の局所化とwriter・callback・破棄までの依存閉包、(4)利用者が要求するなら実ABI／FFIであり、現在のlexical元Node技術／test-only Array4からは導けない。`ptr`数値、Copy handle、host seed、metadata、per-hop uncheckedだけから生存・所有・所属権限を生成しない。製品上は**well-factored C**を主要baselineとして、同じO(1)操作・失敗・API条件下で**library author＋ordinary-safe callerのL/R/P/V/C UB防止監査・再監査依存閉包**の純減を測る（Rustへの監査優位を第2関門の必須条件にはしない）。[H1 #268](https://github.com/wakairo/NewLang_Compiler/issues/268)の凍結5-root detach/adoptionは**HOLD／UNDECIDED**、自動再開しない。Product GOは**未実証**。

**低手戻りExplorationの進捗（[Coordination #570](https://github.com/wakairo/NewLang_Compiler/issues/570)）：** V #573は実ソース確認・Coordination裁定まで完了。M #571とP #572は別途ユーザー起動が必要で、起票≠実装／PASS。

| 独立Track・段階 | 最初に検証する問い／成果の上限 | 停止・次の投資判断 |
| --- | --- | --- |
| **[V #573](https://github.com/wakairo/NewLang_Compiler/issues/573#issuecomment-6234396445)：Gate 1／real source・完了** | 実libuvの元request内queue linkの既知request O(1)解除と、Redisの元listNodeポインタを保持しての直接削除をsource確認。[C裁定](https://github.com/wakairo/NewLang_Compiler/issues/573#issuecomment-6234516564)でも受理。libuvのmutex・callback／同時実行をv0が実装済みとはしない | **実C利用の存在はPASS、Rustを排除する顧客要件・ABI／metadata拒否は未実証 → Gate 1 INCONCLUSIVE／Product HOLD**。実利用受付[#470](https://github.com/wakairo/NewLang_Compiler/issues/470)は継続 |
| **[M #571](https://github.com/wakairo/NewLang_Compiler/issues/571)：source/issuer DAG／read-only** | P471のlexical field currentnessから実heap-original owner・receipt／return・protected membership・ordinary-safe APIまで、**最初の未選択／未実装段階**と必要な証明責務を分離 | 新規source／semantic判断が必要なら§4.2とCoordinationに戻す。read-only研究はsource採用ではない |
| **[P #572](https://github.com/wakairo/NewLang_Compiler/issues/572)：有限技術rung** | まず既存mainで**3 distinct lexical Nodeの相互`next/prev`を2回順に更新**できるか。SOURCE→CHECK→公開owned→C17/native／元fieldの負例、または正確な最初の停止点を確認 | 既存能力ならNO-CODE。未選択の広い機構が複数要るなら停止。**heap owner／O(1) full detach／safe API成功とは数えない** |

**順序と投資ゲート：** (A) V #573は実際のC利用例を確認したがRustに対する独占的なC合理性は未確定。Mの必要条件地図・Pの小さな既選択source忠実実装は、追加の顧客証拠の受付と**並行可能**（互いを事前承認・依存完了と見なさない）。(B) 真のheap原権限発行と可逆なnonCopy返却・refundを**その段階のsource/CHECK→公開owned→C17/native**で独立に検証。(C) protected membershipと通常safe callerの**全合法state／writer／失敗／teardown**での安全閉包、O(1) full-operation／メタデータ／ABI費用とfair Rust/C比較へ進む。**次の大きなP/M/F機構はこれらの成果とコスト／停止条件に応じCoordinationが別途判断**し、Exploration GOをProduct GOや一般安全性証明に読み替えない。

以下の2026-10-17前半以前のcheckpointは**当時の固定authority・独立R判定・価値HOLDを残した履歴**であり、上記の現在地を上書きしない。

## 履歴checkpoint — 2026-10-17（Draft17.32／PR #543採用、Array4はtest-only opt-in）

**固定authority:** Compiler/main [`1c6d9cab7349a7e501b04c9157f25f60df242e02`](https://github.com/wakairo/NewLang_Compiler/commit/1c6d9cab7349a7e501b04c9157f25f60df242e02)、[CURRENT_SPEC](reference/CURRENT_SPEC.md) → **canonical Draft 17.32**（[merged PR #533](https://github.com/wakairo/NewLang_Compiler/pull/533)）。**SPEC:** `Array<Option<u8>,4>`を4つの**直接qualified** `Option<u8>::None` leafでwhole-value構築する閉じたsource profile。**一つの直接constructorごとに元lexical Array root 1つ＋従属Option要素4つ**であり、**同一関数内の独立した直接constructor 2件も規範上選択済み**。一般`Array<T,N>`、間接leaf、indexed element/ref、nonCopy `Option<OwnedFrame>`、Ring／owner custody／ordinary-safe APIではない。

**採用済み実装と有限証拠（SPEC ≠ 通常CLI）：** [merged PR #543](https://github.com/wakairo/NewLang_Compiler/pull/543)は**テスト専用 `newlangc_p534` opt-in** の登録済み関数source→CHECK→**発行時点の公開checked-owned/validator**→通常strict C17→元Array carrierのnative観測を提供。独立[R #545 calibrated limited PASS](https://github.com/wakairo/NewLang_Compiler/issues/545#issuecomment-6227096033)は**実登録済み関数経路の2元root／8従属None要素**とGCC/Clang O0/O2実配置を検証（発行履歴は現在のlive owner権限でない）。**[R #544 fresh BLIND finding](https://github.com/wakairo/NewLang_Compiler/issues/544#issuecomment-6226699593)は保存**：未登録source-fragmentのprovisional viewは`valid_at`でfail closed、後の較正PASSで初回結果を書き換えない。**通常の `newlangc` はArray4 SOURCE default-OFF**、従って一般ユーザー向けSOURCE/C17/nativeを採用済みとは記さない。固定mainの[FULL5 #38985471659](https://github.com/wakairo/NewLang_Compiler/actions/runs/38985471659)／[NO LLVM/PARTIAL #38985471624](https://github.com/wakairo/NewLang_Compiler/actions/runs/38985471624)は同一SHA・初回SUCCESSだが、Product PASSや一般source有効化ではない。

**最初の未到達と停止：** [P #546](https://github.com/wakairo/NewLang_Compiler/issues/546#issuecomment-6227459153)のopt-in `let b=a` Array Copyや単独Option binding／returnはCHECK受理例がある一方、真正な追加Array発行record／公開証明は不完全で**C17 fail closed**。Copyなら**別destination root＋4つの新規child**が必要で、元root/Allocation/Domainは複製不可。[M #547](https://github.com/wakairo/NewLang_Compiler/issues/547#issuecomment-6229831374)によれば直接constructor以外のC/E/F source適格性は**規範未裁定**であり、Copy自体を違法と決めない。直接leafでない初期化は選択済みpositive profile外。default-ONは**HOLD**。費用の独立観測はk=240直接constructorのC17で**約289MiB peak RSS／約3.2秒（一実行）**、k=241はparser node budget拒否。一般的な性能保証・soundness findingには換算しない。

**価値・代替・投資停止：** [Product Strategy §§1.1–1.2](NewLang_Product_Value_Strategy.md)のNewLang-native低水準libraryと**library author＋ordinary-safe callerのUB予防監査・再監査依存閉包**が評価対象。[V #548](https://github.com/wakairo/NewLang_Compiler/issues/548#issuecomment-6229835479)は元アドレスを保つ二list intrusive descriptor poolを**比較workloadとして選定**し、独立standalone C17 fixtureのnative **4/4**を確認したが、libuv／NewLang／Rust／Zig poolの実行ではない。safe wrapperを持つ同一topology Rust、factor済みC/Zigは強い代替で、NewLang優位の人間負担差は**未実証**。safe Ring／intrusive library **Product HOLD**、一般Array・nonCopy/refund／H1 [#268](https://github.com/wakairo/NewLang_Compiler/issues/268) **HOLD/UNDECIDED**、真の利用事例[#470](https://github.com/wakairo/NewLang_Compiler/issues/470)は受付継続。[Coverage Map §§3.1–3.3](NewLang_C_Replacement_Coverage_Map.md)へ最初の不足を接続するが、これ自体は新機構やProduct GOの認可ではない。

**V552追加実測（2026-10-17、V548の後続・比較証拠のみ）：** [V #552最終報告](https://github.com/wakairo/NewLang_Compiler/issues/552#issuecomment-6230980170)／[C受理・Product HOLD](https://github.com/wakairo/NewLang_Compiler/issues/552#issuecomment-6231025598)は、**未統合・closed [DRAFT PR #553](https://github.com/wakairo/NewLang_Compiler/pull/553)** の固定Rust sourceをGitHub Actions ubuntu-24.04／実`rustc 1.99.0`でコンパイル・native実行。3つの独立heap Nodeの元アドレス、embedded raw `prev/next` と2つのpinned circular headを保持し、private `unsafe`＋ordinary-safe facade、nonCopy `DetachedLease` を実測：**2/2 RustテストPASS、単体native PASS、move後のlease再使用は`E0382`拒否**。**V548時点のRust未実行は当時の正しい履歴**だが、現在は実行済みの強い比較対象。library authorはlive pinned lifetime／alias／原linkの相互整合・membership writer／Drop・teardownを内部監査し続け、soundなsafe wrapperのordinary callerにprivate pointer-hop監査を要求しない。任意safe-call列の`unsafe`健全性・sanitizer／Miri・fallible `Box` OOM/rollback同等性は**未実証**（Rust実装のrecoverable `Result` parityなし）。**NewLangの同一intrusive Pool SOURCE→公開owned→C17/native／安全API／author+caller再監査費用差も未測定**。factored C/Zig・この実Rustを比較基準とし、**NO MEASURED NEWLANG DIFFERENTIAL／Product HOLD、広いintrusive機構投資NO GO**を維持。V552のRust PRはmainへ未統合で、これをProductionの一部と数えない。

以下の2026-10-16以前のcheckpointは**各時点の正しい履歴**であり、現在のDraft選択・CLI公開範囲を上書きしない。

## 履歴checkpoint — 2026-10-16（P521/PR #523統合後。当時の固定authority）

**Compiler/main `2d4317dbd8b3f1b6ddf095ebeb7ec747eb49d809`、CURRENT_SPEC→Draft17.30、FormalProof/main `5437f92d099e228773d31e2323e8ee6228ca21bf`（本機能のformal proofでない）。** [P521の凍結8正例／3負例](https://github.com/wakairo/NewLang_Compiler/issues/521#issuecomment-6212931235)は、従来のmainでCHECK0→C17 `CLI4` の境界にあった「純粋IF後のUnknown Copy `u8`→普通の一引数Copy関数→元の同じ物理Pair.leftへの単一scoped replace」を、[PR523](https://github.com/wakairo/NewLang_Compiler/pull/523)で既存のauthentic call/IF/field証拠を再利用して通常C17/nativeまで接続した。実装は`src/main.c` +75/-14（純増61C行）で、source/parser/checker/public issuer/canonical/ABI/規範変更なし。提出固定HEAD FULL5＋NO LLVM/PARTIAL初回CI成功を統合後main実行と混同しない。[R524 fresh BLIND独立](https://github.com/wakairo/NewLang_Compiler/issues/524#issuecomment-6213334576)は別のR凍結TRUE/FALSEで、元Pair旧61→左137/23・右182、元field/旧戻り値/関数実引数・返値を生存関数内の物理アドレスで観測してSAFE LIMITED。Rの内部公開checked-owned詳細ID／毒入れは**未実測**でありPの証拠と混同しない。

**Goal Alignment — §1.1 UB-prevention audit locality:** この`u8` relayでprivileged原権限は移らず、caller用opaque safe API／合法な状態変更の不変条件保護／通常caller側の監査負担削減は**一切実証していない**。factor済みC/Zig／同トポロジーRustにも安価な同等構成がある。外部実行時未知inputは[M522](https://github.com/wakairo/NewLang_Compiler/issues/522)の複数独立source/ABI＋未知数値比較issuer不足でHOLD。分岐**内部**の元Pair書換えの構造的join（PR510）、named ref helper効果（PR479/494）、H1 owner/Array/FFI/concurrency/ordinary-safe APIもHOLDのまま。**これは有限Exploration採用でありProduct GOではない。**

**次の費用判断：** さらに類似したIF/Pair/backend専用profileを連鎖させるのではなく、既存成果で作者の真のunchecked前提（権限issuer・root/Allocation/D・関連safe writer・refusal/teardown/callback）とordinary-safe callerの合法変更による再監査依存閉包が**どこで縮小するか**、無関係safe変更ではなぜ再監査不要か、実利用[#470](https://github.com/wakairo/NewLang_Compiler/issues/470)を基に同じtopologyのfactor済みC/Zig/Rustと比較する。新機構は別Issueで価値・反証・予算・停止を規定した場合のみ探索認可。既存H1の価値PASS/FAILは未決定。


## 履歴checkpoint — 2026-10-16（P514/P517/PR #515統合後）

**Compiler/main `87675e7b18976d15c80d79fe9e01ccfb4440cf81`、CURRENT_SPEC→Draft17.30、FormalProof/main `5437f92d099e228773d31e2323e8ee6228ca21bf`。** [P #514](https://github.com/wakairo/NewLang_Compiler/issues/514)→[P #517](https://github.com/wakairo/NewLang_Compiler/issues/517)→[merged PR #515](https://github.com/wakairo/NewLang_Compiler/pull/515)は、元Pairを実sourceで条件より前に作り、`usize==usize`が選んだ純粋Copy `u8`を**分岐後に一度だけ元Pair.leftへscoped replace**する物理C17/nativeの有限実装を統合。P側public-owned／mutation/OOM証拠と固定候補FULL5／NO LLVM CI、[R #518 NON-BLIND calibrated独立native](https://github.com/wakairo/NewLang_Compiler/issues/518#issuecomment-6210781475)により元R凍結TRUE/FALSEで旧37→新108/173、right211、元物理Pairのアドレス同一性を生存関数内で確認。R #516のBLIND C17 failureは旧候補の発見として記録を維持。R #518で専用公開owned数値状態dump／issuer毒入れ／破損C感度を**独立測定したとは主張しない**。成功CIは統合前候補HEADであり新mainに対する追加実行ではない。

**区別すべき不達：** [P506/PR510](https://github.com/wakairo/NewLang_Compiler/pull/510)の「IF各arm**内**で元Pairを変更した後に構造的合流」は公開join証明／native未達の別HOLD。一般effects/CFG、非Copy owner、H1/STB、named helper、[M513](https://github.com/wakairo/NewLang_Compiler/issues/513)で二機構不足と確認した本物のArray source/element boundsも未到達。作者・通常callerのaudit/re-audit dependency closure縮小は未実測で**Product GOなし**。C/Zig/同トポロジーRustでも純粋値を先に選んで後から一度書けるので、これをNewLang優位と数えない。

**次の投資裁定：** 追加のPair/IF専用backend規則、Pair構造合流証明、Array二機構を自動連鎖しない。既存技術経路を必要とする**本当のC-class元オブジェクト＋普通callerの操作**が見つかり、そのUB防止監査依存閉包・同等にfactorされたC/Zig/Rustとの比較が可能なときだけ、Process §§4.2–4.3.1の設計履歴／Goal Alignment Gateで新しい有限課題を認可する。実利用case募集[Issue #470](https://github.com/wakairo/NewLang_Compiler/issues/470)はOPEN INTAKEのみ、Trackの自動起動ではない。


## 履歴checkpoint — 2026-10-16（P508／PR #511統合後）

**Compiler/main `e4af76784a6fe4174730af0671eb58bf606aac91`、CURRENT_SPEC→Draft17.30不変、FormalProof/main `5437f92d099e228773d31e2323e8ee6228ca21bf`。** 非規範の能力・投資checkpoint。§4.1 Issue中心のTrack引継ぎを守り、Track起動はユーザーが別スレッドから行う。

- **採用済み／純粋Copyの条件付きnative選択：** [P508完了報告](https://github.com/wakairo/NewLang_Compiler/issues/508#issuecomment-6208605972)と[Coordination裁定](https://github.com/wakairo/NewLang_Compiler/issues/508#issuecomment-6208693083)、[merged PR #511](https://github.com/wakairo/NewLang_Compiler/pull/511)。凍結したTRUE/FALSE原sourceは一つの`usize == usize` boolを作り、二正常armの`u8(7)`/`u8(8)`を実C17の物理条件演算で選び、結果を次のCopy localへ運ぶ。SOURCE/CHECK→public-owned両arm→通常strict C17→GCC/Clang O0/O2 native7/8、壊したarm/condition/copy拒否、private source artifactのfork-membership等の変異拒否とOOM回復。FULL5＋NO LLVM CIは固定HEAD `a573764e5c720ffc0f513afa6df53076d1fab05c`ですべて成功（統合後mainの別CIではない）。一般IF/effects/元Pair物理Change、通常caller価値の実証ではない。
- **同時進行の元Pair探索は安全性昇格せず停止：** [P506/PR510](https://github.com/wakairo/NewLang_Compiler/pull/510) は固定未統合PR503上の実原Pair左右arm同一root構造的CHECK0とpostjoin unknown-left/right9の**Stage1候補**。共通entry／immediate post／Change lineageを独立に確認する公開validatorが**未実装のためStage2 HOLD**、C17/nativeなし。PR503/510、named helper PR479/494は依然DRAFT／DO NOT MERGE。CHECK0もCI greenも公開証明PASSではない。
- **今後の投資とGoal Alignment：** スカラーifはC/Zig/Rustでも容易でありNewLang固有のUB防止監査・再監査依存閉包の改善は未計測。次にPair専用の400–800行の公開validatorと別の物理IFを即認可するのではなく、(A)本当に必要な元C-class operation・ordinary callerの依存閉包へつなぐ最小根拠、(B)既存scoped/direct APIや局所unchecked境界、(C)fixed Array一要素Copy/bounds拒否等の独立かつ再利用可能なC replacementの技術frontier、を比較する。追加新機構はProcess §4.3の4行GateをIssueへ記録し、§4.2の新source/意味論選択がある場合は履歴照合する。元heap H1・STB・Ring・Array source/ABIの未到達、Product GO HOLDを維持。

## 履歴checkpoint — 2026-10-16（P504／PR #507統合後）

**Compiler/main `eacbd0fd84f3f9ef90813435ac71633a770f3f80`、CURRENT_SPEC→Draft17.30不変、FormalProof/main `5437f92d099e228773d31e2323e8ee6228ca21bf`。** これは非規範の最新投資checkpoint。以下の以前のmain SHA／当時の未対応判定は、各時点の正確な履歴として保持する。

- **実装済み／通常native数量比較：** [Track P #504](https://github.com/wakairo/NewLang_Compiler/issues/504#issuecomment-6206332324)／[merged PR #507](https://github.com/wakairo/NewLang_Compiler/pull/507) は既存の§5.2一つの平坦な`usize == usize`を、実source→型付きchecked/public-owned bool→通常C17の**元の実operand `size_t`による`_Bool=(lhs == rhs)`**→GCC/Clang O0/O2のTRUE/FALSE nativeへ接続。sourceのCopy/renamed localも観測し、同じobserverが壊れたC比較・Copyを拒否。P497固定元sourceは無変更、旧parser診断に依存するテスト**期待診断だけを新precision／semanticへ移行**。元のP497 literal native、OOM/型/ref等の採用済み回帰は保持。固定候補HEADのFULL5×295、NO LLVM294 CIは成功（**統合後mainを別実行した証拠ではない**）。
- **実行分岐は別段階：** 普通の`if`はCHECK時に2 armを検査できるがC17/nativeは**CLI4 `V1-BACKEND-UNSUPPORTED`**。元Pairにおいて左右の分岐が同じ元`p@left`を`replace(7/8)`する原sourceは**CHECK3 `P6-JOIN-PRECISION`**で停止。独立[M505](https://github.com/wakairo/NewLang_Compiler/issues/505#issuecomment-6206080130)が、子`left`変更と同時に親Pair current value/factも更新され、親が非flatで`arm_frame`を通らない**正確なcode-path**を確認。これは不正ソース発見でも安全なPair合流の成功でもなく、**構造を保ったUnknown左＋保全右の保守的合流・独立公開検証・一元C17物理IF**という未実装の別義務がある。
- **並列可能な新しい有限投資：** [Track P #508](https://github.com/wakairo/NewLang_Compiler/issues/508) は**元bool条件で選ぶu8 Copy値7/8の実物理IF**（元Pair書込なし）、[Track P #506](https://github.com/wakairo/NewLang_Compiler/issues/506) は**固定未統合PR503実験HEAD上で元Pairの構造的CHECK合流と独立公開検証器**（元Pair nativeなし）。両方とも**別スレッドからUSER起動が必要**で、Issue作成は着手を意味しない。別authorityの枝を無断でrebase／mergeしない。成功まで本格的な元Pair conditional nativeは未達で、一般effects/CFG/SSA/ownerを導入する許可でもない。
- **価値の境界：** 実演したのはCopyスカラ計算であり、作者＋普通callerのUB防止監査・再監査依存範囲減少を測ったわけではない。元heap H1、STB、Ring、Array source、named helper PR479/494のProduct／技術HOLDは維持。次の判断用の非直列な依存関係は[Coverage Map §§3.1–3.3](NewLang_C_Replacement_Coverage_Map.md)へ反映。

## 履歴checkpoint — 2026-10-16（PR #500統合後）

**現行authority（再確認）:** Compiler/main `0d29d61c114f179af8e7300096fd85ec485ad2e7`、CURRENT_SPEC→canonical **Draft17.30変更なし**、FormalProof/main `5437f92d099e228773d31e2323e8ee6228ca21bf`。以下は既存仕様のfaithful有限実装を区別する**非規範のcheckpoint**であり、過去のauthority SHAを指す下の記録はそれぞれの時点の履歴とする。

- **採用済み [P497／merged PR #500](https://github.com/wakairo/NewLang_Compiler/pull/500)：** §6 `usize(DIGITS)` を実source→真正checked `usize` Copy値／public view→現native-target `size_t` C17→GCC/Clang O0/O2で元local 3/9 を有限実測。大きすぎるliteralはsemantic拒否、OOM失敗後の状態rollback、異なるC target幅はemitted static assertionで拒否。**実cross-targetビルド／ABI／一般数量演算ではない**。提出固定HEAD FULL初回5構成×290／NO LLVM289 CI成功と、**別途未確認の統合後main CI**を混同しない。
- **次の未到達：** 元の凍結TRUE/FALSE `usize == usize` は**CLI3 parser `P5-EXPRESSION-UNSUPPORTED`**、comparandに対応する真正`bool`のsource/CHECK、二分岐の元Pairの物理Change/Reset・C17/nativeをまだ実証していない。独立IF emitter不足はコード調査に基づく**予測**。比較負例2件がparserで止まったことを意味論的安全拒否と誤記しない。[Track P #501](https://github.com/wakairo/NewLang_Compiler/issues/501)はUSER起動が必要な**一比較SOURCE/CHECK探索**であって、一般算術／IF nativeを一度に実装する許可ではない。
- **Product/H1/STB不変：** これがlibrary作者＋ordinary callerのUB予防監査・再監査依存閉包を実証的に縮めたとは言えない。Arrayは[M499](https://github.com/wakairo/NewLang_Compiler/issues/499#issuecomment-6205260677)で§25.2機構FITだが真の構築・要素投影のsource-first未選択を確認し、**P投資は歴史的構文選択までHOLD**。PR479/494・cJSON H1・STB・Ringの既存HOLD／DRAFT条件は不変。最新の多枝依存関係は[Coverage Map §§3.1–3.3](NewLang_C_Replacement_Coverage_Map.md)。

## 最新補遺 — 2026-10-16（下記2026-10-15 checkpointより優先）

**現行authority（確認済み）:** Compiler/main [`ad0604d2c766eac045273eba15170027f199d650`](https://github.com/wakairo/NewLang_Compiler/commit/ad0604d2c766eac045273eba15170027f199d650)、[CURRENT_SPEC](reference/CURRENT_SPEC.md) → **Draft 17.30 不変**。FormalProof/main `5437f92d099e228773d31e2323e8ee6228ca21bf`（F62/PR63の有限・条件付きLean補遺を統合済み。実compilerからの一般的な証明bridgeではない）。前回のmainからの新規採用はロードマップ／Coverage Mapの2文書を整合した[Docs PR #487](https://github.com/wakairo/NewLang_Compiler/pull/487)と、**テスト・観測専用**の[P490/PR #492](https://github.com/wakairo/NewLang_Compiler/pull/492)であり、一般のnamed-call checker／backend意味論をmainへ追加したものではない。

- **二正常分岐後の元Node読み直し（merged / 限定native PASS）：** [P #490](https://github.com/wakairo/NewLang_Compiler/issues/490)／[PR #492](https://github.com/wakairo/NewLang_Compiler/pull/492) の固定C1は、実sourceの非対称`Some(q)`跨りroot更新後、元`tail@next`を**後続のsource式でCopy read**。SOURCE/CHECK → source-issued公開owned/validator → 通常C17 → GCC/Clang O0/O2で実物理head/tailと`post_join_read=Some(original tail)`を有限実測した。一方、公開checkerの合流後`Option`とCopyは**保守的なUnknown（variant/provenance未確定）**のままであり、実行時にSomeだったことを静的権限に変換しない。
- **合流後の再match／再loan（未達 / CHECK precision）：** 同じ事前凍結C2は後続`match joined_tail` の時点で `CLI3 NODE-JOIN-POST-PRECISION`。Some(p)から元Nodeを再loanする段階、owned／C17／nativeには**到達していない**。C1のnative PASSをC2や一般再借用のPASSとして数えない。
- **名前付きNode helper（未統合の公開証明研究だけ）：** [P #493](https://github.com/wakairo/NewLang_Compiler/issues/493)の[PR #494](https://github.com/wakairo/NewLang_Compiler/pull/494)は、未統合[PR #479](https://github.com/wakairo/NewLang_Compiler/pull/479)を固定親とした、**分岐なし**実`set_link`呼出しのsource-issued entry/body/Change-Reset/old Copy/caller post公開certificateと別replay validatorの**default-OFF実験**。[独立R #495](https://github.com/wakairo/NewLang_Compiler/issues/495#issuecomment-6204729857)は初回情報隔離のもと同型別root、古いscope／occurrence、誤った旧戻り値等の公開viewを拒否できることを有限確認（**独立OOM注入は未実施**）。Rの有限SAFEは採用許可ではない。[C裁定](https://github.com/wakairo/NewLang_Compiler/issues/495#issuecomment-6204806851)により**PR #494もPR #479もDRAFT / DO NOT MERGE**。既存P478の**関数を二正常分岐内で呼ぶ**実sourceは引き続き`CHECK CLI3 P9-CONTINUATION-PRECISION`、名前付きhelperのC17/nativeも未実施。
- **CIと価値の分離：** PR #492の**統合前・固定HEAD** FULL 5構成×285／PARTIAL284成功、候補PR #494の**未統合・固定HEAD** FULL 5構成×291／PARTIAL290成功、R-only NO LLVM/GCC13・Clang成功を区別する。これらを**PR #492統合後mainの新規CI確認**や一般soundness・名付けた関数の実native成功には読み替えない。Product Strategy §1.1の**library author＋ordinary callerのUB予防監査・再監査依存閉包削減**は依然として未実証。約530行の実験的replayと5つの状態複製の費用もあり、[V #484](https://github.com/wakairo/NewLang_Compiler/issues/484)の価値HOLDを解除しない。代替は実測済みP471インラインchecked `replace` と、公平なC／Zig／同topology Rustのsafe wrapper。cJSON H1 #268、STB allocator bridge、RingのProduct GO／実装HOLDも不変。

**投資上の結論：** 技術探索の有限正例は保持するが、PR #494のProduction採用・一般effect/CFG／P9合流拡張・Rの単なる反復には進まない。[実利用者の作業事例受付 #470](https://github.com/wakairo/NewLang_Compiler/issues/470)等から、元C topologyを保持した具体操作と作者＋普通callerの監査負担の真の差を確認した場合にのみ、別Issueで§4.3/4.3.1の投資可否を判断する。本補遺は非規範的な事実・停止線の整合であり、新Trackや意味論選択を認可しない。

**能力依存地図への移行（2026-10-16）：** [C Replacement Coverage Map §3.1–3.3](NewLang_C_Replacement_Coverage_Map.md)に、source→CHECK→公開owned→C17/nativeの**最初の未到達点**、`u8→u8` Copy関数（PR404）の既統合実績、原Node（PR472/492）、未統合named call（PR479/494）、guard付きquantity・Array・byte・heap owner・ABIの**非直列**の枝を整理した。次の探索候補は[Track P #497](https://github.com/wakairo/NewLang_Compiler/issues/497)（USER LAUNCH REQUIRED）の比較一操作＋caller分岐の最初の停止点を固定する低費用gate（§5.2既存規則／§4.2 syntax履歴照合が前提）。これはRing、cJSON H1、安全なordinary APIのProduct GOや一般演算実装の認可ではない。実装する場合は別Issueでsource・negative・CI・撤退条件を凍結する。

## 履歴checkpoint — 2026-10-15（P471/PR #472統合済み、P478/PR #479当時HOLD）

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
