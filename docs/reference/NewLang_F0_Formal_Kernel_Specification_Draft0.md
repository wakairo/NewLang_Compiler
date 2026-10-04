# NewLang F0 Formal Kernel Specification — Draft 0

**Status:** Non-normative formalization bridge  
**Source of truth:** `NewLang_v0_spec_Draft17_4.md`  
**Purpose:** Draft 17.4 の semantic core のうち、最初の machine-checked proof に必要な最小部分を切り出す。  
**Non-goal:** この文書自体を新しい言語仕様にしない。Draft 17.4 と矛盾する場合は Draft 17.4 が優先する。

---

## 0. この文書の役割

F0 は NewLang v0 全体の形式化ではない。

最初の目的は、Draft 17.4 で既にかなり安定している以下の核を、proof assistant 上で小さな transition system として定義し、

```text
legal transition
    +
well-formed pre-state
    ->
well-formed post-state
```

を machine-check することである。

対象とする核は次の二つである。

```text
A. current semantic value / dependency kernel

    ValuePackage
    CurrentValueFact
    replace
    store
    swap
    surviving dependency
    candidate post-state well-formedness

B. lifetime / occupancy kernel

    vacant typed place / slot responsibility
    live lifetime root incarnation
    governing LifetimeDomain relation
    initialize
    take
    destroy
```

この二つを同じ abstract state の直交する軸として定義する。

F0 の最初の証明では、source syntax、parser、type checker、LLVM、C ABI、FFI 等を証明対象に含めない。

---

# 1. F0 の成功条件

F0 の最初の milestone は、少なくとも以下を machine-checked theorem として成立させることである。

## F0-T1. Transition preservation

```text
WellFormed(S)
Step(S, op, result, S')
-------------------------
WellFormed(S')
```

ここで `Step` は F0 で定義した合法な semantic transition relation である。

## F0-T2. Dependency non-laundering

transition 後へ survive する semantic package の dependency は、post-state で live な fact だけを参照する。

```text
WellFormed(S')
->
Deps(AllSurvivingPackages(S'))
    ⊆
LiveFacts(S')
```

これは Draft 17.4 §13.5a の candidate post-state formulation を直接形式化する。

## F0-T3. Lifetime/occupancy exclusivity

一つの F0 root location は同時に、

```text
vacant typed place
live lifetime root
```

の両方にはならない。

## F0-T4. `initialize` / `take` round-trip conservation

合法な:

```text
vacant
  --initialize-->
live root
  --take-->
vacant
```

は、root location の occupancy responsibility を duplicate / lose しない。

## F0-T5. Root governing-domain preservation

live root は exactly one live `DomainId` に governed される。

`replace` / `store` / `swap` はこの root governing relation を変更しない。

`take` / `destroy` は relation を終了する。

`initialize` は指定された live domain への fresh relation を作る。

---

# 2. F0 で意図的に扱わないもの

F0 Draft 0 では以下を formal state へ入れない。

これは「重要でない」という意味ではなく、最初の theorem に不要であり、後から orthogonal extension として追加できるためである。

## 2.1 Structural aggregate state

以下は F1 候補とする。

- fixed field / subobject place tree
- ancestor / descendant current-value invalidation
- known-disjoint sibling relation
- aggregate whole-value update
- array elements
- structural projection identity

F0 では place は **flat** とする。

したがって F0 の semantic overlap は:

```text
overlap(P, Q) := P == Q
```

である。

後にこれを structural overlap relation へ一般化する。

## 2.2 Conditional occurrence

以下は F1 候補とする。

- sum active payload
- `OccurrenceFact`
- `Reset(P)`
- conditional descendant
- payload ref lifetime

F0 Draft 0 の dependency atom は current-value dependency を中心とする。

## 2.3 Scope dependency

以下は後段とする。

- lexical scope identity
- block result
- return / break / continue
- nonescaping callable
- scope-exit compatibility
- transient dependency liveness

F0 Draft 0 では、すべての surviving package を state 内に明示することで、まず semantic-value dependency だけを証明する。

## 2.4 Function boundary

以下は F2 候補とする。

- interface-relative `Place`
- `Change / Reset / EndRoot / Unknown`
- symbolic substitution
- wrapper / alias actual
- recursion / loop fixpoint
- callbacks
- generic dependent calls
- public semantic summaries

## 2.5 Backing geometry

F0 Draft 0 では `BackingRegion` identity / byte range / alignment の詳細 geometry を証明しない。

必要なのは:

```text
RootLocationId
```

が一つの typed occupancy site を表すことだけである。

後に:

```text
RootLocationId
    -> BackingRegionId × ByteRange
```

へ精密化する。

したがって F0 は Draft 17.4 の ordinary-safe distinct-BackingRegion non-alias invariantを否定しない。
単にその幾何学的 proof をまだ行わない。

## 2.6 Opaque relocation

`relocate` は F1/F2 候補とする。

理由:

- source root end
- destination fresh root start
- same DomainId への fresh governing relation
- occupancy responsibility conservation
- fresh provenance

を同時に扱うため、最初の theorem を不必要に大きくする。

## 2.7 FFI / external backing / C ABI / LLVM

すべて F0 外。

M7 で得られた backend / interop closure は、F0 semantic kernel の theorem を成立させるためには不要である。

---

# 3. Formalization boundary

## 3.1 Normative / non-normative separation

F0 では三層を区別する。

```text
Draft 17.4
    normative semantics

F0 Formal Kernel Specification
    normative semantics の小さい抽出・対応表

Lean representation
    theorem proving のための concrete encoding
```

Lean 側で例えば:

```lean
abbrev PlaceId := Nat
```

と書いても、NewLang specification が `PlaceId = Nat` と定めたことにはならない。

同様に:

- finite map
- list
- finite set
- natural-number fresh id
- inductive datatype

等は proof representation であり、runtime representation でも compiler IR requirement でもない。

## 3.2 Formalization difficulty is not a language-design criterion

原則:

> Lean で証明しにくいという理由だけで Draft 17.4 semantics を変更しない。

proof が詰まった場合は少なくとも:

```text
A. Draft 17.4 に soundness hole がある
B. Draft 17.4 の prose が曖昧
C. F0 extraction が誤っている
D. Lean representation が悪い
E. 補助 lemma / invariant が不足
```

に分類してから判断する。

---

# 4. Primitive identities

F0 は以下の identity sort を持つ。

```text
PlaceId
IncarnationId
ValueFactId
DomainId
PackageId
RootLocationId
```

これらは互いに nominally distinct とする。

## 4.1 `PlaceId`

typed live place の semantic identity。

F0 Draft 0 では structural parent/child relationを持たない。

## 4.2 `RootLocationId`

typed lifetime root を置ける occupancy site。

F0 Draft 0 では backing/range geometryを抽象化した identity とする。

各 `PlaceId` は exactly one `RootLocationId` に対応してよい。

最初の Lean model では簡単のため:

```text
PlaceId == RootLocationId
```

としてもよい。

ただしこれは proof encoding choice であり、normative identity equalityではない。

## 4.3 `IncarnationId`

同じ root location で lifetime が再開始された場合も fresh になる。

```text
initialize after take
    ->
new IncarnationId
```

old ptr 等が将来追加された場合、old incarnation と new incarnation を区別するために使う。

## 4.4 `ValueFactId`

ある place の current semantic value fact identity。

lifetime incarnation とは別物である。

```text
replace/store/distinct swap
    -> fresh ValueFactId

same-place swap
    -> same ValueFactId
```

## 4.5 `DomainId`

`LifetimeDomain` semantic identity。

domain value の machine location ではない。

## 4.6 `PackageId`

F0 proof model 上で semantic package の carrier identity を追跡するために使ってよい。

`PackageId` 自体は NewLang source semantic value の user-visible identity を意味しない。

---

# 5. Semantic facts

F0 Draft 0 の `Fact` は最小限:

```text
Fact ::=
    ValueFact(PlaceId, ValueFactId)
  | DomainLive(DomainId)
```

とする。

将来:

```text
OccurrenceFact(...)
ScopeLive(...)
BackingLive(...)
```

等を追加できる。

## 5.1 `LiveFacts(S)`

state `S` から導出される fact 集合。

少なくとも:

```text
for each live place P:
    ValueFact(P, currentFact(P)) ∈ LiveFacts(S)

for each live domain D:
    DomainLive(D) ∈ LiveFacts(S)
```

である。

`LiveFacts` は state の secondary derived view とし、第二の mutable source of truth にしない。

---

# 6. ValuePackage

F0 の semantic package:

```text
ValuePackage {
    payload        : OpaqueValue
    dependencies   : FiniteSet<Fact>
    authorities    : FiniteSet<AuthorityAtom>   // optional in F0.0
    discardable    : Bool                       // proof-side static abstraction
}
```

## 6.1 `payload`

visible semantic value は F0 では opaque。

整数、struct、domain value等の内部構造は証明しない。

F0 の theorem は payload の具体値に依存しない。

## 6.2 `dependencies`

Draft 17.4 の hidden semantic dependencies の F0 representation。

F0 Draft 0 では主に:

```text
ValueFact(P, V)
```

への dependency を扱う。

## 6.3 `authorities`

authority conservation theorem を追加するときの extension point。

最初の preservation proof では空集合または uninterpreted finite set としてよい。

将来ここへ:

- `LifetimeDomain` value identity
- `Storage` claim
- `Allocation` authority
- ptr provenance

等の value-owned authority を精密化できる。

## 6.4 `discardable`

`store` / `destroy` の static applicability を kernel transition 上で表すための abstraction。

これは runtime bit を要求しない。

Lean 上では:

```text
Discardable : Package -> Prop
```

としてもよい。

---

# 7. Package carriers

dependency-survival を正しく表すため、F0 state は installed current value だけでなく、transition 後へ survive する transient/result package も表す。

conceptually:

```text
Carrier ::=
    Installed(PlaceId)
  | Loose(PackageId)
```

`Loose` は:

- function local value
- operation result
- transition 後へ存続する abstract value

の proof-level representative である。

source syntax の `let` 等を直接意味しない。

## 7.1 Exactly-one carrier

F0 の最初の model では、一つの tracked `PackageId` は同時に複数 carrier に存在しない。

これにより `replace` / `take` / `swap` の transfer を明確化する。

Copy semantics は F0 Draft 0 では operation として扱わない。

後に Copy package の duplication rule を追加できる。

---

# 8. Root occupancy state

各 `RootLocationId` は conceptually:

```text
Occupancy ::=
    Vacant
  | LiveRoot {
        place          : PlaceId
        incarnation    : IncarnationId
        currentFact    : ValueFactId
        package        : PackageId
        governing      : DomainId
    }
```

の exactly one state を持つ。

## 8.1 `Vacant`

Draft 17.4 の `slot<T>` empty occupancy responsibility を F0 では root-location state として抽象化する。

これは surface `slot<T>` value を否定しない。

F0 の目的は occupancy conservation の theorem であり、slot token の source-level move semantics は後段で精密化できる。

## 8.2 `LiveRoot`

lifetime-root incarnation が現在 live。

少なくとも:

- fresh `IncarnationId`
- current `ValueFactId`
- installed `PackageId`
- governing `DomainId`

を持つ。

---

# 9. State

F0 state の reference shape:

```text
State {
    occupancy      : RootLocationId -> Occupancy
    packages       : PackageId -> ValuePackage
    loosePackages  : FiniteSet<PackageId>
    liveDomains    : FiniteSet<DomainId>
}
```

`LiveRoot.package` と `loosePackages` が package carrierを表す。

実際の Lean encoding は変更してよい。

例えば carrier map:

```text
carrier : PackageId -> Option Carrier
```

へ正規化してもよい。

---

# 10. WellFormed

F0 の中心 invariant を `WellFormed(S)` とする。

少なくとも以下の conjunction とする。

## WF-1. Carrier uniqueness

tracked package は exactly one carrier を持つ。

```text
PackageId p:
    installed at one place
    XOR
    loose
    XOR
    absent/consumed
```

同じ package が二つの placeへ同時に installされない。

## WF-2. Occupancy exclusivity

各 root location は exactly one:

```text
Vacant
LiveRoot
```

である。

## WF-3. One current package per live place

live root は exactly one current package を持つ。

## WF-4. One current value fact per live place

live root は exactly one current `ValueFactId` を持つ。

その fact:

```text
ValueFact(P, V)
```

だけが `P` の current value fact として `LiveFacts(S)` に存在する。

past fact は current factとしてliveではない。

## WF-5. Governing domain liveness

```text
LiveRoot(... governing = D ...)
    ->
D ∈ liveDomains
```

## WF-6. Surviving dependency validity

state 内に存在するすべての surviving packageについて:

```text
dependencies(pkg)
    ⊆
LiveFacts(S)
```

installed / loose のどちらも含む。

これが F0 の最重要 invariant である。

## WF-7. No carrier references missing package

carrier が参照する `PackageId` は `packages` 内に定義されている。

## WF-8. Package ownership consistency

一つの live root の current package は、その root の installed carrier と一致する。

F0.0 ではこれ以上の authority algebraを要求しない。

---

# 11. Freshness

transition は以下を fresh に生成し得る。

```text
IncarnationId
ValueFactId
PackageId   // proof representation上必要な場合
```

freshness:

```text
fresh(x, S)
```

は、`S` 内で現在または履歴上追跡対象の identity と衝突しないことを意味する。

最初の Lean model では:

```text
nextId
```

counterを state に含めてもよいが、normative requirementではない。

より抽象的には transition relation の existential fresh variable としてよい。

---

# 12. Operation input values

`replace` / `store` / `initialize` が受け取る `new_value` は、F0 state 内では `Loose(package)` として存在するとする。

したがって operation は package を生成するのではなく transfer する。

例:

```text
Loose(newPkg)
  --replace into P-->
Installed(P, newPkg)
```

これにより dependency / authority の transferを明示できる。

---

# 13. Candidate-post-state rule

各 operation は:

1. structural candidate post-state を作る
2. `WellFormed(candidate)` を検査する
3. well-formedなら `Step` として成立する

ものとして定義する。

conceptually:

```text
RawStep(S, op, r, S_candidate)
    &&
WellFormed(S_candidate)
    ->
Step(S, op, r, S_candidate)
```

これにより Draft 17.4 の:

```text
Deps(all surviving packages)
    ⊆
LiveFacts(candidate post-state)
```

を transition legality の共通原理として使う。

個別 operation の dependency conflict ruleを二重に定義しない。

---

# 14. `replace`

入力:

```text
replace(P, newPkg) -> oldPkg
```

preconditions:

- `P` は live
- `newPkg` は loose
- destination への write authorization は成立している
- type compatibility は F0 外の static premiseとして成立している

raw transition:

```text
before:
    P = LiveRoot(O, VF_old, oldPkg, D)
    newPkg is Loose

after:
    P = LiveRoot(O, VF_new, newPkg, D)
    oldPkg becomes Loose result
    newPkg no longer Loose
```

where:

```text
VF_new is fresh
```

preserved:

```text
PlaceId
IncarnationId O
DomainId D
root occupancy = LiveRoot
```

changed:

```text
current ValueFact
current package
```

## REPLACE-1. Old package survives

`oldPkg` は result として surviveする。

したがって:

```text
oldPkg.dependencies contains ValueFact(P, VF_old)
```

なら `VF_old` が post-stateから消えるため candidate state は `WF-6` を満たさない。

つまり reject。

## REPLACE-2. Incoming package also survives

`newPkg` の dependency も installed current value として surviveする。

従って `newPkg` が invalidated fact に依存する場合も reject。

## REPLACE-3. No exclusive lifetime authority required

F0 transition semantics は root incarnation を終了しない。

したがって ending-authority preconditionは持たない。

---

# 15. `store`

入力:

```text
store(P, newPkg) -> unit
```

preconditions:

- `P` live
- `newPkg` loose
- write authorization
- old package is discardable

raw transition:

```text
before:
    P = LiveRoot(O, VF_old, oldPkg, D)

after:
    P = LiveRoot(O, VF_new, newPkg, D)
    oldPkg is consumed/discarded
```

`oldPkg` は post-stateへ surviveしない。

そのため old package **だけ** が:

```text
ValueFact(P, VF_old)
```

へ依存していた場合、その dependency は blocker ではない。

ただし:

- another loose package
- another installed package
- incoming `newPkg`

に同じ dependency が surviveするなら candidate state は rejectされる。

これは `store` を literal `replace` then `discard` として legality checkしない理由を形式化する。

---

# 16. `swap`

入力:

```text
swap(A, B) -> unit
```

## 16.1 Same-place

```text
A == B
```

なら semantic no-op。

```text
S' = S
```

current fact も package も変更しない。

## 16.2 Distinct-place

preconditions:

- A, B live
- same semantic type
- both write-authorized
- `A != B`

before:

```text
A = LiveRoot(OA, VFA, pkgA, DA)
B = LiveRoot(OB, VFB, pkgB, DB)
```

after:

```text
A = LiveRoot(OA, VFA', pkgB, DA)
B = LiveRoot(OB, VFB', pkgA, DB)
```

with fresh:

```text
VFA'
VFB'
```

preserved:

```text
OA
OB
DA
DB
```

both packages survive。

従って pkgA/pkgB が old current factsへ dependencyを持つ場合、post-state `WF-6` が失敗し得る。

「相手placeへ一緒に移ったから dependency も安全」という exemption は設けない。

---

# 17. `initialize`

F0 abstraction:

```text
initialize(L, valuePkg, D)
    -> fresh root at L
```

preconditions:

```text
occupancy(L) == Vacant
valuePkg is Loose
D ∈ liveDomains
ordinary stability/domain authorization premise holds
```

post-state:

```text
occupancy(L) =
    LiveRoot(
        place       = P,
        incarnation = O_fresh,
        currentFact = VF_fresh,
        package     = valuePkg,
        governing   = D
    )
```

`valuePkg` は no longer Loose。

fresh:

```text
O_fresh
VF_fresh
```

`initialize` は:

- old live root を終了しない
- prior incarnation identity を再利用しない
- packageに含まれる source placement を governing relationとして復元しない
- D への fresh root-state governing relationを作る

F0 Draft 0 では `ptr<T>` result はまだ stateへ入れない。

F0 reference extensionで:

```text
PtrToken(L, O_fresh)
```

を返す。

---

# 18. `take`

F0 abstraction:

```text
take(L, endingAuthorityFor(D))
    -> (oldPkg, Vacant L)
```

preconditions:

- `L` is `LiveRoot`
- target is lifetime root（F0 では全live occupancyがroot）
- governing domain = D
- Dへのexclusive lifetime-ending authorization premise
- dependency legalityはcandidate post-state `WellFormed`で判定

before:

```text
L = LiveRoot(P, O, VF_old, oldPkg, D)
```

after:

```text
L = Vacant
oldPkg becomes Loose result
```

ended:

```text
Incarnation O
ValueFact(P, VF_old)
governing relation O -> D
```

`DomainId D` 自体は終了しない。

old packageは resultとして surviveするため、ended factへのdependencyをcarryするなら rejectされ得る。

---

# 19. `destroy`

F0 abstraction:

```text
destroy(L, endingAuthorityFor(D))
    -> Vacant L
```

preconditions:

- `take` と同じlifetime/root/domain authorization
- current package is discardable

post-state:

```text
L = Vacant
oldPkg consumed/discarded
```

`destroy` legality は:

```text
take then return old package
```

として一度成立させてからdiscardするのではない。

old packageは post-stateに surviveしない。

そのため old packageだけに存在した dependency は、自身のdestructionをblockしない。

---

# 20. LifetimeDomain

F0 Draft 0 では `LifetimeDomain` を semantic identity `DomainId` として扱う。

state:

```text
liveDomains : FiniteSet<DomainId>
```

live root:

```text
governing : DomainId
```

を exactly one 持つ。

## 20.1 Domain value location is excluded

`LifetimeDomain` value自身をどのplaceに格納するかは F0.0 では証明しない。

つまりまず:

```text
DomainId semantic identity
```

と:

```text
place holding LifetimeDomain value
```

を分離する Draft 17.4 の判断を利用する。

## 20.2 Domain transfer

F0.0では domain value transferを operationとして定義しない。

F0.2で追加する。

追加時の requirement:

```text
DomainId survives
governed roots survive
value carrier may move
```

## 20.3 Domain finalization

F0.2候補:

```text
finalizeDomain(D)
```

preconditions:

```text
D ∈ liveDomains
no live root governed by D
no surviving package depends on DomainLive(D)
```

post-state:

```text
D ∉ liveDomains
```

最終 theorem候補:

```text
finalizeDomain cannot strand a governed live root
```

---

# 21. Authorization abstraction

F0 は borrow checker全体をまだ証明しない。

したがって primitive transition の capability requirementは最初は proposition parameter とする。

例:

```text
CanWrite(S, P)
CanEndRoot(S, P, D)
CanInitialize(S, L, D)
```

重要:

これらを「常にtrue」として theorem を弱めてはならない。

ただし F0 transition preservation theorem は:

```text
authorization premises hold
```

を仮定して semantic-state preservationだけを証明してよい。

F0 reference extensionでこれらを:

- `ref<write,T>`
- ordinary `ref<read,LifetimeDomain>`
- `exclusive ref<read,LifetimeDomain>`

の derivation ruleへ接続する。

---

# 22. F0 reference extension

Transition kernel が安定した後、次を追加する。

## 22.1 PtrToken

```text
PtrToken {
    location     : RootLocationId
    incarnation  : IncarnationId
}
```

persistent / Copy。

`take` / `destroy` で自動消去しない。

したがって stale tokenを表現できる。

## 22.2 AcquireRef

conceptual:

```text
acquireRef(ptr, stableDomainEvidence)
```

が成功するには:

```text
occupancy(ptr.location) == LiveRoot(... incarnation = ptr.incarnation ...)
governing domain == stable evidence domain
```

が必要。

これにより theorem:

```text
AcquireRef succeeds
    ->
target incarnation is currently live
```

を得る。

さらに:

```text
take/destroy old incarnation
initialize same location with fresh incarnation
```

後もold ptrのincarnation idは一致しないため、safe refを再取得できない。

## 22.3 Ref scope

lexical scope / dependencyは F0 reference extensionの第二段階。

最初はref acquisition theoremだけでよい。

---

# 23. Authority conservation extension

F0.0 の `ValuePackage.authorities` は uninterpreted でもよい。

次段では:

```text
AuthorityAtom
```

に少なくとも affine/noncopy authorityを表現し、

```text
AuthorityMultiplicity(S, a) <= 1
```

を `WellFormed`へ追加できる。

候補:

- `StorageClaim`
- `AllocationAuthority`
- `LifetimeDomainValue`
- exclusive authority token

ただし ordinary Copy `ref` 等と affine authorityを同じ algebraへ無理に統合しない。

---

# 24. Draft 17.4 との traceability

| F0 concept | Draft 17.4 source | F0 treatment |
|---|---|---|
| Backing / occupancy | §3.1–§3.7 | geometryを`RootLocationId`へ抽象化 |
| `slot<T>` responsibility | §3.4, §3.6 | `Vacant` stateとして抽象化 |
| object incarnation | §3.5 | `IncarnationId` |
| lifetime root | §3.6 | F0 live occupancyはすべてroot |
| `ptr<T>` | §10 | initial kernel外、reference extensionで追加 |
| ordinary `ref` | §11 | authorization premiseとして開始 |
| `LifetimeDomain` | §13 | `DomainId` + governing relation |
| safety implication | §13.5 | reference extensionで接続 |
| `ValuePackage` | §13.5a | central F0 datatype |
| current value fact | §13.5a | `ValueFactId` |
| surviving dependency | §13.5a | `WF-6` |
| candidate post-state | §13.5a | `RawStep + WellFormed` |
| effect algebra | §13.5b | F0では直接使わず、後にStepから導出 |
| `initialize` | §14.1 | primitive Step |
| `take` | §14.2 | primitive Step |
| `destroy` | §14.3 | primitive Step |
| fixed subobject restriction | §14.4 | flat F0では自動的に満たす |
| domain transfer/finalize | §14.5–§14.6 | F0.2 extension |
| `replace/store/swap` | §17.4 | primitive Step |

この表は formalization の coverage map であり、Draft 17.4の節を置換しない。

---

# 25. F0 で最初に書く Lean declarations

conceptual skeleton:

```lean
namespace NewLang.F0

opaque PlaceId       : Type
opaque IncarnationId : Type
opaque ValueFactId   : Type
opaque DomainId      : Type
opaque PackageId     : Type
opaque RootLocationId : Type

inductive Fact
  | value  : PlaceId -> ValueFactId -> Fact
  | domain : DomainId -> Fact

structure ValuePackage where
  deps        : Finset Fact
  discardable : Bool

inductive Occupancy
  | vacant
  | live (place : PlaceId)
         (inc : IncarnationId)
         (fact : ValueFactId)
         (pkg : PackageId)
         (domain : DomainId)

structure State where
  occupancy     : RootLocationId -> Occupancy
  packages      : PackageId -> Option ValuePackage
  loosePackages : Finset PackageId
  liveDomains   : Finset DomainId
```

実装時には decidable equality / finite-support representation等に合わせて調整する。

重要なのは datatype 名ではなく invariant structureである。

---

# 26. 最初の Lean predicates

conceptually:

```lean
def LiveFacts (s : State) : Set Fact := ...

def IsInstalled (s : State) (pkg : PackageId) : Prop := ...

def Survives (s : State) (pkg : PackageId) : Prop :=
  IsInstalled s pkg || pkg ∈ s.loosePackages

def DependenciesValid (s : State) : Prop :=
  ∀ pkg,
    Survives s pkg ->
    packageDeps s pkg ⊆ LiveFacts s

def CarrierUnique (s : State) : Prop := ...

def DomainsValid (s : State) : Prop := ...

def WellFormed (s : State) : Prop :=
  CarrierUnique s
  ∧ DependenciesValid s
  ∧ DomainsValid s
  ∧ ...
```

---

# 27. 最初の transition relation

まず executable function より relational semanticsを優先する。

```lean
inductive RawStep : State -> Op -> Result -> State -> Prop
```

と:

```lean
def Step (s : State) (op : Op) (r : Result) (s' : State) : Prop :=
  RawStep s op r s' ∧ WellFormed s'
```

の形を第一候補とする。

理由:

- freshnessを existential に扱いやすい
- normative semanticsを特定algorithmへ固定しない
- later implementation checkerとの refinement proofを分離できる

---

# 28. 最初の proof sequence

## F0.0 — State invariant only

1. identity types
2. `Fact`
3. `ValuePackage`
4. flat `State`
5. `LiveFacts`
6. `WellFormed`

この段階では operationなし。

## F0.1 — `replace`

最初の本格 theorem:

```text
replace_step_preserves_well_formed
```

さらに negative lemma:

```text
old package depends on old current fact
    ->
no legal replace Step exists
```

これが最初の semantic dependency break-test。

## F0.2 — `store`

証明:

```text
old-only dependency may disappear with discarded old package
```

これにより `replace` と `store` の semantic differenceを machine-checkする。

## F0.3 — `swap`

証明:

```text
same-place swap is identity
distinct swap preserves incarnations/domains
distinct swap cannot launder old current-value dependency
```

## F0.4 — lifetime / occupancy

`initialize`, `take`, `destroy`を追加。

証明:

```text
vacant/live exclusivity
initialize creates fresh incarnation
take returns to vacant
destroy returns to vacant
governing relation lifecycle
```

## F0.5 — ptr/ref acquisition

persistent ptr incarnation identityを追加。

証明:

```text
ended incarnation ptr cannot acquire safe ref
reinitialize same location does not revive old ptr
```

---

# 29. Primary theorem candidates

実装時の theorem name 候補:

```text
wellFormed_livePkg_deps_live
replace_preserves_wellFormed
replace_preserves_incarnation
replace_preserves_governingDomain

store_preserves_wellFormed
store_can_eliminate_old_only_dependency

swap_same_is_identity
swap_distinct_preserves_incarnations
swap_distinct_preserves_domains
swap_does_not_launder_dependencies

initialize_preserves_wellFormed
initialize_creates_fresh_incarnation
initialize_consumes_vacancy

take_preserves_wellFormed
take_ends_incarnation
take_restores_vacancy
take_does_not_transfer_governing_relation

destroy_preserves_wellFormed
destroy_old_package_does_not_survive

finalize_domain_no_governed_roots

acquire_ref_implies_live_incarnation
stale_ptr_cannot_acquire_ref
reinitialize_does_not_revive_old_ptr
```

---

# 30. Counterexample-oriented proof policy

F0 は proof completionだけでなく specification break-testとして使う。

各 major theorem では、少なくとも一つ「preconditionを外すと反例が作れる」ことも確認する。

例:

## 30.1 Replace

surviving-dependency checkを削ると:

```text
oldPkg depends on ValueFact(P, oldVF)
replace P
oldPkg survives
oldVF dead
```

という malformed post-stateが作れる。

## 30.2 Store

old packageを誤ってsurvivor扱いすると、本来合法な:

```text
old self-dependent package
store(P, independent new)
```

をfalse rejectする。

これは soundnessではなく precision/spec mismatch。

## 30.3 Take

root/domain-ending authority premiseを削ると lifetime stability modelとの接続時に unsound。

## 30.4 Fresh incarnation

`initialize` が old `IncarnationId` を再利用すると stale ptr revivalが起こり得る。

F0.5で明示的反例とする。

---

# 31. What F0 does not prove

F0 completion時にも、以下を主張してはならない。

- NewLang v0 全体の type safety
- parser/type checker correctness
- source program progress/preservation
- compiler analysis completeness
- absence of false rejects
- LLVM lowering correctness
- C ABI correctness
- FFI safety
- concurrency safety
- aggregate/sum soundness
- raw storage geometry correctness
- allocator correctness
- termination

F0 が直接証明するのは:

> 採用済みsemantic kernelの限定されたtransition systemが、その明示したglobal invariantsを保存すること。

---

# 32. F0 から次へ進む gate

F0を「成功」と扱う最低条件:

```text
1. Lean projectがversion-pinnedでbuildする
2. F0 definitionsにsorry / axiomによるsemantic shortcutがない
3. replace/store/swap preservationがmachine-checked
4. initialize/take/destroy preservationがmachine-checked
5. at least one intentionally broken ruleからcounterexampleを再現
6. Draft 17.4 traceability tableが維持される
7. proof difficultyのためだけのDraft 17.4 semantic changeをしていない
```

`Classical.choice` 等の通常のlogic facility使用は「semantic shortcut」とはみなさない。
問題にするのは、証明したい核心を `axiom` で仮定すること。

---

# 33. Proposed repository shape

```text
newlang-formal/
├── lean-toolchain
├── lakefile.toml
├── README.md
├── docs/
│   └── F0_Formal_Kernel_Specification.md
└── NewLang/
    └── F0/
        ├── Id.lean
        ├── Fact.lean
        ├── Package.lean
        ├── State.lean
        ├── WellFormed.lean
        ├── Transition.lean
        ├── Replace.lean
        ├── Store.lean
        ├── Swap.lean
        ├── Lifetime.lean
        ├── Domain.lean
        ├── Pointer.lean
        └── Proof/
            ├── Preservation.lean
            ├── Dependency.lean
            ├── Occupancy.lean
            └── StalePointer.lean
```

最初から全fileを作る必要はない。

F0.0は:

```text
Id.lean
Fact.lean
Package.lean
State.lean
WellFormed.lean
```

だけで始めてよい。

---

# 34. Adjudication rules during formalization

formalizationからDraft 17.4へのfeedbackは、次の分類で記録する。

```text
FORMAL-HOLE
    well-formed reachable stateからspec上legalなstepで
    invariant violationが導ける

FORMAL-AMBIGUITY
    Draft proseから複数の非同値formal transitionが読める

FORMAL-EXTRACTION
    F0文書の抽象化がDraftとずれていた

FORMAL-ENCODING
    Lean representationだけの問題

FORMAL-LEMMA
    semanticsは明確だがproof decompositionが不足

FORMAL-SCOPE
    F0外の機構が必要になった
```

`FORMAL-HOLE` / `FORMAL-AMBIGUITY` だけを specification change候補とする。

---

# 35. Draft 0 judgment

Draft 17.4 には、F0開始のために不足している新しい言語機構は見当たらない。

特に最初の形式化に必要な核は既に:

```text
place-owned state
vs
value-owned ValuePackage

current-value fact identity

surviving dependency

candidate post-state well-formedness

lifetime-preserving replace/store/swap

lifetime-ending initialize/take/destroy boundary

root governing LifetimeDomain relation
```

として分離されている。

したがって最初のLean modelでは、新しいsemantic ruleを発明せず、

> Draft 17.4 の既存ruleをflat-place / flat-root modelへ忠実に抽出する

ことを優先する。

最初の実装対象は `replace` とする。

理由は:

- current-value fact
- package transfer
- dependency survival
- incarnation preservation
- governing-domain preservation

というF0の中心概念を最小の一操作で同時に試せるためである。

`replace_preserves_well_formed` が自然に証明できれば、その同じ machineryを `store` / `swap` / lifetime transitionへ育てる。

逆にこの最小caseでproofが不自然に複雑になるなら、Draft 17.4、F0 extraction、Lean encodingのいずれに原因があるかを早期に切り分ける。
