# NewLang v0 仕様書（Draft 17.31）

> 状態: **設計検証用ドラフト**
>
> この文書は、NewLang v0 コンパイラを実装し、設計をスクラップ・アンド・ビルドするための仕様書である。
> ISO 規格相当の網羅性・厳密性・用語整備を目標としない。
>
> v0 で最も重要なのは、**実装可能な意味論の核を明示し、設計上の仮定を検証可能にすること**である。
> 表面構文は、意味論が確定している場合でも暫定でよい。



## Draft 17.31 の主変更 — PROPOSED/UNMERGED original-grant source bridge (Issue #272)

**CANDIDATE ONLY.** Add §3.2c, a bounded ordinary complete-value nominal and same-source original Allocation/typed-root/Domain proof contract; preserve Draft17.30 core, two-H LiveTail profile and exact five-root PRE-detach substrate. CURRENT_SPEC.md remains Draft17.30 until independently reviewed/merged. No source/native/product PASS.

## Draft 17.30 の主変更 — Issue #234 five-original-root three-field SOURCE substrate

**ADOPTED / CANONICAL:** independently [ACCEPTED PR #235](https://github.com/wakairo/NewLang_Compiler/pull/235) and merged on Compiler `main`. Add a closed same-H
five-static-fallible-site nested main and one H declaration with exactly
three separate fixed `next`, `prev`, `child` pointer links plus `payload:u8`,
with existing mode-preserving `ref@field` and current-value/occurrence
transitions. This source-only admission constructs the faithful fixed
cJSON *pre-detach* graph of five independent original allocated roots
and seven link facts, then explicitly releases all five originals.
No B detach/adoption, general allocator/owner API, core lifetime
change, compiler/native/Lean work or North Star PASS follows.
Only §3.2b is newly adopted as a bounded source alternative; old narrower forms remain.

## Draft 17.29 の主変更 — Issue #214 bounded two-H durable LiveTail custody

**ADOPTED / CANONICAL:** independently ACCEPTED in Compiler [PR #215](https://github.com/wakairo/NewLang_Compiler/pull/215) and merged into `main`; `CURRENT_SPEC.md` selected Draft 17.29 at that merge; the current canonical Draft is 17.30. This bounded source rule selects one further exact same-unit/same-H source path
AFTER the existing Draft17.28 §18.1b physical head-link detach and original
still-live tail LiveTail return. One separately named known-direct recipient
takes exactly that original nonCopy LiveTail by value, inserts it into a
caller-owned ordinary live Option<LiveTail> local by scoped nonexclusive write
ref, and returns unit **without ending or freeing the tail**. Later, after
recipient return and a fresh loan, the caller moves the complete exact same
original live owner value out, uses the already selected terminal receiver to
end/deallocate the tail, and separately releases the head. This adds neither
a third allocation nor an H field. No general owner/container, RAII, effect,
layout/ABI, indirection or cJSON product validation is selected.

The type Option<LiveTail> remains nonCopy **and nonDiscardable even at None**.
A bounded proof-checked *known-None consuming match* exception is therefore
necessary for two **exact** source locations. It is NOT an implicit drop or
general relaxation of §26.11 exhaustiveness. See §18.1c.

## Draft 17.28 の主変更 — Issue #203 bounded live detached tail nonCopy owner RETURN

This candidate adds exactly one closed, same-unit, known-direct source route for
**detaching head H_h.next from Some(ptr_t) to None inside the returning
function using a caller-supplied head D_h-scoped write-ref**, then returning
a STILL LIVE H tail's original Allocation(R_t) and LifetimeDomain(D_t)
alongside its Copy provenance-bearing ptr<H> via a compiler-registered nominal
LiveTail value. The producer may not end or free that H root. This selects an
independently definition-checked conditional LiveTail construction and a
source-proven **every-call** O/R/D match, plus a preserved result/whole-destructure
correlation. It does not mint a new owner, typed root, Storage, slot or domain.
A subsequent separate §18.1a receiver can release the same original H root
after return and explicit optional D-matched reloan; main independently releases
the head. Same two nested fallible allocations and their 0/1/2 original
responsibilities; no third H, new policy, RAII, general owner API or backend
authorization. This is a candidate, not normative until independent Coordination
review/merge. Exact source positive/negative cases: §18.1b.

## Draft 17.27 の主変更 — bounded ordinary known-call typed-owner applicability (Issue #194)

Draft17.27 explicitly selects a **compiler-inferred, every-known-callsite-proved
relational applicability rule** for one ordinary non-generic receiver function
that consumes an existing live two-H tail's matching `ptr<H>`,
`Allocation`, and `LifetimeDomain`, ending the tail root and releasing
the same full backing. Definition-time checking remains independent and
conditional on **recorded, unsynthesized** root/R/D entry requirements;
all actual calls must discharge them before backend. §20.4 forbids a
hidden human-proof contract. The three mismatched type-correct callers
are invalid. This is a source/semantic *admissibility clarification*
with zero changes to core value, lifetime, backing, dependency or release
laws and zero new general owner/effect mechanism. Candidate-only until
independent Coordination ACCEPT and merge. No P implementation/native
proof is claimed.

## Draft 17.26 の主変更 — bounded two allocated H source composition

Issue #183: **exactly one completed bounded recursive nominal H** remains the only
selected type. Extend Draft17.25's one-live-allocated-H **source profile** to
**exactly two static calls to the already-selected
`try_allocate_one<H>() -> Option<OneBacking>`**, with the second call inside the
first success arm of a consuming `match`. This permits two *simultaneously live
instances* `H_h` and `H_t` of the same nominal type (not two distinct H type
registrations), in independent BackingRegions with independent LifetimeDomains.
The single-call signature, semantics, fresh allocation failure contract, and
non-Copy claims do not change. The former Draft17.24 multiple-live-root
exclusion was a deliberate **source-admission fence**, reopened only for this
finite static two-call shape; neither an unsound core invariant nor a general
permission for arbitrary repeated allocation.

Three normal paths:
- first `None`: second call not evaluated; no successful grant, domain, root or
  release, normal result `unit`;
- first `Some(head_bundle)`, second `None`: first complete typed root
  `O_h` and independent claims survive the second failure; explicitly
  `destroy`, `erase_slot`, `finalize_domain`, then matching
  `deallocate` for head **inside the second-None arm**;
- both `Some`: simultaneously live disjoint root/backing pairs
  `(O_h,R_h,D_h)` and `(O_t,R_t,D_t)`, write head link
  `None -> Some(ptr_t)`, Copy/match and safely reloan tail **only under
  stability from D_t**, unlink `Some -> None`, then two independent explicit
  root/slot/domain/backing releases. Persistent `ptr_t` is neither deref
  authority nor transfer of `Allocation_t`/Storage/domain.

The typed field ProjectionId, ordinary read/write acquisition, §10.1 provenance,
§11 mode, §13 dependency / loan scopes, §17 Change, §26 Reset, §27 finite
branch identity/availability, §3.1 distinct-live-backing invariant and §14
root lifetime transitions remain **unchanged: core semantic delta = 0**.
Unknown-as-safe, matching based on coincident local numeric IDs, pointer-based
implicit dereference, scope escape, implicit release, new allocator/effects,
generic calls, general N roots/loops, third root, detach, traversal,
external/FFI backing, C ABI/layout, modules and cJSON are not authorized.

**§4.2 old-design check:** DI-009 **KEEP** exact-size native one-H allocation
result and explicit distinct owner/slot/domain split, **INTENTIONALLY REPLACE**
only the old multiple-live-root **source Deferred fence** for this two-instance
static nesting; **DEFER** general allocator, unbounded/more roots, owner policy
and arbitrary layouts. Old experimental `NewLang_v0_surface_Draft1.md` and
`NewLang_v0_surface_Draft1_1.md` used success-only
`heap_allocate(sizeof(T), alignof(T))`, explicit grants/domain/lifetime and
matching free but did not validate two fallible nested `Option<OneBacking>`
worlds. Historical evidence is nonnormative. DI-010 **KEEP** current scoped
write-root and mode-preserving link projection; DI-007/DI-001 **KEEP** typed
`@` / `::` / reserved `.`, general ref/ptr projection **DEFER**;
DI-006 unified loan spelling **DEFER**; DI-002 future private representation
**DEFER enforcement**. No retroactive claim of complete old M9 transcript
review. Update DI-009 candidate index as PROPOSED/UNMERGED.

## Draft 17.25 の主変更 — scoped heap-resident H link ref acquisition/projection

Issue #176 は、Draft 17.24のsingle-allocated-H lifecycle sourceを前提に、
**heap-resident Hのown committed recursive linkをscope-boundに読み書きする**
二つの独立した境界をclosed source profileとして選定する。

1. **root access acquisition:** `ref_from_ptr(write,p,stable)` を
   §10.1既存の「対応するwrite accessを要求するptr→ref operation」の
   *selected bounded source spelling* として追加する。
   必ずcurrent root H incarnation、pointer provenance、BackingRegionの
   read/write access permission、range/alignment/representation、
   matching explicit `ref<read,LifetimeDomain>` stabilityとscope/dependencyを検証する。
   `ptr<H>`だけではrefもwrite authorityも作れない。
   read modeの既存`ref_from_ptr(read,p,stable)`はそのまま。
2. **root refからのexact fixed-field projection:** §17.1の既存
   statically typed projectionをclosed `r@link`へ写像し、
   `r: ref<read,H>`なら`ref<read,Option<ptr<H>>>`、
   `w: ref<write,H>`なら`ref<write,Option<ptr<H>>>`を返す。
   派生refはroot ref・stable domain ref・current root incarnationへ
   scope/dependencyが伝播し、rootより長生きしない。
   `read(r@link)`はCopyな現link semantic value packageのread、
   `replace(w@link,new_link)`は§17.4既存のordinary write/Change(L)と
   §26のOption Resetに写像する。
   **read→write access amplificationを絶対に行わない**。

`@link`はonly exactly completed §16.3 bounded recursive nominal `H`の
committed `Option<ptr<H>>` linkに限る。root refのbaseは
scope-boundのordinary `ref<read/write,H>`**lexical ref binding name**であり、
persistent `ptr<H>`、arbitrary expr、call-result、nested `@field`は対象外。
`H`の`payload:u8`もこのprofileではsource-admitしない。
`@` field、`::` sum、`.` future receiverのcategoryはそのまま。
`ref<write,H>`はordinary aliasable writeであって
exclusive/unique/noalias/end-root authorityではない。

**Core semantic delta = 0**。BackingRegion、occupancy、root placement、
governing identity、loan scope、ptr liveness、read/write≠exclusive、
fixed child incarnation、value/occurrence dependency、Change/Reset、deallocation、
§3.2 allocation failure及びlifecycleを保持する。
Production/backend/test/CI workはこの候補に含めない。

### §4.2 historical design-intent adjudication

- **DI-007 KEEP / TARGETED ADOPT CANDIDATE:** 旧non-normative
  `NewLang_v0_module_private_receiver_surface_breaktest.md`では
  `ref<read,S>@field -> ref<read,Field>`、
  `ref<write,S>@field -> ref<write,Field>`と
  `ptr<S>@field -> ptr<Field>`が分かれていた。
  旧`NewLang_v0_surface_Draft1.md`は同等のmode-preserving ref projectionを
  `r.field`/`w.field`と綴っていたが、旧breaktestとcanonical Draft17.22以降の
  punctuation adjudicationでは`@`がfield。
  今回は**ref-baseのlink-only subsetだけKEEP**する。
  ptr-base projection/dereferenceやgeneral ref-base membersは**DEFER**。
- **DI-006 DEFER:** `loan read place as r`等の統一loan候補は
  引き続きfinal syntaxの候補。`loan_read(life)`という
  existing bounded spellingをこのgateだけ再利用する。
- **DI-001 KEEP:** `@` typed projection/`::` constructor/`.` receiver予約。
  `@`のbase categoryをまず検証し、field可否の失敗による
  dotted receiver/sumなどへのfallbackを認めない。
- **DI-002 KEEP / DEFER enforcement:** 将来のdefining-module-private
  representationではfield projection/constructor/destructureのvisibilityを
  一貫させる。現v0はone visibility domainのまま。
- **DI-009 KEEP:** adopted bounded fallible allocated-H pathに全責任を保持し、
  host-seeded authority、general allocatorやABIを追加しない。
- 新設するDI-010は**PROPOSED / unmerged**のみ。
  旧M9全文はアクセス確認しておらず、非規範実験とcanonicalの権威を混同しない。

## Draft 17.24 の主変更 — single allocated recursive root lifecycle source bridge

Issue #169の**限定的**なsource/API選定である。Draft17.23で既にある
`Allocation` / `Storage` / `slot<T>` / `LifetimeDomain` /
`initialize` / `destroy` / `deallocate` / `Option` /
backing identity / root incarnation / ptr provenanceとdependency semanticsを変えない。

本revisionは、§16.3でexactly completedの**単一bounded recursive nominal `H`**について、
次の1個のlifecycleをactual sourceで組み立てるためのclosed source routeを§3.2に固定する。

```text
try_allocate_one<H>() -> Option<OneBacking>
Some(OneBacking { allocation: Allocation, raw: Storage })
    -> into_slot<H>(raw)
    -> lifetime_domain()
    -> loan_read(life) { |stable| initialize(slot, H_value, stable) }
    -> live allocated root ptr<H>
    -> lexical head@link Some(ptr) / read+match / explicit-domain safe reloan
    -> lexical head@link None
    -> loan_exclusive_read(life) { |ending| destroy(ptr, ending) }
    -> erase_slot<H>(empty_slot)
    -> finalize_domain(life)
    -> deallocate(allocation, full_raw)
```

`None`はallocation失敗で、**新規`Allocation` / `Storage` /
`LifetimeDomain`やbacking claimをcallerに公開しない**。
`Some`は同じfresh nonalias BackingRegionに対応する一個のnon-Copyかつ
non-Discardable `OneBacking` payloadを所有し、
そのwhole-value destructuringで`Allocation`とfull-range`Storage`を
正しく分離する。releaseをhost helperへ隠さない。
型`H`は`sizeof(H) >= 1`で、allocatorが作るsafe full-rangeは
**exact `sizeof(H)` / `alignof(H)`**である。
compiler-authorized layout factのみを使用し、C struct layout / target ABI /
一般`Layout<T>` sourceは不要である。

本profileは**新しいaffine claim model、general allocator / platform imports、
constructor/destructor、implicit borrowing、general generic source、
new error/effect system、C ABI、automatic cleanup**を導入しない。
追加するのは既存semantic transitionsへ写像するclosed source formsと
唯一のfailure-aware platform allocation entranceだけである。
後続P実装やNorth Star/cJSON PASSの認可・主張ではない。

### §4.2 historical design-intent gate

- **DI-008 KEEP:** raw `Storage` vs allocation authority vs typed occupancyを分離する。
  旧non-normative `NewLang_v0_surface_Draft1.md`の
  `heap_allocate(sizeof(T),alignof(T))` / `into_slot<T>` /
  `lifetime_domain` / `initialize` / `destroy` / deallocateのexplicit pathを
  保持し、`heap_allocate`の**一般source name/layout selectionはDEFER**。
  このfirst gateでは失敗の`Option<OneBacking>`とclosed exact Hを明示する。
- **DI-006 KEEP/DEFER:** 旧`loan [exclusive] read ... as ...`の
  four-mode unified syntaxは依然有力な最終候補。
  現`loan_read(local){|r|...}`を維持し、
  このwitnessに必要な`loan_exclusive_read(life){|ending|...}`を
  closed **provisional** sourceとして追加するだけで、最終loan文法を決めない。
- **DI-007 DEFER:** general `ptr@field` / `ref@field`を採用しない。
  allocated tailへのreloanは§10.1の**explicit matching domain stability**のみ、
  lexical headのexisting bounded `@` linkだけを操作する。
- **DI-002 KEEP/DEFER:** future private representation/named construct/destructure
  visibility方針は維持、moduleやaccess controlを導入しない。
- **DI-001 KEEP:** `@`はbounded field、`::`はsum constructor、
  `.`はfuture receiver用である。
- **DI-009 PROPOSED:** narrow failure-aware native allocation sourceの
  先行案と比較・non-goalsをLedgerへ記録する。historical evidenceは
  current Draftを上書きしない。旧`NewLang_M9`チャット全文を
  完全再監査できたとは主張しない。

本revisionは**core semantic delta = 0**。
`try_allocate_one<H>`以外のinfallible allocatorやfailure ABI、
additional persistent `ptr` authority、general layout・FFIを導入しない。

## Draft 17.23 の主変更 — completed recursive nominal のbounded `@link` source

Issue #157は、Draft 17.22の区切り記号と型付きfixed field意味論の上で、
§16.3の**exactly completed bounded recursive nominal**をstatic typeとする
current live direct lexical localについて、**committed recursive link fieldだけ**を
one-level `@` sourceで読み書きできるようにするtargeted clarificationである。

```newlang
let link = head@next;
loan_write(head@next) { |w|
    replace(w, Option<ptr<Node>>::Some(p))
}
```

- §17.1の`local_name@field_name` / `loan_write(local_name@field_name)`という
  **既存grammarを変更せず**、bounded eligible base-type/field profileを拡張する。
  `Pair { left:u8,right:u8 }`について既存のread/loan規則は保持する。
- 追加するbaseは§16.3でfield set/type/projectionが**exactly once completion済み**の
  一個のbounded recursive nominalのcurrent live direct lexical localだけ。
  同nominalにdeclaredされたlink field（例`next`）だけが新たにsource-admissibleであり、
  exact field typeは`Option<ptr<Self>>`。payload `u8` は完成したfixed fieldではあるが
  このsource profileでは**admitしない**。
- 読み取りはcurrent linkの`Option<ptr<Self>>` semantic value packageのordinary Copy。
  `ptr` provenance・value dependencies・conditional occurrenceに関する情報は洗浄しない。
  write loanは同じlink fixed subobjectへのordinary（non-exclusive、non-unique）
  `ref<write,Option<ptr<Self>>>`を生成し、§13.8のexactly-once / scope-nonescape /
  normal-result forwarding、§13.7のimplicit local stabilityを再利用する。
- `replace(w,new_link)`は§17.4のexisting `Change(L)`であり、新しいprimitiveではない。
  enclosing Node/root・link・disjoint payload siblingのincarnationは保持し、
  linkとancestorのcurrent factだけを更新する。§26.7に従う
  `None -> Some` / `Some -> None` / `Some(old) -> Some(new)`は
  old conditional payload occurrenceを終了し（存在する場合）、
  必要ならfresh occurrenceを開始する。old packageのreturn・surviving
  dependency checkingは§13.5a/§17.4を変更しない。
- `@`はfield routeのみ、`::`は既存§26.3 sum constructor routeのみ。
  `.`は将来のreceiver/chaining用に予約したまま。
  punctuationをcategory-selectした後にeligible lexical base/type、次にfield/variantを
  判定する。unknown/disallowed `head@payload` 等をconstructor/receiverへfallbackしない。
  **旧PR #149のmember-conditioned dotted ambiguity ruleは復活させない**。
- `loan_read(tail)` / `ptr_from_ref(r)` / `loan_read_ptr(p)` は
  §10.1–§10.2 / §13.7–§13.8の既存generic `T` semantic/source ruleを再利用する。
  production frontendの旧u8-only fenceやNode executable/backend非対応を
  新しいcanonical semantic restrictionと見なさない。
  direct aggregate-root `loan_write(node)` は引き続き追加しない。
- **core semantic delta = 0**。§16.3 completion、§17.1 Pair identity、
  §17.4 `Change`、§26 conditional occurrence、§10 ptr liveness/provenance、
  §13 hidden dependency、`Copy`/`Discardable`は全て維持する。

本revisionは**source-onlyのbounded candidate**であり、
compilerの新しいNode projection実装・recursive Checked-C/native backend、
allocation/lifecycle、cJSON、general member/ptr-base/nested field、FFI/modules、
general generic frontend等を認可しない。

### §4.2 historical design-intent check

[DI-001] Draft 17.22が採用した`@` field / `::` sum / `.` reservationを
**KEEP**する。旧Issue #148/PR #149の`head.next`案は現canonicalとは非整合であり
`head@next`へ **INTENTIONALLY REPLACE** する。旧dotted candidateの
member-filter-before-category bugを修正するのではなく、category collision自体が
現`@`/`::` grammarで存在しないことを確認する。

[DI-002] 将来のdefining-module-private representation / named construction /
destructuringの一貫したvisibility方向を **KEEP / enforcement DEFER** とし、
このrevisionのone visibility domainへ新しいpublic/private ruleを追加しない。

[DI-007] ref/ptr-base `ref@field`/`ptr@field`のgeneral sourceは **DEFER**。
`@`をimplicit dereferenceやaccess amplificationに使用しない。

[DI-003/DI-004] `Self`/explicit `self` parameter/receiver-call eligibilityは
**DEFER**。ここでの`Self`は完成したbounded recursive nominal自身を指す
**semantic type identityの略記**であり、新しいsource-visible `Self`や
module-primary type syntaxではない。receiver-like `head.next`もsource-admitしない。

旧Issue #148/PR #149のsemantic discussionはreference evidenceであり、
canonical authorityはmerge済みDraft 17.22に限る。
旧チャット`NewLang_M9`全文を読んだと仮定しない。
新しい選択がいずれも関連Ledger IDの許容したbounded profileの拡張/Deferに留まり、
unmerged candidateを`ADOPTED`としないためLedger自体は今回変更しない。
Coordination ACCEPT/merge後に必要なstatus追記を別途裁定する。

## Draft 17.22 の主変更 — bounded source punctuation separation

Issue #150 は、Draft 17.20/17.21で導入したbounded fixed-field sourceと、
Draft 17.10以来のclosed nominal sum constructor sourceの **表層構文のみ** を
次のように再裁定するtargeted source-only clarificationである。

```text
local@field                              bounded fixed-field Copy read
loan_write(local@field) { |w| ... }     bounded fixed-field ordinary write loan
SumType::Variant                        qualified payloadless sum constructor
SumType::Variant(expression)            qualified unary-payload sum constructor
```

**core semantic delta = 0**。§3.8 / §10 / §11 / §13 / §14 / §17の
place/incarnation/typed projection/ordinary ref/implicit local stability/
`Change`/`Reset`/dependency/replace、および§26のsum value/conditional occurrenceは変更しない。

- `@` はfield designatorを導入するexact one-character punctuationとする。
  これまで`local.field`をadmitしていた§17.1のPair/u8-only閉じたprofileの
  spellingを`local@field`へ置き換える。base/field/type/scope restrictionは変更しない。
  `loan_write(local@field)`も同じfixed fieldへのordinary `ref<write,u8>`を生成する。
- `::` は§26.3の**closed qualified sum constructorだけ**のための
  contextual adjacent two-`:` punctuationとする。空白を挟む`: :`は同じpunctuationではない。
  `sum_type.variant`を`sum_type::variant`へ置き換える。
  candidateはexplicit sum typeから解決し、variant/arity failure後のfallbackはない。
  このrevisionはgeneral `::` module qualification文法を追加しない。
- `.` は将来のreceiver-call/chaining方向のために **reservationのみ** とする。
  `x.f(args)` のsource admission、implicit borrow/deref、bound method、
  general member-expression systemは本revisionで導入しない。
  既存のunqualified/associated first-argument lookup semanticsは変更しない。
- field `local@field` とqualified constructor `SumType::Variant` は
  source punctuationによって**構文的に別category**となるため、
  Draft 17.20の`name.member` field-versus-sum ambiguous dotted candidate
  resolutionは本closed profileから削除する。
  field failureをconstructorへ、constructor failureをfield/memberへfallbackしない。
  同名のvalue localとsum typeがあっても、そのことだけで`@`/`::`は曖昧にならない。
- Draft 17.21のbounded recursive Nodeは型登録・completionのみを追加している。
  このrevisionはその`Node`のlink-field source read/writeを**追加しない**。
  PR #149 candidateの`node.next` / `loan_write(node.next)`は今後の
  別途審査済みsource mappingにより`node@next` / `loan_write(node@next)`として
  改訂する必要がある。Node payload等のfield source admissionも増やさない。
- §28.6のmodule/import/visibilityは引き続き **Deferred**。
  将来moduleが追加されたときの設計方向として、
  field projection / named-field construction / named destructuring /
  field pattern / interior ref/ptr projectionを**defining-module-private by default**
  とするno-foreclosure principleを記録する。
  public/transparent representation escape hatchやsum variant visibilityは別途裁定する。
  現在のone visibility domainでは既存のPair/Node sourceをvisibility errorにしない。

過去revisionの`Draft 17.20 の主変更`や`Draft 17.10 の主変更`に記載される
`.` field/constructor spellingはそのrevisionの**履歴説明**であり、
Draft 17.22のlive source ruleを意味しない。
semantic-only symbolic place pathで使用する`.field(ProjectionId)`等は
source punctuationではなく、本revisionでは変更しない。

本revisionはsource-onlyの独立candidateであり、未mergeのPR #149が
別candidate `Draft 17.22` を名乗っていても、そのbranchをauthorityとせず
現canonical Draft 17.21に直接立脚する。
どちらをmainへ先に入れるか、後続candidateのrevision番号をどう再採番するかは
independent Coordination reviewで決める。

## Draft 17.21 の主変更

Draft 17.21 は Issue #142 で裁定する lexical-root recursive topology gate の前提として、
**bounded recursive nominal identity / completion rule** と、その1ケースだけのsource mappingを追加する。

Draft 17.20までの object incarnation / ptr provenance / ref stability / fixed-subobject /
sum occurrence / dependency / `Change` / `Reset` / `replace` semanticsは変更しない。
本revisionのsemantic deltaは **declaration/type-graph constructionだけ** である。

1. **bounded nominal header identityをfield resolutionより先に成立させる**
   - one semantic compilation unit内のbounded recursive aggregate declarationは、
     field typeを解決する前にstable nominal declaration identity/headerを得てよい。
   - header identityはcompletionで別identityへ置き換わらない。
   - incompleteはcompiler semantic stateであり、source-visible C-style forward/incomplete typeではない。

2. **incomplete headerを`ptr<Header>`のdirect targetとしてだけ許す**
   - first gateでincomplete nominalを参照できるselected constructorはexactly `ptr<T>`。
   - `ptr<Header>` type formationはheaderのlayout / Copy / Discardable / field setを要求しない。
   - これはptr **type** formationだけであり、ptr value / provenance / object incarnation /
     future-incarnation reservationを生成しない。
   - `ref<Header>`、`slot<Header>`、by-value field、その他constructorを
     recursion-breakingとして本revisionで一般化しない。

3. **completionをexactly onceにする**
   - all selected field typesのresolutionとcycle validation成功後にfield setを一度だけcommitする。
   - aggregate construction / destructuring / field access / full-shape property query等はcompletion前に行えない。
   - aggregateのCopy / Discardableはcompletion後にfield typesから§4.5どおり導出する。
   - unresolved header、duplicate completion、inconsistent completionはsemantic errorで、
     failed semantic-unit registrationはpartial header/completionをpublic stateへ残さない。

4. **value-containment cycleとptr indirectionを分離する**
   - aggregate fixed fieldおよびsum payloadはvalue containmentを作る。
   - `ptr<T>` target edgeはvalue containment edgeではなく、本bounded ruleのindirection barrierである。
   - incomplete headerへ戻るpathがptr barrierを一度もcrossしない場合はunbroken by-value recursionとしてrejectする。
   - 従って `Node { next: ptr<Node> }` と `Node { next: Option<ptr<Node>> }` はcandidate admissible、
     `Node { next: Node }` はrejectする。
   - mutual recursionのsource admissionはDeferredだが、将来
     `A -> B -> A` by-value cycleをrejectし、ptr-indirected cycleをadmitできるcriterionを保持する。
   - physical C layoutはcycle admissibilityのsemantic oracleではない。

5. **whole-unit header collectionをsource orderから独立にする**
   - bounded categoryはconceptually
     `collect header -> resolve selected field types -> validate cycle -> exact completion -> value/body checking`
     の順で処理してよい。
   - physical file / textual declaration orderで結果を変えない。
   - general forward declaration syntaxやgeneral declaration-order ruleは導入しない。

6. **first experimentのoptional linkはcanonical `Option<ptr<Node>>` を使う**
   - generic Optionは§26のexisting ordinary closed-sum semanticsそのものを使う。
   - first gateではexact `Option<ptr<ThisNominal>>` type source formだけをadmitし、
     general generic-type frontend / inference / declaration grammarを追加しない。
   - dedicated `MaybeNodePtr` source sumは選択しない。
     current compilerにとってconcrete sum registrationが実装しやすいことだけを理由に、
     canonical Optionと重複するpermanent source declaration surfaceを増やさない。
   - nullable-ptr semanticsを導入しない。

7. **bounded recursive aggregate source profileを1 shapeだけ追加する**
   - exact profileは2 field:
     `link_field: Option<ptr<ThisNominal>>` と `payload_field: u8`。
   - general aggregate declaration、general generic type、mutual recursive declarations、
     methods / implicit dereference / general member chains / layout annotationは固定しない。
   - Draft 17.20のPair-only `local.field` / `loan_write(local.field)` source profileは本revisionで拡張しない。
     Node field read/write source wideningは別のtargeted taskである。

allocation / raw Storage / node-per-allocation lifecycle / recursive delete / cJSON、
general recursive type machinery、general generic frontend、FFI incomplete type、separate compilationはDeferredのままである。
本revisionはrecursive Node production implementationをauthorizationしない。

## Draft 17.20 の主変更

Draft 17.20 は Issue #128 で裁定する North Star fixed-field projection/mutation gate のための
**targeted bounded fixed-field source clarification** である。

Draft 17.19までの fixed-subobject incarnation / typed projection / ordinary ref /
dependency / `Change(P)` / `replace` semanticsは変更しない。
**semantic delta = 0** である。

本revisionは、既にcanonicalなfixed-field semanticsをactual sourceへ写像するため、
registered AVS `Pair { left: u8, right: u8 }` localに限って次のclosed profileを追加する。

1. **Copy field readをone-level local field formへ固定する**
   - `local_name.field_name` をbounded fixed-field Copy readとする。
   - baseはcurrent live ordinary lexical localで、static typeはregistered AVS `Pair`に限定する。
   - fieldはそのPairにdeclaredされた `left` / `right` のいずれかで、resultはcurrent field `u8` valueのordinary Copyである。
   - nested projection、arbitrary expression base、ptr base、generic unknown-field accessは本profileに含めない。

2. **field write capabilityはdirect field loanで取得する**
   - exact formは `loan_write(local_name.field_name) { |w| ... }`。
   - `w` は対象fixed field place/incarnationへのexactly ordinary `ref<write,u8>`。
   - aggregate rootへの広いwrite refやdedicated projection capabilityをsource上で先に作らない。
   - §13.8のexactly-once scope / nonescape / normal-result forwardingをそのまま再利用する。

3. **field-vs-sum dotted spellingをboundedに分離する**
   - simple `name.member` expressionでは、left `name` がregistered AVS Pairのordinary lexical localとして成立する
     bounded field candidateと、既存§26.3のsum-type candidateを別categoryとして解決する。
   - field categoryだけが成立する場合はbounded fixed-field route、
     sum-type categoryだけが成立する場合はexisting constructor routeを選ぶ。
   - 同じleft spellingが両categoryで成立する場合は本bounded profileではambiguity errorとし、
     新しいvalue-vs-type precedenceを導入しない。
   - category選択後のunknown field / unknown variant等を他categoryへfallbackしない。
   - frontendはsemantic resolution後にfield accessをbase binding identity + opaque ProjectionId + field typeへ固定し、
     backendへsource tokenのreparseやC offset-derived authorityを要求しない。

4. **mutationは既存structural state ruleへ直接写像する**
   - field `L` への `replace(w, new)` は既存 `Change(L)`。
   - target field incarnationとenclosing root incarnationは維持する。
   - ancestor current-value factはcompositionally refreshし、known-disjoint sibling current-value fact/incarnationは維持する。
   - surviving dependency conflictは既存§13.5a/§13.5bのoverlap ruleだけで判定する。
   - exclusive/noalias/lifetime-ending authorityや新effect categoryを追加しない。

profile外のnested/general member expression、aggregate-wide write loan、ptr field source form、
recursive nominal / ptr-valued link mutation、raw allocation/lifecycle等を本revisionだけでlanguage-invalidとは決めない。
それらは引き続き将来のtargeted source workまたはDeferredである。

## Draft 17.19 の主変更

Draft 17.19 は Issue #113 で裁定する stable-root mutation North Star gate のための
**targeted bounded ordinary write-loan source clarification** である。

Draft 17.18までの `ref<write,T>` / ptr / lifetime / dependency / `replace` semanticsは変更しない。
**semantic delta = 0** である。
本revisionは、simple current lexical local rootへordinary write capabilityを得る
North-Star-only source mappingだけを追加する。

1. **direct lexical-local `u8` write loanをbounded source formとして固定する**
   - exact formは `loan_write(local_name) { |w| ... }`。
   - `local_name` はstatic typeがcore `u8` のcurrent live ordinary lexical local rootを指す
     simple local binding nameに限定する。
   - field/subobject projection、arbitrary place expression、ptr operand、aggregate root、
     other element/type familiesは本closed profileに含めない。
   - `w` はloan body内だけで有効なfresh ordinary lexical bindingで、
     static capabilityはexactly ordinary `ref<write,u8>` である。

2. **writeをexclusiveへ強めない**
   - `loan_write` はordinary mutation authorityだけを生成する。
   - `ref<write,u8>` は既存§11.2どおりCopy / aliasableであり、
     exclusive ref、unique ownership、lifetime-ending authorityを生成しない。
   - acquisitionや後続mutationの合法性は既存のscope/dependency/effect conflict ruleへ委ねる。

3. **implicit local authorityとexactly-once scopeを再利用する**
   - current local rootのimplicit governing identityとcompiler-managed stability loanを使う。
   - programmer-visible `LifetimeDomain`、Allocation / Storage / raw backingを要求しない。
   - bodyは既存loan bodyと同じexactly-once lexical blockで、
     generated refはscope-bound / non-escapingである。

4. **Draft 17.18のnormal-result forwardingをそのまま適用する**
   - body resultとsurviving current-value stateについてscope-exit compatibilityを検査する。
   - compatibleならwrite loanを終了した後、unchanged result packageをordinary outer value flowへforwardする。
   - write loan専用result category / effect mechanismを追加しない。

5. **canonical `replace` へ直接接続する**
   - `replace(w, u8(9))` は既存§17.4のordinary `ref<write,T>` parameterへそのまま適合する。
   - successful replaceは`Change(P)`を起こしcurrent value/factを更新するが、
     root incarnation / governing relation / location identityは維持する。
   - preexisting ptr provenanceはcurrent-value identityではなくplace/incarnationを指すため、
     same root incarnationがcurrentならreplace後もそのptrはpotentially safe-reacquirableである。
   - actual lifetime endは別の`EndRoot` semanticsであり、old ptrからのsafe reacquisitionは従来どおりrejectする。

profile外の`loan_write` formを本revisionだけでlanguage-invalidとは決めない。
general/final loan syntax、write loanの他型/other-place forms、parser keyword/name policy、
field projection/mutation、raw storage等は引き続きProvisional / Deferredである。

## Draft 17.18 の主変更

Draft 17.18 は Issue #108 で裁定する local-root scalar ptr/ref North Star gate のための
**targeted loan-result / bounded source clarification** である。

Draft 17.17までの ptr/ref / LifetimeDomain / implicit local lifetime authority /
semantic dependency modelは変更しない。**semantic delta = 0** である。
本revisionは、既存意味論をactual source/value flowへ一意に写像するために、
loan normal-result forwardingとbounded local-root source profileだけを追加する。

1. **loan normal-result forwardingを固定する**
   - exactly-once loan bodyのnormal resultは、通常の `SemanticValuePackage` としてbodyから得る。
   - loan scope終了時に§13.5aのscope-exit compatibilityを検査し、ending loan scopeへ依存する
     result/current-stateがboundary後へsurviveする場合はrejectする。
   - compatibleならloan scopeを終了した後、body result packageを変更せずenclosing value flowへforwardする。
     boundaryはhidden dependencyを削除・弱化・新規推測しない。
   - 従ってscoped `ref` 自体はescapeできないが、§10.2で得たpersistent `ptr<T>` は
     ending loan scopeへのblocking dependencyを持たない限りescapeできる。

2. **ordinary receivingで十分とする**
   - forwarded loan resultはordinary expression resultと同じvalue flowへ入る。
   - `let` 等の既存receiverをそのまま使用し、loan専用receiver / result category /
     general effect systemを追加しない。
   - outer continuationによるresult useはloan scope終了後にのみ開始する。

3. **North Star用のbounded read-only source profileを固定する**
   - direct local loan: `loan_read(local_name) { |r| ... }`
   - safe ref->ptr conversion: `ptr_from_ref(r)`
   - local-derived ptr reacquisition: `loan_read_ptr(ptr_name) { |r| ... }`
   - これらはIssue #108のclosed profileだけのProvisional mappingであり、
     final/general loan syntax、write-loan spelling、field projection、raw storage APIを決定しない。

4. **local-derived ptr->refは§10.1 / §13.7をそのまま使う**
   - `loan_read_ptr` はptrだけからstabilityを推論しない。
   - compilerがptr provenanceをcurrent live lexical-local incarnationと、そのimplicit governing identityへ
     対応付けられる場合だけhidden stability loanを開始し、body内に `ref<read,T>` を生成する。
   - liveness / provenance / alignment / representation / range / read access /
     occurrence dependency等の既存§10.1 obligationをすべて満たす必要がある。
   - local incarnation終了後のreacquisitionはrejectし、生成refがliveな間のconflicting local endもrejectする。

5. **profile外をlanguage-invalidとは決めない**
   - richer operand syntax、write variant、same-spelling ordinary declarationとの一般的coexistence、
     final parser keyword policy等は本revisionでは固定しない。
   - production gateがこのclosed profileを越える必要がある場合はCoordinationへ戻す。

## Draft 17.17 の主変更

Draft 17.17 は、M9.14で閉じる exact minimal `loop` / `continue` / `break` source profile の
**targeted source/control-context revision** である。

Draft 17.16のHYBRID cyclic-state semantics、post-fixpoint relation、affine responsibility、
dependency/current-value recurrence、finite break join、zero-break semanticsは変更しない。
本revisionは、それらへexact source grammarとlexical control-target ruleを与える。

1. **exact loop expression grammarを固定する**
   - exact formは `loop '(' [loop_parameter_list] ')' lexical_block`。
   - zero parameterは `loop () { ... }`。
   - `loop { ... }` shorthandは本closed profileに含めない。
   - parameterは `parameter_name '=' expression`。
   - explicit type annotationを追加せず、initializerのnormal result static typeをparameter static typeとする。
   - parameter listのtrailing commaは許可しない。

2. **initializer order / scopeを固定する**
   - initializerはleft-to-rightに評価する。
   - 全initializerはloop parameter導入前のouter pre-loop lexical environmentでlookupする。
   - earlier parameter spellingはlater initializerへ自動的にscope導入されない。
   - 全initializerのnormal transferが成立した後、first iterationのfresh parameter bindingsを一括して導入する。

3. **continue / breakをdedicated terminating block itemとして固定する**
   - exact continueは `continue '(' [argument_list] ')' ';'`。
   - zero carried valueは `continue();`。
   - trailing commaは許可しない。
   - exact breakは `break expression ';'`。
   - bare `break;` は含めず、unit resultは `break unit;`。
   - continue / breakはordinary expression / tail expressionではない。

4. **nearest active loop targetを固定する**
   - ordinary nested lexical block / if arm / match armはcurrent loop-control targetを継承する。
   - nested loopはinner targetをnew nearest targetとしてshadowする。
   - callable block / loan body boundaryはouter loop targetを継承しない。
   - labels / multi-level break/continueは追加しない。

5. **`loop` / `continue` / `break` をordinary lexical structural-reserved setへ追加する**
   - lexer-wide dedicated keyword tokenizationは要求しない。
   - field / variant / member labelを一律予約しない。
   - malformed structural syntaxをsame-spelling ordinary name/callへfallbackしない。

## Draft 17.16 の主変更

Draft 17.16 は、M9.13で閉じる loop header の
**hybrid cyclic-state semantic contract revision** である。

Draft 17.15までのexact loop-carried transfer / scope-exit / outer non-Copy availability ruleを維持しつつ、
hidden dependency、changing semantic-package identity、outer Copy/current-value stateが
backedgeを通って再帰する場合のnormative semantic boundaryを固定する。

本revisionはexact `loop` / `continue` / `break` source grammarを固定しない。
current structural reserved setも変更しない。

1. **reference semanticsとchecker abstractionを分離する**
   - language reference semanticsは、initial entryから有限回のreachable `continue` edgeを経て到達し得るconcrete loop-header states / executionsである。
   - compilerはそれらをsoundly subsumeするabstract header state `H` を使ってよい。
   - `H` はentry stateを含み、`H`から到達し得るすべてのreachable continue successorを再び含むinductive post-fixpointでなければならない。
   - exact least-fixpoint algorithm自体はlanguage requirementではない。

2. **HYBRID state decompositionを固定する**
   - parameter count/type、per-iteration fresh binding、affine responsibility conservation、
     iteration-scope exit compatibility、captured outer non-Copy availabilityはexact invariant。
   - carried package identity abstraction、hidden dependency、outer Copy/current-value state、
     dependency-bearing current-state alternativesはcyclic abstract componentになり得る。

3. **symbolic affine packageを認める**
   - fresh iteration binding identityとdynamic semantic-package identityを同一視しない。
   - unchanged carryではsame package responsibilityをtransferしてよい。
   - transformed carryではold packageをconsumeしfresh same-type packageをnext iterationへ渡してよい。
   - abstract headerは各concrete stateでexactly one current affine responsibilityが存在することを守り、
     static type equalityだけからidentity equalityを仮定しない。

4. **continue / break / return edge classを分離する**
   - continueだけがheader recurrenceへ戻る。
   - breakはcyclic headerから導かれるfinite normal loop-exit joinへ流れる。
   - returnはloop header / break joinへ入らずenclosing function exitへ流れる。
   - iteration-local dependency escapeは各edgeでjoin前にrejectする。

5. **zero-break loopを明示的に許す**
   - statically reachable break edgeが0本でもloop自体をstatic language errorにしない。
   - normal loop outgoing edge / normal loop resultは0本。
   - bottom / never / synthetic `unit` resultを導入しない。
   - reachable return edgeは通常どおりfunction exitへ流れ、残りのexecutionはdivergeし得る。

6. **sound implementation freedomを固定する**
   - finite may-set、`Unknown`、widening、monotone finite abstract domain、inductive post-fixpoint、
     conservative analysis-precision rejectionを許す。
   - reachable dependency/stateのunder-approximation、dependency erasure、affine responsibility duplication/loss、
     first-iteration-only shortcutは禁止。
   - `Unknown` / wideningは安全性blockerを消してunsafe programをacceptする意味で使ってはならない。

## Draft 17.15 の主変更

Draft 17.15 は、M9.11で閉じる exact minimal value-producing `if` source profile の
**targeted finite-control-flow source revision** である。

Draft 17.14までの ownership / lifetime / dependency / normal-join / return semantics は変更しない。
本revisionは§27.3 / §27.4の既存finite branch/join semanticsへexact source surfaceを与え、
`if` / `else` のordinary lexical name policyを追加する。

1. **exact value-producing `if` grammarを固定する**
   - exact formは `if (expression) lexical_block else lexical_block`。
   - condition parenthesesはこのconstruct固有のdelimiterであり、general grouping expressionを導入しない。
   - conditionはexactly once評価し、static type `bool` を要求する。
   - exactly one armを評価する。
   - arm bodyは既存§19.1 lexical blockそのもの。

2. **`else`をmandatoryとする**
   - no-`else` formは本closed profileに含めない。
   - implicit false-path `unit` armやstatement-only `if` categoryを導入しない。
   - effect-only conditionalもexplicit two-arm expressionとして書く。

3. **direct `else if` sugarを固定しない**
   - exact grammarでは `else` の直後はlexical block。
   - chained conditionはelse-arm lexical blockのtailにnested `if` expressionを書く。
   - future sugarを禁止する決定ではない。

4. **`if` / `else` をcurrent structural reserved setへ追加する**
   - ordinary lexical namespaceへexact spelling `if` / `else` を導入できない。
   - lexer-wide keyword tokenizationやfield / variant / member labelのglobal reservationは要求しない。
   - malformed structural sourceをsame-spelling ordinary lookupへfallbackしない。
   - `loop` / `break` / `continue` は本revisionで予約しない。

5. **既存join / termination semanticsをそのままsourceへ接続する**
   - normal armsは§27.3 / §27.4どおりexact result type / availability / dependency / memory-state joinを行う。
   - terminating armはnormal joinへ参加しない。
   - normal outgoing edgeが0本でもbottom / never / synthetic `unit`を作らない。
   - `if`専用ownership state machineやimplicit conversionを追加しない。

## Draft 17.14 の主変更

Draft 17.14 は、R6-01 / M9.9で確認された structural source word と ordinary lexical name の
source/name-admissibility gapを局所的に閉じる
**targeted ordinary-name reservation revision** である。

Draft 17.13までの ownership / lifetime / dependency / call / return / whole-unit visibility semantics は変更しない。
本revisionは、current closed source profileの骨格を作る
`fn` / `let` / `return` / `match` と ordinary lexical namespace の関係だけを固定する。

1. **small structural setをordinary lexical namespaceから予約する**
   - current reserved structural spellingsはexactly `fn` / `let` / `return` / `match`。
   - ordinary function / parameter / local binding等、ordinary lexical namespaceへこれらのexact spellingを導入してはならない。
   - このsetはcurrent closed profileのtargeted setであり、future keyword inventoryを先取りしない。

2. **lexer-wide keyword redesignは要求しない**
   - lexerはこれらを他のwordと同じtoken classで表現してよい。
   - restrictionはordinary lexical source-name admissibilityで適用してよい。
   - parser implementationへdedicated keyword token kindを要求しない。

3. **member/nominal namespaceまで自動的に予約しない**
   - field / variant / member label等、ordinary lexical bindingを導入しない別namespaceのspellingは本revisionだけで一律禁止しない。
   - ただしmember/payload spellingをfresh ordinary local bindingへ受けるsource formでは、そのlocal binding nameに本structural setを使えない。

4. **structural source interpretationを安定化する**
   - `fn` / `let` / `return` / `match` をordinary call/nameとして温存するための
     distinguishing-shape / fallback規則は持たない。
   - current grammarでこれらがstructural introducerとして現れる位置では、そのstructural meaningを持つ。
   - malformed structural syntaxをordinary-name parseへfallbackして別programとして救済しない。

5. **`unit` は別のdistinguished-core caseとして維持する**
   - `unit` もordinary lexical namespaceへ導入できないが、理由はstructural source wordだからではない。
   - §4.9どおりcore singleton type/valueを直接表すdistinguished spellingである。
   - structural reserved setと `unit` はlexer token classを共有してよいが、semantic categoryを混同しない。

## Draft 17.13 の主変更

Draft 17.13 は、M9.8で選択した ordinary non-generic function declaration source gap を局所的に閉じる
**targeted declaration/source-name-resolution revision** である。

Draft 17.12までの ownership / lifetime / dependency / call / return / function-exit semantics は変更しない。
本revisionは、P8/P9/P10でhost registrationから与えていたordinary function identity/signature/bodyを、
sourceのtop-level declarationから与えるための最小profileだけを固定する。

1. **minimal non-generic ordinary `fn` declarationを固定する**
   - exact formは `fn name(parameters) -> ResultType lexical_block`。
   - zero parameterは `()`。
   - parameterは `name : Type`、複数parameterはcomma区切り。
   - trailing commaは本closed profileでは持たない。
   - result typeはunitを含め常にexplicitで、省略result shorthandは持たない。
   - `->` は隣接した `-` と `>` のcontextual two-character punctuatorで、`- >` は不受理。
   - bodyは既存§19.1 lexical blockそのもので、body後のsemicolonは持たない。

2. **top-level ordinary declarationとしてのみ導入する**
   - ordinary function declarationはone semantic compilation unitのtop-level ordinary declarationである。
   - lexical block itemとしてnested/local functionを導入しない。
   - general-purposeな全declaration category用 `compilation_unit := item*` grammarは本revisionでは固定しない。

3. **ordinary function signaturesをsemantic unit内でorder-independentにvisibleとする**
   - bounded ordinary function declarationsのname/signatureはbody checkingより前にsemanticにavailableである。
   - forward callはresolveする。
   - physical file / textual declaration order / host input orderでvisibilityを変えない。
   - prototype / forward-declaration syntaxを追加しない。
   - 同じordinary lexical namespace内のduplicate ordinary function nameはsignature差に関係なくerrorで、overloadを導入しない。

4. **recursionのname resolutionとanalysis precisionを分離する**
   - self / mutual recursive ordinary function namesも同じorder-independent visibilityでresolveする。
   - recursive SCCのsemantic analysisは既存§13.5cに従う。
   - compilerがそのSCCをsoundにanalysisできない場合のprecision rejectionは実装上許容し得るが、
     source-level recursion禁止ruleは本revisionでは追加しない。

5. **ordinary / associated / generic surfaceを混ぜない**
   - 本profileの `fn` はordinary lexical function declarationだけを生成する。
   - §21.7のassociated-function setへ暗黙registrationしない。
   - `fn f<T>(...)` 等のgeneric-looking declarationは本closed grammarの外。
   - generic parameter punctuation / constraints / associated-function source syntaxは固定しない。

## Draft 17.12 の主変更

Draft 17.12 は、R5-01 / M9.6で確認された `unit` source-name ambiguityを局所的に閉じる
**targeted source/name-resolution clarification revision** である。

Draft 17.11までの ownership / lifetime / dependency / explicit-return control semantics は変更しない。
本revisionは、core singleton type/valueのsource spelling `unit` と ordinary lexical name lookup / binding introduction の関係だけを固定する。

1. **`unit` をdistinguished core source spellingとして固定する**
   - type/value expression positionの `unit` はcore unit type/valueを表す。
   - `unit` はordinary lexical lookupでshadowされるpredeclared bindingではない。
   - ordinary lexical namespaceへ `unit` というnameを導入してはならない。

2. **token-level keyword redesignは行わない**
   - lexerが `unit` を一般word tokenとして表現してもよい。
   - reservationはtoken categoryではなくsource-name admissibility / resolution ruleである。
   - field label / variant label等、ordinary lexical bindingを導入しない別namespaceのspellingを本revisionだけで一律禁止しない。

3. **ordinary binding introducerは `unit` を拒否する**
   - ordinary `let` binding
   - multi-result receiver
   - aggregate destructuringが導入するfresh local binding
   - match payload binding
   - source-visible ordinary function/callable/loop parameter等、ordinary lexical bindingを導入するform
   ではexact spelling `unit` をbinding nameとして使用できない。
   source `fn` declaration grammar自体は本revisionでは固定しない。

4. **builtin identityをlexical lookupと混ぜない**
   - `unit` expressionは常にcore singleton valueを表す。
   - `unit;` expression statementやblock tail `unit` も同じcore value。
   - ordinary lexical lookup ruleは `unit` 以外について従来どおり。
   - `let unit = ...` を許可しつつ参照だけbuiltinへ吸収する到達不能binding modelは採用しない。

## Draft 17.11 の主変更

Draft 17.11 は、M9.5で選択した ordinary-function explicit-return source gap を局所的に閉じる
**targeted source-only clarification revision** である。

Draft 17.10までの ownership / lifetime / dependency / function-exit / control-flow semantics は変更しない。
本revisionは、semantic contextですでにidentity/signatureが既知の ordinary function body に対し、
既存の `return expr` semantics を§19.1のexact lexical-block sourceへ一意に写像する最小formだけを固定する。

1. **explicit returnをdedicated lexical block itemとして固定する**
   - exact formは `return expression;`。
   - `return` はordinary value expressionではなく、normal block resultを持たないterminating block item。
   - semicolonは常に必須で、semicolon無しのtail-position returnは本profileに含めない。

2. **bare returnを追加しない**
   - `return;` は本profileに含めない。
   - unit-result functionで明示的returnが必要なら、既存singleton valueを使って `return unit;` と書く。
   - C/Rust等の慣習だけを理由に別のbare-return formを増やさない。

3. **ordinary nested lexical block / match armへreturn contextを伝播する**
   - host-known / registered ordinary-function bodyをreturn-enabled rootとする。
   - そのordinary lexical descendantsはreturn-enabled contextを継承し、match arm lexical blockからもenclosing ordinary functionへreturnできる。
   - nonescaping callable blockまたはloan bodyへ入る境界では、本revisionのreturn-enabled source profileを継承しない。
     callable / loanにおけるnon-local return policyは本revisionでは固定しない。

4. **terminated edgeは既存normal join semanticsをそのまま使う**
   - return edgeは残りのblock item / tail expressionを実行せず、normal block / match / availability joinへ参加しない。
   - normal outgoing edgeが0本のconstructのためにbottom / never typeやsynthetic `unit` resultを追加しない。
   - unreachable-code warning / lint policyはnormative semanticsにしない。

5. **source ordinary-function declarationは固定しない**
   - `fn name(...)` declaration grammar、top-level item grammar、forward reference、recursion、duplicate/name-category policy、
     generic function declaration syntaxは引き続き本profileのscope外。
   - function identity/signatureはhost registration等のsemantic contextから既知でよい。

## Draft 17.10 の主変更

Draft 17.10 は、M9.1で選択した closed-sum source boundary を局所的に閉じる
**targeted source-only clarification revision** である。

Draft 17.9までの ownership / lifetime / dependency / occurrence / sum-value semantics は変更しない。
本revisionは、§26で既に定義済みのclosed nominal sum semanticsへ ordinary source を一意に写像するため、
semantic contextで既知のsum typeに対する最小constructor / match profileだけを固定する。

1. **qualified constructor source formを固定する**
   - payloadless variantは `SumType.Variant`、
     unary payload variantは `SumType.Variant(expression)` をvalid closed formとする。
   - `SumType` はsurrounding type source syntaxで既に表現可能で、semantic contextで具体的なclosed nominal sum typeへ解決されるtype formとする。
   - constructorはordinary function / associated-function / member lookupへfallbackしない。
   - unqualified variant inference、constructor overload ranking、ADL的lookupを追加しない。

2. **match arm source grammarを固定する**
   - closed formは `match expression { ... }`。
   - armは `pattern => { ... }` で、bodyは既存§19.1 lexical block。
   - arm separatorは `,`、trailing commaは許可する。
   - `=>` は隣接した `=` と `>` からなるcontextual two-character punctuatorであり、`= >` はarm separatorではない。
   - newlineは引き続きwhitespaceでありarm separatorではない。
   - empty arm listはsyntaxとして許可し、closed sumに対するlegalityは通常のexhaustiveness ruleで判定する。

3. **v0 sum patternのexact source formsを固定する**
   - `Variant`
   - `Variant(binding)`
   - `Variant(_)`
   だけを本profileで受理する。
   - variant名はscrutineeのstatic nominal sum typeだけに対して解決する。
   - standalone `_`、nested / OR / literal / range / aggregate pattern、guard、implicit ref/mut modeは追加しない。

4. **consuming / borrowed mappingをsyntaxではなく既存static type ruleへ接続する**
   - ordinary sum value scrutineeはconsuming match。
   - `ref<read,Sum>` / `ref<write,Sum>` scrutineeはborrowed match。
   - mode keyword、implicit borrow/deref、source-visible lifetime/effect annotationを追加しない。

general sum declaration grammarは引き続きProvisionalである。
本revisionはpredefined `Option<T>` / `Result<T,E>` およびsemantic contextで既知の同等closed nominal sumを対象とし、
sum declaration、general pattern system、production frontend実装を固定しない。

## Draft 17.9 の主変更

Draft 17.9 は、production compiler P5 Phase A で確認された source-surface gap を局所的に閉じる
**targeted source-surface clarification revision** である。

Draft 17.8 までの ownership / lifetime / dependency / raw-storage / aggregate-value semantics は変更しない。
本revisionは、既に定義済みの意味論を production frontend へ写像するために必要な最小 source formだけを固定する。

1. **multi-result operation の dedicated receiving form を固定する**
   - `let (a, b, ...) = expression` を、複数の独立したresult responsibilityをfresh bindingsへ受け取る専用binding formとする。
   - これはtuple value / tuple type / general tuple patternを導入しない。
   - RHSを一度だけ評価し、result countとreceiver countがexactly一致した場合だけ、全receiver bindingを一括して成立させる。
   - 途中failureで一部receiverだけを成立させない。

2. **lexical block の最小 item / tail-result source ruleを固定する**
   - newlineは従来どおりwhitespaceでありstatement terminatorではない。
   - non-tail itemは `;` で終了する。
   - 最後のsemicolon無しexpressionだけがtail resultになる。
   - tail expressionが無いblockのnormal resultは `unit`。
   - `expr;` はresultをdiscardするsource formであり、ordinary `Discardable` ruleを迂回しない。

3. **registered fixed-shape aggregate の最小 construction / whole-value destructuring formを固定する**
   - semantic contextで既知のnominal aggregateについて `Pair { a: expr_a, b: expr_b }` をaggregate value constructionのclosed formとする。
   - `let Pair { a, b } = expr` をwhole-value destructuringのclosed formとする。
   - fieldは全てexactly once指定し、partial-move state、rest pattern、nested pattern、renaming、aggregate declaration grammarは追加しない。

sum / match のexact constructor / arm surfaceは引き続きProvisionalとする。
P5は本revisionで固定するaggregate routeを使用できる。

## Draft 17.8 の主変更

Draft 17.8 は、R1 post-fix targeted revalidation で確認された `take` の access-boundary gap を閉じる
**single-issue semantic clarification revision** である。

Draft 17.7 の Storage / initialize / ptr / span / dependency semantics は維持し、次の一点だけを明文化する。

1. **ordinary-safe `take` は source current value を ordinary value-flow へ取り出すための read access を要求する**
   - explicit BackingRegion 上のrootでは、source backingがordinary read accessを許さなければならない。
   - write-only backingで `initialize` / mutation / `destroy` が可能であっても、`take` によってold `T` valueを回収してread authorityを迂回してはならない。
   - `destroy` はold valueをcallerへmaterializeしない単一transitionなので、`take + discard` という説明だけを理由にordinary read accessを要求しない。
   - target/platform固有のlifetime-end contractが追加accessを要求する場合は、そのboundary contractに従う。

## Draft 17.7 の主変更

Draft 17.7 は、独立 Red Team R1 first-pass で確認された boundary gap を局所的に閉じる
**targeted semantic closure revision** である。

Draft 17.6 の lifetime / dependency / occurrence / raw-byte model は維持し、
次の5点だけを明確化する。

1. **Storage claim は v0 では non-empty とし、split を exact partition として定義する**
   - safe `Storage` のrange lengthは常に1以上とする。
   - `split(storage,k)` は `0 < k < storage_len(storage)` を要求する。
   - resultは元rangeを欠落・重複なくexactly partitionする。
   - zero-size allocation requestのlibrary/platform surfaceはcore Storage semanticsとは分離する。

2. **typed `initialize` にdestination backing write-access obligationを追加する**
   - `slot<T>` 自体はempty occupancy claimであり得るが、ordinary-safe `initialize` は
     `T` representationを成立させるために必要なtarget-defined write accessを要求する。

3. **implicit local lifetime authorityを ptr -> ref のstability evidenceへ接続する**
   - §10.1のexplicit domain-ref formはexplicit `LifetimeDomain` の代表形とする。
   - §13.7のcompiler-managed local governing identityでは、同等のhidden stability evidenceをcompilerが供給してよい。
   - `unchecked` でもsemantic stability proof自体は省略できない。

4. **zero-length span は BackingRegion identityを要求しない**
   - length 0 のspanはobject / byte rangeをcoverせず、dummy object、sentinel pointer、
     zero-byte allocation、dummy BackingRegionを必要としない。
   - non-empty spanのsingle-live-BackingRegion ruleは維持する。

5. **R1のconservative-rejection findingsはsemantic変更にしない**
   - sum `Discardable` は引き続きtype-wide static propertyとする。
   - compilerがpermitted widening / proof-precision不足によりsafe programをrejectし得ることも維持する。

## Draft 17.6 の主変更

Draft 17.6 は Draft 17.5 を基礎に、M8.6 representative systems workload validation で発見され、
M8.3R targeted reopen で閉じた **raw Storage scalar byte bridge** を normative text へ統合する
**minor raw-storage closure revision** である。

Draft 17.5までのobject / lifetime / authority / placement / dependency / relocation semanticsは維持する。
本revisionはwire / disk / network / database page等のportable explicit codecをordinary NewLang sourceで記述するために、
既存raw representation stateと既存core `byte` scalarの間の最小bridgeだけを追加する。

1. **`byte` をordinary raw representation octet scalarとして閉じる**
   - `byte` はinteger familyとは別のsemantic scalarであり、exactly 256 valuesを持つ。
   - v0 ordinary Storage profileではone Storage byte = one 8-bit octetとする。
   - `byte` はCopy + Discardableで、arithmeticを暗黙には持たない。
   - `byte` と `u8` の間にexplicit total bijective value conversionを持つ。

2. **raw representation validityをDefined / Unspecifiedとして明文化する**
   - `Defined` はruntimeで一つのwell-defined byte valueを持つことを意味し、compilerがexact constantを知ることを意味しない。
   - fresh allocationやtyped lifetime end後のraw bytesは`Unspecified`でよい。
   - `Unspecified`をordinary-safe scalar readでsemantic `byte`へ変換してはならない。

3. **Storage-relative scalar byte observation / updateを追加する**
   - `storage_read_byte(ref<read,Storage>, usize) -> byte`
   - `storage_write_byte(ref<read,Storage>, usize, byte) -> unit`
   - 両operationはStorage claimをconsume / replace / split / mergeせず、typed lifetimeやptr provenanceを生成しない。

4. **safe readはDefinedを要求し、writeはDefinedを確立する**
   - readはbounds、ordinary raw-read access、selected byteのDefined validityをpreconditionとする。
   - writeはboundsとordinary raw-write accessをpreconditionとし、selected byteをDefined(value)へ更新する。

5. **raw-definednessはhidden semantic factとして扱う**
   - source-visible `RawRange` / `DefinedStorage` / initialization bitmap typeは追加しない。
   - compilerはrange / prefix / singleton等のsound approximationで`RawDefined` factを保持・要約してよい。
   - proofを失った場合はDefinedを仮定せず、compile-time rejectionまたはexplicit `unchecked`へ落とす。

6. **external byte-producing boundaryのextension pointを固定する**
   - future platform / I/O / FFI operationが実際にdefined external bytesを書いた場合、そのsemantic summaryは対象rangeの`RawDefined`を確立してよい。
   - これはStorage authority、typed lifetime、ValuePackage、ptr provenanceをmintしない。

7. **`copy_raw_bytes`との非対称性を明文化する**
   - raw copyは`Unspecified`を含むrepresentation stateをpointwiseに転送できる。
   - scalar readは`Unspecified`をinterpretできない。
   - scalar writeはselected byteをDefinedにする。

Draft 17.6 はpointer arithmetic、raw pointer、persistent raw range/slice、bytes-to-typed overlay、
`repr(C)` / packed struct、general transmute、typed raw multi-byte load/store、general reflection / `Layout<T>`、
FFI subsystemを追加しない。

## Draft 17.5 の主変更

Draft 17.5 は Draft 17.4 の semantic model を変更せず、§12 の exclusive reborrow と
§18.2 の ordinary non-Copy argument transfer の関係を call / primitive-operation boundary で
明文化する **normative clarification revision** である。

production compiler P3 の実装検討で、次の二つの規則だけを読むと適用順序が曖昧に見えることが確認された。

- ordinary non-Copy binding を argument に使うと consume / ownership transfer する。
- `exclusive ref` からは、より短い scope の exclusive child を reborrowでき、child終了後にparent authorityを再利用できる。

Draft 17.5 はこの関係を次のように固定する。

1. **already-selected parameter / primitive operandへのexclusive-ref argument useはreborrowする**
   - selected callee parameterまたはprimitive operandが、既存規則の下でcompatibleな `exclusive ref<...,T>` を要求し、
     actualがexisting exclusive-ref bindingである場合、そのargument useはordinary non-Copy transferではなく§12のexclusive reborrowである。
   - child exclusive refはcall / operation extent以下のfresh hidden scope identityを持つ。
   - childがliveな間、parent exclusive refのconflicting useはsuspendされる。
   - child終了後、parent bindingは再びusableになる。

2. **これはgeneral implicit borrowではない**
   - ordinary value `T` から `ref<T>` / `exclusive ref<T>` を暗黙生成しない。
   - overload ranking、user-defined conversion、general implicit conversionを追加しない。
   - exclusive-ref間の新しいauthority weakeningを追加しない。type compatibilityは既存規則で決まる。

3. **ordinary non-Copy transferは維持する**
   - `let e2 = e1` のようなordinary value transferでは、exclusive refも通常のnon-Copy valueとしてsourceをconsumeする。
   - `LifetimeDomain` / `Allocation` / `Storage` 等の他のaffine/non-Copy authority argumentは、別途reborrow ruleが規定されない限り§18.2のordinary transferに従う。

4. **`take` / `destroy` の `ending` へ適用する**
   - canonical `take(p, ending)` / `destroy(p, ending)` は、callerのouter `ending : exclusive ref<read,LifetimeDomain>` をconsumeしない。
   - operation-local exclusive childがlifetime-ending authorityを行使し、operation終了後はouter `ending` を逐次再利用できる。

このrevisionはsemantic mechanismを追加しない。
§12で既に採用済みのexclusive reborrowをfunction / primitive-operation argument useへ明示適用し、
§18.2の一般non-Copy transfer ruleとの優先関係を固定するだけである。

## Draft 17.4 の主変更

Draft 17.4 は Draft 17.3 と M7.3–M7.5 の backend / C-interop / foreign-boundary pressure test を統合した
**M7 closure / status revision** である。

Draft 17.3 の object / lifetime / placement / dependency / raw-storage semantics は変更しない。
新しい source-visible FFI、effect system、function-pointer mechanism、C-layout aggregate mechanismも追加しない。

M7 の adjudication は次の三点だけを normative text へ反映する。

1. **native aggregate と C ABI representation を明確に分離する**
   - native aggregate / sum は、foreign boundaryを越えることだけでC ABI layout identityを得ない。
   - C aggregate interoperabilityを提供する場合、native semantic valueと明示的なC compatibility representationの間を
     semantic field / variant marshallingで接続する。
   - targetごとのC calling / return classificationはcompatibility/backend layerの責務であり、
     NewLang source-level ownership / lifetime / alias semanticsを変更しない。

2. **foreign signature と semantic summary を分離する**
   - foreign function signature、calling convention identity、pointer type、aggregate parameter shapeだけから、
     semantic non-mutation、no-retain、callback behavior、ordinary-return behavior等を推論しない。
   - adequate summaryが無い場合は§13.5cの既存`Unknown` ruleへfallbackする。

3. **v0 scopeを閉じる**
   - persistent function pointer、normative basic FFI surface、general external/static-backing APIは
     v0 core/APIから **Deferred** とする。
   - experimental compiler hook / compatibility frontend / generated C shimを用いた検証は妨げない。
   - external backingのordinary-safe import obligation、foreign raw location / `ValuePackage` boundary、
     `Unknown`の非permission性はDraft 17.3までに既にnormativeであり、そのまま維持する。

LLVM `noalias` / capture attributes / `llvm.lifetime.*`、target ABI classifier、generated C shim strategy、
foreign Checked-MIR fact representation等の具体loweringはsource semanticsではなくbackend contractに属する。

Draft 17.4 はDraft 18級のsemantic revisionではなく、M7で得た反証結果を既存境界へ帰着させ、
「何を今は仕様化しないか」を確定するminor closure revisionとする。

## Draft 17.3 の主変更

Draft 17.3 は Draft 17.2 を基礎に、M6.2 / M6.3 のdynamic-container pressure testと
既存言語・runtimeの外部調査から得た **authority-preserving dynamic-region closure** を反映する
**semantic consolidation revision** である。

Draft 17.2 の object / lifetime / placement / dependency / raw-storage semantics は維持する。
本revisionの中心は、新しいcontainer primitiveを増やすことではなく、
Draft 17.2 §15 のdynamic bridgeを次の小さな一般則へ狭めることである。

```text
metadata describes occupancy
metadata does not mint or erase authority
responsibility is conserved and transferred
```

1. **dynamic container metadataとauthorityを分離する**
   - container metadataはsteady-stateのoperational occupancyを記述してよい。
   - metadataの値だけから`Storage` / `slot<T>` / live-object responsibilityを生成または消去してはならない。
   - metadataが`empty`を示すことは、それ自体ではraw Storage authorityの存在を意味しない。

2. **dynamic hidden responsibilityをrooted ownerに保存する**
   - dynamic containerは、BackingRegion/root originに結び付いたunique responsibility ownerの内部へ
     vacant/live responsibilityを封じてよい。
   - selected locationのclaimを外へ出す操作はauthority reconstructionではなく、
     ownerが既に保持するresponsibilityの一時的なtransferでなければならない。
   - claimが外にある間、owner側は同じresponsibilityを重複利用してはならない。
   - claim return / lifetime transitionではresponsibilityをexactly once ownerへ戻す、または別の明示的consumerへtransferする。

3. **whole Storageはreconstructせず、保持していたroot responsibilityを閉じて返す**
   - explicit `Storage` partitionが存在するならsafe `merge`を優先する。
   - dynamic region内部へresponsibilityを封じた場合は、全live responsibility・outstanding claim・borrow・transitionを閉じた後、
     region/root ownerをconsumeしてoriginのraw responsibilityを返してよい。
   - metadata / pointerだけを根拠に一般的なwhole-Storage authorityを再構成するprimitiveはv0 coreへ固定しない。

4. **steady-state metadataとtransition-local responsibilityを区別する**
   - lifetime transition中、一時的にpublic metadataとphysical/object stateが一致しないことは許される。
   - その間の未公開responsibilityはscoped claim / guard等が所有し、責任を失わせてはならない。
   - inconsistent public stateをsafe observer / reentrant callbackへ公開してはならない。
   - fallible stepを含む場合は、failure pathがresponsibilityを一意にrestore / returnしてからcontrolを外へ返さなければならない。

5. **compiler-provided layout knowledgeとmemory authorityを分離する**
   - opaque generic `T`についても、compilerはtarget-dependentなsize / alignment / element stride等のlayout factsを提供してよい。
   - そのknowledgeは`Allocation` / `Storage` / occupancy / provenance authorityを持たない。
   - field offset / padding map / C-compatible ABI / cross-version ABIを公開することを意味しない。
   - `Layout<T>`等の具体的source spelling、runtime materializationの有無、query APIはProvisionalとする。

6. **dynamic claim source spellingを固定しない**
   - semantic baseはroot/origin-boundなnon-duplicating responsibility transferとする。
   - linear receipt/token、nonescaping closure helper、privileged/dependent hook等の具体surfaceはlibrary/API polish対象である。
   - existing `block(...)`上でlinear claimを明示的にthreadできる場合、そのためだけにexactly-once callable kindを追加しない。

7. **zero-sized storable rootはv0 raw-storage closureの対象に広げない**
   - §23.1の`sizeof(T) >= 1` for storable typesを維持する。
   - future zero-sized storable objectを導入する場合、zero byte extentとlogical object responsibility multiplicityを分離して設計しなければならない。

Draft 17.3 はgeneral Pin / relocation trait、dynamic initialized-index-set typing、implicit object creation、
metadata-derived authority reconstruction、transparent pointer rebindingを導入しない。
本revisionはDraft 18級のsemantic redesignではなく、M6.2 / M6.3で得たauthority境界のminor closure revisionとする。


## Draft 17.2 の主変更

Draft 17.2 は Surface Draft 1.1、identity-validation、および numeric-closure break test の結果を反映した
**normative API-surface semantic closure revision** である。

Draft 17.1 の object / lifetime / occupancy / dependency / backing semantics は維持し、
実際のsystems workloadをsource surfaceへ写像したときに不足した小さなoperationと、
既存numeric / authority ruleのsurface-visible closureだけを追加する。

1. **`Storage` start address observationを追加する**
   - `storage_addr(ref<read,Storage>) -> addr` を ordinary-safe total observation とする。
   - `addr` はauthority / provenance / BackingRegion identityではない。
   - address equalityだけからStorage mergeability、Allocation matching、ptr provenance等を導かない。

2. **`Array<T,N>` whole-value consuming decompositionを追加する**
   - `consume_array(Array<T,N>, block(T)->unit)` 相当operationで、
     received Array valueをindex順に各element valueへexactly-once decompositionできる。
   - source-visible partially-moved Array stateは導入しない。
   - `N == 0` ではconsumer invocationは0回。
   - ordinary argument value-use ruleは維持し、Copy Array argumentを強制moveしない。

3. **ordinary write capabilityからread capabilityへのauthority weakeningをsource ruleとして明文化する**
   - `ref<write,T> -> ref<read,T>`
   - `span<write,T> -> span<read,T>`
   - candidate lookup/ranking用のgeneral implicit conversionにはしない。
   - `exclusive ref` は既存reborrow ruleを使用し、このCopy-capability weakeningには含めない。

4. **`usize` と `uintptr` のnumeric roleを分離して閉じる**
   - `usize` は size / length / count / index / byte offset / alignmentを表すtarget-sized non-negative quantity scalar。
   - `uintptr` はauthority-free numeric machine-address coordinate。
   - `uintptr` をgeneral-purpose unsigned integerとはせず、
     v0では point/displacementとして意味のある小さなmixed algebraだけを持つ。
   - `uintptr + uintptr` 等のsemantically suspicious operationをcoreへ導入しない。

5. **ordinary-safe backingに対するnumeric address coherenceを明文化する**
   - Storage byte displacementとnumeric address coordinateの対応。
   - `alignof` / allocator alignmentと`uintptr % usize`の対応。
   - このcoherenceを提供できないspecial backing / address modelはtarget/platform extension側で扱う。

Draft 17.2 は Pin / pointer arithmetic / general RawRange / trait / effect system / partial-move aggregate 等を導入しない。
本revisionはDraft 18級のsemantic redesignではなく、API polishから得たminor closure revisionとする。


## Draft 17.1 の主変更

Draft 17.1 は、Draft 17 の semantic-core closure 後に行った二本のDeep Research
（FFI / function pointer / external backing、および in-place initialization / out-pointer / field projection）と、
それらを統合したcompiler boundary experimentの結果を反映する **closure clarification** である。

Draft 17 のobject / value / occupancy / dependency semanticsを変更せず、将来のforeign / platform extensionが
既存のsafe coreを暗黙に破らないための境界だけを明文化する。

1. **external backing importのordinary-safe proof obligationを明文化する**
   - external / platform backingをordinary-safe `BackingRegion`として公開してよいのは、公開期間について
     §3.1のnon-alias invariant、backing liveness、range / alignment / access propertyをboundary側が保証できる場合だけである。
   - shared mapping / virtual alias / MMIO / DMA等でその保証を与えられないviewは、aliasするindependent safe `BackingRegion`として公開しない。

2. **pre-lifetime raw locationと`ptr<T>`を明確に分離する**
   - FFI / platform / localized `unchecked` boundaryがobject lifetime開始前のraw location tokenを扱うことは、
     future `T` incarnationを指すsafe `ptr<T>`の予約を意味しない。
   - raw location token自体からsafe `ref<T>`をmaterializeしてはならない。

3. **future privileged / foreign lifetime-start extension pointを予約する**
   - ordinary safe v0 coreのlifetime-start operationは引き続き`initialize(slot<T>, value, domain)`である。
   - ただし将来、FFI / platform / localized `unchecked` transitionが、destinationにcomplete valid representationと
     complete semantic `ValuePackage`が成立したことを別途保証したうえで、`initialize`と同じfresh-root postconditionを
     確立することを妨げない。
   - これはDraft 17.1で新しいsource-visible primitiveを追加するものではない。

4. **foreign bytesとsemantic `ValuePackage`を同一視しない**
   - raw bytesがvalid representationを形成しても、それだけで`Allocation` / `Storage` / `LifetimeDomain` identity、
     ptr provenance、hidden dependency等のvalue-owned semanticsを生成しない。
   - foreign constructionがtyped NewLang rootを開始するfuture extensionでは、それらをboundary contract / wrapperが
     明示的に成立させなければならない。

5. **`Unknown`はescape / retention / asynchronous useへのpermissionではない**
   - summary無しFFI / indirect callを`Unknown`として扱う既存ruleは維持する。
   - `Unknown`はscope-bound capabilityやraw out-locationのretention、callbackのcall-return後 invocation、
     foreign unwind / non-local transfer等を暗黙に許可しない。これらには別のexplicit foreign-boundary contractが必要である。

safe field-by-field aggregate construction、safe future-incarnation `ptr<T>`、general multi-view backing、
escaping closure / asynchronous callback semantics、general source-visible effect systemは引き続きDeferredである。

Draft 17.1 はsemantic mechanism revisionではないため、Draft 18ではなくminor closure revisionとする。


## Draft 17 の主変更

Draft 17 は Draft 16 の semantic-core closure candidate を API polish 前に再確認し、
新しい mechanism を追加せずに残っていた **root governing relation / pre-lifetime ptr / partial-construction wording** の境界を閉じる。

Draft 16 の placement / BackingRegion / aggregate construction / relocation の判断は維持する。

Draft 17 の変更は次の通り。

1. **root governing `LifetimeDomain` relation を place/state-owned state として明文化する**
   - root が `DomainId D` に governed される relation 自体は semantic `ValuePackage` の一部ではない。
   - `take` / ordinary consume-out で source governing relation は value と一緒に transfer されない。
   - fresh root の governing relation はその lifetime-start operation が与える domain から作る。
   - opaque distinct relocation だけは relocation semantics の特則として source と同じ `D` へ fresh governing relation を張る。
   - value 自体に含まれる `LifetimeDomain` identity/authority value の transfer とは別である。

2. **safe pre-lifetime `ptr<T>` minting を v0 core に導入しない**
   - safe `Storage` / `slot<T>` / `addr` から、まだ存在しない future object incarnation 用の `ptr<T>` を作る primitive は持たない。
   - safe root lifetime start では `initialize` が fresh incarnation を開始し、その結果として最初の `ptr<T>` を返す。
   - safe に得た `ptr<T>` が non-live になり得るのは、その origin incarnation/backing が後で終了して stale/dangling になった場合である。
   - platform / FFI / localized `unchecked` boundary が raw location token を導入することは別途許し得るが、future incarnation reservation semantics を safe core に持ち込まない。

3. **Draft 15 由来の stale partial-initialization wording を整理する**
   - ordinary aggregate construction は partially-live target aggregate state を持たない。
   - core が追跡する explicit partial occupancy は `Storage` / `slot` に限定し、dynamic containerは§15のrooted responsibility bridgeで扱う。
   - safe field-by-field aggregate construction は Draft 16 と同じく Deferred のままとする。

Draft 17 でも:

- pre-lifetime/future-incarnation pointer reservation
- construction typestate / commit protocol
- Pin / Unpin
- Relocatable / general relocation trait
- source-visible lifetime/effect system
- general multi-view physical-alias model

は追加しない。

Draft 17 の目的は、API spelling を決め始める前に **何が value とともに移り、何が root state に残るか** と
**ptr provenance がいつ発生するか** を明確にすることだけである。

---

## Draft 16 の主変更

Draft 16 は Draft 15 全体を横断して破壊レビューし、
§3 BackingRegion / occupancy、§13 semantic value package、§14 lifetime transition、
§16–17 aggregate construction、§24 opaque relocation の間に残っていた
**placement / backing identity / construction semantics の境界**を閉じる。

Draft 15 で確定した:

```text
typed semantic move      = take + initialize
raw byte transfer        = Storage-authorized overlap-safe transfer
opaque object relocation = localized runtime boundary
```

という三分割、および:

- `ref<read,Storage>` は ordinary place capability
- opaque relocation は source governing `LifetimeDomain` identity を保存
- raw/object occupancy responsibility を conservation する
- same-place は BackingRegion identity + exact range の semantic no-op
- unspecified raw representation state は raw copy 可能
- ordinary raw transferにはBackingRegion access propertyが必要
- distinct relocationはfresh destination provenanceへの入口を作る

という Draft 15 の判断は維持する。

Draft 16 の変更は次の通り。

1. **object/root placement backing relation を place/state-owned state として明文化する**
   - live lifetime root は `(BackingRegion identity, occupied byte range)` からなる placement relation を持つ。
   - placement relation は semantic `ValuePackage` の一部ではない。
   - `take` / consume-out / relocation でsource placementはvalueと一緒にtransferされない。
   - `initialize` / consume-out destination / relocation destination はdestination側の backing/range からfresh placement relationを得る。
   - value内部の `Storage` claim、`Allocation` authority、ptr provenance、hidden backing dependency等は従来どおりvalue-owned stateとしてtransferする。

2. **distinct live BackingRegion identity の ordinary-safe non-alias invariant を追加する**
   - ordinary safe native semanticsでは、異なるlive BackingRegion identitiesは同じabstract backing byte instanceを同時に共有しない。
   - machine address equality / inequalityとは別のabstract-machine invariantである。
   - virtual alias / shared mapping / FFI等のspecial backingはplatform-specific / localized `unchecked` boundaryで扱い、aliasする二つのordinary independent BackingRegionとしてsafe coreへ公開しない。
   - multiple-view backing model自体はv0ではDeferred。

3. **safe field-by-field aggregate construction を v0 core からDeferredへ移す**
   - `slot<Pair>` からfield `slot<A>`をloanして`initialize`する一般mechanismをv0 coreに持たない。
   - ordinary aggregateはsemantic valueとして構築し、whole `slot<Pair>` へ一度に`initialize`する。
   - compiler/backendはobservable partial object stateを作らない限りin-place constructionへ最適化してよい。
   - dynamic-index partial initializationは従来どおりcontainer metadata + localized dynamic-container bridgeで扱う。ただしDraft 17.3では、このbridgeをmetadata-derived authority reconstructionではなく§15のrooted responsibility transferに限定する。

4. **`into_slot<T>` のrange sizeをexactにする**
   - input `Storage` range lengthは `sizeof(T)` とexactly equalでなければならない。
   - larger raw rangeは`split`でexact-size fragmentを作ってから`into_slot<T>`する。

5. **opaque relocation の structural freshness を明文化する**
   - distinct relocationではplace-owned structural stateをtransferしない。
   - source root placement / root incarnation / fixed-subobject incarnations / current-value facts / conditional occurrence identitiesはsource側で終了する。
   - destination側にfresh root placement、fresh root/fixed-subobject incarnations、fresh current-value facts、必要なfresh conditional occurrencesをordinary structural semanticsに従って作る。
   - exactly-once transferするのはsemantic value packageである。

Draft 16 でも:

- persistent `RawRange` / `RawSpan`
- Pin / Unpin
- Relocatable / TriviallyMovable
- general relocation trait
- general cleanup effect system
- source-visible lifetime parameter
- general multi-view physical-alias model
- safe in-place partial aggregate construction protocol

は追加しない。

Draft 16 の目的は新しいpolicyを導入することではなく、
**place-owned physical placement と transferable semantic value を最後まで分離し、
既存の小さいmechanism setを閉じること**である。

API spelling は引き続き provisional とする。

---

## 0. 文書の読み方

この仕様書では、項目を次の状態で扱う。

- **Fix**: v0 の意味論として採用する。
- **Provisional**: v0 実装で試すが、実装経験により変更してよい。
- **Deferred**: v0 の対象外。将来仕様で検討する。
- **Open**: v0 コンパイラの実装前または実装中に詰める必要がある。

コード例は原則として概念記法であり、最終構文を固定しない。

---

# 1. 言語の目的

## 1.1 主対象

NewLang は、C が現在担っている低水準システムプログラミング領域を主対象とする。

- OS / kernel
- embedded / bare metal
- allocator / runtime / GC
- database internals
- game engine
- custom container
- intrusive data structure
- memory reclamation
- low-level library
- C で書かれている既存コードの段階的移行

一般アプリケーション開発のすべてを一つの言語で置き換えることは目的としない。

## 1.2 設計原則

NewLang v0 の中心原則は次である。

> **Policy-neutral, mechanism-strict.**

言語は、特定のメモリ管理方式・所有権設計・コンテナ構造を一律に強制しない。
一方で、局所的かつ機械的に確認可能な事実はコンパイラが厳格に検査する。

別の表現では、

> C を選びたくなる状況で、データ構造とアルゴリズムそのものについて直接考えられ、
> C より事故が少なく、Rust 固有の ownership / borrow の認知負荷を必要以上に持ち込まないこと。

を狙う。

## 1.3 v0 で重視するもの

- 意味論の小ささ
- 直交性
- 低水準制御
- 明示性
- unsafe / unchecked 責任の局所化
- コンパイラで検査できるものは検査する
- 実装して壊せること
- 後から機能追加できる余白を残すこと

## 1.4 v0 で重視しないもの

- 構文糖の豊富さ
- 高度な型推論
- すべての抽象化を言語本体で表現すること
- 既存言語の便利機能の網羅
- 安定 ABI の早期固定
- 高度な最適化
- 完全なソース互換性

---

# 2. v0 の非目標

以下は v0 では原則として導入しない。

- traits / interfaces / type classes
- associated output types
- specialization
- higher-kinded types
- subtyping
- variance system
- existential types
- escaping closures
- async / await
- exceptions / panic
- RAII / user-defined destructor
- automatic scope cleanup
- general effect system
- multi-dispatch
- function overload ranking
- dependent result type inference
- concurrency / atomics
- volatile / MMIO の本格仕様
- packed struct
- `repr(C)` 相当
- explicit field offsets
- general representation transmute
- general pointer arithmetic
- nullable pointer as primitive
- arbitrary integer-width literal arithmetic beyond v0 needs
- Pin / Unpin 型system
- relocation trait / destructive-move trait
- automatic repair of self/interior pointers during move or replace

---

# 3. Abstract Machine の基本概念

## 3.1 BackingRegion

**Provisional**

`BackingRegion` は、ある byte range の backing bytes 自体が現在存在することを表す
abstract-machine identity である。

BackingRegion は少なくとも概念的に:

- fresh identity
- byte range / size
- required alignment information
- target-defined access properties

を持つ。

BackingRegion identity は machine address と同一ではない。
同じ address range が後に再確保されても、別 BackingRegion incarnation として扱える。

### ordinary-safe BackingRegion non-alias invariant

ordinary safe NewLang native semanticsでは、異なる二つの **live** `BackingRegion` identityが
同じ abstract backing byte instance を同時に共有してはならない。

conceptually:

```text
R1 live
R2 live
R1 != R2
    => AbstractBackingBytes(R1) ∩ AbstractBackingBytes(R2) = empty
```

このinvariantはnumeric machine addressの比較規則ではない。
address reuse、address-space差、target-specific address representation等により、
machine address値だけからBackingRegion identityまたはalias関係を決めてはならない。

OSのvirtual alias、shared mapping、device/platform mapping、FFIから導入された外部alias等、
同じunderlying bytesを複数viewから同時に観測する必要があるmemoryは、
ordinary safe coreでaliasする複数のindependent BackingRegionとしてmintしてはならない。

そのようなmemoryは:

- target/platform-specific backing operation
- FFI / compatibility layer
- localized `unchecked` boundary
- 将来のsingle backing identity + multiple view model

等で扱う。

v0はgeneral multi-view backing modelを定義しない。
platform boundaryがこのinvariant外のaliasを内部で扱う場合も、
そのaliasをordinary safe `Storage` / `slot<T>` / live-object claimの重複として外部へ公開してはならない。

### external / platform backing import obligation

external / platform backingをordinary-safe `BackingRegion`としてsafe coreへ公開するboundaryは、
その公開期間について少なくとも:

- backing bytesが必要なextentでliveである
- 公開するrange / alignment / target-defined access propertyが正しい
- 同じabstract backing bytesをaliasする別のordinary-safe live `BackingRegion`を同時に公開しない

ことを保証しなければならない。

この保証を与えられないshared mapping / virtual alias / device / DMA / foreign viewは、
ordinary-safe independent `BackingRegion`へpromotionしてはならない。
そのviewはplatform / FFI / localized `unchecked` boundary内部に留めるか、
将来のsingle-backing + multiple-view extensionで扱う。

このimport obligationはnumeric address equalityからBackingRegion identityを推論する規則ではない。
同じmachine address / physical pageを観測したという理由だけで同じ`BackingRegion` identityを自動的に再利用してはならない。

この規則によりordinary safe stateでは、distinct BackingRegion identityを
occupancy / relocation / alias analysis上のnon-alias根拠として用いてよい。

BackingRegion identity を per-object runtime tag としてmaterializeすることは要求しない。
compiler / semantic IR が必要な範囲で保持してよい。

BackingRegion は次のいずれかで管理され得る。

- explicit dynamic allocation
- compiler-managed lexical backing
- static / externally provided backing
- C / platform compatibility boundaryから導入されたbacking

本節はallocation policyを一律に定めない。

## 3.2 Allocation authority

explicit に最終 deallocation 可能な BackingRegion に対し、
conceptual `Allocation` authority を一つ対応させる。

性質:

- non-Copy
- non-Discardable
- backing region identity を保持する
- backing bytes を最終的に deallocate する authority
- occupancy claim ではない
- object lifetime-ending authority ではない
- alias exclusivity authority ではない
- dereference authority ではない

> **Backing lifetime authority != object lifetime authority.**

`Allocation` のvalue自体はtransfer可能である。

```text
let a2 = a1
```

のようなnon-Copy transferではauthority valueの保存場所だけが変わり、
対応する BackingRegion identity と既存の `Storage` / `slot` / live objectとの関係は維持される。

`Allocation` の concrete allocator handle / pointer / size 等のruntime representationは
allocator / platform APIに依存してよい。
本仕様が要求するのはsemantic authorityであり、
追加のper-subobject runtime metadataではない。

### conceptual allocate

native allocator surface は後で決めるが、semanticには:

```text
allocate(size, align)
    -> (Allocation, Storage)
```

相当の成功結果を想定する。

返される `Storage` は新しい BackingRegion のfull rangeをcoverするraw claimである。

v0のsafe `Storage` claimは§3.3によりnon-emptyである。
従ってこのconceptual success transitionが`Storage`を返す場合、requested / resulting range sizeは1以上でなければならない。
zero-size allocation requestをlibrary / allocator APIがどのように表現・short-circuit・失敗扱いするかは
そのsurfaceの責務であり、zero-length `Storage`をmintして解決してはならない。

allocation failureの `Result` 等のsurfaceはsum type設計後に決める。

### conceptual deallocate

```text
deallocate(
    allocation: Allocation,
    storage: Storage
) -> unit
```

は少なくとも:

- `allocation` と `storage` が同じ BackingRegion identity に属する
- `storage` がその BackingRegion の full range をcoverする
- required allocator / platform preconditions が成立する

ことを要求する。

`deallocate` は両authorityをconsumeし、その BackingRegion lifetimeを終了する。

`Allocation` 単独ではdeallocateできない。

full-range raw `Storage` が必要であるため、
safe pathではそのrangeとoverlapする:

- live lifetime root
- `slot<T>`
- other `Storage`

のoccupancy claimは残っていない。

dangling可能な `ptr<T>` は残っていてよい。
deallocation後、それらはdead BackingRegionを指すpersistent tokenとなり、
safe ref生成には使えない。

### Draft 17.24 bounded allocated recursive root source profile

**Applicability:** one semantic compilation unit内で§16.3どおり
exactly once completedしたone bounded recursive nominal `H`だけを対象とする
**closed and provisional** actual-source mappingである。
`H`は`Option<ptr<H>>` recursive linkと`u8` siblingを持ち、
§16.3.5により`Discardable(H) == true`。
このprofileでgeneral arbitrary `T` や別のnominalへsource admissionを拡張しない。
type argument `H` はコンパイル時に明確にresolveされたこの一個のnominalのみであり、
generic function/declaration grammarを一般化しない。

#### Closed built-in source forms

```text
one_allocation
    := 'try_allocate_one' '<' H '>' '(' ')'

allocation_result_type
    := 'Option' '<' 'OneBacking' '>'

one_backing_whole_destructure
    := 'let' 'OneBacking' '{' 'allocation' ',' 'raw' '}' '=' expression ';'

one_into_slot
    := 'into_slot' '<' H '>' '(' storage_name ')'

one_erase_slot
    := 'erase_slot' '<' H '>' '(' slot_name ')'

new_domain
    := 'lifetime_domain' '(' ')'

domain_finalization
    := 'finalize_domain' '(' domain_name ')'

explicit_domain_read_reloan
    := 'ref_from_ptr' '(' 'read' ',' ptr_name ',' stable_ref_name ')'

domain_ending_loan
    := 'loan_exclusive_read' '(' domain_name ')' loan_body

one_deallocate
    := 'deallocate' '(' allocation_name ',' storage_name ')'
```

上記`H`、`OneBacking`、`read`はこのclosed builtin routeの
resolved type/contextual spellingであり、ordinary generic instantiationや
任意のmember/function namespace fallbackではない。
`initialize(slot, value, stable)`と`destroy(ptr, ending)`も
**既存§14.1/§14.3の同じsemantic primitiveを呼び出す**
本profileに限るclosed call mappingとする。
`loan_read(life){|stable|...}`は§13.8の既存direct-local read loan、
`loan_write(head@link)`は§17.1の既存bounded field write loanである。

`OneBacking`はこのselected `H`に対してcompilerが登録する
**固定二field nominal result package**であり、
`{allocation: Allocation, raw: Storage}`をexactly once保持する。
`OneBacking`自体はnon-Copy / non-Discardableである。
`Option<OneBacking>`は§26のexisting concrete closed sum
(`None` / `Some(OneBacking)`)であり、§26.12のby-value matchを使える。
payloadを`Some(_)`で捨てることは§26.13のDiscardable ruleでrejectする。
`OneBacking`はcallerへ返された実在のclaim pairをordinary
whole-value destructuringで取り出すための包みであり、
未提供の`(Allocation,Storage)` tuple source/ABIを導入しない。
compiler / allocatorがこれ以外のauthorityを内包して
callerからcleanupを隠してはならない。

`try_allocate_one<H>()`は成功時に:
1. safe native platform allocationによりfresh live BackingRegion identity `R`を
   ちょうど一つ生成し、§3.1のdistinct live non-alias contractを守る。
2. compiler-authorized target`sizeof(H)`（≥1）/ `alignof(H)`に対して
   safe readable/writableな**logical full-range**を確保する。
   allocator-private capacity/header/roundingは公開BackingRegion rangeに含めず、
   deallocation contractと一致するhandleを`Allocation`として保持する。
3. 唯一の`Allocation(R)`とfull-range nonempty `Storage(R)`を
   `Some(OneBacking)`へmove-inする。この時点ではtyped `H` rootは存在しない。
4. failなら`None`のみを返し、caller-visibleなlive`R`やnon-Discardable claimを
   mint/retainしない。allocator内で失敗前に取得したplatform resourcesがあれば
   builtin boundaryが**return前に自ら回収**し、callerへsuccess authorityを
   隠して作ったことにはしない。recoverable allocation failureをtrap扱いしない。

このplatform allocation boundaryは**記述した一つのnative allocation primitive
だけ**をcompiler/targetが実装するものであり、C `malloc` FFI、host-seeded
`Allocation`/`Storage`、untracked `unchecked` owner、GC
または一般source allocator policyを許可しない。

`into_slot<H>(raw)`は§3.4の**exact range length = sizeof(H)**とalignment,
backing liveness, raw occupancyを再検査して`slot<H>`へtransferする。
この成功profileでは上記allocation resultからstraight-lineで渡す場合、
その条件は既知である。oversized Storageの暗黙切捨て、empty claim、
remainder破棄、`slot`とoverlapする raw claimは許可しない。
`erase_slot<H>(empty)`はtyped lifetime終了後にだけ§3.4どおり
同じBackingRegion/full-rangeの`Storage`を返す。

`lifetime_domain()`は§13のfresh `LifetimeDomain` authorityを返し、
`finalize_domain(life)`は同じnon-Copy authorityをconsumeする。
finalizationは§13.8/§14に従い、governed live rootおよびdomain-dependent
live refが無い場合だけ成立する。
`loan_exclusive_read(life){|ending|body}`は
lexical local`life`に対するfresh **exclusive `ref<read,LifetimeDomain>`**
をexactly-once lexical body内だけで生成する。
live ordinary refや同domain dependent refがある間はexclusive loan取得をrejectする。
これは§13.4/§12.1の既存lifetime-ending authorityであり、
ordinary `ref<write,T>`をexclusiveへ強めるものではない。
`loan_body`は§13.8の既存body/normal-result-forwarding/nonescapeを再利用。
**最終的なloan syntax（旧`loan exclusive read ... as`を含む）は引き続きProvisional。**

`ref_from_ptr(read,p,stable)`は§10.1のread-capability
safe ptr→refそのものを、このclosed caseでspelling選定しただけである。
明示`stable: ref<read,LifetimeDomain>`がrootのgoverning identityと一致し、
ptrがcurrent live `H` root incarnationを指すこと、read permission,
alignment, representation, occurrence/dependencies等を証明できる場合だけ
ordinary `ref<read,H>`をloan-scope内で生成する。
`loan_read_ptr(p)`は**local-derived ptr profile**であって
allocated `H`について安易に流用しない。
implicit deref / unproved `unchecked ref_from_ptr(p)` / persistent ref
を新設しない。

`destroy(p,ending)`は§14.3に従い、対象`H`rootのcurrent lifetimeと
**same governing domain**のexclusive ending authorityを要求する。
successはrootのplacement/current incarnationをEndRootし
exact empty `slot<H>`を返す。`Discardable(H)`は§16.3.5により成立する。
同時にliveなconflicting refやsurviving Value/Occurrence/backing
dependencyがあればrejectする。non-Discardable ownerを捨ててのcleanupは不可。
`finalize_domain(life)`はこのafter-destroy stateでだけ許され、
戻ったslot/Storageはdomain authorityそのものを含まない。
`deallocate(allocation,raw)`は§3.2に従いsame`R`,
**full-range**かつ他のlive occupancy無しを証明して両非Copy責任を
exactly once consumeする。ptr tokenはdanglingとして残り得るが、
safe reloanは失敗する。

#### Actual-source schematic success/failure witness

```newlang
struct Node {
    next: Option<ptr<Node>>,
    payload: u8,
}

match try_allocate_one<Node>() {
    None => {
        unit
    },
    Some(bundle) => {
        let OneBacking { allocation, raw } = bundle;
        let empty = into_slot<Node>(raw);
        let life = lifetime_domain();
        let tail = loan_read(life) { |stable|
            initialize(
                empty,
                Node { next: Option<ptr<Node>>::None, payload: u8(2) },
                stable
            )
        };

        let head = Node {
            next: Option<ptr<Node>>::None,
            payload: u8(1)
        };

        let old_none = loan_write(head@next) { |w|
            replace(w, Option<ptr<Node>>::Some(tail))
        };

        let observed = head@next;
        match observed {
            None => { unit },
            Some(q) => {
                loan_read(life) { |stable|
                    let access = ref_from_ptr(read, q, stable);
                    unit
                }
            },
        };

        let old_some = loan_write(head@next) { |w|
            replace(w, Option<ptr<Node>>::None)
        };

        let empty_again = loan_exclusive_read(life) { |ending|
            destroy(tail, ending)
        };
        let full_raw = erase_slot<Node>(empty_again);
        finalize_domain(life);
        deallocate(allocation, full_raw);
        unit
    },
}
```

**This is a candidate source contract, not evidence that the current production
frontend/backend executes the witness.** The exact `H` may be renamed with
its committed link/payload field labels without changing the semantic rule;
the example's `Node` / `next` / `payload` are not privileged spellings.

- `None` branch creates no user-owned authority, ends with`unit`.
- `Some` is a non-Copy/non-Discardable aggregate obligation.
  Its field whole-destructuring moves **both** claims.
  No early return/branch exit may abandon either, the domain, or slot.
- All operations after `Some` in the closed witness are safe deterministic
  checked transitions with known preconditions; only allocation can fail
  recoverably in this first gate. Later independently fallible initialization
  or custom allocator/retry is **not** admitted.
- `old_none`, `old_some` and copied `Option<ptr<H>>` may contain
  persistent ptr tokens, but these are Copy/Discardable and do not themselves
  prolong tail root lifetime. Unlink makes head link`None` before `EndRoot`.
  Conditional`Some` occurrence Reset remains governed by §26.
- The order destroy → erase_slot → finalize_domain → matching deallocate
  leaves no live typed occupancy/slot/Storage/Allocation/domain responsibility.
  Same-address fresh reuse never makes an old`ptr<H>` current.

#### Falsifiable later-P obligations (NOT authorized here)

Positive native observer must record a real target/native allocator event
(not lexical substitution), actual typed H occupying that allocated region,
the same H address stored as the lexical head link and safely reloaned
**before EndRoot**, actual unlink, root lifetime end,
exactly one same-backing matching deallocation, and no observer cleanup.
This source/checked artifact must be the authority for native behavior,
not source-text parse, C policy or host-seeded tokens.

Required negative/control matrix, with checker rejection where the selected
source profile can express the case:

| Input/control | Required evidence |
| --- | --- |
| `None` on failing allocation | branch consumes no responsibility; no hidden allocation leak/trap |
| `Some` bundle dropped, `Some(_)`, missing raw/allocation/domain/slot cleanup | non-Discardable rejection |
| live `ref<read,H>` at `destroy` or live ordinary`life` loan | exclusive ending authority unavailable, reject |
| stale `tail` ptr reloan after `destroy`, after deallocate, or fresh same address | liveness/provenance check reject |
| second destroy/finalize/deallocate | non-Copy consumed/root ended, reject |
| `into_slot` with wrong size/alignment or live occupancy | reject, no forged/partial occupancy |
| wrong BackingRegion / non-full Storage with`deallocate` | reject if provably wrong; unproved matching cannot be accepted as safe |
| `deallocate` with live root, slot or raw fragment | full raw occupancy unavailable, reject |
| `head@payload`, ptr/ref-base `@field`, general/unbounded additional heap node | Draft17.24 one-root source baselineではoutside; Draft17.26は上記の静的second same-H allocation **だけ**を別途admit。payload/ptr-fieldやunbounded/third allocationは依然outside |
| still-linked head ptr at tail EndRoot | ptr is nonblocking; unlink is product witness obligation, **not** invented lifetime-safety error |
| arbitrary `Result` / general `Layout<T>`, external allocator, FFI | separate source decision remains Deferred |
| allocation internal failure with partial resource | no partially published authority or raw claim; atomic `None` contract |
| positive that only a host hook allocates/frees | anti-gaming FAIL, not evidence of NewLang source responsibility |

Draft17.24's one-root source profile **did not license multiple live allocated
nodes**. Draft17.26 above admits **only** a separately specified finite
two-trial/same-H/nested-match, maximum two simultaneous allocated root
composition. This does **not** license arbitrary/more live roots, a
general allocator, pointer ownership policy, detach, traversal,
recursive deletion, concurrency, general pointer arithmetic,
C ABI/FFI or cJSON.
If implementation later proves this closed mapping requires changing
BackingRegion disjointness, occupancy conservation, ptr incarnation,
domain ending, full-range deallocation or hidden dependencies, this candidate
must be returned to independent M/Coordination rather than weakened.

### Draft 17.26 closed nested two-allocation H source profile

This **source-only cardinality/lexical nesting extension** admits one complete
`fn main() -> unit` using **two and only two distinct syntactic
`try_allocate_one<H>()` sites**, the second under the first
`Some` arm. Both invoke the *same* already registered closed builtin for the
*same completed nominal H*. All existing §§3.1–3.4,10–14,16.1/16.3,17.1,19.1,
26 and 27 rules are reused unchanged. No general factory, new pattern/tuple,
generic allocator, destructor, loop or implicit placement operation.

- Each success creates one **fresh** `BackingRegion` `R`,
  `Allocation(R)`, exactly matching full-range `Storage(R)`, and after
  `into_slot<H>` / `lifetime_domain()` / `initialize` a **fresh**
  typed root incarnation `O` and governing `D`. Two overlapping
  successes satisfy `R_h != R_t`, `O_h != O_t`, `D_h != D_t`,
  disjoint live abstract backing bytes and no cross-owner substitution.
  Numeric temporary IDs from different conditional worlds are not
  identity equalities; preserve origin/path/world qualification.
- The outer `Some(head_bundle)` destructures `OneBacking` into unique
  `Allocation_h` and full raw_h. Moving the `allocation` binding to
  `allocation_h` keeps the original owner available in the nested branches,
  while a nested `Some(tail_bundle)` may bind the same exact
  `OneBacking { allocation, raw }` field-name shorthand in a **new lexical
  scope**, then move to `allocation_t`.
  Each owner/slot/domain is consumed **exactly once in every normal arm in
  which that owner exists**. A branch that leaves a non-Discardable owner
  live/unused at scope exit is invalid; if outer non-Copy availability
  differs across join edges, §27.3 rejects the join.
- Both `None` cases are **failure-atomic**: first None creates neither R;
  second None creates no tail grant while head's `Allocation_h`,
  governed live root, and D_h remain available for explicit cleanup.
  A failing allocation may not silently consume any preexisting head owner.
- Head's `ptr_h` is reacquired under `loan_read(life_h)` using
  `ref_from_ptr(write,ptr_h,stable_h)`, then committed `w@next`
  `ref<write,Option<ptr<H>>>` through §17.1 exact ProjectionId.
  `replace` maps to `Change(L_h)` and conditional `Reset`.
  Link Copy `read(r@next)` keeps value/provenance/dependencies; its
  `Some(q)` is a `ptr<H>`, not a tail claim.
  `ref_from_ptr(read,q,stable_t)` MUST be nested in an
  ordinary `loan_read(life_t)` with matching D_t, never D_h.
  Pointer read/write/provenance/current-incarnation rules are unmodified.
- No root or independent fixed-child lifetime is ended by field write/unlink.
  All projected and root/tail refs end before their corresponding
  exclusive D loan. After `Some -> None`, the tail root is still live.
  Explicit matching `destroy` / `erase_slot` / `finalize_domain` /
  `deallocate` is required separately for each live root.
  Old ptr tokens can dangle after EndRoot but cannot become safely reloaned.

#### Complete three-path admitted-source candidate

```newlang
struct Node { next: Option<ptr<Node>>, payload: u8, }

fn main() -> unit {
    match try_allocate_one<Node>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let allocation_h = allocation;
            let vacant_h = into_slot<Node>(raw);
            let life_h = lifetime_domain();
            let ptr_h = loan_read(life_h) { |stable_h|
                initialize(
                    vacant_h,
                    Node { next: Option<ptr<Node>>::None, payload: u8(1) },
                    stable_h
                )
            };

            match try_allocate_one<Node>() {
                None => {
                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<Node>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let allocation_t = allocation;
                    let vacant_t = into_slot<Node>(raw);
                    let life_t = lifetime_domain();
                    let ptr_t = loan_read(life_t) { |stable_t|
                        initialize(
                            vacant_t,
                            Node { next: Option<ptr<Node>>::None, payload: u8(2) },
                            stable_t
                        )
                    };

                    let old_none = loan_read(life_h) { |stable_h|
                        let head_w = ref_from_ptr(write, ptr_h, stable_h);
                        replace(head_w@next, Option<ptr<Node>>::Some(ptr_t))
                    };
                    let observed = loan_read(life_h) { |stable_h|
                        let head_r = ref_from_ptr(read, ptr_h, stable_h);
                        read(head_r@next)
                    };
                    match observed {
                        None => { unit },
                        Some(q) => {
                            loan_read(life_t) { |stable_t|
                                let tail_r = ref_from_ptr(read, q, stable_t);
                                unit
                            }
                        },
                    };
                    let old_some = loan_read(life_h) { |stable_h|
                        let head_w2 = ref_from_ptr(write, ptr_h, stable_h);
                        replace(head_w2@next, Option<ptr<Node>>::None)
                    };

                    let empty_t = loan_exclusive_read(life_t) { |ending_t|
                        destroy(ptr_t, ending_t)
                    };
                    let full_t = erase_slot<Node>(empty_t);
                    finalize_domain(life_t);
                    deallocate(allocation_t, full_t);

                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<Node>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
            }
        },
    }
}
```

This is a **newly proposed spec-source witness, NOT current
production/native execution evidence**. Its `Option<OneBacking>`
consuming match returns unit on every normal edge; both nested
arms consume first-success responsibility, and the second Some
also consumes second-success responsibility. The same nominal H
is used twice, without register/completion duplication.
The only change from Draft17.25 is bounded second-allocation
**source eligibility**.

#### Future falsification/control matrix — NOT test/CI implementation

| Case | Normative/source expectation or future evidence |
| --- | --- |
| first `None` | no second allocation/typed roots/domain/free |
| second `None` | one first-root EndRoot, one exactly matched free, no tail owner |
| both `Some` | two disjoint simultaneous R/D/root identities, heap head link holds actual heap-tail ptr; two matched frees |
| duplicate, omit or cross-use A_h/A_t/Storage | nonCopy availability / full-range / BackingRegion mismatch reject |
| use wrong D_h for tail reloan/destroy | same-governing-domain rule reject |
| first-None path fabricates head claims | reject, no success grant |
| one normal nested branch consumes head owner, other does not | §27.3 outer availability join mismatch reject |
| read-only head ref used for write | §11 mode amplification reject |
| head link Copy treated as tail owner/deref | invalid authority; ptr token alone carries no release or safe ref |
| live ref at EndRoot; stale q reloan after tail EndRoot | scope/ending or §10.1 current-liveness violation reject |
| `Some -> None` link write | root/field incarnation preserved, conditional occurrence Reset and Change(L_h), no tail EndRoot |
| accidentally equal numeric IDs across nested worlds | no equality proof; preserve world-qualified semantic identities |
| two-H source accepted by future checker but backend unsupported | implementation coverage limitation, not new core law |
| arbitrary third heap root/unbounded allocations | outside this bounded source profile, not generally an unsafe language operation |
| observer injects authority or repairs native link | anti-gaming FAIL; only checked source-backed runtime work counts |

Separate actual-source negative diagnostics from checked-only/programmatic
identity attacks and generated-C corruption controls.
No frozen cJSON/preregistration/oracle change or next-track start.

## 3.2b Draft 17.30 — exact five original heap H roots and three fixed pointer-field source substrate

**ADOPTED / CANONICAL — Compiler Issue #234, PR #235.** This is an additive,
source-admission-only, finite profile, not an assertion that Draft17.29
already admits five roots or that any current compiler has parsed/executed
the source example. It selects the *pre-detach* heap graph substrate only.
The two-root one-link profile of Draft17.29 remains unchanged and valid;
none of §18.1a/b/c producer/recipient/LiveTail source routes is automatically
generalized to the three-link/five-root H. No middle-B detach, owner adoption,
recursive delete, delayed custody, extra function, concurrent publication
or full cJSON North Star PASS is in this selection.

### 3.2b.1 Exact finite lexical/nominal source additions

For a *single* semantic compilation unit and exactly one completed recursive
nominal H, admit the following **one additional alternative** to §16.3's
one-link H declaration:

~~~text
bounded_five_root_three_link_H :=
  'struct' H '{'
    'next'  ':' 'Option<ptr<H>>' ','
    'prev'  ':' 'Option<ptr<H>>' ','
    'child' ':' 'Option<ptr<H>>' ','
    'payload' ':' 'u8' [',']
  '}'
~~~

The three named field declarations are **three separate actual committed
fixed-subobject places**, with three distinct compiler-owned opaque
ProjectionIds, stable field identities/known-disjoint sibling relations,
and the *same* enclosing typed H root incarnation. They are not alias
spellings for a single slot, not compiler-emitted fake C fields and not
independent lifetime roots. Their labels and order select this specific
frozen cJSON source profile; they do **not** establish owner/release policy
or permanent ABI offsets/layout. No sixth field, second recursive nominal,
different self-pointer target, generic field, non-pointer owner field,
source-visible forward header, repetition/array shorthand or general
struct declaration is admitted. H completion remains transactional under
§16.3.1–6: create one stable H header, resolve each `ptr<H>` against the
same H, reject value-containment cycles, complete all four fields once,
rollback the *entire* incomplete declaration on any error. Exactly
complete H has `Copy=true, Discardable=true`: three separate
`Option<ptr<H>>` Copy/Discardable fields and one u8. No recursive by-value
H occurs.

One additional **single exact** allocation-source shape, rather than
general allocator access: one `fn main()->unit` has exactly **five
syntactically distinct**, strictly nested, known
`try_allocate_one<H>()` expressions. Site i+1 occurs only inside site i's
`Some` arm and executes only after original success i. Each `Some`
whole-destructures its own nonCopy `OneBacking` into its original unique
`Allocation(R_i)` and full-range nonempty `Storage(R_i)`, uses the existing
§3.4 `into_slot<H>` exact layout check, creates an original independently
governing `LifetimeDomain D_i`, then whole `initialize` to one typed
root O_i/ptr_i; `None` must create **no** extra owner and all previously
succeeded sites on that branch must explicitly recover full raw/finish
D/deallocate. All five R_i/O_i/D_i/A_i are **different origin identities**;
each newly allocated region must be disjoint from every currently live
original R_j. A compiler-internal numeric ID equality across branches
or distinct analysis worlds is never a source proof of equality/alias.
A sixth site, reversed/independent/recursive allocation, arbitrary
unbounded loop, general-purpose allocator, third-party region import,
heap-owner registration or synthetic creation is NOT source-admitted.

For this completed three-link H, extend §17.1's existing **bounded,
mode-preserving ref-base field source category only**:
`ref_name@next`, `ref_name@prev`, `ref_name@child`, each resolving from
`ref<read,H>` to the corresponding `ref<read,Option<ptr<H>>>`,
and likewise `ref<write,H>` to `ref<write,Option<ptr<H>>>`.
`read(ref_name@field)` and `replace(ref_name@field, Some/None)`
retain the existing exact §17.1/§17.4 source meaning, using the
selected field's *distinct* ProjectionId and the caller's explicit
current domain-backed H ref. For completeness, existing direct-local
`local_name@field` Copy read and `loan_write(local_name@field)`
may select exactly those three committed fields when the local is
itself the completed three-link H (not a heap `ptr<H>`).
No `@payload`, ptr-base deref, `r.field`, general field overload,
implicit borrow, write from read, `exclusive` upgrade or noalias.
A derived ordinary ref remains non-escaping and dependent on its
root's correct D-scoped loan. Permission/access/liveness/provenance
conditions of §10.1 and §11 remain entirely unchanged.

### 3.2b.2 Independent field current value and conditional occurrence laws

Let one current live H root O_A have fixed fields
L_next, L_prev, L_child. Their identities are pairwise distinct,
fixed within O_A, and sibling-disjoint for §13.5a/b dependencies.
Each has its **own** current `Option<ptr<H>>` sum root and, when
Some, its own current conditional payload occurrence P_field.
`replace(w@next,Some(ptr_B))` must apply
`Change(L_next)+Reset(L_next)`, preserve O_A and all three
fixed-field incarnations, invalidate only dependent OLD `next`
current-value/conditional-occurrence facts plus necessary overlapping
ancestor facts, and leave `prev`/`child` disjoint sibling value and
occurrence facts intact. The analogous rules apply separately to
`prev` and `child`. Same old/new pointer address still creates a fresh
Some payload occurrence under §26.7. A current projected/payload ref
whose dependency is invalidated may NOT survive the change; unclear
alias or `Unknown` is conservatively a blocker. An ordinary
`ref<write,H>` is **not exclusive**. The read and replace effects
and any *known direct function* call that already exists in this
single unit must use opaque per-field projection identity,
body-sensitive post-state substitution and alias-aware checks;
this substrate selects **no new function body/typed owner summary
source**.

A Copy `ptr<H>` stored in any link preserves original target
provenance but is NOT an owning edge, live loan, or release authority,
and by itself does not block target EndRoot. Copying a whole current
`Option<ptr<H>>` does not create or prolong the active field payload
occurrence identity. An old **payload-dependent ref** cannot survive
the old occurrence's Reset, while an old copied `ptr<H>` may still
point to a live target and be safely reloaned **only with** fresh
valid §10.1 current-root/correct-D/access/stability proof; it never
upgrades to A/D/Storage. The five unique nonCopy A_i/D_i remain
held by their original branch-local owner bindings through this
pre-detach positive witness. Nothing here implies cJSON's
`next/prev/child` field values form an ownership graph.

### 3.2b.3 Exact five-site full source-shaped positive witness

**ADOPTED SOURCE CONTRACT: NOT supported by the production compiler/native
yet and NOT a cJSON port.**
No source macro/`SET` pseudo-intrinsic, host-injected checker fact,
privileged semantic hook, reused physical proxy root or unchecked
per-hop repair appears. The source uses existing individual
`loan_read`, `ref_from_ptr(write,...,stable)`, resolved `@field`,
ordinary `replace`, original EndRoot/full Storage recovery and
explicit matched deallocation. The only new admissions are this
exact 5-site/four-field H shape and expanded field eligibility.
The checkpoint is reached **before any detach/adoption operation**.
Cleanup after that checkpoint is normal independent memory
responsibility discharge; copied links may become dangling tokens
but are never dereferenced, reborrowed or used for release.

~~~newlang
struct Node {
    next: Option<ptr<Node>>,
    prev: Option<ptr<Node>>,
    child: Option<ptr<Node>>,
    payload: u8,
}

fn main() -> unit {
    match try_allocate_one<Node>() {  // independent allocation site 1: src
        None => {
            unit
        },
        Some(bundle_src) => {
            let OneBacking { allocation, raw } = bundle_src;
            let allocation_src = allocation;
            let vacant_src = into_slot<Node>(raw);
            let life_src = lifetime_domain();
            let ptr_src = loan_read(life_src) { |stable_src|
                initialize(vacant_src, Node {
                    next: Option<ptr<Node>>::None,
                    prev: Option<ptr<Node>>::None,
                    child: Option<ptr<Node>>::None,
                    payload: u8(1)
                }, stable_src)
            };
            match try_allocate_one<Node>() {  // independent allocation site 2: A
                None => {
                    let empty_src = loan_exclusive_read(life_src) { |ending_src|
                        destroy(ptr_src, ending_src)
                    };
                    let full_src = erase_slot<Node>(empty_src);
                    finalize_domain(life_src);
                    deallocate(allocation_src, full_src);
                    unit
                },
                Some(bundle_A) => {
                    let OneBacking { allocation, raw } = bundle_A;
                    let allocation_A = allocation;
                    let vacant_A = into_slot<Node>(raw);
                    let life_A = lifetime_domain();
                    let ptr_A = loan_read(life_A) { |stable_A|
                        initialize(vacant_A, Node {
                            next: Option<ptr<Node>>::None,
                            prev: Option<ptr<Node>>::None,
                            child: Option<ptr<Node>>::None,
                            payload: u8(2)
                        }, stable_A)
                    };
                    match try_allocate_one<Node>() {  // independent allocation site 3: B
                        None => {
                            let empty_A = loan_exclusive_read(life_A) { |ending_A|
                                destroy(ptr_A, ending_A)
                            };
                            let full_A = erase_slot<Node>(empty_A);
                            finalize_domain(life_A);
                            deallocate(allocation_A, full_A);
                            let empty_src = loan_exclusive_read(life_src) { |ending_src|
                                destroy(ptr_src, ending_src)
                            };
                            let full_src = erase_slot<Node>(empty_src);
                            finalize_domain(life_src);
                            deallocate(allocation_src, full_src);
                            unit
                        },
                        Some(bundle_B) => {
                            let OneBacking { allocation, raw } = bundle_B;
                            let allocation_B = allocation;
                            let vacant_B = into_slot<Node>(raw);
                            let life_B = lifetime_domain();
                            let ptr_B = loan_read(life_B) { |stable_B|
                                initialize(vacant_B, Node {
                                    next: Option<ptr<Node>>::None,
                                    prev: Option<ptr<Node>>::None,
                                    child: Option<ptr<Node>>::None,
                                    payload: u8(3)
                                }, stable_B)
                            };
                            match try_allocate_one<Node>() {  // independent allocation site 4: C
                                None => {
                                    let empty_B = loan_exclusive_read(life_B) { |ending_B|
                                        destroy(ptr_B, ending_B)
                                    };
                                    let full_B = erase_slot<Node>(empty_B);
                                    finalize_domain(life_B);
                                    deallocate(allocation_B, full_B);
                                    let empty_A = loan_exclusive_read(life_A) { |ending_A|
                                        destroy(ptr_A, ending_A)
                                    };
                                    let full_A = erase_slot<Node>(empty_A);
                                    finalize_domain(life_A);
                                    deallocate(allocation_A, full_A);
                                    let empty_src = loan_exclusive_read(life_src) { |ending_src|
                                        destroy(ptr_src, ending_src)
                                    };
                                    let full_src = erase_slot<Node>(empty_src);
                                    finalize_domain(life_src);
                                    deallocate(allocation_src, full_src);
                                    unit
                                },
                                Some(bundle_C) => {
                                    let OneBacking { allocation, raw } = bundle_C;
                                    let allocation_C = allocation;
                                    let vacant_C = into_slot<Node>(raw);
                                    let life_C = lifetime_domain();
                                    let ptr_C = loan_read(life_C) { |stable_C|
                                        initialize(vacant_C, Node {
                                            next: Option<ptr<Node>>::None,
                                            prev: Option<ptr<Node>>::None,
                                            child: Option<ptr<Node>>::None,
                                            payload: u8(4)
                                        }, stable_C)
                                    };
                                    match try_allocate_one<Node>() {  // independent allocation site 5: dst
                                        None => {
                                            let empty_C = loan_exclusive_read(life_C) { |ending_C|
                                                destroy(ptr_C, ending_C)
                                            };
                                            let full_C = erase_slot<Node>(empty_C);
                                            finalize_domain(life_C);
                                            deallocate(allocation_C, full_C);
                                            let empty_B = loan_exclusive_read(life_B) { |ending_B|
                                                destroy(ptr_B, ending_B)
                                            };
                                            let full_B = erase_slot<Node>(empty_B);
                                            finalize_domain(life_B);
                                            deallocate(allocation_B, full_B);
                                            let empty_A = loan_exclusive_read(life_A) { |ending_A|
                                                destroy(ptr_A, ending_A)
                                            };
                                            let full_A = erase_slot<Node>(empty_A);
                                            finalize_domain(life_A);
                                            deallocate(allocation_A, full_A);
                                            let empty_src = loan_exclusive_read(life_src) { |ending_src|
                                                destroy(ptr_src, ending_src)
                                            };
                                            let full_src = erase_slot<Node>(empty_src);
                                            finalize_domain(life_src);
                                            deallocate(allocation_src, full_src);
                                            unit
                                        },
                                        Some(bundle_dst) => {
                                            let OneBacking { allocation, raw } = bundle_dst;
                                            let allocation_dst = allocation;
                                            let vacant_dst = into_slot<Node>(raw);
                                            let life_dst = lifetime_domain();
                                            let ptr_dst = loan_read(life_dst) { |stable_dst|
                                                initialize(vacant_dst, Node {
                                                    next: Option<ptr<Node>>::None,
                                                    prev: Option<ptr<Node>>::None,
                                                    child: Option<ptr<Node>>::None,
                                                    payload: u8(5)
                                                }, stable_dst)
                                            };
                                            // Pre-detach checkpoint: exactly five original live roots;
                                            // distinct R_src/R_A/R_B/R_C/R_dst and D_src/D_A/D_B/D_C/D_dst.
                                            // dst.child stays None from its whole initial construction.
                                            let old_src_child = loan_read(life_src) { |stable_src_child|
                                                let w_src_child = ref_from_ptr(write, ptr_src, stable_src_child);
                                                replace(w_src_child@child,
                                                    Option<ptr<Node>>::Some(ptr_A))
                                            };
                                            let old_A_prev = loan_read(life_A) { |stable_A_prev|
                                                let w_A_prev = ref_from_ptr(write, ptr_A, stable_A_prev);
                                                replace(w_A_prev@prev,
                                                    Option<ptr<Node>>::Some(ptr_C))
                                            };
                                            let old_A_next = loan_read(life_A) { |stable_A_next|
                                                let w_A_next = ref_from_ptr(write, ptr_A, stable_A_next);
                                                replace(w_A_next@next,
                                                    Option<ptr<Node>>::Some(ptr_B))
                                            };
                                            let old_B_prev = loan_read(life_B) { |stable_B_prev|
                                                let w_B_prev = ref_from_ptr(write, ptr_B, stable_B_prev);
                                                replace(w_B_prev@prev,
                                                    Option<ptr<Node>>::Some(ptr_A))
                                            };
                                            let old_B_next = loan_read(life_B) { |stable_B_next|
                                                let w_B_next = ref_from_ptr(write, ptr_B, stable_B_next);
                                                replace(w_B_next@next,
                                                    Option<ptr<Node>>::Some(ptr_C))
                                            };
                                            let old_C_prev = loan_read(life_C) { |stable_C_prev|
                                                let w_C_prev = ref_from_ptr(write, ptr_C, stable_C_prev);
                                                replace(w_C_prev@prev,
                                                    Option<ptr<Node>>::Some(ptr_B))
                                            };
                                            // At this exact point seven current cJSON facts hold:
                                            // src.child=A; A.prev=C; A.next=B; B.prev=A;
                                            // B.next=C; C.prev=B; dst.child=None.
                                            // A.prev=C is the FIRST-CHILD TAIL SHORTCUT, not A.prev=None.
                                            // No B detach, no adoption, no other graph mutation.
                                            // Test-end cleanup ONLY. Copy ptr links can dangle and
                                            // are never dereferenced or reloaned after target EndRoot.
                                            let empty_dst = loan_exclusive_read(life_dst) { |ending_dst|
                                                destroy(ptr_dst, ending_dst)
                                            };
                                            let full_dst = erase_slot<Node>(empty_dst);
                                            finalize_domain(life_dst);
                                            deallocate(allocation_dst, full_dst);
                                            let empty_C = loan_exclusive_read(life_C) { |ending_C|
                                                destroy(ptr_C, ending_C)
                                            };
                                            let full_C = erase_slot<Node>(empty_C);
                                            finalize_domain(life_C);
                                            deallocate(allocation_C, full_C);
                                            let empty_B = loan_exclusive_read(life_B) { |ending_B|
                                                destroy(ptr_B, ending_B)
                                            };
                                            let full_B = erase_slot<Node>(empty_B);
                                            finalize_domain(life_B);
                                            deallocate(allocation_B, full_B);
                                            let empty_A = loan_exclusive_read(life_A) { |ending_A|
                                                destroy(ptr_A, ending_A)
                                            };
                                            let full_A = erase_slot<Node>(empty_A);
                                            finalize_domain(life_A);
                                            deallocate(allocation_A, full_A);
                                            let empty_src = loan_exclusive_read(life_src) { |ending_src|
                                                destroy(ptr_src, ending_src)
                                            };
                                            let full_src = erase_slot<Node>(empty_src);
                                            finalize_domain(life_src);
                                            deallocate(allocation_src, full_src);
                                            unit
                                        },
                                    }
                                },
                            }
                        },
                    }
                },
            }
        },
    }
}
~~~

**Actual source-positive obligations, not runtime measurements:** At the
checkpoint the current field semantic values must be:
`src.child=Some(A)`, `A.prev=Some(C)` (the exact *first child's prev
tail shortcut*), `A.next=Some(B)`, `B.prev=Some(A)`,
`B.next=Some(C)`, `C.prev=Some(B)` and `dst.child=None` from
its initialization. These are **six populated pointer-link facts plus
one empty destination fact**, source established by six independent
scoped ref-projection / replace operations on five actual live H roots.
This graph has *five original simultaneously live allocations*. It
does not implement cJSON traversal or a B detach/adoption operation.
With all those loans expired, no typed-reference access to freed
roots occurs in teardown. Outstanding Copy tokens stored inside
still-live roots may become dangling; §10.1 forbids reloan after
their targets EndRoot, but persistent ptr tokens alone do not block
explicit separate destruction. A compiler that cannot prove a
particular current O/R/D or field write must reject (precision
failure, not a permission to fabricate a safe proof).

**Failure-world responsibility table (all paths fully accounted):**

| First failing site | Original successful grants | Exact releases | Outcome |
| --- | --- | --- | --- |
| #1 src | none | 0 | no graph or live authority |
| #2 A | src | 1: src | no synthetic A |
| #3 B | src, A | 2: A, src | A/src each EndRoot/full raw/finalize/free |
| #4 C | src, A, B | 3: B, A, src | no C or dst grant |
| #5 dst | src, A, B, C | 4: C, B, A, src | no dst grant |
| no failure | src, A, B, C, dst | 5: dst, C, B, A, src **after** checkpoint | all original roots/allocations freed exactly once |

For every success site the only grants are one fresh complete
`OneBacking` (original A+full Storage) then the corresponding typed
root/Domain, with full typed occupancy never overlapping an owned
full raw claim. At each failure, no new nonCopy responsibility
appears and all existing original A_i/D_i are available and
explicitly consumed on that branch. At all successful normal exits
no A_i, D_i, slot or full raw is left Available/stranded;
existing §18.3/§27.3 normal availability joins apply. No hidden
destructor, implicit Drop or exception unwind is assumed.
Source-level `replace` old link values are `Option<ptr<H>>` Copy and
Discardable; they are NOT transferable nonCopy owner receipts.

### 3.2b.4 Required adversarial rejection and scope limitations

| Adversarial source/state | Required result and classification |
| --- | --- |
| `try_allocate_one<H>()` sixth syntactic site, or sixth R introduced as original by Copy/pointer alias | **SOURCE REJECT** if extra site; **SEMANTIC REJECT** synthetic/double backing claim. No implicit grant. |
| Allocator returns same live R for two `Some` successes, aliases full raw or clones nonCopy A_i | **SEMANTIC REJECT** or fail-closed platform-boundary violation under §3.1/3.2; not made safe by a different variable name. |
| `ref_from_ptr(write, ptr_B, stable_A)` with equal H static type | **SEMANTIC REJECT** (§10.1 wrong governing D); ordinary write-ref cannot substitute for correct stability. |
| Ending O_A with `Allocation_B`, `Storage_A`, or `D_C`, or consuming A_i twice | **SEMANTIC REJECT** wrong original identity/full R/nonCopy availability; Copy ptr does not own B. |
| `root_w@payload`, `root_w@unknown`, `ptr_A@next`, other nominal, not-yet-completed H | **SOURCE/SEMANTIC REJECT**: only three exact resolved fixed pointer fields from proper scoped H ref; no fallback or ptr deref. |
| Treat A.next and A.prev as the *same* projected place/ProjectionId, or assume they are different physical allocations | **SEMANTIC/COMPILER UNSOUND**: distinct sibling fixed places inside *one* O_A; not extra regions. |
| Read/match Some from A.next, retain occurrence-dependent `ref` and then `replace(A.next,None)` | **SEMANTIC REJECT** of surviving old payload dependency (§26.7); a copied standalone ptr may remain valid if target still live but cannot preserve old payload occurrence. |
| Call `destroy(ptr_A,ending_A)` with A-dependent scoped `ref` still live, or derived field ref leaves loan scope | **SEMANTIC REJECT** (§10/§11/§13/§18.8). |
| Lose Allocation_B/Domain_B on #4 NULL branch, or normal-join path has A/D Available on only one edge | **SEMANTIC REJECT** nonDiscardable normal-exit/§27.3; no leaked hidden destructor. |
| Use a copied link ptr as a recipient ownership receipt or recover Storage_B while O_B remains live | **SEMANTIC REJECT** authority/occupancy mismatch; stored ptr is nonowning locator only. |
| `ref<read,H>@child` used as write destination, or may/Unknown root/member alias assumed noalias | **SEMANTIC REJECT / conservative precision REJECT**; ordinary ref<write> nonexclusive. |
| Correctly safe link wiring but a wrong cJSON list invariant (e.g. `A.prev=None` instead of C) | **APPLICATION POLICY / FUTURE ORACLE FAILURE**, NOT by itself a NewLang memory-safety rejection. |

This finite source contract **does not** infer true list membership,
parent ownership, reference-vs-owning child semantics, recursive
cleanup order, or cJSON unlink rules. The precise seven facts are
an application test oracle. A verifier may reject more source programs
than semantically safe ones when current world/field/alias facts are
Unknown; it must never treat Unknown as free authority.
There is NO new semantic rule letting a live `Storage` overlap a
typed root, `ptr<H>` dereference, automatic root-transfer, hidden
scope extension, ref noalias, or a link become an owner.

### 3.2b.5 §4.2 Gate A–D / alternatives / no-foreclosure

**Current normative:** Draft17.29 §3.2 precisely two nested
same-H fallible sites; §16.3 exactly one committed recursive
Option ptr field plus u8; §17.1 fixed mode-preserving H field
projection, §17.4/§26.7 exact Change/Reset; §§3.1–3.4/
10–14/18/27 root/backing/domain/availability; DI-009/010/011/012/013.
None already source-admits third/fourth/fifth site or
second/third fixed link.

**Historical / non-normative:** experimental SurfaceDraft1/1.1
general aggregate/field and explicit allocator/source sketches,
former `requires fn`/unchecked bridges; early module/private receiver
break-test's `@` ref projection; M0 front-end implemented those
experimental fields but deliberately omitted Allocation/Domain/
destroy; historic Draft17.21 first self-recursive H completion,
Draft17.22 punctuation adjudication (Issue #150/PR #151),
Draft17.23/25 scoped field restrictions and Draft17.26 two-site
adoption. Past full M9 chat archive has not been exhaustively audited.
These experiments inform this bounded additive selection but are
NOT silently adopted as general syntax/authority.

**Gate C choices:**
- (A) **INTENTIONALLY EXTEND SOURCE ONLY — selected and adopted**:
  exactly five same-H static nested fallible sites and one precise
  three named fixed pointer-field H alternative, mode-preserving
  projection for all three. **KEEP** distinct original A/R/O/D,
  §16.3 transactional H completion, separate field ProjectionIds,
  ptr nonowning provenance, §13/17/26 dependencies and conservative
  direct-call semantics.
- (B) **DEFER** retain existing two-site/one-link shape and use
  general/raw allocator, synthetic C proxy fields, runtime registry,
  `requires`/per-hop unchecked or generic user-defined `Owner<T>`
  to claim equivalence: it cannot prove five-original-root current
  source or three independent typed subobject projection with
  the preregistered zero per-hop unchecked fidelity criterion.
- (C) **DEFER** broad general N-site allocation, arbitrary
  recursive record/field/visibility type construction, universal
  field mutation, graph owner contract, implicit cJSON cleanup,
  FFI, threads and modules. No core invariant must change for
  the narrow profile; generalization is a separate design issue.
- (D) **HOLD** only if a further soundness review identifies an
  unavoidable original R/O/D overlap/conditional-occurrence
  contradiction not addressable by the existing §3/§10–14/§17/§26
  mechanisms, or if the specified source identity cannot be
  proven without host-seeded type-only authority. No such
  contradiction was established by this bounded M design.

**Blast radius / no-foreclosure:** additive source eligibility
(5 nested sums and three committed H fields) affects AST
declaration completion, exact field ProjectionId metadata,
per-field current-memory Change/Reset, branch-specific A/D
world identities, checker precision and **future** C emission
for five originals; independent formal/Red Team/native observers
would need separate authorizations to test it. It does **not**
change layout/ABI commitments or make `@` field into indirect
pointer deref, introduce new lifetime/owner semantics,
increase general accepted allocation count outside this profile,
or automatically authorize §18.1a/b/c arbitrary three-field
owner producer/adopter. The future fixed cJSON target still
requires separately established middle-B detach/adoption and
North Star scoring. Product target, topology >=5/6,
B1–B8 corpus, human-obligation L/R/P/V/C and anti-gaming
rules remain UNCHANGED. Source substrate acceptance alone
is not five-root compiler/native execution or cJSON PASS.


## 3.2c Draft 17.31 candidate — original five-root source-correlated LiveRoot handoff

**PROPOSED / UNMERGED, Compiler Issue #272. NOT canonical while CURRENT_SPEC.md selects Draft17.30.**
This is one narrow, independently reviewable source-admission candidate. It is neither an accepted
source program nor a production/Lean/native result. The selected Draft17.30 §3.2b
five-original-root, three-field PRE-detach source and separate Draft17.29
§18.1a–c two-root LiveTail/Option are unchanged.

### 3.2c.1 Frozen scope and bounded user-declared values

One semantic compilation unit has exactly §3.2b completed H =
Node{next,prev,child:Option<ptr<Node>>,payload:u8}, five nested and distinct
successful original allocation sites src,A,B,C,dst, six initial field writes,
four detach field Changes and two adopt Changes, all original independent
BackingRegions/typed roots/domains and 0..5 failure paths. No sixth heap Node,
new H owner field, extra allocation, root remint, GC/Drop/RAII, owner registry,
unchecked at each pointer hop, general arbitrary owner graph or actor.

Admit **only** complete, acyclic, non-generic, fixed-shape user-declared
ordinary value-only nominal structs after the H is fully completed. For this
bounded profile, the declarations below are the exact source example;
they are **NOT new compiler-builtin owner classes**:

~~~newlang
struct LiveRoot {
    ptr: ptr<Node>,
    allocation: Allocation,
    domain: LifetimeDomain,
}
struct TreeFour {
    root: LiveRoot,
    first: LiveRoot,
    middle: LiveRoot,
    last: LiveRoot,
}
struct TreeThree {
    root: LiveRoot,
    first: LiveRoot,
    last: LiveRoot,
}
struct TreeTwo {
    root: LiveRoot,
    child: LiveRoot,
}
struct DetachResult {
    donor: TreeThree,
    detached: LiveRoot,
}
~~~

Proposed lexical declaration extension (no arbitrary general struct parser):
~~~text
owner_nominal :=
    'struct' fresh_type_name '{' closed_field (',' closed_field)* [','] '}'
closed_field := fresh_field_name ':' owner_component_type
owner_component_type :=
    'ptr<' completed_H '>' | 'Allocation' | 'LifetimeDomain'
    | previously_completed_acyclic_owner_nominal
~~~
Each definition must have all fixed fields, be complete and acyclic with
no generic instantiation, optional parts, self-referential by-value field,
unknown type or second H nominal. Nominal declarations do not themselves
assert that the fields form a matched original root. No source-level
owner identity parameter, magic Node/variable name, hidden runtime witness
or special syntax is attached to LiveRoot. General nonrecursive struct
declarations, privacy, visibility, generic owner values and future
receiver/method surface remain Deferred.

These values use **existing** §4.5 structural nonCopy/nonDiscardable,
§16.1 complete consuming destructure and §16.2 full-field construction.
Every original Allocation and LifetimeDomain is nonCopy/nonDiscardable;
ptr<Node> is Copy. No partial extraction of a nonCopy field, no implicit
destructor, no new local field projection or loan syntax.

### 3.2c.2 NONCIRCULAR origin inference, not a presumed Matched lemma

A correct owner grant must be **derived from actual checked source steps**.
For each independent successful existing try_allocate_one<Node> site i:
1. Some(OneBacking{allocation:a_i,raw:s_i}) supplies one fresh R_i
   with independent compiler-owned facts
   AOrigin(a_i)=R_i and FullRaw(s_i)=(R_i,full).
   If None, there is no R_i, a_i or s_i.
2. into_slot<Node>(s_i) consumes that SAME full Storage(R_i), yielding
   slot<Node>(R_i). It cannot mint a new region or preserve a second
   simultaneously owned full raw value.
3. d_i=lifetime_domain() gives a fresh original nonCopy D_i.
   loan_read(d_i){|stable_i|
      initialize(slot_i,Node{...},stable_i)
   } starts current original O_i inside R_i governed by D_i, returns
   p_i whose provenance/root incarnation is exactly (R_i,O_i,I_i).
   No copied ptr token or favorable callee requirement creates that fact;
   the original local stability loan ends before packet construction.
4. LiveRoot{ptr:p_i,allocation:a_i,domain:d_i} copies only p_i and consumes
   the ORIGINAL available a_i and d_i once. Its complete value preserves
   separately sourced original A(R_i), governing D_i and p_i/O_i/R_i
   identities as semantic value constituents (§13.5a/§18.6). The packet
   is a fresh LOCAL value incarnation; O_i is a separate HEAP root and
   remains live through nonCopy packet transfers.

For use by a matched-owner consumer, the compiler checks conceptually:

~~~text
GrantMatched(S; p,a,d) :=
  exists R,O,I,D:
      CurrentTypedRoot(S,O,R,I)
   && PtrOrigin(p) == (R,O,I)
   && CurrentAllocation(a) && OriginalAllocationBacking(a) == R
   && CurrentDomain(d) && DomainIdentity(d) == D
   && GoverningDomain(S,O) == D
   && RecoverableFullOriginalSlot(S,O,R)
   && UniqueCurrentNonCopyCarrier(S,a,d)
~~~

**This predicate is NOT a new user assertion, source expression, core
authority constructor, or a proof assumed from LiveRoot's nominal name.**
All equalities are obtained by actual allocator/result/storage/slot/root/domain
operations and ordinary nonCopy value flow (steps 1–4). In particular
the Allocation/BackingRegion equality is **independent** of the
p/current O/domain/access proof and MUST NOT be inferred merely from
ref_from_ptr or H field writes.

RecoverableFullOriginalSlot is the existing §3.4 full-range occupancy/
claim history: while O_i is typed live, no simultaneously current raw
Storage(R_i) is invented; later exact destroy→erase_slot can return
the SAME full Storage(R_i) to deallocate(a_i,raw_i). GrantMatched alone
does NOT grant EndRoot; at actual operations, current O liveness, correct
access, exact ptr incarnation, compatible exclusive domain ending loan,
no surviving semantic-dependency/borrow/occurrence conflicts and valid
whole raw recovery must also be checked.

Ordinary mismatched LiveRoot **construction, whole consume, move,
pure assembly and return** are not by themselves violations of the
language core. Ordinary records carry their ACTUAL nonCopy components,
not a promise that p/Allocation/Domain designate the same heap root.
A mixed packet may be legally passed, returned, completely destructured,
re-partitioned with other original nonCopy constituents, then safely
discharged. Affine uniqueness, loan/scope compatibility and nonDiscardable
normal-exit duties still apply. Such a mismatch fails only an actual
operational use requiring a matching original-root grant; a specific
cJSON H1 return witness with an unmatched B member fails H1 as a
PRODUCT condition, not as a general ordinary source/return violation.

**F #47 decisive negative:** original p_B and D_B, but original A_C
instead of A_B. B.prev write using p_B with a real D_B-scoped ref may
succeed; the write does not read Allocation. However R_B≠R_C,
OriginalAllocationBacking(A_C)=R_C and PtrOrigin(p_B).R=R_B.
Therefore GrantMatched(p_B,A_C,D_B) is false. At an actual original-B
release-using operation, or a named callee whose independently checked
body executes that release, the requiring predicate MUST reject on
original-Allocation inequality. Ordinary TreeTwo assembly and return
with the mixed packet are permitted if all regular affine/loan duties
are satisfied. Its B member is only H1 PRODUCT-negative if this is
the separately specified cJSON original-B caller witness.
Similarly (p_B,A_B,D_C) fails GoverningDomain(O_B)==D_C, a stale p_B
fails CurrentTypedRoot, and two simultaneous packets reusing A_B/D_B
fail §4/§18 nonCopy consumed-use. Numeric ptr equality never fixes
any mismatch. No wrong-Allocation error is credited merely because the
current 5-H parser cannot parse the new source.

### 3.2c.3 Minimal source/AST/call contract; LiveTail reuse and no new field loan

All new packet source constructors and whole destructuring use **existing**
§16.1/§16.2 semantics; all actual new source locations/declarations are
additively admitted only in this §3.2c profile. The source compiler must
retain exact origin and current-value facts in aggregate constituents.
A whole consume destroys only the old **LOCAL owner-value** incarnation,
and makes fresh local/formal/result incarnations; it does not EndRoot O_B.
A fresh complete re-pack after an ended borrow retains the original
p/O/R/A/D constituent identities and their TRUE or FALSE relationships;
it does not remint an allocation/domain or repair a mismatch.

**Three distinct judgments, with no new source-level marker:**

(I) **Ordinary whole-value custody / affine completeness.** Complete
construction, destructuring, nonCopy move, known-direct pure assembly,
return and subsequent local placement conserve exactly the actual
original Allocation and Domain constituents. Each current nonCopy
value has a single available carrier, former input placements become
Consumed, scoped ref dependencies are respected, and normal exits do
not drop nonDiscardable values. A pure ordinary record operation DOES
NOT require its copied ptr, Allocation and Domain to be a matched
root triple. A mixed return retains mixed origin facts, not a new grant.

(II) **Source-derived matched operational use.** At each ACTUAL
primitive access, or at a named call that actually uses the authority
in its checked body, infer only the needed body-sensitive conditions.
For a scoped H field read/write, require valid current p/O, governing D,
field access, loan and Change/Reset conditions; an unused Allocation
need not equal R. For root destroy, full original slot/raw recovery,
original Domain finalization and matched deallocate, require the
separate original AllocationBacking(a)==R, PtrOrigin(p)==current
O@R incarnation, Governs(O)==D and all full range/current-loan/
scope conditions. Only such demanding operations/calls must reject
Unknown matching. There is no unconditional matching requirement
at every call or return.

(III) **Specific cJSON H1 product witness.** For the one registered
five-original successful caller, after its named attach/assemble
RETURN, inspect whether its one CURRENT TreeTwo value actually
contains original K_dst AND original K_B, with no other current
B ownership carrier. Later finish_two must still independently
prove its actual B/dst release calls. An unmatched or unrelated
caller-held B result can fail this H1 product oracle without being
an illegal ordinary NewLang value. H2 remains a separate historical
actor test. The H1 observer supplies no authority to a source checker.

**P #276 ordinary pure return witness (source-level illustration):**
~~~newlang
fn assemble(first: LiveRoot, second: LiveRoot) -> TreeTwo {
    let combined = TreeTwo { root: first, child: second };
    return combined;
}
~~~
This definition creates NO matched-grant requirement on either packet:
the body only consumes and assembles actual nonCopy member values.
For an actual second packet with original (p_B,A_C,D_B) and genuine
first packet K_dst, the named assembly and complete return may be
core-safe under (I), EVEN THOUGH the returned B-like member is mixed.
A later consumer may whole-destructure and re-pair original A_C/A_B
with the real corresponding C/B ptrs/domains before legitimate matched
terminal operations. Dropping or duplicating any nonCopy original A/D
is never permitted. If this returned value is examined as the particular
cJSON H1 TreeB witness, its B member FAILS (III); if uncorrected B
is later passed to finish_root, THAT demanding call FAILS (II).

The existing §18.1a definition-time independently symbolic ordinary
known-direct inferred-requirement method is reused and extended from
separate two-H p/a/d parameters to complete source components of these
small acyclic nonCopy record values:
- At definition time, start with fresh, UNCORRELATED symbolic p/a/d
  formals and full aggregate constituents. Check each actual source BODY
  operation independently: ordinary whole assembly/move/return requires
  affine completeness (I); H field access requires current p/O/D/access
  and proper loan/Change/Reset (II); original lifetime-ending release
  needs the additional original R/Allocation/full-slot/Domain matches
  (II). Do NOT infer root-release equality merely from an owner-like
  nominal, pure whole result, unrelated ptr field or caller name.
  Source-defined consumers infer their conditional requirements ONLY
  from the actual primitive or transitive known-callee uses in the body,
  not from assumed favorable caller relations. Reject unprovable body
  steps or nonDiscardable normal exits.
- At **every actual known-direct call**, check the ordinary nonCopy
  argument and all caller-current binding/member availability first (I).
  Substitute actual source original-provenance and current world facts
  for every operational condition inferred from the independently
  checked callee BODY. Prove ONLY the needed p/O/D field-access and/or
  full original p/O/R/A/D terminal matches (II), plus relevant loans,
  Change/Reset, aliases, complete full range and consuming effects
  under §13.5c/§18.7–8. There is NO unconditional
  AllocationBacking(a)==R check on a pure assembler, pure forwarder,
  or ordinary whole record return. Unknown rejects an UNSATISFIED
  REQUIRED operation precondition, not a mismatched but harmless value.
  No live ghost duplicate original A_B/D_B or conflicting active D_B
  loan may be overlooked just because the local call slots look valid.
- For **every ordinary complete result**, carry the exact original
  source constituent identities and matching OR mismatching relations
  unmodified; consume old parameter/input/formal/result binding places
  exactly once and forbid escaping scoped refs. Do not make any
  general matched-result guarantee for LiveRoot, TreeTwo or similar
  nominal types. Only the specific cJSON original-five caller's H1
  product assessment separately checks whether returned TreeTwo
  contains genuine K_dst and K_B (III); its failure is not an extra
  general semantic error. Each later release-using named terminal
  must separately satisfy its body-derived requirements (II).
  Checked semantic artifact tracks source origins, nonCopy consume
  edges, nested member paths, inherited conditional use-requirements,
  current result places, loan/effect state and actual original
  carrier uniqueness. Backend cannot mint Allocation authority
  from the Copy dst.child or record spelling.

**Cheapest stable domain source:** do not add loan_read(ticket@domain),
ticket@ptr, arbitrary field borrowing or ref-to-field owner grant.
Whole-destructure a LiveRoot into its three existing ordinary local
values. Use already selected loan_read(domain){|stable|...} and
ref_from_ptr(write,ptr,stable), end this lexical loan and all derived
H refs, THEN whole-reconstruct LiveRoot with the SAME original
allocation/domain and copied ptr. Moving the domain back while its loan
is active is rejected under §14.5 transfer/dependency blocking.
A DomainLive fact alone is NOT an owner-field-place loan: F #47's
DomainLive-only transfer counterexample must not be reintroduced.

**Reuse of LiveTail:** §18.1b already has compiler-known TWO-H
LiveTail{owned_ptr,owned_allocation,owned_domain}, its producer's
source-derived correlation and named-return transfer, and §18.1c
caller-owned Option. This new general source-declared LiveRoot
uses the **same three-component value and actual-call proof method**;
it does not generalize the compiler-known LiveTail producer nor grant
an arbitrary constructor trusted authority, and does not change
§18.1c's nonDiscardable-None/known-None exception. Avoid an additional
privileged DetachedHolder/Owner/TreeD syntax. Why a new ordinary
value nominal instead of renaming the registered LiveTail: the existing
LiveTail SOURCE constructor is intentionally closed to that two-H
producer; silently aliasing/generalizing it would overwrite accepted
bounded admission. The one additional ordinary record source mechanism
can carry the same original 3-component semantics without magic.

### 3.2c.4 Positive whole-value handoff with explicit local-domain scopes

The following functions are ordinary, non-generic, monomorphic, known-direct
SAME-UNIT source in the NEW bounded admission. They are not builtins and
their names do not certify matched ownership. Their definition bodies,
actual callers and effects must independently pass §3.2c.3.

~~~newlang
fn set_next(ticket: LiveRoot, value: Option<ptr<Node>>) -> LiveRoot {
    let LiveRoot { ptr, allocation, domain } = ticket;
    loan_read(domain) { |stable|
        let w = ref_from_ptr(write, ptr, stable);
        let old = replace(w@next, value);
        unit
    };
    LiveRoot { ptr: ptr, allocation: allocation, domain: domain }
}
fn set_prev(ticket: LiveRoot, value: Option<ptr<Node>>) -> LiveRoot {
    let LiveRoot { ptr, allocation, domain } = ticket;
    loan_read(domain) { |stable|
        let w = ref_from_ptr(write, ptr, stable);
        let old = replace(w@prev, value);
        unit
    };
    LiveRoot { ptr: ptr, allocation: allocation, domain: domain }
}
fn set_child(ticket: LiveRoot, value: Option<ptr<Node>>) -> LiveRoot {
    let LiveRoot { ptr, allocation, domain } = ticket;
    loan_read(domain) { |stable|
        let w = ref_from_ptr(write, ptr, stable);
        let old = replace(w@child, value);
        unit
    };
    LiveRoot { ptr: ptr, allocation: allocation, domain: domain }
}
fn detach_middle(tree: TreeFour, first_ptr: ptr<Node>,
                 last_ptr: ptr<Node>) -> DetachResult {
    let TreeFour { root, first, middle, last } = tree;
    let first_now = set_next(first, Option<ptr<Node>>::Some(last_ptr));
    let last_now = set_prev(last, Option<ptr<Node>>::Some(first_ptr));
    let b_unlinked_prev = set_prev(middle, Option<ptr<Node>>::None);
    let detached = set_next(b_unlinked_prev, Option<ptr<Node>>::None);
    let donor = TreeThree { root: root, first: first_now, last: last_now };
    DetachResult { donor: donor, detached: detached }
}
fn attach_whole(receiver: LiveRoot, detached: LiveRoot,
                b_ptr: ptr<Node>) -> TreeTwo {
    let new_dst = set_child(receiver, Option<ptr<Node>>::Some(b_ptr));
    let new_b = set_prev(detached, Option<ptr<Node>>::Some(b_ptr));
    TreeTwo { root: new_dst, child: new_b }
}
fn finish_root(ticket: LiveRoot) -> unit {
    let LiveRoot { ptr, allocation, domain } = ticket;
    let vacant = loan_exclusive_read(domain) { |ending|
        destroy(ptr, ending)
    };
    let raw = erase_slot<Node>(vacant);
    finalize_domain(domain);
    deallocate(allocation, raw);
    unit
}
fn finish_three(t: TreeThree) -> unit {
    let TreeThree { root, first, last } = t;
    finish_root(first);
    finish_root(last);
    finish_root(root);
    unit
}
fn finish_two(t: TreeTwo) -> unit {
    let TreeTwo { root, child } = t;
    finish_root(child);
    finish_root(root);
    unit
}
fn finish_four(t: TreeFour) -> unit {
    let TreeFour { root, first, middle, last } = t;
    finish_root(middle);
    finish_root(first);
    finish_root(last);
    finish_root(root);
    unit
}
~~~

The intermediate old Option<ptr<Node>> values are Copy/Discardable;
this is NOT old Option<LiveTail> which remains nonDiscardable even at None.
All domain loans inside helpers end before the LiveRoot re-pack. At
entry and exit of every known call, actual p/O/R/A/D constituent
identities (including any mismatch), nonCopy/current-carrier state
and no-escaping-loan obligations are preserved. Exact original
p/O/R/Allocation/Domain matching is demanded only for operations
or transitive named-callee requirements that actually use the
corresponding authority; the specific H1 original-B result identity
is a separate product witness. This bounded example
uses 3 reusable field helpers rather than a NEW generic field-loan syntax;
it does increase function/source ceremony, to be measured before product PASS.
The ordinary Copy first_ptr/last_ptr/b_ptr actuals must be source-proved
equal to the original first(A)/last(C)/B ticket ptr origin to satisfy
the cJSON topology oracle; names alone do not prove list membership.
Field link graph correctness is a product/library condition, not a new
universal memory safety axiom.

After the original §3.2b five physical successful OneBacking/init sites,
six initial link writes and closure of all original loan scopes, this
candidate source continuation applies; all p names are the already
source-issued original Copy ptrs, all Allocation/domain variables retain
their independent, still-current original nonCopy values:

~~~newlang
let donor_a = TreeFour {
    root: LiveRoot { ptr: ptr_src, allocation: allocation_src, domain: life_src },
    first: LiveRoot { ptr: ptr_A, allocation: allocation_A, domain: life_A },
    middle: LiveRoot { ptr: ptr_B, allocation: allocation_B, domain: life_B },
    last: LiveRoot { ptr: ptr_C, allocation: allocation_C, domain: life_C }
};
let receiver_empty = LiveRoot {
    ptr: ptr_dst, allocation: allocation_dst, domain: life_dst
};
let split = detach_middle(donor_a, ptr_A, ptr_C);
let DetachResult { donor, detached } = split;
let receiver_b = attach_whole(receiver_empty, detached, ptr_B);
finish_three(donor);        // actual A,C,src original frees
finish_two(receiver_b);    // original B,dst after A EndRoot
~~~

For the **specific genuine five-original H1 positive caller above**,
the intended product condition at the named attach RETURN is:
**one current WHOLE TreeTwo** has root=original K_dst and
child=original K_B. An ordinary attach call that returns a
mixed TreeTwo is still core-legal where its body only changes
properly permitted Copy H links and moves nonCopy components;
it does not automatically fulfill H1. In this positive caller,
donor TreeThree holds only src,A,C; the old donor_b/receiver_empty/
detached actuals/formals are Consumed
and no active D/H ref escapes. World still has original live B/dst;
there is no new B malloc or sixth H. finish_three frees original
A(#2),C(#4),src(#1) first; finish_two consumes TreeTwo alone to
destroy→erase_slot→finalize_domain→deallocate original B(#3) and
original dst(#5), each exactly once. The old TWO-H terminal
headLive(A) is still FALSE after A is freed: finish_two is a **new
independently checked source known-call** that requires only current
original B/dst matched grants, no A. F #47's accepted dst/B receiver
view and physical cleanup DO NOT establish this actual-source call
refinement; its proof remains an explicit Gate E blocker.

### 3.2c.5 Original 0–5 worlds, refusal and semantic negative oracles

Exactly existing five physical fallible sites in order src→A→B→C→dst.
0 successes→no frees, 1→src, 2→A/src, 3→B/A/src,
4→C/B/A/src, 5 accepted→donor A,C,src then receiver B,dst.
All are original allocations; no extra staging source allocation.

Before detach rejection: TreeFour still holds B, receiver_empty holds dst;
finish_four + finish_root(dst) or exact nonCopy whole refund.
After detach, before attach rejection: complete returned TreeThree +
LiveRoot(B) + LiveRoot(dst) remain to be separately finished or moved.
After attach in the **specific H1-accepted caller**: TreeTwo alone
contains both genuine original K_dst and K_B and must be finished
or whole-moved before normal exit. Other ordinary complete return
values may have mismatched constituents and require later complete
repartitioning before any matched terminal; they still cannot
silently drop nonCopy components at normal exit.
This **straight-line known direct attach** has no callbacks,
allocation, FFI, unwind, exception, fallible step, early error/false
edge or unknown effect after entry checks; both safe Copy-link replace
operations are total when applicability is checked. Returning bool
false after consuming the owners is invalid. A future fallible attach
requires complete typed refund with both ORIGINAL nonCopy grants and
consistent graph rollback, or committed TreeTwo; not added here.
§26/§27 normal joins cannot abandon a nonDiscardable owner.

Required source-eligible semantic-negative oracles:
- original p_B/D_B + A_C → ACCEPT an otherwise legal ordinary
  LiveRoot/TreeTwo constructor, move and pure assembly/return;
  REJECT an actual B matched-release operation or the independently
  checked requiring finish_root call, by original AllocationBacking
  mismatch, even though a proper D_B-scoped B.prev H write may succeed;
  if observed at the specific H1 result, report product H1 NO,
  NOT ordinary source rejection;
- p_B/A_B + D_C → ordinary mixed value custody still possible;
  reject the actual ref access or matched terminal needing
  Governs(O_B)==D_C, not a pure record move/return;
- stale original p_B O/incarnation → stale Copy locator may exist;
  reject actual reloan/root-ending requiring current typed root,
  not ordinary copying of the ptr token;
- duplicate original B nonCopy A_B or D_B in two packet values →
  reject consumed-use; a local call-entry subset is insufficient,
  check all actual current bindings and current loan scopes;
- a surviving loan_read(domain_B) or H derived ref while moving/repacking
  D_B → dependency conflict; DomainLive alone is NOT the blocker;
- reuse of consumed original TreeFour/detached after named handoff →
  reject availability; stale Some occurrence across Change/Reset →
  reject §13/§26 dependency;
- attempted old A-head terminal after original A EndRoot → reject
  exact known-call applicability; use new separately checked B/dst terminal;
- unknown matched origin interpreted as success, ignored nonDiscardable
  packet on a refusal/error edge or double free → reject;
- TreeB Copy child pointer plus original B packet STILL in unrelated
  caller variable is **H0 product failure only**, not an intrinsically
  unsafe v0 program. Caller-held genuine B can safely whole-move or
  explicitly matched destroy/full raw/finalize/deallocate if it meets
  existing v0 lifetime/loan rules. Copy dangling link must not be
  safely reborrowed and can violate cJSON library observable policy.
  Historical H2 independent actor stress is distinct and unchanged.

No negative which the current Draft17.30 parser merely cannot express
may be credited as a future B1–B7 semantic-safety rejection.
B8 ownership-vs-reference policy is independently assessed.

### 3.2c.6 §4.2 historical Gate A–D and hard independent Gate E blockers

| Evidence class / old intent | Decision for proposed source | No-foreclosure / reason |
| --- | --- | --- |
| Current Draft17.30 §§3,10–14,16–19,26–27 + DI-009/010/014 | KEEP core/backing/slot/domain/ptr/Change/Reset/whole nonCopy/five-original 0–5 | Only add source admission, never synthetic original authority or changed root semantics |
| Draft17.29 §18.1a–c / DI-011/012/013 | KEEP existing TWO-H conditional known-call, LiveTail and caller Option exact semantics; INTENTIONALLY extend proof method to five-source original packet constituents | No arbitrary LiveTail constructor, no reinterpretation of known-None Option, no old A-head terminal generalization |
| DI-001/002/004/005/006/007; older nonnormative Surface Draft1/1.1 and M0 | KEEP @ vs :: vs . split and no implicit borrow; DEFER general field/ptr projection, receiver/module/visibility/generic primary and unified loan syntax | Whole-destructure + local lifetime_domain loan is sufficient within this proposal |
| DI-008 and §16.1/16.2 | KEEP complete aggregate consume/repack | No partial-live aggregate, hidden Drop or invented generic tuple return |
| M #270/#271 source research, F #43/#47 and human-merged noncanonical formal PR #48 (FormalProof/main 08c8b8da4b9dbe5e125e4be0643bffb28294bfad) | PROPOSED source / formal HOLD | The current rich source refinement is NOT established by Lean ticket projection; do not count full source or product PASS |
| Generic dynamic TreeOwner, Owner<T> runtime registry, FFI, GC/RAII, 6th H | DEFER | Avoid broad core change, ABI lock-in and spurious human/product value claims |

**Genuine CORE rule change: NONE proposed.** Issue #279
**INTENTIONALLY NARROWS** matched-original-root tests to actual
release-using operations and transitive requiring named calls:
ordinary nonCopy custody and complete return must preserve actual
even mismatched source constituents without imposing matching.
The third, specific H1 TreeTwo returned-member test is a
product oracle and adds no universal type-based owner restriction.
Nontrivial bounded source/AST obligations remain: acyclic nominal
registration, original source provenance through whole returns,
conditional body-specific original R/A/O/D terminal inference,
global all-binding nonCopy availability and loan conflicts,
and the independent after-A two-member matched terminal.

**Bounded experimental evidence, NOT source selection:** P #274/#276,
draft experimental PRs #275/#277, independently accepted by Coordination,
show that a real mixed ordinary record can be source moved/returned,
including a repaired legal return, but genuine p_B/D_B + original
Allocation_C fails only at a later requiring finish_root(tail)
actual call with P193-CALL-BACKING. The P #276 38-source corpus
contains 11 semantic-accepted/backend-unsupported cases and 21
genuine semantic rejects. Two independent member-path conditional
requirements for source-defined finish_two(TreeTwo) remain
UNPROVED at P276-TERMINAL-SUMMARY-PRECISION; P #278 is separately
investigating this on an experimental branch. These are NOT native
C17, complete 4-detach/2-adopt, rich F source evaluator, full H1
product verification or a canonical Draft17.31 adoption.
The candidate remains **DRAFT / UNMERGED**.

**Hard Gate E BLOCK until independently checked**: preservation
of actual source OneBacking/initialize-derived p/O/R/A/D **identities
including a potentially nonmatching Allocation** through ordinary
aggregate/call/return (no fabricated equality); proving the
original same-R Allocation equality only at a **demanding
release-using** primitive/named call; current LiveRoot source-place
and old/current consume obligations across local domain loans,
external ghost duplicate holders/borrowers; TWO separately inferred
member-path terminal requirements carried transitively by
finish_two after original A died; exact 0..5 normal/refusal/Change/
Reset exits and checked artifact/backend handoff. No owner result
or product H1 witness may be used to presuppose its own proof.
If required operational facts cannot be derived from actual
source without matched/post-WF assumptions, fail the candidate,
not silently widen core.

Full frozen North Star remains: original pinned upstream cJSON,
node-per-physical allocation, ≥5/6 critical topology, 0 per-hop
unchecked, ≥5 actual semantic B1–B7 negative classes eventually,
B8 application policy, honest C/Zig/topology-preserving Rust
source/diagnostic/human L/R/P/V/C cost comparison. New nominal records
and extra helpers may add significant source burden; no benchmark,
LOC equality, ABI optimization or PASS is claimed. Parent gate #268
and full original-B production/native STOP remain.


## 3.3 Storage

`Storage` は、liveな BackingRegion 内の
**現在 raw である byte range に対する affine occupancy claim** である。

性質:

- non-Copy
- non-Discardable
- exactly one BackingRegion identity に属する
- backing range を保持する
- backing range length は **1以上** である
- overlap する occupancy claim を同時に保持してはならない
- allocation 全体のownerではない
- mixed live/raw allocation 全体を表す一般ownerではない

`Storage` の存在はBackingRegion自体のdeallocation authorityを表さない。

`Storage` は raw occupancy **claim value** であり、
その claim が authorize する backing bytes 自体は `Storage` value の field/subobject ではない。

`ref<read, Storage>` / `ref<write, Storage>` は ordinary ref と同じ **place capability** である。
`Storage` だけに特別な current-value stability semantics を与えない。

従って lifetime-preserving `replace` によりそのplaceのcurrent `Storage` valueが変わった後も、
live ordinary refは同じplaceを参照し、新current valueを見る。

一方、そのclaimがauthorizeするraw backing bytes自体は `Storage` value のfield/subobjectではないため、
`ref<read, Storage>` がliveでもauthorized raw backing bytesをread-onlyにはしない。

あるprevious `Storage` current valueのrange / identityに基づいて得たproofやderived factは、
そのcurrent valueが変化すれば既存§13.5a / §17.4のcurrent-value fact ruleに従ってinvalidateされ、
必要なoperation entryで再証明しなければならない。

### split

概念的に:

```text
split(storage, k) -> (Storage, Storage)
```

とする。

元`storage`のrangeを:

```text
R = [base, base + n)
```

とすると、preconditionは:

```text
0 < k
k < n
```

であり、result `left` / `right` はexactly:

```text
range(left)  = [base,     base + k)
range(right) = [base + k, base + n)

storage_len(left)  = k
storage_len(right) = n - k
```

でなければならない。

従って:

- 元claimをconsumeする
- 両resultは元claimと同じ BackingRegion identityに属する
- 両resultは互いにdisjointである
- `range(left) ∪ range(right) = R`
- 元rangeのbyte responsibilityを欠落・複製しない

ことを要求する。

v0の`Storage`はnon-emptyなので、`k == 0` / `k == n` はvalid splitではない。
empty claimを生成してendpoint splitを表現してはならない。

非消費的な `substorage()` のようなAPIでoverlap claimを作ってはならない。

### merge

概念的に:

```text
merge(a: Storage, b: Storage) -> Storage
```

は少なくとも:

- same BackingRegion identity
- disjoint
- adjacent ranges
- union が contiguous

をpreconditionとし、
二つのclaimをconsumeしてunion rangeの一つのraw claimを返す。

`split` / `merge` は BackingRegion identity を変更しない。

full-range Storageを再構築できれば、
matching `Allocation` とともにsafe `deallocate`へ渡せる。

## 3.4 slot<T>

`slot<T>` は、liveな BackingRegion 内の
`T` 用の **definitely-empty typed occupancy claim** である。

性質:

- affine
- non-Copy
- non-Discardable
- exactly one BackingRegion identity / range に属する
- live `T` ではない
- `T` のobject lifetimeはまだ始まっていない

### Storage -> slot

conceptual conversion:

```text
into_slot<T>(storage: Storage) -> slot<T>
```

は少なくとも:

- range length が **exactly `sizeof(T)`** である
- required alignmentを満たす
- rangeがrawである

ことを要求する。

元 `Storage` claimをconsumeし、同じBackingRegion/rangeのtyped empty claimを返す。

larger raw rangeから`slot<T>`を作る場合は、先に`split`等で
exact `sizeof(T)` rangeの`Storage` fragmentを作らなければならない。
`into_slot<T>` 自体はunused tail bytesを暗黙に残したり別claimへ分解したりしない。

### slot -> Storage

```text
erase_slot<T>(slot: slot<T>) -> Storage
```

はsafe total operationとする。

`slot<T>` はlive objectを含まないことが型/claim上で保証されているため、
同じBackingRegion/rangeのraw `Storage` へ戻せる。

概念的遷移:

```text
Storage
  -> slot<T>
  -> initialize
  -> live lifetime-root T
  -> take / destroy
  -> slot<T>
  -> Storage
```

## 3.5 object incarnation

同じ machine address が再利用されても、
object lifetime が終了し再開した場合は別の **object incarnation** である。

例:

```text
address A
  incarnation #1
  lifetime ends
  incarnation #2
```

#1 を指していた pointer は、#2 の開始によって復活しない。

object incarnation は一つのlive BackingRegion / range上に存在する。

### object/root placement backing relation

各live lifetime root incarnation `O` はplace/state-ownedな **placement backing relation** を持つ。

conceptually:

```text
Placement(O) = (BackingRegion identity R, occupied byte range B)
```

これは「現在そのobject incarnationがどのbacking bytesを占有しているか」を表す。
semantic value `T` 自体の一部ではなく、`T`を別placeへtransferしても一緒にはtransferされない。

fixed subobjectのplacementはenclosing root placement + structural layout relationから導かれる。
conditional subobject occurrenceのplacementもcurrent root/structural stateから導かれ、
occurrence identityと同様にsource placeからvalue packageへtransferされない。

root lifetime startではdestination storage/locationからfresh placement relationを作る。
root lifetime endではsource placement relationを終了する。

したがって:

```text
take / consume-out:
    source Placement(O) does not enter returned/transferred ValuePackage

initialize / fresh destination:
    destination gets its own Placement(O2)
```

である。

一方、`T` のsemantic value package内部に含まれる:

- `Storage` BackingRegion/range claim
- `Allocation` authority
- `ptr` provenance
- hidden backing dependency

等はvalue-owned stateであり、通常のvalue transfer ruleに従う。
**objectのplacement backing** と **value内部がcarryするbacking-related state** を混同してはならない。

BackingRegion lifetimeが終了すれば、そのregion上のlive objectは存在できない。
safe deallocation pathではfull-range `Storage` requirementにより、
その前にlive occupancyをすべてrawへ戻す必要がある。

### same-or-disjoint live `T` referent invariant

**Provisional**

safe native semanticsで、同じstatic storable type `T` の二つのcurrent live object/subobject referentを同時に考える。

それらは:

```text
same object/place

    or

distinct objects with disjoint occupied byte ranges
```

のいずれかである。

safe native operationは、distinctでありながらbyte rangeが部分的にoverlapする
二つのlive `T` object incarnationを生成しない。

これはordinary aggregateのparent/child subobject等について
「すべてのtyped object rangeが互いにdisjoint」であることを意味しない。
ancestor/descendant containmentは存在し得る。
この規則が直接対象とするのは、同じ`T`としてsafe typed operationへ渡される二つのreferentである。

C compatibility / FFI / localized `unchecked` boundaryがtyped object validityを主張する場合も、
その結果をsafe `ref<...,T>`として扱うためにはこのabstract-machine invariantを満たさなければならない。
falseな主張に基づいてsafe typed operationを実行した場合はNewLang UBである。

このinvariantにより、§17.4のtyped `swap` は
distinctnessのためのgeneral integer/range proofを要求せずに定義できる。

## 3.6 lifetime root と occupancy claim

**Provisional**

`slot<T>` が持つ affine claim は、
単なる「未初期化bit」ではなく、
その typed place が現在emptyであり、
次の `T` root incarnationを開始できることを表す。

```text
slot<T>
    -- initialize -->
live lifetime-root T
    -- take -->
(T, slot<T>)
```

`initialize` は `slot<T>` claimをconsumeし、
そのunique occupancy responsibilityをlive object stateへ移す。

`take` はそのlive root incarnationを終了し、
semantic value `T` とempty occupancy claim `slot<T>` に戻す。

このclaim conservationはabstract-machine semanticsであり、
runtime objectごとのoccupancy bitを要求しない。

### dynamic-region responsibility conservation

Draft 17.3では、dynamic containerが個々の`Storage` / `slot<T>` tokenをsource-visibleに常時保持しなくても、
**責任そのものが消えたことにはならない**とする。

container / runtimeはconceptually、あるBackingRegion/root originに結び付いたuniqueなdynamic-region responsibility ownerの内部へ、
複数rangeのvacant/live responsibilityを封じてよい。
この内部partitionをcompilerがarbitrary dynamic initialized-index setとして展開・追跡することは要求しない。

steady-state metadataは、例えば:

```text
Vec: len
Ring: head, len
Hash table: bucket/control state
allocator: block/free metadata
```

によって現在のoperational occupancyを記述してよい。
ただし:

```text
metadata state != occupancy/lifetime authority
```

である。

metadataの変更だけで:

- vacant responsibilityを新しく作る
- live non-Discardable responsibilityを消す
- overlapping responsibilityを二重に作る
- full-range `Storage` authorityを復元する

ことはできない。

selected rangeのclaimをregion外へ出すprivileged transitionは、
region ownerが既に保持しているresponsibilityの一部をexactly once外へtransferする。
claimがoutstandingな間、region ownerはそのsame responsibilityを保有しているものとして重複利用してはならない。
claim return / `initialize` / `take` / `erase_slot`等のtransition後には、
対応するpost-state responsibilityをexactly once region ownerまたは別の明示的consumerへtransferする。

この規則はlinear receipt/tokenをsource-level core typeとして要求しない。
実装はscoped receipt、nonescaping helper、library-private authority value等を使用してよいが、
そのobservable semanticsはroot/origin-bound responsibility conservationを満たさなければならない。

### lifetime root

**lifetime root** は、enclosing live objectのlifetimeを壊さずに
自身のlifetimeを独立に終了できるobject incarnationである。

典型例:

- lexical local のcomplete object
- complete `slot<T>` から `initialize` されたobject
- container backing region中で、独立slotから個別にlifetime開始されたelement

lifetime rootではない典型例:

- live ordinary struct のfixed field
- live native array のelement
- enclosing rootの一部としてlifetimeを持つfixed subobject

fixed subobject のlifetimeは原則としてenclosing root lifetimeに従う。

したがってlive `Pair` の `pair.a` だけを `take` して
`Pair` をpartially-live stateにしてはならない。

root statusは `ptr<T>` のsource-level type parameterには含めない。
compilerが証明できない場合、root requirementを持つoperationには
`unchecked` が必要になり得る。
preconditionが実際にはfalseならNewLang UBである。

projected field ptr等についてcompilerがnon-rootであることを知っている場合は、
safe `take` / `destroy` を拒否する。

## 3.7 backing dependency

occupancy claim / live object は、そのbytesを提供する BackingRegion がliveであることを前提とする。

explicit `Allocation`-backed regionでは、
`Allocation` valueのsource locationではなく BackingRegion identity に関係付ける。

したがって `Allocation` authority 自体を別bindingへtransferしても、
既存の `Storage` / `slot` / live objectは有効なままである。

一方、compiler-managed lexical backingでは、
そのbacking extentをcompilerが知る。

そのregionから派生した:

- `Storage`
- `slot<T>`
- backing lifetime自体を要求するその他のaffine claim

をbacking extentの外へescapeさせてはならない。

このdependencyはsource-level lifetime parameterを要求せず、
hidden backing-scope dependencyとして追跡してよい。

`ptr<T>` は例外であり、backing extentを越えて保持できる。
BackingRegion終了後はdangling tokenになるだけであり、
safe dereference authorityにはならない。

scope-bound `ref` は従来どおりobject/lifetime stability ruleに従うため、
backing終了後までescapeできない。

## 3.8 fixed subobject incarnation

fixed-shape aggregate rootがliveである間、
そのfixed field / fixed native-array elementは
enclosing rootに従うsubobject incarnationを持つ。

lifetime-preserving whole-value `replace` / `store` は、
enclosing root incarnationとfixed subobject incarnationを維持したまま
current semantic valuesだけを更新する。

payload sum typeのconditional subobjectはこのruleの対象外とし、§26で別途定義する。

---

# 4. 型と値

**Provisional**

## 4.1 compile-time value properties

v0 では各型について少なくとも次の二つの compiler-known property を持つ。

```text
Copy:        yes / no
Discardable: yes / no
```

これらは trait / interface / runtime flag ではない。

compiler が type checking / availability analysis / operation applicability のために
compile-time に保持・計算する static metadata であり、
runtime object representation に property bit を埋め込まない。

したがって property metadata 自体について:

- runtime memory overhead: 0
- runtime CPU overhead: 0
- dynamic property check: なし

とする。

Copy value の実際の複製に必要な runtime cost は value representation と optimization に依存し、
property metadata の runtime costとは別である。

v0 では次の combination だけを許す。

| Copy | Discardable | 意味 |
|---|---|---|
| yes | yes | copy 可能、暗黙 discard 可能 |
| no | yes | affine、暗黙 discard 可能 |
| no | no | affine、normal exit までに明示的 consume / transfer が必要 |

次は許さない。

```text
Copy = yes
Discardable = no
```

すなわち invariant:

> **Copy => Discardable**

を持つ。

## 4.2 Copy

Copy な型の binding を value-use すると、
元 binding を Available のまま保持して value を複製する。

non-Copy な型の binding を value-useすると、
元 binding を consume して destination へ ownership / value responsibility を transfer する。

source-level explicit `move` keyword は v0 に存在しない。

## 4.3 Discardable

Discardable な値は、Available のまま normal scope exit に到達してもよい。

non-Discardable な non-Copy binding は、
すべての normal exit path で Available のまま残ってはならない。

normal exit までに、その値を:

- 別の value destination へ transfer
- consumer operation へ transfer
- whole-value consuming destructure
- explicit terminating / consuming operation

のいずれかにより consume する必要がある。

implicit destructor / automatic cleanup は存在しない。

`non-Discardable` は:

> correct resource cleanup protocol が実行されたこと

を証明する property ではない。

保証するのは:

> 値が normal control flow 上で暗黙に失われないこと

までである。

## 4.4 core type properties

v0 の core types は少なくとも次の property を持つ。

| type | Copy | Discardable |
|---|---:|---:|
| integer / `bool` / `byte` / `char` / `unit` | yes | yes |
| `addr` / `uintptr` | yes | yes |
| `ptr<T>` | yes | yes |
| `ref<read,T>` | yes | yes |
| `ref<write,T>` | yes | yes |
| `span<read,T>` | yes | yes |
| `span<write,T>` | yes | yes |
| `exclusive ref<...,T>` | no | yes |
| `Allocation` | no | no |
| `Storage` | no | no |
| `slot<T>` | no | no |
| `LifetimeDomain` | no | no |

ordinary ref / span は scope-bound capability だが Copy である。
copy された ref / span は同じ underlying authority と同じ scope dependency を共有する。

元ref / spanがsemantic dependencyを持つ場合、
copyされたref / spanも同じsemantic dependency descriptorを持つ。

`ref<write,T>` / `span<write,T>` は exclusive ではないため、Copy によって alias が増えても semantics と矛盾しない。

exclusive ref は duplicate できないため non-Copy。
ただし単に scope を終了して exclusive capability を使わないこと自体は許されるため Discardable とする。

`Allocation`, `Storage`, `slot<T>`, `LifetimeDomain` は
backing / occupancy / lifetime に関するaffine authorityまたはclaimを保持するため
non-Copy + non-Discardable とする。

## 4.5 structural derivation

ordinary aggregate type の既定 property は constituent value types から structural に導出する。

概念的に:

```text
Copy(S)
    = every constituent value type is Copy

Discardable(S)
    = every constituent value type is Discardable
```

例:

```text
struct Pair<A, B> {
    a: A
    b: B
}
```

なら:

```text
Copy(Pair<A,B>)
    = Copy(A) && Copy(B)

Discardable(Pair<A,B>)
    = Discardable(A) && Discardable(B)
```

native array についても概念的に:

```text
Copy(Array<T,N>)        = Copy(T)
Discardable(Array<T,N>) = Discardable(T)
```

とする。

将来 payload sum type を導入する場合も、
property はすべての possible payload constituent に対する conservative structural derivationを基本とする。
variant-sensitive runtime property tracking は行わない。

Draft 17.21の§16.3 bounded nominal headerは、completion前にはaggregate value typeとしての
`Copy(Header)` / `Discardable(Header)` queryを成立させない。
field set確定後にのみ本節のstructural derivationを行う。

ただし `ptr<Header>` のCopy / Discardableは§4.4の **ptr constructor自身のproperty** であり、
target nominalのCopy / Discardableを問い合わせない。
従って§16.3で許可するincomplete header targetから `ptr<Header>` typeを形成するために、
Headerのaggregate property completionを先に要求しない。

## 4.6 nominal property restriction

type definition は structural property から **能力を減らす方向だけ** restriction を追加できる。

概念構文:

```text
noncopy struct UniqueHandle {
    raw: u32
}

nondiscardable struct FileOwner {
    fd: i32
}
```

surface syntax は Provisional。

規則:

- `noncopy` は Copy を false にする。
- `nondiscardable` は Discardable を false にする。
- `Copy => Discardable` により `nondiscardable` type は自動的に non-Copy。
- field / constituent が non-Copy なのに nominal declaration で Copy を true にする positive override はない。
- field / constituent が non-Discardable なのに nominal declaration で Discardable を true にする positive override はない。
- custom Copy implementation / custom discard implementation は v0 に入れない。

conceptually:

```text
Discardable(S)
    = all_constituents_discardable
      && !declared_nondiscardable

Copy(S)
    = all_constituents_copy
      && !declared_noncopy
      && Discardable(S)
```

property restriction 自体は runtime representation を変更しない。

## 4.7 value-use

ordinary expression で binding を value として使用する場合:

- static type が Copy なら copy
- static type が non-Copy なら binding を consumeし、valueをdestinationへtransfer

とする。

例:

```text
let y = x
f(x)
return x;
break x;
continue(x);
```

は同じ value-use rule に従う。

generic `T` は Copy と仮定できないため、
generic definition-time checking では `T` の value-use を consuming use として扱う。

concrete instantiation が Copy 型であっても、
generic source program の availability requirement を後から緩めない。

## 4.8 generic property expressions

generic aggregate は property expression を保持してよい。

例:

```text
Pair<T,U>
```

について:

```text
Copy        = Copy(T) && Copy(U)
Discardable = Discardable(T) && Discardable(U)
```

concrete instantiation 時には concrete arguments から property を計算できる。

ただし generic body の definition-time checking では
unconstrained generic parameter の Copy / Discardable を仮定しない。

v0 は `T: Copy` / `T: Discardable` constraint syntax を導入しない。

## 4.9 unit

`unit` は payload を持たない ordinary singleton type / value である。

source spellingのexact ruleとして、`unit` は **distinguished core spelling** とする。

- type positionの `unit` はcore unit typeを表す。
- value expression positionの `unit` はcore singleton valueを表す。
- `unit` はordinary lexical lookupでshadowされるpredeclared bindingではない。
- ordinary lexical namespaceへexact spelling `unit` のnameを導入してはならない。
- 従ってordinary lexical binding / declarationは `unit` をshadowできない。

これはgeneral keyword-system redesignではない。
lexerは `unit` を他のwordと同じtoken classで表現してよく、
restrictionはsource-name admissibility / name resolutionで適用してよい。

§21.8の `fn` / `let` / `return` / `match` / `if` / `else` / `loop` / `continue` / `break` structural reserved setとはcategoryが異なる。
それらはsource grammarの骨格を安定化するためordinary lexical namespaceから予約されるのに対し、
`unit` はcore singleton type/value identityを直接表すdistinguished spellingとして予約される。

field name / variant name等、ordinary lexical bindingを導入しないnominal/member namespaceのspellingまで
本ruleだけで一律に予約しない。
ただし、そのfield/payloadを受けるsource formが同名のfresh ordinary local bindingを導入する場合、
そのreceiver binding nameとしての `unit` は不受理となる。

「結果として情報を返さない」function / block / branch も `unit` を返すものとして扱う。

v0 では special `void` value type を必要としない。

---

# 5. 数値型

## 5.1 数値型family

v0 では少なくとも以下を持つ。

```text
i8 i16 i32 i64
u8 u16 u32 u64
m8 m16 m32 m64

usize
uintptr
```

- `iN`: two's complement signed integer
- `uN`: fixed-width non-negative scalar integer
- `mN`: true modulo `2^N` integer
- `usize`: target-sized non-negative **quantity scalar**
- `uintptr`: target-defined **numeric machine-address coordinate**

`usize` は少なくとも:

```text
size
length
count
index
byte offset
alignment
```

を表すためのcore quantity typeである。

`uintptr` は ordinary `addr` valueをlosslessにnumeric representationへ移すための型であり、
authority / provenance / BackingRegion identityを持たない。

`usize` と `uintptr` は、runtime representation / bit widthが同一targetでもdistinct nominal typeとする。
`uintptr` を `uN` / `usize` と同じgeneral-purpose unsigned integer familyとして扱わない。

`bool`, `byte`, `char` はこれらの数値型と別型とする。

## 5.1a `byte` = ordinary raw representation octet scalar

`byte` は integer family とは別のsemantic scalar typeであり、v0では:

```text
byte
    exactly 256 values
    Copy
    Discardable
    no arithmetic implied
```

とする。

v0 ordinary Storage target profileでは:

```text
one Storage byte = one 8-bit octet
```

とし、`byte` はその一つのoctet valueをsemantic valueとして表す。

従ってordinary storable `byte`について:

```text
sizeof(byte) == usize(1)
alignof(byte) == usize(1)
```

とする。

これはCのtarget-dependent `CHAR_BIT`をcoreへ輸入する規則ではない。
v0 native ordinary-memory profile自身をoctet-addressed memoryに限定する決定である。
future non-octet-addressed target supportはtarget-model extensionまたは別revisionで検討する。

`byte` valueはraw backing location、Storage claim、typed-object lifetime、ptr provenanceを表さない。
raw representation stateから`byte` valueを得る条件は§24で定義する。

## 5.2 ordinary scalar / quantity arithmetic

`iN` / `uN` の通常算術、および`usize`のquantity arithmeticは、
結果がその型の表現可能範囲に入ることを precondition とする。

precondition をコンパイラが証明できなければ compile error。
programmer が保証できる場合は `unchecked` を使用できる。

`usize` は v0 で少なくとも:

```text
usize + usize -> usize
usize - usize -> usize
usize * usize -> usize
usize / usize -> usize
usize % usize -> usize

usize == usize -> bool
usize != usize -> bool
usize <  usize -> bool
usize <= usize -> bool
usize >  usize -> bool
usize >= usize -> bool
```

を持つ。

`+` / `-` / `*` は数学的結果が`usize`で表現可能であることを要求する。
`/` / `%` は除数が0でないことを要求する。

shift / bitwise operationは`usize`のv0 core requirementとしない。

`uintptr`のbuilt-in algebraはこのordinary scalar ruleを継承せず、§9で別途定義する。

## 5.3 modular arithmetic

`mN` は v0 では少なくとも以下を total operation とする。

- `+`
- `-`
- unary `-`
- `*`
- `==`
- `!=`

division / remainder / ordering / shift / bitwise / rotation は v0 では原則として追加しない。

## 5.4 division / remainder

signed division:

```text
q = trunc_toward_zero(a / b)
```

remainder:

```text
a = q * b + r
|r| < |b|
```

`r` の符号は dividend に従う。

preconditions:

- division: `b != 0` かつ `!(a == MIN && b == -1)`
- remainder: `b != 0`

`MIN % -1 == 0` は defined とする。

unsigned `uN` および `usize` の division / remainder は:

```text
q = floor(a / b)
a = q * b + r
0 <= r < b
```

とし、preconditionは:

```text
b != 0
```

である。

## 5.5 shift

v0 の shift は `uN` のみに提供する。

- `uN << n`: high bits discard
- `uN >> n`: zero fill
- precondition: `n < N`

signed shift は v0 では提供しない。

`usize` / `uintptr` のshiftはDraft 17.2 closureでは提供しない。

## 5.6 conversion と reinterpretation

### value conversion

```text
T(x)
```

は数学的な値を保つ変換であり、`x` が `T` で表現可能であることを precondition とする。

`usize` / `uintptr` と他の数値型の間も、
ordinary value conversion `T(x)` を使用できる。
変換元の数学的値がdestination typeで表現可能であることがpreconditionである。

このexplicit value conversionは、`uintptr`でv0 coreが直接提供しないunusual numeric manipulationを
fixed-width `uN` 側で行うescape hatchとして使用してよい。
ただしinteger value conversionはptr provenance / Storage / BackingRegion identityを生成しない。

### `byte` / `u8` conversion

`byte` はinteger familyではないが、portable codecのために`u8`とのexplicit total value conversionを持つ。

```text
u8(b: byte) -> u8
byte(x: u8) -> byte
```

両conversionはbijectionであり、全256 valuesに対してtotalである。

conceptual source spellingはordinary explicit conversion formを使用する。

```text
let n = u8(b)
let b2 = byte(n)
```

このruleはimplicit numeric conversion、candidate ranking、bit reinterpretation、raw-memory accessを追加しない。
`byte`自体にarithmeticを追加するものでもない。

### bit reinterpretation

```text
reinterpret_bits<T>(x)
```

は same-width integer 間の bit pattern reinterpretation に限定する。

v0 では general transmute として使用しない。

---

# 6. 整数 literal

v0 では bare integer literal を expression としない。

例:

```text
i32(-2)
u8(255)
usize(3)
m32(10)
```

literal は数学的整数として解釈し、target type range に対して range-check する。

wrap / saturate / modulo は行わない。

`i32(-2)` の `-2` は「unary minus applied to 2」ではなく、typed literal construction として扱ってよい。
これにより signed minimum も自然に表現できる。

---

# 7. evaluation order

## 7.1 原則

value computation は source order で left-to-right に完全評価する。

```text
f(expr1, expr2, expr3)
```

は概念的に、

```text
v1 = eval(expr1)
v2 = eval(expr2)
v3 = eval(expr3)
call f(v1, v2, v3)
```

である。

各 operand の side effect / consume / ownership transfer / state transition は次 operand の評価より前に完了する。
## 7.2 memory value update

ordinary binding への reassignment は存在しない。

live object への memory value update operation は概念的に:

1. destination location / ref を評価
2. RHS value を評価
3. old semantic value を適切に処理
4. new semantic value を store

の順序とする。

old value を返さず失う `store` / overwrite と、
old value を caller へ返す `replace` の適用条件は §17.4 で定義する。

二つのlive place間のtyped semantic value exchangeである `swap` も§17.4で定義する。
`swap` は二つのdestination refをsource orderで評価した後、
observableなempty stateを作らない一つのsemantic transitionとして扱う。

## 7.3 composite expression

field / element は source order で評価する。

## 7.4 lazy control operation

`&&`, `||`, `if` は lazy / conditional operation とする。

`++` / `--` は v0 に入れない。

---

# 8. precondition / unchecked / assert

## 8.1 ordinary operation

operation は必要に応じて precondition を持つ。

programmer は control flow 等で precondition を成立させることを基本とする。
compiler が成立を確認できれば operation を使用できる。
確認できなければ compile error。

## 8.2 unchecked

compiler が precondition を確認できない場合でも、programmer が成立を保証できるなら `unchecked` により proof responsibility を引き受けられる。

`unchecked` operation の precondition violation は Debug / Release を問わず NewLang UB である。

Debug build では runtime で確認可能な unchecked precondition に diagnostic check を挿入してよい。
違反時は catch 不能な diagnostic trap で終了する。

Debug build がすべての UB を検出することは保証しない。

Release build では check を省略でき、compiler は precondition 成立を optimization に利用してよい。

## 8.3 total checked API

予想される失敗は `Result`, `Option` 等の値として扱う。

`try_*` は operation を totalize し、failure を値として扱いたい場合に用いる。

## 8.4 assert

`assert(P)` は、

> programmer がこの地点で P が真であると宣言する

operation である。

P が false なら contract violation / NewLang UB である。

実行モード:

- `contracts=check`: P を評価し、false なら diagnostic trap
- `contracts=assume`: runtime check を省略でき、compiler は P を仮定する

assert expression は observable side effect を持ってはならない。

assert 自身も通常 operation の precondition に従う。

### fact propagation

assert から得た fact は textual expression ではなく value / memory-location version に紐づく。

依存する mutation / call / alias effect が発生した場合、fact は conservative に invalidate される。

---

# 9. addr / uintptr

## 9.1 addr

`addr` は nominal machine address value である。

`addr` は authority / provenance を持たない。

> **Address is not authority.**

v0 で少なくとも:

- `==`
- `!=`

を許可する。

`addr` 自体のordering / arithmeticは core primitive として必須にしない。

`addr -> uintptr` は explicit total lossless conversion とする。

`uintptr -> addr` は numeric address value だけを生成し、
provenance / authority / BackingRegion identityを生成しない。

## 9.2 uintptr = numeric address coordinate

`uintptr` はgeneral-purpose unsigned quantityではなく、
ordinary machine addressの **numeric coordinate** を表すnominal typeである。

v0では少なくとも:

```text
uintptr == uintptr -> bool
uintptr != uintptr -> bool

uintptr + usize -> uintptr
uintptr - usize -> uintptr

uintptr % usize -> usize
```

を持つ。

意味は:

```text
point + displacement -> point
point - displacement -> point
point mod period      -> quantity
```

である。

precondition:

```text
base + offset:
    mathematical resultがuintptrで表現可能

base - offset:
    offset <= base

base % period:
    period != 0
```

とする。

`uintptr % usize` のresultはperiod未満のquantityなので `usize` で返す。

Draft 17.2 v0 coreでは次を提供しない:

```text
uintptr + uintptr
uintptr - uintptr
uintptr * uintptr
uintptr / uintptr
uintptr / usize
uintptr shift
uintptr bitwise operations
```

特に`uintptr + uintptr`はordinary address-coordinate algebraに意味を持たない。

`uintptr - uintptr -> usize` のnumeric distanceもv0ではDeferredとする。
それはcommon BackingRegion / provenanceを証明せず、C pointer subtractionと同一視してはならないためである。

必要なlow-level codeは§5.6のexplicit value conversionで適切な`uN`へ移し、
ordinary integer arithmeticを明示的に使用できる。

## 9.3 ordinary-safe byte-coordinate coherence

ordinary-safe BackingRegion / Storageとしてcoreへ公開されるmemoryについて、
numeric address coordinateはbyte rangeとcoherentでなければならない。

Storage rangeのstart addressを `a` とし、
そのrange内のbyte displacement `k: usize` を考えると、
対応するbyte locationのnumeric coordinateはconceptually:

```text
uintptr(a) + k
```

である。

特にvalidな:

```text
(left, right) = split(storage, k)
```

について、split前start addressを `a0` とすると:

```text
uintptr(storage_addr(right))
    == uintptr(a0) + k
```

が成立する。

これはnumeric location relationであり、computed coordinateからStorage claimを生成する規則ではない。

ordinary alignmentについても、valid nonzero alignment `A: usize` に対して:

```text
aligned(a, A)
    iff
uintptr(a) % A == usize(0)
```

とする。

従って`alignof(T)`とallocator alignment計算は同じnumeric coordinate ruleへ従う。

このbyte-coordinate / alignment coherenceをordinary `addr` / `uintptr`として提供できないspecial backing/address modelは、
target/platform-specific extensionで扱う。

---

# 10. ptr<T>

`ptr<T>` は provenance-bearing typed persistent location token である。

abstract machine上では少なくとも:

- location / typed projection relation
- object incarnation relation
- originating BackingRegion identity / provenance

を必要な範囲で保持する。

性質:

- Copy
- Discardable
- persistent
- dangling になり得る
- misaligned / non-live location を指し得る
- dereference できない

Draft 17.21の§16.3 bounded recursive declarationでは、incomplete nominal header `H` を
**direct target type identity** として `ptr<H>` typeを形成してよい。
このtype formationは:

- `H` objectを作らない
- `ptr<H>` valueを作らない
- provenance / incarnation / backing relationを捏造しない
- future `H` incarnationへの予約を行わない
- `H` のlayoutやfield setを要求しない

というpure type-graph operationである。
ordinary ptr valueは従来どおりlive object/ref等のcanonical routeからのみ得られ、
safe ptr->refは本章のcurrent-liveness / provenance / stability ruleを満たさなければならない。

v0 では以下を提供しない。

- pointer arithmetic
- pointer subtraction
- pointer ordering
- one-past pointer
- `ptr<T>` 同士の semantic equality

address 比較が必要なら `addr(p)` を用いる。

## 10.1 ptr -> ref

`ptr<T>` から `ref` を生成する operation は、対象 object incarnation の current liveness と、ref の将来の lifetime stability の両方を必要とする。

少なくとも以下を precondition とする。

- valid provenance
- originating BackingRegion が現在live
- `ptr` が現在 live な `T` object incarnation を指す
- initialized / valid value representation
- required alignment
- sufficient range
- required read/write access
- 対象 incarnation がcurrent governing identity `D` に属する
- stability evidence が **同じ `D`** の継続を保証する
  - programmer-visible `LifetimeDomain` の場合はordinary domain refが同じ `D` を参照する
  - §13.7のimplicit local governing identityの場合はcompiler-managed hidden stability evidenceでよい
- 対象がconditional subobjectなら、そのspecific occurrenceがcurrentにliveであり、必要なsemantic dependencyを生成refへ付与できる
- 生成される `ref` のすべての use が、その stability evidence / semantic dependency の有効scope内にある

概念的には:

```text
ref_from_ptr(
    p: ptr<T>,
    stable: ref<read, LifetimeDomain>
) -> ref<read, T>
```

または write access を要求する対応 operation を想定する。

current liveness、metadata と object lifetime の対応、provenance 等を compiler が証明できなければ `unchecked` を使用できる。

ただし `unchecked` は、必要な stability evidence 自体を省略する機構ではない。

つまり v0 では、概念的な:

```text
unchecked ref_from_ptr(p)
```

ではなく、

```text
unchecked ref_from_ptr(p, stable)
```

の形を要求する。

上記の二引数形は、programmer-visible `LifetimeDomain` を用いる場合の代表形である。

対象が§13.7のcompiler-managed implicit local governing identityに属することをcompilerが証明できる場合は、
そのlocalのimplicit lifetime-stability loanを、同じ意味の **hidden stability evidence** として用いてよい。
この場合、source programがmaterialized `LifetimeDomain` valueや
`ref<read,LifetimeDomain>` argumentを明示的に構築する必要はない。

ただしcompilerは少なくとも:

- ptrがそのcurrent live local incarnationを指す
- localのimplicit governing identityがcurrentである
- generated refの全useがそのlocal lifetime / hidden stability loan内に収まる

ことを証明しなければならない。

従って「`unchecked`でもstability evidenceを省略できない」という規則は、
**semantic evidence自体を省略できない**という意味であり、
§13.7のimplicit local evidenceまでprogrammer-visible argumentとしてmaterializeせよ、という意味ではない。

explicit domain-ref caseで生成された `ref` は `stable` に **scope-dependent** であり、
implicit-local caseでは対応するhidden stability loanにscope-dependentである。
いずれもdependency sourceより長生きしてはならない。

さらにconditional subobject等、root stabilityだけでは存在継続を保証できない対象では、型固有のsemantic dependencyを追加する。
`ref<read,LifetimeDomain>`だけでconditional payload occurrenceの継続まで保証されるわけではない。

### Draft 17.25 bounded explicit-domain write-root reloan source

Draft 17.24 §3.2の`explicit_domain_read_reloan`に加え、
**そのprofileで実際に開始済みのexact completed bounded recursive nominal
`H` lifetime rootだけ**について:

```text
explicit_domain_write_reloan
    := 'ref_from_ptr' '(' 'write' ',' ptr_name ',' stable_ref_name ')'
```

をclosed **Provisional** sourceとして採用する。
`write`はここでは選択されたbuiltin mode tokenであり、
arbitrary overload/callable/function matchingを追加しない。
`ptr_name`は既存actual-sourceの`ptr<H>` ordinary lexical binding、
`stable_ref_name`はexplicit current
`ref<read,LifetimeDomain>` bindingである。
`write`が証明できないときはreadへ自動fallbackしたり
read-only refを生成してあとからwriteへ増幅したりしない。

successでfresh ordinary `ref<write,H>`を作る条件は、
§10.1の**すべて**の前提に加えて、
`H`のactual occupied BackingRegionとpointer access evidenceが
**write access**を許すこと、governing identity Dとstableが一致すること、
current root incarnationとfull valid representation/current occupancy、
no blocking dependency conflictが成立すること。
`ref<read,LifetimeDomain>`はroot *stability*を証明するが
対象Hのwrite permissionを単独では供給しない。
返るrefはstable/domain-loanにscope-dependent、
必要なsemantic current-value/occurrence dependenciesを保持し、
scope外へ出せない。`ref<write,H>`は§11.2の**ordinary
(nonexclusive, nonunique, non-noalias)** capabilityであり、
rootのdestroy/take/end authorityではない。
一般`T`、ptr to subobject、unproven foreign backing、implicit
`ptr@field`やlayout-guessed writeを認めない。

§3.2の既存`ref_from_ptr(read,p,stable)`はread-onlyであり、
今回のfield writeへのshortcutとして再解釈しない。

## 10.2 ref -> ptr

`ref -> ptr` は safe total operation とする。

生成された `ptr` は ref scope を越えて保持できる。
元 object lifetime終了後、またはoriginating BackingRegion deallocation後はdanglingになり得る。

## 10.3 ptr -> ptr<U>

v0 では general `ptr<T> -> ptr<U>` conversion を提供しない。

typed location derivation が必要なら、型で定義された projection を用いる。
raw/untyped backing を typed object として使う場合は、`Storage -> slot<T> -> initialize` により
fresh object incarnation を開始し、`initialize` が返す `ptr<T>` を用いる。

### safe pre-lifetime ptr minting は行わない

ordinary safe v0 core は、`Storage` / `slot<T>` / `addr` から、
**まだ開始していない future `T` object incarnation** を指す `ptr<T>` を予約/mintする operation を提供しない。

したがって safe path では:

```text
Storage
  -> slot<T>
  -> initialize(value, domain)
  -> fresh live root incarnation O
  -> ptr<T> to O
```

の順序になる。

safe に生成済みの `ptr<T>` は、origin object lifetime end / BackingRegion end 後に
stale / dangling な non-live token として残り得る。
これは future incarnation への事前 token と同じではない。

platform / FFI / localized `unchecked` boundary は target-specific provenance rule に従って
raw location token を導入し得るが、その token は`ptr<T>`ではなく、future `T` incarnationの予約でもない。
raw location tokenは、そのlocationで将来`T` lifetimeが開始されること、そのfuture incarnation identity、
またはtyped semantic valueの存在を保証しない。

その token を safe `ref` へ変換するには、まずcurrent live typed incarnationが成立していなければならず、
通常どおり current liveness / provenance / governing-domain stability を証明しなければならない。

この boundary により、direct self-pointer / interior-pointer construction のためだけに
future-incarnation reservation semantics を core に追加しない。必要な library は、whole-object lifetime start 後の explicit fixup、logical handle / registry indirection、または localized `unchecked` implementation を用いる。

---

# 11. ref

`ref` は現在 dereference 可能な block-scoped capability である。

ordinary `ref<read,T>` / `ref<write,T>` は Copy + Discardable。
copy しても scope identity / scope dependency を越えた authority は生成されない。

最低限:

```text
ref<read, T>
ref<write, T>
```

を持つ。

## 11.1 scope

`ref` は scope-bound / non-escaping capability である。

v0 では少なくとも:

- return できない
- persistent field に保存できない
- global に保存できない
- escaping closure に capture できない

nested non-escaping block には渡せる。

### scope identity

compiler は各 scope-bound capability に、source から直接参照できない **scope identity** を関連付ける。

loan primitive によって生成された capability の scope identity は、その loan の non-escaping block extent に対応する。

ある scope-bound capability `A` が別の scope-bound capability `B` を使って派生された場合、`A` は `B` に scope-dependent である。

規則:

```text
A depends on B
    => every use of A occurs while B is live
```

依存関係は transitive である。

```text
A depends on B
B depends on C
    => A depends on C
```

compiler は、派生 capability を外側の binding へ持ち出す、block result として返す、またはその他の方法で派生元 capability より長生きさせる program を拒否しなければならない。

この規則は source-level lifetime parameter を要求しない。
v0 compiler は hidden scope identity / loan extent として追跡してよい。

### local object からの ref

通常 local object から生成された ref は、その local の implicit lifetime-stability loan に依存する。

したがって、その ref が live な間は local object の consume / transfer / destroy 等の conflicting lifetime-ending operation は行えない。

## 11.2 write != exclusive

`ref<write,T>` は mutation authority であり、exclusive ownership ではない。

複数の `ref<write,T>` が alias していてもよい。

したがって `ref<write,T>` だけから LLVM `noalias` 等を付与してはならない。

## 11.3 write != lifetime-root-ending authority

`ref<write,T>` は **lifetime root incarnation** を終了させる権限を持たない。

`destroy`, `take`, root move-out, reclaim, relocate 等、independently lifetime-ending rootを終了するoperationには別の lifetime-ending authority が必要である。

ただし、live rootの **型定義されたcurrent-value transition** に伴って、そのrootに従属するconditional subobject incarnationが開始・終了することはできる。

代表例はsum typeのvariant/payload transitionである。

```text
Option<T> root S
    Some payload P

replace(ref<write,Option<T>>, None)

S preserved
P ends
```

これはroot `S` のlifetime-ending operationではない。

従って:

> **write authority may change dependent subobject state, but does not end the enclosing lifetime root.**

とする。

## 11.4 write -> read authority weakening

ordinary capabilityについて:

```text
ref<write,T> -> ref<read,T>
```

はsafe total authority reductionとする。

source surfaceでは、callee parameter等の **既に選択済みのcontext** が
`ref<read,T>` を要求する場合に、`ref<write,T>` をbuilt-in contextual weakeningで使用してよい。

これは:

- implicit borrowではない
- user-defined/general implicit conversionではない
- function / associated candidate lookupを混ぜるranking ruleではない

とする。

candidate resolution後のparameter compatibilityとしてのみ適用する。

reverse:

```text
ref<read,T> -> ref<write,T>
```

は提供しない。

`exclusive ref` はnon-Copy authorityであるためこのordinary Copy-capability weakeningには含めず、
§12のexclusive reborrow ruleを使用する。

---

# 12. exclusive ref

`exclusive ref` は、その対象 location と overlap する他の live ref が存在しないことを保証する scope-bound capability である。

性質:

- non-Copy
- Discardable
- non-escaping
- block-scoped
- hidden scope identity を持つ

exclusive ref から、より短い scope の ordinary ref または exclusive ref を reborrow できる。

いずれの child reborrow も元 exclusive ref に scope-dependent である。

child ordinary/exclusive ref およびそこから transitively 派生した capability が live な間、
元 exclusive ref の conflicting use は suspended される。

概念的に:

```text
exclusive E0
    -> ordinary child O
        -> derived ref R

R live
    => O live
    => conflicting use of E0 unavailable
```

または:

```text
exclusive E0
    -> exclusive child E1

E1 live
    => conflicting use of E0 unavailable

E1 ends
    => E0 usable again
```

exclusive -> exclusive reborrow は、同じ lifetime-ending authority を複数の逐次 operation で安全に再利用するために用いる。

`exclusive` は `write` と独立した概念である。

## 12.1 call / primitive operand への exclusive reborrow

already-selected callee parameterまたはprimitive operandが、既存のtype / authority compatibility ruleの下で
`exclusive ref<mode,T>` を要求し、actual argumentがexisting exclusive-ref bindingである場合、
そのargument useは§18.2のordinary non-Copy consume / ownership transferではなく、
本節の **exclusive reborrow** を行う。

conceptually:

```text
caller:
    parent E0 : exclusive ref<mode,T>
        Available

call / primitive operation:
    E0
      -> child E1 : exclusive ref<mode,T>
           scope <= call / operation extent

while E1 live:
    conflicting use of E0 suspended

when E1 ends:
    E0 usable again
```

callee parameterまたはprimitive operandが見るauthorityはfresh child `E1` であり、
callerのparent binding `E0` のownership transferではない。

このruleは:

- existing exclusive authorityからのscope-shortening reborrowである
- ordinary value `T` からrefを作るimplicit borrowではない
- Copyではない
- general implicit conversionではない
- overload / candidate ranking ruleではない
- exclusive-ref用の新しいwrite->read等のweakening ruleを追加しない

とする。

type compatibilityはcandidate / primitive operation選択後に既存規則で判定する。
compatibleでないexclusive refを、このreborrow ruleだけを理由にparameter typeへ適合させてはならない。

一方、exclusive refをordinary valueとしてtransferする場合:

```text
let e2 = e1
```

は通常のnon-Copy value-useであり:

```text
e1 -> Consumed
e2 -> Available
```

となる。

同様に、`LifetimeDomain` / `Allocation` / `Storage` 等のnon-Copy authority valueをargumentとして渡す場合も、
その型に別の明示reborrow ruleが無い限り§18.2のordinary transferに従う。

代表的な逐次利用:

```text
loan exclusive read life as ending {
    take(p1, ending)
    take(p2, ending)
}
```

第一の`take`は`ending`自体をconsumeせず、operation-local child exclusive refを使用する。
そのchildは第一のoperation終了時に終了し、第二の`take`で同じparent `ending`から別のchildをreborrowできる。

このruleは`take` / `destroy`のlifetime-ending authority operandにも適用する。
各operation中にchild authorityがliveな間だけparentのconflicting useがsuspendされる。

---

# 13. LifetimeDomain

## 13.1 目的

`LifetimeDomain` は、explicitly managed object incarnation の lifetime-ending authority を表す。

性質:

- opaque
- non-Copy
- non-Discardable
- fresh domain identity を持つ
- address とは無関係

## 13.2 object と domain

explicitly managed live object incarnation は一つの governing lifetime domain に属する。

この **root governing-domain relation** は root incarnation 側の place/state-owned relation であり、
semantic `ValuePackage` の一部ではない。
`LifetimeDomain` identity/value 自体が semantic value の constituent として package に含まれる場合とは区別する。

object incarnation が lifetime を開始した時点で governing domain が定まり、その incarnation が終了するまで変更されない。

field / subobject は原則として enclosing object と同じ governing domain に属する。

`ptr<T>` の型には domain parameter を含めないが、abstract machine 上では、その `ptr` が由来する object incarnation と governing domain の関係を保持する。

同じ machine address が後に別 domain の新しい incarnation に再利用されても、古い `ptr` の governing-domain relation が新 incarnation へ付け替わることはない。

## 13.3 ordinary domain ref = stability evidence

```text
ref<read, LifetimeDomain>
```

は、

> その domain に属する現在 live な **lifetime-root incarnation** が、
> この domain ref の scope 中に終了しない

ことを保証する stability evidence として用いる。

safe `ref<T>` を `ptr<T>` から生成する場合、対象 incarnation の governing domain と **同じ domain** への live ordinary ref が必要である。

生成された target `ref<T>` は、その ordinary domain ref に scope-dependent である。

対象がconditional subobjectなら、root lifetime stabilityに加えて、そのspecific subobject occurrenceを保護するsemantic dependencyも必要である。

したがって:

```text
target ref live
    => corresponding domain ordinary ref live
```

が normative に成立しなければならない。

## 13.4 exclusive domain ref = lifetime-ending authority access

lifetime-root incarnation を終了させる operation は、

```text
exclusive ref<read, LifetimeDomain>
```

相当の authority を要求する。

必要なのは domain data への write ではなく、他の stability loan と共存しない exclusive access である。

さらに、その exclusive domain ref が参照する domain は、対象 object incarnation の governing domain と一致しなければならない。

したがって、domain `B` への exclusive authority を使って、domain `A` に属する object lifetime を終了させることはできない。

compiler がこの domain identity の一致を証明できない operation では `unchecked` が必要になり得るが、exclusive domain capability 自体を省略することはできない。

## 13.5 安全性の核心

lifetime-root incarnation `O` が domain `D` に属し、`O` またはそのroot-stable subobjectへのsafe ref `R` が `D` への stability evidence `S` から派生した場合:

```text
R live
  => R depends on S
  => S live
  => ordinary ref to D live
  => exclusive ref to D unavailable
  => root-lifetime-ending operation for O unavailable
  => O root lifetime cannot end
```

この implication chain は v0 の reference lifetime safety の核心である。

ここで保証されるのは lifetime stability であり、target object の alias exclusivity ではない。

## 13.5a semantic dependency / semantic value package

scope-bound capabilityは、place lifetimeだけでなく、
ある **semantic identity / state fact / subobject occurrence** に依存してよい。

compilerは概念的に:

```text
capability C
    semantic-depends-on F
```

というhidden dependency relationを追跡する。

dependencyはscope dependencyと同様にtransitiveである。

Draft 12ではhidden dependencyをtransient value-flow nodeだけのpropertyとはみなさない。
semantic valueはconceptually:

```text
SemanticValuePackage(V) {
    semantic_value: V
    scope_dependencies
    semantic_dependencies
    backing_dependencies
}
```

相当のpackageとして扱える。

これはruntime wrapperやper-object dependency tagを要求しない。
compiler / semantic IR上のghost metadataでよい。
またsource type systemへdependency parameterを追加するものでもない。

ordinary integer等の値は通常empty dependency setを持つ。
ref / span / exclusive ref / loan-derived capability / conditional payload ref / lexical backing由来claim等だけが
meaningful dependencyを持つことが多い。

semantic identity / authority / provenanceそのものはdependency setではなくsemantic value側の情報である。
例えば`LifetimeDomain` identityや`Storage`のBackingRegion/range claimはvalueとともにtransferされ、
「そのidentityを変更してはならない」というblocking relationだけをhidden dependencyで表す。

### value-flow propagation

hidden dependencyはvalue flowやmemory installationによってlaunderされてはならない。

少なくとも:

```text
Deps(copy(V))       = Deps(V)
Deps(transfer(V))   = Deps(V)
```

とする。

constructor / aggregate / wrapper / projection等では、
result semanticsに応じて:

```text
Deps(result)
    = inherited dependencies
      + newly introduced dependencies
```

を持ち得る。

scope-bound capabilityをaggregate等へ包める場合も、
wrapper一枚を介してdependencyを消してはならない。

function argument / result、local binding、memory placeのcurrent valueの間を移動しても同じである。

### current-value state / canonical structural state

live typed place `P` は、object/subobject incarnationとは別に
**current semantic value fact** を持つ。

ただしaggregate rootとstructural subplaceに
complete semantic valueを独立重複して保持するものとは考えない。

live lifetime rootごとに、abstract-machine上conceptually一つの:

```text
canonical structural current-state
```

がある。

例:

```text
struct S {
    a: A
    b: B
}

root s

CurrentState(s)
├── s.a
└── s.b
```

`CurrentValue(s)` は root node 自身のlocal semantic fragmentと
`CurrentValue(s.a)` / `CurrentValue(s.b)` 等のcurrent structural childrenから
compositionally決まる。

従ってparent aggregate current valueはchild valueを含むviewであるが、
parent nodeにchildrenのcomplete value/dependencyを第二のsource of truthとしてcopyしない。

各tracked structural place `P` にはconceptually:

```text
CurrentValueFact(P)
```

がある。

compiler-internal exact checkerでは、例えば:

```text
ValueFact {
    place: P
    identity: V
}
```

または:

```text
ValueStamp(P) = V
```

のようなfresh ghost identityとして表現してよい。

これはruntime version counter/tagを要求しない。

#### subplace update と ancestor current-value fact

structural subplace `P` のsemantic valueを変更すると、
`P`自身だけでなく、そのvalueをcompositionally含むancestor placeのcurrent semantic valueも変わる。

例えば:

```text
S
├── a
└── b
```

で:

```text
replace(S.a, new_a)
```

するとconceptually:

```text
Value(S.a): changes
Value(S):   changes
Value(S.b): preserved
```

となる。

known-disjoint siblingのcurrent-value factは維持する。

これは§13.5bの:

```text
Change(P) conflicts Value(Q)
    iff P and Q may semantically overlap
```

というabstract effect relationと整合する。

#### whole-value update

whole aggregate place `P` のreplace/store/swapでは、
P以下でsemantic valueが置き換わるstructural current-value factは更新される。

fixed field/subobjectの **place / lifetime incarnation** は既存ruleどおり維持できる。
current-value factがfreshになることとobject/subobject incarnationが終了することを混同しない。

### dependency ownership / structural current-value dependency

hidden dependencyは、それをcarryするsemantic subvalue/packageに属する。

known structural subvalueについては、dependencyをsmallest meaningful subvalueへ保持できる。

例えば:

```text
Holder
├── borrowed
└── count
```

で`borrowed`だけがshort-lived refをcarryするならconceptually:

```text
LocalDeps(Holder)          = {}
LocalDeps(Holder.borrowed) = { short dependency }
LocalDeps(Holder.count)    = {}
```

とできる。

aggregate全体のdependencyは:

```text
Deps(Holder subtree)
    = LocalDeps(Holder)
      ∪ Deps(Holder.borrowed)
      ∪ Deps(Holder.count)
```

のようにderiveできる。

descendant dependencyをparent nodeへ第二のindependent copyとして保持する必要はない。

これにより:

```text
store(holder.borrowed, None)
```

で`borrowed` subtreeだけのdependent packageを終了し、
`Holder` rootや`count`を維持できる。

field / payload等は既存のstructural place/projection identityを再利用する。

dynamic index / range / pointer-indirected graph等を精密に区別できない場合、
compilerはsoundなlarger place / may-set / `Unknown`へwidenしてよい。
一般heap shape analysisはv0 requirementではない。

### place-owned state と value-owned semantic package

installed current-stateには、semantic valueそのものとplace/lifetime側のstateが共存するが、
transfer時には区別する。

conceptually place-ownedなもの:

```text
PlaceId
lifetime-root incarnation
root placement backing relation (BackingRegion identity / occupied range)
root governing LifetimeDomain relation (DomainId)
fixed subobject place/incarnation relation
conditional occurrence identity
current-value fact identity
```

conceptually semantic-value-ownedなもの:

```text
visible semantic value
LifetimeDomain identity
Allocation authority
Storage BackingRegion/range claim
ptr provenance / location token state
hidden scope dependency
hidden semantic dependency
hidden backing dependency
```

後者はsemantic value packageとともにcopy/transfer/installされ得る。

前者はsource placeからvalueと一緒にtransferされない。

root placement backing relationは、valueが偶然あるBackingRegion上に置かれているという
place/state factであり、その事実だけを理由にsemantic `ValuePackage`へsource-placement dependencyを付けない。
`take` / consume-out / opaque relocation後のdestination placementはdestination側からfreshに決まる。

同様に root governing `LifetimeDomain` relation も source root incarnation 側のstateであり、
`take` / ordinary consume-out の result `ValuePackage`へextractしない。
fresh destination root は、その lifetime-start operation が指定/管理する domain から fresh governing relation を得る。
したがって `take` した `T` を別domainで `initialize` すること自体は、`T`内部のhidden dependency/authorityがそれを別途禁止しない限り、root governing relationのtransferとはみなさない。

opaque distinct relocationは例外であり、relocation semanticsが source governing `DomainId D` を読み、
destination fresh rootへ **同じ D とのfresh relation** を作る。これは root relation を ValuePackage に入れてtransferすることを意味しない。

ただしvalue内部のauthority/provenance/capabilityが独自にBackingRegionへ依存している場合、
そのvalue-owned backing state/dependencyは従来どおりpackageとともにtransferする。

特にsum payloadでは、old payload semantic valueはold sum packageとともにtransferできるが、
source placeのold payload occurrence identityそのものはtransferしない。
destinationへinstallされたconditional payloadはdestination側でfresh occurrenceを得る。

### current-value state liveness

memory placeのcurrent-value dependencyは、そのsemantic valueがplaceにinstallされている間liveである。

```text
replace(P, dependent_value)
use(P)
// no more reads
```

でlast useが終わっても、`P`にそのvalueがcurrent valueとして残っているならdependencyは残る。

current-value stateは少なくとも:

- another `replace` / `store`
- distinct-place `swap`
- `take` / `destroy`
- consume-out
- enclosing object/subobject lifetime end

等のsemantic transitionで終了・移動する。

transient capabilityのdependency livenessを将来NLL的に短縮できることと、
memoryに現在格納されているvalueの存在をlast-useで消せることは別である。

compiler IRがmemory SSA等で過去のversion/historyを保持しても、
abstract-machine上で各tracked structural placeにcurrentなのは一factだけである。
old current-value factはtransition後のcurrent factとして残存しない。


### lexical block result

lexical block resultがdependency-bearing valueを返す場合、
そのresult valueはdependencyを保持する。

```text
let x = {
    ...
    dependent_ref
}
```

はconceptually:

```text
Deps(x) = Deps(dependent_ref)
```

である。

ただしresultがblock exit後もsurviveし、そのdependency source scopeがblock exitで終了する場合は
後述のscope-exit compatibilityによりrejectする。

### control-flow join

複数のnormal control-flow edgeから同じvalue / current-value stateへjoinする場合、
visible stateだけでなくhidden dependencyも同じedge relationに従う。

normative modelは概念的なhidden SSA phi / block argumentまたはmemory-state phiである。

```text
r =
    if (cond) {
        ref_a        // depends on F_a
    } else {
        ref_b        // depends on F_b
    }

hidden:
    dep_r = phi(F_a, F_b)
```

memory current-value stateでも同様に:

```text
if (cond) {
    replace(P, a)
} else {
    replace(P, b)
}

post CurrentDeps(P) = phi(Deps(a), Deps(b))
```

とみなせる。

これはruntime phi/tagを要求しない。

compilerはsoundな近似として:

```text
MayDeps(P) = Deps(a) ∪ Deps(b)
```

等を使用してよい。

ただしunionはimplementation approximationであり、language semantics自体がpath correlationを失うことを意味しない。
terminator edgeはnormal result / normal-state joinには参加しない。

### loop-carried dependency / cyclic header state

`continue(values...)` がnext iterationのfresh loop parameter bindingsへvalueを渡す時、
各valueのhidden dependencyも同じcontrol-flow edgeを通る。

loopのlanguage reference semanticsは、initial entryから有限回のreachable continue edgeを経て
実際に到達し得るconcrete semantic states / package identities / dependency factsを意味する。
language semantics自体がpath correlationを失ったabstract stateへ置き換わるわけではない。

compilerはそのreachable-state集合を直接列挙する代わりに、
§27.5のconceptual loop-header abstract state `H` を用いてよい。
`H` は少なくとも:

- initial entry stateをsoundly含む
- `H`の任意のrepresented concrete header stateからreachable continue edgeを一回実行して得るsuccessor stateをsoundly含む

inductive post-fixpointでなければならない。

conceptually、抽象状態の包含を `⊑` と書けば:

```text
EntryState ⊑ H

for each statically reachable continue edge i:
    ContinueTransfer_i(H) ⊑ H
```

または同値に:

```text
Join(EntryState, ContinueTransfers(H)) ⊑ H
```

とみなせる。

このnotation / abstract domain / Join operator自体をcompiler representationとして固定しない。
exact least-fixpoint iterationをcompilerへ要求しない。
soundなstronger inductive post-fixpoint、finite may-set、`Unknown`、wideningを使用してよい。

ただし:

- first iterationのconcrete stateだけでbodyをcheckし、その結論をlater iterationへ無条件再利用してはならない
- widening / `Unknown`でpossible dependencyをemptyとして扱ってはならない
- safetyに必要なidentity/dependency correlationを失った場合、unsafe acceptではなくconservative reject / analysis-precision rejectionへ倒す

ものとする。

caller-visible / outer current-value stateをloop中に変更する場合も、
later iterationは一般にentry時のcurrent-value factへ戻らない。
そのstateはhidden loop-carried memory stateまたは上記のsound cyclic abstractionで扱う。

iterationごとにfresh semantic occurrence / value fact / package identityが生じる場合も、
symbolic header state / recursive block argumentとして表現できるため、
compilerがidentityのinfinite static setを列挙する必要はない。

### surviving dependency

operationまたはcontrol-flow edgeがdependency source fact `F` をinvalidateする場合、
そのoperation/edgeを越えて `F` に依存するsemantic value packageが **survive** するならconflictする。

conceptually:

```text
dependent package survives
    && transition/edge invalidates required fact
    => conflict
```

survivalには少なくとも以下を含む。

- function / block / callable resultとしてreturn/leaveする
- another binding / placeへtransferする
- `replace` / `take`のresultとして返す
- `swap`で相手placeへ移る
- caller-visible memoryのcurrent valueとして残る
- loop next iterationへ持ち越す

一方、dependent package自体が同じ **unobservable atomic semantic transition** で
unconditionally discarded / endedされ、transition後へ一切残らない場合、そのdependencyはsurviveしない。

このruleにより、old packageだけに含まれるdependencyについて:

```text
replace : old package returns      -> survives
store   : old package discarded    -> does not survive

take    : old package returns      -> survives
destroy : old package discarded    -> does not survive

swap    : both packages transfer   -> survive
```

という差を表せる。

incoming new valueや外部aliasに同じdependencyが存在しtransition後へsurviveする場合は、
old packageがdiscardされてもconflictは消えない。

### candidate post-state well-formedness

surviving-dependency ruleは、semantic checkerにおいて
candidate post-stateのwell-formednessとして同値に表現できる。

conceptually、state `S` がliveとみなすfact集合を:

```text
LiveFacts(S)
```

とする。

少なくとも:

```text
live scopes
live backing facts
current ValueFact(P)
current conditional OccurrenceFact(P)
```

等を含む。

semantic transition / control-flow edgeでcandidate post-state `S'` を構成した後、
そのstateにsurviveする全semantic value packageについて:

```text
Deps(all surviving packages) ⊆ LiveFacts(S')
```

でなければならない。

つまり:

```text
surviving package depends on F
F is absent from candidate post-state
    => reject
```

である。

これは§13.5aのsurviving-dependency ruleのreference formulationであり、
新しいsource-visible mechanismではない。

例えば:

```text
store(P, new)
```

ではold packageがcandidate post-stateへ存在しないため、
old packageだけがcarryしていたdependencyは検査対象としてsurviveしない。

一方:

```text
replace(P, new)
```

ではold packageがresultとしてcandidate post-stateへsurviveするため、
old `Value(P)` factへのdependencyが残るならrejectし得る。

同様に:

```text
take    -> old package survives
destroy -> old package does not survive
swap    -> both packages survive at the opposite places
```

となる。

実装はtransition前後のeffect/dependency conflictを直接検査してもよく、
candidate post-state invariantを直接検査してもよい。
両者は仕様上同じ合法性判定を与えなければならない。


### scope-exit compatibility

control-flow edgeでscope `S` が終了する時、
そのedge後もsurviveするsemantic value / current-value stateは `S` にscope-dependentであってはならない。

このruleは少なくとも:

- lexical block exit
- named function `return` / normal completion
- nonescaping callable invocation return / `leave`
- loop `continue`
- loop `break`

に共通して適用する。

例えばcallee-local refを一時的にcaller-visible placeへinstallしても、
callee-local scope終了前にそのdependent current-value stateを終了・復元できればよい。
callee return後まで残るならrejectする。

callback専用の「parameter refをmemoryへ保存してはならない」という別規則は導入しない。

### current-value identity dependency

`ref<read, LifetimeDomain>` をstability evidenceとして用いる場合、
そのcapabilityは単なるdomain storage placeではなく、
**現在保持されているdomain identity** にsemantic-dependentである。

従って、そのidentityまたはtransitively derived capabilityがtransition後へsurviveする間、
そのcurrent domain identityを変更・consumeするoperationはconflictする。

例として `State.domain` のcurrent domain identityに依存するcapabilityがsurviveするなら、
`State.domain`自体だけでなくenclosing `State` whole-value replace/store/swapも、
そのidentityをpreserveすると証明できない限り禁止される。

ただしdependent capabilityがdestroy/store等の同じatomic transition内でのみ存在し、
transition後へsurviveしない場合はsurviving-dependency ruleに従う。

### conditional occurrence dependency

sum typeのactive payload等、
current stateによって存在するconditional subobjectには
abstract-machine上の **occurrence identity** を与える。

payload ref等は:
```text
capability C
    occurrence-depends-on payload occurrence P
```

とできる。

`replace(payload_ref, new)` のようにpayload occurrence `P` を維持するoperationは共存できる。

一方、whole-sum transitionがold active payload occurrenceを終了する場合、
`P` に依存するpackageがtransition後へsurviveするならconflictする。

### ptr provenance != blocking dependency

persistent `ptr<T>` は:

- backing / location provenance
- originating object / payload occurrence relation

を保持し得る。

ただしptr valueを保持しているだけでは、
そのobject / payload occurrenceの終了を禁止するblocking semantic dependencyにはしない。

従ってpayload由来ptrが存在していてもwhole-sum transitionは可能であり、
old payload occurrence終了後そのptrはdanglingになる。

safe ptr->refでcurrent liveness / occurrenceを再証明した場合、
生成されたscope-bound refへ必要なsemantic dependencyを付与する。

> persistent provenance is not a blocking loan.

### transient dependency liveness

memory current-value stateではなくtransient value-flow nodeだけについて、
v0 compilerはまずlexical scopeを基準にdependencyをliveと扱ってよい。

```text
{
    let r = dependent_ref
    use(r)
} // transient dependency may end here

invalidate_parent()
```

last-use直後にtransient dependencyを短縮するnon-lexical analysisはv0必須ではない。
将来NLL的shorteningを導入してもsource semanticsを変更する必要はない。

## 13.5b place-relative semantic effect algebra

function / module boundaryでsemantic invalidationを扱うため、
compilerはeffect targetを **interface-relative structural place** として表現する。

これはsource-level effect typeではない。

### relative place

conceptual grammar:

```text
Place ::= Referent(ParameterValuePath) Projection*
        | Ambient(Symbol) Projection*
        | UnknownPlace

ParameterValuePath ::= Param(index) ValueField*

Projection ::= .field(ProjectionId)
             | .payload(VariantId)
```

`Referent(...)` はparameter value自身ではなく、
そのparameterまたはparameter内fieldが保持する ref / ptr / capability が指すplaceを表す。

Draft 10以降の`span<...,T>` parameterについては、`Referent(...)` が
そのspanがcoverするtyped contiguous interval全体を表してよい。
このrange-placeのcoarse effect semanticsは§25.12で定義する。

例:

```text
fn f(s: ref<write, State<T>>)

Referent(Param(0))
Referent(Param(0)).field(option)
```

wrapper内のcapabilityなら:

```text
struct Args<T> {
    target: ref<write,T>
}

Referent(Param(0).field(target))
```

と表せる。

parameter valueのlocal copy自体へのmutationはcaller-visible effectではないためsummary対象にしない。

### opaque projection identity

`ProjectionId` はsource-visible field nameである必要はない。

compilerはnominal aggregate projectionについて:

- projection identity
- parent/child containment relation
- known disjoint sibling relation
- projection result type / semantic categoryとして必要な最小情報

をopaque semantic metadataとして保持してよい。

v0ではこのmetadataをone semantic compilation unit内のcompiler stateとして持てばよい。

future module / separate compilationでtransportが必要になった場合も、
source-visible field nameやphysical field offset / layoutをsemantic APIとして公開する必要はない。

### structural boundary

relative placeは **interfaceから構造的に導出できるplace** だけを表す。

persistent ptr等を辿ったarbitrary graph traversal先がinterface-relativeに表現できない場合、
そのeffectは`UnknownPlace`または`Unknown` effectへwidenする。

v0はsummary precisionのためにgeneral heap reachability / shape analysisを導入しない。

### symbolic dependency atom

summary / call-site conflict checkingでは、semantic dependencyをconceptually:

```text
Dependency ::= Value(Place)
             | Occurrence(Place)
```

として抽象化できる。

`Value(P)` は P のcurrent semantic value / value identityが維持されることへのdependency。

body-local exact checkerではこれをconceptually:

```text
ValueFact(P, current-fact-identity)
```

として具体化してよい。

`Occurrence(P)` はconditional subobject occurrence P自体が継続して存在することへのdependency。

body-local exact checkerではこれをconceptually:

```text
OccurrenceFact(P, current-occurrence-identity)
```

として具体化してよい。

`Value(Place)` / `Occurrence(Place)` はfunction/callable summary用のabstract interface-relative formであり、
exact installed-state representationそのものを固定するものではない。

root lifetime stabilityは新しい`Root(...)` dependencyへ複製せず、
既存のscope dependency + `LifetimeDomain` mechanismで扱う。

### effect atom

function summaryの基本effect atomは:

```text
EffectAtom ::= Change(Place)
             | Reset(Place)
             | EndRoot(Place)
             | Unknown
```

とする。

#### Change(P)

Pのcurrent semantic valueを変更し得る。

P自身のobject/subobject incarnationを終了することまでは意味しない。

#### Reset(P)

P自身のincarnationを維持しながら、
Pの **strict conditional descendants** のoccurrenceを終了・再開始し得る。

従って`Reset(P)`は`Occurrence(P)`そのものとは直接conflictしない。

これは例えばpayload occurrence P 自身を維持したまま、
P のcurrent valueがnested sumを含むため、そのnested payload occurrenceだけが作り直されるcaseを表せる。

whole-sum replaceではsum place Sに対して:

```text
Change(S)
Reset(S)
```

を持ち、active payload occurrenceはSのstrict conditional descendantなのでinvalidateされる。

payload occurrence P 自身へのreplaceではPは維持されるが、
P内部にconditional descendantsがあればそれらはresetされ得るため:

```text
Change(P)
Reset(P)   // nested conditional descendants only
```

と表せる。

型上P以下にconditional subobjectが存在し得ないとcompiler-knownなら`Reset(P)`は省略してよい。

#### EndRoot(P)

Pのlifetime-root incarnationを終了し得る。

`take` / `destroy` / root consume-out等のlifetime-ending effectを表す。

#### Unknown

interface-relative structural place / effect kindへ十分に表現できないpotential invalidation。

preservation/disjointnessを別途証明できないlive semantic dependencyとはconflictする。

### conflict relation

以下のeffect/dependency conflict relationは、§13.5aのsurviving-dependency ruleに従い、
そのtransition / edgeを越えてsurviveするdependency occurrenceに対して適用する。
同じatomic transitionでdependent package自体がunconditionally discarded / endedされる場合、
そのpackageだけに含まれるdependencyはpost-transition blockerではない。

少なくとも:

```text
Change(P) conflicts Value(Q)
    iff P and Q may semantically overlap

Reset(P) conflicts Occurrence(Q)
    iff Q is a strict conditional descendant of P

EndRoot(P) conflicts dependency on Q
    iff Q's lifetime/state is contained in root P

Unknown conflicts live dependency
    unless preservation/disjointness is independently proved
```

とする。

`overlap` はsame placeだけでなくancestor/descendantを含む。
known-disjoint fixed sibling fieldsはoverlapしない。

`Reset` のcontainment testはdirectionalである。
inner sumをresetしてもouter payload occurrenceが維持されるcaseを許す。

### primitive effect inference

core operationから少なくとも:

```text
replace/store at P:
    Change(P)
    Reset(P) if P may contain conditional descendants

swap at P, Q:
    if P and Q are proven same place:
        no effect
    otherwise:
        Change(P)
        Reset(P) if P may contain conditional descendants
        Change(Q)
        Reset(Q) if Q may contain conditional descendants

payload-only replace at occurrence P:
    Change(P)
    Reset(P) only for conditional descendants strictly inside P

root take/destroy/consume-out at P:
    EndRoot(P)
```

相当をinferできる。

fixed sibling field mutationはそのfield placeだけを`Change`する。

### effect set / join

ordinary concrete summaryはeffect atomのfinite may-setでよい。

```text
Summary = finite set of EffectAtom
```

empty setはno caller-visible invalidation。

branch join / sequential compositionはいずれもmay-effect unionでよい。

```text
summary(if)      = summary(then) ∪ summary(else)
summary(a; b)    = summary(a) ∪ summary(b)
```

summaryはoperation orderingを保持しない。
local orderingに依存するsafetyはcallee bodyのstrict evaluation-order checkerで検査する。

### summary normalization

compilerはsemantic equivalenceを保つ範囲でsummaryをnormalize / compressしてよい。

`Reset(P)` は `Change(P)` をsubsumesしない。
`Value(P)` 系dependencyとのconflictを保持するため、value transitionが両方を起こし得る場合は両atomを残す。


例えば`EndRoot(P)`がP以下のより狭いlifetime/state invalidation effectをsubsumesすると判断できる場合、
artifactから冗長atomを省略してよい。

normalizationはsource semanticsではなくartifact optimizationである。

## 13.5c function / generic / callable semantic summary

### function boundary is not a dependency boundary

function / callable boundaryはhidden dependencyをerase / shorten / resetするboundaryではない。

known ordinary function callについて、v0のreference semanticsはconceptually:

```text
actual argument / referent relationをcallee bodyへsubstitute
    -> strict source evaluation orderでbody semanticsを適用
    -> caller-visible post-stateを得る
```

ものと同値である。

これはliteral source inliningをimplementationに要求しない。
compilerはsummary / typed IR / abstract interpretation / memoization等を使ってよい。

caller-visible mutable placeのcurrent semantic valueがcallee中で変化した場合、
そのpost-call current-value dependency / semantic identity relationもcallee body semanticsに従う。
function boundaryを越えたことだけを理由にdependency-free / identity-unknown-as-safeとしてはならない。

### post-current-value analysis

v0 specificationはgeneral source-visible / stable serialized `PostCurrentDeps` languageを定義しない。

implementationはconceptually:

```text
PreCurrentState(Place)
    -> callee body semantics
    -> PostCurrentState(Place)
```

を表す任意のcompiler-internal representationを使用してよい。

ただし少なくとも:

- source-order sequential writes
- same actual place passed to multiple formal parameters
- same-place `swap` no-op
- distinct `swap` simultaneous exchange
- branch / loop join
- scope-exit compatibility

をsource semanticsどおり扱わなければならない。

proofできないpost-stateをempty dependency / known identityとして扱ってはならない。
`Unknown` / may-set / conservative larger-place stateへwidenしてよい。

### call-site place substitution

callee summaryのrelative placeはactual argument / referent relationをsubstituteしてcaller contextへ写像する。

例:

```text
fn clear(x: ref<write,Option<T>>)
summary(clear) = {
    Change(Referent(Param(0))),
    Reset(Referent(Param(0)))
}
```

を:

```text
clear(field_ref(state, .option))
```

と呼ぶならcaller側では:

```text
Change(state.option)
Reset(state.option)
```

相当にsubstituteする。

actual argument place relationを十分に表現できない場合、そのatomは`Unknown`へwidenしてよい。

### ordinary function summary inference

ordinary known functionのsummaryは:

- core operation effects
- direct body effects
- known callee summaries after substitution

からinferする。

transitive call chainでも同じsubstitution + unionを用いる。

recursive SCCではfinite monotone abstract domain上のfixpointでよい。

### aliasing formal parameters

formal parameter summaryは互いにdisjointと仮定しない。

call siteでactual placeをsubstituteした後、
live dependencyとのoverlap / containmentを判定する。

同じactual placeが複数formal parameterへ渡されても同じruleを用いる。

### ambient effect

explicit parameterを通らないcaller-visible mutable rootを将来導入する場合:

```text
Ambient(Symbol)
```

をanchorとして使える。

ambient root identityを公開interfaceへ表せない場合は`Unknown`へwidenする。

### symbolic summary expression

nonescaping callable parameterまたはgeneric dependent callのように、
definition-timeにconcrete summaryが未確定なcall targetについて、
compilerはconceptually:

```text
SummaryVar ::= symbolic callee/callable summary variable

EffectExpr ::= Empty
             | EffectAtom
             | EffectExpr ∪ EffectExpr
             | Apply(SummaryVar, Substitution)
```

を扱ってよい。

これはsource-visible effect polymorphismではない。

`Apply` はsummaryが確定した時点でrelative place substitutionを行い、
通常のeffect atom / conflict ruleへ帰着する。

### generic dependent call

§21のdependent associated callでcalleeがinstantiation-timeまで未確定なら、
そのcall siteのeffect summaryだけでなく、caller-visible post-current-value state / dependency flowも
instantiation-timeまでsymbolic / deferredでよい。

live semantic dependencyとのcompatibility、scope-exit compatibility、
または後続safe operationに必要なsemantic identity / backing relationがdefinition-timeに決められない場合、
compilerは **deferred semantic compatibility obligation** を記録する。

instantiation-timeにassociated functionとbody/summaryを確定し、
substitution後に通常のconflict / post-state / scope-exit checkを行う。

このobligationは既存のinferred generic requirementsと同様に
artifact / diagnostics / IDEで追跡可能とする。

unknown post-stateをdependency-freeと仮定してはならない。

### nonescaping callable effect summary

nonescaping callable parameterごとにcompilerはconceptually:

```text
CallableSemanticSummary {
    effects: EffectExpr
    result_dependencies: DependencyExpr
}
```

相当を扱ってよい。

actual callable blockのbodyからeffectをinferし、
callee body内のcallable invocation siteへ`Apply(...)`する。

0..N回invocation可能でもmay-effect summary自体はunionでよい。

actual callableがcaller-visible current-value stateを変更する場合も、
invocation body semanticsからpost-stateを追跡する。0..N回でexact identity / correlationを保てない場合は
soundなmay-stateへwidenしてよい。invocation-local scopeに依存するstateをcallable return後へ残してはならない。

### callable result dependency

callable block resultがhidden dependency-bearing valueなら、
そのdependencyもinvocation resultへ流れなければならない。

definition-timeにresult dependencyがsymbolicな場合、conceptually:

```text
DependencyExpr ::= Empty
                 | Value(Place)
                 | Occurrence(Place)
                 | InputDeps(index)
                 | DependencyExpr ∪ DependencyExpr
                 | ApplyResultDeps(SummaryVar, Substitution)
```

程度のcompiler-internal expressionを用いてよい。

例えばidentity-like callable:

```text
{ |x| x }
```

ではresult dependencyは`InputDeps(0)`相当である。

caller/callee invocationでactual block parameter place/dependencyをsubstituteし、
result valueのhidden dependencyへ接続する。

### callback result compatibility

calleeがcallback resultを受け取った後にsemantic invalidationを行う場合、
result dependencyがconcreteでなければcompatibility checkをsymbolic obligationとして保持する。

actual callableが確定したcall siteでresult dependencyをsubstituteし、
subsequent `Change` / `Reset` / `EndRoot` と通常のconflict checkを行う。

従ってdependency-bearing callback resultをv0で一律禁止しない。

### callback ordering

summary expression自体はoperation orderを保持しない。

しかしcallee bodyではstrict source evaluation orderに従って:

```text
{
    let v = payload_ref
    block(v)
} // v dependency ends

reset(parent)
```

のようなsafe orderingと、dependency live中にresetするunsafe orderingを区別できる。

interprocedural sequence effect algebraはv0に導入しない。

### unknown / external call

summaryが無いcall / FFI / indirect target等は、
preservationを証明できない範囲で`Unknown`として扱う。

foreign function signature、calling convention identity、pointer type、aggregate parameter shape等の
**representation / ABI informationそれ自体はsemantic summaryではない**。
これらだけを根拠に、semantic non-mutation、location retentionの不在、callbackの同期性、
ordinary return / non-local transferの不在を仮定してはならない。

`Unknown`は「foreign / indirect codeなので何をしてもよい」というpermissionではない。
特に、既存のscope / lifetime ruleでは許されない:

- scope-bound `ref` / callable /その他 capability のescape
- raw out-locationやborrowed locationのcall return後retention
- call return後のasynchronous / retained callback invocation
- foreign unwind / longjmp等のnon-local control transfer
- lifetime / ownership authorityの暗黙transfer

を`Unknown`だけで正当化してはならない。
これらをv0またはfuture FFIで許す場合は、その挙動を表すexplicit foreign-boundary contract / privileged transitionを別途定義する。

function pointer / FFIをv0へ含める場合、そのsummary sourceまたはfallbackを別途定義する。
known / finite-known indirect targetをcompilerが追跡できる場合にbody-sensitive analysisを再利用することは妨げない。
完全にunknownなtargetは既存の`Unknown` precision boundaryへfallbackしてよい。

### public semantic contract

public functionのcompiler-generated semantic summaryはsource syntaxに現れなくても、
callerのwell-formednessを左右するsemantic interfaceの一部である。

summaryがより広いplace / stronger invalidationへwidenすると、
以前compileできたcallerがrejectされ得る。

従ってpublic functionのsummary wideningは **source-breaking changeになり得る**。

summary narrowingはdependency checker上callerを悪化させない。

compiler / docs / IDEはsummaryを可視化できることが望ましい。

v0ではsummaryをone semantic compilation unit内で再計算してよく、
stable module artifactへserializeする義務はない。

future module / separate compilationでtransportする場合は、
source-visible private field名ではなくopaque projection identity等のsemantic metadataを用いてよい。

### no general source-level effect system

v0はこのために:

- user-written `effects(...)`
- source-visible effect polymorphism
- general effect subtyping
- general heap reachability / shape effect language
- source-visible general post-current-value / relational state transformer language

を導入しない。

必要なのはcompiler-internal place-relative summary algebraとsymbolic substitutionである。

## 13.6 domain granularity

domain の粒度は library / abstraction policy である。

例:

- owner<T>: object 単位
- arena: arena 全体
- ring: ring 全体
- pool: pool 全体

coarse domain は unrelated object の lifetime end まで保守的に禁止し得るが、v0 では許容する。

## 13.7 implicit local lifetime authority

通常の lexical local binding に対して programmer が毎回 `LifetimeDomain` を明示する必要はない。

compiler は local object に必要な lifetime-ending authority と governing identity を implicit に管理してよい。

local から ref が作られる場合、compiler は ref の hidden scope dependency をその implicit stability loan に結び付ける。

local由来のpersistent `ptr<T>` から、そのlocalがcurrentにliveな間にsafe refを再取得する場合も、
§10.1に従い、このimplicit stability loanをcompiler-managed stability evidenceとして使用してよい。
これはptrだけからstabilityを推論する規則ではなく、compilerがptrのcurrent incarnationと
そのlocalのimplicit governing identityの一致を証明できる場合に限る。

local から ref が作られている間、その local を consume / transfer / destroy して lifetime を終了させる conflicting operation は禁止される。

## 13.8 loan scope と surface syntax

**Provisional**

loan acquisition の最終 surface syntax は Draft 12 でも固定しない。

core semantics として必要なのは:

- place から ordinary ref を loan する primitive
- place から exclusive ref を loan する primitive
- 生成 capability の scope を **exactly-once lexical block** に限定すること

loan body は ordinary 0..N callable block ではない。

概念的には:

```text
loan_read(life) { |stable|
    ...
}
```

の `{ ... }` 部分は一度だけ評価される lexical block であり、
block 内では outer non-Copy binding を通常の value-use 規則に従って consume できる。

function-shaped primitive か dedicated special form かは surface syntax の問題として保留する。
semantic 上は caller の place に loan state を作る compiler primitive である。

### Draft 17.18 bounded local-root source / result profile

Issue #108のNorth Star local-root scalar gateに限り、次のclosed source mappingを
**Provisional** として固定する。

```text
direct_local_read_loan
    := loan_read '(' local_name ')' loan_body

local_ptr_read_loan
    := loan_read_ptr '(' ptr_name ')' loan_body

ref_to_ptr
    := ptr_from_ref '(' ref_name ')'

loan_body
    := '{' '|' ref_name '|' block_item* [tail_expr] '}'
```

`block_item` / `tail_expr` のsequencingとnormal block resultは§19.1に従う。
`loan_body` の `|ref_name|` はloan-local capability binderであり、
このbodyを§19.2の0..N callable blockへ変換するものではない。
bodyはexactly once評価されるlexical blockである。

このbounded profileでは:

- `local_name` はcurrent live ordinary lexical local rootを指すsimple local binding nameに限定する。
  field/subobject projectionやgeneral place grammarを本profileへ追加しない。
- `ptr_name` はordinary local bindingに保持された `ptr<T>` に限定する。
  arbitrary ptr expressionを本profileへ追加しない。
- `ref_name` はそのloan body内だけで有効なfresh ordinary lexical binding nameである。
- `loan_read(local_name)` は§11.1 / §13.7のimplicit local stability loanを開始し、
  body内の `ref_name` を `ref<read,T>` とする。
- `ptr_from_ref(ref_name)` は§10.2のsafe total ref->ptr operationへ写像する。
  result `ptr<T>` はlocation/provenanceを保持するが、
  originating ref/loan scopeを存続させるblocking dependencyを自動的には持たない。
  他のscope / semantic / backing dependencyがresult semanticsとして存在する場合はそのまま保持する。
- `loan_read_ptr(ptr_name)` は§10.1のsafe ptr->ref operationを要求する。
  compilerはptr provenanceが指すcurrent `T` incarnationと、
  current live lexical local rootの§13.7 implicit governing identityが一致することを証明し、
  そのlocalのhidden stability loanをstability evidenceとして使わなければならない。
  ptrだけからstabilityを推論してはならない。
  required alignment / range / valid representation / read access /
  current conditional occurrence / additional semantic dependency等、§10.1のpreconditionは省略しない。
- Draft 17.18で固定したこのread profile自体はread-onlyである。
  Draft 17.19は下記の別bounded formとしてdirect lexical-local `u8` write loanだけを追加する。
  §10.1 / §11のwrite semantics自体は変更しない。

### Draft 17.19 bounded direct-local write source profile

Issue #113のstable-root mutation North Star gateに限り、次のclosed source mappingを
**Provisional** として固定する。

```text
direct_local_write_loan
    := loan_write '(' local_name ')' loan_body
```

`loan_body` は上記Draft 17.18 profileと同じexactly-once lexical body grammarを再利用する。

このbounded write profileでは:

- `local_name` はstatic typeがcore `u8` のcurrent live ordinary lexical local rootを指す
  simple local binding nameに限定する。
- field/subobject projection、arbitrary place expression、ptr operand、aggregate root、
  other typesは本profileへ追加しない。
- body binderはfresh ordinary lexical bindingで、static capabilityはexactly
  **ordinary `ref<write,u8>`** である。
- generated write refは§11.1どおりscope-bound / non-escapingであり、
  local rootの§13.7 implicit governing identityに対するcompiler-managed stability loanへ依存する。
- programmer-visible `LifetimeDomain` / Allocation / Storage / raw backingは要求しない。
- local rootに必要なordinary write accessが成立していなければこのsafe write loanは成立しない。
- `ref<write,u8>` は§11.2どおりexclusiveではない。
  capabilityの生成によってunique ownership、exclusive ref、lifetime-ending authority、
  LLVM `noalias`相当の保証を導入しない。
- same placeを指すordinary refsが他にliveでも、それだけでwrite loanやmutationを禁止しない。
  actual mutationの合法性は§13.5a / §13.5b / §17.4のexisting dependency/effect conflict ruleで判定する。
- Draft 17.19の `loan_write(local_name)` spellingはIssue #113のdirect-local bounded profileとして維持する。
  Draft 17.20は§17.1でregistered Pairのone-level fixed-fieldだけを別のclosed profileとして追加する。
  それ以外のricher operand/type profile、general/final write-loan syntax、same-spelling ordinary declarationとの
  language-wide parser/name policyは固定せず、profile外を本revisionだけでlanguage-invalidとは決めない。

#### loan normal-result forwarding

`loan_read` / `loan_read_ptr` / `loan_write` は、normal completionに関してvalue-producing expressionである。
bodyがnormal completionしてresult package `R` を生成した場合、conceptually次の順序で扱う。

```text
1. evaluate loan body exactly once and obtain R
2. check scope-exit compatibility for the ending loan scope
3. if incompatible: reject
4. end the generated ref / hidden stability loan
5. expose the unchanged R as the enclosing loan-expression result
6. only then continue outer value flow / receiving / use
```

step 2では、`R` だけでなくboundary後へsurviveするcurrent-value stateについても
§13.5aの通常のscope-exit compatibilityを適用する。

forwardingはdependency laundering boundaryではない。

```text
Deps(loan_normal_result) = Deps(R)
```

したがって:

- body resultがloan-bound `ref` 自体、またはending loan scopeへのscope dependencyを
  transitively保持するpackageならrejectする。
- `ptr_from_ref(r)` のresult `ptr<T>` は、§10.2および
  「persistent provenance is not a blocking loan」の規則に従い、
  ending loan scopeへのblocking dependencyが無ければforwardできる。
- crossingによってloanと無関係なdependencyを削除してはならない。
- resultのstatic typeだけを見てescape可否を決めてはならない。

forwardされたresultには通常のvalue-flow / receiving ruleを適用する。
write loanについても追加のscope-exit result categoryは存在しない。
body内mutation後のcurrent-value stateがending write-loan scopeに依存してsurviveする場合は、
step 2の既存scope-exit compatibilityがrejectするため、write専用post-state ruleを追加しない。

例えば:

```newlang
let p = loan_read(x) { |r|
    ptr_from_ref(r)
};
```

は、body resultがscope-exit compatibleである場合、
loan scope終了後にordinary `let` binding `p` が同じ `ptr<T>` packageを受け取る。
loan専用receiverや追加result/effect categoryは存在しない。

local-derived ptrからのreacquisitionは例えば:

```newlang
loan_read_ptr(p) { |r2|
    ...
}
```

と書く。
このformが成立するのは、`p` のprovenanceがcurrent live local incarnationを識別し、
§10.1 / §13.7のhidden stability evidenceをcompilerが構成できる間だけである。
originating local incarnation終了後は、persistent `p` 自体が保持可能でも
safe reacquisitionはrejectする。
reacquired `r2` がliveな間は、そのlocalをconsume / transfer / destroyする
conflicting lifetime-ending operationも既存ruleによりrejectする。

上記 `loan_read` / `loan_read_ptr` / `ptr_from_ref` spellingsは、
Issue #108のbounded product profileでのみsource meaningを固定する。
一般/final loan syntax、bounded profile外のwrite-loan spelling、general place/ptr operand grammar、
same-spelling ordinary declarationとのlanguage-wide parser/name policyは引き続きProvisionalであり、
profile外入力を本revisionだけでlanguage-invalidとは決めない。

Draft 10以降のdynamic-container span materializationも、§25.10に従い、
このexactly-once lexical loan-body mechanismを再利用してよい。
spanのために別種のescaping lifetime mechanismは追加しない。

---

# 14. initialize / take / destroy

## 14.1 initialize

conceptual primitive:

```text
initialize(
    slot<T>,
    value: T,
    ref<read, LifetimeDomain>
) -> ptr<T>
```

は `slot<T>` のempty occupancy claimを消費し、
同じ BackingRegion / range 上で
新しい **lifetime-root `T` object incarnation** を開始する。

ordinary-safe `initialize` はさらに、
destination BackingRegion / rangeが`T`のvalid representationを成立させるために必要な
target-defined write accessを持つことをpreconditionとする。
ordinary writable memory profileでは、これはdestinationへのordinary write permissionを意味する。
compilerがstoreを最適化で除去できる場合でも、このabstract-machine access preconditionを弱めてはならない。

このときdestination rootのfresh placement backing relationは、
consumeした`slot<T>`のBackingRegion identity / exact rangeから作る。
`value: T` がどのsource placementから来たかはdestination placementを決めない。

渡された ordinary domain ref が参照するdomainを、
そのincarnationの governing domainとして **fresh root-state relation** に記録する。
このrelationは `value: T` の ValuePackage から復元/継承するものではない。

lifetime開始自体は既存incarnationのlifetimeを終了させないため、
ordinary domain refでよい。

`slot<T>` のaffine claimは消滅するのではなく、
live root object stateへ移る。

返された `ptr<T>` はpersistent tokenであり、domain refより長生きしてよい。
後にそのptrからsafe refを生成するには、current incarnationのgoverning domainに対応する
新たなstability evidenceが必要である。

### future privileged / foreign lifetime-start extension point

ordinary safe v0 coreでは、`slot<T>`からtyped root lifetimeを開始するsource-visible operationは
引き続き本節の`initialize(slot<T>, value, domain)`だけである。

ただし本仕様は、future platform / FFI / localized `unchecked` extensionが、
`value: T`をordinary value-flowから受け取る代わりに、boundary contractによってdestination上に:

1. complete valid `T` representation
2. complete semantic `ValuePackage<T>`

が成立したことを保証し、同じfresh-root lifetime-start postconditionを確立することを禁止しない。
そのようなfuture transitionは少なくとも:

- destinationのunique empty occupancy responsibilityを消費する
- destination representationを成立させるために必要なtarget/platform access contractを満たす
- fresh root incarnation / placement / governing-domain relation / current-value factsを作る
- fixed / conditional structural stateをordinary lifetime-start semanticsに従ってfreshに作る
- lifetime-start完了後にのみ最初のsafe provenance-bearing `ptr<T>`を公開する

必要がある。

raw representation bytesだけから`Allocation` / `Storage` / `LifetimeDomain` identity、ptr provenance、
hidden dependencyその他のvalue-owned semantic authorityを推測/mintしてはならない。
必要な`ValuePackage`がtrivialでない型では、foreign contract / wrapperがそのsemanticsを別途成立させる必要がある。

partial raw byte writeはtyped lifetime-startではない。
foreign operationが一部bytesだけを書き換えて失敗した場合も、上記条件が成立しない限り`slot<T>`はsemanticにはemptyのままである。

このsubsectionはfuture extension pointのcompatibility reservationであり、Draft 17.1に新しいsource primitiveを追加しない。

## 14.2 take

conceptual transition:

```text
take(
    ptr<T>,
    exclusive ref<read, LifetimeDomain>
) -> (T, slot<T>)
```

第二argumentのexclusive domain refをexisting exclusive-ref bindingから渡す場合、
§12.1の **operation-local exclusive reborrow** を用いる。
callerのouter exclusive-ref bindingそのものはconsumeされず、
operation中だけconflicting useがsuspendされ、operation終了後に再びusableになる。

これはlifetime-ending authority requirementを弱めるものではない。
child exclusive refは、引き続きtarget rootのgoverning domainと同じdomainを参照しなければならない。

`take` は対象 **lifetime-root** object incarnationのlifetimeを終了し、

- old semantic value `T`
- 同じ BackingRegion / range のtyped placeに対する definitely-empty `slot<T>` claim

を返す。

source rootのplacement backing relationと governing-domain relationはroot lifetime endとともに終了し、
returned semantic value `T` へtransferされない。
返される`slot<T>`が同じBackingRegion/rangeのoccupancy responsibilityを引き継ぐ。

少なくとも以下をpreconditionとする。

- `ptr` が現在liveな `T` object incarnationを指す
- target incarnationがindependently lifetime-endingな **lifetime root** である
- そのincarnationのgoverning domainが `D`
- 渡されたexclusive domain refが **同じ `D`** を参照する
- source current semantic value `T` をordinary value-flowへ取り出すために必要な **ordinary read access** が成立する
  - explicit BackingRegion上のrootでは、そのsource backingがordinary read accessを許す
  - ptr tokenが保持するaccess evidenceもreadを許す
- target / overlapping stateの終了によりinvalidになるdependencyを持ち、かつ`take` result等としてtransition後へsurviveするconflicting capability/valueがない
- その他operationに必要なprovenance / representation条件が成立する

compilerがcurrent liveness / root status / domain correspondence等を証明できなければ
`unchecked` が必要になり得る。

ただし `unchecked` は必要なexclusive domain capabilityを省略する手段ではない。
またfalseなroot assumptionはUBである。

### compiler-managed local

ordinary lexical localのconsume-outでは、
compilerがimplicit lifetime root / storage claimを内部管理してよい。

source-level ordinary transfer:

```text
let y = x
f(x)
return x
```

でnon-Copy `x` をconsumeする場合も、
semanticにはsource root incarnationを終了してvalueをtransferする。

source storageをprogrammerへ返す必要がない場合、
empty claimはcompilerが内部的に処理してよい。

### explicit storage

explicit slot/storage managementでは、
`take` 後のempty claimを `slot<T>` としてsurface APIへ返せる。

これにより:

```text
slot<T>
  -> initialize
  -> live root T
  -> take
  -> slot<T>
```

がaffine claimを失わずに閉じる。

## 14.3 destroy

`destroy(p, ending)` の `ending` も `take` と同じく§12.1のoperation-local exclusive reborrowに従う。
existing outer exclusive-ref bindingをargumentとして使っても、そのouter binding自体はconsumeされない。

`destroy` はvisible value / occupancy semanticsとしてconceptually:

```text
(value, slot) = take(...)
discard(value)
return slot
```

である。

ただしdependency-survival checkingおよびaccess applicabilityでは、
`take`を独立に成立させた後に`discard`するliteral desugaringとはみなさず、
old semantic value packageを外部へ返さず終了させる **一つのdestroy transition** として扱う。

従ってordinary native `destroy` は、old `T` valueをcaller-visible ordinary valueとしてmaterializeしないことだけを理由に
§14.2のordinary read-access preconditionを継承しない。
write-only backing上のrootでも、lifetime-ending authorityその他の`destroy` preconditionを満たすなら
ordinary-safe `destroy` を許してよい。

ただしtarget/platform固有のbacking contractがlifetime end自体に追加accessを要求する場合は、そのcontractに従う。

従ってold value内部だけに存在し、destroy transition後へsurviveしないdependencyは、
そのold package自身の終了をblockしない。
外部alias / incoming state等に同じdependencyがsurviveする場合は通常どおりconflictする。

従ってstatic applicability condition:

```text
Discardable(T) == true
```

を要求する。

non-Discardable `T` のlifetimeを終了したい場合は `take` してvalueをcaller側で明示的に処理する。

automatic destructor / Drop protocolは導入しない。

## 14.4 fixed subobject を単独終了しない

live ordinary aggregateのfixed fieldやlive native array elementは、
原則としてindependent lifetime rootではない。

従って:

```text
take(ptr_to_field(pair, .a), ending)
```

によって:

```text
Pair live
a dead
b live
```

のようなordinary partially-live aggregate stateを作ることはsafe operationではできない。

partial construction / dynamic container occupancyは、
Storage / slot / §15のrooted dynamic-region responsibility mechanismで扱う。

## 14.5 LifetimeDomain transfer

`LifetimeDomain` の **value transfer** と **domain identity finalization** を分離する。

LifetimeDomain valueをnon-Copy transferしてもdomain identity自体は維持される。

例:

```text
let d2 = d1
```

では:

- source object incarnation `d1` はconsumeされる
- destination object incarnation `d2` に同じdomain identityがtransferされる
- そのidentityにgovernされるlive objectsはそのまま残ってよい

governing relationはLifetimeDomain valueのmachine addressではなくdomain identityに結び付く。

ただし source/current domain identityにvalue-dependentなordinary/exclusive capabilityがliveなら、
通常のdependency conflictによりtransferはできない。

## 14.6 LifetimeDomain finalization

domain identity自体を終了するexplicit operationを **finalization** と呼ぶ。

surface syntaxは未確定だが、conceptually:

```text
finalize_domain(domain: LifetimeDomain) -> unit
```

相当を想定する。

finalizationは少なくとも:

> そのdomain identityにgovernされるlive object incarnationが残っていない

ことをpreconditionとする。

さらに、そのdomain identityにsemantic-dependentなcapabilityがliveであってはならない。

`LifetimeDomain` はnon-Discardableなので、
domain identityを暗黙discardしてfinalizationを迂回することはできない。

value transferだけではfinalization preconditionを要求しない。

dynamic containerではmetadataに基づくlocalized `unchecked` が必要になり得る。

---

# 15. dynamic partial initialization / dynamic region responsibility

v0 は dynamic-index partial initialization / occupancy を
language core のper-element stateとして追跡しない。

方針:

> container metadata はsteady-stateのoperational occupancyを記述する。  
> authority / responsibilityは、BackingRegion/root originに結び付いたownerからtransferされる。  
> metadata自体はauthorityをmint / eraseしない。

例:

- Vec の `len`
- Ring の `head` / `len`
- Hash table bucket / control state
- allocator free-list / block metadata

`cell<T>` のようなper-element runtime occupancy bitを持つcore abstractionはv0に入れない。
compilerがarbitrary dynamic initialized-index setをtype stateとして保持することも要求しない。

## 15.1 rooted dynamic claim transfer

Hash tableやallocatorのようにempty/live subrangesがdynamicに散在する場合、
各rangeについてsource-level `slot<T>` / `Storage` valueを永続的に保持することは要求しない。

代わりにcontainer / runtimeはconceptually、BackingRegion/root originに結び付いた
**dynamic-region responsibility owner** の内部へvacant/live responsibilityを封じてよい。
このownerはsource-visible `SealedRegion<T>`等の特定型である必要はない。

selected locationのclaimを外へ出すprivileged operationは、conceptually:

```text
region owns vacant responsibility Q
    -> scoped vacant claim Q
       + region state in which Q is suspended/outstanding

region owns live T responsibility Q
    -> scoped live claim Q
       + region state in which Q is suspended/outstanding
```

という **transfer** である。

privileged boundaryがmetadataから確認するのは:

- selected locationがcontainer invariant上vacant/liveのどちらであるべきか
- root / range / alignment / layout等が期待状態と一致するか

である。
metadataはclaim authorityのoriginではない。
実状態とmetadata invariantが一致しないのにprivileged boundaryがclaimを公開した場合はNewLang UBである。

claimがregion外にある間:

- 同じresponsibilityからoverlapping claimを再生成してはならない
- region teardown / whole-storage returnでそのresponsibilityを無視してはならない
- live responsibilityがnon-Discardableならmetadata変更だけで消してはならない

claimはexisting lifecycle operationへ接続する。

```text
vacant responsibility
    -> slot<T>
    -> initialize
    -> live T responsibility

live T responsibility
    -> take
    -> value T + slot<T>
    -> erase_slot
    -> vacant responsibility
```

operation後のresponsibilityはregion ownerへ返すか、別の明示的consumerへtransferする。

exact source spellingはProvisionalである。
linear receipt/tokenを使う場合はnon-duplicating / single-consumption / root-boundでなければならない。
既存のnonescaping `block(...)` helperは、claimをlinear valueとしてthreadし各control-flow pathでconsume/returnするなら、
このprotocolのergonomic sugarとして使用してよい。
そのためだけにexactly-once callable categoryを追加しない。

dependent / privileged named hookはcontainer-specific operation implementationの選択に使用してよいが、
hidden responsibilityのownerを置き換えるauthority mechanismではない。

### no general metadata-derived authority reconstruction

v0 coreはconceptualな:

```text
metadata says vacant -> mint slot<T>
metadata says fully raw/vacant -> mint whole Storage
```

という一般authority reconstructionを定義しない。

特にmetadata / pointerだけを根拠とする一般的なwhole-storage recovery primitiveはFixしない。

explicit raw partitionsがcallerに存在する場合はsafe `erase_slot` / `merge`でfull-range `Storage`を再構成する。
dynamic regionがresponsibilityを封じている場合は、少なくとも:

- all hidden live responsibilities have been ended / transferred
- all hidden responsibilities are vacant/raw-compatible
- no scoped claim is outstanding
- no conflicting stable borrow is live
- no transition is active

ことを満たした上でregion/root ownerをconsumeし、**保持していたorigin responsibilityを返す**。
これはmetadataから新authorityを再構成するoperationではない。

## 15.2 steady-state metadata / transition-local responsibility

`metadata is the occupancy SSOT` は、外部から観測可能なsteady stateについての原則である。
すべての内部instruction boundaryでmetadataだけが全責任を表すことを要求しない。

push / insert / remove / grow等の途中では、一時的に:

```text
public metadata state
physical/object state
```

が一致しないことがあり得る。

その区間では、未公開または取り外し中のresponsibilityを
scoped claim / transition guard / equivalent library-private stateが保持しなければならない。
これはsteady-state occupancy metadataの不要な二重管理ではなく、
現在transition中のresponsibility ownerを明示するものである。

inconsistent public invariantの間は:

- safe observerへcontainerを公開しない
- reentrant callbackからcontainer stateを観測させない
- overlapping claimを別経路から公開しない

ことを要求する。

fallible operationをtransition中に含める場合は、failure pathがcontrolを外へ返す前に:

- input responsibilityをcallerへ返す
- old region stateをrestoreする
- またはnew valid steady stateへcommitする

のいずれかをexactly once行わなければならない。
その保証を与えられないfallible external operation / callbackはtransition開始前に完了させる。

## 15.3 source/API surface status

Draft 17.3でFixするのはauthority semanticsであり、具体surfaceではない。

Provisional:

- linear receipt/tokenのexact type / spelling
- prefix/two-range helper API
- region consume -> whole `Storage` のsurface spelling
- compiler-private vs library-visible authority carrier

safe Vec-like prefix helper、ring two-range helper等をlibrary layerに置き、
一般claim protocolのproof burdenをcommon operationへ吸収してよい。

v0 coreはdynamic containerのために:

- arbitrary initialized-index set solver
- dependent occupancy type
- persistent raw-range capability
- general metadata-derived authority reconstruction

を追加しない。

---

# 16. aggregate / partial initialization / whole-value destructuring

## 16.1 ordinary aggregate value

ordinary binding 上の live aggregate について、
v0 は non-Copy field の **ordinary partial move** を提供しない。

例えば `pair: Pair<A,B>` から non-Copy field `a` だけを consuming extraction し、

```text
pair = partially moved
```

という binding state を作ることはしない。

non-Copy constituent を取り出す場合は aggregate 全体を value-use して destructure する。

v0 の最小canonical source formとして、semantic contextで既知のfixed-shape nominal aggregateに対し:

```text
aggregate_binding := 'let' TypeName '{' field_name (',' field_name)* [','] '}' '=' expression
```

をwhole-value destructuring bindingとして固定する。

例:

```text
let Pair { a, b } = pair;
```

このclosed formでは:

- aggregateは少なくとも1 fieldを持つ。
- patternはそのaggregateの全fieldをexactly once指定する。
- duplicate / missing / unknown fieldはcompile error。
- pattern中の各 `field_name` は同名のfresh local bindingも導入するshorthandとする。
- そのfresh local binding nameには§4.9および§21.8のordinary-name admissibility ruleを適用する。
  従って `let Type { unit } = expression` に加え、
  `let Type { match } = expression` 等もfield label自体ではなく、同名のreserved ordinary local bindingを導入しようとする点で不受理。
- field label `unit` / `fn` / `let` / `return` / `match` / `if` / `else` / `loop` / `continue` / `break` 自体を一般に禁止するruleは本revisionでは追加しない。
- renaming / rest / nested patternはこのformに含めない。
- RHSは一度だけ評価し、whole aggregate valueを得た後にdestructureする。
- RHSがordinary bindingのnon-Copy aggregateならそのbinding全体をconsumeする。
- RHSがCopy aggregateなら通常のvalue-useに従ってaggregate valueをcopyし、source bindingはAvailableのまま。
- fresh field bindingsはwhole destructuringが成功した後に一括してscopeへ導入される。途中のpartially-bound stateはobservableにしない。

これはtuple patternではなくnominal aggregateのfield-name based whole-value destructuringである。

`pair` が non-Copy ならこの destructuring は `pair` 全体を consume し、
fresh bindings `a`, `b` を生成する。

aggregate field が Copy の場合は、aggregateをconsumeせずfield valueだけをcopyしてよい。

例:

```text
let fd_copy = file@fd
```

`fd` が Copy なら `file` は Available のままである。

Draft 17.20のIssue #128では`local_name.field_name`がactual sourceとして固定されていた。
Draft 17.22は§17.1のlive source formを`local_name@field_name`へ改める。
なお上記`file@fd`はgeneric fixed aggregateの**概念的なCopy field read例**であり、
このrevisionのactual-source profileは引き続きregistered AVS
`Pair { left: u8, right: u8 }`のdirect local / one-level fieldに限定する。
general member-expression systemやgeneric unknown-field lookupを意味しない。

一部の field を変更した新しい aggregate が必要なら、
whole-value destructuring 後に再構成できる。

```text
let Pair { a, b } = pair

let next =
    Pair {
        a: transform(a),
        b: b
    }
```

functional-update sugar は将来追加できるが、v0 core semantics には不要。

### nominal non-Discardable と destructuring

`nondiscardable` restriction は hidden destructor / hidden obligation token を生成しない。

したがって:
```text
nondiscardable struct FileOwner {
    fd: i32
}
```

を whole-value destructure した場合:

```text
let FileOwner { fd } = file
```

`file` 自体は明示的に consume されたものとする。

以後の obligation は生成された field bindings の型に従う。
`fd: i32` が Discardable なら、compiler は `close(fd)` が呼ばれたことまでは証明しない。

representation を外部 code から直接 dismantle させたくない場合、
future module / visibility mechanismで field access / destructuring を制限する。

Draft 17.22の§28.6は将来のdefining-module-private representationを
no-foreclosure design directionとして明示するが、
現在のone visibility domainへ新しいprivate access enforcementを導入しない。

Draft 12 v0 はone visibility domainなので、
representation hidingに依存するabstraction enforcementまでは提供しない。

これはv0 prototypeの既知の制限であり、
将来visibilityを追加してもwhole-value destructuring semantics自体は変更しない。

## 16.2 aggregate construction / safe in-place partial construction

ordinary safe v0 coreでは、**まだliveでないaggregate rootのfieldだけを個別にlifetime-startする
safe field-by-field in-place construction protocolを持たない**。

ordinary aggregateはまずsemantic valueとして構築する。

v0 の最小canonical source formとして、semantic contextで既知のfixed-shape nominal aggregateに対し:

```text
aggregate_expr := TypeName '{' field_init (',' field_init)* [','] '}'
field_init     := field_name ':' expression
```

をaggregate value construction expressionとして固定する。

- aggregateは少なくとも1 fieldを持つ。
- declared fieldをexactly once指定する。
- duplicate / missing / unknown fieldはcompile error。
- field initializerはsource orderでleft-to-rightに評価する。
- field source orderとsemantic field identityは別であり、field名で対応付ける。
- 各initializerのCopy / consume / dependency semanticsは通常のexpression ruleに従う。
- constructionはcomplete semantic aggregate valueを一度に成立させ、target storage上のpartially-live aggregateを公開しない。
- aggregate declaration syntax、field shorthand construction、functional updateはこのclosed formに含めない。

例:

```text
let value = Pair {
    a: make_a(),
    b: make_b(),
};

initialize(pair_slot, value, stable_domain)
```

aggregate value construction中のtemporary value flowはordinary expression / binding semanticsであり、
target `slot<Pair>` 内にpartially-live `Pair` objectを公開することを意味しない。

`initialize(slot<Pair>, value, ...)` がwhole `Pair` root lifetimeを開始し、
その時点でfixed field/subobject incarnationsもordinary structural semanticsに従って開始する。

compiler/backendはobservable semanticsを変えず、途中のpartially-live target stateを
safe programへ露出しない限り、physical in-place constructionへ最適化してよい。

safe field-by-field in-place aggregate constructionがreal workloadで必要と確認された場合、
construction subobject / commit等の意味論を別途設計する。v0では **Deferred** とする。

final-address-dependent / self-referential constructionが必要な場合も、
それだけを理由にsafe future-incarnation `ptr<T>`やpartially-live fixed fieldを導入しない。
v0ではwhole-root lifetime-start後のexplicit fixup、logical handle / registry、
またはlocalized `unchecked` / foreign raw-location construction boundaryを用いる。
将来safe construction mechanismを追加する場合も、fixed fieldをindependent lifetime rootとして扱う必要があるとは限らず、
construction-only stateとして独立設計する。

dynamic-index partial initialization / custom container occupancyは§15どおり
container metadata + localized `unchecked` bridgeで扱う。

## 16.3 bounded recursive nominal header / exact completion

Draft 17.21はIssue #142のfirst lexical-root topology gateに限り、
one semantic compilation unit内で **at most one** bounded recursive aggregate declarationをadmitする。

closed source shape:

```text
bounded_recursive_aggregate_decl :=
    'struct' nominal_name '{'
        link_field ':' bounded_recursive_link_type ','
        payload_field ':' 'u8' [',']
    '}'

bounded_recursive_link_type :=
    'Option' '<' 'ptr' '<' target_nominal_name '>' '>'
```

semantic restrictions:

- `nominal_name` はordinary top-level nominal declaration nameとしてadmissibleでなければならない。
- `link_field` と `payload_field` はdistinct fixed-field labels。
- `target_nominal_name` はこのbounded profileでは **same declaration identity** `nominal_name` へ解決しなければならない。
  unknown target nameはunknown-type error、別nominal targetはprofile外である。
- field count / order / typesは上記exact shapeだけ。
- nominal restriction、additional field、nested/general member declaration、array、ref、slot、
  arbitrary generic field typeは本profileに含めない。

代表形:

```text
struct Node {
    next: Option<ptr<Node>>,
    payload: u8,
}
```

`Node` / `next` / `payload` 自体をspecial builtin nameにはしない。
上記はshapeの代表例であり、semantic identityは§28.3どおりsource spellingそのものではない。

### 16.3.1 header creation

bounded declarationのname-admissibility / duplicate top-level collisionを検査した後、
field type resolutionより前にそのdeclarationへstable nominal header identity `H` を割り当ててよい。

`H` は:

- same semantic compilation unit内でstable
- later exact completion後も同じnominal declaration identity
- source path / textual order / byte offset / backend C type nameとは独立

である。

header creationはaggregate value constructionではない。
incomplete stateはcompiler semantic stateであり、
programmer-visible `forward struct` / `incomplete` qualifierやC compatible incomplete typeを導入しない。

### 16.3.2 incomplete headerで許可すること

first gateでincomplete `H` がparticipateできるtype formationはexactly:

```text
ptr<H>
```

のdirect target identityだけとする。

`ptr<H>` は§4.4どおりCopy + Discardableであり、
target `H` のfield set / structural properties / physical layoutを要求しない。

このpermissionはrecursive declaration resolution用であって、
completion前の `H` について以下を許可しない:

- `H { ... }` aggregate value construction
- whole destructuring
- fixed-field projection / field lookup
- `Copy(H)` / `Discardable(H)` 等full-shape property query
- ordinary value/local/root creation of type `H`
- `ref<...,H>` / `slot<H>` 等をrecursion-breaking constructorとして扱うこと
- source-visible lifetime / layout / ABI query

このprofileで `ptr<H>` typeを形成してもptr valueは生成されない。
従って§10.3のsafe pre-lifetime ptr minting禁止を弱めない。

### 16.3.3 exact Option link instantiation

`bounded_recursive_link_type` の `Option<ptr<H>>` は、
§26のpredefined generic `Option<T>` を `T = ptr<H>` で具体化した
ordinary closed nominal sumである。

このexact instantiationについて:

```text
None
Some(ptr<H>)
```

だけを持ち、Copy / Discardable、constructor、match、conditional payload occurrence、
whole-sum replace/reset等は§26をそのまま適用する。

same semantic unitで同じ `Option<ptr<H>>` type source formが再出現した場合、
同じpredefined Option declaration + same type argument identityからなるsame concrete typeへresolveする。

本profileは:

- arbitrary `Foo<T>`
- general generic type parser
- generic inference
- user-defined generic sum declaration
- specialization

を追加しない。

completion後、exact `Option<ptr<H>>` type formは§26.3の `sum_type` として使用してよい。
従ってconstructor spellingは既存ruleの:

```text
Option<ptr<Node>>::None
Option<ptr<Node>>::Some(p)
```

であり、新しいnullable-ptr constructorを導入しない。

### 16.3.4 value-containment cycle validation

bounded declaration graphでは、aggregate fixed fieldとsum payloadを
**value-containment edge** とする。

first gateでselected recursion-breaking constructorは `ptr<T>` だけである。
`ptr<T>` からtarget `T` へのtype relationはtyped target relationではあるが、
value-containment edgeではない。

従ってsemantic cycle validationではconceptually:

```text
aggregate field edge      : follows
sum payload edge          : follows
ptr target edge           : stops value-containment traversal
```

とする。

bounded nominal header `H` からvalue-containment edgeだけを辿って `H` 自身へ戻るcycleが存在すればrejectする。

例:

```text
Node { next: ptr<Node> }                 // ptr barrier -> admissible criterion
Node { next: Option<ptr<Node>> }         // aggregate -> sum -> ptr barrier -> admissible
Node { next: Node }                      // unbroken by-value cycle -> reject
```

mutual declaration sourceは本profileではadmitしないが、criterionは将来:

```text
A { b: B }
B { a: A }                               // unbroken by-value cycle -> reject

A { b: ptr<B> }
B { a: ptr<A> }                          // ptr barriers -> candidate admissible
```

と拡張できる形を保持する。

この判定はsemantic type/value containmentに基づき、
C struct size / target ABI / physical offset / backend recursive-type acceptanceをoracleにしてはならない。

### 16.3.5 exact completion

field type resolution、selected Option instantiation、value-containment cycle validationが成功した後、
header `H` をexactly once completionする。

completionで:

- complete fixed field set / declaration-order field identityをcommit
- each field type identityをcommit
- opaque projection metadataを作成可能にする
- §4.5によりCopy / Discardableをderive
- aggregate construction/destructuringとfull-shape typecheckingをadmit可能にする

このbounded Node shapeでは:

```text
Copy(ptr<H>)                    = true
Discardable(ptr<H>)             = true
Copy(Option<ptr<H>>)            = true
Discardable(Option<ptr<H>>)     = true
Copy(H)                         = true
Discardable(H)                  = true
```

である。ただし将来nominal restrictionを追加する場合の§4.6は別途通常どおり適用する。

completionはidempotent APIではない。
same field setを二度completeする場合もduplicate completion errorであり、
different field set/typeを二度目に与える場合はinconsistent completion errorである。

semantic-unit declaration registration終了時にincomplete headerが残ればerror。
unknown field type / unresolved target / cycle error / duplicate/inconsistent completionを含む失敗では、
headerだけがpublic canonical contextへ残るpartial registrationを許さない。
semantic-unit registration transaction全体をrollbackする。

### 16.3.6 whole-unit collection / source order

本bounded declaration categoryについてfrontendはconceptually:

```text
collect bounded recursive aggregate declaration
    -> admit/check top-level name
    -> create nominal header H
    -> resolve Option<ptr<H>>
    -> validate value-containment cycle
    -> exact completion
    -> check value/function bodies
```

と処理する。

same semantic compilation unitではphysical file / textual declaration position / function positionにより
header visibilityまたはcompletion resultを変えてはならない。
value/function body checkingはrequired bounded type declarationsのcompletion後に行う。

これは§18.1 function signature collectionと同じくclosed declaration-category-specific ruleであり、
all future declaration categoriesへgeneral forward-reference semanticsを付与しない。

source-visible prototype / forward `struct Node;` / C-style incomplete declarationは追加しない。

### 16.3.7 existing runtime semanticsとのcomposition

completion後のNode objectはordinary fixed-shape nominal aggregateである。

- fixed fieldsは§3.8 / §14 / §17のexisting fixed-subobject identityに従う。
- `next: Option<ptr<Node>>` fieldはfixed field placeであり、そのcurrent Option payloadだけが
  §26のconditional occurrenceを持つ。
- persistent ptr payloadはtarget Node provenance/incarnation relationを保持するが、
  ptr value自体はtarget EndRootをblockしない。
- target EndRoot後のstored ptrはdangling tokenとなり、safe reacquisitionは§10.1でrejectされる。
- same physical locationにfresh Node incarnationが開始してもold ptrは復活しない。
- future field replaceで `next` valueがchangeする場合、fixed field/root incarnationはexisting field ruleどおりpreserveされ、
  old Option payload occurrenceは§26.7 / existing Reset ruleどおりendし、新payload occurrenceが必要ならfreshに開始する。
- sibling `payload: u8` current-value fact/incarnationはknown-disjoint sibling ruleどおりpreserveできる。

Draft 17.21ではこの§16.3はNode declaration/type graphだけをadmitし、
Draft 17.22時点のactual-source `@` field profileはPair/u8限定だった。
Draft 17.23では§17.1のclosed source profileをtargetedに拡張し、
**exactly completed bounded recursive nominalのcommitted link fieldだけ**を
`node@next` readおよび`loan_write(node@next)`のeligible fieldとする。
payload field / nested / ptr-base / ref-base / general Node member sourceは追加しない。
旧`node.next` spellingはDraft 17.23のlive field sourceとして不適格である。


---

# 17. typed location projection

## 17.1 live aggregate field

```text
ref<write, Pair> -> ref<write, A>
```

のような statically-known field projection は safe。

### Draft 17.25 exact H link ref-base source slice

§17.1冒頭に既に存在するtyped `ref<read/write,H>`から
statically-known fixed fieldへの**mode-preserving ref projection**に、
以下のclosed sourceを対応させる。
§17.23の`local_name@link` direct-live-local Copy read /
`loan_write(local_name@link)`とは**base categoryとresult typeが異なる**。

```text
bounded_ref_link
    := ref_name '@' field_name

bounded_ref_link_read
    := 'read' '(' bounded_ref_link ')'

bounded_ref_link_write_destination
    := bounded_ref_link   // operand to existing replace/store by ref<write,T>
```

- `ref_name`は現scopeでliveなordinary lexical **ref binding**であり、
  static typeはexactly `ref<read,H>`または`ref<write,H>`。
  baseのstatic ref-kind/target typeをfield spellingより前に確認し、
  §16.3 exactly completed `H`以外、ptr、aggregate valueや
  arbitrary expression baseはこのrouteでは扱わない。
- `field_name`は`H`に宣言・completedされた**committed**
  recursive `Option<ptr<H>>` link fieldのみ。別field、
  `payload:u8`、incomplete/unknown projectionはreject
  （同じ`@`categoryのまま、別candidateへfallbackなし）。
- `r: ref<read,H>` の `r@link` は
  exactly `ref<read,Option<ptr<H>>>`。
  `w: ref<write,H>` の `w@link` は
  exactly `ref<write,Option<ptr<H>>>`。
  **ref modeは常に維持し、readからwriteやexclusiveへの
  implicit conversionは禁止**。
  write refをread consumerへ渡す必要がある場合だけ§11.4
  explicit selected-contextのsafe weakeningが可能。
- projected refはfixed-childに対する既存の
  **opaque `ProjectionId`**を使用する。fieldのplace/incarnationは
  enclosing rootに従い、新たなindependent lifetime root/placement/backingを
  mintしない。root refのscope identityとそのD stability evidence、
  source root/provenance、current value/occurrence dependencyを
  追加・伝播させる。nested loanならscope shorter、outer escape不可。
- `read(r@link)`は既存§17.4で使うref readの
  **exactly `Option<ptr<H>>` Copy current-value operation**に
  限るselected builtin source spellingで、ptr provenance /
  hidden dependencyをそのままCopyする。生成されるfield refそのものを
  ptr-tokenへ変えたり、read後にwrite authorityを提供しない。
  `read(w@link)`も既存write->read weakening/read authorityで許可するが
  `r@link`を`replace`destinationにすることはできない。
- `replace(w@link, Option<ptr<H>>::Some(p))`や
  `replace(w@link, Option<ptr<H>>::None)`は、
  resolved `ref<write,Option<ptr<H>>>`を§17.4既存primitiveへ渡す
  **one bounded source expression operand**。
  `Change(L)`はlinkとancestor current factsを更新し、
  known-disjoint payload siblingとfixed root/link incarnationを保持する。
  §26 conditional `Some` payload occurrenceは`Reset`時に終了/再開始される。
  surviving Value(root/link)、Occurrence、scope dependenciesが
  invalidatedされるならreject; unknown effectをsafeと仮定しない。
- projected ordinary refsがliveでも、それだけではwriteは排他的にならない。
  noalias、automatic borrow/reborrow、implicit dereference、
  pointer ownership、field lifetime end権限を導入しない。
  source `p@link` with `p:ptr<H>`は依然未許可で、
  `r.link`、`r::link`、nested/other-field/general memberも未許可。
- `read` built-inはこのresult type/candidate fieldに**限定**する。
  general `read(ref<T>)`、arbitrary ref field syntax、generic/read API、
  copy-if-Copy over all typesのsource designは別途裁定を要する。

#### Minimal heap-head / lexical-tail witness (future P contract only)

```newlang
struct Node { next: Option<ptr<Node>>, payload: u8, }

match try_allocate_one<Node>() {
    None => { unit },
    Some(bundle) => {
        let OneBacking { allocation, raw } = bundle;
        let vacant = into_slot<Node>(raw);
        let life = lifetime_domain();
        let heap_head = loan_read(life) { |stable|
            initialize(
                vacant,
                Node { next: Option<ptr<Node>>::None, payload: u8(1) },
                stable
            )
        };
        let lexical_tail = Node {
            next: Option<ptr<Node>>::None, payload: u8(2)
        };
        let tail_ptr = loan_read(lexical_tail) { |r| ptr_from_ref(r) };

        let old_none = loan_read(life) { |stable|
            let root_w = ref_from_ptr(write, heap_head, stable);
            replace(root_w@next, Option<ptr<Node>>::Some(tail_ptr))
        };

        let seen = loan_read(life) { |stable|
            let root_r = ref_from_ptr(read, heap_head, stable);
            read(root_r@next)
        };
        match seen {
            None => { unit },
            Some(q) => {
                loan_read_ptr(q) { |tail_read| unit }
            },
        };

        let old_some = loan_read(life) { |stable|
            let root_w2 = ref_from_ptr(write, heap_head, stable);
            replace(root_w2@next, Option<ptr<Node>>::None)
        };

        let empty = loan_exclusive_read(life) { |ending|
            destroy(heap_head, ending)
        };
        let full_raw = erase_slot<Node>(empty);
        finalize_domain(life);
        deallocate(allocation, full_raw);
        unit
    },
}
```

この例は **actual-source design targetでありproduction/native実行実績ではない**。
本revisionで新たに認めるのは`ref_from_ptr(write,...)`、
`ref_name@link` mode-preserving projectionと、
`read(ref_name@link)` Copy current valueだけである。
fallible allocation、explicit root/domain、lexical tail ptr、
`match`、`replace`、`destroy`/raw return/finalize/deallocateは既存規則を再利用する。
`old_none`/`old_some`や`seen`はold Option value packageを保持するが、
persistent pointer valueそのものはrootのscoped refではなく、
ptr-origin/dependencyを洗浄せず保持する。
**generated refとそれに依存するvalueは各loan scope外へescapeできない。**

#### Required destructive/negative matrix (not executed tests)

| 試行 | 規則 / 結果 |
| --- | --- |
| `ref_from_ptr(read,p,stable)` + `read(r@link)` | read-only Copy、provenance保持 |
| `ref_from_ptr(write,p,stable)` + `replace(w@link,v)` | write permission証明時のみ、ordinary `Change(L)` |
| read-only root `r@link`→`replace` | **reject**、read→write amplification不可 |
| write rootだがbacking write permissionが無い | root ref生成を**reject**、domain stabilityだけでは不足 |
| ptrがdangling/EndRoot後・別incarnation/別BackingRegion | `ref_from_ptr`をreject、writeやfield projectionへ進まない |
| stableが別LifetimeDomain / absent / expired | `ref_from_ptr`reject、uncheckedで省略不可 |
| `ptr<H>@link` / implicit `*p` / arbitrary expr or nested `@` | profile外、ref acquisitionを迂回不可 |
| `r@payload` / `r@unknown` / incomplete H | reject、別source categoryへのfallback無し |
| `ref<read,H>`から`ref<write,Link>`への代入/変換 | reject、downstream write authorityをmintしない |
| derived link refをouter return/fieldへ保存 | reject、scope-dependent nonescape |
| live projected refを残し`destroy(heap_head)` | incompatible stability/end authority、reject |
| read current link, then conflicting `Change(L)` with surviving dependent ref/value | §13.5a effect/dependency check、必要ならreject |
| `None->Some`、`Some->None`、`Some(old)->Some(new)` | §26 old occurrence End / new fresh、fixed root/link preserved |
| known-disjoint payload sibling `Value(S)` | link-only `Change(L)`ではそのsibling factは保持 |
| second free / wrong BackingRegion / nonfull `Storage` | §3.2 existing affine/full-range rejection |
| actual emitted C guessed offset / host-injected write, copied lexical proxy | anti-gaming FAIL、semantic `ProjectionId`が唯一のfield authority |
| still-linked ptr token at EndRoot without live refs | ptr itself is nonblocking、unlink required by **product witness** not invented core error |
| richer source-valid ref projections beyond closed H/link | explicit backend unsupported / Deferred, not mislabeled unsafe |

本gateは**複数heap roots、heap→heap連結、owner transfer、detach、traversal、
recursive delete、general member/ref/ptr-base projection、implicit borrowing、
general loan syntax、FFI/C ABI/LLVM/layout、cJSON、new effects**を導入しない。
もし将来実装や独立reviewで§10/11/13/17/26のcore access/dependency invariantsを
変更しなければ正当化できない具体例が出たら、この候補をBLOCKしてCoordinationへ戻す。

### Draft 17.23 bounded fixed-field source profile

Draft 17.20では`.`で導入していたIssue #128のbounded Pair field sourceを、
Draft 17.22では`@`に置き換えた。
Draft 17.23では**同じ`@` grammarを維持したまま**、§16.3の
exactly completed bounded recursive nominal link fieldにsource eligibilityを限定拡張する。

先行するIssue #128のregistered AVS Pair shape:

```text
Pair {
    left: u8
    right: u8
}
```

については従来どおりのPair/u8 fieldsを認め、これに加えて
§16.3のexactly completed bounded recursive nominalのcommitted
`Option<ptr<Self>>` link field **だけ**を認める。
いずれもcurrent live direct lexical localをbaseとし、次の共通closed source mappingを
**Provisional** として固定する。

```text
bounded_fixed_field
    := local_name '@' field_name

bounded_copy_field_read
    := bounded_fixed_field

bounded_fixed_field_write_loan
    := 'loan_write' '(' bounded_fixed_field ')' loan_body
```

ここで:

- `local_name` はcurrent live in-scope ordinary lexical **direct local** bindingであり、
  static typeは(a)上記registered `Pair`、または
  (b)§16.3の**exactly completed** bounded recursive nominal `H`。
  static type `H` の判定はfield nameを検査する前に行う。
  incomplete header、別nominal、general generic typeはこのrouteへ入れない。
- `field_name` は(a)ではregistered Pairのdeclared fixed field
  `left` / `right`（各`u8`）、
  (b)ではexact completed `H` declarationの**committed recursive link field**
  （例`next`、exact type`Option<ptr<H>>`）に限定する。
  同`H`の`payload: u8`はfixed fieldだが**本source profileでは非許可**である。
- frontendはsource field spellingをcompleted nominal metadataのfield identityへ
  静的に解決し、opaque `ProjectionId`、fixed field identity/index、exact field type
  （`u8` または `Option<ptr<H>>`）を確定する。
  source field名やC ABI offsetをbackend semantic authorityとして渡さない。
- `@` は単一punctuationであり、operandはsimple current lexical local。
  nested `local@a@b`、arbitrary expression base、call result base、
  ptr/ref base、generic `T@foo`、unknown structural duck typingは本closed profileに含めない。
- `local.field`はfield readとして受理しない。`.`からfieldへfallbackしない。

#### Copy field read

expression positionの `local_name@field_name` は、target field current valueのordinary Copyを返す。
Pair/`u8`だけでなく、§16.3のcommitted linkの
`Option<ptr<H>>`も§16.3.5 / §26.4によりCopyである。
field place/incarnationやenclosing rootをconsumeしない。
result packageはfield current semantic value packageの通常のCopy semanticsに従い、
ptrのprovenance、hidden value/occurrence dependencies等を保持し、launderしない。
Copyした`Option<ptr<H>>`の`Some(p)`から得る`ptr<H>`は
target`H`のcurrent livenessを証明せず、safe ptr->refは§10.1を再度要求する。

#### ordinary write field loan

`loan_write(local_name@field_name) { |w| ... }` は、
enclosing local rootの§13.7 compiler-managed implicit governing identity / stability loanを使って、
body内に対象fixed field placeへのfresh ordinary `ref<write,T_field>` binding `w` を生成する。
`T_field`はPairならexactly `u8`、completed bounded Node linkならexactly
`Option<ptr<H>>`である。Node root全体へのwrite loanではない。

- generated capabilityは§11どおりordinary writeであり、exclusive / unique / noaliasではない。
- fixed fieldはindependent lifetime rootにならず、enclosing rootに従うexisting subobject incarnationを使う。
- bodyは§13.8の既存 `loan_body` と同じexactly-once lexical block。
- ref nonescape、scope-exit compatibility、unchanged normal-result forwardingは§13.8をそのまま適用する。
- Pairでの`replace(w,u8(...))`、Node linkでの
  `replace(w,Option<ptr<H>>::Some(p))` /
  `replace(w,Option<ptr<H>>::None)` は§17.4へ直接写像し、
  新しいmutation primitive・nullable ptr semanticsを導入しない。
- Node linkのwhole-sum replacementでは§26.7により
  `None->Some`、`Some->None`、`Some(old)->Some(new)`のいずれでも
  old conditional payload occurrence（あれば）をendし、
  new active payload occurrence（あれば）をfreshに開始する。
  固定Node root/link/sibling field自体のincarnationをend/recreateしない。
  `replace`のold-result semantic packageは従来どおり返され、
  surviving old occurrence/value dependencyが§13.5aに違反する場合はrejectする。

target field placeを`L`、enclosing fixed-shape Pairまたはcompleted Node rootを`R`、
known-disjoint sibling field（Nodeでは`payload`）を`S`とすると、
successful field replaceはexisting rulesにより:

```text
incarnation(R): preserved
incarnation(L): preserved
incarnation(S): preserved

Value(L): changes
Value(R): changes compositionally
Value(S): preserved
```

となる。

effectはexisting `Change(L)` であり、`Value(L)` またはoverlapするancestor `Value(R)` への
surviving dependencyとはconflictする。
known-disjoint sibling `Value(S)` dependencyは、LのChangeだけを理由にはconflictしない。

#### source category separation and no fallback

Draft 17.22ではsource punctuationがカテゴリを決める。
`simple_name@field_name` と `sum_type::variant_name`は
**互いに異なるclosed production** であり、
過去の`simple_name.member_name`共通candidate-resolutionを行わない。

```text
field route:
    local_name '@' field_name
    -> current in-scope live direct ordinary lexical local
    -> exact registered AVS Pair OR exactly completed §16.3 bounded H
    -> Pair left/right OR H committed recursive link (never payload)
    -> resolved opaque ProjectionId + exact type + Copy field read

sum constructor route:
    sum_type '::' variant_name ['(' expression ')']
    -> exact concrete closed nominal sum type
    -> its declared variant + exact arity
    -> nominal sum construction
```

- `@` routeは最初にbase lexical binding/static eligible category
  （registered Pairまたはexactly completed §16.3 bounded recursive nominal）を検査し、
  その後でfield existence / eligible fieldを検査する。
  Nodeについて`payload`またはunknown memberを指定しても
  categoryを取り消さず、`@` route内のdiagnosticとしてrejectする。
  unknown fieldでsum constructorやfuture member/receiver lookupへfallbackしない。
- `::` routeは最初にsum type categoryを検査し、
  その後でvariant existence/payload arityを検査する。
  unknown type/variant/arityでfield/receiver/ordinary function lookupへfallbackしない。
- `loan_write(local_name@field_name)`はbounded field designatorだけをoperandに取る。
  `loan_write(local_name::field_name)`、`loan_write(local_name.field_name)`や
  arbitrary place expressionを受理しない。
- `name`が同spellingのvalue-localとsum typeの両方に解決される場合でも、
  `name@left`はfield candidateだけ、
  `name::Some(v)`はsum type candidateだけを検査する。
  source category間のambiguous dotted errorやvalue-first/type-first precedenceは不要。
- `name.foo`はこのprofileのfixed-field expressionでもsum constructorでもない。
  将来のreceiver-call方向に予約するが、このrevisionではそのadmissionを追加しない。
- `@`はimplicit dereference / auto-borrow / nonexclusive ref amplificationを意味しない。
  完全にresolvedされたbase binding identity + field ProjectionId + field typeを
  semantic/checked representationに渡し、C offsetやtokenのbackend再解析は認めない。

以上はdirect Pair one-level fieldと§16.3 bounded `H` link-only one-level field /
existing closed sum constructorの区別だけであり、
general methods、`ptr@field`、`ref@field`、nested/general member expression、
module-qualified source、ADLやoverload rankingを導入しない。

#### Draft 17.23 Node link source / lexical ptr-root composition

```newlang
struct Node {
    next: Option<ptr<Node>>,
    payload: u8,
}

let tail = Node{next: Option<ptr<Node>>::None, payload: u8(2)};
let tail_ptr = loan_read(tail) { |r| ptr_from_ref(r) };
let head = Node{next: Option<ptr<Node>>::None, payload: u8(1) };

let old_none = loan_write(head@next) { |w|
    replace(w, Option<ptr<Node>>::Some(tail_ptr))
};

let link_copy = head@next;
let observed = match link_copy {
    None => { unit },
    Some(q) => {
        loan_read_ptr(q) { |tail_read| unit }
    },
};

let old_some = loan_write(head@next) { |w|
    replace(w, Option<ptr<Node>>::None)
};
```

これは**将来の2 lexical-root topology source witness**であり、
新field mappingをCoordinationがACCEPTした場合にabstract machine上で
構成可能な最小例を示す。**current production compiler/backendで実行確認した例ではない**。

- **already normative:** §16.3 Node exact declaration/completion/construction、
  §26 `Option<ptr<Node>>::None/Some`、`match`、`Copy`、
  §13.7/§13.8 generic local-root `loan_read(tail)`、
  §10.2 `ptr_from_ref`、§10.1 `loan_read_ptr(q)`のliveness/provenance/stability要求。
  ptr tokenは参照loan bodyを抜けても保持できるが、tailがcurrent liveであり
  implicit governing identityをcompilerが再立証できるときだけ再loanできる。
- **newly source-admitted by this candidate:** `head@next` Copy readと
  `loan_write(head@next)`というlink fixed-field ordinary write loan。
  `replace` / `Change(L)` / `Reset`は**新規semantic mechanismではない**。
- **production catch-up only (not authorized here):** completed Nodeの
  generic `loan_read` / `loan_read_ptr`で残るproduction-only u8 fence、
  Node link Copy/write checked evidence、required source tests、backend topology execution。
- **still Deferred / excluded:** raw allocation/new lifetime root construction、
  backend recursive Node lowering/Checked-C、general member/ptr-base `@`、
  Node payload field source projection、FFI、cJSON。

`tail`のEndRoot後や同じlocationでfresh `tail` incarnationが開始した後も
old `tail_ptr` tokenの保持・linkへのCopy格納そのものはptr値としてあり得るが、
`loan_read_ptr(tail_ptr)`によるold targetの再取得は§10.1でrejectする。
ptr existenceやlinkが`Some`であることからcurrent target livenessを推論しない。

#### Draft 17.23 bounded destructive source/semantic obligations

| ケース | 期待結果・既存根拠 |
| --- | --- |
| Pairの`p@left`/`loan_write(p@left)` | canonical Pair/u8 sourceを保持 |
| exactly completed current `head@next` | declared link `Option<ptr<H>>`のordinary Copy、正しいopaque ProjectionId |
| `loan_write(head@next)` | `ref<write,Option<ptr<H>>>` ordinary / non-exclusive、escape不可 |
| `head@payload` / `head@unknown` | Nodeはeligible baseでもfieldが非許可、reject、fallback無し |
| wrong base / incomplete header | eligible lexical local/static type/completion以前でreject |
| `a@b@c` / ptr- or ref-base `p@next` | closed one-level direct-local profile外、reject |
| old `head.next` / `loan_write(head.next)` | `.` はreceiver予約、field routeにfallbackせずreject |
| value-only `head::next` / `loan_write(head::next)` | `::`はsum constructor専用、reject |
| same spelling value-local+sum type | `@/::` punctuationでcategory分離、field/variant名が失敗してもcross fallbackしない |
| `Option<ptr<H>>::None/Some(p)` | §26.3の既存closed sum constructor、そのまま |
| `None -> Some` | link/root incarnation保持、fresh conditional payload occurrence |
| `Some -> None` | link/root incarnation保持、old payload occurrence End |
| `Some(old) -> Some(new)` | link/root incarnation保持、old occurrence End + fresh new occurrence（same variantでも） |
| surviving old `Occurrence`-dependent ref | ending occurrenceを必要とする外部refがsurviveすれば§13.5a/§26でreject |
| old copied `ptr<H>` token | ptr provenance/target identityを保持、link値の変更だけではtarget lifetimeを終えない |
| target EndRoot後のstale ptr | dangling tokenの保持自体は許し得るがsafe reacquisitionは§10.1でreject |
| same location fresh target incarnation | stale ptrはfresh incarnationを指すことにならない、§10.1でreject |
| active `Value(L)` / `Value(R)` dependency | linkへの`Change(L)`とoverlapならconflict |
| known-disjoint payload `Value(S)` dependency | link`Change(L)`のみではconflictしない |
| loan-generated link ref escape / invalid result | §13.8のnonescapeとscope-exit forwardingによりreject |
| `loan_write(head)` aggregate-root | §13.8 bounded write-root u8 profileにより未許可 |
| bad recursive declaration / failed field completion | §16.3 registration transaction rollback、partial headerやtyped field evidenceを残さない |

このmatrixは新しいptr dereference privilegeやlifetime/occurrence authorityを導入しない。
semantic checkerはfield identityとsource range、current value facts、dependency、
current root provenanceを検証し、旧dotted shared candidateのresolutionを再導入しない。

## 17.2 ptr field projection

```text
ptr<Pair> -> ptr<A>
```

は safe typed location derivation とする。

元 ptr が dangling でも projection 自体は location token の導出として可能。
derived ptr は元 object move に追従しない。

projected ptr は元の location derivation を semantic provenance として保持する。
fixed field / native-array elementへのprojectionは、そのsubobjectを独立 lifetime rootにはしない。
従って projected ptr を持つこと自体は、safe `take` authorityを生成しない。

## 17.3 partial aggregate construction

**Deferred**

v0 safe coreでは、uninitialized `slot<Pair>` からfield `slot<A>`等をprojectionして
fieldだけを`initialize`するoperationを提供しない。

特にDraft 15までのconceptual:

```text
pair_slot.with_field(.a) { |a_slot|
    initialize(...)
}
```

はv0 core semanticsから削除する。

理由は、`initialize` がfresh lifetime-rootを開始するという§14の意味と、
完成後のfixed fieldがindependent lifetime rootではないという§3.6 / §14.4の意味を
追加transition無しで両立できないためである。

ordinary safe constructionは§16.2のwhole semantic value + whole `initialize`を用いる。
dynamic container / runtimeのpartial occupancyは§15のauthority-preserving rooted dynamic-region bridgeを用いる。

FFI / localized `unchecked` codeがuninitialized aggregate storageの一部bytesを書き換えること自体は、
field subobject lifetime-startまたは`PartiallyLiveAggregate` stateを意味しない。
§14.1のfuture extension pointでcomplete typed rootを開始する条件が成立するまでは、safe coreから見たtyped occupancyはemptyのままである。

## 17.4 replace / store / swap

**Provisional**

`ref<write,T>` を通じてlive objectの **current semantic value package** を更新できる。

Draft 9以降の `replace` / `store` をobject lifetime-ending operationではない
**lifetime-preserving value transition** とする判断は維持する。Draft 12ではvisible valueだけでなく、
§13.5aのhidden dependencyもcurrent semantic value packageの一部としてtransitionする。

target object incarnationとgoverning `LifetimeDomain` は維持する。
fixed-shape aggregateではfixed field/subobject incarnationも維持する。

operation legalityは§13.5aのsurviving-dependency ruleに従う。

### replace

conceptual operation:

```text
replace(
    destination: ref<write,T>,
    new_value: T
) -> T
```

transition:

```text
before:
    place P
    incarnation = O
    current value = old

replace(P, new)

after:
    place P
    incarnation = O      // preserved
    current value = new

result:
    old
```

observableなempty stateは存在しない。

old semantic value packageはresultへtransferされ、
new semantic value packageは同じlive placeへtransfer-inされる。

conceptually:

```text
before:
    Current(P) = (old, D_old)
    Deps(new)  = D_new

after:
    result     = (old, D_old)
    Current(P) = (new, D_new)
```

old packageはresultとしてsurviveするため、old packageがtarget current value/occurrence等にblocking-dependentなら
そのdependencyは`replace` transitionをblockし得る。

`replace` はlifetime-ending operationではないため,
targetに対するexclusive refやexclusive LifetimeDomain authorityを要求しない。

old value responsibilityをcallerへ返すため、
`T` がnon-Discardableでも使用できる。

返されたold valueは通常のCopy / non-Copy / Discardable ruleに従う。

### store / overwrite

conceptual operation:

```text
store(
    destination: ref<write,T>,
    new_value: T
) -> unit
```

はvisible value semanticsとしてconceptually:

```text
old = replace(destination, new_value)
discard(old)
unit
```

と等価である。

ただしdependency-survival checkingでは、一度独立した`replace` resultを成立させてからdiscardするliteral desugaringではなく、
old packageを外部へ返さず終了させる **一つのstore transition** として扱う。

```text
before:
    Current(P) = (old, D_old)
    Deps(new)  = D_new

after:
    old package ended/discarded
    Current(P) = (new, D_new)
```

従ってold package内部だけに含まれ、store後へsurviveしないdependencyは、そのold package自身のoverwriteをblockしない。
incoming `new_value`、外部alias、他place等に同じdependencyがsurviveする場合は通常どおりconflictする。

従ってstatic applicability condition:

```text
Discardable(T) == true
```

を要求する。

これはruntime checkではない。

generic `T` はDiscardableと仮定できないため、
unconstrained generic bodyでは通常 `store(ref<write,T>, value)` を使用できない。

### swap

conceptual operation:

```text
swap(
    a: ref<write,T>,
    b: ref<write,T>
) -> unit
```

は二つのcurrent live `T` placeの **semantic valuesだけ** を交換する
lifetime-preserving transitionである。

argument expressionは通常どおりleft-to-rightに評価する。
両refの評価後にreferent relationを確定し、その後swap transitionを行う。

#### same place

`a` と `b` がsame current `T` object/placeを参照する場合:

```text
swap(a, b)
```

はsemantic no-opである。

- current valueは変わらない
- object/subobject incarnationは変わらない
- conditional occurrenceは終了しない
- `Change` / `Reset` effectを生じない

compilerがsame-placeを証明できない場合、effect checkingでconservativeに
distinct-placeの場合を含むmay-effectとして扱ってよい。

#### distinct places

`a` と `b` がdistinctなら、§3.5のsame-or-disjoint invariantにより
二つのcurrent live `T` object/subobject byte rangesはdisjointである。

conceptually:

```text
before:
    place A
        incarnation = OA
        current value = va

    place B
        incarnation = OB
        current value = vb

swap(A, B)

after:
    place A
        incarnation = OA      // preserved
        current value = vb

    place B
        incarnation = OB      // preserved
        current value = va
```

observableなempty / uninitialized / partially-live intermediate stateは存在しない。

各target object/subobject incarnationおよびそのplace側のgoverning lifetime relationは維持する。
visible semantic value、identity / authority / provenance-bearing component、hidden dependencyは
一つのsemantic value packageとして通常のvalue transfer semanticsに従って相手placeへ移る。

conceptually:

```text
before:
    Current(A) = (va, Da)
    Current(B) = (vb, Db)

after:
    Current(A) = (vb, Db)
    Current(B) = (va, Da)
```

両old packageは相手placeへsurviveするため、swapがinvalidatesするfactへのblocking dependencyを
同じswapで移動することを理由にconflictから除外してはならない。

`swap` は:

- `Copy(T)` を要求しない
- `Discardable(T)` を要求しない
- exclusive refを要求しない
- exclusive `LifetimeDomain` authorityを要求しない
- target lifetime rootを終了しない

従ってunconstrained generic `T` に対して使用できる。

#### fixed aggregate / conditional descendants

`T` がfixed-shape aggregateなら、各target placeのfixed field/subobject incarnationも維持する。
existing ptr/refはそれぞれsame field/subobject placeを指し続け、swap後のnew current valueを見る。

一方、whole-value swapによってcurrent sum value等が変わる場合、
old conditional descendant occurrenceは終了し、必要ならfresh occurrenceが開始する。
従ってdistinct-place swapは各targetに通常の`Change` / `Reset` semanticsを適用する。

#### not a concurrency atomic

`swap` の「一つのsemantic transition」は、future multithreadingにおけるhardware / synchronization atomicityを意味しない。
v0はsingle-threadであり、future threadingでは別途data-race / synchronization ruleを定める。

### aliasing ordinary refs

ordinary `ref<read,T>` / `ref<write,T>` はplace capabilityであり、
`replace` / `store` / `swap` のためにexclusiveである必要はない。

same placeを指すordinary refsがliveでも、
semantic-dependency conflictが無ければreplace/store/swapに使用できる。

single-thread v0ではstrict evaluation orderに従う。

```text
r_read  -> P
r_write -> P

replace(r_write, new)
read(r_read)
```

では `r_read` はreplace後もsame place `P` を参照し、
new current valueを見る。

future concurrencyでは別途synchronization ruleが必要である。

### surviving semantic-dependency conflict

targetまたはoverlapping placeのsemantic identity / occurrenceに依存するsemantic value packageがある場合、
`replace` / `store` / `swap` 等がそのdependency sourceをpreserveするか、
またdependent packageがtransition後へsurviveするかを判定する。

preserveできるoperationは許可できる。
dependency sourceをinvalidateしてもdependent packageが同じatomic transition内でunconditionally ended/discardedされるなら、
そのpackageだけに含まれるdependencyはpost-transition conflictを生じない。

一方、result / another place / outer memory / external alias等へsurviveするdependencyは通常どおりconflictする。

例:

- `State.domain` current domain identityへのdependency
- sum active payload occurrenceへのdependency
- lexical backing scopeへのdependency

このruleは§13.5aのsemantic value package / surviving-dependency relationを用いる。

### fixed aggregate

fixed-shape aggregateのwhole-value replace/store、およびdistinct-place whole-value swapでは:

- enclosing object incarnationを維持
- fixed field/subobject incarnationを維持
- existing field ptr/refはsame field placeを指し続ける
- replace/store/swap後はnew field current valueを見る

とする。

whole-value replace/store/swapはfixed field lifetimeを終了させない。

### address-sensitive values

v0はgeneric move / replace / swap時に:

- self pointer
- interior pointer
- intrusive structure link
- external registry
- callback context containing own address

等を自動修復しない。

semantic value内部のptrはvalue transfer前のaddressを保持したままであり、
replaceでold valueを返した場合も、swapでvalueを相手placeへ移した場合も、その内部ptrを自動retargetしない。

これはv0における意図的なabstraction boundaryである。

address-sensitive semantic invariantを守るlibraryは:

- physical address-sensitive objectをstable backingに置く
- logical handle / registry indirectionを公開する
- relocation時に既知のself/interior/link/registry stateを明示fixupする
- raw address handleを公開する場合は、relocationによるinvalidationをAPI contractとして明示する
- 必要な箇所だけlocalized `unchecked` boundaryを設ける

等で管理する。

重要:

> address-sensitive representation自体をunrestricted first-class by-value `T` としてcallerへ渡した場合、
> library APIだけではgeneric move / `swap` を完全には封じられない。

v0はone visibility domainであり、さらにwhole-value `swap` はfield visibilityを必要としないため、
representation hidingだけをstable-address invariantのenforcement mechanismとして扱ってはならない。

v0はPin / Unpin / nonreplaceable / nonmovable / nonswappable property / relocation traitを導入しない。
stable-address invariantをordinary safe abstractionとしてenforceしたい場合、
physical object自体をunrestricted movable client valueとして公開しない設計を優先する。

ただしstale `ptr<T>` はdereference authorityではない。
safe ref creationには従来どおりcurrent liveness / provenance / domain stabilityのpreconditionが必要である。

### evaluation order

memory value updateは§7のevaluation orderに従う。

destinationを評価した後、RHS / `new_value` の評価を完全に終え、
その時点のdestination current valueとnew valueをatomic semantic transitionとして交換する。

同じplaceへのaliasing mutationがRHS中に発生しても、
single-threadではこの順序によりdeterministicである。

`swap(a, b)` は `a` expression、`b` expressionの順に完全に評価し、
両referentを確定した後にsame-place no-opまたはdistinct-place exchange transitionを行う。

### sum type

payload sum typeではwhole-value replace/store/swapによりconditional payload subobject lifetimeが変化し得る。

whole-sum transitionでは:

```text
parent sum root incarnation:
    preserved

old active payload occurrence:
    ends

new active payload occurrence:
    begins fresh if the new variant has a payload
```

とする。

これはold/new variantが同じ場合も同様である。

payload自身への`replace(ref<write,Payload>, new)`では payload occurrenceを維持しcurrent semantic valueだけを更新する。

distinctな二つのwhole sum placeをswapする場合、各sum root incarnationは維持するが、
各placeのold active payload occurrenceは終了し、swap後のcurrent valueに応じてfresh occurrenceが開始する。
同一sum placeのswapはno-opなのでoccurrenceも維持する。

詳細は§26。

---

# 18. function

## 18.1 ordinary function

Draft 17.13で、ordinary **non-generic** function declarationのexact closed source profileを次に固定する。

```text
ordinary_function_decl :=
    'fn' function_name '(' [parameter_list] ')' '->' result_type lexical_block

parameter_list :=
    parameter (',' parameter)*

parameter :=
    parameter_name ':' parameter_type
```

ここで:

- `fn` はこのtop-level declarationを導入するstructural source wordであり、§21.8のcurrent structural reserved setに属する。
  lexer-wide keyword token classは要求しないが、exact spelling `fn` をordinary lexical nameとして導入してはならない。
- `function_name` / `parameter_name` はordinary lexical source nameであり、
  §4.9のdistinguished spelling `unit` および§21.8のcurrent structural reserved set
  `fn` / `let` / `return` / `match` / `if` / `else` / `loop` / `continue` / `break` を使用できない。
- parameter nameは同一function signature内で互いにdistinctでなければならない。
- zero parameterはexactly `()`。
- parameter listのtrailing commaは本closed profileでは許可しない。
- `:` は独立punctuationであり、通常のtoken間whitespaceは意味を持たない。
- `->` は隣接した `-` tokenと `>` tokenからなるcontextual two-character punctuator。
  `- >` はresult arrowではない。
- `parameter_type` / `result_type` はsurrounding v0 type source profileで表現可能なtype formであり、
  本profile自身はfunction type parameterを導入しない。
- result typeは常にexplicit。unit-result functionも `-> unit` と書く。
- declaration bodyはexactly §19.1の `lexical_block`。
- closing `}` の後にdeclaration semicolonは付けない。
- `fn f<T>(...)` のようにfunction nameの直後へgeneric parameter listを置くformは本closed grammarに含めない。

代表例:

```text
fn ping() -> unit {
    return unit;
}

fn id(x: Token) -> Token {
    x
}

fn exchange(a: ref<write,Token>, b: ref<write,Token>) -> unit {
    swap(a, b);
}
```

ordinary function declarationは§28.2のsingle ordinary lexical namespaceに属する
**top-level declaration**であり、§19.1 lexical blockの `block_item` ではない。
従ってnested/local `fn` declarationは本profileに含めない。

本profileだけのためにgeneral-purposeな全declaration category用
`compilation_unit := item*` source grammarを固定しない。
implementationは一つまたは複数のphysical source inputからtop-level ordinary function declarationsを収集してよい。
semantic unitへの所属とvisibilityは§28に従う。

### ordinary function declaration visibility

同じv0 semantic compilation unitに属する本profileのordinary function declarationについて、
**function nameとexact signatureはordinary function bodyのname resolution / definition-time checkingより前にsemanticにavailable**
であるものとする。

従ってsource / physical file上のdeclaration順序に関係なく:

```text
fn first(x: A) -> B {
    second(x)
}

fn second(x: A) -> B {
    ...
}
```

の `second` はordinary lexical functionとしてresolveする。

implementationはliteral two-pass parserを要求されない。
predeclaration table、whole-unit collection、lazy declaration discovery等、
observable name-resolution semanticsが同値な方式を使用してよい。

visibilityをphysical file path / file input order / textual declaration-before-useで変えてはならない。
本profileはprototype / forward-declaration-only source formを持たない。

same semantic compilation unitのordinary lexical namespaceに同じ `function_name` を導入する
二つのordinary function declarationは、parameter/result signatureが異なってもduplicate-name errorである。
v0はordinary function overload set / overload rankingを導入しない。

既に同じordinary lexical namespaceへ成立している別categoryのtop-level ordinary declarationと
同じnameを導入する場合もname collisionである。
その別categoryのsource declaration grammar自体を本節で新たに固定しない。

§21.7のassociated-function setはordinary lexical namespaceとは別candidate sourceなので、
associated set内に同じspellingのfunctionが存在しても、それだけでordinary declarationのduplicateとはしない。
本profileの `fn` declarationをassociated setへ暗黙registrationしてはならない。

self / mutual recursionについてもname visibilityは同じである。
例えば:

```text
fn recur(x: Token) -> Token {
    recur(x)
}
```

のcallee nameはself declarationへresolveする。
recursive SCCのsummary / dependency analysisは§13.5cに従う。
特定compilerが必要なfixpoint/precisionをまだ実装していない場合、
soundnessのためanalysis-precision rejectionしてよいが、
そのimplementation limitをsource-level name-resolution failureまたはrecursion禁止ruleとして扱ってはならない。

上記declarationの parameter は semantic には fresh initialized value binding である。

parameter type 自身が caller から callee へ渡すものを表す。

例:

- `T`: value
- `ptr<T>`: persistent location token
- `ref<read,T>`: scoped read capability
- `ref<write,T>`: scoped write capability
- affine authority: authority value

## 18.1a Draft 17.27 bounded ordinary known-call typed-owner applicability

**Selected closed source/semantic clarification (Issue #194; core authority laws unchanged).**
This clause applies only to a top-level ordinary non-generic, nonescaping, known-direct-called
`fn` in the same single semantic compilation unit as the Draft17.26 §3.2 two-H
source profile. Its function has **exactly** three authority-related parameters
of static types `ptr<H>`, `Allocation`, `LifetimeDomain` (names may differ),
returns `unit`, creates no additional allocation/root, and only (optionally)
performs a same-D bounded scoped read reloan before the already selected
`loan_exclusive_read(d) -> destroy(p, ending) -> erase_slot<H>(empty) ->
finalize_domain(d) -> deallocate(a, full_raw)` sequence. Here `H` is the
one §16.3 completed bounded recursive nominal. No recursion, indirect call,
forwarding of these obligations, early transfer/escape of the received
non-Copy authority, or further source category is selected by this clause.
The two `try_allocate_one<H>()` sites remain solely in the §3.2 nested
`main` profile; this receiver has zero allocation sites.

**Independent definition-time check remains mandatory.** The compiler registers
the ordinary declaration/signature under §18.1, then independently checks its
body with fresh **uncorrelated** symbolic parameter value packages. It must
type-check all operations, consume non-Discardable parameters on every normal
exit, reject local ref/dependency escape, invalid ordering and inherently
invalid operations, and derive the exact body-sensitive post-state. It must
**not** invent a live root, root/backing/domain equality, permission,
provenance, Storage, or owner value from the three parameter types or from
a hypothetical favorable caller. When a selected operation's otherwise valid
applicability cannot yet be discharged because of the **input relations
between these three symbols**, the definition checker may conditionally
derive and retain the following finite, compiler-inferred relational entry
requirements **rather than claiming that they are already true**:

```text
p : Param(ptr<H>), a : Param(Allocation), d : Param(LifetimeDomain)
O : the one currently live complete typed H root referred to by p
R : the existing live BackingRegion that contains exactly O's full H occupancy
D : the existing governing identity of O
RequiredAtEntry(p,a,d):
  p has valid current O provenance / incarnation, range and needed access
  O is governed by the same D carried by d
  R is the identical backing region carried by a
  full-range typed occupancy is unique; no simultaneously available raw claim
  ordinary/exclusive loan and hidden dependency scopes are compatible
  exact EndRoot -> empty slot -> full original R Storage recovery is provable
  a and d are the original available nonCopy affine authority values
```

These are **obligations**, not assumed facts, minted identities, values,
implicit `unchecked` or a signature-only safe precondition. The independent
checker proves the receiver body *conditionally* under them by applying the
existing §3, §10–14, §18.2–18.8 and §23 primitive rules in strict source order;
an unprovable body step not expressible by exactly these entry requirements,
an inconsistent requirement set or an unfulfilled normal-exit/escape obligation
must reject the **definition**. The compiler records the obligations and exact
relative post-state with the checked definition; no unrelated ordinary
functions acquire generalized typed-owner caller requirements by analogy.
Definition-time checking is never replaced by checking only one good call.

**Every actual known direct call is a separate mandatory proof.** After
left-to-right argument evaluation and the §18.2 consume/copy rules, but before
admitting callee execution, the compiler substitutes the actual
argument/provenance/root/region/domain and current memory/dependency relations
for `p,a,d` in `RequiredAtEntry`. It must prove **every predicate** from
the actual source-founded caller state. Unknown/may-set, numeric-address
coincidence, same static types, different BackingRegion/domain identities,
stale object incarnation, live conflicting loans, or a missing full-range
occupancy/recovery proof do **not** discharge any predicate. Reject the call
before backend whenever proof fails. Every call site in the semantic
compilation unit is checked; no unchecked human assertion may satisfy the
ordinary safe call. A compiler may conservatively reject a source-valid
call when it lacks precision, but may not treat the absence of proof as
success. In particular, the exact same-typed calls
`release(ptr_h, allocation_t, life_t)`,
`release(ptr_t, allocation_h, life_t)` and
`release(ptr_t, allocation_t, life_h)` must reject whenever the original
two distinct `H_h/H_t` roots are live.

For a proven matched `release(ptr_t, allocation_t, life_t)`, the
caller consumes the **existing** nonCopy `Allocation_t` and
`LifetimeDomain D_t`; the Copy `ptr_t` carries a location/provenance
token only. Fresh callee *parameter binding incarnations* receive the
value-owned original R_t / D_t identities; they do **not** create a new
heap root or retarget `ptr_t`. The separately placed current O_t remains
live until the receiver's existing exclusive same-D EndRoot, followed by
the **same** recovered full-R_t Storage claim, D_t finalization and
matching A_t deallocation, exactly once. No usable caller A_t/D_t remains.
The callee post-state, including `EndRoot(O_t)`, consumed authority,
cleared current lifetime/occupancy and retained stale-safe-pointer
restrictions, is substituted back to the caller under §13.5c/§18.7–18.8.
Earlier `Change`/`Reset` from the caller's `Option<ptr<H>>` link
detach concern only the head link occurrence; they neither produce nor
transfer the tail owner, and cannot be laundered through this call.

The three §3.2 fallible worlds remain distinct: first `None` has
zero roots/grants/release; second `None` cleans up only head once
without calling receiver; both `Some` permits the proven call to
release only tail in the callee and independently releases head in
the caller. No authority is imported from an unreachable branch.

**§20.4 boundary.** This is a *compiler-proven application condition*
for an ordinary safe direct call, not a human-proof contract or permission
to expose arbitrary `ptr/Allocation/LifetimeDomain` triples as a
universally safe API. The compiler must preserve and, on failure,
diagnose the inferred relational obligations. It is analogous only
in *proof strategy* to §24.1b's bounded `RawDefined` caller
requirements: raw definedness neither grants typed lifetime-ending
authority nor authorizes this clause outside its closed profile.
No source-visible `requires`, effect, lifetime or ownership annotation
is introduced. No generalized owner package, separate compilation,
callback/fnptr, receiver syntax, other dynamic root, general allocator,
implicit destructor/RAII, raw-to-typed reconstruction or FFI is implied.

## 18.1b Draft 17.28 bounded live-tail nonCopy result carrier and known return

**Candidate selection (Issue #203; unmerged):** Extend **only** Draft 17.27
§3.2's same-H two-static-fallible-site, one semantic compilation unit profile by
one additional top-level known-direct ordinary NON-generic producer:
fn detach_and_return_tail(
  head_link: ref<write,Option<ptr<H>>>,
  p:ptr<H>, a:Allocation, d:LifetimeDomain
) -> LiveTail.
It allocates zero H roots, never ends O_t/D_t/R_t and contains no
ref-dependent result, recursive/indirect/escaping call or extra allocation.
It first uses the **existing scoped head write-ref** to replace the current
H_h.next Some(p) by None; then returns the complete LiveTail value. The
separate head link ref is derived from live O_h under a D_h-scoped write
reloan. Neither original head Allocation_h nor head Domain D_h is passed
or consumed. The existing §18.1a terminal receiver is unchanged.
One selected both-Some branch may:
1. create the actual H_h.next=Some(ptr_t) while distinct O_h/O_t live;
2. within a head D_h-scoped reloan, call this producer with a real scoped
   head-link write ref plus original ptr_t, A_t and D_t; the producer itself
   performs head-link Some(ptr_t)->None;
3. receive and wholly destructure the returned LiveTail **after**
   the head ref/loan scope ends, optionally reloan using D_t, then call the
   existing terminal §18.1a receiver with the same original O_t/R_t/D_t;
4. separately end and deallocate O_h/R_h via A_h/D_h in caller main.
No other LiveTail producer/consumer source category is selected. This is
not a general owner interface.

### 18.1b.1 closed compiler-known nominal result and source spelling

After the single permitted §16.3 H declaration is fully completed, the compiler
may register **exactly one** H-specific, non-recursive, monomorphic,
fixed-three-field nominal `LiveTail` with:
~~~text
LiveTail {
    owned_ptr: ptr<H>,
    owned_allocation: Allocation,
    owned_domain: LifetimeDomain,
}
~~~
No source `struct LiveTail` declaration, generic `LiveTail<T>`, arbitrary
`Owner`, implicit constructor, destructor, owner trait, or user-chosen
representation/layout/ABI is implied. Just like the existing compiler-known
`OneBacking`, the name resolves to this one closed type in this profile;
a conflicting top-level ordinary declaration rejects. Derived `Copy` and
`Discardable` are both **false**, because the Allocation/Domain field values
are non-Copy and non-Discardable. `LiveTail` contains neither a live H
placement/occupancy root nor a `Storage`/slot claim.

Only the exact already selected §16.2
`LiveTail { owned_ptr: expr, owned_allocation: expr, owned_domain: expr }`
complete aggregate construction and §16.1
`let LiveTail { owned_ptr, owned_allocation, owned_domain } = expr;`
whole-value shorthand destructuring are exposed. Exactly all three fields
must be present once; field initializers evaluate left-to-right with ordinary
Copy/consume behavior and atomic complete value publication. No partial
aggregate move, field-by-field lifetime start, arbitrary field projection,
module privacy rule, pattern renaming or tuple/result syntax is selected.
The specific source constructor is admissible **only inside** the above
independently checked producer; the whole destructure is admissible only on
its known-return result in the selected both-Some caller. The nominal
field names are source spellings, not distinct authority categories.

### 18.1b.2 independent conditional producer definition check

Definition checker shall independently typecheck this producer using fresh,
*uncorrelated* symbolic formal parameter value packages. The only permitted
conditional input obligations for a LiveTail construction/return are
§18.1a's finite RequiredAtEntry(p,a,d) concerning the **existing** one current
complete O_t, same original full R_t and A_t, same governing D_t and d,
valid present ptr provenance/access, unique live typed occupancy, absent
simultaneous full raw claim, compatible loans/current dependencies and intact
eventual original full-R recovery after a **future** EndRoot. The producer
does **NOT** perform EndRoot or assume it has happened. Its input must have
sufficient future lifetime-ending/full-range recovery capability without
using or constructing raw Storage now.

A second, separately inferred, **finite head-link applicability** obligation
requires an independently valid scoped ref<write,Option<ptr<H>>> pointing to
the committed link field of a **different CURRENT H_h** on disjoint R_h and
governed D_h. At entry its current exact Option semantic value must be
Some(p) with the *same* provenance/current O_t identity as the tail ptr,
not just an address coincidence; caller evidence must show the ref's
read/write backing access and D_h-dependent loan scope, no conflicting
surviving link occurrence dependency, and correct relative field projection.
The head_link and p/a/d formals are uncorrelated in the independent
definition check. The receiver may assume these obligations **only
conditionally**, never from parameter static types or an imagined main.
The body performs exactly the existing replace(head_link,None) under
§17.4/§26.7, producing caller-visible Change/Reset without ending O_h/O_t.
The resulting LiveTail value must **not** depend on head_link or its
callee-local/head-D_h scoped capability. A body that cannot prove this
nonescape/result independence fails definition checking.

The definition checker may retain *unproved* finite relative O/R/D input
requirements exactly as §18.1a allows for its terminal body, and must
independently verify the body, complete constructor fields, left-to-right
Copy/consume, nonDiscardable exit compatibility, and result-nonescape.
No body assumption of a favorable actual caller, forged matched parameters,
host proof, or implicit `unchecked` is allowed. A no-caller invalid
definition rejects. A compiler incapable of retaining the correct symbolic
result relation must reject for precision, not infer it from three types.

On successful construction, the existing A_t and D_t authority **values**
move once from the producer parameters into one ordinary LiveTail value.
The Copy p token carries existing O_t provenance. The **compiler-internal
semantic correlation** carried by this value is:
~~~text
CarriesLiveH(live_tail, O_t, R_t, D_t):
    ptr field locates same CURRENT live O_t on existing R_t
    Allocation field contains exactly original A_t / R_t authority
    LifetimeDomain field contains exactly original D_t identity/authority
    O_t is typed-live with unique occupancy; no overlapping raw Storage
    no ending/free/reparenting/relocation of O_t or region is implied
~~~
This relation is **evidence**, not a new linear owner token, an assertion,
a duplicate Allocation, a backing region, a root or a warranty derived from
plain nominal static type. It is created only by compiler-proved constructor
applicability, not from field byte patterns or constructor spelling alone.
It must follow §13.5c / §18.5–18.8 hidden semantic value-package flow.

### 18.1b.3 every direct call, result, caller and terminal consumption

At **every actual known direct producer call** in the selected closed source,
after ordinary left-to-right argument evaluation, the compiler must prove all
the corresponding §18.1a RequiredAtEntry predicates from the actual
world-qualified CURRENT O/R/D/provenance/occupancy/loan/affine state. The
caller must **also** prove the above actual H_h link-ref/source/current
Some(p) correspondence, R_h disjoint from R_t, D_h-scoped ref liveness,
ordinary write permission, and the absence of surviving dependencies at
the replace. The producer, not a post-hoc C helper, changes the original
head link to None before returning the still-live tail obligation.
When the enclosing head loan scope expires, its result LiveTail cannot
depend on that head scope, and after the producer returns O_h/O_t remain
live; the caller later closes D_h only during independent head cleanup.
Wrong head ref, wrong current link payload, read-only scope or Unknown
value/provenance must reject even with a correctly matched tail triple. Mismatched
ptr/Allocation/Domain identities, Unknown or may-only proof, stale ptr/root,
live D_t-dependent ref or exclusive loan, consumed/duplicated authority,
no future full-R recovery proof, phantom None grants or numeric coincidental
IDs **must reject before backend**. No successful sibling call may stand in
for another call. The original donor A_t/D_t are then consumed by §18.2,
fresh producer formal bindings hold their original value-owned R_t/D_t
identities, and the separate heap O_t incarnation remains unchanged and live.

`return LiveTail {...};` applies §18.5/18.6 ordinary nonCopy consume-out
from the locally complete LiveTail value to the function result. The local
LiveTail *placement* incarnation ends; the returned value has a fresh result
placement, but the independent existing heap O_t placement, its R_t
backing and governing D_t relation do not move, end or become fresh.
The compiler's relative `CarriesLiveH` fact follows the result across
the known direct call to the fresh caller binding. It is not weakened to
type-only correlation or `Unknown`. If any live callee-local scoped ref or
current-value dependency would escape, §18.8 rejects.

The caller must wholly destructure the one nonCopy LiveTail result by
§16.1 exactly once. This consumes the package incarnation and transfers
the original A_t/D_t into fresh distinct field bindings while keeping
p's original Copy provenance. `CarriesLiveH` decomposes into the
corresponding *caller-evidenced* current O_t/R_t/D_t relations, not three
independently forged owners. Any subsequent actual §18.1a
`receive_and_release_tail(owned_ptr, owned_allocation, owned_domain)`
must **again** pass the existing every-call proof: same still-live O_t,
same full R_t, same D_t, no surviving scoped refs, no stale/unknown state.
That receiver's existing EndRoot → empty H slot → same full-R raw recovery →
D_t finalization → matched A_t deallocation remains unchanged; head cleanup
remains independent. The return/producer itself performs **zero frees**.
Its **one** head-link replace is a caller-visible post-state Change/Reset.
The caller's head loan must end before later D_h exclusive EndRoot; its
return result must survive that expiry without a head-scoped dependency.

First allocation `None`: no R_h/R_t, no producer/receiver, zero releases.
Second allocation `None`: only O_h/R_h/D_h exists and head's one
explicit matching release; no phantom LiveTail result. Both `Some`: two
distinct live O_h/O_t before detach, one LiveTail return with original
A_t/D_t and O_t still live, one later receiver tail release, one donor head
release. NonDiscardable normal-exit checks apply separately to all arms;
a result of a branch that has no tail must not be invented to unify them.
The preexisting pointer link `Some(ptr_t)->None` is a topology/product
observation: Copy link clearing alone is **not** producer authority and is
not a memory-safety precondition for packaging.

### 18.1b.4 full selected actual-source-shaped positive witness

The following is **proposed candidate source**, not admitted by canonical
Draft17.27 or executed by current P/F/native. It retains precisely two
syntactic `try_allocate_one<Node>` calls, solely in `main`.

~~~newlang
struct Node { next: Option<ptr<Node>>, payload: u8, }

fn detach_and_return_tail(
    head_link: ref<write, Option<ptr<Node>>>,
    p: ptr<Node>,
    a: Allocation,
    d: LifetimeDomain
) -> LiveTail {
    let old_link = replace(
        head_link, Option<ptr<Node>>::None);
    return LiveTail {
        owned_ptr: p,
        owned_allocation: a,
        owned_domain: d
    };
}

fn receive_and_release_tail(
    tail_ptr: ptr<Node>,
    tail_allocation: Allocation,
    tail_life: LifetimeDomain
) -> unit {
    loan_read(tail_life) { |stable_t|
        let tail_r = ref_from_ptr(read, tail_ptr, stable_t);
        unit
    };
    let empty_t = loan_exclusive_read(tail_life) { |ending_t|
        destroy(tail_ptr, ending_t)
    };
    let full_t = erase_slot<Node>(empty_t);
    finalize_domain(tail_life);
    deallocate(tail_allocation, full_t);
    unit
}

fn main() -> unit {
    match try_allocate_one<Node>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let allocation_h = allocation;
            let vacant_h = into_slot<Node>(raw);
            let life_h = lifetime_domain();
            let ptr_h = loan_read(life_h) { |stable_h|
                initialize(vacant_h,
                    Node { next: Option<ptr<Node>>::None, payload: u8(1) },
                    stable_h)
            };

            match try_allocate_one<Node>() {
                None => {
                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<Node>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let allocation_t = allocation;
                    let vacant_t = into_slot<Node>(raw);
                    let life_t = lifetime_domain();
                    let ptr_t = loan_read(life_t) { |stable_t|
                        initialize(vacant_t,
                            Node { next: Option<ptr<Node>>::None,
                                   payload: u8(2) },
                            stable_t)
                    };

                    let old_none = loan_read(life_h) { |stable_h|
                        let head_w = ref_from_ptr(write, ptr_h, stable_h);
                        replace(head_w@next,
                            Option<ptr<Node>>::Some(ptr_t))
                    };
                    let seen = loan_read(life_h) { |stable_h|
                        let head_r = ref_from_ptr(read, ptr_h, stable_h);
                        read(head_r@next)
                    };
                    match seen {
                        None => { unit },
                        Some(q) => {
                            loan_read(life_t) { |stable_t|
                                let tail_r = ref_from_ptr(read, q, stable_t);
                                unit
                            }
                        },
                    };
                    let LiveTail {
                        owned_ptr,
                        owned_allocation,
                        owned_domain
                    } = loan_read(life_h) { |stable_h|
                        let head_w2 = ref_from_ptr(write, ptr_h, stable_h);
                        detach_and_return_tail(
                            head_w2@next, ptr_t, allocation_t, life_t)
                    };

                    loan_read(owned_domain) { |stable_t|
                        let live_t = ref_from_ptr(read, owned_ptr, stable_t);
                        unit
                    };

                    receive_and_release_tail(
                        owned_ptr, owned_allocation, owned_domain);

                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<Node>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
            }
        },
    }
}
~~~

**Syntactic caveat:** this example uses only already selected
`fn`, `return expr;`, consuming match, nominal complete construct/whole
destructure, one selected H declaration, `loan_read`, `loan_exclusive_read`
and `@` projection source spellings **except** for new compiler-known
`LiveTail` source/category/result/correlation selected *only by this
candidate*. The producer and second known call after return are new
closed source admissibility, NOT implementation precision within
Draft17.27. The example is an independent safe-source design criterion;
it is not an assertion that a current compiler accepts it.

### 18.1b.5 adversarial falsification and hard boundary

| Case | Mandatory disposition |
| --- | --- |
| correct p_t/A_t/D_t; return, reloan, receiver free once | eligible conditional positive; preserve O_t live across return |
| p_h/A_t/D_t | REJECT: p targets O_h governed D_h |
| p_t/A_h/D_t | REJECT: A_h backs R_h, not R_t |
| p_t/A_t/D_h | REJECT: D_h does not govern O_t |
| head-link ref is None, points to Some(ptr_h)/other root, or is read-only | REJECT: actual current Some(ptr_t) and write-capability proof missing |
| producer's returned LiveTail depends on caller head-loan scope | REJECT: §18.8 escape, not a transferable owner result |
| construct after destroy(O_t) | REJECT: no current live typed H root |
| construct while overlapping full raw Storage claimed | REJECT: occupancy conflict; cannot mint a second claim |
| live D_t-dependent ref/exclusive loan at transfer | REJECT: nonCopy domain transfer/dependency conflict |
| discard, duplicate or Copy LiveTail or drop A_t/D_t on return path | REJECT: nonCopy/nonDiscardable/normal-exit rules |
| return only ptr_t with A_t/D_t unconsumed | REJECT: no complete nonCopy owner result |
| attempt partial aggregate extraction without whole destructure | REJECT: §16.1 no ordinary partial move |
| same name/type but wrong actual R/D in another call site | REJECT: independent every-call proof |
| stale ptr_t after receiver EndRoot/free | token may remain; safe reloan REJECT |
| first/second None tries to produce borrowed tail owner | REJECT: missing root/authority; no phantom Some |
| post-return caller passes same A_t/D_t twice | REJECT second use after consume |
| Unknown / may-only / cross-world numeric-ID coincidence | REJECT or conservative precision reject, never silently admit |
| general `LiveTail<Other>`, 3rd H, forwarding producer, module/indirect entry | DEFERRED source, not a proved current unsafe program |

All negative categories are semantic obligations/expected compile-time
rejections, NOT claims of existing compiler diagnostic names or tests.
If compiler cannot preserve the O/R/D relation across temporary construction,
callee return, fresh caller binding and destructure, it must stop with
analysis-precision rejection before backend. No source reparse/host C
post-hoc proof can fill missing authority.

### 18.1b.6 no-foreclosure / separated downstream evidence

This one monomorphic LiveTail is a **bounded evidence-carrying *value*
envelope**, not a mandatory universal owning container type or source-visible
contract syntax. A future general aggregate/Vec/intrusive graph may hold
or transfer authorities differently; reference child vs owning child is
application policy, not inferred from `Option<ptr<H>>`.
No general ownership graph, separate compilation, indirect callback, ABI/FFI,
external/overlapping safe BackingRegion, general allocator, RAII, deep
recursive delete, concurrency, generic inferred owner/effect system, private
module, general tuple, return of raw Storage with live H, or relocation
is selected. Maintain §3 disjoint region nonalias, §14.5 D identity,
§18.6 placement-vs-value, §20.4 no hidden human proof.
Old `requires fn` / unchecked experimental hook is not silently made safe.

If separately authorized later, production must first prove independently
checked uncorrelated producer definition; every known producer call's exact
O/R/D/affine/loan conditions; single LiveTail nonCopy result value flow
and exact current head-link Some(ptr_t)->None from a D_h-scoped
mode-preserving write ref; single LiveTail nonCopy result value flow and
after-return current O_t and A_t/D_t identity; whole destructure and
existing §18.1a receiver positive/negative; 0/1/2 branch joins and
transactional failure. It must publish owned checked evidence or reject
with no C. A later *independently authorized* native oracle must show
original physical tail still live after actual producer return and
before separate receiver EndRoot/free, same real R_t pointer and matching
A_t handle, no implicit producer free, head unlink while tail live,
with **physical Some(ptr_t)->None head unlink performed inside
the source-lowered producer** (not donor source or C observer),
receiver tail real free then donor head free, exactly 0/1/2 matched frees
under first/second/both malloc outcomes, source-driven machine effects,
negative/observer corruption controls. Neither P/F/R nor cJSON/benchmark
PASS is authorized by this candidate itself.

## 18.1c Draft 17.29 — bounded two-H live-tail nonCopy custody after recipient return

**Canonical bounded source rule (Issue #214; accepted PR #215).** Reuse precisely §3.2/§16.3's
completed one-link `H` and **two and only two** syntactic nested fallible
`try_allocate_one<H>()` sites, with first-None / second-None / both-Some
disposal. Reuse Draft17.28 §18.1b's exact four-argument producer,
`LiveTail { owned_ptr:ptr<H>, owned_allocation:Allocation,
owned_domain:LifetimeDomain }`, head-link `Some(ptr_t)->None` inside its
own function, existing `CarriesLiveH` original O_t/R_t/D_t/A_t returned value
relation, and the exact existing §18.1a `unit` terminal receiver. Add **one**
new top-level named, monomorphic, non-generic, known-direct recipient
function, plus its exact caller-owned ordinary local custody, scoped
loan, extraction and final consumption source route. All source in one
semantic compilation unit. No new typed heap H root, BackingRegion, Storage,
new Allocation, lifetime domain, parent/child owning policy, hidden
destructor or source-visible dependent contract.

### 18.1c.1 Exact selected source categories; what is NOT already admitted

Current Draft17.28 §§16/17/18/26 provide the *core semantic basis*:
a nonCopy authority value may move through fresh value places without
moving the independently located heap H root (§14.5, §18.6); an ordinary
`ref<write,T>` may whole-`replace` its current semantic value and return
the previous value (§17.4); `Option<T>` is nonCopy/nonDiscardable when
`T=LiveTail` (§26.4); consuming `match` carries its active Some payload
as a value (§26.12); known direct calls preserve caller-visible
post-state (§13.5c/§18.7). None of these grants general actual-source
`LiveTail` storage/extraction under §18.1b's closed producer/receiver route.

This revision selects only the following additions inside the existing
both-Some branch, **after** the head-scoped producer's return:
- `let packet = loan_read(life_h) { |stable_h| ... exact existing
  detach_and_return_tail(head_w@next,ptr_t,A_t,D_t) }`, storing the
  full returned original `LiveTail` in a fresh caller binding **without**
  requiring immediate whole destructure;
- `let custody = Option<LiveTail>::None;`, one distinct caller-owned
  ordinary **local** sum place with its compiler-managed implicit local
  governing identity (§13.7), initially current None; exactly scoped
  `loan_write(custody) { |sink| ... }` yields one ordinary
  `ref<write,Option<LiveTail>>` (not exclusive or noalias);
- the exact independent known-direct
  `fn recipient_adopt(sink:ref<write,Option<LiveTail>>,
                      packet:LiveTail)->unit`
  with exactly its body in §18.1c.4. The function may not allocate,
  release, retain `sink`, borrow the B heap root, call unknown functions,
  return an owner/ref, or change any other place;
- `Option<LiveTail>::Some(packet)` whole construction consumes the one
  complete original packet; `replace(sink,Some(packet))` performs one
  lifetime-preserving entire sum update and transfers the old sum out;
  no partial occupancy or implicit owner cloning;
- later, after the first scoped loan and recipient call are over,
  exactly one new `loan_write(custody)` obtains the old
  `Option<LiveTail>` by
  `replace(slot,Option<LiveTail>::None)`; an exhaustive consuming match
  on this result transfers the original `LiveTail` Some payload into
  a fresh binding; existing exact `let LiveTail { owned_ptr,
  owned_allocation, owned_domain } = saved;` whole destructure now
  admits that **same proved original** payload, followed by the exact
  existing §18.1a terminal call in that Some arm;
- after the extraction, the caller consumes the now-proved-None local
  `custody` by the special *known-None consuming match* below.
  The original head H_h remains the donor's responsibility.

The above grants **no** general `Option<Owner<T>>`, mutable heap owner
field, user-visible generic `Owner`, other `LiveTail` constructor, function
result/relay, escaping `ref`, borrowed match owner extraction, or additional
`try_allocate_one<H>` site. No `ptr<H>` can mint or recover an
Allocation/Domain. The receiver's source proof is still required after
the later match; a sum's static type alone never proves current O/R/D.

### 18.1c.2 The critical nonDiscardable-None and exhaustiveness rule

**Non-copy/discard law unchanged:** `LiveTail` has `Copy=false` and
`Discardable=false`, hence `Option<LiveTail>` has both properties false
**regardless of its current variant** (§26.4). Therefore:
- `store(sink,Some(packet))` is NOT allowed: it attempts to discard the
  old statically nonDiscardable sum;
- `replace(sink,Some(packet));` as a discarded expression result is NOT
  allowed: its old `Option<LiveTail>` remains nonDiscardable even if
  entry was provably None;
- `Some(_)` wildcard or an implicitly dropped displaced Some authority
  is NOT allowed (§26.13);
- an ordinary one-arm `match x { None => ... }` remains NON-EXHAUSTIVE
  under §26.11 unless the following **two-site exact source exception**
  is mechanically proved.

**New extremely bounded source admissibility / proof-oriented elimination:**
Within this precise two-H profile only, allow
`match exact_option_value { None => { unit }, };` with the `Some` arm
*absent* in **exactly two** places:
1. `exact_option_value` is the **old-value result** of the recipient's
   sole `replace(sink, Option<LiveTail>::Some(packet))`. Its current
   variant MUST be proven to be None from a finite inferred
   `Current(Referent(sink))==None` at that caller, with no alias/current
   state invalidation before replace;
2. `exact_option_value` is the single caller-local `custody` after
   the one later `replace(custody,Option<LiveTail>::None)`, and after
   its new loan scope has ended. Its current variant MUST be proven
   None, with no intervening uncertain effect/alias/rebinding.

This is a narrowly delimited *statically unreachable-variant* match
coverage rule: the compiler must prove **by source semantic facts** that
the omitted Some arm is impossible in every actual execution of that
match. A type, programmer comment, address equality, runtime assertion,
optimistic may-set or human `CHECK` can NEVER supply this proof.
Definition-time known direct recipient checking may record the
`Current(sink)==None` predicate conditionally over a symbolic formal
ref, but EVERY actual caller must prove it independently before the
nonCopy packet is consumed. When this proof cannot be established,
compile-time rejection occurs BEFORE backend. It is not a general
refinement-type matcher, not a silent discard of an unexamined value,
not `unreachable`/panic/abort source, not automatic destruction, and
does not weaken normal §26.11 completeness elsewhere.

On a valid old-None match, the **entire** nonCopy
`Option<LiveTail>::None` *value* is consumed by §26.12, with **zero**
owner payloads to extract or discard. There is no hidden owner
destruction or authority generation. In the later exhaustive
old-Option match, `Some(saved)` transfers the complete carried owner
package and its original symbolic O_t/R_t/D_t/A_t identity into
`saved`; the empty `custody` local still requires the second
known-None match above. No use of `Some(_)` is allowed.

### 18.1c.3 Independent definition checker / exact call and state obligations

The recipient **definition** is checked with fresh, initially
UNCORRELATED symbolic `sink` referent and `packet` value, just as
§18.1a/b infer finite conditional requirements rather than assume a
favorable main. The only permitted conditional requirements are:
1. `packet` holds exactly ONE intact, current live H_t owner
   `CarriesLiveH(packet,O_t,R_t,D_t,A_t)` carried from the approved
   §18.1b producer, same current O_t incarnation on the original
   live/full R_t backing, original unique nonCopy Allocation A_t
   and D_t. This is **not** guaranteed by the static type `LiveTail`
   alone, a copied ptr or a fabricated constructor.
2. `sink` is a current valid ordinary scoped **write ref** to the
   *different caller-owned ordinary live local*
   `Option<LiveTail>` place C, with sufficient write permission and
   current **exact None**, with no surviving occurrence-dependent
   capability or conflicting ancestor/current-value dependency.
   Referent C is distinct from every parameter/result place and
   from R_t/R_h and is stable across this call (its caller implicit
   local governing identity, not a new user LifetimeDomain).
3. No hidden dependence of the packet on `sink`, its loan, donor
   head loan, recipient locals, or any expiring borrowed scope;
   actual `sink` aliases/overlap and intervening effects must be
   precisely substituted under §13.5c. Nonexclusive `ref<write>`
   does not prove noalias or prevent unrelated aliases by fiat.

At **each** actual known direct call the compiler must source-prove
all three requirement groups with world-qualified O/R/D/occurrence
identities; Missing/may/Unknown identities, stale O_t, consumed A_t/D_t,
already occupied C=Some(previous), a read-only or expired sink ref, or
live forbidden loan/ref dependency REJECT. No proof can be borrowed
from a favorable sibling call or from the all-Some branch as a whole.

Source-order: evaluating argument `sink` copies the scope-bound ordinary
ref, evaluating `packet` CONSUMES the original nonCopy caller binding.
Function-local `packet` then moves exactly once into the fresh complete
`Some(packet)` payload; `replace` changes ONLY caller's C sum current
semantic value from None to Some(original owner). The old None sum
returns as a nonCopy old-value result and is explicitly consumed by
the exact proven-None one-arm match. The recipient returns `unit`;
callee's nonCopy parameter is already Consumed; there is no residual
local authority and **no EndRoot/finalize/deallocate**. Caller-visible
post-state is `Current(C)=Some(original packet)` with a fresh conditional
Some payload occurrence P_C, updated Change/Reset/Value dependencies
but unchanged *C root incarnation* and unchanged independently placed
live **H_t root O_t/R_t/D_t** (§17.4/§26.7/§18.6). Owner package
semantic correlation is transferred into the new payload, not
reconstructed. The result/caller memory state MUST NOT depend on
callee local scope or the expired sink loan (§11/§18.8).

At the later caller read/write loan (the prior sink loan has ended),
`replace(C,None)` consumes the new value None into C, returns OLD
`Some(original packet)`, and ends P_C without ending original heap O_t.
Any live ref to P_C, or unproven alias effect that may have invalidated
the package relation, forbids safe continuation. An exhaustive by-value
match transfers the old Some payload's semantic package from the
returned old sum to fresh `saved:LiveTail` with original identity.
The terminal receiver again must independently prove
`RequiredAtEntry(owned_ptr,owned_allocation,owned_domain)` exactly on
current original O_t/R_t/D_t/A_t before performing the familiar
EndRoot→erase_slot/full Storage→finalize D_t→deallocate A_t.
After that, the C local is statically proved None and explicitly
consumed; donor independently releases O_h. No persistent borrowed
capability or stale owner remains.

### 18.1c.4 Exact canonical source-shaped witness, including both failure worlds

**Adopted source-shaped test contract, NOT yet fully admitted by the production
semantic checker or native-executed.** Lines introduced by this revision, including
`loan_write(custody)`, `Option<LiveTail>::Some` and the two
`match ... { None => ... }` forms, are **not** admitted by canonical
Draft17.28. Existing `struct Node`, `try_allocate_one`, head producer,
scoped H reloan, `replace` link and terminal receiver are quoted from
the existing accepted `tests/fixtures/live_tail_return.nl` source path
and retain exactly the same meaning. There are exactly two
syntactic allocation sites, not a third root.

~~~newlang
struct Node { next: Option<ptr<Node>>, payload: u8, }

fn detach_and_return_tail(
    head_link: ref<write, Option<ptr<Node>>>,
    p: ptr<Node>, a: Allocation, d: LifetimeDomain
) -> LiveTail {
    let old_link = replace(head_link, Option<ptr<Node>>::None);
    return LiveTail {
        owned_ptr: p,
        owned_allocation: a,
        owned_domain: d
    };
}

fn recipient_adopt(
    sink: ref<write, Option<LiveTail>>,
    packet: LiveTail
) -> unit {
    let displaced = replace(
        sink, Option<LiveTail>::Some(packet));
    match displaced {
        None => { unit },
    };
    unit
}

fn receive_and_release_tail(
    tail_ptr: ptr<Node>,
    tail_allocation: Allocation,
    tail_life: LifetimeDomain
) -> unit {
    loan_read(tail_life) { |stable_t|
        let tail_r = ref_from_ptr(read, tail_ptr, stable_t);
        unit
    };
    let empty_t = loan_exclusive_read(tail_life) { |ending_t|
        destroy(tail_ptr, ending_t)
    };
    let full_t = erase_slot<Node>(empty_t);
    finalize_domain(tail_life);
    deallocate(tail_allocation, full_t);
    unit
}

fn main() -> unit {
    match try_allocate_one<Node>() {
        None => { unit },
        Some(head_bundle) => {
            let OneBacking { allocation, raw } = head_bundle;
            let allocation_h = allocation;
            let vacant_h = into_slot<Node>(raw);
            let life_h = lifetime_domain();
            let ptr_h = loan_read(life_h) { |stable_h|
                initialize(vacant_h,
                    Node { next: Option<ptr<Node>>::None, payload: u8(1) },
                    stable_h)
            };

            match try_allocate_one<Node>() {
                None => {
                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<Node>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
                Some(tail_bundle) => {
                    let OneBacking { allocation, raw } = tail_bundle;
                    let allocation_t = allocation;
                    let vacant_t = into_slot<Node>(raw);
                    let life_t = lifetime_domain();
                    let ptr_t = loan_read(life_t) { |stable_t|
                        initialize(vacant_t,
                            Node { next: Option<ptr<Node>>::None,
                                   payload: u8(2) },
                            stable_t)
                    };
                    let old_none = loan_read(life_h) { |stable_h|
                        let head_w = ref_from_ptr(write, ptr_h, stable_h);
                        replace(head_w@next,
                            Option<ptr<Node>>::Some(ptr_t))
                    };
                    let packet = loan_read(life_h) { |stable_h|
                        let head_w2 = ref_from_ptr(write, ptr_h, stable_h);
                        detach_and_return_tail(
                            head_w2@next, ptr_t, allocation_t, life_t)
                    };

                    // Head loan has ended; tail remains O_t on R_t under D_t.
                    // Only THIS caller-owned ordinary local stores A_t/D_t.
                    let custody = Option<LiveTail>::None;

                    // Positive preflight value: None => adopt; replace the
                    // initializer by Some(ptr_h) to exercise REFUSAL before
                    // the nonCopy packet argument is consumed.
                    let admit_flag = Option<ptr<Node>>::None;
                    match admit_flag {
                        None => {
                            loan_write(custody) { |sink|
                                recipient_adopt(sink, packet)
                            };
                            unit
                        },
                        Some(reason) => {
                            // Must release the still-caller-owned packet.
                            let LiveTail {
                                owned_ptr, owned_allocation, owned_domain
                            } = packet;
                            receive_and_release_tail(
                                owned_ptr, owned_allocation, owned_domain);
                            unit
                        },
                    };

                    // The previous sink scope and recipient frame are gone.
                    // This obtains either original Some(packet) (admitted)
                    // or None (refused). Both have explicit consuming arms.
                    let recovered = loan_write(custody) { |sink|
                        replace(sink, Option<LiveTail>::None)
                    };
                    match recovered {
                        None => { unit },
                        Some(saved) => {
                            let LiveTail {
                                owned_ptr, owned_allocation, owned_domain
                            } = saved;
                            receive_and_release_tail(
                                owned_ptr, owned_allocation, owned_domain);
                            unit
                        },
                    };

                    // custody itself is still nonDiscardable at None.
                    // Proof of exact current None is required, NOT a drop.
                    match custody {
                        None => { unit },
                    };

                    let empty_h = loan_exclusive_read(life_h) { |ending_h|
                        destroy(ptr_h, ending_h)
                    };
                    let full_h = erase_slot<Node>(empty_h);
                    finalize_domain(life_h);
                    deallocate(allocation_h, full_h);
                    unit
                },
            }
        },
    }
}
~~~

`admit_flag` is a Copy policy example, not a safety proof or new
language feature; positive value None calls the recipient. The explicitly
specified alternative `Some(ptr_h)` exercises pre-consume refusal
without consuming `packet` into recipient or fabricating a third root.
No `Some` payload automatically owns the node indicated by its Copy
ptr; the packet's original A_t/D_t are the only deallocation rights.

### 18.1c.5 Exact outcomes, failure / rollback / negative test contract

| Runtime branch | Original allocations and owners | Actual legal normal-exit result |
| --- | --- | --- |
| first `None` | no R_h/R_t, no packet, no custody | 0 frees, no recipient or terminal receiver |
| head `Some`, second `None` | only original R_h/A_h/D_h | head EndRoot/full R_h/finalize/deallocate ONCE |
| both `Some`, recipient accepts | distinct R_h/R_t live; produced packet originally A_t/D_t | donor packet consumed at call; recipient installs original packet in caller C and returns unit with zero frees; caller later extracts, terminal frees original tail ONCE, then donor head ONCE (2 frees) |
| both `Some`, **pre-consume refusal** | same original two roots, packet remains caller-owned | explicit terminal tail free in refusal arm; C remains None; old C extraction yields None; head independently freed (2 frees), no owner duplication |
| sink originally `Some(previous)` or state Unknown | potentially additional nonCopy responsibility | REJECT the recipient call before value consumption/effects; do not silently replace and lose prior owner |
| runtime-failing / post-consume refusal | packet already transferred | NOT ADMITTED: a `bool` return dropping owner is forbidden; a different explicit return/reinstallation protocol would require new authorization |

All actual primitive writes for this slice are *total source-level semantic
transitions* once their preconditions are proven. No third runtime
allocation/exception/partial initialization is added by sum storage:
`Option<LiveTail>` is an ordinary caller local; the whole sum
`None->Some(packet)` update and later `Some->None` update are each single
semantic transitions (§17.4/§26.7), not a promise of CPU atomic
cross-thread publication. Runtime allocation failure exists only at
the two existing `try_allocate_one<H>()` sites, each handled. Compiler
analysis/emitter resource/OOM failure must leave no accepted partially
checked artifact; that is a later production verification condition.
There is NO automatic rollback after ownership consume, no hidden
destructor, no C observer repairing a state, and no destructor-like
handling of an unreachable `Some` branch.

**Adversarial required compile-time cases:**
1. `packet` has wrong O_t ptr / A_h / D_h or nonmatching world-qualified
   backing: reject; `LiveTail` static type/copy pointer insufficient.
2. donor reuses `allocation_t`/`life_t` or `packet` after producer or
   recipient consume: reject; no double owner/terminal free.
3. sink's current value `Some(prior)` (even if static type same) or
   may-be-Some: reject *every actual* recipient call, cannot discard prior.
4. `store(sink,Some(packet))`, or `replace` old result dropped: reject
   because `Option<LiveTail>` nonDiscardable even at None.
5. omitted Some arm in any other match, or the bounded None-only match
   without source-proven exact None: reject under §26.11.
6. `Some(_)` wildcard when the payload is nonDiscardable LiveTail:
   reject under §26.13.
7. an ordinary write-ref still live from a conflicting/overlapping
   place, an old payload-dependent ref surviving `Some->None`, or a
   stale loan escaping call/return: reject on §13/§18.8.
8. read-only sink ref used for `replace` or ptr token made into ref
   without same D/provenance: reject; write never amplifies.
9. recipient returns unit while leaving nonCopy `packet` Available,
   or installs only `owned_ptr` without A_t/D_t: reject on
   §18.3 / absence of complete nonCopy transfer.
10. after extraction reuse stale Some payload ref or phantom sum
    provenance, or terminal free using wrong A/D: reject.
11. `sink` in caller and another ref aliasing its same sum place with
    unmodeled mutation before call: reject or conservatively
    precision-reject if `Current(sink)==None` is Unknown. Ordinary
    write-ref is NOT noalias; transitive source effects count.
12. first/second allocation None falsely constructing packet/sink
    owner, or branch availability mismatch: reject (§27.3).
13. application policy choosing refusal or storing an unrelated Copy
    ptr in H_h.next without minting wrong release authority is not
    automatically a language memory-safety violation; distinguish
    product oracle from semantic rejection.

**Future implementation tests, NOT RUN OR AUTHORIZED NOW:**
definition checker with completely uncorrelated symbolic `sink`
and `packet`, conditional source-proven exact vacant state and original
carrier correspondence, all actual caller checks, both None-only
consuming sites, duplicate/alias/Unknown corruption controls,
transactional checker/artifact OOM rejection, source renaming
metamorphism; optionally later independently authorized native C17
observer confirms *same original* tail allocation/root/domain
through producer → recipient caller-local Some → recipient return →
later old-value move → receiver free, no frees inside producer or
recipient, original head cleanup and 0/1/2 world counts. Do not infer from this adopted source contract that the production compiler
already admits its entire source, or that native execution, a complete
source-to-model proof, independent R validation or cJSON PASS has occurred.
A separate bounded finite FormalProof model does not establish production
source-checker or native correctness.

### 18.1c.6 Gate §4.2 / compatibility and no foreclosure

- **KEEP** (§3.1–3.4, §10–14, §14.5, §17.4, §18.6):
  original O_t/R_t/D_t/A_t conserved, `ref<write>` nonexclusive,
  no borrowed ref escaping, no authority from Copy ptr or Option
  static type, no simultaneous full raw while occupied.
- **KEEP** (§16.1/16.2/§26.4/26.7/26.12/§27):
  whole-aggregate nonCopy move, nonDiscardable statically,
  entire sum updates with fresh Some occurrence, ordinary
  consuming match with payload moved, sound branch join.
- **INTENTIONALLY EXTEND** only one 2-H same-H known-direct recipient
  and caller-owned ordinary `Option<LiveTail>` slot source,
  strict independently inferred/current None per-call proof and
  exactly two statically known-None one-arm consuming match sites.
  Those two exceptions are **new source admissibility**, not a
  general relaxation of §26.11 or hidden conditional Drop.
  §18.1b existing terminal receiver remains same operation.
- **DEFER** general owner-container APIs, arbitrary aggregate fields,
  heap custody, non-None replacement policies, receiver failure after
  consume, user-written proof/contracts, traits/effects, RAII,
  foreign ABI/layout/FFI, extra roots, three-link nodes, nested
  C graph ownership, callbacks/indirect calls, concurrency,
  separate compilation, generic inference beyond this one H.
- **No foreclosure:** a `LiveTail` in a *caller ordinary local* does
  NOT imply that a physical cJSON parent owns its child on ptr
  assignment. Future real containers may implement explicit
  custodial policy without changing the core `ptr` semantics;
  the source exact match exception is **not** a general
  instance-based `Discardable` trait nor a forced universal owner
  envelope. Keep physical memory/ABI and runtime owner tables
  outside the language claim.

**Historical evidence:** DI-001/006/007/008/009/010/011/012
and experimental Surface Draft1/1.1 ordinary nonCopy functions and
whole aggregate / sum construction, optional experimental owner
bridges; older M0 receiver/module/private source experimental only
(no Allocation/LifetimeDomain/destroy). Earlier normative Draft17.26
two allocation sites, Draft17.27 §18.1a terminal known call,
Draft17.28 §18.1b only original live-tail return already adopted.
Issue #213 five-root feasibility is product pressure, not
authorization. The complete older M9 conversation is not assumed
exhaustively audited.

## 18.2 argument passing

argument expression は source order で評価し、通常の value-use 規則を適用する。

existing Copy binding を argument に使うと copy。
existing non-Copy binding を argument に使うと consume / ownership transfer。

ただし、already-selected parameterまたはprimitive operandがcompatibleな`exclusive ref<...,T>`を要求し、
actualがexisting exclusive-ref bindingである場合は例外とする。
このargument useは§12.1のexclusive reborrowであり、parent exclusive-ref bindingのownership transferではない。

child exclusive refのscopeはcall / operation extent以下であり、
childがliveな間だけparentのconflicting useをsuspendする。
child終了後、parent bindingは再びusableになる。

この例外はexclusive-ref argument useだけに適用する。
ordinary `T` からのimplicit borrowや、他のnon-Copy / affine authority value一般へのreborrowを導入しない。

```text
f(x)
```

だけでよく、explicit `move` keyword は存在しない。

ordinary transferの場合、callee parameter は transfer された値を持つ fresh binding になる。

§12.1のexclusive reborrowの場合、callee parameterはcaller parentから派生した
fresh child exclusive capability bindingであり、caller parentのownershipを受け取るわけではない。

argument valueがhidden scope / semantic dependencyを持つ場合、
callee parameter bindingはそのdependencyを受け継ぐ。
function callはdependency laundering boundaryではない。

## 18.3 parameter lifetime

parameter は callee local binding である。

non-Discardable non-Copy parameter は、すべての normal function exit path で Available のまま残ってはならない。

## 18.4 no implicit borrow

ordinary function boundary は暗黙 borrow を行わない。

parameter が `ref<read,T>` を要求するなら caller はその ref / loaned capability を明示的に用意する。

§12.1のexclusive reborrowはこのruleの例外ではない。
actual argumentは既にexclusive-ref capabilityであり、ordinary valueからrefを暗黙生成していない。

value-use が non-Copy を自動 consume することと、implicit borrow は別概念である。

## 18.5 function result

function body は exactly-once lexical block である。

normal completion した場合、body の末尾 expression が function result になる。

result valueがhidden dependencyを持つ場合、
通常のvalue flowとしてresult側へdependencyを伝播する。
ただしordinary `ref`等のscope-bound capabilityは既存のnon-escape ruleを満たさなければならない。

```text
fn identity<T>(x: T) -> T {
    x
}
```

generic `T` は non-Copy として扱われるため、末尾 `x` の value-use が parameter binding を consume して result へ transfer する。

early exit が必要な場合、§19.1のexact lexical-block sourceでは:

```text
return expr;
```

を control-flow terminator item として使用できる。

semantic にいう `return expr` はこのterminatorを指し、sourceではsemicolonを含む上記formを用いる。
`expr` にも通常の value-use 規則を適用する。
`return` 自体は ordinary expressionではなく、normal block resultを持たない。

scope-bound ref / capability は、その scope dependency を満たさない形で function から return できない。
v0 の ordinary `ref` は原則として return 不可。

`ptr<T>` や transferable owner / authority は return できる。

## 18.6 value transfer / consume-out と pointer

non-Copy valueをexisting source placeからordinary value destinationへtransferする場合、
source placeからvalueを **consume-out** する。

```text
let y = x
f(x)
return x
```

等が該当する。

sourceがordinary lifetime rootなら:

- source object incarnationは終了
- source placement backing relationとsource governing-domain relationは終了し、semantic value packageへtransferされない
- source current semantic value packageはdestinationへtransfer
- destinationには別object incarnationが開始
- destination placement backing relationはdestination local/storageからfreshに決まる
- destination governing-domain relationはdestination lifetime-start側のexplicit/implicit domainからfreshに決まる
- source側に残るvacant/raw Storage responsibilityはcompiler-managed localなら内部処理してよい

value内部がcarryする`Storage` / `Allocation` / ptr provenance / backing dependency等は
semantic value packageの一部として通常どおりtransferする。
source object自身のplacement relationとは別物である。

既存 `ptr` はdestination objectへretargetしない。

これはlifetime-preserving `replace` と異なる。

```text
ordinary consume-out:
    source incarnation ends

replace:
    destination incarnation remains
    destination current value changes
```

ABI / optimizerはphysical copyを省略してよいが、
source semantics上のincarnation distinctionを壊してはならない。

address-sensitive value内部のself/interior ptrを自動修復しない。

## 18.7 caller-visible current-value state

function callはcaller-visible memoryのhidden dependency stateをlaunderしない。

calleeが`ref<write,T>`等を通じてcaller-visible placeを更新した場合、
call後のcurrent semantic value packageはcallee body semanticsから得られるpost-stateである。

例:

```text
fn exchange<T>(a: ref<write,T>, b: ref<write,T>) {
    swap(a, b)
}
```

について:

```text
exchange(x, y)
```

はdependency / identity stateについてdirect `swap(x,y)`と同じsemantic resultを持つ。

```text
exchange(x, x)
```

ならbody semantics上same-place swapなのでno-opである。

v0 compilerはbody-sensitive analysisをliteral inlining以外の方法で実装してよい。
exact post-stateを証明できない場合はmay-set / `Unknown`へwidenしてよいが、dependency-freeとは仮定しない。

## 18.8 function exit compatibility

normal completion / `return` edgeでcallee-local scopeが終了する時、
caller-visible memory、function result、その他edge後へsurviveするsemantic value packageが
callee-local scopeに依存していてはならない。

従ってcallee-local refをcaller-visible placeへ一時的にinstallすること自体は一律禁止しない。
return前にそのdependent current-value stateを終了・restoreできればよい。

```text
fn ok(dst: ref<write, Option<ref<T>>>) {
    local x: T
    let r = ref(x)
    let old = replace(dst, Some(r))
    use(...)
    replace(dst, old)
}
```

のようなshapeはscope-exit ruleを満たし得る。

一方、`Some(r)`をcaller-visible `dst`へ残したままreturnするpathはrejectする。


---

# 19. block

v0 では `{}` に共通する lexical body syntax を使うが、
semantic role として **lexical block expression** と **nonescaping callable block** を区別する。

どちらも first-class escaping closure ではない。

## 19.1 lexical block expression

lexical block:

```text
{
    ...
    result_expr
}
```

は control が到達した時に一度評価される。

### minimal source sequencing rule

v0 の少なくとも次のclosed block subsetについて、source sequencingを固定する。

```text
block      := '{' block_item* [tail_expr] '}'
block_item := binding ';'
            | expression ';'
            | return_item
            | continue_item
            | break_item

return_item   := 'return' expression ';'
continue_item := 'continue' '(' [continue_argument_list] ')' ';'
continue_argument_list := expression (',' expression)*
break_item    := 'break' expression ';'
```

ここで `binding` は少なくとも:

- §27.1のordinary single binding
- §27.1aのmulti-result binding
- §16.1のwhole-value aggregate destructuring binding

を含む。

規則:

- newline / CRLF は従来どおりtoken間whitespaceであり、item terminatorではない。
- non-tail `binding` / `expression` itemは `;` で終了する。
- `let` / `return` / `continue` / `break` は§21.8のcurrent structural reserved setに属し、ordinary lexical nameとして導入できない。
  従ってblock-item positionで `let` はclosed binding formのintroducer、`return` / `continue` / `break` は有効なcontrol contextでdedicated terminating itemのintroducerとして扱い、同spellingのordinary call/name interpretationとの競合を作らない。
- explicit return itemはexactly `return expression;` であり、semicolonは常に必須。
  `return(...)` をordinary function callとして解釈するsource routeは本closed profileに存在しない。
- `return expression` はtail expressionではない。block末尾でもsemicolonを省略できない。
- bare return terminatorは本closed profileに含めない。unit resultを明示的にreturnする場合は `return unit;` を用いる。
- continue itemはexactly `continue([arguments]);` であり、parenthesesとsemicolonは必須。zero argumentは `continue();`。trailing commaは許可しない。
- break itemはexactly `break expression;` であり、expressionとsemicolonは必須。bare `break;` は含めず、unit resultは `break unit;` と書く。
- `return` / `continue` / `break` itemはいずれもtail expressionではなく、block末尾でもsemicolonを省略できない。
- block末尾のsemicolon無しordinary expressionだけがtail expressionになり得る。
- tail expressionが無いnormal completionのblock resultは `unit`。
- `expression;` はそのexpressionが生成したvisible result responsibilityをblock resultへforwardせずdiscardする。
  discardされる各resultは通常のstatic `Discardable` requirementを満たさなければならない。
- `;` はlifetime end / destructor / implicit cleanupを生成しない。
- multi-result expressionの一部だけをexpression statementで捨てる機構はない。全resultをdiscardするなら全resultがDiscardableでなければならない。

この規則はblock sequencing / tail判定と、上記explicit return itemのsource integrationだけを固定する。
ordinary non-generic function declarationは§18.1で別のtop-level source profileとして固定するが、
lexical block itemへfunction declarationを追加しない。
§27.4のclosed `if`と§27.5のclosed `loop`はordinary expressionとしてこのblock grammarへ統合し、専用statement categoryは追加しない。
`if` / `loop`をnon-tail block itemとして使う場合は他のexpressionと同じくwhole expressionの後に`;`を付け、tail expressionなら付けない。
`continue` / `break` はordinary expressionではなく上記dedicated terminating block itemである。
その他general control syntax、recovery、newline-sensitive grammar等を追加しない。

normal completion 時、末尾 expression が block result になる。

末尾expressionがhidden dependencyを持つvalueなら、
block resultも同じdependencyを保持する。
blockはdependency laundering boundaryではない。

function body、`if` arm、loop の current iteration body、loan body は lexical block である。

exactly once の lexical evaluation なので、outer non-Copy binding を block 内で consume してよい。
block 後の continuation が存在する場合、その availability state は通常の control-flow join rule に従う。

### explicit return source context

`return expression;` は、semantic contextですでにidentity/signatureが既知の
**ordinary named-function body** をrootとするreturn-enabled lexical contextでのみ本closed profileのsourceとして有効である。
source `fn` declarationが存在することは要求しない。host registration等でordinary function contextを与えてよい。

ordinary lexical blockがreturn-enabled lexical contextの内部にあり、
その間にnonescaping callable blockまたはloan bodyのboundaryを跨がない場合、
そのblockもreturn-enabled contextを継承する。
従って、ordinary nested lexical blockや§26のmatch arm lexical blockから:

```text
return expression;
```

と書けば、§27.9どおりenclosing ordinary named functionを終了する。

return itemを評価するedgeでは:

- return expressionをexactly once評価し、通常のvalue-useを適用する
- function result / hidden dependencyを通常どおりtransferする
- §18.8 / §13.5aのfunction-exit compatibilityを検査する
- implicit cleanupを行わない
- そのedgeでは残りのblock item / tail expressionを評価しない
- そのedgeをnormal block / match / availability joinへ参加させない

normal outgoing edgeが一つも残らないconstructについて、
v0はbottom / never typeまたはsynthetic normal resultを導入しない。
normal result/type/availability joinは存在するnormal edgeだけに適用する。

return itemより後のsourceがそのedgeから到達不能であること自体を、
normative error / warningとするruleは本profileでは追加しない。
compilerは通常のsyntax / static checkingを行ってよいが、
unreachable textからreturn edge後のnormal availability/stateを捏造してはならない。

nonescaping callable blockまたはloan bodyへ入ると、
本revisionのreturn-enabled source profileはそこで継承を停止する。
そのcontext内の `return expression;` は本closed profileではsource-admissibleとしない。
これはfuture callable/loanのnon-local return policyを恒久的に禁止する決定ではない。
callable invocation自身のterminatorは引き続き§19.4の `leave` であり、
loanのfinal source surfaceは§13.8どおりProvisionalである。

### loop-control source context

§27.5のloop bodyへ入ると、そのloopを**nearest active loop-control target**とする。

ordinary nested lexical block、§27.4のif arm、§26のmatch armは、
その間にnew loopまたはcontrol boundaryを跨がない限りcurrent loop-control targetを継承する。
従って、それらのnested lexical block内の `continue(...);` / `break expression;` はnearest active loopをtargetとする。

nested loopへ入るとinner loopがnew nearest targetになり、outer loop targetをshadowする。

- inner `continue` はinner loop headerだけへ戻る
- inner `break` はinner loop exitだけへ流れる
- inner loopがnormal exitした後のouter lexical contextではouter loop targetが再びcurrent targetになる

labels / multi-level break / multi-level continueは本closed profileに含めない。

nonescaping callable blockまたはloan bodyへ入ると、outer loop-control targetの継承を停止する。
そのboundary内でnew loopへ入れば、そのinner loopだけがtargetになる。
従ってcallable / loan body内の `continue` / `break` がboundary外のsource loopをnon-localにcontrolすることはない。

ordinary named-function bodyもcallerのloop-control targetを継承しない。
callee source内のloop-control itemはcallee自身のactive loopだけをtargetにできる。

active loop-control targetが無いsource contextの `continue(...);` / `break expression;` はsource-admissibility errorである。

continue / break itemを評価するedgeでは、そのedgeの残りblock item / tail expressionを評価しない。
terminator後のsourceをnormative error / warningとするunreachable-code policyは本revisionでは追加しないが、
unreachable textからterminator後のnormal availability/stateを捏造してはならない。

## 19.2 nonescaping callable block

function parameter 等として渡される callable block は概念的に:

```text
{ |x: T| ... }
```

の形を持つ。

callee は call の dynamic extent 内で zero or more times synchronous に invoke できる。

- retain できない
- return / global / persistent field へ保存できない
- nested nonescaping callable parameter へ forwarding できる
- block parameter は invocation ごとの fresh binding

general closure object / heap allocationを要求しない。

nonescaping callable parameterは§13.5cのsymbolic callable summaryを持ち得る。
callerから渡されたactual blockのeffects / result dependencyはcall siteでsubstituteされる。

## 19.3 callable block capture

capture は hidden copy ではなく outer binding への lexical access である。

0..N invocation の callable blockでは、captured non-Copy binding の availability state は各 normal invocation exit で invocation entry と同じでなければならない。

したがって outer owner を一回目の invocation で恒久的に consume して二回目を不能にすることはできない。

memory mutation 等、binding availability を変えない effect は別途通常規則に従う。

captured scope/semantic dependencyはcallable block invocation environmentに保持され、
blockをcalleeへ渡すことで消えてはならない。

## 19.4 callable block result / leave

callable block invocation も normal completion 時には末尾 expressionを result にできる。

result expressionがhidden dependencyを持つ場合、
invocation resultもそのdependencyを保持する。
callable boundaryはdependency laundering boundaryではない。

callable invocation return / `leave` でinvocation-local parameter/capture scopeが終了する場合、
outer memory等へsurviveするcurrent-value stateはそのending scopeに依存していてはならない。

callee側でactual blockがまだsymbolicなら、§13.5cの`result_dependencies` / deferred compatibility obligationを用いる。

必要なら:

```text
leave expr
```

を current callable block invocation だけを終了する terminator として使用できる。

`leave` は enclosing named function から return しない。
implicit cleanup は行わない。

## 19.5 lexical block と callable block の共通性

両者は:

- lexical scoping
- expression evaluation order
- parameter/result type checking
- availability checking
- scope-bound capability rule

を共有する。

相違は主に invocation cardinality と escapeability である。

loan bodyを callable block とみなして `once block` 型を導入するのではなく、
loan は lexical block を一度評価する primitive とする。

---

# 20. requires function

## 20.1 定義

`requires fn` は、ordinary type checking / value semantics だけでは表現されない追加の caller obligation を持つ function である。

概念例:

```text
requires fn live_at(...)
```

## 20.2 call

`requires fn` call は explicit `unchecked` を要求する。

```text
unchecked live_at(...)
```

caller は documented precondition の proof responsibility を引き受ける。

違反は NewLang UB。

## 20.3 body

`requires fn` body は ambient unchecked context ではない。

body 内の unchecked operation は個別に `unchecked` を明示する。

## 20.4 ordinary safe API の責任

ordinary `fn` は caller に hidden human-proof obligation を要求してはならない。

ordinary API 内部で `unchecked` を使うことはできるが、その precondition を ordinary interface から到達可能なすべての状態で library implementation が保証しなければならない。

ordinary safe API だけを合法的に使って UB が発生した場合、原則として safe abstraction / compiler / runtime 側の bug とみなす。

v0ではmodule visibilityがDeferredなので、representation hidingによってのみ成立する「public boundary」の強制までは要求しない。

---

# 21. generic

## 21.1 基本

v0 は nominal parametric generics を持つ。

すべての generic type parameter は v0 では `Sized` とする。

実装方式はまず monomorphization を想定する。

## 21.2 generic T の既定能力

generic `T` について:

- value transfer / consume 可能
- `sizeof(T)` 利用可能
- `alignof(T)` 利用可能

以下は仮定しない。

- Copy
- Discardable
- equality
- ordering
- hash
- serialization
- any user-defined capability

## 21.3 constraint syntax

v0 では `T: Copy`, `T: Discardable`, `T: Hash` 等の明示的 constraint syntax を導入しない。

Copy / Discardable は user trait requirement ではなく compiler-known type property である。

core operation が `Discardable(T)` や `Copy(T)` を static applicability condition とする場合、
unconstrained generic `T` についてその condition は成立すると仮定しない。

したがって generic replacement API は old value を返す形を優先する。

## 21.4 definition-time checking

generic body は definition-time に可能な限り type-check する。

少なくとも:

- parsing
- lexical name resolution
- non-dependent function call
- control flow
- return
- non-Copy availability / consume state
- ordinary binding availability と Storage/slot initialization state
- non-Discardable handling
- known field access
- known conversion
- block rules

を definition-time に検査する。

## 21.5 dependent named function call

generic type に依存する **unqualified named function call** だけは instantiation-time まで resolution を遅延できる。

例:

```text
hash(key, seed)
equal(a, b)
```

unknown generic field access 等を遅延してはならない。

```text
x@foo
```

で `T` に `foo` field があるかを instantiation-time に調べる structural duck typing は v0 に入れない。
これは`@`をgeneral generic field lookupへ開くものではない。

## 21.6 associated lookup

dependent unqualified call:

```text
f(x, ...)
```

は第一引数の outer nominal type を dispatch type とする。

第一引数が `ref<...,T>` 等なら referent 側の nominal type を使う。

multi-dispatch は行わない。

## 21.7 associated function coherence

各 nominal dispatch type は、semantic に自身の **associated-function set** を所有する。

associated function declarationはdefinition-timeに:

```text
AssociatedWith(function, dispatch_nominal_type)
```

相当の一意なregistrationを持つ。

dependent associated lookupは:

```text
dispatch nominal type
    -> that nominal type's associated-function set
```

だけをcandidate sourceとする。

global lexical namespaceを同名functionについてscanしない。
future import setもassociated candidate setを拡張しない。

v0ではassociated registrationの最終surface syntaxを固定しないが、
registration自体はexplicit semantic relationであり、偶然同名のfree functionをcandidateにしない。

future module systemでは、このassociated setのregistration locationを
dispatch nominal typeのdefining module/homeへ制限してよい。

ただしmodule導入によってcandidate ownership ruleそのものを変更しない。

外部 nominal type に新しい semantics をopen-endedに追加するmodelはv0に含めない。
必要なら nominal wrapper / newtype を使う。

## 21.8 ordinary / future qualified lookup

v0ではordinary non-dependent nameはsingle compilation unitのlexical namespaceからlookupする。

ただし§4.9のdistinguished core spelling `unit` は例外であり、
ordinary lexical candidate setへ入るpredeclared bindingとして扱わない。
value expression `unit` は直接core singleton valueを表し、ordinary lexical bindingでshadowできない。
ordinary lexical namespaceへ `unit` を導入するsource formはname-admissibility errorとする。

この例外は `unit` のtargeted source ruleであり、
他のordinary nameについてbuiltin-first lookupやgeneral reserved-name tableを導入しない。

### current structural source-word reservation

current closed source profileでは、次のsmall structural setを
**ordinary lexical namespaceへ導入できないsource spelling** とする。

```text
fn
let
return
match
if
else
loop
continue
break
```

このsetはcurrent v0 source grammarの骨格を作るdeclaration / binding / control / expression introducer / branch delimiter / loop terminatorだけを対象とする。
general future keyword inventoryではない。

ordinary lexical name admissibility、lexer token class、member namespace、semantic lookupは別段階である。

- lexerは `fn` / `let` / `return` / `match` / `if` / `else` / `loop` / `continue` / `break` を他のwordと同じtoken classで表現してよい。
  dedicated keyword token kindはnormative requirementではない。
- ordinary function name、parameter、ordinary local binding、loop parameter、multi-result receiver、
  aggregate destructuringが導入するfresh local、match payload binding等、
  ordinary lexical namespaceへnameを導入するsource formでは上記spellingを使用できない。
- source parserは、上記spellingをordinary lexical nameとして到達可能に保つための
  distinguishing-shape / backtracking / fallback semanticsを提供する必要がない。
  current grammarでstructural source roleを持つ位置では、そのstructural meaningを持つ。
- malformed `fn` declaration、`let` binding、`return` item、`match` expression、
  `if` expression / `else` delimiter、`loop` expression、`continue` item、`break` itemを
  同spellingのordinary lexical name / free-function callとしてfallback解釈して別のvalid programへ変えてはならない。
- field name / variant name / member label等、ordinary lexical namespaceではない別namespaceまで
  本ruleだけで一律に予約しない。
  ただし、そのmember/payload spellingをfresh ordinary lexical bindingへ受けるsource formでは
  receiver binding nameとして上記spellingを使用できない。

§4.9の `unit` は本structural setに含めない。
`unit` もordinary lexical namespaceへ導入できないが、
core singleton type/valueを直接表すdistinguished core spellingであり、reservationのsemantic reasonが異なる。

本区別により、狭い文法位置だけで意味を持つtype/modifier-like wordまで自動的にordinary-name reservationへ昇格させない。
例えばcurrent frontendでtype/loan syntaxに現れる `ptr` / `ref` / `read` / `write` / `exclusive` / `using` 等は、
それぞれのclosed grammatical positionのruleに従い、本structural setへ自動追加しない。

Draft 17.17でexact loop source closureとともに `loop` / `continue` / `break` を追加した。
将来new structural source wordをordinary lexical namespaceから予約する場合は、そのfeature導入時に個別に裁定する。

dependent associated lookupとordinary lexical candidateを同一candidate setに混ぜない。

§18.1のordinary non-generic `fn` declarationはordinary lexical function candidateだけを導入する。
同じspellingがnominal typeのassociated-function setにも存在してよいが、
non-dependent ordinary callのcandidate setとdependent associated lookupのcandidate setをmergeしてはならない。
§18.1のsource `fn` 自体にはassociated registrationの意味を持たせない。

future module systemでは:

```text
module::f(x)
```

等のqualified ordinary lookupを追加してよい。
これはfuture qualificationの概念例であり、現revisionは
§26のclosed sum constructor以外の`::` source lookupをadmitしない。

ただしqualification / importはordinary lexical lookupの機構であり、
§21.7のassociated-function ownership/candidate setを暗黙に拡張してはならない。

## 21.9 overlap / ranking

v0 では:

- specialization なし
- overload ranking なし
- implicit conversion-based candidate selection なし
- backtracking なし

overlapping associated generic definitions は definition-time error。

## 21.10 dependent result type

dependent call を遅延可能とするためには、complete expected result type が generic definition-time に一意に決まっていなければならない。

その type expression は generic parameter を含んでよい。

例:

```text
let h: u64 = hash(key)
let x: Option<T> = next(...)
return convert(...)  // enclosing return type が expected type を与える場合
```

compiler は associated candidate search の結果から dependent call result type を推論しない。

v0 は associated output type inference を持たない。

## 21.10a dependent semantic summary

associated targetがinstantiation-timeまで未確定なdependent callでは、
calleeのsemantic effect summaryだけでなく、caller-visible post-current-value dependency / semantic identity stateも
definition-timeには未確定でよい。

そのcallがlive semantic dependencyと共存できるか、scope-exit compatibilityを満たすか、
後続safe operationに必要なidentity / backing relationをpreserveするかをdefinition-timeに決められない場合、
§13.5cのdeferred semantic compatibility obligationを記録する。

instantiation-timeにcandidate / body or summary / relative-place substitutionを確定し、
通常のconflict algebra、post-state、scope-exit ruleで検査する。

unknown post-stateをdependency-free / known identityとみなしてはならない。
このためだけにgeneric lifetime / effect / post-state constraint syntaxを追加しない。

## 21.11 inferred requirements

generic body から抽出される dependent operation requirements は public generic API contract の一部である。

例:

```text
fn find<K,V,Seed>(...)
```

に対して、

```text
requires at instantiation:
    hash(ref<read,K>, ref<read,Seed>) -> u64
    equal(ref<read,K>, ref<read,K>) -> bool
```
のような requirements が得られる。

public generic body 変更によって requirement が増えることは breaking API change とみなす。

dependent semantic compatibility obligationもpublic generic contract metadataの一部になり得る。
body変更によってよりstrongなsemantic preservation requirementが必要になる場合もsource-breakingになり得る。

compiler / docs / IDE は requirements / deferred semantic obligations を可視化できることが望ましい。

v0ではgeneric bodyをsame semantic compilation unit内で直接参照・instantiateしてよく、
stable serialized generic interfaceを要求しない。
future separate compilationではbody / typed semantic IR / equivalent semantic representationをtransportしてよい。

---

# 22. generic container と non-Discardable

v0 の generic container は、element が non-Discardable でも成立する API を優先する。

原則:

- insert/push: non-Copy value-use により ownership transfer
- fallible insert: failure 時に ownership を caller へ返す
- remove/pop: ownership を返す
- replace: old value を返すため non-Discardable T にも成立
- overwrite/store without old value: `Discardable(T)` が compiler-known の場合のみ
- drain: block へ ownership transfer
- teardown: consumer block へ ownership transfer
- backing deallocation: all live elementsを終了/transferし、
  outstanding claim / borrowを閉じた上でrooted region responsibilityをconsumeしてwhole raw Storageを返し、Allocationと共にdeallocate

`clear()` のように element ownership を消す API は generic T では自動的には成立しない。

将来、trailing block omission を empty block の syntax sugar として扱う余地を残す。
v0 では必須ではない。

---

# 23. aggregate layout

ordinary native struct layout は opaque とする。

保証するもの:

- field existence
- field type
- field non-overlap
- safe typed projection

保証しないもの:

- field order
- field offset
- padding location
- tail padding
- cross-compiler stable ABI
- cross-version stable ABI

compiler は field reorder を行ってよい。

## 23.1 sizeof / alignof

`sizeof(T)` は target-dependent compile-time `usize` であり、storable T representation が占有する bytes を返す。

`alignof(T)` は T lifetime 開始に必要な alignment を返す。

すべての storable type について:

```text
sizeof(T) >= 1
```
を保証する。

### compiler-provided generic layout knowledge

opaque generic `T` をraw storage上へ配置するlibrary codeのために、
compilerはtarget-dependentなlayout factsをsymbolically提供し、concrete instantiation後に確定してよい。

最低限必要なknowledgeはconceptually:

```text
size(T)
alignment(T)
element stride(T)
```

である。
v0 native arrayではadjacent element strideは既存ruleどおり`sizeof(T)`である。

このknowledgeをsource/API上で:

```text
Layout<T>
```

等のopaque witnessへpackageすることを許すが、exact spelling / runtime representationはProvisionalである。
compilerはwitnessをruntime materializeせずcompile-time eraseしてもよい。

layout knowledgeは:

- `Allocation` authorityではない
- `Storage` / `slot<T>` occupancy responsibilityではない
- ptr provenanceではない
- particular value / loanへのsemantic dependencyではない

従って、witnessをCopy可能に実装してもmemory authorityは複製されない。

ordinary opaque nominal `T`についてlayout knowledgeを提供しても、次を公開したことにはならない。

- field offset
- padding location/map
- field order
- C-compatible aggregate layout
- cross-compiler / cross-version stable ABI

sourceがopaque aggregate layoutを数値で推測して`Storage -> slot<T>`を行ってはならないが、
compiler-provided layout knowledgeを使ってsize/alignmentを満たしたstorageを準備しtyped placementへ接続することは許される。

§23.1の`sizeof(T) >= 1`をDraft 17.3でも維持する。
zero-sized semantic valueの存在とは別に、zero byte extentへ複数のindependent storable object responsibilitiesを重ねる一般mechanismはv0 raw-storage coreへ導入しない。
future zero-sized storable typeを導入する場合はbyte extentとlogical object multiplicityを別に定義しなければならない。

## 23.2 padding

padding は semantic value の一部ではない。

- initialization state を持たない
- value equality に参加しない
- semantic assignment/copy で bitwise preservation を保証しない

typed semantic copy と raw byte copy は同義ではない。

## 23.3 offsetof

ordinary struct に対する core `offsetof` は v0 では提供しない。

typed field projection を使用する。

---

# 24. bytes / raw Storage / typed object

byte storage は latent typed object ではない。

safe な:

```text
bytes -> ref<S>
```

overlay は提供しない。

wire / disk / network format は byte sequence と explicit encode/decode operation で扱う。

v0 では packed struct / unaligned field ref / general representation overlay を core に入れない。

## 24.0 raw representation state

raw backing bytes は abstract machine 上で常に **representation state** を持つ。

scalar observationのvalidityとして、各raw byteはconceptually:

```text
RawRepValidity =
    Defined
  | Unspecified
```

のいずれかである。

`Defined` は、そのbyteがruntimeで一つのwell-defined 8-bit representation valueを持つことを意味する。
これはcompilerがそのexact constantを知っていることを意味しない。
説明上、actual runtime valueも併記して:

```text
DefinedByte(runtime-value)
Unspecified
```

と表記してよい。

fresh allocation や object lifetime end 直後など、
まだtyped semantic valueとして解釈されていないraw bytesのrepresentation stateは
**Unspecified** でよい。

`copy_raw_bytes` はこのrepresentation stateをtyped valueとして解釈せずに転送してよい。

従って:

```text
unspecified raw representation stateをcopyする
```

ことそれ自体は:

- UBではない
- typed semantic valueのreadではない
- object initializationではない
- ptr provenanceの生成ではない

とする。

後続operationがそのbytesをtyped value / external format / integer等として解釈する場合は、
そのoperation固有のvalidity / initialization / decode preconditionに従う。

## 24.1 Storage byte length

**Provisional**

raw Storage claim の byte length を得る total observation を持つ。

conceptual signature:

```text
storage_len(
    storage: ref<read, Storage>
) -> usize
```

このoperationは:

- `Storage` をconsumeしない
- `Storage` current valueを変更しない
- subclaim / range tokenを作らない
- absolute machine addressを公開しない

offsetを扱うraw-memory operationは、原則としてこのStorage-relative lengthに対してboundsを定義する。

## 24.1a Storage start address

**Provisional**

ordinary-safe raw Storage claim のstart machine addressを得る total observation を持つ。

conceptual signature:

```text
storage_addr(
    storage: ref<read, Storage>
) -> addr
```

このoperationは:

- `Storage` をconsumeしない
- `Storage` current valueを変更しない
- `addr`以外のcapability / claimを生成しない
- ptr provenanceをmintしない
- BackingRegion identityを公開/生成しない

とする。

特に:

```text
storage_addr(a) == storage_addr(b)
```

だけから:

```text
same BackingRegion
same occupancy claim
merge(a,b) legality
Allocation matching
ptr provenance
```

を導いてはならない。

返された`addr`はCopy + Discardableなnumeric address valueとして、
元Storage claimのconsume/replacementやBackingRegion end後もvalueとして保持できる。
そのことはauthority継続を意味しない。

`storage_addr` / `storage_len` / §9.3のcoherenceにより、
generic allocatorはordinary-safe backingのstart coordinateとbyte extentを観測できる。

## 24.1b Storage scalar byte observation / update

portable explicit codecのため、ordinary-safe raw `Storage`に対してone-byte representation scalarを観測・更新するoperationを持つ。

canonical signatures:

```text
storage_read_byte(
    storage: ref<read, Storage>,
    offset: usize
) -> byte

storage_write_byte(
    storage: ref<read, Storage>,
    offset: usize,
    value: byte
) -> unit
```

両operationの`offset`はStorage-relative byte offsetである。
numeric `addr`やpointer arithmeticを経由しない。

### `storage_read_byte`

`storage_read_byte(storage, offset)` はcall entry時点で少なくとも:

```text
offset < storage_len(storage)
selected backing permits ordinary raw read
selected raw representation byte is Defined
```

をpreconditionとする。

selected representation stateがconceptually:

```text
DefinedByte(b)
```

ならresultはsemantic `byte` value `b` である。

`Unspecified` byteに対するordinary-safe scalar readは **not applicable** とする。
`Unspecified`をarbitrary semantic byteへ変換してはならない。
compilerがDefined preconditionを証明できなければ§8に従いcompile errorとし、
programmer / platform contractが成立を保証できる場合だけexplicit `unchecked`でproof responsibilityを引き受けられる。
falseなDefined assumptionはprecondition violationである。

readはraw representationを変更せず、Storage valueをconsume / replace / split / mergeしない。

### `storage_write_byte`

`storage_write_byte(storage, offset, value)` はcall entry時点で少なくとも:

```text
offset < storage_len(storage)
selected backing permits ordinary raw write
```

をpreconditionとする。

selected byteのpre-state validityはDefined / Unspecifiedのどちらでもよい。
normal completion後:

```text
RawRepState(offset) = DefinedByte(value)
```

となる。

selected byte以外のraw representation stateは保存する。

writeもStorage valueをconsume / replace / split / mergeせず、Storage claim identity / range / BackingRegion relationを変更しない。
従って`copy_raw_bytes`と同様に`ref<read,Storage>`を使用する。

ここで`ref<read,Storage>`の`read`は **Storage semantic valueへのcapability mode** であり、
そのclaimがauthorizeするbacking bytesをread-onlyにする意味ではない。
ordinary raw-write permissionはBackingRegion access propertyとして別に要求する。

### raw-vs-typed boundary

`storage_read_byte` / `storage_write_byte` はそれ自体では:

- typed object lifetimeを開始・終了しない
- `ValuePackage`を生成・installしない
- `ptr<T>` / `ref<T>` provenanceをmintしない
- `Storage` / `Allocation` / `LifetimeDomain` authorityを生成しない
- subclaim / persistent raw range / byte pointerを生成しない

とする。

特に、あるraw rangeへtyped `T`のvalid representationと同じbyte列を書いても、
`T` object lifetimeは開始しない。typed root開始には既存のtyped lifetime-start mechanismが必要である。

またlive typed objectがoccupyするbytesとoverlapするraw `Storage` claimはordinary-safe stateでは同時に存在できないため、
このoperationをlive object representation inspectionのescape hatchとして使用できない。

### raw representation fact / function boundary

raw-definednessはauthorityではない。
compilerはsafe applicabilityを証明するため、conceptually:

```text
RawDefined(backing-or-referent, [start, end))
```

のようなhidden semantic factを保持してよい。

implementationはwhole range、prefix、interval、singleton等へsoundに近似・widenしてよい。
source-visible `RawRange` / `DefinedStorage` / raw-initialization typestateを追加せず、
mandatory runtime shadow bitmapまたはmandatory exact per-byte compiler bitmapも要求しない。

ordinary function bodyが`storage_read_byte`を含む場合、compilerは必要なbounds / raw-read access / `RawDefined` requirementを
caller-visible internal semantic summaryとしてbody-sensitiveに伝播してよい。
callerでそのrequirementを証明できなければordinary callをrejectする。
compiler precisionを失ったことを`Defined`の証拠として扱ってはならない。

programmerまたはruntime contractだけがrequirementを保証できるboundaryでは、
既存`requires fn` / explicit `unchecked` policyを使用できる。
このためにsource-visible effect / precondition annotation languageを追加しない。

### external byte-producing boundary extension point

future platform / I/O / FFI operationが実際にdefined external bytesをraw Storageへ書いた場合、
そのnormal-success semantic summaryは対象rangeについて`RawDefined`を確立してよい。

例えばcapacity `cap`のdestinationへruntime `count` bytesを書いたoperationは、適切なboundsの下でconceptually:

```text
RawDefined(destination, [usize(0), count))
```

をpostconditionとして持てる。

これはmetadataの値だけからdefinednessをmintする規則ではない。
実際にbytesを書いた、または同等のboundary contractを満たしたoperationのsemantic postconditionでなければならない。

`RawDefined`成立はStorage authority、typed lifetime、ValuePackage、ptr provenanceを生成しない。
general I/O / FFI source API自体はこのrevisionでは定義しない。

## 24.2 overlap-safe raw byte copy

**Provisional**

v0は一つのoverlap-safe raw byte copy semanticsを持つ。

conceptual signature:

```text
copy_raw_bytes(
    dst: ref<read, Storage>,
    dst_offset: usize,
    src: ref<read, Storage>,
    src_offset: usize,
    count: usize,
) -> unit
```

final keyword / named-argument spellingはDeferredである。
上記argument structureとsemantic ruleをDraft 15のProvisional coreとする。

### authority

`Storage` はaffine raw occupancy claimであり、non-Copy / non-Discardableのままである。

`copy_raw_bytes` はStorage valueをconsume / replace / split / mergeしない。
source / destinationとも `ref<read, Storage>` を通して、
**call entry時点のcurrent Storage claim** が持つ raw occupancy authority を使用する。

ここで `ref<read, Storage>` に Storage-specific current-value stability semantics は追加しない。
ordinary ref と同様に、lifetime-preserving `replace` 後もsame placeを参照し、
subsequent callではnew current Storage claimを使用する。

raw backing bytesは`Storage` semantic valueのfield/subobjectではないため、
destination bytesを書き換えることを理由に `ref<write, Storage>` は要求しない。

ordinary refはCopyかつalias可能なので、
same Storage claimをsource / destinationの両方へ渡してよい。

例:

```text
let raw = ref<read>(storage)

copy_raw_bytes(
    raw, dst_offset,
    raw, src_offset,
    count,
)
```

これは同一Storage内のoverlap-safe `memmove`相当operationであり、
overlapする二つのaffine Storage claimを作らない。

distinct Storage claimsをsource / destinationへ渡してもよい。
同じBackingRegionに属するdistinct live Storage claimsは、
既存Storage invariantにより互いにdisjointでなければならない。

### backing access property

`copy_raw_bytes` はordinary raw-memory transfer operationである。

少なくとも:

- source selection の BackingRegion access property が ordinary raw read を許す
- destination selection の BackingRegion access property が ordinary raw write を許す

ことをpreconditionとする。

volatile / MMIO / device register / target-specific side effect等、
ordinary memory copy semanticsと同一視できないBackingRegionに対しては、
target / platform-specific operationを使用する。

targetがあるspecial backingについてordinary raw transferとのcompatibilityを明示的に定義することは妨げない。

### range / bounds

offsetは各Storage claim-relativeである。

destination:

```text
dst_offset <= storage_len(dst)
count <= storage_len(dst) - dst_offset
```

source:

```text
src_offset <= storage_len(src)
count <= storage_len(src) - src_offset
```

をpreconditionとする。

このsubtraction formによりunsigned addition overflowをprecondition式自体へ持ち込まない。

`count == 0` は許可する。
zero-length selectionでは:

```text
offset == storage_len(storage)
```

をvalid end anchorとしてよい。

dynamic valueからpreconditionをestablishする場合は§8のordinary checked API / `unchecked` ruleに従う。

### semantic result

operation開始前のsource representation-state sequenceを `S` とすると、
operation後のdestination representation stateは `S` と等しい。

このtransferはraw validityもpointwiseに保存する。

```text
source DefinedByte(b)
    -> destination DefinedByte(b)

source Unspecified
    -> destination Unspecified
```

mixed rangeでも各対応byte stateをtransferする。
source / destinationが部分overlapする場合もpre-operation source snapshotに対してこの結果を保証する。

従って`copy_raw_bytes`は`Unspecified`を含むsourceをcopyできるが、
`storage_read_byte` / `storage_write_byte`の逐次compositionと一般には等価ではない。
`storage_read_byte`は`Unspecified`をsemantic `byte`へinterpretできないためである。

このoperationはそれ自体では:

- object lifetimeを開始しない
- object lifetimeを終了しない
- typed semantic valueを生成しない
- ptr provenanceをmintしない
- `Allocation` / `Storage` / `LifetimeDomain` 等のsemantic authorityを複製しない
- source / destination Storage claim valueを変更しない

backendはforward/backward copy、target intrinsic、vectorization、library `memmove` equivalent等へlowerしてよい。

non-overlap専用の第二semantic primitiveはv0で要求しない。
distinct claim identity等からnon-overlapが証明できる場合の最適化はimplementation問題である。

### dependency / effect

`ref<read, Storage>` はordinary refと同じplace capabilityであり、
そのrefがliveであることだけを理由に:

```text
Value(StoragePlace)
```

dependencyを自動追加しない。

従って lifetime-preserving:

```text
replace(ref<write, Storage>, new_storage)
```

が他のordinary aliasing ruleと矛盾しなければ、
live read refが存在していても実行できる。
その後read refはsame placeのnew current `Storage` valueを見る。

ただしold Storage current valueの:

- BackingRegion identity
- range
- `storage_len`
- access property

等に依存していたproof / condition / semantic factは、
そのcurrent valueのchangeにより既存ruleに従ってinvalidateされる。

したがって、old claimに対して成立したboundsをnew claimへ無条件に使い回してはならない。
`copy_raw_bytes` のpreconditionはcall entry時のcurrent claimについて成立しなければならない。

一方、split / merge / transfer-away等がStorage referentのlifetime終了を伴う場合は、
ordinary live refが存在する限り既存reference lifetime ruleによりそのlifetime end自体が禁止される。

authorized raw backing bytesの変更だけでは:

```text
Change(StoragePlace)
```

を発生させない。

`Value(StoragePlace)` はStorage capability valueのcurrent-value factであり、
そのBackingRegion内の全byte contentを意味しない。

これはcompiler/backendがraw backing writeをmemory effectとして無視してよいという意味ではない。
code generation / ordinary memory alias analysisでは実memory writeとして扱う。

## 24.3 typed semantic movement は raw byte copyではない

live `T` のordinary movementは従来どおり:

```text
live T
    -- take -->
T value + slot<T>
    -- initialize -->
fresh destination T
```

で表す。

`copy_raw_bytes` はlive typed objectを別placeへsemantic moveするoperationではない。

typed moveを実装時にbyte copyへlowerできる場合でも、
source semantics上はvalue transfer / source lifetime end / destination lifetime startとして扱う。

v0はこのために:

- `Relocatable`
- `TriviallyMovable`
- `Pin`
- `Unpin`

等を導入しない。

## 24.4 opaque lifetime-root relocation

**Provisional**

GC / runtime / allocator等では、
complete live object rootをphysical memory上で移動し、
かつdestinationがsource自身のbytesとpartial overlapする必要がある。

このcaseをordinary `copy_raw_bytes`や`take + initialize`へ偽装しない。

v0はlocalized privileged / `unchecked` semantic transitionとして
**opaque lifetime-root relocation** を定義する。

conceptual form:

```text
relocate_opaque(
    source_root,
    destination_location,
    runtime_layout,
    required_authorities
)
```

final source spelling、destination authority carrier、runtime metadata representation、
fresh destination location tokenのconcrete return shapeはDeferredである。

### abstract location / overlap

本節のrelocation locationは概念的に:

```text
(BackingRegion identity, byte range)
```

で識別する。

same-placeとは:

```text
same BackingRegion identity
&& exactly same byte range
```

である。

numeric machine address equalityだけではsame-placeとしない。

source / destination rangeのoverlap relationも、
同じBackingRegion identity内のbyte range relationとして評価する。
§3.1のordinary-safe BackingRegion non-alias invariantにより、
distinct live BackingRegion identitiesは本relocation semantics上はdistinct backingとして扱う。
platform-specific multi-view/alias backingはこの推論をordinary safe coreへ持ち込んではならない。

### common identification preconditions

same-place / distinct-placeの分岐を判定する前に、少なくとも:

1. `source_root` が現在liveなindependently lifetime-ending rootとそのcomplete lifetime treeを識別できる
2. runtime size / alignment / layout metadataがsource root representationと一致する
3. destination locationがlive BackingRegion内のwell-formed rangeとして識別できる

ことを要求する。

same-place判定自体はrepresentation transferを行わないため、
ordinary raw read / write access propertyを要求しない。

### same-place case

source / destinationがsame abstract location:

```text
same BackingRegion identity
+
exactly same byte range
```

ならsemantic no-opとする。

このbranchでは:

- source incarnationを維持する
- governing `LifetimeDomain` identityを維持する
- occupancy responsibilityを変更しない
- fresh destination incarnationを作らない
- old ptr tokenをこのcallだけでstaleにしない
- source lifetime-ending authorityを要求しない
- surviving ordinary ref / stability capabilityが存在していても、それだけを理由にrejectしない

same-place branchはlifetimeを終了しないため、
distinct-place relocation用のending-authority / surviving-dependency preconditionを課さない。

ただしcommon identification preconditionにfalse assumptionを用いた場合は通常の`unchecked` ruleに従う。

### distinct-place preconditions

source / destinationがsame abstract locationでない場合、
common identification preconditionに加えて少なくとも以下を満たす。

1. ordinary root-ending ruleが要求する **source rootのgoverning `LifetimeDomain` `D` への lifetime-ending authority** をcallerが持つ。
2. transition後へsurviveするlive ref / stability capability / semantic dependencyと矛盾しない。
3. destinationはrequired size / alignmentを満たす。
4. source BackingRegion access propertyがrepresentation readを許し、destination BackingRegion access propertyがrepresentation writeを許す。
5. sourceとdestinationは、same BackingRegion内でpartial overlapしてよい。
6. destinationのうちsource lifetime tree外にあるbytesには、caller/runtimeが **raw occupancy responsibility** を持つ。
7. destinationは、このtransitionでlifetimeを終了しないthird live object / lifetime treeとoverlapしない。
8. fallible allocation、metadata lookup、policy decision、callback、user operationはこのtransition開始前に完了している。

ordinary representation transfer semanticsと同一視できないvolatile / MMIO / device-specific backingでは、
platform-defined relocation operationを使用しなければならない。

ここで **raw occupancy responsibility** は新しいsource-level typeを意味しない。

具体的には:

- explicit `Storage`
- §15のrooted dynamic-region ownerが保持するvacant responsibility
- runtime / allocatorのrooted responsibility ownerからscoped transferされたraw claim

等で表現してよい。

metadataはselected rangeがvacantであるというcontainer invariantの証明材料にはなり得るが、
raw occupancy authorityのoriginではない。

### governing LifetimeDomain

distinct-place relocationはobjectのphysical locationを変更するoperationであり、
governing domain identityを変更するoperationではない。

source rootがgoverning domain `D` に属する場合:

```text
source incarnation O1 governed by D
    -- relocate -->
destination fresh incarnation O2 governed by D
```

とする。

package内に別の`LifetimeDomain` value/identityが含まれている場合のvalue transferと、
root自身のgoverning-domain relationを混同してはならない。

relocationを使ってrootのgoverning domainを別identityへ付け替えてはならない。

### occupancy responsibility conservation

distinct-place relocationはobject occupancyとraw occupancy responsibilityを保存する。

source / destination が占めるabstract backing-byte setsをそれぞれ `S` / `D` とすると:

- `D - S` のraw occupancy responsibilityをtransitionがconsumeし、
  fresh destination root occupancyへ変換する。
- `S - D` はsource root lifetime endによりrawになり、
  exactly one raw occupancy responsibilityをtransition後に持つ。
- `S ∩ D` はtransition全体を通してrelocated rootのoccupancyに属し、
  同じbytesにraw responsibilityを重複生成しない。

transition後の `S - D` raw responsibilityは:

- returned `Storage`
- caller-owned rooted dynamic-region responsibility stateへのreturn
- runtime / allocatorのrooted responsibility ownerへのreturn

等で保持してよい。

concrete carrierはsurface designに委ねるが、
semanticにはraw occupancy responsibilityが失われたりduplicateしたりしてはならない。

### distinct-place transition

abstract machine上の一つのunobservable transitionとして:

1. representationをoverlap-safeにtransferする
2. source root incarnation、source placement backing relation、source governing-domain relation、source側のplace-owned structural stateを終了する
3. destination BackingRegion/rangeにfresh root placement backing relationを作り、fresh root incarnationを開始する
4. destinationのfixed subobject incarnations、current-value fact identities、必要なconditional occurrence identitiesをordinary structural semanticsに従ってfreshに作る
5. fresh destination rootに、sourceと同じ `D` への **fresh governing-domain relation** を作る
6. source **semantic value packageだけ** をdestinationへexactly once transferする
7. §24.4のoccupancy responsibility conservationを適用する
8. old ptr tokenはvalueとして残り得るがstaleになる
9. package内のsemantic identity / authority / value-owned backing stateはtransferされ、複製されない
10. fresh destination incarnationに対応する **new provenance-bearing location token** を生成可能なstateを作る

とする。

relocationはsourceの:

```text
PlaceId
root/fixed-subobject incarnation identity
placement backing relation
current-value fact identity
conditional occurrence identity
```

をdestinationへtransferしない。
これらはdestination location側でfreshになる。

一方、semantic `ValuePackage`内部の`Storage` claim、`Allocation` authority、
ptr provenance、hidden backing dependency等はordinary value-owned stateとしてexactly once transferされる。

new destination tokenのconcrete surfaceは:

- relocation operationのreturn value
- caller-owned runtime metadataへのsame-transition update
- platform/runtime-defined result carrier

のいずれでもよい。

ただしstale source `ptr` をdestination incarnationのtokenとして再利用してはならない。

このoperationは:

- self pointer
- interior pointer
- intrusive link
- external registry
- callback/context内のold-address ptr
- arbitrary escaped ptr copy

を自動retargetしない。

address-sensitive invariantは§17.4のboundaryに従う。

### no recoverable mid-transition state

precondition validation後、relocation transition内で:

- callback
- user code
- recoverable allocation
- `Result`等によるrecoverable failure

を発生させない。

abstract machineはhalf-relocated objectをrecoverable continuationへ露出しない。

これはconcurrency atomicityではなく、single-thread v0におけるsemantic atomicityである。

## 24.5 dynamic occupancy / raw subrange

persistent raw-range capabilityはv0 coreへ追加しない。

dynamic container / runtime内部のraw subrangeをrelocationへ渡す場合も、§15のresponsibility ruleに従う。
metadataがrawであると記録しているだけでは`Storage` authorityを生成できない。

containerのrooted responsibility ownerが対象rangeのvacant responsibilityを保持している場合に限り、
privileged boundaryはそのresponsibilityをtemporary `Storage` / equivalent raw claimとしてscoped transferしてよい。

source / destinationがoverlap/touchし、contiguous union全体が同じrooted ownerのvacant responsibilityでcoverされる場合は:

```text
union rangeのresponsibilityを一つのtemporary raw claimとしてtransfer
same scoped raw claimをsrc/dstに使用
```

してよい。

source / destinationがdisjointであり、それぞれのrangeのresponsibilityをownerが保持する場合は:

```text
two disjoint temporary raw claims
```

としてtransferしてよい。
二つのselection間のgapがrawである必要はない。

operation終了時、post-state raw responsibilityはexactly once同じorigin ownerへ返すか、
relocation transitionが定義する別の明示的consumerへtransferする。
metadata updateだけへresponsibilityを「戻した」とみなしてはならない。

このためだけに:

```text
RawRange
RawSlice
RawSpan
persistent substorage
(ptr<byte>, len) capability
```

等を導入しない。

---

# 25. arrays / span

**Provisional**

Draft 10以降ではnative arrayとcontiguous range accessを、
C pointer arithmetic / one-past modelから独立に定義する。

中心原則は:

```text
ref<mode,T>
    = one live T への scope-bound access capability

span<mode,T>
    = contiguous 0..N live T への scope-bound access capability
```

である。

`span` はowner / container / allocation handleではない。

## 25.1 native Array<T,N>

`Array<T,N>` はcompile-time element count `N: usize` を持つfixed-size aggregate typeである。

`N == 0` を許す。

最低限:

- elementsはindex orderを持つ
- elementsはcontiguous
- adjacent element start間のstrideは `sizeof(T)`
- element `i` のvalid index conditionは `i < N`
- each elementはarray rootに従うfixed subobject
- elementは独立lifetime rootではない

とする。

`N == 0` のarrayはlive elementを一つも持たない。

§23.1の `sizeof(type) >= 1` はarray typeにも適用されるため、
`Array<T,0>` のobject representation自体が0 bytesであることは要求しない。

contiguityはelement subobject間のsemantic/layout relationであり、
zero-element arrayにdummy element objectを要求しない。

## 25.2 array element projection

live array refからのelement projectionはsafe typed derivationである。

conceptually:

```text
array_element(
    a: ref<mode, Array<T,N>>,
    i: usize
) -> ref<mode,T>
```

precondition:

```text
i < N
```

result refはparent array refにscope-dependentであり、
parentより長生きしてはならない。

array elementはfixed subobjectなので、このprojectionは独立lifetime root authorityを生成しない。

persistent array ptrからも、boundsを満たすfixed element location tokenをsafeにprojectionできる。

conceptually:

```text
array_element_ptr(
    p: ptr<Array<T,N>>,
    i: usize
) -> ptr<T>
```

precondition:

```text
i < N
```

parent ptrがdanglingでもprojection自体はtyped location derivationとして可能である。
result ptrはparentのBackingRegion / object-incarnation / projection provenanceを引き継ぎ、
新しいlifetime rootやdereference authorityを生成しない。

## 25.2a whole-value Array consuming decomposition

generic `Array<T,N>` valueを、source-visible partial-move stateを導入せずに
各element valueへwhole-value decompositionするoperationを持つ。

conceptually:

```text
consume_array<T,N>(
    array: Array<T,N>,
    consumer: block(T) -> unit
) -> unit
```

call siteのArray argumentは§4.7のordinary value-use ruleに従う。

従って:

```text
Copy(Array<T,N>) == true
    => argument valueをcopyし、caller bindingはAvailableのまま

Copy(Array<T,N>) == false
    => caller bindingをconsumeしてreceived Array valueをtransfer
```

とする。

`consume_array` 自体がCopy valueを強制moveするspecial ruleは持たない。

received Array valueについて:

```text
element 0
element 1
...
element N-1
```

をindex orderで、各一回だけfresh callable parameterへtransferする。

normal completion後、received Array valueは残らない。

source programは途中の:

```text
[Consumed, Consumed, Available, ...]
```

のようなpartially-moved Array stateを観測できない。

`N == 0` ではconsumer invocationは0回であり、
received zero-element Array value自体はこのexplicit consuming operationにより処理される。

consumerはexisting nonescaping callable ruleに従う。
concrete `N == 1` 等を理由にouter captured non-Copy bindingを恒久consumeできる
special cardinality ruleは追加しない。

v0ではrecoverable mid-decomposition failure protocolを導入しない。
`consumer: block(T)->unit` のnormal completion structureを用いる。

## 25.3 span type

v0は:

```text
span<read,T>
span<write,T>
```

をscope-bound contiguous access capabilityとして持つ。

ordinary spanのpropertyは:

- Copy
- Discardable
- nonescaping
- hidden scope identityを持つ
- hidden scope / semantic dependencyを持ち得る

である。

`Copy(span<...,T>)` / `Discardable(span<...,T>)` は `T` のpropertyに依存しない。
spanはcovered valuesをownしないためである。

`span<write,T>` はmutation authorityでありexclusive authorityではない。
複数のwrite spansがoverlapしていてもよい。

spanはlifetime-ending authorityを持たず、covered elementを`take` / `destroy`する権限を生成しない。
`exclusive span`はv0 coreに導入しない。

従ってwrite spanだけからLLVM `noalias` 等を導いてはならない。

## 25.4 span abstract semantics

`S: span<mode,T>` と `len(S) == n` がliveなら、
Sは **0個以上のcurrent live `T` object locations** の
contiguous sequenceへのcurrent access capabilityを表す。

`n > 0` の場合、そのsequenceは一つのlive BackingRegion内にあり、
covered elementsをconceptually:

```text
E[0], E[1], ... E[n-1]
```

とし、少なくとも:

- 各 `E[i]` はcurrent live `T` object/subobject
- index orderとmemory orderが一致する
- adjacent element start間のstrideは `sizeof(T)`
- all covered element byte ranges are inside the same live BackingRegion
- modeに応じたread/write accessがbody scope中成立する
- covered element locations / required root lifetimesがspan scope中失効しない

ことを要求する。

`n == 0` の場合、live `T` objectもbacking byte rangeも一つもcoverしない。
そのためabstract semantics上:

- first-element ptr
- one-past ptr
- null ptr
- dummy T object
- BackingRegion identity
- zero-byte / dummy backing allocation

を要求しない。

zero-length spanがnon-empty parentからderiveされた場合、implementationがparent BackingRegion informationを
内部表現として保持してもよいが、それはzero-length span validityのsemantic requirementではない。

implementationはspanをmachine address + length等へloweringしてよいが、
そのrepresentationはsource semanticsではない。

## 25.5 span does not freeze values

spanのlivenessは、covered elementsの **current semantic valueが不変であること** を意味しない。

aliasing `ref<write,T>` / `span<write,T>` 等から、
covered elementを`replace` / `store`してよい。

span自身が要求するのは主に:

- covered T object/subobject incarnationの継続
- location / backing stability
- access authorityの継続

である。

span自身はcovered elementのcurrent-value identityへblocking semantic dependencyを自動追加しない。

例えば `span<read,Option<T>>` からelement refを取り、
さらに`Some` payload refを取得した場合、
payload refだけがそのspecific conditional occurrenceへのdependencyを追加する。

従ってwhole-array/element replaceでfixed element incarnationが維持されるならspan自体は残り得るが、
old payload occurrenceへ依存するcapabilityは通常の`Reset` conflict ruleに従う。

## 25.6 len / element

```text
len(s: span<mode,T>) -> usize
```

はsafe total operationである。

index projection:

```text
element(
    s: span<mode,T>,
    i: usize
) -> ref<mode,T>
```

はprecondition:

```text
i < len(s)
```

を持つ。

result refはspanのscope dependencyとsemantic dependencyを継承し、
spanより長生きしてはならない。

`element` はrange capabilityからpoint capabilityへのtyped projectionであり、
pointer arithmeticをsource semanticsとして導入しない。

## 25.7 subspan

conceptual operation:

```text
subspan(
    s: span<mode,T>,
    start: usize,
    count: usize
) -> span<mode,T>
```

はprecondition:

```text
start <= len(s)
count <= len(s) - start
```

を持つ。

`start + count <= len(s)` ではなく上記形を基本にすることで、
precondition evaluation自体のunsigned overflowを避ける。

result spanはparent spanのscope / semantic dependencyを継承する。

parent spanはconsumeされない。
従ってparentとsubspan、または互いにoverlapする複数subspanを同時に保持してよい。

特に:

```text
subspan(s, len(s), 0)
```

はvalid zero-length spanである。

v0はdisjoint split専用primitiveをcore requirementとしない。
必要なら二つの`subspan`を作れる。
write spanでもoverlap自体はillegalではない。

## 25.8 read/write authority reduction

```text
span<write,T> -> span<read,T>
```

はsafe total authority reductionとする。

reverse conversionは提供しない。

この関係はordinary `ref<write,T> -> ref<read,T>` と同じである。

§11.4と同様に、already-selected contextが`span<read,T>`を要求する場合、
`span<write,T>`をbuilt-in contextual weakeningで使用してよい。
candidate lookup / overload rankingのgeneral implicit conversionにはしない。

## 25.9 Array -> span

live native array refから全element spanをsafeに導出できる。

conceptually:

```text
array_span(
    a: ref<mode, Array<T,N>>
) -> span<mode,T>
```

result lengthは `N`。

result spanはarray refにscope-dependentであり、
array refが持つsemantic dependencyも継承する。

`Array<T,0>` からはvalid zero-length spanを得る。

Cのarray-to-pointer decayに相当するimplicit conversionは導入しない。
Array -> spanはtyped semantic operationとして明示する。

## 25.10 dynamic container / custom backing bridge

language coreはVec / Ring / Arena等のmetadataやcapacity policyを理解しない。

statically-known fixed array projection以外からspanを作るlibraryは、
localized privileged / `unchecked` boundaryを使用してよい。
ただしmetadataはcovered elementsのlivenessを記述するevidenceであってaccess/lifetime authorityのoriginではない。
non-empty span loanは、container/rooted regionが既に保持するlive-object responsibilityとstability authorityからderiveしなければならない。
zero-length spanはcovered object / backing rangeを持たないため、proven length 0を表すためだけにdummy BackingRegionや
lifetime authorityを新しく用意する必要はない。

conceptually:

```text
unchecked loan_span<mode,T>(container-specific evidence...) { |s|
    ...
}
```

のようなprimitiveを想定するが、surface syntaxは§13.8のloan syntaxと合わせてOpenとする。

loan bodyはspan scopeを越えてcapabilityを持ち出せない。

少なくともbody entryからexitまで:

- `len > 0` ならcovered rangeが同一live BackingRegion内にある
- element layout / alignment / strideが`T`と一致する
- `len`個すべてのcovered objectがcurrent live `T`
- covered element locationsが変わらない
- covered lifetime-root incarnationsが終了しない
- requested modeに必要なaccessが成立する
- source metadataとactual occupancy/livenessが一致する

ことをpreconditionとする。

falseならNewLang UBである。

v0でindependently managed container elementsをsafeにloanする代表方式は、
covered rootsを一つのcommon/coarse `LifetimeDomain`でstabilizeすることである。

```text
ordinary ref<read, LifetimeDomain D> live
    => exclusive D unavailable
    => covered roots cannot be lifetime-ended
```

という既存mechanismを再利用する。

coarse domainのためspan外のunrelated element lifetime-ending operationまで禁止されることをv0では許容する。
range-local lifetime authority / dynamically-many independent domainsの精密なloanはv0 coreに追加しない。

## 25.11 scope / function boundary

spanはordinary refと同様にscope-bound / nonescapingである。

少なくとも:

- function parameterとして受け取れる
- nested function/callableへより短いscopeで渡せる
- local aggregateに包む場合もdependencyを保持する
- persistent fieldへ保存できない
- globalへ保存できない
- escaping closureへcaptureできない
- dependency sourceより長いscopeへ持ち出せない
- ordinary function resultとしてreturnできない

とする。

従ってv0では:

```text
fn tail(s: span<read,T>) -> span<read,T>
```

のようなborrowed-result APIを一般には書けない。

必要なら:

- `(start, count)` 等のordinary valueを返し、caller側で`subspan`する
- caller-owned lexical block内でsubspanを作る
- nonescaping callback / loan bodyでderived spanを使用する

等を用いる。

`ResultScopeDeps = InputScopeDeps(...)` のようなinterprocedural borrowed-result lifetime polymorphismは、
spanだけのためには導入せずDeferredとする。

これはspan固有の問題ではなく、ordinary refをfunctionから返せないv0の既存境界と同じである。
## 25.12 effect summary / overlap precision

v0はspan導入のためにgeneral integer/range theorem proverを追加しない。

span parameterの`Referent(...)`は、そのspanがcoverするtyped interval全体を表すrelative range placeとして扱ってよい。

span経由でdynamic index elementを変更するcalleeは、
covered range全体に対するconservativeな:

```text
Change(Referent(Param(i)))
Reset(Referent(Param(i)))   // covered Tがconditional descendantを持ち得る場合
```

相当へsummaryをwidenしてよい。

ここでspan referentに対する`Change` / `Reset`は、
covered elementsへpointwise / may-effectとして適用されるものと解釈する。

異なるspan actualsについては、compilerがderivationからdisjointと証明できない限りmay-overlapとしてよい。

v0 semanticsは以下を要求しない。

- symbolic integer inequality solving
- general interval algebra
- Presburger arithmetic
- `i != j` からのgeneral element disjointness proof
- arbitrary dynamic subrange separation proof

constant indexやcommon-parent subspanの明白なdisjointnessをimplementationが追加精密化してよいが、
それはv0 source semanticsの必須能力ではない。

## 25.13 persistent range token はDeferred

v0 coreはfirst-class persistent:

```text
(ptr<T>, len)
range<T>
```

等を定義しない。

理由は、persistent rangeに:

- empty-range anchor
- object incarnation relation
- backing reuse後のstaleness
- independently-live elementsのidentity
- reacquisition semantics

という追加問題が発生する一方、v0での必須use caseがまだ確認できていないためである。

persistent subrange stateが必要なlibraryは、まずcontainer/owner固有の:

```text
owner identity / handle
start
count
```

等を保持し、利用時にscoped spanを再取得する設計を検討する。

persistent range tokenは実コードで繰り返し必要性が確認された場合に再検討する。

## 25.14 raw bytesとの分離

spanは **live typed objectsへのaccess capability** であり、raw occupancy claimではない。

従って:

- `Storage` の代替ではない
- uninitialized bytesをtyped spanとして安全に見る機構ではない
- §24のraw byte copy authorityを表さない
- §24のopaque lifetime-root relocationを表さない

raw `Storage` / overlap-safe raw byte copy / typed semantic move /
live `byte` span / opaque root relocationは区別する。

backendがtyped operationを`memcpy`等へ最適化することはsource semanticsとは別問題である。

## 25.15 C compatibilityとは独立

native span semanticsはCの:

- pointer arithmetic
- one-past pointer
- array decay
- begin/end pointer idiom

を再現するためのものではない。

C frontendは必要なら§31のunchecked compatibility primitiveへloweringする。
C compatibilityを理由にnative spanへpersistent pointer-range semanticsを追加しない。

---

# 26. sum type / Option / Result / match

**Provisional**

v0 のsum typeは **nominal / closed / payload-carrying value type** とする。

sum typeをC `switch`用tagged unionの特殊機構としてではなく、

> valueをvariantごとのexactly-once lexical blockへdispatchする value-producing control construct

として扱う。

## 26.1 declaration model

general sum declaration source syntaxは引き続き **Provisional** とする。

conceptual declaration shape:

```text
sum Option<T> {
    None,
    Some(T),
}

sum Result<T, E> {
    Ok(T),
    Err(E),
}
```

v0 の各variantはsemanticに:

```text
Variant
Variant(T)
```

のいずれかとする。

一variantが複数fieldを直接持つmodelはv0に入れない。必要ならaggregateをpayload valueにする。

variant名はdefining sum type内で一意でなければならない。

M9.1 / Draft 17.10 は**general declaration grammarを固定しない**。
ただしsemantic contextで既知のclosed nominal sumに対する§26.3 constructorと§26.9–26.10 match/pattern source profileは、
本節のconceptual declaration spellingから独立してexact closed source formとして固定する。

## 26.2 Option / Result

`Option<T>` / `Result<T,E>` は一般sum mechanism上のpredefined nominal typesとする。

```text
Option<T>:
    None
    Some(T)

Result<T,E>:
    Ok(T)
    Err(E)
```

特別なownership / lifetime semanticsは持たない。

Draft 17.21の§16.3 bounded recursive aggregate profileでは、
general generic type frontendを開かず、exact `Option<ptr<H>>` formだけを
そのpredefined Option mechanismのconcrete instantiationとしてsource-admitする。
このinstantiationにspecialなoptional-pointer/nullability semanticsは無く、
本節以下のordinary sum semanticsをそのまま使う。

## 26.3 construction

constructorはnominal value constructionでありordinary function lookup / associated-function lookup / member lookupには参加しない。

Draft 17.10で、semantic contextで既知のclosed nominal sumに対するconstructorのexact closed source formを次に固定する。

```text
payloadless_constructor :=
    sum_type '::' variant_name

payload_constructor :=
    sum_type '::' variant_name '(' expression ')'
```

ここで:

- `sum_type` は、surrounding type source syntaxで既に表現可能であり、semantic analysisで具体的なclosed nominal sum typeへ解決されるtype form。
- `variant_name` は一つのvariant identifier。
- `::` は隣接した二つの`:`からなるcontextual two-character punctuation。
  `: :`は同じ`::`ではない。
  `(`、`)` も上記productionのliteral punctuation。
- `::`のここでのsource admissionは既知のconcrete closed nominal sum constructorだけ。
  general module/type qualification grammarやfree-function qualified callsは追加しない。
- payload constructorのargumentはexactly one ordinary expressionであり、trailing commaはこのclosed formでは持たない。

代表例:

```text
Option<u32>::None
Option<u32>::Some(u32(42))

Result<u32, Error>::Ok(u32(42))
Result<u32, Error>::Err(error)
```

constructor candidateはqualified `sum_type::variant_name` として解決し、unqualified variant inferenceを行わない。

resolution / applicability:

1. `sum_type` をtypeとして解決し、closed nominal sumであることを要求する。
2. `variant_name` はそのsum type自身のvariant setだけから解決する。
3. unknown variant、別sumにだけ存在するvariant、payload arity mismatchはconstructor semantic error。
4. payloadless variantのvalid source formはparentheses無し、payload variantのvalid source formはexactly one expressionを持つparentheses付きform。
5. payload expressionには通常のleft-to-right evaluation / value-use ruleを適用する。
6. successful constructor expressionのstatic result typeはexactly resolved `sum_type`。

constructor-like spellingのresolution failureをordinary function / associated-function / member lookupへfallbackしてはならない。
同名variantを持つ別sum typeが存在しても、explicit `sum_type` qualificationによりcandidate setは混ざらない。

このsource profileはgeneral generic inference、specialization、constructor overload ranking、ADL的lookupを追加しない。

## 26.4 Copy / Discardable

sum typeのpropertyは全possible payload typeからstaticに導出する。

payloadなしvariantは各propertyについて`true`を寄与する。

conceptually:

```text
Copy(Sum)
    = all possible payload types are Copy

Discardable(Sum)
    = all possible payload types are Discardable
```

runtime current variantによってpropertyを変えない。

## 26.5 representation

sum representationはopaque。

semanticに保証するのは:

- current variant identity
- active variantがpayloadを持つ場合、そのcurrent payload semantic value

のみ。

保証しないもの:

- discriminant size
- discriminant integer value
- variant declaration orderとrepresentationの対応
- payload offset
- explicit tag byteの存在
- niche optimizationの有無
- cross-compiler / cross-version ABI stability

v0 coreではraw discriminant integerを取得するprimitiveを提供しない。

variant observationは`match`を用いる。

## 26.6 sum root と conditional payload occurrence

live sum objectは通常のlifetime-root incarnationを持ち得る。

payload付きvariantがactiveな間、payloadはenclosing sum rootに従う **conditional subobject occurrence** を持つ。

例:

```text
Option<T> root S
current variant = Some
payload occurrence = P
```

payload occurrence `P` はlifetime rootではない。

従ってpayload ptrを得ても:

```text
take(payload_ptr, ...)
destroy(payload_ptr, ...)
```

によってpayloadだけをindependently lifetime-endしてはならない。

payload occurrence lifetimeはenclosing sum state transitionに従う。

## 26.7 whole-sum replace / store

whole-sum `replace` はenclosing sum root incarnationを維持する。

ただしconditional payload occurrenceは作り直す。

```text
before:
    sum root S
    old active payload occurrence P_old, if any

whole replace

after:
    sum root S                    // preserved
    old P_old                     // ended
    new active payload P_new      // fresh, if any
```

これはold/new variantが同一でも同じ。

```text
Some(old) -> Some(new)
```

でも`P_old`終了 + fresh `P_new`開始とする。

variant equalityによる特例を作らない。

targetに存在したold payload semantic valueはreturned old sum valueへtransferされる。target側のold payload occurrence identity自体はtransferしない。

`store` は従来どおり`replace + discard old`であり、sum type自体が`Discardable`であることを要求する。

## 26.8 payload-only replace

live payload refに対する:

```text
replace(payload_ref, new_payload)
```

はordinary same-place replaceである。

```text
payload occurrence P:
    preserved

payload current semantic value:
    old -> new
```

従ってpayload occurrence `P` にsemantic-dependentなcapabilityがliveでも、このoperationが`P`をpreserveするとcompiler-knownなら許可できる。

## 26.9 match

`match` はvalue-producing expression。

source word `match` は§21.8のcurrent structural reserved setに属し、
ordinary function / parameter / local等のordinary lexical nameとして導入できない。
従ってexpression positionの `match` は本closed match expressionのstructural introducerであり、
同spellingのordinary free-function call `match(...)` へfallbackするsource routeは持たない。
field / variant / member labelとしての `match` まで本ruleだけで一律に予約しない。

Draft 17.10でexact closed source formを次に固定する。

```text
match_expression :=
    'match' expression '{'
        [ match_arm (',' match_arm)* [','] ]
    '}'

match_arm :=
    sum_pattern '=>' lexical_block
```

`lexical_block` は§19.1の既存 `{ ... }` blockであり、arm bodyをbare expressionにはしない。

`=>` はmatch-arm contextで用いる**隣接した二文字punctuator**とする。
lexicallyは `=` tokenの直後に空白なしで `>` tokenが続く形であり、`= >` はarm separatorではない。

arm間のseparatorは `,` である。

- newline / CRLFは従来どおりwhitespaceであり、arm separator / terminatorではない。
- 最終armのtrailing commaは許可する。
- comma無しで二つのarmを並べてはならない。
- empty arm list `match expression { }` はsyntaxとして許可する。
  そのmatchがsemanticにvalidかは§26.11のexhaustivenessで判定し、少なくともvariantを持つclosed sumではmissing-arm errorになる。
  本ruleはzero-variant sum declarationを新たに導入・保証するものではない。

例:

```text
let y =
    match x {
        Some(v) => {
            transform(v)
        },
        None => {
            default_value()
        },
    }
```

scrutinee expressionは一度だけ評価する。

current variantに対応するarmだけを選択し、そのarm bodyをexactly once評価する。
各arm bodyは通常のblock result / availability / terminator ruleに従う。
fallthroughはない。

match modeを選ぶsource keywordは追加しない。
§26.12 / §26.14どおりscrutineeのstatic typeがordinary sum valueならconsuming match、
`ref<read,Sum>` / `ref<write,Sum>`ならborrowed matchとする。
direct `ptr<Sum>` やその他の型を暗黙borrow / derefしてmatch対象へ変換しない。

## 26.10 v0 pattern

Draft 17.10でexact closed pattern formsを次に固定する。

```text
sum_pattern :=
      variant_name
    | variant_name '(' binding_name ')'
    | variant_name '(' '_' ')'
```

意味:

- `Variant`: payloadless variantだけに適用できる。
- `Variant(binding)`: unary payload variantに適用し、payloadをfresh arm bindingへ渡す。
- `Variant(_)`: unary payload variantに適用し、payload binding/capabilityを作らない。
- payload arityとpattern shapeが一致しなければsemantic pattern error。

pattern中の`variant_name`はscrutineeの**static nominal sum type自身のvariant set**だけに対して解決する。
ordinary function lookup / associated lookup / constructor lookup /他sumのvariant setへfallbackしない。
unknown variantやscrutineeとは別sumにだけ存在するvariantはsemantic pattern-resolution error。

`binding_name` はfresh ordinary binding nameであり、selected armのlexical blockへ入る直前に成立し、
そのarm body内だけでscopeを持つ。
§4.9および§21.8のordinary-name admissibility ruleにより、payload binding nameとして
exact spelling `unit` / `fn` / `let` / `return` / `match` / `if` / `else` / `loop` / `continue` / `break` は使用できない。
このrestrictionはpayloadのfresh ordinary bindingに適用するもので、variant label自体を同じsetで一律予約しない。
他arm、scrutinee expression、match後のcommon continuationへそのbinding自体を漏らさない。

exact spelling `_` はこのclosed profileでは `Variant(_)` のpayload-discard markerとしてだけ特別扱いする。
standalone `_` armはgeneral wildcard patternではなく、このgrammarに含まれない。
general sum declaration syntaxが未固定なので、`_` をvariant declaration名として扱うpolicyも本節では固定しない。

例:

```text
None
Some(v)
Some(_)
Ok(value)
Err(error)
```

v0では以下を入れない。

- general wildcard arm `_`
- match guard
- OR pattern
- nested pattern
- literal / range pattern
- implicit ref / mut pattern mode
- arbitrary aggregate destructuring in pattern
- qualified variant pattern

payloadがaggregateならarm body内で既存のwhole-value destructuringを使う。

## 26.11 exhaustiveness

`match` はexhaustiveでなければならない。

closed nominal sumのすべてのvariantについて、各variantをexactly one armでcoverする。

- missing variant: semantic compile error
- duplicate variant arm: semantic compile error
- unknown / wrong-sum variant: semantic pattern-resolution error
- pattern payload arity mismatch: semantic pattern error
- whole-match wildcard arm: v0 source grammarには存在しない

従ってempty arm list自体は§26.9のclosed grammar上parse可能だが、
variantを一つ以上持つsumでは通常のmissing-variant semantic errorになる。
duplicate/missing/wrong-sumをsyntax errorへ変換してpattern/exhaustiveness semanticsを隠してはならない。

## 26.12 consuming match

sourceに`move` / `consume` match mode keywordは追加しない。
scrutineeのstatic typeがordinary sum valueなら通常のvalue-use ruleを適用する。

`x` がCopyならsum valueをcopyしてmatchし、original bindingはAvailableのまま。

`x` がnon-Copyならwhole sum valueをconsumeする。

payload付きactive variantでは、active payload semantic valueをfresh arm bindingへtransferする。

ordinary bindingをpartially-moved stateにはしない。

## 26.13 consuming `Variant(_)`

by-value consuming matchで`Variant(_)`を選択した場合、active payload semantic valueをdiscardする。

従ってそのpayload typeはstaticに:

```text
Discardable(payload_type) == true
```

でなければならない。

## 26.14 borrowed match

sourceに`borrow` / `ref` match mode keywordは追加しない。
scrutineeのstatic typeがordinary `ref<read,Sum>` / `ref<write,Sum>` ならborrowed matchとする。

`r: ref<read, Option<T>>` に対する `Some(v)` では:

```text
v: ref<read,T>
```

を生成する。

`r: ref<write, Option<T>>` なら:

```text
v: ref<write,T>
```

を生成する。

これはgeneral implicit borrow/derefではない。programmerが既にref valueをscrutineeとして明示的に与えており、`match` がそのsum refをvariant-select / payload-projectするoperationである。

direct `ptr<Sum>` matchはv0に提供しない。必要ならまずsafe refを生成する。

## 26.15 borrowed `Variant(_)`

borrowed matchで`Variant(_)`を使う場合、payload ref/capabilityを生成しない。

従ってpayload occurrence dependencyも生成しない。

payload valueをconsume/discardしないため、payload typeの`Discardable` propertyも要求しない。

## 26.16 payload ref dependency

borrowed matchで生成されたpayload refは:

```text
payload_ref
    scope-depends-on parent sum ref
    occurrence-depends-on current payload occurrence P
```

とする。

dependencyはarm lexical scopeに限定されない。

payload refをmatch expression result等として外へ渡せる場合、通常のscope dependencyを満たす限り、そのderived capabilityはpayload occurrence dependencyを保持したままarm後もliveでよい。

## 26.17 payload ref と state transition

payload occurrence `P` にdependentなcapabilityがliveな間:

```text
replace(parent_sum_ref, ...)
store(parent_sum_ref, ...)
```

等、`P`を終了し得るoperationは禁止する。

一方:

```text
replace(payload_ref, new_payload)
store(payload_ref, new_payload)
```

が`P`をpreserveするなら通常のproperty ruleの範囲で許可できる。

same parent placeへaliasする別write refを通じたtransitionも、`P`をinvalidateし得るなら同じくconflictする。

compilerがalias/disjointnessまたはpreservationを証明できなければconservativeにconflictとする。

## 26.18 function call 越しのdependency

payload occurrence dependencyを含むsemantic dependencyのcall-boundary ruleは§13.5bに従う。

dependency-bearing payload refをparameterへ渡せばcallee parameterはdependencyを継承する。

potentially overlapping broad write refをcalleeへ渡す場合も、
calleeのcompiler-generated semantic invalidation summaryから
payload occurrence preservationを証明できる場合はcall可能。

preservationを証明できない場合はconservativeにrejectする。

従って:

```text
update_payload(payload_write_ref)
```

はpayload occurrenceをpreserveするsummaryなら許可できる。

一方:

```text
reset_parent(parent_write_ref)
```

がconditional payload occurrenceをinvalidateし得るsummaryなら、
該当payload capabilityがliveでalias可能な場合は拒否する。

sum type専用のgeneral effect systemは導入しない。

## 26.19 payload ptr

live payload refからpersistent `ptr<T>` をderiveしてよい。

そのptrはcurrent payload occurrence `P`を指すsemantic provenanceを保持し得る。

ただしptr value自体は`P`を維持するblocking semantic dependencyを持たない。

従ってwhole-sum transitionで`P`を終了してよい。
transition後もptr value自体は残ってよいがdanglingになる。

同じphysical addressで後にfresh payload occurrence `P2`が開始しても、stale ptrは`P2`へ復活しない。

## 26.20 payload ptr から ref

conditional payload由来`ptr<T>`からsafe refを再生成する場合、通常のptr->ref preconditionに加えて:

- originating/enclosing sum rootがlive
- ptrが指すpayload occurrence `P`が現在live
- current active conditional subobjectが **同じ `P`**
- 生成refへ`P` occurrence dependencyを付与できる

ことを要求する。

compilerがこれを証明できない場合はsafe ref生成不可。

localized `unchecked` boundaryを用いる場合でも、実際に同じlive occurrenceであることがpreconditionとなる。

## 26.21 parent refs across transition

payload-dependent capabilityが存在しない限り、ordinary parent refsはwhole-sum transitionを越えてliveでよい。

parent root incarnationは維持されるため、read refはtransition後のcurrent variant/valueを観測できる。

```text
parent refs:
    stable across whole-sum value transition

payload refs:
    stable only while their payload occurrence survives
```

## 26.22 payloadless arm と later mutation

`match` arm selectionはscrutinee evaluation時に一度決まる。

payload-dependent capabilityを生成していないarmで、後からaliasing writeによりparent sum variantを変更しても、既に選択されたarmが途中で切り替わることはない。

match selection自体はparent stateをarm終了までfreezeしない。

## 26.23 match-derived variant fact

compilerはselected arm内で`current variant was X at match selection`等のfactを使用できる。

ただしmemory-backed parent valueのcurrent variantに関するfactは、関連mutation / unknown call / alias effectでconservativeにinvalidateする。

payload occurrence capabilityがliveなら、そのoccurrence existenceはfactではなくsemantic dependencyにより保護される。

## 26.24 match result

normal completionするarmsのresult typeはexactly同一でなければならない。

implicit conversion / overload rankingは行わない。

`return`, `break`, `continue`等でterminatesするarmはcurrent normal match-result joinに参加しない。

このruleは`if`と共通。

## 26.25 outer availability join

normal completionする各armからcommon continuationへjoinする場合、outer non-Copy binding availability stateは全normal incoming edgeで一致しなければならない。

sum type専用のownership join stateは導入しない。

## 26.26 non-Discardable payload

consuming `Variant(binding)` はactive payload valueをfresh bindingへtransferするため、non-Discardable payloadでも成立する。

以後、そのarm bindingに通常のnon-Discardable ruleを適用する。

## 26.27 nested sum

nested borrowed matchはsemantic dependencyをtransitively合成する。

outer payload occurrenceとinner payload occurrenceの両方に依存するderived payload capabilityを作れる。

outer variant transitionもinner variant transitionも、該当occurrenceをinvalidateするならdependent capabilityがliveな間は禁止される。

## 26.28 same payload type in different variants

同じpayload typeを持つ別variantでもoccurrence identityはvariant occurrenceごとに別。

same physical locationでもold occurrence終了 + fresh occurrence開始とする。

whole-sum same-variant replaceでも同様。

## 26.29 generic sum

generic payload `T`について通常のgeneric ruleを適用する。

`T`をCopy / Discardableと仮定しないため、`Option<T>` / `Result<T,E>`もそのpropertyを仮定せずdefinition-timeにcheckする。

## 26.30 Result early return

v0では`?`等の専用propagation syntaxを導入しない。

明示的に:

```text
let value =
    match parse(input) {
        Ok(v) => {
            v
        },
        Err(e) => {
            return Result<Output, Error>::Err(e);
        },
    }
```

と書く。

`Err` armはterminatorなのでnormal match-result joinに参加しない。

## 26.31 Option early return

同様に:

```text
let value =
    match option {
        Some(v) => {
            v
        },
        None => {
            return Option<Output>::None;
        },
    }
```

と書ける。

将来propagation sugarを追加する場合も、本節の明示的`match` semanticsへのsyntax sugarとして定義できる。

## 26.32 v0 non-goals

v0 sum/match coreには少なくとも以下を含めない。

- `if let`
- `while let`
- `?`
- match guards
- OR patterns
- nested patterns
- literal / range patterns
- whole wildcard arm
- implicit borrow/deref matching
- ordinary binding partial move
- payload-specific lifetime-root take/destroy
- user-visible variant-stability token
- exclusive variant-transition authority
- runtime borrow flag
- runtime variant-dependent Copy/Discardable property

必要性はreal code / diagnostics / compiler complexityを観測してから判断する。

---

# 27. bindings / value-oriented control flow / availability analysis

**Provisional**

Draft 9 では ordinary binding の definite-initialization subsystem を可能な限り削り、
single-assignment + value flow + non-Copy availability analysis で置き換える。

## 27.1 ordinary binding

ordinary local binding は:

```text
let x = expression
```

の形で生成し、必ず initializer を持つ。

source word `let` は§21.8のcurrent structural reserved setに属し、
ordinary lexical nameとして導入できない。
block-item positionではclosed binding formのstructural introducerとして扱い、
ordinary function/local name `let` とのsource ambiguityを作らない。

binding nameはordinary lexical nameであり、§4.9のdistinguished core spelling `unit` および
§21.8のcurrent structural reserved set `fn` / `let` / `return` / `match` / `if` / `else` / `loop` / `continue` / `break` を使用できない。
従って:

```text
let unit = expression
```

はbinding-name admissibility errorであり、`unit` をshadowするlocal bindingを生成しない。

v0 では以下を持たない。

- initializer のない ordinary local
- ordinary binding への再代入
- ordinary binding の再初期化
- `var` / `mut` のような mutable-binding mode

binding immutability は object immutability を意味しない。

memory mutation は `ref<write>` / Storage / container operation 等で行える。

## 27.1a multi-result binding

複数の独立したresult responsibilityを返すexpressionに対し、v0は専用のmulti-result bindingを持つ。

closed source form:

```text
multi_result_binding := 'let' '(' name ',' name (',' name)* ')' '=' expression
```

block itemとして使う場合は§19.1に従い末尾に `;` を付ける。

例:

```text
let (value, empty) = take(p, ending);
```

規則:

- receiver countは2以上。
- RHSはexactly once評価される。
- RHSのchecked result countはreceiver countとexactly一致しなければならない。
- resultはchecked result orderに従ってleft-to-rightにreceiverへ対応する。
- receiver名は互いにdistinctで、current lexical scopeに対するfresh binding名でなければならない。
- receiver名に§4.9のdistinguished core spelling `unit` または
  §21.8のcurrent structural reserved set `fn` / `let` / `return` / `match` / `if` / `else` / `loop` / `continue` / `break` を使用してはならない。
- receiver bindingはRHS内ではscopeに入らない。
- RHS evaluation / semantic applicability / arity check / receivingの全てが成功した後に、全receiver bindingが一括してAvailableになる。
- failure時に一部receiverだけを成立させてはならない。
- produced result responsibilityからfresh bindingへのreceivingはownership transferであり、source bindingからのordinary Copy-useではない。
- receiverを省略するwildcard、nested receiver pattern、rest receiverはv0のこのformに含めない。

このsyntaxのparenthesesは**result receiver listのdelimiter**であり、tuple value / tuple type / general tuple destructuringを意味しない。
`take` の `T` と `slot<T>` は引き続き二つの独立したresult responsibilityであり、一つのtuple packageにはならない。

## 27.2 availability state

non-Copy ordinary binding について compiler が追跡する基本 availability state は:

```text
Available
Consumed
```

である。

binding の value-use は Available を要求し、non-Copy ならその後 Consumed になる。

Consumed binding を再び value-use してはならない。

Copy binding は value-use によって消費されない。

ordinary binding に `Uninitialized` / `MaybeInitialized` / `PartiallyMoved` state は設けない。

partial occupancy / initialization は `Storage` / `slot` と§15のrooted dynamic-region responsibility bridge側で扱う。
ordinary aggregate value construction自体にはpartially-live target aggregate state machineを設けない。
safe field-by-field in-place aggregate constructionは§16–17どおりDeferredである。

## 27.3 normal control-flow join

複数の normal control-flow edge が同じ continuation へ join する場合、
outer non-Copy binding の availability state はすべての normal incoming edge で一致しなければならない。

例:

```text
if (cond) {
    consume(x)
} else {
    unit
}

// one path: x = Consumed
// other:    x = Available
```

の後に共通 continuation を作ることはできない。

必要なら control-flow-dependent ownership を result value として再構成する。

control-flow terminator でその construct を離れる edge は、その construct の normal result / normal-state join には参加しない。

value resultがhidden semantic dependencyを持つ場合、
dependencyもvisible resultと同じincoming edgeでjoinする。

caller-visible / outer memoryのcurrent-value stateが各edgeで異なる場合も、
post-join current value/dependency stateは同じCFG relationでjoinする。

normative modelは§13.5aのhidden phi / block argument / memory-state phiである。
compilerはsoundなmay-set approximationを用いてよい。

## 27.4 if expression

`if` は value-producing expression である。

Draft 17.15でexact closed source formを次に固定する。

```text
if_expression :=
    'if' '(' expression ')' lexical_block 'else' lexical_block
```

ここで:

- `if` / `else` は§21.8のcurrent structural reserved setに属し、
  ordinary function / parameter / local等のordinary lexical nameとして導入できない。
- lexer-wide dedicated keyword token kindは要求しない。
- conditionを囲む `(` / `)` はこの `if` construct固有のdelimiterであり、
  general parenthesized/grouping expression syntaxを導入するものではない。
- condition parenthesesはoptionalではない。exact sourceでは必須。
- arm bodyはexactly §19.1の `lexical_block`。
- newline / CRLFは通常のtoken間whitespaceであり、branch separatorではない。
- `if_expression` 自体はtrailing semicolonを持たない。
  block itemとしてresultをdiscardする場合の `;` は§19.1のordinary expression-item ruleに従う。

condition expressionはexactly once評価し、static type `bool` を要求する。
condition resultに従ってexactly one lexical armを評価する。
未選択armをruntimeで評価しない。

### mandatory `else`

本closed value-producing profileでは `else` armをmandatoryとする。

```text
if (cond) {
    effect();
}
```

のようなno-`else` formは本grammarに含めない。

false pathへimplicit `unit` armを挿入せず、statement-only `if` categoryも追加しない。
effect-only conditionalが必要なら、例えば:

```text
if (cond) {
    effect();
} else {
}
```

のように両armをexplicit lexical blockとして書ける。
このexpressionをnon-tail block itemとして使う場合はwhole `if` expressionの後に§19.1どおり `;` を付ける。

### direct `else if`

direct:

```text
if (a) {
    x
} else if (b) {
    y
} else {
    z
}
```

は本closed grammarに含めない。
`else` の直後は必ずlexical blockである。

同じfinite branchingはnested `if`をelse-arm blockのtail expressionとして:

```text
if (a) {
    x
} else {
    if (b) {
        y
    } else {
        z
    }
}
```

と表現する。
future `else if` sugarを追加する可能性はDeferredであり、本revisionではsyntax identity / flattening ruleを固定しない。

### result / normal join

normal completionするarmsのresult typeはexactly同一でなければならない。
v0 はこの join のための implicit numeric conversion / overload ranking を行わない。

normal armのvisible result、hidden semantic dependency、outer availability、
caller-visible/current-value memory stateは§27.3の同じnormal CFG edge relationでjoinする。

Copy resultにimplicit extra copyを追加しない。
non-Copy arm resultは通常のvalue-use / transfer semanticsに従って `if` resultへ流れる。
arm-local scopeを越えてsurviveするresult / memory stateは既存scope/dependency compatibility ruleを満たさなければならない。

same static ref typeを持つnormal arm resultが異なるvalid dependency/provenance factを持つ場合も、
それだけを理由に `if` を禁止しない。
§27.3どおりsoundなjoined dependency/stateを保持し、証明不能な場合はcompilerがconservativeにwiden / precision-rejectしてよい。
特定production representationはsource semanticsではない。

### terminating arms

一方のarmが `return` 等でcurrent `if` normal continuationへ到達しない場合、
そのedgeはcurrent normal result type / availability / dependency / memory-state joinへ参加しない。
残るnormal edgeだけがnormal `if` result/post-stateを決める。

両armがterminatesしnormal outgoing edgeが0本の場合も、
bottom / never typeまたはsynthetic normal `unit` resultを導入しない。
このcaseのnormal result/type/state joinは存在しない。

### composition

nested `if`、§26の `match`、§19.1 / §27.9のexplicit `return` は、
各inner constructが生成するnormal / terminating edgeを外側の既存join ruleへそのまま合成する。

例えばinner `match` armが `return` すれば、そのedgeはinner matchのnormal joinにもouter `if`のnormal joinにも参加しない。
inner matchのnormal resultだけがその `if` armのlexical-block resultとしてouter joinへ参加する。

本profileは `if let` / pattern condition / match guard / ternary conditional / short-circuit boolean syntaxを追加しない。

## 27.5 loop expression

loop は loop-carried state を parameter values として明示する value-producing control construct である。

Draft 17.17でexact closed source formを次に固定する。

```text
loop_expression :=
    'loop' '(' [loop_parameter_list] ')' lexical_block

loop_parameter_list :=
    loop_parameter (',' loop_parameter)*

loop_parameter :=
    parameter_name '=' expression
```

ここで:

- `loop` は§21.8のcurrent structural reserved setに属し、ordinary function / parameter / local等のordinary lexical nameとして導入できない。
- lexer-wide dedicated keyword token kindは要求しない。
- parameter-list parenthesesはmandatoryであり、general grouping expressionを導入するものではない。
- zero parameter loopはexactly `loop () lexical_block`。
- parenthesisを省略した `loop lexical_block` / `loop { ... }` shorthandは本closed profileに含めない。
- loop parameter listのtrailing commaは許可しない。
- `parameter_name` はfresh ordinary lexical binding nameであり、同一loop parameter list内で互いにdistinctでなければならない。
- `parameter_name` には§4.9の `unit` または§21.8のcurrent structural reserved setを使用できない。
- admissibleなouter ordinary bindingと同じspellingを使う場合は、loop body内でそのouter nameを通常のinner lexical bindingとしてshadowする。

### parameter static type / initializer

loop parameterへsource-visible type annotationは本closed profileで追加しない。

各initializer expressionのnormal completionはexactly one valueを生成しなければならない。
そのvalueのstatic typeを対応するloop parameterのstatic typeとする。

initializer expressionsはleft-to-rightに評価する。
評価時のname lookupは**loop parameter導入前のouter pre-loop lexical environment**だけを使う。

従って:

```text
loop (a = outer_a, b = a) { ... }
```

で、new loop parameter `a` は `b` initializerのscopeには入らない。

- outer lexical `a` が存在するなら `b = a` はそのouter `a` をlookupする
- outer lexical `a` が存在しないならnew parameter `a`へresolveせず、ordinary unknown-name ruleに従う

initializerのleft-to-right evaluationにより、earlier initializerがouter stateをconsume / mutateした効果はlater initializerへ通常どおり反映される。
ただしearlier new loop parameter bindingがlater initializerへ見えることはない。

すべてのinitializer evaluation / receivingがnormalに成功した後、
first iterationのfresh loop parameter bindingsを一括して導入し、各initializer value responsibilityを対応するbindingへtransferする。

各later iterationではDraft 17.16どおりcontinue edgeからfresh bindingsを導入する。

loop bodyはexactly§19.1の `lexical_block` であり、そのiterationのloop parameter bindingsはbodyとordinary nested lexical descendantsでscopeを持つ。

`loop_expression` 自体はordinary expressionである。
non-tail block itemとして用いる場合は§19.1どおりwhole expressionの後に `;` を付け、tail expressionなら付けない。

代表例:

```text
loop (i = initial) {
    if (done(i)) {
        break i;
    } else {
        continue(next(i));
    }
}
```

zero-parameter example:

```text
loop () {
    continue();
}
```

### reference semantics / loop-header state

language reference semanticsは、initial argument packagesから始まり、
reachable continue edgeを有限回実行して到達し得るすべてのconcrete loop-header stateとexecution pathで定義する。

compilerはこれを直接unroll / enumerateする必要はない。
概念的なabstract loop-header state `H` を用いてよい。

`H` は少なくとも次の二層を持つとみなせる。

#### exact invariant layer

各reachable concrete edgeでexactに満たす:

- loop parameter count
- 各parameterのstatic type
- iterationごとのfresh binding identity
- ordinary Copy / non-Copy value-useとaffine responsibility conservation
- ending iteration-scopeとのscope/dependency compatibility
- loop entryからcaptureしたouter non-Copy bindingのavailability invariant

#### cyclic abstract layer

必要に応じてcyclicに近似してよい:

- parameter slotが現在carryするsemantic-package identity / possible origin
- carried valueのhidden dependency
- caller-visible / outer Copy/current-value facts
- current-value hidden dependency
- soundに保持できるfinite ref / dependency alternatives
- safetyに必要なcorrelation metadata

`H` はentryとbackedgeに対するinductive post-fixpointでなければならない。

abstract-state inclusionをconceptually `⊑` とすれば:

```text
EntryState ⊑ H

for each statically reachable continue edge i:
    ContinueTransfer_i(H) ⊑ H
```

または:

```text
Join(EntryState, ContinueTransfers(H)) ⊑ H
```

と表せる。

ここで `ContinueTransfer_i(H)` は、
`H`が表す任意のconcrete header stateからcurrent iteration bodyを通常のsource semanticsで実行し、
そのcontinue edgeへ到達した時のsuccessor stateをsoundly表すものとする。

reference semanticsは具体的reachable executionsであり、
specificationはcompilerへexact least fixed pointの計算を要求しない。
compilerは任意のsoundなinductive over-approximationを用いてよい。

### first iteration / later iterations

first iterationのfresh parameter bindingsはinitial argument packagesを受け取る。

later iterationのfresh parameter bindingsはreachable continue edgeがtransferしたpackagesを受け取る。

したがって:

- first iteration concrete stateだけをinputとしてbodyをcheckし、
  その結果をlater iterationへ無条件再利用するmodelは不十分
- 一回のsymbolic body analysisで済ませる場合、そのinput `H` 自体がentry + all reachable backedgesをsoundly subsumeしていなければならない

### symbolic affine package

fresh iteration binding identityと、bindingがownするdynamic semantic-package identityは別概念である。

non-Copy parameter slotについて、header abstractionはconceptually:

> current affine package carried by this loop-parameter slot

を表してよい。

各represented concrete header stateでは、そのslotにexactly one current affine responsibilityが存在しなければならない。

#### unchanged carry

current package `P` をconsume / transferしてcontinue argumentへ渡す場合、
next iterationのfresh bindingは同じdynamic package `P` のresponsibilityを受け取ってよい。

binding identityはfreshだがpackage responsibilityは一つだけsurviveする。

#### transformed carry

current package `P` をconsumeし、新しいsame-type package `Q` を生成してcontinueへ渡すことも、
通常のownership / dependency ruleを満たす限りlegalである。

この場合、next iterationのdynamic package identityを `P` と同一視してはならない。

header abstractionはunbounded concrete package-ID列を列挙する必要はないが:

- static type equalityからidentity equalityを推論しない
- possible originが複数でもaffine responsibilityをduplicateしない
- identity correlationを失って安全性を証明できない場合はprecision rejectしてよい

ものとする。

### static reachability basis

本節で `reachable continue / break / return edge` とは、
ordinary static control-flow semanticsで不可能と証明されていないedgeをいう。

host fixture / test harness / incidental runtime-known condition valueだけを理由にedgeを削除してはならない。

既存static semanticsがあるedgeへnormal controlが到達しないことを証明する場合はそのedgeを除外してよい。
本revisionはgeneral constant propagation / symbolic executionを要求しない。

### nested loop / body-sensitive call composition

nested loopは各自のheader state `H` を持つ。

- inner continueはinner headerだけへ戻る
- inner breakはinner loopのnormal result/post-stateとしてouter iterationへ戻る
- inner returnはenclosing function exitへ流れる
- outer continueはinner loopからnormalに戻ったpost-stateを通常どおりouter header transferへ用いる

nested fixed pointがimplementation上必要でも、それは新しいlanguage semantic categoryではない。
nested-loop production supportを本revisionは要求しない。

known acyclic body-sensitive callはcurrent iteration内の通常のstate transferとして扱う。
calleeのcaller-visible post-state / dependencyはcontinue / break / return edgeへそのまま流れる。
repeated executionはloop headerのcyclic abstractionが要約してよく、literal infinite inliningを要求しない。

recursive function SCC analysisは独立したprecision problemであり、本revisionでは解かない。

## 27.6 continue

Draft 17.17でexact source itemを次に固定する。

```text
continue_item :=
    'continue' '(' [continue_argument_list] ')' ';'

continue_argument_list :=
    expression (',' expression)*
```

ここで:

- `continue` は§21.8のcurrent structural reserved setに属し、ordinary lexical name / ordinary free-function calleeとして導入できない。
- parenthesesはmandatory。
- semicolonはmandatory。
- zero argumentはexactly `continue();`。
- argument listのtrailing commaは許可しない。
- `continue_item` はordinary expression / tail expressionではなく§19.1のdedicated terminating block item。
- active loop-control targetが無いcontextではsource-admissibility error。
- malformed `continue` structural syntaxをsame-spelling ordinary call/nameへfallbackしない。

continue edgeはcurrent iterationを終了し、各 argument expression の値をnearest active loopのnext iteration fresh parameter bindingsへ渡す。

source/semantic integration:

- argument countはtarget loop parameter countとexactly一致
- corresponding static typeはtarget loop parameter static typeとexactly一致
- argument expressionsはleft-to-right evaluation
- 各argument expressionのnormal completionはexactly one value
- value-useはCopy/consumeの通常規則に従う
- affine/non-Copy responsibilityをduplicate / loseしない
- argument valueのhidden dependencyもnext iteration parameterへ伝播する
- current iteration-local scopeに依存するsurviving value/current-value stateをedge越しに残さない

zero-parameter loopでは `continue();` がexactly arity 0として対応する。

continueはcurrent iteration lexical blockのnormal resultを持たない。

### captured outer non-Copy availability

loop entryからcaptureしたouter non-Copy bindingについて、
各statically reachable continue edgeのavailability stateはloop-entry stateとexactly一致しなければならない。

このruleはcyclic abstract wideningの対象にしない。
`Available / Consumed`をMaybe状態へ昇格させない。

iteration間で変化させたいaffine/non-Copy stateは、
通常loop parameter/packageとして明示的にcarryする。

break / return edgeにはこのcontinue-header invariantを適用しない。
それぞれ§27.7 / §27.9のexit ruleへ従う。

### multiple continue edges / dependency recurrence

複数のreachable continue edgeがある場合、
各edgeは上記exact obligationsを個別に満たしたうえで同じheader abstraction `H`へfeedする。

dependency / current-value / package-origin factsはliteral equalityを要求しない。
soundなjoin / finite alternatives / larger may-set / `Unknown` / wideningを用いてよい。

ただし:

- iteration-local dependencyをheaderへ持ち込まない
- possible safety dependencyをwideningでeraseしない
- one continue edgeのstateだけをcanonical next-header stateとして選ばない
- affine package identityをstatic typeだけで同一視しない
- lost correlationがunsafe acceptanceへつながる場合はconservative reject / analysis-precision rejectionへ倒す

ものとする。

## 27.7 break

Draft 17.17でexact source itemを次に固定する。

```text
break_item :=
    'break' expression ';'
```

ここで:

- `break` は§21.8のcurrent structural reserved setに属し、ordinary lexical name / ordinary free-function calleeとして導入できない。
- result expressionはmandatory。
- semicolonはmandatory。
- bare `break;` は本closed profileに含めない。
- unit resultを明示する場合は `break unit;` を用いる。
- `break_item` はordinary expression / tail expressionではなく§19.1のdedicated terminating block item。
- active loop-control targetが無いcontextではsource-admissibility error。
- `break(...)` をbreak専用call-like source formとして追加しない。
  current closed expression grammarはgeneral parenthesized/grouping expressionを持たないため、`break(expr);` をこのbreak grammarの別綴りとして扱わない。
- malformed `break` structural syntaxをsame-spelling ordinary call/nameへfallbackしない。

break result expressionはexactly once評価し、通常のvalue-useを適用する。

break edgeはnearest active loopを終了し、result valueをそのloop expressionのnormal resultとして外側へ渡す。

source/semantic integration:

- reachable break resultのstatic typeは既存§27.7 ruleどおりexactly一致
- break exitでouter non-Copy availabilityをjoinする場合は既存exact agreement ruleを適用
- ending loop/current-iteration scopeへのdependencyをsurviving result/current-value stateへ漏らさない
- breakはheader recurrenceへfeedしない
- conditional non-Copy break identityのlanguage legalityはDraft 17.16どおり維持し、production representation不足をsource restrictionにしない

breakはcurrent iteration blockのnormal resultを持たない。

### finite loop-exit join

header `H` がsoundに確立された後、
reachable break edgesはloopのfinite normal exit setとして扱う。

Copy result、ordinary ref/dependency result、outer Copy/current-value stateは
§27.3と同じfinite normal-join semanticsを用いる。

異なるbreak edgeがsoundな異なるnon-Copy dynamic package identityを結果としてtransferすること自体は
language-level illegalではない。
runtimeでは実行されたedge由来のexactly one responsibilityだけがsurviveする。

compilerがそのconditional affine identityをsoundに表現できない場合、
analysis-precision rejectionとしてよい。
一つのbreak edgeのidentityを選ぶ、fresh package identityを型だけからmintする、responsibilityをduplicateすることは不可。

break post-stateは、`H`からbody semanticsを通じてそのbreak edgeへ到達したstateであり、
entry stateへrestoreされる必要はない。

## 27.8 loop fall-through / zero normal exit

v0 prototype では loop body の normal fall-through を許可しない。

各statically reachable iteration path は最終的に:

- continue edge
- break edge
- return edge
- その他 current control construct を終了する明示的 terminator

のいずれかへ到達する。

implicit continue は存在しない。

Draft 17.17のsourceでは、normal fall-throughを避けるterminatorは§19.1のdedicated `continue_item` / `break_item` / `return_item`等として明示する。
nearest-loop targeting / nested-control-context inheritance / callable・loan boundaryは§19.1のloop-control source contextに従う。

### continue / break / return separation

edge classはsemantic上disjointである。

```text
continue -> current loop header recurrence
break    -> current loop normal exit
return   -> enclosing named-function exit
```

return edgeは:

- loop header recurrenceへ参加しない
- loop break result/state joinへ参加しない
- enclosing function exit semanticsへ流れる

return behaviorはsoundなheader `H`からiteration bodyを解析して得られるexitとして扱えばよく、
semantic modelがiteration回数だけreturn evidenceをunrollすることを要求しない。

### zero-break / no-normal-loop-exit

statically reachable break edgeが0本でも、それだけを理由にloopをstatic language errorにしない。

この場合:

- loop constructから外側へ戻るnormal outgoing edgeは0本
- normal loop result / normal post-state joinは存在しない
- bottom / never typeを導入しない
- synthetic normal `unit` resultを導入しない
- reachable return edgeは通常どおりenclosing function exitへ流れる
- remaining executionはcontinueを繰り返してdivergeし得る

これは「terminationをcompilerが証明した」という意味ではなく、
ordinary static control-flow graph上にnormal break exitが無いというsemantic outcomeである。

## 27.9 return

§19.1のreturn-enabled ordinary-function lexical contextにおけるexact source formは:

```text
return expr;
```

である。

`return expr` は enclosing named function を終了する terminator semanticsを表す。
source `return` はordinary expressionではなくdedicated terminating block itemであり、
semicolon無しtail formやbare `return;` は本closed profileに含めない。

`expr` はexactly once評価し、function result type と一致し、通常の value-use rule を適用する。

`return` edge は enclosing lexical block の normal result join に参加せず、
そのedge上では後続block item / tail expressionを評価しない。
match / if等のnormal joinでも同じterminating-edge exclusionを使う。

function-local scopesがreturn edgeで終了するため、
function result / caller-visible memory等、return後へsurviveするstateはending function-local scopeに依存していてはならない。

source ordinary-function declaration grammarは本節では定義しない。
function identity/signatureがhost registration等ですでにknownなordinary-function bodyにも同じsource ruleを適用できる。

## 27.10 ordinary partial move

ordinary aggregate binding の non-Copy partial move は許可しない。

whole-value destructuring / reconstruction を使う。

これにより ordinary non-Copy binding の state space を原則として `Available / Consumed` に限定する。

## 27.11 implementation model

このsource semanticsはSSA / block-argument style IRへloweringできる。

conceptually:

```text
loop_header(i, acc, hidden_state):
    ...
    branch loop_header(next_i, next_acc, next_hidden_state)

loop_exit(result, post_state):
    ...
```

のように、continue valuesとhidden current/dependency stateをsuccessor block argumentsとして表せる。

source-level mutable local / phi-assignment syntaxを導入する必要はない。

semantic dependencyも同じCFG/SSA edgeへghost metadataとしてloweringできる。

conceptually:

```text
visible:
    r = phi(r_a, r_b)

hidden:
    dep_r = phi(dep_a, dep_b)
```

とみなせる。

hidden dependency metadataはsource/runtime ABIへmaterializeする必要はない。

### cyclic analysis freedom

SSA/block-argument representation自体はloop safety proofではない。
compilerは§27.5のheader post-fixpoint obligationをsoundに満たさなければならない。

許されるimplementationには例えば:

- finite may-set
- monotone finite abstract domain
- `Unknown`
- widening
- memoized transfer
- worklist iteration
- inductive post-fixpoint証明
- safe correlation lossに対するconservative analysis-precision rejection

がある。

language specificationは:

- exact least-fixed-point iteration
- particular lattice representation
- particular widening operator
- fixed iteration count
- concrete ValueId enumeration
- source-visible loop invariant annotation

を要求しない。

widening / `Unknown`はreachable concrete stateのunder-approximationに使ってはならない。
可能なdependency / provenance / current-value blockerを消してunsafe operationをacceptしてはならない。

affine/non-Copy abstractionでは、possible identityがunknownでも
各concrete executionでexactly one responsibilityが存在することを保たなければならない。
その証明に必要なcorrelationをrepresentationできない場合はprecision rejectする。

first-iteration-only stateをlater iterationへそのまま再利用するoptimizationは、
すべてのreachable continue successorがそのstate/invariantへ戻ることを別途証明できる場合に限りsoundである。

---

# 28. compilation unit / nominal identity / future modules

## 28.1 v0 semantic compilation unit

**Fix**

v0 compilerは **one semantic compilation unit** を処理する。

implementationは一つまたは複数のphysical source fileを入力としてよい。

ただしphysical file boundaryはsemantic boundaryではない。

従ってsource fileは:

- namespaceを生成しない
- visibility boundaryを生成しない
- nominal identityを決めない
- associated lookup candidate setを決めない
- semantic effect ownershipを決めない
- separate compilation unitを意味しない

file path / input orderもlanguage semanticsへ使用しない。

source locationはdiagnostic / tooling metadataとして保持してよい。

## 28.2 single lexical namespace

**Provisional**

v0ではtop-level ordinary declarationsを一つのlexical namespaceで扱う。

ordinary declaration lookupはこのsingle compilation unit内で完結する。

異なるphysical source fileに分かれていても同じnamespaceに属する。

Draft 17.13の§18.1 ordinary non-generic function declarationについては、
同じsemantic compilation unit内のfunction name/signature visibilityをdeclaration textual orderから独立とする。
function bodyのordinary function name resolution / definition-time checkingより前に、
そのunitに属するbounded ordinary function signaturesがsemanticにavailableでなければならない。
これは全future declaration categoryへorder-independent visibilityを一般化するruleではなく、
§18.1で閉じたordinary function declaration categoryのexact ruleである。

Draft 17.21の§16.3 bounded recursive aggregate declarationについても、
そのclosed categoryに限ってnominal header collectionをfield resolution / value-body checkingより先に行い、
physical file / textual declaration orderから独立にする。
これは§16.3 exact profileのための追加ruleであり、general aggregate declaration ordering /
source-visible forward declaration / all nominal categoriesのorder-independent visibilityを意味しない。

duplicate-name ruleの詳細は各declaration categoryで定義するが、
physical fileが異なること自体はname collisionを解消しない。
§18.1のordinary functionではsame-name duplicate / ordinary top-level name collisionをerrorとし、
signature差によるoverloadを認めない。

## 28.3 semantic declaration identity

**Fix**

nominal type等、identityを持つdeclarationには
source spellingとは独立したsemantic declaration identityを与える。

少なくともnominal type identityは:

- simple textual name
- source file path
- source file order
- declaration byte offset
- compiler object-file number
- future module artifact record number

そのものではない。

同じspellingでも将来異なるmodule/homeの別declarationなら別identityになり得る。

future module qualificationはdeclaration identityをlookup / nameする機構であり、
nominal semantics自体の代替ではない。

## 28.4 nominal associated home

**Fix**

各nominal dispatch typeは自身のassociated-function setを所有する。

associated registrationは§21.7に従い、
特定nominal declaration identityへsemanticに結び付く。

v0ではmodule syntaxを持たなくてもこのownershipを成立させる。

future module systemはconceptually:

```text
NominalHome(T) = defining module of T
```

のようにconcrete homeを与えてよい。

ただしfuture import / re-exportによって、
Tのassociated candidate setがopen-endedに増えるmodelにはしない。

## 28.5 files != modules != compilation units
**Fix**

次の三概念を同一視しない。

```text
physical source file
semantic module
compilation unit
```

v0では:

```text
one or more source files
    -> one semantic compilation unit
    -> no semantic module boundary
```

でよい。

futureでは、一つのmoduleを複数fileに分けてもよく、
複数moduleを一回のcompilation unitでcompileしてもよい。

future separate compilation unit境界もmodule境界と一致するとは限らない。

## 28.6 module / import / visibility

**Deferred**

### Draft 17.22 future private-representation no-foreclosure

module/import/visibilityは依然としてDeferredである。
ただし将来moduleを導入する際のsource/abstraction設計方向を、
既存§16 / §17 / §28のsemantic representationと矛盾しないよう次のように整理する。

- fixed aggregateの**representation field** はdefining module内からのみ
  source projection可能とする **private by default** を第一候補とする。
  将来explicit `public` / `transparent representation` 等のescape hatchは別に裁定し、
  fieldを永久に公開不能とするruleは固定しない。
- field read/write `@`、ref/ptr-to-field typed projection、named-field aggregate literal、
  whole named-field destructuring、および将来field patternsは
  **同じrepresentation visibility policy** へ従う方向を維持する。
  field accessだけprivateにし、named construction/destructuringから同じrepresentationを
  無制限に読み書きできる抜け道を作らない。
- sum variant/constructor/match visibilityはaggregate representation privacyから
  独立に裁定する。struct fields privateだから全sum variantsもprivateとは推論しない。
- compilerのstructural Copy/Discardable、opaque ProjectionId、typechecking/proof、
  layout metadataはfield visibilityとは独立したinternal semantic factsを保持してよい。
- ordinary whole-value operations/Copy/consumeやalready-authorized lifetime/effect mechanismは、
  fieldをprivateと呼ぶだけで新しく禁止したりaccess authorityを増減させたりしない。
  address-sensitive/private representationを公開したwhole-value move/swap等が可能なケースの
  invariant enforcementは別途所有・API・visibility設計が必要である。

これは **将来へのnon-foreclosure design direction** であり、
現v0でdefining moduleを実際に導入したり、追加のvisibility errorを生成する規範ではない。
current one visibility domain内のPair/Node source witnessは従来どおりadmissibleである。
field visibilityをまだ持たないことを理由に`@`ではなく`.`を採用しない。

v0 coreでは以下を要求しない。

- `module` declaration
- `import`
- `re-export`
- `public` / `private`
- module-qualified source names
- circular module dependency rule
- file-to-module mapping

v0はone visibility domainとして扱ってよい。

従ってrepresentation hidingを利用したabstraction enforcementはv0では完全ではない。

future module/visibility導入時も:

- nominal identity
- associated-function ownership
- ordinary vs associated lookup separation
- generic requirement semantics
- semantic dependency/effect semantics

を変更しないことを優先する。

## 28.7 separate compilation / artifact

**Deferred**

v0は以下を保証しない。

- independent source compilation
- stable module interface artifact
- stable binary library ABI
- binary-only generic library distribution
- signature-only downstream type checking
- cross-version semantic artifact compatibility

v0 compilerはwhole semantic compilation unitのsource/bodyを直接参照してよい。

generic instantiationにはgeneric bodyを、
semantic effect checkingにはcallee bodyからinferしたsummary等を
compiler内部で直接利用してよい。

compilerはperformance optimizationとして:

- parsed AST cache
- typed IR cache
- generic instantiation cache
- inferred requirement cache
- effect summary cache
- opaque projection graph cache

等を持ってよい。

ただしcache formatはlanguage semanticsではない。

## 28.8 future semantic interface freedom

**Fix**

future separate compilationでdownstream compilerが必要とするsemantic informationを:

- source
- generic body
- typed semantic IR
- inferred generic requirements
- deferred semantic obligations
- semantic effect summary
- callable dependency transformer
- current-value dependency / semantic identity post-state representation
- opaque projection graph
- その他equivalent semantic representation

としてtransportしてよい。

> **public semantic interface = source-level signature only**

とは規定しない。

これによりbinary-only compatibility要求のために
generic semanticsやeffect semanticsをerasure等へ歪めることを避ける。

## 28.9 semantic transparency requirement

**Fix**

future separate compilation / caching / module artifactはoptimization / packaging mechanismである。

同じsemantic programについて、
single-compilation-unitでbodyを直接見た場合とartifact経由の場合で:

- name resolution
- associated lookup
- generic validity
- availability checking
- semantic dependency checking
- current-value dependency / scope-exit checking
- place/effect conflict checking

のlanguage-level resultを意図的に変えてはならない。

artifactが必要情報を表現できない場合、
language semanticsを弱めるのではなくartifact representationを拡張するか、
source / richer semantic representationを要求する。

---

# 29. errors / failure model

v0 に exception / panic はない。

通常予想される失敗はvalueとして表現する。

主に:

- `Option<T>`
- `Result<T,E>`

を用いる。

v0では`?`等の専用propagation syntaxを要求しない。early returnは§26のvalue-producing `match` + `return`で表現できる。

を想定する。

fatal diagnostic trap は recoverable failure mechanism ではない。

---

# 30. UB

少なくとも以下は NewLang UB になり得る。

- unchecked precondition violation
- assert false
- invalid ptr -> ref assumption
- dangling / dead incarnation を live と仮定
- invalid alignment / range
- invalid lifetime transition
- invalid integer precondition
- invalid value conversion precondition
- requires function precondition violation
- LifetimeDomain finalization precondition violation
- その他 specification で precondition violation と定義されたもの

ordinary safe API を合法的に使用した caller は、その API 内部の unchecked obligation の責任を負わない。

---

# 31. C interoperability / migration

v0 native core semantics は C ABI に合わせて歪めない。

方針:

- C frontend / compatibility frontend を別層に置く
- shared semantic IR を検討する
- C pointer arithmetic 等は compatibility primitive へ lowering する
- native aggregate layout と C aggregate layout を同一視しない
- native aggregate / sum はforeign boundaryを越えるだけではC ABI representation identityを得ない
- C aggregate boundaryを提供する場合は、明示的C compatibility representationとのsemantic field / variant marshallingを用いる
- raw native aggregate bitcopyをdefault C marshalling ruleとしない
- C calling / return classificationはtarget-specific compatibility/backend resultであり、source ownership / lifetime semanticsではない
- compatibility frontend / generated shimがtarget C compilerへexact ABI loweringを委ねることを許す

C ABI aggregate compatibility、stable native ABI、direct target-specific aggregate classifierは v0 core の必須要件ではない。
normative FFI surface自体は§34のとおりDeferredである。

---

# 32. concurrency

**Deferred**

v0 は single-thread を前提とする。

将来:

- ref は thread-local capability
- ptr は persistent token として thread crossing 可能
- safe ref acquisition は synchronization / protection protocol を必要とし得る
- hazard / epoch / RCU / mutex 等を lifetime stability mechanism として統合可能

現在の LifetimeDomain semantics は future concurrency でそのまま静的 loan だけを意味するとは限らない。

重要:

> lifetime stability != noalias

---

# 33. compiler resource semantics

language semantics ではなく implementation requirement として、generic monomorphization に対して少なくとも以下を持つことを推奨する。

- instantiation memoization
- exact cycle detection
- recursion / expansion depth limit
- total instantiation budget
- graceful resource-exhaustion diagnostic
- semantic invalidation summary memoization
- place/effect summary normalization
- opaque projection graph canonicalization
- symbolic `Apply` memoization
- callable result-dependency expression memoization
- recursive call-summary fixpoint / SCC handling
- current-value dependency-state memoization / widening
- whole-compilation-unit analysis cache observation
- post-state precision-loss / Unknown observation
- code-size observation tooling

具体的 threshold は v0 specification では固定しない。

---

# 34. v0 Draft 17.4 の Provisional / Deferred / implementation-later 一覧

v0 compiler の本格実装前または実装中に詰める。

Draft 13でsemantic checker実装前に必要だった:

```text
canonical structural current-state
dependency ownership
place-owned state / value-owned package separation
current-value fact identity
candidate post-state well-formedness
```

は引き続きProvisionalである。

Draft 14ではさらに§24で:

```text
overlap-safe raw byte copy
borrowed Storage authority
claim-relative offset
opaque lifetime-root relocation semantics
```

をProvisionalに進めた。

これらの具体的compiler representationは
`NewLang_v0_semantic_checker_model_Draft17_1.md` をbaseline reference modelとして用いる。
Draft 17.2追加operationのchecker loweringはclosure vectors / implementation updateで補う。
Draft 17.3はdynamic authority ruleとlayout-knowledge boundaryのsemantic consolidationであり、
新しい必須source primitiveを追加しない。M6.2 / M6.3 prototypeをreference pressure modelとして用いる。
Draft 17.4はM7.3–M7.5のbackend/interop pressureをadjudicateし、semantic mechanismを増やさず、
FFI/function-pointer/general external-backing surfaceのv0 statusをDeferredへ閉じる。

Draft 17.4で特に残すProvisional / Deferred / implementation-later事項:

- rooted dynamic-region responsibilityのexact source carrier / receipt/token spelling
- safe prefix / two-range helper APIとregion consume -> whole `Storage` surface
- compiler-provided layout knowledgeを`Layout<T>`等へmaterializeするかcompile-time eraseするか
- layout witnessのexact query API
- address-sensitive valueのin-place lifetime termination / relocation policy
- future zero-sized storable objectのlogical multiplicity model
- FFI / external backingからdynamic region responsibilityをadoptする具体surface (**Deferred**; authority-conservation rule自体はnormative)

1. `storage_len` / `storage_addr` / `copy_raw_bytes` の最終surface spelling
   - semantic parameter structureとordinary-ref authority ruleは§24でProvisional
   - `storage_addr`のauthority-free observation semanticsと§9.3 coherenceはDraft 17.2 closureで定義
   - named arguments / wrapper / intrinsic spellingはAPI polishで決める
2. opaque lifetime-root relocation の最終source spelling / destination raw-responsibility carrier / runtime layout carrier / fresh-token result carrier
   - governing-domain preservation / occupancy conservation / same-place semantics / fresh provenance requirementは§24でProvisional
3. loan primitive の最終 surface syntax（span loan/materializationを含む）
4. LifetimeDomain の concrete construction / transfer / finalization API surface
5. Allocation / allocator の concrete surface API と failure representation
6. general external/static backing API — **Deferred from v0 normative surface**
   - ordinary-safe import obligation自体は§3.1でnormative
   - shared / aliased / device viewをsafe `BackingRegion`へpromotionしないruleもnormative
   - v0 target validationではminimal experimental platform / unchecked hookを使用してよい
   - general ergonomic import/adoption API、multi-view model、device/MMIO semanticsはfuture workとする
7. general sum declaration の最終surface spelling
   - Draft 17.10でknown closed sumに対するqualified constructor + match exact source profileは固定した
   - general sum declaration grammar自体は引き続きProvisional
8. `consume_array` の最終surface spelling
   - whole-value / exact-once / no-partial-move semanticsは§25.2aでDraft 17.2 closure
9. explicit trap / abort / halt primitive
10. persistent function pointer — **Deferred from v0 core/API**
   - existing nonescaping callableとは別のpersistent captureless code-pointer facilityとして将来追加できるspaceを残す
   - known / finite-known targetはexisting body-sensitive analysisを再利用でき、unknown targetは`Unknown`へfallback可能であることをM7で確認済み
   - v0 callableをpersistent/escaping用途へ歪めない
11. normative basic FFI surface — **Deferred from v0 core/API**
   - FFI-specific effect / no-retain / synchronous-callback等のsource contract spellingは固定しない
   - raw out-locationを渡すcaller-provided storage APIも固定しない
   - successful foreign constructionからfresh typed lifetimeを開始するprivileged transitionは§14のextension pointだけを予約し、source primitiveはDeferred
   - foreign-retained location/callback、foreign unwind/non-local transferのsafe semanticsはDeferred
   - raw representationからnontrivial semantic `ValuePackage`を構成するcontract/type boundaryもDeferred
   - experimental compiler hook / C compatibility frontend / generated shimはvalidation目的で使用してよい
12. direct C aggregate ABI lowering / aggregate varargs optimization — **implementation-later**
   - native semanticsやFFI source APIとは独立したbackend concern
   - initial production compilerはgenerated C shimへexact ABI classificationを委ねてよい
   - target-specific direct loweringは実需要とdifferential testingに応じて追加する

array / span authority modelはDraft 10で:

```text
Array<T,N>
scope-bound span<read,T> / span<write,T>
no persistent range token
```

として **Provisional** に進め、Draft 12でも維持する。

Draft 11ではさらにtyped semantic:

```text
swap(ref<write,T>, ref<write,T>) -> unit
```

を **Provisional** に追加し、Draft 12でも維持する。
raw/uninitialized memory exchangeやgeneral relocation semanticsはこの`swap`へ統合しない。

Draft 12ではその破壊試験から、transient value / memory current valueを統一する:

```text
semantic value package
current-value dependency state
surviving-dependency rule
scope-exit compatibility
function/callable boundary non-laundering
```

を **Provisional** に追加した。

`store` / `destroy` はvisible value semanticsではそれぞれ`replace + discard` / `take + discard`相当だが、
dependency-survival checkingではold packageを外へ返さない一つのcombined transitionとして扱う。

Draft 13ではさらに、aggregate root / structural subplaceのcurrent stateを
canonical structural current-stateとして一つのtree/view modelへ整理し、
dependency ownership、current-value fact identity、candidate post-state well-formednessをProvisionalに追加した。

Draft 14ではPrototype 10–18の破壊試験を踏まえ、
新しいgeneral ownership policyを追加せず、
§24のraw byte copy / borrowed Storage authority / opaque root relocationだけを
新たなProvisional semantic surfaceとして統合した。

dynamic occupancy bridge、fallible cleanup、narrow control-subplace exclusivity、
workloadごとのstable allocation / logical handle policyは
既存mechanismで表現できたためlanguage coreには追加しない。


Draft 15ではDraft 14全体レビューを踏まえ、
§24に残っていたsemantic hole / special-case semanticsを閉じた。

特に:

- opaque relocationでsource governing LifetimeDomainをdestinationへ保存
- raw occupancy responsibility conservation
- same-placeをBackingRegion identity + exact rangeで定義
- same-place no-opからending-authority requirementを除去
- `ref<read,Storage>`専用current-value stabilityを削除
- unspecified raw representation state / access property / fresh destination provenanceを明文化

した。

新しいgeneral mechanismは追加していない。

Draft 16ではDraft 15全体破壊レビューを踏まえ、次を閉じた。

- root placement backing relationをplace/state-owned stateとして明文化
- distinct live BackingRegion identitiesのordinary-safe non-alias invariantを追加
- `into_slot<T>`をexact `sizeof(T)` rangeへ限定
- safe field-by-field aggregate constructionをDeferredへ移動
- opaque distinct relocationでplace-owned structural stateをfreshenし、ValuePackageだけをtransferすると明文化

これらも新しいgeneral ownership policyではなく、既存のplacement / occupancy / value-transfer境界のclosureである。

Draft 17ではAPI polish前のclosure reviewを踏まえ、さらに:

- root governing `LifetimeDomain` relationをplace/state-owned stateとして明文化
- ordinary `take` / consume-outではそのrelationをValuePackageへtransferせず、fresh root lifetime-start側でrelationを作ると明文化
- opaque relocationは同じ`DomainId`へのfresh governing relationを作るspecial transitionだと整理
- safe `Storage` / `slot` / `addr`からfuture incarnation用`ptr<T>`をmintするmechanismをv0非導入と決定
- stale partial-aggregate construction wordingをStorage/slot + dynamic metadata modelへ整理

した。新しいgeneral pointer reservation / construction typestate mechanismは追加していない。

Draft 17.3ではM6.2 / M6.3のpressure testと外部調査を踏まえ、dynamic container boundaryを次のように整理した。

- metadataはoccupancyを記述するがauthorityをmint / eraseしない
- hidden vacant/live responsibilityはBackingRegion/root originに結び付いたownerに保存する
- dynamic claim extractionはauthority reconstructionではなくscoped responsibility transferとする
- whole `Storage`はmetadataからrecoverせず、explicit mergeまたはrooted ownerのconsumeで返す
- transition-local guardはsteady-state metadataと別の一時responsibility ownerとして許す
- opaque generic typeのcompiler-provided layout knowledgeをmemory authority / ABI exposureから分離する
- exact receipt/token / `Layout<T>` source spellingはProvisionalに残す

compiler dynamic initialized-index set、implicit object creation、general metadata-derived authority reconstructionは導入していない。


LifetimeDomain / Storage等のauthority-bearing valueのswap自体は一律禁止しない。
exact identity trackingを失った場合は後続safe operationをconservativeにrejectしてよい。

ただし以下は意図的にDeferredする。

- functionからborrowed ref/spanを返すgeneral scope polymorphism
- persistent range token / dangling span
- range-local lifetime-ending authority
- dynamically-many independent LifetimeDomainを束ねるrange loan
- general index/range disjointness solver
- precise interval effect algebra
- source-visible lifetime parameter / region polymorphism
- general source-visible effect annotation
- stable general interprocedural post-current-value / relational state transformer language

module / import / visibility / separate compilation / stable semantic artifact encoding は
Draft 9以降、意図的に **Deferred** とする。

v0ではone semantic compilation unitを採用し、
future導入のために必要なnominal identity / associated ownership / semantic-interface freedomだけをFixする。

binding / control flow / ordinary definite initialization は Draft 2 以降で、
single-assignment + value-flow + availability analysis の Provisional model に置き換えた。

Copy / Discardable property system は Draft 3 以降で Provisional に固定した。

replace / store / take / destroy / consume-out の統一モデル、
lifetime root、semantic dependencyの前身となるcurrent-value dependency、LifetimeDomain transfer/finalization split は
Draft 4 以降で Provisional に固定した。

BackingRegion / Allocation authority / backing dependency /
Storage split-merge / Storage-slot roundtrip / dynamic claim bridge は
Draft 5 以降で Provisional に固定した。Draft 17.3ではdynamic bridgeのauthority semanticsを
metadata reconstructionからrooted responsibility transferへ狭めた。

closed nominal sum / conditional payload occurrence /
semantic dependency / consuming+borrowed match / exhaustiveness /
`Variant(_)` / `Option` / `Result` semantics は
Draft 6 以降で Provisional に固定した。

hidden dependency propagation / hidden phi-block argument /
loop-carried dependency / function parameter propagation /
compiler-generated semantic invalidation summary /
ptr provenanceとblocking dependencyの分離は
Draft 7 以降で Provisional に固定した。

interface-relative structural place /
`Value`・`Occurrence` dependency abstraction /
`Change`・`Reset`・`EndRoot`・`Unknown` effect algebra /
summary substitution / opaque projection identity /
symbolic `SummaryVar` / callable result dependency /
generic deferred semantic compatibility obligationは
Draft 8 で Provisional に固定した。

one semantic compilation unit / declaration identity / closed associated ownership /
future semantic-interface freedomはDraft 9でFixした。

scope-bound contiguous `span<read/write,T>` / zero-length semantics /
Array -> span / dynamic-container localized bridge /
no persistent range tokenはDraft 10でProvisionalにし、Draft 12でも維持した。
Draft 17.3ではspan / container metadataがauthority originではないことを明文化した。

実装時には特に:

- large state の destructure/reconstruct boilerplate
- loop parameter の肥大
- callable block capture restriction
- source-level `move` keyword 不在の diagnostics
- borrowed spanをreturnできないことによるAPI friction
- coarse LifetimeDomainがspan外のcontainer operationまで止める頻度
- coarse range effect summaryによるfalse positive
- current-value dependency state数 / max dependency-set size
- body-sensitive call analysisのcompile time / peak memory
- function / callable summary cache hit率とSCC fixpoint iteration数
- may-set / `Unknown` widening回数
- direct callでは通るがwrapper化でprecision lossによりrejectされる件数
- LifetimeDomain / Storage等のexact identity correlation lossでsafe operationがrejectされる頻度
- scope-exit compatibility rejectionの集中箇所

を観測し、必要なら Draft 17 以降で修正する。

---

# 35. v0 Draft 17.3 の判断基準

この仕様は「正しい最終仕様」であることを目的としない。

v0 compiler を作りながら、以下を観測する。

- `unchecked` がどこに集中するか
- LifetimeDomain が library author にとって自然か
- coarse domain が実用上どれだけ不便か
- non-Discardable generic API が自然か
- negative property restriction (`noncopy` / `nondiscardable`) が十分か
- replace/store と take/destroy のlifetime boundaryがsystems codeで自然か
- `swap` がnon-Copy in-place algorithmsを十分小さく支えられるか
- same-place `swap` no-op semanticsがdiagnostics / optimization / dependency checkingで自然か
- typed `swap` / `take + initialize` / raw byte copy / opaque root relocationの境界がlibrary実装で自然か
- ordinary `ref<read,Storage>`をspecial stability無しでraw-copy authorityとして使う区別がlibrary author / compiler diagnosticsに自然か
- raw backing byte mutationを`Change(StoragePlace)`としないmodelがoptimization / diagnostics双方で明瞭か
- persistent raw-range token無しでgap buffer / packed page / allocator / runtime codeを自然に書けるか
- opaque relocationでsource-self-overlapを許しthird-live-object overlapだけを止めるruleがGC/runtimeで十分か
- opaque relocationのunchecked preconditionがruntime/allocatorへ局所化できるか
- lifetime root preconditionがcontainer/allocator実装で過度なuncheckedを要求しないか
- BackingRegion / Allocation separationがallocator / arena / containerで自然か
- root placement backing relationとvalue-owned backing stateの分離がchecker / diagnosticsで自然か
- root governing-domain relationとvalue-owned `LifetimeDomain` identityの分離がchecker / diagnosticsで自然か
- distinct live BackingRegion non-alias invariantがordinary safe codeでは十分で、OS/FFI special aliasをlocalized boundaryへ閉じ込められるか
- full-range Storage requirementがdeallocation APIを過度に煩雑にしないか
- rooted dynamic-region claim extractionのtrusted proof obligationをcontainer内部へ局所化し、authority conservationを機械的に保てるか
- backing dependency trackingがlexical storageで過度に複雑化しないか
- semantic dependency trackingが局所的に保てるか
- semantic value package / current-value stateの概念がlibrary codeで自然か
- canonical structural current-stateがparent/child stateの二重管理を避けつつ自然に実装できるか
- dependency ownershipをstructural subvalueへ保持するmodelがfalse positiveとmetadata量の両方を抑えられるか
- exact `ValueFact` / `OccurrenceFact` とabstract effect summaryの分離がdiagnostics / implementationで明瞭か
- candidate post-state well-formedness checkerがprimitiveごとのspecial-caseを増やさず実装できるか
- surviving-dependency ruleが`replace/store/take/destroy/swap`を不必要に止めないか
- scope-exit compatibilityだけでfunction/callback/loopの短寿命dependency escapeを十分小さく説明できるか
- hidden dependency phi/may-set近似がcompile-time/state explosionを起こさないか
- structural subplace granularityがfield/payload codeで十分で、general heap-shape analysisを要求しないか
- authority identity correlation lossが`LifetimeDomain` / `Storage` codeでuncheckedを急増させないか
- place-relative effect summaryがwhole-compilation-unit compiler内で小さく保てるか
- opaque projection graphが将来visibilityを追加してもrepresentationを過度に露出しないか
- physical file boundaryをsemantic boundaryにしない方針がtooling上自然か
- nominal associated ownershipがmodule無しでも明瞭に実装できるか
- future artifactをsignature-onlyに固定しない方針でseparate compilation余地を保てるか
- symbolic `Apply` / callback result dependencyがcompile-time/state explosionを起こさないか
- public summary wideningをbreaking contractとして扱う運用が現実的か
- generic deferred semantic obligationがdiagnostic上理解可能か
- pointer graph effectの`Unknown` wideningが実用コードを過度にrejectしないか
- unknown/ambient effect fallbackが実用コードを過度にrejectしないか
- lexical dependency lifetimeでergonomicsが十分か
- payload occurrence dependencyがordinary container/state codeで過度にconservativeでないか
- borrowed match中のpotential alias call rejectionが実用上許容できるか
- consuming match + non-Discardable payloadが自然か
- `Variant(_)`だけでv0 pattern ergonomicsが十分か
- `?`無しの明示的Result propagationがv0として許容できるか
- address-sensitive invariantをprogrammer responsibilityに残す境界が実用的か
- dependent generic lookup が十分か
- inferred requirements の可視性が十分か
- result-type rule が過度に窮屈でないか
- safe field-by-field aggregate constructionをDeferredにしたことでreal systems codeへ過度な制約が出ないか
- safe pre-lifetime ptr mintingを持たないことでself-referential/intrusive constructionに許容不能なfrictionが出ないか
- dynamic partial initializationがsteady-state metadata + rooted responsibility transferだけで十分か
- immutable binding / value-flow model が real systems code で過度な boilerplate を生まないか
- loop-carried state が過度に肥大しないか
- explicit `move` keyword 不在でも diagnostics / ownership transfer が十分明瞭か
- compilation time / memory が許容範囲か
- `span<read/write,T>` がrefのrange版として理解可能か
- persistent range token無しでもreal systems APIを自然に書けるか
- borrowed span return不可がparser / range library等で過度なboilerplateを生まないか
- Array -> span / subspan / element projectionだけでfixed-array codeが自然か
- dynamic containerのspan materialization boundaryをlibrary内部へ局所化し、metadataからaccess authorityをmintしない設計が自然か
- common/coarse LifetimeDomainによるspan stabilityがcontainer操作を過度に止めないか
- overlapping write spansを許す`write != exclusive`がsingle-thread systems codeで自然か
- zero-length spanがpointer sentinel無しで自然に実装できるか
- coarse range effect summaryが実用コードを過度にrejectしないか
- span導入がpointer arithmetic / one-past等のC semanticsをnative coreへ逆流させないか
- C migration path が現実的か

v0 で不自然さが出た設計は、互換性よりも修正を優先してよい。
