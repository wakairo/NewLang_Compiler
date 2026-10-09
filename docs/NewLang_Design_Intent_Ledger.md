# NewLang Design-Intent Ledger（旧案・設計意図の索引）

> Status: **non-normative historical evidence index**（旧案の保管庫・将来の再裁定メモ）
>
> **canonical authorityではない。** 現行言語規則は必ず
> [CURRENT_SPEC.md](reference/CURRENT_SPEC.md) が指す `NewLang_Compiler/main` のDraftで確認する。
>
> 必須の使い方: [NewLang_Design_Decision_Procedure.md](NewLang_Design_Decision_Procedure.md)
>
> 初回監査: [Issue #153](https://github.com/wakairo/NewLang_Compiler/issues/153);
> 再発防止: [Issue #154](https://github.com/wakairo/NewLang_Compiler/issues/154)

## 1. この台帳の読み方

この台帳が答えるのは **「かつて何を検討し、何が未解決か」** である。
現在正しいプログラムの定義や承認された言語機能はcanonical Draftが決める。

各IDは長期追跡可能な識別子とし、旧案を変更した場合も行を削除せず、
新規Issue/PR、理由、結果をlinkする。行のstatusは説明上のもの:

- **ADOPTED**: mainにmergeされた現行Draftに反映された設計判断（範囲を限定して記す）。
- **DEFERRED / OPEN**: 検討したが現在のnormative source languageでは未導入・未確定。
- **BOUNDED**: 一部の閉じたprofileにのみsource/semantic規則が採用され、一般構文は未確定。
- **SUPERSEDED**: かつての試行/採用案がreview付きで変更された。証拠は消さない。
- **EXPERIMENTAL**: 実験レポートの設計/テスト結果でありnormativeではない。
- **REJECTED (reason recorded)**: 明示的な裁定と根拠がある場合だけ使用する。

同一IDに採用済みの部分とDeferredの部分があれば、**両方を明示する**。
Draft/Issueの未merge候補は `PROPOSED (unmerged)` と表記し、ADOPTEDにしない。

## 2. 初期ledger（2026-10-08、Issue #153の監査結果）

### DI-001 — source punctuation: `.` / `::` / `@`

- **検索語:** field access, dotted, member, constructor, receiver, chaining, qualification, visibility
- **旧案:** Draft17.2ベースの非規範break-testとM0 frontend実験では
  `x.f(...)`をreceiver-call、`Foo::f(...)`をqualification、
  `x@field`をtyped representation field projectionとして分離した。
- **中間の変更:** Draft17.10では `SumType.Variant`、Draft17.20では
  `local.field` をclosed source grammarとして採用した。
  旧experimentとの差をその時点で十分に比較・記録していなかった。
- **現状:** **ADOPTED / BOUNDED**。
  [Issue #150](https://github.com/wakairo/NewLang_Compiler/issues/150) /
  [PR #151](https://github.com/wakairo/NewLang_Compiler/pull/151)のレビュー・mergeにより
  Draft17.22 §17.1のbounded Pair fieldは `local@field`、
  §26.3のclosed sum constructorは `SumType::Variant`。
  `.` は将来のreceiver/chainingのために予約しただけで、receiver-callは未導入。
- **将来の確認点:** 一般member、nested/ref/ptr field projection、
  receiver eligibility、module-qualified callsを導入するとき再照合する。
  symbolic semantic `Place.field(ProjectionId)` はsource punctuationではない。
- **出典:** 旧 `NewLang_v0_module_private_receiver_surface_breaktest.md`、
  旧 `NewLang_FrontEnd_M0_REPORT.md`（いずれもnon-normative）、
  [#153](https://github.com/wakairo/NewLang_Compiler/issues/153)、
  [#150](https://github.com/wakairo/NewLang_Compiler/issues/150)、[PR #151](https://github.com/wakairo/NewLang_Compiler/pull/151)。

### DI-002 — defining-module-private representationとtransparent escape

- **検索語:** private fields, visibility, construction, destructuring, opaque fields, intrusive, transparent, sum visibility
- **旧案:** field projection・named aggregate construction・named whole-value destructuring・
  将来field pattern・interior ref/ptr projectionは同じrepresentation visibilityに従う。
  defining module内部を第一候補とし、public/transparent representationは将来追加の余地を残す。
  sum variant visibilityは独立に扱う。
- **現状:** **DEFERRED / DIRECTION PRESERVED**。
  Draft17.22 §28.6にno-foreclosureとして記録したが、
  v0は依然one visibility domainで、module/private accessを実装していない。
- **将来の確認点:** `Self`、module/file mapping、public/transparent API、
  intrusive node / simple Point、sum variant accessとの分離。
- **出典:** 旧 `NewLang_v0_module_private_receiver_surface_breaktest.md`、
  [#153](https://github.com/wakairo/NewLang_Compiler/issues/153)、[PR #151](https://github.com/wakairo/NewLang_Compiler/pull/151)。

### DI-003 — module primary nominal type / `Self`

- **検索語:** module Foo, primary nominal, Self, declaring module, module-owned type
- **旧案:** `module Foo` はprimary nominal type `Foo`を持ち、
  そのmodule内の `Self` はprimary nominal typeを意味するという実験案。
- **現状:** **DEFERRED / OPEN**。
  Draft17.22 §28.6のmodule/import/visibilityはDeferred。
  現行sourceへ `Self`やmodule-primary-type mechanismを読み込まない。
- **将来の確認点:** 主たるnominalを持たないmoduleの可否、
  複数type、representation privacyとの関係、generic primary type。
- **出典:** 旧 `NewLang_v0_module_private_receiver_surface_breaktest.md`、
  [#153 AUDIT-LEGACY-02](https://github.com/wakairo/NewLang_Compiler/issues/153)。

### DI-004 — explicit `self` parameterとreceiver-call eligibility

- **検索語:** self, receiver, x.f(a), f(x,a), associated lookup, lexical selection, bound method
- **旧案:** 第一引数名 `self` のordinary functionをreceiver-call可能とし、
  `x.f(a)` は `f(x,a)` と同じFunctionIdへ解決できる。
  implicit borrow/derefやbound-method valueは導入せず、
  lexical/associated candidate selection後にtype compatibilityを確認し、
  mismatch時に別categoryへfallbackしないというM0実験結果。
- **現状:** **DEFERRED / OPEN**。
  Draft17.22 §21.6–§21.8のfirst-argument associated lookupはcanonicalだが、
  explicit `self`のsyntactic eligibilityや `x.f(a)` はまだnormative sourceではない。
  `.` はDI-001どおり将来用。
- **将来の確認点:** `self`という名前が必須か、extension function、
  ownership transfer vs explicit loan、selection/no-fallback、
  ordinary/associated coherence、callable values。
- **出典:** 旧 `NewLang_v0_module_private_receiver_surface_breaktest.md`、
  旧 `NewLang_FrontEnd_M0_REPORT.md`、
  [#153 AUDIT-LEGACY-02](https://github.com/wakairo/NewLang_Compiler/issues/153)。

### DI-005 — generic primary typeと `Self<T>`

- **検索語:** generic Vec, module Vec, primary generic, Self<T>, type instantiation
- **旧案:** `module Vec` / primary `Vec<T>` / `Self<T>` の関係は
  旧break-testでも**未解決問題**だった。
- **現状:** **DEFERRED / OPEN**。
  generic checking/monomorphizationのcanonical semanticsから
  module source grammarやgeneric primary-type ownershipは導かない。
- **将来の確認点:** primary typeのtype argumentsをどのscopeでbindするか、
  specialization/instance毎のtype/module identity、
  `Self<T>`の必要性と代替表記。
- **出典:** 旧 `NewLang_v0_module_private_receiver_surface_breaktest.md`、
  [#153 AUDIT-LEGACY-05](https://github.com/wakairo/NewLang_Compiler/issues/153)。

### DI-006 — lexical loan source spelling

- **検索語:** loan read, loan write, loan_read, loan_write, loan_read_ptr, exclusive, lexical block
- **旧案:** non-normative `NewLang_v0_surface_Draft1.md` には
  `loan read place as r { ... }`、`loan write place as r { ... }`、
  exclusive read/write版という4-mode統一syntaxがあった。
- **現状:** **BOUNDED / final OPEN**。
  Draft17.18/17.19の `loan_read(local){|r|...}`、
  `loan_read_ptr(ptr){|r|...}`、`loan_write(local){|w|...}` は
  targeted source profile。Draft17.22 §13.8はfinal/general loan syntaxをProvisionalに残す。
- **将来の確認点:** 一般loan surfaceを固定する際、両案を比較する。
  `loan_read`を実装した事実だけで他のloan mode/APIを不当にforecloseしない。
- **出典:** 旧 `NewLang_v0_surface_Draft1.md`、
  [#153 AUDIT-LEGACY-03](https://github.com/wakairo/NewLang_Compiler/issues/153)。

### DI-007 — ref / ptr `@field` source expansion

- **検索語:** field projection, ref<write,S>@field, ptr<S>@field, ptr dereference, place
- **旧案:** fieldのtyped source projectionとして
  `value@field`、`ref<read,S>@field`、`ref<write,S>@field`、
  `ptr<S>@field` を検証した。
- **現状:** **BOUNDED / Deferred extension**。
  Draft17.22 §17.1の実ソースはexact Pair direct lexical-local fieldのみ。
  §17.2等のprojection/liveness semanticsから
  ref/ptr baseのgeneral source grammarやimplicit dereferenceを自動導入しない。
- **将来の確認点:** access amplification禁止、root stability、
  field privacy、ptrがderef不可である点とtyped ptr projectionの区別。
- **出典:** 旧 `NewLang_v0_module_private_receiver_surface_breaktest.md`、
  [#153 AUDIT-LEGACY-04](https://github.com/wakairo/NewLang_Compiler/issues/153)。

### DI-008 — 保存された旧workload由来の追加候補

- **検索語:** storage_addr, consume_array, raw Storage, Array<T,N>, non-Discardable
- **旧案:** workload validationで
  `storage_addr`（住所の観測 != authority）と
  `consume_array`（Array全体をconsumeし非Discardable elementを
  exactly once分解、partial moveを露出しない）が候補として現れた。
- **現状:** **PRESERVED in canonical semantics**。
  Draft17.22には両方が記載されている。
  これは導入が不足しているというfindingではなく、意図継承が成立している対照例。
- **出典:** 旧 `NewLang_v0_surface_Draft1.md`、
  旧 `NewLang_v0_identity_validation_SurfaceDraft1.md`、
  [#153 AUDIT-LEGACY-07](https://github.com/wakairo/NewLang_Compiler/issues/153)。

### DI-009 — dynamically allocated single root / exact layout / failure branch (Issue #169)

- **検索語:** heap_allocate, try_allocate_one, Allocation, Storage, slot, lifetime_domain, initialize, destroy, finalize_domain, deallocate, layout, failure
- **旧候補:** non-normative `NewLang_v0_surface_Draft1.md` §6–§8, §33では
  `heap_allocate(sizeof(Record),alignof(Record))`をsuccess-onlyに書き、
  `into_slot<Record>` / explicit `lifetime_domain` / lexical ordinary and
  exclusive domain loans / `initialize` / `destroy` / `into_storage` /
  matching `heap_deallocate`をworkload-testedした。
  allocator failure/error branchや既存bounded source grammarは未確定だった。
  `NewLang_v0_surface_Draft1_1.md`も同系の候補を保持する。
- **現canonical:** Draft 17.24 §3.2は下記closed profileを固定し、
  §3.1–§3.4, §13, §14, §23.1, §26, §29の責任・lifetime規則を再利用する。
  general native allocator、general layout witness、final loan syntaxは未固定。
- **候補状態:** **ADOPTED / BOUNDED** (Issue #169 / merged PR #170、
  independent Coordination ACCEPT)。
  exactly completed bounded recursive nominal `H`について
  compiler-authorized exact `sizeof(H)`/alignで1 backingを割り当て、
  `Option<OneBacking>`の`None`/responsibility-bearing`Some`を返す
  closed source profileを選定。生存終了とdeallocationの責任は
  `Allocation`、`Storage`、`slot<H>`、`LifetimeDomain`で明示する。
- **裁定・未決定:** **KEEP** distinct backing/occupancy/domain/ptr authority、
  explicit releaseとold surfaceのseparation。
  **INTENTIONALLY REPLACE（本profileに限る）** success-only
  `heap_allocate(size,align)`表記をfailure-aware exact H operationに。
  **DEFER** general allocator interface/trait、general
  `Layout<T>` spelling、FFI/C ABI/native struct offsets、fallible init、
  multiple dynamic roots、final unified loan syntax (DI-006)。
- **Issue #183 targeted source reopening: ADOPTED / BOUNDED.**
  Draft17.25までは複数live Hのsource admissionはDeferredだった。
  canonical Draft17.26は同じcompleted nominal Hに限り、
  outer Some-arm内部のsecond `try_allocate_one<H>()`という
  **two static allocation sites / maximum two live distinct H roots**と
  nested `Option<OneBacking>` consuming matchの三経路だけを許す。
  first Noneはzero owners、second Noneはhead ownersだけを
  手動cleanup、both Someは別`R_h/R_t`・`D_h/D_t`で
  heap-head linkへheap-tailのptrを格納しtail自身のD_tでreloan後、
  両方のaffine owner/domain/full backingをexactly once解放。
  **KEEP** old experimental and current explicit
  Allocation/Storage/slot/LifetimeDomain separation、per-call failure,
  §3.1 disjoint backing、DI-010 scoped ref/field rules。
  **INTENTIONALLY REPLACE** multiple live allocated H
  **source fence only for two static calls**。
  **DEFER** third/unbounded roots、general allocator/layout、
  ownership policy、FFI/external alias、modules、unified loan syntax。
  旧experimental Surface Draft1/1.1はsuccessful explicit allocation
  sketchであり、two nested fallible Option worldsの実証ではなかった。
  旧M9チャット全文未監査。
  [PR #184 Coordination ACCEPT](https://github.com/wakairo/NewLang_Compiler/pull/184#issuecomment-6054622279)
  とmerge `1c089ba933a31d73fc16013fdaee638741f13f3f`によるsource採用。
  production checker / two-H nativeの証明とは区別する。
  [Issue #183](https://github.com/wakairo/NewLang_Compiler/issues/183)。
- **レビュー注意:** source designはcanonical採用済み。
  product North Star successやnative heap executionをこのdesign aloneで主張しない。
  full old `NewLang_M9` chat は完全照合できていない。
- **出典:** [Issue #169](https://github.com/wakairo/NewLang_Compiler/issues/169)、
  [PR #170 Coordination ACCEPT](https://github.com/wakairo/NewLang_Compiler/pull/170#issuecomment-6051813657)、
  [Issue #168](https://github.com/wakairo/NewLang_Compiler/issues/168)、
  `NewLang_v0_surface_Draft1.md`, `NewLang_v0_surface_Draft1_1.md`、
  `docs/reference/CURRENT_SPEC.md`。

### DI-010 — heap H scoped root reloan / ref-base link projection (Issue #176)

- **検索語:** ref_from_ptr write, ref<read,H>@link, ref<write,H>@link, ref-based field, read authority, scope, ProjectionId, ptr dereference
- **過去のnon-normative根拠:** `NewLang_v0_module_private_receiver_surface_breaktest.md`
  (Draft17.2ベース)は`value@field`のCopyと
  `ref<read/write,S>@field`のmode-preserving typed ref projection、
  `ptr<S>@field`のlocation-only projectionを**明示的に別操作**として提案した。
  さらに旧`NewLang_v0_surface_Draft1.md`の初期実験は
  `r.field`/`w.field`という旧punctuationだが、
  read/write mode保持とptr非derefは同じ意図だった。
- **採用前のcanonical:** Draft17.24 §10.1はproper provenance/liveness/stability
  とread/write backing permissionに基づくptr→ref core semanticsを持つが、
  §3.2 sourceではallocated Hの`ref_from_ptr(read,p,stable)`のみ。
  §17.1 coreにはmode-preserving fixed field projectionがあるが、
  actual `@link`は**direct lexical H local-only**。
- **今回の判断:** **ADOPTED / BOUNDED**。
  [PR #177独立Coordination ACCEPT](https://github.com/wakairo/NewLang_Compiler/pull/177#issuecomment-6053120459)
  とmain merge `060731a121cb29ba6cfe2559d135a4736c3dc429`により
  Draft17.25 §§10.1/17.1のclosed sourceとして採用。
  source採用はproduction/native実行実績を意味しない。
  `ref_from_ptr(write,p,stable)`でexplicit-domain-backed
  write-authorized H root refを取得し、そのscoped ref bindingから
  committed link fieldだけ`r@link`で
  `ref<read/write,Option<ptr<H>>>`へmode-preserving projection。
  bounded `read(r@link)` Copy / `replace(w@link,...) ` ordinary write。
  **read→writeは不可**。
- **§4.2 disposition:** historical ref-base typed mode-preserving sourceは**KEEP / selected limited subset**。
  general `ptr@field` location-token/sourceやimplicit deref、general
  ref-base field/loan、future module-private accessは**DEFER**。
  DI-001の`@`/`::`/`.` separationは**KEEP**。
  DI-006のfinal unified loan spellingは**DEFER**、
  DI-009のexact allocated H authority/lifecycleは**KEEP**。
- **No-foreclosure:** H/link-only root ref projectionはlater named-module visibility、
  general ref projectionやptr location projection、receiver `.`/
  final loan spelling、other nominal/generic typeを決めない。
  scoped root refのpermissionからwriteを増幅しないことは今後も不変。
- **出典:** [Issue #176](https://github.com/wakairo/NewLang_Compiler/issues/176)、
  `NewLang_v0_module_private_receiver_surface_breaktest.md`、
  `NewLang_v0_surface_Draft1.md`、[Issue #153](https://github.com/wakairo/NewLang_Compiler/issues/153)、
  Draft17.24 §10.1/§17.1、[Issue #175](https://github.com/wakairo/NewLang_Compiler/issues/175)。
  旧`NewLang_M9`全文への独立アクセスは未確認であり、網羅調査したと主張しない。

### DI-011 — ordinary non-generic known-call typed-owner applicability (Issue #194; ADOPTED / BOUNDED)

- **検索語:** ordinary fn definition time, inferred caller requirement, §13.5c, §18.1, §20.4, §24.1b, root/backing/domain, typed EndRoot, release, hidden obligation
- **旧案・evidence class:** non-normative `NewLang_v0_surface_Draft1.md` / `Draft1_1.md` §17 ordinary named/nonCopy calls and §21–22 explicit `unchecked` / `requires fn` localized human obligations; experimental `NewLang_FrontEnd_M0_REPORT.md` implemented ordinary calls but explicitly excluded Allocation/LifetimeDomain/destroy; historic Draft13/17 generic dependent-call symbolic/summary work and current Draft17.26 §13.5c are **not** direct ordinary typed-release admission. Current §24.1b infers caller `RawDefined` obligations solely for raw-byte applicability, which is not typed owner authority.
- **Gap found:** [P #193 HOLD](https://github.com/wakairo/NewLang_Compiler/issues/193#issuecomment-6058459442) and [independent Coordination HOLD](https://github.com/wakairo/NewLang_Compiler/issues/193#issuecomment-6058535724): independent nongeneric definition checker cannot assume uncorrelated formal `ptr<H>`, `Allocation`, `LifetimeDomain` originate from the same live typed H root/BackingRegion/domain. Same-typed wrong calls must reject before backend; existing §§13.5c/18.1 do not explicitly decide whether symbolic inferred typed lifecycle obligations are admissible.
- **Decision / adopted:** [Issue #194](https://github.com/wakairo/NewLang_Compiler/issues/194), canonical **Draft17.27 §18.1a**. **KEEP** independent definition-time structural/scope/nonCopy exit checking, no forged R/O/D assumptions, §13.5c known call substitution/post-state, §14.5 D identity transfer, §20.4 no hidden human-proof, DI-009/010 unique full-R explicit lifecycle and DI-001/007 mode/field distinctions. **INTENTIONALLY CLARIFY** one *closed* ordinary non-generic same-unit known direct receiver (exact `ptr<H>,Allocation,LifetimeDomain` / `unit`, no extra allocation) by recording precisely inferred relational obligations at definition and requiring compiler evidence at **every** actual caller before admitting effects/consume; incompatible or Unknown triples reject. This is conditional definition proof, **not** accepting only a favorable call or turning a signature into owner grant.
- **Why bounded:** type-only/host-granted relation would make mismatched `ptr_h,A_t,D_t`, `ptr_t,A_h,D_t`, `ptr_t,A_t,D_h` unsound. Inferred obligations are compile-time proof conditions, not human assurance. Core semantic delta 0; caller/callee H root, BackingRegion and D are preserved, full raw only recovered after EndRoot, first/second failure path retains zero/one cleanup.
- **DEFER / no-foreclosure:** general inferred typed-owner contract for arbitrary functions, `requires fn` as default, new `OwnedNode`, user-visible effects/lifetimes, generalized contracts, indirect/recursive/escaping calls, separate compilation, 3+ roots, general allocator/FFI, auto-release, owner-edge policy, modules/private and final loan syntax DI-002/006. §24.1b raw byte facts remain distinct.
- **Status:** **ADOPTED / BOUNDED**; [PR #195 independent Coordination ACCEPT](https://github.com/wakairo/NewLang_Compiler/pull/195#issuecomment-6058758846), merged as `90bfaca797e55edbcd6ecd0b26383929c25d8605`. Draft17.27 §18.1a is canonical; [#193 pre-backend P resumption](https://github.com/wakairo/NewLang_Compiler/issues/193#issuecomment-6058786761) is authorized. This status does not claim production acceptance or cross-frame native evidence. Old `NewLang_M9` conversation not exhaustively recovered.

### DI-012 — live detached H tail nonCopy RETURN carrier (Issue #203; ADOPTED / BOUNDED)

- **検索語:** live detached tail, LiveTail, ordinary nonCopy function result, typed-root/Allocation/LifetimeDomain return, source-shaped owner carrier, §18.1b, exact R/O/D identity, no hidden human proof
- **Canonical precursor (normative-current):** Draft17.27 §§3.1–3.4, 10–14, 13.5c, 14.5, 16.1/16.2, 18.1a/18.2/18.5/18.6/18.8, 20.4, 26–27 permit nonCopy authority value transfer/ordinary result, preserve governing D/value-owned R identity, require independent known-call source proof, but §18.1a's exactly-three-arg `unit` terminal callee **frees** live tail. No current closed source returns its STILL LIVE O_t/R_t/D_t/A_t from a function as a complete owner obligation. [#198 M no-foreclosure audit](https://github.com/wakairo/NewLang_Compiler/issues/198#issuecomment-6059858330) identified high C-replacement composability pressure.
- **Historical evidence (experimental/non-normative):** `NewLang_v0_surface_Draft1.md`, `NewLang_v0_surface_Draft1_1.md` present nonCopy function argument/result, nominal aggregate whole construction/destructuring, success-only heap-owning manual cleanup, localized `requires fn`/`unchecked`. `NewLang_v0_module_private_receiver_surface_breaktest.md` and `NewLang_FrontEnd_M0_REPORT.md` include ordinary/receiver syntax experiments; early M0 explicitly excluded Allocation/LifetimeDomain/destroy. DI-008 `consume_array` whole-owner decompose is an analogue, not a correlated live-root carrier. These old experiments do NOT make the new return source already valid. Complete old `NewLang_M9` thread not exhaustively audited.
- **Options adjudicated (Gate C):** **A existing nominal/return only — NOT SELECTED**, because `OneBacking` pairs Allocation with *raw Storage* and cannot represent live occupied H; no general three-field nominal declaration source or machine-proved return correlation for arbitrary triples is currently selected. Returning `ptr<H>` alone loses A/D; multi-result `let (a,b,...)` is NOT a general tuple result. **B narrowly targeted source/correlation — SELECTED AS CANDIDATE**: compiler-known monomorphic `LiveTail` triple (ptr<H>, original Allocation, original LifetimeDomain); independently conditional known ordinary producer `fn detach_and_return_tail(head_link:ref<write,Option<ptr<H>>>,p,a,d)->LiveTail` only in the same two-H one-unit closed source; every actual known direct producer call proves §18.1a O_t/R_t/D_t source-origin predicates **and** a distinct live H_h/R_h/D_h-scoped ref<write,Option<ptr<H>>> with exact current Some(p) contents; the producer itself replaces that head link with None before returning the still-live tail; construction, nonCopy return, whole destructure preserve same *existing* O/R/D and never manufacture raw or implicit free; optional D-scoped read and original §18.1a terminal consumer thereafter. **C callback/continuation/out-parameter — DEFER**, as it either fails actual by-value return or needs broader callback/placement/owner API and extra scope obligations. **D HOLD — not selected at M hypothesis level** because no concrete core invariant contradiction identified within selected strict applicability; independent Coord may BLOCK.
- **KEEP:** unique backing/nonalias (§3.1), source-origin-only Allocation and full raw after EndRoot (§3.2), root vs semantic value *placement* distinction (§18.6), same domain identity under move (§14.5), nonCopy/NonDiscardable whole aggregates (§16.1/16.2), body-sensitive caller/post-state (§13.5c/18.7/18.8), independent definition/check of all calls and §20.4 no human-proof, DI-009/010/011 and DI-001/007 explicit ptr/ref vs ownership. **INTENTIONALLY EXTEND SOURCE ONLY** by one specific H-bound `LiveTail` known nominal construction + producer-performed head-link detach + result + caller whole destructure path; preserve immutable core memory/authority laws.
- **False progress and adversarial oracle:** same-type `p_h,A_t,D_t`, `p_t,A_h,D_t`, `p_t,A_t,D_h`, stale/ended root, active D_t loan, phantom None, duplicate/drop/partial-move A/D, result with only Copy ptr, using current live Storage simultaneously, may-only/Unknown matched relation must reject. Positive requires an actual checked head-link Some(ptr_t)->None transition **inside the producer**, result independence from head loan scope, and O_t remain live *after actual return*, A_t/D_t original singleton nonCopy, later receiver performs one matched EndRoot/erase/full Storage/finalize/free, head independent, 0/1/2 failure worlds unchanged. No P/native evidence claimed from this design.
- **DEFER / no-foreclosure:** source-defined generic owner/result type, arbitrary nested owning aggregates, Vec, reference-versus-owning child policy, third/unbounded H, multi-hop forwarding, indirect/foreign call, modules/separate compilation/ABI, external aliased backing, general allocator/FFI, RAII, source-visible lifetimes/effects, concurrency, cJSON benchmark. This H-specific value package is not a prescribed permanent `Owner<T>` API and does not infer ownership from ptr/link.
- **Status:** **ADOPTED / BOUNDED** — Draft17.28 §18.1b, [PR #207 independent Coordination ACCEPT](https://github.com/wakairo/NewLang_Compiler/pull/207#issuecomment-6062271098), merged as `e6248e8389a8d9f86039f4d3cd5c7e18d1421e02`. This status records normative source adoption only; production admission/native evidence and downstream authorization remain separate (Track P source gate: Issue #208).

### DI-013 — bounded post-return durable LiveTail custody through caller Option (Issue #214; ADOPTED / BOUNDED)

- **検索語:** LiveTail, Option<LiveTail>, durable nonCopy custody, recipient_adopt, source-proven current None, nonDiscardable Option, known direct call, consuming match exception, same original O/R/D, §18.1c
- **Canonical current basis:** Draft17.28 §§3.1–3.4/13.5c/14.5/16.1–3/17.4/18.1a–b/18.2/18.5–8/20.4/26–27. Draft17.28 §18.1b proves one matched *still live* H_t's original A_t/D_t/p_t result and then a terminal receiver, but source-selects NO distinct nonterminal adopter depositing the whole original nonCopy package into caller-owned Option. `Option<LiveTail>` is statically nonCopy/nonDiscardable even at None; `store` or dropping `replace`'s old None cannot be quietly declared safe.
- **Historical Gate B evidence classes:** DI-001 (representation/operator split), DI-006 (final loan spelling deferred), DI-007/010 (mode-preserving ref field, no ptr deref/authority amplification), DI-008 (`consume_array` whole-value obligation analogue, *not* sum owner solution), DI-009 (exact two fallible H/explicit 0–2 cleanup), DI-011 (independent symbolic known call proof), DI-012 (original live-tail carrier and no general owner contract). `NewLang_v0_surface_Draft1.md` / `Draft1_1.md` experimentally sketch general nonCopy functions/whole aggregates and earlier loan/Option; `NewLang_v0_module_private_receiver_surface_breaktest.md` and `NewLang_FrontEnd_M0_REPORT.md` are experimental module/ref/receiver syntax without Allocation/LifetimeDomain/destroy. None normatively admits this custody route. Full old M9 thread not exhaustively audited.
- **Alternatives & Gate C selection:** **A existing source alone: NOT ADMITTED**, since §18.1b only allows its producer, direct caller whole destructure and terminal release, not `Option<LiveTail>::Some(packet)` placement, local `loan_write(custody)` and later Some extraction. `replace` gives back a nonDiscardable old sum; even source-provably None cannot simply be dropped or consumed by a nonexhaustive one-arm match under §26.11. **B selected PROPOSED bounded:** one ordinary monomorphic known-direct `recipient_adopt(ref<write,Option<LiveTail>>,LiveTail)->unit`, exact caller-owned ordinary local `custody` and whole Option replace/extract/consume, same original H_t A/D/provenance; *two exact statically-proven-None consuming one-arm match sites* (recipient displaced None, caller empty local), accepted only under independent conditional definition-time and EVERY actual call/current-state proof. No runtime free in adopter, no implicit drop; earlier `Some` occupancy REJECT. **C alternative exhaustive Some-release arm:** introduces unnecessary extra recipient free path/terminal call; even if caller states None it broadens behavior, violates the clean nonterminal actor experiment. **D general owner/Drop/effect/RAII/Result:** DEFER, not needed for two-H success+pre-consume refusal. **HOLD:** if exact source proof for conditional current sink None or value-owned O/R/D through sum-payload move cannot be maintained, reject rather than mint privileges.
- **KEEP:** §3 distinct R and unique A/typed root conservation; §14.5 original nonCopy D identity; §18.6 value/placement distinction and non-escaping scoped refs; §17.4 current-value Change/Reset/old-result; §26 static nonDiscardable and Some payload occurrence/new identity; §13.5c body-sensitive alias-safe caller post-state, §20.4 no human proof. **INTENTIONALLY EXTEND, source only:** this one caller local `Option<LiveTail>`, one actual recipient whose call is conditionally admitted with sink current None & original proven carrier, two exact None-only matches with source-proven unreachable Some, and one extraction/terminal route. **DEFER:** third H, 3-field H, generic owner/container, nonlocal/heap custody, arbitrary runtime fallible adopter/post-consume rollback, implicit cleanup, general None-exhaustiveness rule, new owner traits/effects, FFI/ABI, modules, concurrency, cJSON port.
- **Safety oracle:** wrong type-compatible original p/A/D; old sink Some or may-Some; duplicated/lost nonCopy A/D; `store` / dropped old nonDiscardable Option; `Some(_)` hiding original owner; omitted Some arm without proven None; stale payload occurrence or surviving scoped ref; alias/may-state invalidating None; receiver after adoption at donor; false first/second None grant; post-consume refusal returning bool. Each REJECT rather than assuming runtime owner mint, hidden destructor, C helper or noalias. Success gives producer→recipient→caller-owned Some→recipient returns→caller extracts original owner→terminal frees once; first-NULL 0, second-NULL head-only 1, both-Some exactly original tail/head 2.
- **Status:** **ADOPTED / BOUNDED** — Draft17.29 §18.1c was independently **ACCEPTED** in [PR #215 Coordination review](https://github.com/wakairo/NewLang_Compiler/pull/215#issuecomment-6071900405) and merged as Compiler main `e7db9e986bcabc2095c7e6371b4e30025a9c497f`. Current canonical `CURRENT_SPEC.md` → Draft17.29. This is **normative source selection only**, not proof of production semantic checker/native execution; no F/R/V/Q/cJSON or five-root change authorized by this entry.

## 3. 欠落・更新・accessibilityの扱い

初回indexはIssue #153の**限定的な監査**から作成したもので、
旧チャット `NewLang_M9` 全文を網羅したものではない。
旧artifactの一部はProject Library/会話にのみ残り、リポジトリにraw copyが無い。
したがってリンク切れ・資料未アクセスは根拠なし/旧案不存在を意味しない。

新たな旧案が分かったら同じIssue/PRで根拠を示してIDを追加する。
関連design decisionを行うたび、本ledgerの該当IDを
**KEEP / INTENTIONALLY REPLACE / DEFER** のどれにするかを
[設計判断手順](NewLang_Design_Decision_Procedure.md) に従って記録する。

ledgerのstatusが古い・不明な場合は`main`のcanonical Draftへ戻って照合する。
台帳の誤りを仕様変更で修復せず、まず台帳自体を訂正する。
