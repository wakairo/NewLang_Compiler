# NewLang v0 仕様書（Draft 17.4）

> 状態: **設計検証用ドラフト**
>
> この文書は、NewLang v0 コンパイラを実装し、設計をスクラップ・アンド・ビルドするための仕様書である。
> ISO 規格相当の網羅性・厳密性・用語整備を目標としない。
>
> v0 で最も重要なのは、**実装可能な意味論の核を明示し、設計上の仮定を検証可能にすること**である。
> 表面構文は、意味論が確定している場合でも暫定でよい。



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

## 3.3 Storage

`Storage` は、liveな BackingRegion 内の
**現在 raw である byte range に対する affine occupancy claim** である。

性質:

- non-Copy
- non-Discardable
- exactly one BackingRegion identity に属する
- backing range を保持する
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

は元claimをconsumeし、
同じ BackingRegion 内の互いにdisjointな二つのraw claimsを返す。

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
return x
break x
continue(x)
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
- 対象 incarnation が governing `LifetimeDomain` `D` に属する
- stability evidence として渡された ordinary domain ref が **同じ `D`** を参照する
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

生成された `ref` は `stable` に **scope-dependent** であり、`stable` より長生きしてはならない。

さらにconditional subobject等、root stabilityだけでは存在継続を保証できない対象では、型固有のsemantic dependencyを追加する。
`ref<read,LifetimeDomain>`だけでconditional payload occurrenceの継続まで保証されるわけではない。

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
    if cond {
        ref_a        // depends on F_a
    } else {
        ref_b        // depends on F_b
    }

hidden:
    dep_r = phi(F_a, F_b)
```

memory current-value stateでも同様に:

```text
if cond {
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

### loop-carried dependency

`continue(values...)` がnext iterationのfresh loop parameter bindingsへvalueを渡す時、
各valueのhidden dependencyも同じcontrol-flow edgeを通る。

caller-visible / outer current-value stateをloop中に変更する場合も、
compilerはhidden loop-carried memory stateまたはsoundなfixpoint / may-setとして扱ってよい。

iterationごとにfresh semantic occurrenceが生じる場合も、
hidden phi / recursive block argumentとして表現できるため、
compilerがoccurrence identityのinfinite static setを列挙する必要はない。

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

は対象 **lifetime-root** object incarnationのlifetimeを終了し、

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
- target / overlapping stateの終了によりinvalidになるdependencyを持ち、かつ`take` result等としてtransition後へsurviveするconflicting capability/valueがない
- operationに必要なprovenance / access / representation条件が成立する

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

`destroy` はvisible value / occupancy semanticsとしてconceptually:

```text
(value, slot) = take(...)
discard(value)
return slot
```

である。

ただしdependency-survival checkingでは`take`を独立に成立させた後に`discard`するliteral desugaringとはみなさず、
old semantic value packageを外部へ返さず終了させる **一つのdestroy transition** として扱う。

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
metadata says empty  -> mint whole Storage
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

概念例:

```text
let Pair { a, b } = pair
```

`pair` が non-Copy ならこの destructuring は `pair` 全体を consume し、
fresh bindings `a`, `b` を生成する。

aggregate field が Copy の場合は、aggregateをconsumeせずfield valueだけをcopyしてよい。

例:

```text
let fd_copy = file.fd
```

`fd` が Copy なら `file` は Available のままである。

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

Draft 12 v0 はone visibility domainなので、
representation hidingに依存するabstraction enforcementまでは提供しない。

これはv0 prototypeの既知の制限であり、
将来visibilityを追加してもwhole-value destructuring semantics自体は変更しない。

## 16.2 aggregate construction / safe in-place partial construction

ordinary safe v0 coreでは、**まだliveでないaggregate rootのfieldだけを個別にlifetime-startする
safe field-by-field in-place construction protocolを持たない**。

ordinary aggregateはまずsemantic valueとして構築する。

概念例:

```text
let value = Pair {
    a: make_a(),
    b: make_b(),
}

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

---

# 17. typed location projection

## 17.1 live aggregate field

```text
ref<write, Pair> -> ref<write, A>
```

のような statically-known field projection は safe。

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

```text
fn f(a: A, b: B) -> R
```

の parameter は semantic には fresh initialized value binding である。

parameter type 自身が caller から callee へ渡すものを表す。

例:

- `T`: value
- `ptr<T>`: persistent location token
- `ref<read,T>`: scoped read capability
- `ref<write,T>`: scoped write capability
- affine authority: authority value

## 18.2 argument passing

argument expression は source order で評価し、通常の value-use 規則を適用する。

existing Copy binding を argument に使うと copy。
existing non-Copy binding を argument に使うと consume / ownership transfer。

```text
f(x)
```

だけでよく、explicit `move` keyword は存在しない。

callee parameter は transfer された値を持つ fresh binding になる。

argument valueがhidden scope / semantic dependencyを持つ場合、
callee parameter bindingはそのdependencyを受け継ぐ。
function callはdependency laundering boundaryではない。

## 18.3 parameter lifetime

parameter は callee local binding である。

non-Discardable non-Copy parameter は、すべての normal function exit path で Available のまま残ってはならない。

## 18.4 no implicit borrow

ordinary function boundary は暗黙 borrow を行わない。

parameter が `ref<read,T>` を要求するなら caller はその ref / loaned capability を明示的に用意する。

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

early exit が必要な場合は:

```text
return expr
```

を control-flow terminator として使用できる。

`return expr` の `expr` にも通常の value-use 規則を適用する。
`return` 自体は normal block result を持たない。

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
- source empty storage claimはcompiler-managed localなら内部処理してよい

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

normal completion 時、末尾 expression が block result になる。

末尾expressionがhidden dependencyを持つvalueなら、
block resultも同じdependencyを保持する。
blockはdependency laundering boundaryではない。

function body、`if` arm、loop の current iteration body、loan body は lexical block である。

exactly once の lexical evaluation なので、outer non-Copy binding を block 内で consume してよい。
block 後の continuation が存在する場合、その availability state は通常の control-flow join rule に従う。

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
x.foo
```

で `T` に `foo` field があるかを instantiation-time に調べる structural duck typing は v0 に入れない。

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

dependent associated lookupとordinary lexical candidateを同一candidate setに混ぜない。

future module systemでは:

```text
module.f(x)
```

等のqualified ordinary lookupを追加してよい。

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

fresh allocation や object lifetime end 直後など、
まだtyped semantic valueとして解釈されていないraw bytesのrepresentation stateは
**unspecified** でよい。

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

source / destinationが部分overlapしてもこの結果を保証する。

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
Sは一つのlive BackingRegion内にある **0個以上のcurrent live `T` object locations** の
contiguous sequenceへのcurrent access capabilityを表す。

`n > 0` の場合、covered elementsをconceptually:

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

`n == 0` の場合、live `T` objectを一つもcoverしない。
そのためabstract semantics上:

- first-element ptr
- one-past ptr
- null ptr
- dummy T object

を要求しない。

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
span loanは、container/rooted regionが既に保持するlive-object responsibilityとstability authorityからderiveしなければならない。

conceptually:

```text
unchecked loan_span<mode,T>(container-specific evidence...) { |s|
    ...
}
```

のようなprimitiveを想定するが、surface syntaxは§13.8のloan syntaxと合わせてOpenとする。

loan bodyはspan scopeを越えてcapabilityを持ち出せない。

少なくともbody entryからexitまで:

- covered rangeが同一live BackingRegion内にある
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

conceptual syntax:

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

v0 の各variantは:

```text
Variant
Variant(T)
```

のいずれかとする。

一variantが複数fieldを直接持つsyntaxはv0に入れない。必要ならstructをpayloadにする。

variant名はdefining sum type内で一意でなければならない。

exact surface syntaxはProvisionalだがsemanticsは本節で定義する。

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

## 26.3 construction

constructorはnominal value constructionでありordinary function lookupには参加しない。

conceptual syntax:

```text
Option<u32>.None
Option<u32>.Some(u32(42))

Result<u32, Error>.Ok(u32(42))
Result<u32, Error>.Err(error)
```

v0ではpayloadなしvariant等のtype inference問題を避けるため、constructorのsum typeを明示する形を基本とする。

constructor argumentには通常のvalue-use ruleを適用する。

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

```text
let y =
    match x {
        Some(v) => {
            transform(v)
        }

        None => {
            default_value()
        }
    }
```

scrutinee expressionは一度だけ評価する。

current variantに対応するarmだけを選択し、そのarm bodyをexactly once評価する。

各arm bodyはlexical blockであり、通常のblock result / availability / terminator ruleに従う。

fallthroughはない。

## 26.10 v0 pattern

v0 patternは次だけ。

```text
Variant
Variant(binding)
Variant(_)
```

pattern中のvariant名はscrutineeのstatic nominal sum typeに対して解決する。
ordinary function lookup / associated lookupには参加しない。

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

payloadがaggregateならarm body内で既存のwhole-value destructuringを使う。

## 26.11 exhaustiveness

`match` はexhaustiveでなければならない。

closed nominal sumのすべてのvariantについて、各variantをexactly one armでcoverする。

- missing variant: compile error
- duplicate variant arm: compile error
- whole-match wildcard arm: v0では存在しない

## 26.12 consuming match

scrutineeがordinary sum valueなら通常のvalue-use ruleを適用する。

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

scrutineeがordinary refならborrowed matchとする。

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
        }

        Err(e) => {
            return Result<Output, Error>.Err(e)
        }
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
        }

        None => {
            return Option<Output>.None
        }
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

v0 では以下を持たない。

- initializer のない ordinary local
- ordinary binding への再代入
- ordinary binding の再初期化
- `var` / `mut` のような mutable-binding mode

binding immutability は object immutability を意味しない。

memory mutation は `ref<write>` / Storage / container operation 等で行える。

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
if cond {
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

概念例:

```text
let y =
    if cond {
        i32(3)
    } else {
        i32(5)
    }
```

condition は `bool`。

normal completion する arms の result type は同一でなければならない。
v0 はこの join のための implicit numeric conversion / overload ranking を行わない。

arm は lexical block である。

一方の arm が `return`, `break`, `continue` 等で terminates する場合、その arm は current normal result type join に参加しない。

## 27.5 loop expression

loop は loop-carried state を parameter values として明示する value-producing control construct である。

概念例:

```text
let result =
    loop (i = u32(0), acc = u32(0)) {
        if i == n {
            break acc
        } else {
            continue(i + u32(1), acc + i)
        }
    }
```

初期 argument は left-to-right に評価する。

各 iteration で `i`, `acc` は **fresh initialized bindings** であり、前 iteration の binding を再代入しているわけではない。

loop parameter valueがhidden dependencyを持つ場合、
そのdependencyもiterationごとのfresh parameter bindingへblock argumentとして流れる。

loop body は current iteration の exactly-once lexical block である。

## 27.6 continue

```text
continue(expr1, expr2, ...)
```

は current iteration を終了し、各 expression の値を次 iteration の fresh loop-parameter bindings へ渡す terminator である。

- arity は loop parameter count と一致
- corresponding type は一致
- arguments は left-to-right evaluation
- value-use は Copy/consume の通常規則に従う
- argument valueのhidden dependencyもnext iteration parameterへ伝播する

`continue` は normal block result を持たない。

current iteration-local scopeが`continue` edgeで終了するため、
next iteration / outer memoryへsurviveするsemantic value/current-value stateは
そのending iteration-local scopeに依存していてはならない。

次 iteration にまたいで変化させたい non-Copy state は、原則として loop parameter に含める。

outer non-Copy binding の availability を iteration 間で path-dependent に変化させない。

loop entry から capture した non-Copy binding について、
各 reachable `continue` edge の availability state は loop entry state と一致しなければならない。

iteration 間で変化させたい non-Copy state は loop parameter/value として明示する。

## 27.7 break

```text
break expr
```

は loop を終了し、`expr` を loop expression result とする terminator である。

すべての normal `break` result は同一 type でなければならない。

loop exit で outer non-Copy binding の availability stateを合流する場合、
すべての break edge でその state が一致しなければならない。

`break` は current iteration block の normal result を持たない。

loop / current iteration-local scopeが`break` edgeで終了する場合、
break resultやouter memoryへsurviveするstateはそのending scopeに依存していてはならない。

## 27.8 loop fall-through

v0 prototype では loop body の normal fall-through を許可しない。

各 reachable iteration path は最終的に:

- `continue(...)`
- `break expr`
- `return expr`
- その他 current control construct を終了する明示的 terminator

のいずれかへ到達する。

implicit continue は存在しない。

## 27.9 return

`return expr` は enclosing named function を終了する terminator である。

`expr` は function result type と一致し、通常の value-use rule を適用する。

`return` edge は enclosing lexical block の normal result join に参加しない。

function-local scopesがreturn edgeで終了するため、
function result / caller-visible memory等、return後へsurviveするstateはending function-local scopeに依存していてはならない。

## 27.10 ordinary partial move

ordinary aggregate binding の non-Copy partial move は許可しない。

whole-value destructuring / reconstruction を使う。

これにより ordinary non-Copy binding の state space を原則として `Available / Consumed` に限定する。

## 27.11 implementation model

この source semantics は SSA / block-argument style IR に直接 lowering できる。

概念的には:

```text
loop_header(i, acc):
    if done(i):
        branch exit(acc)
    else:
        branch loop_header(i + 1, acc + i)

exit(result):
    ...
```

のように、`continue(values...)` は successor block arguments への branch として表せる。

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

duplicate-name ruleの詳細は各declaration categoryで定義するが、
physical fileが異なること自体はname collisionを解消しない。

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
7. sum declaration / constructor の最終surface spelling
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
