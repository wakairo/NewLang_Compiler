# Draft 17.14 review resolution

Track: M

Compiler Issue #75 / M9.9 の targeted adjudication。
R6-01で露呈した structural source word と ordinary lexical name のdisambiguation / name-admissibilityだけを閉じる。

Draft 17.14 はcandidateであり、mainへmergeされるまではcanonicalではない。

## Authority / review continuation

開始authority:

- Compiler main: `d7d4238fb2f50140c96b875e954b72051e6110d3`
- main `CURRENT_SPEC.md` -> Draft 17.13
- R6 first-pass #74: FINDINGS
- accepted finding: R6-01

初回M9.9 candidateはB案（contextual disambiguation）を選択し、
PR #76 head `b1a617a6f8b2dd776c1aa80b45debe88dee742ae` で READY FOR REVIEW まで到達した。

その後 Coordination review:

- Issue #75 comment 6014862004
- PR #76 comment 6014861450

で **CHANGES REQUESTED**。

reviewはB案自体のinternal coherenceではなく、
small structural ordinary-name reservation A'との比較が不十分であることを指摘した。

M9.9 continuationではBを既決事項とせず、A'とBを再裁定した。

## R6-01 source-level reproduction

```text
fn match() -> unit { unit }

fn caller() -> unit {
    match()
}
```

Draft 17.13では:

- `function_name` はordinary lexical source name
- exact `unit`だけが明示的にordinary lexical nameとしてinadmissible
- `match` はclosed match expression introducer

だったため、ordinary function `match` と structural `match` のsource interpretationが衝突した。

R6-01は **SOURCE-SURFACE-GAP / production correctness mismatch** として有効。

## Current source-word inventory

M9.9で対象とするcurrent closed structural set:

```text
fn
let
return
match
```

共通点:

- source grammarの骨格を作る
- declaration / binding / control / expressionのprimary introducer
- ordinary lexical nameとして許すとcurrent grammarで直接衝突する

`unit` は別category:

- structural introducerではない
- core singleton type/valueを直接表すdistinguished core spelling
- ordinary lexical namespaceへ導入不可

current frontendには `ptr` / `ref` / `read` / `write` / `exclusive` / `using` 等の
type/modifier-like wordも存在するが、これらは狭いdedicated grammatical positionに置かれ、
current ordinary expression/name grammarのprimary introducerと同じ種類の衝突を作っていない。

従ってM9.9は「wordとしてspecialなら全部予約」というruleにはしない。

## Alternatives reconsidered

### A' — small structural setをordinary lexical namespaceからreserve

Exact proposal:

```text
OrdinaryStructuralReserved = {
    fn,
    let,
    return,
    match
}
```

Rules:

- ordinary lexical declaration/binding nameとして上記exact spellingを導入できない
- lexerはgeneric word tokenのままでよい
- dedicated keyword token kindは要求しない
- field / variant / member label等の別namespaceへ自動的にreservationを拡張しない
- member spellingをfresh ordinary localへ受ける場合はordinary-name ruleが適用される
- future structural wordsはfeature導入時に個別裁定する

### B — contextual disambiguation

Initial candidate rule:

- `fn` / `let` / `return` / `match` をordinary lexical nameとして許す
- closed distinguishing shapeが成立したらstructural interpretationへcommit
- それ以前はordinary call/name pathを保持
- commit後はfallbackしない

Bは表現力としてはcoherentだが、ordinary nameとして4 spellingを維持するために
parser/spec/future grammarへpermanent disambiguation contractを導入する。

### C — escape / raw identifier / qualification

現時点では不要。

A'でordinary nameを4 spellingだけ失うcostは小さく、
それを回復するためのescape syntaxを今導入する具体的需要はない。

## A' vs B — explicit adjudication

### 1. Human source readability / surprise

**A' wins.**

Bで許される:

```text
fn fn() -> unit { unit }
fn match() -> unit { unit }

fn caller() -> unit {
    let let = match();
    return()
}
```

のようなsourceは、grammarを知っていても構造語とuser nameの視覚的衝突が大きい。

特に `fn fn` / `let let` / `return()` / `match()` は、
readerがtoken sequenceを見た瞬間にstructural roleを判断できず、
周囲shapeを追ってから再解釈する必要がある。

NewLangはC後継としてcompactであることを重視するが、
keyword count自体の最小化は目的ではない。
4語程度のstructural reservationは認知負荷を減らす方向に働く。

A'では:
- `fn` = function declaration introducer
- `let` = binding introducer
- `return` = function return introducer
- `match` = closed-sum match introducer

というsource readingが安定する。

### 2. Parser / grammar complexity

**A' wins materially.**

Bはordinary lexical name routeを維持するために:

- shape recognition
- lookaheadまたは同値のdelayed commitment
- commit point
- fallback prohibition
- nested contextごとのdisambiguation

をnormative conceptとして持つ。

A'ではordinary lexical name candidateから4 spellingを除外するだけでよい。

lexerはword tokenのままでよく、
parserはstructural positionでspellingを認識し、
ordinary name introducerではcentral name-admissibility checkを行えばよい。

つまりA'のcomplexityは主に**一つのsmall set membership check**であり、
Bのgrammar-level ambiguity managementより小さい。

### 3. Diagnostics / error recovery

**A' wins materially.**

A'なら:

```text
fn match() -> unit { ... }
let return = x;
```

に対し早い段階で:

> reserved structural source spelling cannot be an ordinary lexical name

相当のdiagnosticを出せる。

また:

```text
match(...)
return(...)
let(...)
```

を「ordinary callかstructural constructか」推測してrecoverする必要がない。

Bではmalformed contextual syntaxがordinary name routeへ見えるケースを防ぐため、
commit/no-fallback semantics自体を仕様にする必要がある。

A'ではalternative ordinary interpretationが存在しないので、
error recoveryがprogram meaningを変えるriskが小さい。

### 4. Future grammar compatibility

**A' wins materially.**

Bは初回candidateで:

- grouping expression
- generic call/declaration
- qualification
- callable syntax

を将来追加するとき、
既存 `fn(...)` / `let(...)` / `return(...)` / `match(...)`
ordinary call meaningを保つcompatibility burdenを作った。

A'はこれら4 spellingをordinary lexical namespaceから外すため、
future grammarはstructural useを拡張してもordinary free-name interpretationと衝突しにくい。

これはfuture syntaxを今設計することではなく、
**現在不要なcompatibility debtを作らない**判断である。

### 5. Value gained by preserving the four ordinary names

**Bのcompensating benefitは小さい。**

ordinary free function/local/parameterとして:

- `fn`
- `let`
- `return`
- `match`

をexact spellingで使う実用上の強い需要は見つからない。

通常はより説明的な:

- `matches`
- `match_value`
- `result`
- `binding`
- `function`

等で置換できる。

generated codeもこれら4 spellingを避けるのは容易。

将来FFI等でexternal symbol spellingをexactに保持する必要が生じても、
source-level ordinary nameとexternal linkage nameを同一にする必要はない。
FFIは本task外であり、将来actual pressureが出ればlink-name mechanism等を別途検討できる。

一方、member labelとして `match` 等を使いたい需要はfree lexical nameより現実的なので、
A'はmember/variant/field namespaceを自動予約しない。

従ってBのidentifier-space benefitはA'のcomplexity reductionを上回らない。

### 6. Existing contextual modifier/type words

**A'は「special wordは全部hard reserve」というruleではない。**

current type/loan grammarの:

- `ptr`
- `ref`
- `read`
- `write`
- `exclusive`
- `using`

等は狭いgrammatical positionで意味を持つ。

これらをordinary lexical namespaceから予約する必要はM9.9では示されていない。

この区別は:

> broad structural grammar anchorsはsmall reserved setにする。
> narrow modifier/type wordsは必要なcontextだけでspecialに扱える。

という形で安定している。

### 7. `unit` relationship

`unit` はA' structural setとは**別caseのまま**。

共通点:
- ordinary lexical namespaceへ導入できない
- lexer上generic word tokenでもよい
- member labelsを自動的にglobal reserveしない

相違:
- `fn` / `let` / `return` / `match`:
  source grammarのstructural anchorsだからreserve
- `unit`:
  core singleton type/valueを直接表すdistinguished semantic spellingだからreserve

この区別をcanonical textへ明示する。

## Decision

**A' SELECTED.**

初回B案はsuperseded。

M9.9の最小ruleは:

> current structural source set `fn` / `let` / `return` / `match`
> をordinary lexical namespaceから予約する。
> ただしlexer-wide keyword token化やfield/variant/member namespace reservationは要求しない。

これによりR6-01は「ordinary function `match`をcall可能にする」のではなく、
**そもそもordinary function `match`をsourceで宣言できない**ことで閉じる。

## Required witnesses under A'

### W1 — R6-01

```text
fn match() -> unit { unit }
fn caller() -> unit { match() }
```

**REJECT.**

最初のdeclarationで `function_name = match` がordinary lexical name admissibility error。

production/parserはordinary function `match`をpublishしてはならない。

### W2 — `fn` as ordinary function name

```text
fn fn() -> unit { unit }
```

**REJECT** at function-name admissibility.

normal function declaration:

```text
fn ping() -> unit { unit }
```

は従来どおりvalid。

### W3 — `let` as ordinary function name

```text
fn let() -> unit { unit }
```

**REJECT** at function-name admissibility.

actual binding:

```text
let x = value;
```

は従来どおりstructural binding source。

### W4 — `return` as ordinary function name

```text
fn return() -> unit { unit }
```

**REJECT** at function-name admissibility.

actual:

```text
return unit;
```

は従来どおりterminating return item。

### W5 — nested expression contexts

ordinary free calls:

```text
fn()
let()
return()
match()
```

をcallee ordinary lexical nameとして成立させるsource routeはない。

従ってstatement/tail/argument/match-scrutineeのどのcontextでも、
これらをordinary free-call spellingとして温存するrequirementはない。

structural grammarに合わなければsource syntax/name-admissibility errorでよい。

### W6 — ordinary bindings / parameters

次は全てordinary lexical name admissibility error:

```text
let fn = value;
let let = value;
let return = value;
let match = value;
```

同様に:

- function parameter
- multi-result receiver
- aggregate destructuring fresh local
- match payload fresh binding
- future callable/loop parameterがordinary lexical bindingならそのbinding name

へ同じruleを適用する。

### W7 — member labels

A'はmember/nominal namespaceへ自動拡張しない。

従ってfield / variant / member label spellingとして:

```text
match
let
return
fn
```

を使うこと自体はM9.9だけでは禁止しない。

ただしshorthand destructuring等でそのmember spellingから同名ordinary localを作ろうとすれば、
fresh local側でrejectする。

例:

```text
let Type { match } = value
```

はfield label `match` が悪いのではなく、
ordinary local `match` を導入しようとするためreject。

## Human/API cost audit

A'で失うのはordinary lexical namespaceの4 exact spellingsだけ。

これは:
- language feature countを増やさない
- runtime/ABIを変えない
- ownership semanticsを変えない
- member labelsを失わない
- external symbol spellingを永久に禁止する判断でもない

一方で:
- source reading
- grammar
- parser
- diagnostics
- recovery
- future syntax evolution

の複数層を同時に単純化する。

NewLangの「small language」は「reserved wordを極限まで減らす」ことではなく、
不要なmechanism/ambiguityを持ち込まないことを優先すべきと裁定する。

## External sanity check

Light non-binding comparisonを実施した。

### C

Cはstructural/control wordsをreserved keywordとして持ち、`return` もreservedで再定義できない。

Reference:
- https://en.cppreference.com/c/keyword
- https://en.cppreference.com/c/keyword/return

### Rust

Rust Referenceはkeywordsをstrict / reserved / weakへ分ける。

`fn` / `let` / `match` / `return` はstrict keywords。
一方で `union` / `raw` / `safe` 等はweak/context-specific keywordとして扱う。

Reference:
- https://doc.rust-lang.org/reference/keywords.html

### Kotlin

Kotlinもhard keywordとsoft keywordを分離する。
`fun` / `return` / `when` / `val` / `var` 等のstructural/control wordsはhard keyword。
`by` / `constructor` / `field` / `where` 等はcontext-specific soft keyword。

Reference:
- https://kotlinlang.org/docs/keyword-reference.html

### Relevance to NewLang

majority voteではない。

有用なsanity checkは:

> fundamental structural/control introducerをreservedにしつつ、
> narrower modifier/context wordはcontextualに残す設計は十分一般的で、
> 「modern languageならfundamental introducerもcontextualにするべき」という圧力はない。

A'はこの分離をNewLangのcurrent closed profileへ最小限適用する。

## F2

**NOT TRIGGERED.**

A'はsource-name admissibility / grammar policyであり、
ownership / lifetime / dependency / function-call semanticsを変更しない。

## Exact normative sections affected

Revised candidate Draft 17.14 changes:

1. Draft 17.14 summary
2. §4.9 — `unit` とstructural reserved setのcategory separation
3. §16.1 — destructuring fresh ordinary localへstructural-name restriction
4. §18.1 — function/parameter ordinary-name admissibility
5. §19.1 — `let` / `return` are structural words, no ordinary-call fallback
6. §21.8 — central current structural reserved set
7. §26.9 — `match` structural source word
8. §26.10 — payload fresh ordinary binding restriction
9. §27.1 — local binding name restriction
10. §27.1a — multi-result receiver restriction

No change to:
- whole-unit visibility
- recursion/SCC
- generic declaration syntax
- associated-function source syntax
- modules
- ownership/lifetime/dependency semantics

## Bounded production follow-up

After candidate review/merge only, separate P fix should:

1. add one central ordinary lexical name-admissibility check for:
   - `fn`
   - `let`
   - `return`
   - `match`
   - plus existing separate `unit` rule
2. apply it transactionally to:
   - ordinary function names
   - parameters
   - ordinary locals
   - multi-result receivers
   - aggregate destructuring fresh locals
   - match payload fresh bindings
3. do **not** require lexer token-kind redesign
4. do **not** reject field/variant/member labels merely by spelling
5. remove the requirement that `fn()` / `let()` / `return()` / `match()` remain ordinary free-call paths
6. keep real `fn` declaration / `let` binding / `return expression;` / `match expression {` behavior
7. provide stable reserved-structural-name diagnostics
8. preserve P11 whole-unit visibility, transactionality, OOM rollback and all existing semantic regressions

M9.9 does not start this P work.

## R6-01 targeted revalidation condition

After canonical merge + bounded production fix, targeted R6-01 revalidation should require:

1. original `fn match()...` witness is rejected because `match` cannot be an ordinary lexical function name
2. `fn fn()`, `fn let()`, `fn return()`, `fn match()` all reject consistently at name admissibility
3. parameters and ordinary locals using the four spellings reject consistently
4. multi-result / aggregate destructuring / match payload fresh ordinary bindings use the same rule
5. real `fn` / `let` / `return` / `match` structural forms remain valid
6. exact `unit` remains separately distinguished and non-shadowable
7. field/variant/member labels are not accidentally over-reserved
8. rejection is independent of source order / body reachability and does not publish partial ordinary names

Only then should R6-01 be considered CLOSED.

## Disposition

Revised Candidate Draft 17.14 selects A' and is ready for Coordination review.
