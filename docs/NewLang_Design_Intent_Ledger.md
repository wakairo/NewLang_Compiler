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
