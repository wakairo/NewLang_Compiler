# NewLang C-Replacement Coverage Map

> Status: Track V strategic coverage ledger
>
> この文書はNewLangの **practical C-replacement envelope** を長期的に監査するためのproduct/strategy ledgerである。
> language semanticsのnormative authorityは `docs/reference/CURRENT_SPEC.md` が指すDraftであり、本書はそれを上書きしない。
> current implementation stateは変化するため、個別milestoneのexact evidenceはIssue / PR / report / Git historyを参照する。

## 現行追記 — 2026-10-16（P521/PR523統合、R524独立BLIND後）

**Compiler/main `2d4317dbd8b3f1b6ddf095ebeb7ec747eb49d809`、canonical Draft17.30、FormalProof/main `5437f92d099e228773d31e2323e8ee6228ca21bf`（この合成のformal proofではない）。** [P521](https://github.com/wakairo/NewLang_Compiler/issues/521#issuecomment-6212931235)／[merged PR523](https://github.com/wakairo/NewLang_Compiler/pull/523)で、元sourceの`usize==usize`→純粋二armの**未知Copy `u8`**→真正な同一unitの`fn echo(u8)->u8`一呼出し→**元lexical Pairの単一scoped field `replace`**→旧値・元field・sibling再読を、同一の真正source/checked-owned/C17/native経路で有限採用。sourceは既知のliteral-origin quantitiesによる**二つの異なるprogram**であり、外部から実行時に変動するguest入力・一般named function効果を意味しない。Pの公開owned実ID検査／privately mutated artifacts・C observer・OOMは実装者側の証拠。

**独立Rと到達限界：** [R524 fresh BLIND terminal](https://github.com/wakairo/NewLang_Compiler/issues/524#issuecomment-6213334576)はR独自の事前凍結source/期待値を使い、通常strict GCC/Clang C17 O0/O2・semantic負例、元Pair生存関数内GDBで同じ元root/child address、TRUE旧61→left137/right182、FALSE旧61→left23/right182、実Copy calleeへの値と戻り値を限定観測。初回R CIのGDB不在はその時点では物理観測**未実施**として保存、R専用環境修正後の2回目で観測。Rは全公開owned node/Unknown joined Copy/call formal IDを別の独立probeで確認せず、独立artifact毒入れも未実施：**Rのその層はUNMEASURED**。P521 FULL5・PARTIALのSUCCESSは固定候補HEADであり、統合後mainの別CIではない。

**Product Strategy §1.1の不変なHOLD：** 単純な`u8` scalar relayの合成は新たな権限／普通callerのsafe abstractionを作らず、**作者＋ordinary-safe callerのUB-prevention監査・再監査依存閉包の縮小はゼロ件実証／UNMEASURED**。十分factorしたC/Zig／同トポロジーRustも純粋値の関数転送と一回の元object書込を行える。[V484](https://github.com/wakairo/NewLang_Compiler/issues/484)が検討した`ref<write,Option<ptr<Node>>>` named helperのcaller poststate/二arm合流はPR479/494未統合の別件であり、P506/PR510 arm内元Pair書換え／joinもHOLD。外部runtime scalar inputは[M522](https://github.com/wakairo/NewLang_Compiler/issues/522#issuecomment-6211866675)のentry issuer＋unknown equality二独立不足でHOLD。H1/Array/owner/FFI/ordinary-safe API/Product GOは未到達。次の機構追加は[#470](https://github.com/wakairo/NewLang_Compiler/issues/470)等で実利用者の元C topology、影響するlegal writer/失敗/teardownと無関係safe変更を区別したaudit dependency closureを確認した場合のみ別途裁定。以下は旧時点の履歴。


## 現行追記 — 2026-10-16（P514/P517/PR515統合、R518独立検証後）

**採用済みCompiler/main `87675e7b18976d15c80d79fe9e01ccfb4440cf81`、canonical Draft17.30、FormalProof/main `5437f92d099e228773d31e2323e8ee6228ca21bf`（今回の機能のformal proofではない）。** [Track P #514](https://github.com/wakairo/NewLang_Compiler/issues/514#issuecomment-6209426211)／[修復P #517](https://github.com/wakairo/NewLang_Compiler/issues/517#issuecomment-6210335348)／[merged PR #515](https://github.com/wakairo/NewLang_Compiler/pull/515) により、**元lexical Pairを条件より前に生成**し、真のsource `usize==usize`の二arm純粋`u8` Copy値を選んだ後、**同じ元Pairのleftへ一度だけscoped `replace`**する有限経路がSOURCE/CHECK→既存source-issued checked-owned／P側公開観測→通常strict C17→物理nativeへ到達。Pairが最初のlocalである必要はなく、`if(a==b)`直接条件も有限対応。元C carrierのrightは保全し、scope終了と戻り旧値を区別する。

**独立性と証拠境界：** [R #516のfresh BLIND](https://github.com/wakairo/NewLang_Compiler/issues/516#issuecomment-6209803463)は旧候補のC17 `CLI4` を発見し（その時点native未到達）、[P #517](https://github.com/wakairo/NewLang_Compiler/issues/517#issuecomment-6210335348) が二つのbackend source-order/直接条件制約を限定修正。[R #518のNON-BLIND calibrated](https://github.com/wakairo/NewLang_Compiler/issues/518#issuecomment-6210781475)は**元R凍結source**でTRUE旧37→左108・右211／FALSE旧37→左173・右211の同一元Pair／同じchildアドレスを生存関数内で独立native実測し、異常arm `u8(256)`はsemantic拒否。Rは全公開owned数値IDの専用独立probeや自前issuer破損／壊したC observerを実行していないため、それらをR PASSと数えない。Pのprivate変異／OOMはP側証拠。PR515 FULL5・NO LLVM/PARTIAL CIは**固定候補HEAD**の成功であり、統合後mainの新規CIではない。

**明確なHOLD：** Pairを**各IF arm内で変更**する[P506/PR510](https://github.com/wakairo/NewLang_Compiler/pull/510)の構造的join／独立公開post validator／物理nativeは**別件の未統合HOLD**。一般IF/effects、2+原rootの複合Change、元heap B custody・H1/STB、名前付きref helper、Array indexed access、ordinary-safe libraryは今回未達。[M #513](https://github.com/wakairo/NewLang_Compiler/issues/513#issuecomment-6208891023)は本物のArray original issuerとelement+bounds projectionの独立二機構を特定し **SOURCE-SELECTION/COST HOLD**。作者＋通常callerのUB防止監査・再監査依存閉包が、よくfactorされたC/Zig／同トポロジーRustより縮小したProduct GOは**未実証**。純粋Copyを先に選んで一度書く構成はそれらの言語でも可能。以下の旧checkpointは当時の正確な履歴であり本checkpointを上書きしない。


## 現行追記 — 2026-10-16（P508/PR511統合後）

**採用済みCompiler/main `e4af76784a6fe4174730af0671eb58bf606aac91`、canonical Draft17.30、FormalProof/main `5437f92d099e228773d31e2323e8ee6228ca21bf`。** [Track P #508](https://github.com/wakairo/NewLang_Compiler/issues/508#issuecomment-6208693083)／[merged PR #511](https://github.com/wakairo/NewLang_Compiler/pull/511) で、元sourceの`usize == usize` boolから二正常armの純粋な`u8(7)`/`u8(8)` Copy値一方を選び、次のlocalへCopyする有限profileがSOURCE/CHECK→source-owned IF/arm→通常strict C17の実conditional→GCC/Clang O0/O2 native TRUE7/FALSE8まで到達。間違えたC条件・arm・Copy carrierを同じ受動的observerで検出し、private artifactのarm混入等の拒否/OOMを検証。これは**一般IF・branch effects・元Pair物理更新・source runtime input・bounds・普通callerの監査負担削減ではない**。正本はProduct Strategy §1.1、[Process §4.3.1](NewLang_Project_Development_Process.md#431-exploration-go-and-product-go)。PR #511のFULL5／NO LLVM CIは**固定提出HEADの成功**であり、統合後mainでの別実行ではない。

**分岐点と保留:** [P506/PR510](https://github.com/wakairo/NewLang_Compiler/pull/510) は固定PR503実験上の元Pair同一root二arm構造的CHECK候補(Stage1)を得たが、**共通entry／joined-postの独立公開validatorがなくStage2 HOLD**、元Pairの物理C17 IF/nativeも未到達。P501/PR503・P506/PR510はDRAFT/未統合、mainに含まれない。次の高額なPair公開証明・一般effect/CFGに直行せず、元C利用者操作と監査依存閉包の価値、既存scoped操作／局所unchecked／固定Array一要素アクセス等の代替を比較する。H1/STB/Ring、named-helper、Product GOのHOLDは維持。以下の過去日付付きcheckpoint・USER起動待ち記述は**各時点の履歴**であり、本節の到達点を上書きしない。

## 履歴追記 — 2026-10-16（P504/PR507統合後）

**現行Compiler/main `eacbd0fd84f3f9ef90813435ac71633a770f3f80`、canonical Draft17.30。** 下の`ad0604d2…`の補遺は**当時の履歴**。§3.1の能力依存地図を最新到達点の根拠とする：一つの平坦な`usize == usize`は[PR507](https://github.com/wakairo/NewLang_Compiler/pull/507)でsource/CHECK/issued bool/通常strict C17/native TRUE/FALSEまでmainへ採用済み。元Pair guarded two-normal-armは`P6-JOIN-PRECISION`、`if`のC17/native分岐はCLI4で未達。[M505](https://github.com/wakairo/NewLang_Compiler/issues/505#issuecomment-6206080130)は元Pair親current value/factの非flat合流で停止する原因と最小構造再構成案を**read-onlyで特定しただけ**。P506公開証明・P508物理Copy分岐はいずれも独立した**USER起動待ち**の探索であり成功未宣言。Product Strategy §1.1の普通caller/author監査・再監査依存閉包、cJSON H1／STB／RingのHOLDは変わらない。PR507の成功は**提出固定HEAD CI**に基づき、統合後mainの新規CIではない。

## 2026-10-16 実装証拠の補遺（strategic statusは変更しない）

現行Compiler/main `ad0604d2c766eac045273eba15170027f199d650`、canonical Draft17.30。これは下記の長期coverage分類に対する**追加の有限技術証拠と未到達境界**であり、`BEACHHEAD-VERIFIED` 等のstatus変更、製品PASS、新規機構の認可ではない。

- **既統合／原物理Node Copy read：** [P490/merged PR #492](https://github.com/wakairo/NewLang_Compiler/pull/492) の事前固定C1では、元lexical`tail.next`を非対称二正常分岐の`Some(q)`跨りroot更新後に**後続sourceから再読**し、SOURCE/CHECK・真正公開owned・strict C17/nativeで実`Some(original tail)`を限定観測。公開postの`Option`とCopyは依然として**Unknown**、検査結果がSome provenanceを発行したわけではない。同じ凍結C2の後続再match/再loanは`NODE-JOIN-POST-PRECISION`でCHECK停止（再loan未実施）。したがって既存persistent ptr／scoped ref／field mutationの限定証拠が増えただけで、一般的なsafe pointer再取得／heap owner／recursive graph製品coverageではない。
- **未統合／通常関数の公開証明候補：** [P493 DRAFT PR #494](https://github.com/wakairo/NewLang_Compiler/pull/494)は未統合の[PR #479](https://github.com/wakairo/NewLang_Compiler/pull/479)上の**分岐なし**`set_link`一body-visible呼出しでsource-issued caller⇄callee公開certificate/replayをdefault-OFF実験として確認。[R495独立有限評価](https://github.com/wakairo/NewLang_Compiler/issues/495#issuecomment-6204729857)は正しい公開viewと誤ったroot/scope/occurrence/old Copy拒否を検証したが独立OOM sweepは未実施。**mainへ採用されていない**。二正常分岐内named callは引き続き`CHECK CLI3 P9-CONTINUATION-PRECISION`、named helperのC17/nativeも到達なし。530行のreplay＋状態複製に見合うordinary caller/authorの監査依存閉包削減も未測定。[C裁定](https://github.com/wakairo/NewLang_Compiler/issues/495#issuecomment-6204806851)はPR479/494 **DRAFT / DO NOT MERGE**、Product HOLDを維持。

元のC topology・実利用操作・safe wrapperを揃えたC/Zig/Rustとの比較は[Product Strategy §1.1](NewLang_Product_Value_Strategy.md#11-ub-prevention-audit-locality-value-hypothesis)に従う。今回のPASSをallocation／nonCopy custody／一般library API／C置換の実証へ昇格しない。

## 1. 目的

North Starを狭く保ちながら、NewLangが将来Cの実用領域を置換できなくなる設計上の穴を見落とさないために使う。

このmapはroadmapでもv1 feature checklistでもない。

特に:

> **Deferred from the current slice != Rejected from the strategic envelope.**

を明示する。

## 2. Coverage status

- **BEACHHEAD-VERIFIED** — bounded actual-source product evidenceがある。
- **SEMANTIC-BASIS** — canonical semantics / production semantic machineryに基礎があるが、real workload/product coverageは未完了。
- **DEFERRED-ADDABLE** — 今はdeferするが、既存設計から後付けできるevidence / researchがある。
- **STRATEGIC-GAP** — C replacement上重要だが、十分なaddability / implementation evidenceがまだない。
- **OPEN** — 必要性または解法自体を追加調査する。
- **OUTSIDE-ENVELOPE** — C互換のためだけに存在し、NewLangの実用C replacementには原則不要。

statusはlanguage validityを意味しない。以下の行の `SPEC`／`SOURCE/CHECK`／`OWNED`／`C17/native` は**有限の技術的証拠段階**を示し、`BEACHHEAD-VERIFIED`（bounded actual-source **product** evidence）や製品成功への自動昇格ではない。一般価値仮説は[Product / Value Strategy §1.1](NewLang_Product_Value_Strategy.md#11-ub-prevention-audit-locality-value-hypothesis)を参照。

## 3. Coverage ledger

| Capability area | Current strategic status | C replacement上の意味 / 現在の見方 |
| --- | --- | --- |
| scalar / lexical value flow / function body | BEACHHEAD-VERIFIED | bounded actual-source executable scalar spineとCopy `u8→u8` direct/nested callはSOURCE/CHECK→OWNED→C17/nativeの有限実証。**名前付きNode書込関数のsum/ref署名・二正常分岐のpost-stateはmain未対応**（[P475](https://github.com/wakairo/NewLang_Compiler/issues/475)はCHECK/P8停止、未統合[P478/PR479](https://github.com/wakairo/NewLang_Compiler/pull/479)でもCHECK/P9停止）。一般numeric familyやeffect/ABIは別。 |
| nominal aggregate value | BEACHHEAD-VERIFIED | bounded aggregate construction / whole destructuringをactual sourceで検証。layout保証とは分離。 |
| persistent ptr + scoped ref | BEACHHEAD-VERIFIED | local-root gateでscoped accessを実証。さらに[PR #458](https://github.com/wakairo/NewLang_Compiler/pull/458)では**元lexical Node**のread-loan由来ptrを別の初期`Some`へ保持し、SOURCE/CHECK→OWNED→C17/nativeの元C addressを有限確認。heap locator／一般安全access APIとは別。 |
| stale ptr after EndRoot | BEACHHEAD-VERIFIED | ptr token自体は残り得るがsafe reacquisitionはrejectする境界を検証。 |
| lifetime-preserving mutation / replace | BEACHHEAD-VERIFIED | stable-root `u8` に加え、[PR #438](https://github.com/wakairo/NewLang_Compiler/pull/438)・[PR #467](https://github.com/wakairo/NewLang_Compiler/pull/467)・[PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)の元lexical `Node@next` でscoped write/old Copy・field/root currentness・sibling維持をSOURCE/CHECK→OWNED→C17/nativeで有限実証。一般CFG/heap ownerは未検証。 |
| ordinary write loan source mapping | BEACHHEAD-VERIFIED | 当初のdirect lexical `u8` profileに加え、`node@next`へのscoped write-loan＋`replace` の**元lexical Node固定field**が[PR #467](https://github.com/wakairo/NewLang_Compiler/pull/467)・[PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)でsource-issued公開owned/C17/native。名前付きcalleeへこのrefを渡す二分岐合流は未実証。 |
| field projection | SEMANTIC-BASIS | `local@field` 一般則はSPEC。実sourceでは固定`Node@next:Option<ptr<Node>>`をCOPY readし、scoped write投影と併せて[PR #467](https://github.com/wakairo/NewLang_Compiler/pull/467)・[PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)のSOURCE/CHECK→OWNED→C17/nativeを有限実測。任意のfield/aggregateのproduct coverageは未検証。 |
| field mutation | SEMANTIC-BASIS | **旧「actual-source未検証」は解消（有限）**。元lexical `Node@next` のwrite-loan+`replace`、旧`Option` Copy、`Change/Reset`と二正常分岐の公開証明、元物理NodeのC17/nativeを[PR #438](https://github.com/wakairo/NewLang_Compiler/pull/438)・[PR #467](https://github.com/wakairo/NewLang_Compiler/pull/467)・[PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)で確認。**SEMANTIC-BASISのまま**：一般field・named-call効果・heap mutation・普通caller製品証拠はない。 |
| recursive nominal / intrusive graph shape | STRATEGIC-GAP | SPECの再帰nominal宣言だけでなく、元lexical `Node` の`None/Some(ptr)`初期化・元pointer topology・matched`Some(q)`別root書込をSOURCE/CHECK→OWNED→C17/nativeで有限実測（[PR #458](https://github.com/wakairo/NewLang_Compiler/pull/458)・[PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)）。**STRATEGIC-GAPを維持**：real heap graph／非Copy custody／一般intrusive ownership・cJSON detach/adoptionは未達。 |
| sum / optional persistent link | SEMANTIC-BASIS | `Option<ptr<Node>>::None/Some`を元lexical `Node@next`に実際に構築し、Copy read、match `Some(q)` の元ptr保持、二正常分岐で別rootに作用する有限SOURCE/CHECK→公開OWNED/validator→strict C17/nativeを[PR #458](https://github.com/wakairo/NewLang_Compiler/pull/458)・[PR #467](https://github.com/wakairo/NewLang_Compiler/pull/467)・[PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)で実測。**SEMANTIC-BASIS**維持：一般graph/unsafe-free ordinary callerへ未一般化。 |
| root lifetime start/end | SEMANTIC-BASIS | root/incarnationモデルとsource検査に加え、元lexical Nodeのlive original carrier／後続scoped accessをC17/nativeで有限観測。別の5-root heap pre-detach/0–5 failure worldの実行証拠はあるが、元B＋dstの非Copy custody、end/releaseとcJSON H1全体の製品到達は未証明。 |
| Allocation / Storage / slot | SEMANTIC-BASIS | raw/typed occupancyのSPEC＋Productionテスト、固定5-root **pre-detach** ／0–5失敗世界の限定native証拠あり。Node `Some(q)` / write-joinはlexical rootsでありAllocationやLifetimeDomainを生成・検証する証拠ではない。一般user-facing source alloc/teardown・owner custodyは未実装。 |
| dynamic container raw-vacant bridge | DEFERRED-ADDABLE | runtime metadataをSSOTにするlocalized privileged bridge方針。一般化しすぎないことが重要。 |
| allocator / deallocator integration | STRATEGIC-GAP | C-class workloadへ重要。language semantics、runtime/library、source APIの境界を段階的に検証する必要。 |
| function pointer / indirect call | DEFERRED-ADDABLE | direct-call body-sensitive modelを壊さずknown-target / unknown-summaryへ拡張できるかを継続監査。 |
| captureless callback + context pointer | DEFERRED-ADDABLE | C FFIの実用最小subsetとして重要。escaping closureとは分離する。 |
| C ABI / FFI | DEFERRED-ADDABLE | C scalar / pointer-like / callback / external global等のminimal boundary候補あり。full C++ ABIやvariadicは別扱い。 |
| external backing / mmap / shared memory | STRATEGIC-GAP | C replacementには重要。safe BackingRegion nonalias invariantを壊さないimport contractが必要。 |
| MMIO / BAR / volatile | STRATEGIC-GAP | platform codeで重要。ordinary memory semanticsとvolatile/device effectを分離する必要。 |
| DMA / physical-vs-virtual alias | OPEN | safe independent backing identityとの整合が難しい。one region + views等を追加検証する。 |
| in-place / caller-allocated / out-pointer construction | DEFERRED-ADDABLE | whole-value + initializeをbaselineとし、FFI/opaque objectで必要なlocalized boundaryを後付け可能か監査。 |
| exact / packed / offset layout | OPEN | ordinary aggregate layoutはopaqueを維持。real workloadで必要ならboundary-specific mechanismを優先し、coreへ安易に入れない。 |
| freestanding / embedded execution | STRATEGIC-GAP | C replacementの重要領域。runtime assumptions、toolchain、startup、platform interfaceの実証が必要。 |
| atomics / concurrency | STRATEGIC-GAP | v0外だがC replacementでは避けにくい。memory modelとownership/lifetime interactionを将来独立設計する。 |
| separate compilation / modules / linkage | STRATEGIC-GAP | one semantic compilation unitのv0から実用scaleへ進む際の重要境界。summary/identity transportが必要。 |
| debugger / profiler / sanitizer interoperability | OPEN | language adoption上重要。semantic modelとは別にtooling coverageを追う必要。 |
| stable library / package / build ecosystem | OPEN | C replacementはlanguage featureだけでは達成できない。build/link/dependency distributionもproduct risk。 |
| C source compatibility | OUTSIDE-ENVELOPE | 目標ではない。migration toolingは将来あり得るがlanguage design authorityにはしない。 |
| C undefined-behavior compatibility | OUTSIDE-ENVELOPE | 継承しないことがNewLangの目的に含まれる。 |
| every compiler extension / preprocessor trick | OUTSIDE-ENVELOPE | practical capabilityが別の安全なmechanismで満たせれば互換は不要。 |

## 3.1 能力依存地図 — 最初の不足と再利用先（2026-10-16）

上の横断的statusは**長期的な採用可能性**の分類であり、以下は**現Productionで一つのsource/programをどこまで実測したか**という別軸である。段階は `SOURCE/CHECK → 公開OWNED → 通常C17 → 実物理NATIVE → 通常callerの価値`。左で停止した実験の右側は**NOT REACHED**であり「右側機構が一般的に不正／不存在」の証拠ではない。`CHECK3 precision`は静的安全性違反の発見と同義ではなく、`CHECK0`も公開証明／nativeを保証しない。下記の「次の最小問い」は**候補であって実装の起動許可ではない**。

| C-classの能力経路 | 固定できる現実の実測足場 | 最初に未到達／不足する境界 | 次に買う情報（成功／停止の反証） |
| --- | --- | --- | --- |
| **Copy値を返す同一unitの通常関数** | [merged PR #404](https://github.com/wakairo/NewLang_Compiler/pull/404)：元sourceの`fn echo(x:u8)->u8{x}`、異なる実引数／旧戻り値のsource-issued検査・限定strict C17/native。旧[P379](https://github.com/wakairo/NewLang_Compiler/issues/379#issuecomment-6134477587)が`Copy引数付きA1 UNKNOWN`としたのは**当時の履歴**。 | 関数を使えることからstateful ref/owner functionのcaller poststateや一般ABIは従わない。 | 数値・選択結果を本物のcallerが次の操作に使うsourceを固定する。単なる`echo(7)`繰返しは新能力に数えない。 |
| **元lexical intrusive Nodeの条件付きlink更新／継続** | [merged PR #472](https://github.com/wakairo/NewLang_Compiler/pull/472)の実`Some(q)`跨りroot書込、および[merged PR #492](https://github.com/wakairo/NewLang_Compiler/pull/492)の合流後`tail@next` Copy再読は、両方SOURCE/CHECK・公開owned・物理C17/nativeで有限実測。 | P490 C2は後続`match joined_tail`が`NODE-JOIN-POST-PRECISION`：**variant/provenanceがUnknown**のままなので`Some(p)`由来reloanは未達。 | 必要な利用者操作が**reloanそのもの**を要する場合だけ、実sourceでroot/provenanceを守る有限相関を費用・負例と比較。物理Some観測から静的Someを捏造しない。 |
| **refを受け取って元fieldを書き換えるnamed helper** | [PR #479](https://github.com/wakairo/NewLang_Compiler/pull/479)は未統合P8→P9、[PR #494](https://github.com/wakairo/NewLang_Compiler/pull/494)はその上の単一**非match** source-issued公開caller/callee証明default-OFF候補、[R #495](https://github.com/wakairo/NewLang_Compiler/issues/495)は限定SAFE（独立OOM未測定）。 | **mainはnamed Option/ref signature CHECK P8**。候補PR479でも二正常分岐中の呼出しは`P9-CONTINUATION-PRECISION`。候補PR494のnamed C17/native、ordinary safe API、author/caller純負担は未達。 | 1 bodyのための530行replay・5 world複製を、実測済みP471 inlineと比較。**監査依存閉包の減少が無い限り次の証明機構を買わない**（[V #484](https://github.com/wakairo/NewLang_Compiler/issues/484)）。 |
| **guard付きquantity／比較・分岐＋元Pair単一後続Change** | **[merged PR500](https://github.com/wakairo/NewLang_Compiler/pull/500) + [merged PR507](https://github.com/wakairo/NewLang_Compiler/pull/507) + [merged PR511](https://github.com/wakairo/NewLang_Compiler/pull/511) + [merged PR515](https://github.com/wakairo/NewLang_Compiler/pull/515) + [merged PR523](https://github.com/wakairo/NewLang_Compiler/pull/523)**：source `usize == usize`→真正bool→純粋二arm `u8` Copy→物理C17比較／選択の後に**元Pairのleftを一度だけscoped replace**し、元rightと旧値を確認する有限source/CHECK/owned/C17/nativeまで採用。独立[R518](https://github.com/wakairo/NewLang_Compiler/issues/518#issuecomment-6210781475)は原物理Pair旧37→左108/173、右211を確認。PR523で一度だけ**普通のsource-authenticated `u8→u8` Copy関数**をIFの選択結果と元Pair Changeの間に挟む合成も有限到達。[R524](https://github.com/wakairo/NewLang_Compiler/issues/524#issuecomment-6213334576)が別凍結sourceにより元Pair旧61→左137/23・右182、call/returnの生存中の物理同一性を独立native実測（公開owned全IDはR未測定）。 | **IFの各arm内で元Pair自体を変更することは別のHOLD**。[P506/PR510](https://github.com/wakairo/NewLang_Compiler/pull/510)のpostjoin公開validatorも元Pairの物理branch writeも未統合。一般effects、別root複合変更、通常caller価値は未達。独立Rは公開owned全IDダンプ／破損C／issuer毒入れ未実施。 | 次の実C-class利用者操作と作者＋通常callerのUB監査依存閉包を測る。純粋Copy先行＋一書込みはC/Zig/Rustで同様に可能なので、専用proof拡張や一般Pair memory joinは価値条件が得られるまでHOLD。 |
| **固定容量・添字付きArray／slot／Ring** | core §15の`head/len` SSOTと§25のArray/index/span/bounds／occupancyは意味論の資産。既存Pair固定fieldはsource/nativeまで到達。 | Arrayの`data[i]`等の**一般要素source producer／bounds proof**は[P379](https://github.com/wakairo/NewLang_Compiler/issues/379#issuecomment-6134477587)時点では未到達。これに比較・loop/backedge・Option occurrence・wraparoundが**別々に**必要。 | まず**一つの固定ArrayのCopy element read＋範囲外拒否**をsource/certificate/nativeで閉じられるか。成功してもRingのfull/empty・refundable owner claimは別。metadata`len/head`から権限を作らない。 |
| **有限byte列decoder／bounded scan** | `byte`/typed scalar、§24 raw observation、§25 scoped spanはcanonicalの設計基盤。現mainのC lexerは**NewLangで実装されたbyte decoderではない**。 | 実NewLang sourceのbounded byte-sequence/access/index、終端／エラー結果、合法な反復を合わせたソースが未達。[P379](https://github.com/wakairo/NewLang_Compiler/issues/379#issuecomment-6134477587)。 | 一つの入力byteから分岐結果を得る以前に、**長さ・寿命を保持した一要素アクセス**の真のsource権限とbounds否定を測る。NUL未検証raw ptrを安全なbyte spanと呼ばない。 |
| **元heap Nodeのdetach→別ownerへの返却／解放** | H1の**5元root pre-detach / 0–5 allocation failure**は実C17/native足場、元B/dst非Copy custodyは別。[H1 #268](https://github.com/wakairo/NewLang_Compiler/issues/268)。 | [M371/M372](https://github.com/wakairo/NewLang_Compiler/issues/372)：original owner receipt/refund/poststateとmembership・representation保護の**二つの独立source/encapsulation gap**。full H1 adoption 0/2、native full 0。 | 低水準`Some(ptr)` hopを増やしても原Allocation/Dは移らない。安全な普通callerの`detach` contractが成立する具体的原権限と保護境界が無い限りH1専用実装を再開しない。 |
| **allocator epoch grow／STB、FFI callback／外部backing** | stb_ds original Cの挙動とF45/M374研究、C ABI／external backing／MMIO等の長期coverage項目は存在。 | STBは**六つのrich source-to-epoch obligation**が同時に未接続。callback/FFI ABI／外部region importと動的typed lifetimeは別の未確認層。元core権限と物理addressを混同しない。 | まず独立した**一つのimport/call境界のoriginal backing・失敗・scope証明**が具体的なconsumerで必要か確かめる。STB/H1を一つの`usize ==`などの小さなsource PASSで代替しない。 |
| **freestanding・DMA/MMIO・concurrency・modules** | 長期C replacementの重要な`STRATEGIC-GAP/OPEN`。現v0は単一semantic unit・concurrency外。 | native portable substrate・外部alias／device effects／memory model／別unit summaryとoriginal owner identityの輸送は**個別の境界**で、lexical Node C17成功では埋まらない。 | userの実platform/ABI契約が与えられたときに**no-foreclosure／一つの境界**から調査。v0にgeneral effects/threadsを先行搭載しない。 |

## 3.2 非直列の依存関係 — 小さな枝で全体の再利用率を増やす

以下は**着手順の強制ではなく、未証明の橋を区別する論理地図**である。`A+B`と書いてもそれらが同一sourceで合成可能と主張しない（各合流は別証拠を要する）。

```text
S0 既にmerged: scalar Copy fn / native-target usize(DIGITS) /
              real source usize == usize -> bool -> strict C17/native /
              Pair field / scoped ref / Option match /
              original lexical Node join + post-read
  |
  +-- G: source bool + two-arm pure Copy selection C17/native [merged PR511]
  |      +-- pure Copy choice -> one later scoped original Pair.left Change
  |      |      [merged PR515; R518 finite calibrated native SAFE]
  |      +-- pure Copy choice -> authenticated u8 Copy relay -> original Pair.left Change
  |      |      [merged PR523; R524 independent BLIND finite C17/native SAFE LIMITED]
  |      +-- (independent J: original Pair changed-root structural join
  |           + public validator [P506 experimental, not merged])
  |             -> faithful original Pair physical IF [NOT REACHED]
  |      +-- numeric checked operations -> bounded count/offset/loop
  |      +-- (with indexed access) -> safe bounded byte decoder
  |
  +-- I: fixed Array source -> indexed Copy read + bounds negative
  |      +-- (with G / loop) -> search/scan
  |      +-- (with Option slot & transition contract) -> fixed Ring
  |
  +-- L: original Node ptr/ref/currentness -> safe following / reloan
  |      +-- direct checked operations [partial merged]
  |      +-- named call entry/body/post [PR494 experimental, not merged]
  |      +-- (with true A/D custody + membership encapsulation)
  |             -> owner-returning detach/adopt ordinary-safe library
  |
  +-- B: original BackingRegion import / runtime ABI / FFI
         +-- allocator epoch exchange [STB HOLD]
         +-- device memory / callback / freestanding [separate gates]
```

**S1→S2→S3→S4→S5の単線レベル上げではない。** 元のユーザー操作に不要な数値・Ring・一般ABIを前提条件にしない。S5も内部不変条件の公開を必要としない小さなbody-visible helperと、別ordinary callerに対してmembershipを守るopaque safe APIでは必要証拠が違う（[M378](https://github.com/wakairo/NewLang_Compiler/issues/378#issuecomment-6134428899)）。成功した小枝をcJSON H1やsafe Ringの得点に足さない。

## 3.3 次の投資分岐と停止規則（Coordination向け候補、非認可）

**探索投資候補Gの現時点の裁定（[merged PR515](https://github.com/wakairo/NewLang_Compiler/pull/515)後）：** 純粋Copyの二arm選択→**元Pairへの後続一回scoped Change**は同一sourceから通常C17/nativeへ有限到達し、[R518](https://github.com/wakairo/NewLang_Compiler/issues/518#issuecomment-6210781475)が元物理identityを独立に観測した。これは**arm内で各々元Pairを変更してjoined postを作る**[P506/PR510](https://github.com/wakairo/NewLang_Compiler/pull/510)の構造的join/public validatorに替わる実用可能な**純粋分岐のみの別プログラム形**であり、任意のeffectsを等価に書き換えられる証拠ではない。さらにこの形はfactor済みC/Zig/Rustにも存在し、author/caller audit localityのNewLang優位は未測定。Pair専用public proof拡張、一般CFG/effectsを次の必須工程として認可せず、実C-class操作の人間監査依存閉包と既存source APIの利用を先に優先する。

**候補I（Gと独立、現在HOLD）**：[M513のread-only事前調査](https://github.com/wakairo/NewLang_Compiler/issues/513#issuecomment-6208891023)で、本当の元固定Array constructor/rootと要素projector＋boundsという**二つの独立source/checked機構不足**を確認。一般source syntaxや原権限／物理backingを選択しないままPを起動しない。Ringやfree-listへ一足飛びに拡張しない。

**優先度を上げない案**：PR494の追加snapshot/replayを一般call-in-matchへ展開すること、P490 C2のprecisionを実行時`Some`で埋めること、元heap B ownerをCopy ptrから推定すること、長いsource未対応領域をCのhost helperだけで処理してNewLang成功と呼ぶこと。いずれも作者＋普通callerの監査依存閉包の改善を示さないか、権限／障害の真の位置を隠す。

**選定に必要な一つの価値条件**：本物のC-class操作において、入力→元物理オブジェクトの変化→普通callerの決定が追えるか、内部の安全条件を破る合法な変更を型／compilerが拒否できるか。同等にfactor済みC、Zig、同一topology Rustのsafe wrapperと比較する。**実装前にその情報がまだ無い探索は、再利用される最小のsource能力に限り費用・停止条件を先に固定する**。Product GOは別判定とする。

## 4. Coverage reviewで見ること

各capabilityについて、単にfeatureの有無ではなく次を確認する。

1. **Real workload necessity** — Cを選ぶ現実の理由になっているか。
2. **Core fit** — 現在のNewLang semanticsへ自然に足せるか。
3. **No foreclosure** — 既存decisionが追加経路を閉ざしていないか。
4. **Boundary localization** — unsafe / platform-specific要素を小さなboundaryへ閉じ込められるか。
5. **Topology preservation** — C workloadのarchitectureを保ったまま扱えるか。
6. **Toolchain reality** — compiler semanticsだけでなくABI/link/debug/build等が実用可能か。
7. **Residual C** — このcapabilityが無いと、どれだけ重要なC islandが残るか。

## 5. Prioritization rule

coverage gapがあるからといって、直ちに実装priorityが高いとは限らない。

優先順位は概ね:

```text
current North Starをblockingする
  > adjacent workloadで繰り返し現れる
  > C replacementに大きなresidual islandを残す
  > addability riskが高く早期検証が必要
  > convenience / compatibility-only
```

とする。

implementation costがAI等で低下しても、semantic interaction、diagnostic burden、compatibility burden、toolchain burdenは別に評価する。

## 6. Early researchを行う条件

current sliceへfeatureを入れなくても、次の場合はearly research / destructive testを行ってよい。

- 後付け時に基礎semantic invariantを壊す恐れがある
- source-visible lifetime/effect等の大きなmechanismを後から要求しそう
- ABI / memory model / external stateと深く結合する
- Java genericsのような恒久的互換制約を早期decisionが作り得る

FFI、function pointer、external backing、in-place construction等の事前調査はこの目的に位置づける。

## 7. North Starとの分離

`NewLang_North_Star_Product_Validation.md` のPASS/FAILを通すために、このmapの全gapを閉じる必要はない。

反対にNorth StarがPASSしても、このmapのStrategic Gapが残る限りC replacement達成とは言わない。

```text
North Star = first-value validation
Coverage Map = long-horizon adoption / replacement risk
```

## 8. 更新方針

新しいDeep Research、M/F/P/R finding、North Star experiment、adjacent workload testによりstatusが変わったときに更新する。

変更理由はGit history / linked Issueを正とし、この文書内にchange logを複製しない。

大きなcapabilityを `OUTSIDE-ENVELOPE` と判定する場合は、単なるv0 deferではなくstrategic decisionなのでCoordinationで明示的に裁定する。

## 9. 関連文書

- `NewLang_Product_Value_Strategy.md` — target ladder / practical C-replacement envelope / no-foreclosure
- `NewLang_North_Star_Product_Validation.md` — current beachhead experiment
- `NewLang_Project_Development_Process.md` — track運用
- `reference/CURRENT_SPEC.md` — canonical language semantics
