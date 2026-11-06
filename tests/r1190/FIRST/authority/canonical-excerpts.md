# Canonical excerpts used by R1190 FIRST

Source: https://github.com/wakairo/NewLang_Compiler/blob/63658cd546be8b99599baf19476486a475adb320/docs/reference/NewLang_v0_spec_Draft17_34.md

Git blob SHA: `84236f4bc29ae35f01f8eca4238d56c2f62d34a0`. Original line numbers below are navigation coordinates for the pinned file. These excerpts do not replace canonical authority.

## Copy and scalar properties — original lines 3138–3147

## 4.2 Copy

Copy な型の binding を value-use すると、
元 binding を Available のまま保持して value を複製する。

non-Copy な型の binding を value-useすると、
元 binding を consume して destination へ ownership / value responsibility を transfer する。

source-level explicit `move` keyword は v0 に存在しない。


## core properties — original lines 3178–3211

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


## value use — original lines 3309–3335

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


## quantity arithmetic — original lines 3471–3504

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


## explicit conversion — original lines 3570–3620

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


## typed literals — original lines 3621–3642

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


## dependency packages — original lines 4455–4529

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

## loan body source — original lines 5709–5761

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

## aggregate construction — original lines 6463–6519

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

## direct field Copy and Change — original lines 7078–7140

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

## two-u8 nominal source — original lines 7265–7293

### Draft 17.33 bounded name-independent two-u8 nominal direct-field source（候補・§17.23 Pair部分の限定更新）

前節のDraft17.23ではregistered `Pair/left/right` と独立のcompleted recursive H committed-linkがactual-source対象だった。本revisionは**Pair側の名前条件だけ**を次のclosed categoryへ置換する。Draft17.23のH link（payloadは不許可）、Draft17.25の`ref<read/write,H>@link`、Array4の特別sourceはそのまま維持する。ここで示す規則は`CURRENT_SPEC.md`が本Draft17.33を選択するbranchでのみ有効であり、以前のDraft17.32の暗黙の拡張ではない。

```text
bounded_two_u8_nominal_decl(T) :=
    complete nonrecursive ordinary nominal T with exactly 2
    distinct declared field names, each of exact core type u8,
    and derived Copy and Discardable properties

bounded_two_u8_field := local_name '@' declared_field_name

bounded_two_u8_copy_read := bounded_two_u8_field

bounded_two_u8_write_loan :=
    'loan_write' '(' bounded_two_u8_field ')' loan_body
```

- `local_name`はsame semantic unit内で静的型がexactly Tである**current live / Available / in-scope direct lexical local**。元の一つのindependent lexical rootに既存のimplicit governing identityがあり、他のlexical rootや同形foreign nominalをbaseにしてはならない。nominalの**completionと非再帰性**はfield labelを調べる前に確定する。incomplete header、recursive Hの別field、Array、generic、nonCopy/nonDiscardable、3個以上/1個だけ/非u8のfieldをこのcategoryに追加しない。
- `declared_field_name`は当該T自身の**二つの互いに異なる宣言済みfield label**のexact match。field spellingが`left/right`である必要はない。field identityは**(exact nominal T, fieldの宣言index 0または1)**であり、`struct Route {channel:u8,marker:u8}`では`route@channel`がindex 0、`route@marker`がindex 1。named aggregate initializerのsource順が宣言順と異なっても§16.2どおりfield名で意味論的に対応付ける。別nominal `Other`が同じ二field layout/名前/整数値を持っても、Tへstructural cast、同一original rootとしての扱い、field provenanceの輸入をしてはならない。
- 既存のstatically resolved opaque projection、nominal/type、base binding、元parent PlaceId/incarnationと従属child PlaceId/incarnation、現在のroot/child ValueFactと型を、**元sourceから機械的に発行**する。fieldは独立したroot、別Allocation、別Domain/BackingRegion claimを生成しない。Copyで別のlexical record valueを受けた場合、その受け手の本物のplacementに**別の**root/child/incarnationsを形成するのであり、元root権限をCopyして再利用しない。source field spellingやC offset、値の一致だけをauthorityとしない。
- `local@field`のreadは、そのcurrent childのexact `u8` value packageの**ordinary Copy**である。enclosing rootは消費しない。value/occurrence dependenciesを消去せず、現在性／loan conflictが証明不能なら既存§13.5a・§17ルールに従って拒否/precision制限する。
- `loan_write(local@field){|w|...}`はそのchildに限る**fresh ordinary nonexclusive `ref<write,u8>`**をexactly-once lexical scope内で提供する。readからのwrite格上げ、exclusive/noalias/lifetime-end、ref escape、呼出者へ未証明の隠れた事前条件転嫁は禁止。終了までoriginal root/fieldのcurrentness、incarnation、valid access/scopeを維持する。通常の`replace(w,new_u8)`は現在の旧Copyを返し、`Change(child)`によってchildとancestorのcurrent factsを更新し、元root・二fieldのincarnationsとknown-disjoint sibling value/current factは維持する。後続readは新しいchild factに基づく。`swap`や任意のfield partial moveはこの追加source選択ではない。
- このsource categoryは**宣言を登録した同一semantic unitのordinary registered-function bodyと、そこから適法に作るsource fragmentの双方**で一貫して有効とする。function-unitだけの特別なPair名whitelistやfragmentだけの隠れた別規則を設けない。checkerの保持するroot/child/index/ValueFact/Change事実と、後続owned検査やC17 emitterの適格性は別のgateであり、**後者が検証できないsource形はC17前にfail closed**とする。語義上はindex 0/1の両fieldへwrite可能だが、P #671単独の有限native実測はindex 0のwrite一回・後続両field Copy readだけだった。独立R #675は別sourceのindex 1 writeをstrict C17/nativeで確認したが、複数nominal/rootsの一般C17実行や広いbackend証明は成立していない。

**限定sourceの拒否義務（本節は実テストのPASS宣言ではない）:** unknown/duplicate field label、initializerの欠落・重複・wrong type、same-layout foreign nominalの混同、declared order/indexのすり替え、ended root/childや旧incarnation、stale/foreign ValueFact、parent-child/root付替え、間違ったsibling currentness、scoped refのescape、read→write promotion、scope外・unknown dependency、resource/OOM partial issuanceを受理しない。別のsource category（Node H、ptr/ref、sum、receiver）へ黙ってfallbackしない。誤ったroot/child証拠があるならC17へ進めず、authentic sourceを起点にした負例/rollbackと必要な独立検証の対象とする。

**§28.6とValue Strategy §1.1の保留境界:** v0は依然**one visibility domain**。このnominal fieldを安全に書けることは、future libraryのhidden invariantを合法writerが壊せない証明ではない。将来のdefining-module-private field projectionではnamed construction/destructuringを含めた同一representation visibilityを強制する方向を**KEEP**し、public/transparent opt-inの別裁定を可能にする。一般ordinary-safe APIとその全safe-reachable states（正常/失敗/teardown/reentrancy/FFI）におけるUB防止監査局所性は**未実証**。public read/writeを自動的なrepresentation privacyやProduct GOとして扱わない。


## replace transition — original lines 7337–7414

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

## ordinary function declaration and fresh formal — original lines 7711–7838

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


## argument and result — original lines 8814–8898

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


## function exit — original lines 8976–9001

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


## block source sequencing — original lines 9002–9069

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

## ordinary binding — original lines 11692–11734

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


## IF — original lines 11827–11950

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


## loop initializer/header/first-later — original lines 11951–12166

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


## Continue — original lines 12167–12238

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


## Break — original lines 12239–12293

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


## loop termination — original lines 12294–12346

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


