# Draft 17.14 review resolution

Track: M

Compiler Issue #75 / M9.9 の targeted adjudication。
R6-01で露呈した contextual source word と ordinary lexical name のdisambiguationだけを閉じる。

Draft 17.14 はcandidateであり、mainへmergeされるまではcanonicalではない。

## Authority

開始時:
- Compiler main: `d7d4238fb2f50140c96b875e954b72051e6110d3`
- `CURRENT_SPEC.md` -> Draft 17.13
- R6 first-pass #74: FINDINGS
- accepted finding: R6-01
- F2: not triggered

## R6-01 source-level reproduction

```text
fn match() -> unit { unit }
fn caller() -> unit { match() }
```

Draft 17.13:
- `function_name` はordinary lexical source name
- exact `unit` だけがordinary lexical nameとしてinadmissible
- `match` はclosed match expression introducer

しかし、`match` spellingをordinary nameとして導入した時、
同じexpression positionで ordinary call と match introducer のどちらを選ぶかが未定義だった。

従ってR6-01は SOURCE-SURFACE-GAP として成立する。

## Current contextual inventory

M9.9で実際にordinary name/expressionと同一positionで競合するcurrent closed spellings:

- `fn`
- `let`
- `return`
- `match`

`unit` は別category:
- contextual introducerではない
- distinguished core spelling
- ordinary lexical namespaceへ導入不可

他にparserがwordとして特別扱いする `ptr/ref/exclusive/read/write/using` 等は、
current closed profileではtype positionまたは専用loan-entryに局所化され、
ordinary expression/nameと同一positionで競合しないためM9.9のreservation/disambiguation対象へ広げない。

## Alternatives

### A — current introducersをordinary lexical namesとしてreserve

例:
- `fn`, `let`, `return`, `match` をfunction/local/parameter nameとしてreject

Rejected.

理由:
- Draft 17.12はexact `unit`だけをtargeted ordinary-name reservationとして明示した
- R6-01以外のspellingまでordinary namespaceから除去する必要がない
- productionの既存 `fn()` ordinary-call preservation evidenceとも逆方向
- member labels等まで波及させる必要もない
- parserを簡単にするためだけのsource restrictionになる

### B — contextual disambiguation

**Selected.**

Rule:
- spelling単独ではcontextual constructへcommitしない
- current syntactic positionでclosed constructのdistinguishing shapeが成立した時だけcontextual interpretationへcommit
- それ以前はordinary word/name/call pathを失わない
- commit後のmalformed syntaxはordinary-name parseへfallbackしない

### C — escape / qualification

Rejected as unnecessary.

R6-01を閉じるためにraw identifier、escape prefix、qualification等を導入する必要はない。
将来modules/qualificationが導入されても、このtaskのpreconditionではない。

## Exact current disambiguation

### fn

Declaration distinguishing prefix:

```text
top-level: fn ordinary_name (
```

- top-levelで成立 -> function declarationへcommit
- lexical block内で成立 -> nested/local declaration shapeとしてreject
- `fn(...)` -> ordinary call/name path

従って:

```text
fn fn() -> unit { unit }
fn caller() -> unit { fn() }
```

はsource-levelで一意。

### let

Bindingへcommitするのはclosed receiver shapeがmandatory `=` まで成立した時だけ。

Examples:

```text
let x = expr
let (a, b) = expr
let Pair { a, b } = expr
```

対して:

```text
let()
let(a, b)
```

は `=` を伴うreceiver shapeでなければordinary call/name path。

### return

return-enabled block-item positionで:

```text
return expression;
```

のcomplete closed shapeが成立した時だけterminating return item。

- `return(...)` -> ordinary call/name path
- `return;` -> terminating bare returnではない
- ordinary name `return` がresolveするなら `return;` はordinary expression statementになり得る
- `return x;` はordinary call grammarでは表現できないのでterminating returnとして一意

### match

```text
match expression {
```

まで成立した時だけmatch expressionへcommit。

- `match(...)` -> ordinary call/name path
- bare/tail `match` -> ordinary name path when it resolves
- `match match() { ... }`:
  first `match` is contextual match; second `match()` is ordinary call

## W1 — R6-01

```text
fn match() -> unit { unit }
fn caller() -> unit { match() }
```

**PASS by rule.**

Declaration name `match` is admissible.
`match()` does not form `match expression {`; it is ordinary call.

## W2 — fn as ordinary function name

```text
fn fn() -> unit { unit }
fn caller() -> unit { fn() }
```

**PASS.**

First `fn` is declaration introducer because `fn ordinary_name (` shape is present.
Second name `fn` is ordinary function name.
Call `fn()` remains ordinary call.

## W3 — let as ordinary function name

```text
fn let() -> unit { unit }
fn caller() -> unit { let() }
```

**PASS.**

`let()` does not form a closed binding receiver followed by `=`.

Contrast:

```text
let x = value;
```

does and therefore commits to binding.

## W4 — return as ordinary function name

```text
fn return() -> unit { unit }
fn caller() -> unit { return() }
```

**PASS.**

`return()` is ordinary call.
`return unit;` is terminating return.
`return;` is not a bare-return terminator.

## W5 — nested expression contexts

For each ordinary call `fn()`, `let()`, `return()`, `match()`:

- expression statement: valid ordinary-call parse where semantics allow
- block tail: ordinary-call parse
- argument to another call: ordinary-call parse
- match scrutinee: e.g.
  ```text
  match match() {
      ...
  }
  ```
  outer first `match` contextual, scrutinee `match()` ordinary call

No spelling-only interception is allowed.

## W6 — ordinary bindings

Selected policy applies to ordinary lexical names, not only function names.

Examples:

```text
let match = value;
match

let let = value;
let

let return = value;
return

let fn = value;
fn
```

are not rejected merely because of spelling.
Whether each use type-checks depends on ordinary semantic context, but source parsing must keep the ordinary name path reachable.

At block-item start:
- first `let` in `let let = ...` commits because receiver+`=` shape exists
- receiver name `let` is ordinary lexical name
- later bare/tail `let` has no binding shape and remains ordinary expression/name

## W7 — member labels

No reservation is added for field/variant/member labels.

Examples such as:
- `Flag.match`
- aggregate field named `let`
- member/variant label `return`
remain governed by their existing member namespace/source rules.

Exact `unit` keeps its prior special rule:
ordinary lexical introduction is forbidden, while member labels are not globally forbidden merely by spelling.

## Why this is a small reusable rule

The fix is broader than a one-off `match` parser patch but smaller than a keyword system.

It establishes one reusable principle for **current contextual source words**:

> contextual syntax wins only after its closed distinguishing shape is established; spelling alone does not reserve the ordinary lexical name.

This separates:
1. lexer token class
2. ordinary-name admissibility
3. source disambiguation
4. semantic lookup

No layer is used as a substitute for another.

## Commit / fallback rule

Once a distinguishing shape is established, parsing commits to that contextual construct.

Examples:
- `let x =` with missing RHS is an invalid binding, not fallback to ordinary `let`
- `match x {` with malformed arms is an invalid match, not fallback to ordinary `match`
- top-level `fn f(` with malformed signature remains an invalid declaration

This avoids error-recovery changing program meaning.

## Future pressure

### parenthesized/grouping expressions

If grouping is added later, already-valid ordinary calls:

```text
fn(...)
let(...)
return(...)
match(...)
```

must not be retroactively reinterpreted merely because `(...)` could become a grouped expression.

Future grouping syntax must preserve these existing ordinary-call meanings or add a new unambiguous rule.

### generic call/declaration

M9.9 does not define generic punctuation.

Future generic call/declaration syntax must preserve:
- current ordinary name admissibility
- current contextual commitment rule
- existing valid call spellings

### modules / qualification

Future qualification can add e.g. `module.match(...)`.
Contextual recognition of an unqualified introducer spelling does not imply reservation of member/qualified names.

### callable blocks

Callable syntax remains outside this closure.
If callable bodies later reuse lexical block items, `let` / `return` contextual handling must respect the callable-specific return/leave rules then in force.
M9.9 does not decide those semantics.

## Targeted external comparison

Not performed.

Local canonical constraints and the existing `fn()` production evidence already distinguish A/B/C sufficiently.
Deep Research / broader keyword survey would not materially affect the minimal decision.

## F2

**NOT TRIGGERED.**

This is source grammar/name disambiguation only.
No ownership/lifetime/dependency semantic invariant changes.

## Exact normative sections affected

Candidate Draft 17.14 changes only:

1. Draft 17.14 summary
2. §18.1 — contextual `fn` declaration vs ordinary `fn(...)`
3. §19.1 — contextual `let` / `return` block-item disambiguation
4. §21.8 — central reusable contextual source-word rule
5. §26 match source section — contextual `match` vs ordinary `match(...)`
6. §27.1 — `let` binding commitment rule

§4.9 `unit` semantics are deliberately unchanged.

## Bounded production follow-up

After candidate review/merge only, separate P fix should:

- implement one shared contextual-shape policy rather than one-off `match` patch
- preserve `fn()` current ordinary-call behavior
- make `match()`, `let()`, `return()` reach ordinary call parsing
- recognize bindings only after closed receiver+`=` shape
- recognize return item only for exact `return expression;`
- preserve malformed-contextual-construct commit/no-fallback behavior
- support ordinary locals/parameters/functions named `fn` / `let` / `return` / `match`
- keep exact `unit` ordinary-name rejection
- avoid over-reserving member labels
- add nested statement/tail/argument/match-scrutinee tests
- preserve P11 whole-unit declaration behavior, transactionality and all prior regressions

M9.9 does not start that P work.

## R6-01 targeted revalidation condition

After canonical merge + bounded production fix, targeted R6-01 revalidation should require:

1. original `fn match ...; match()` witness passes ordinary-call parsing/name resolution
2. `fn fn()`, `fn let()`, `fn return()`, `fn match()` declarations and corresponding calls are consistently reachable
3. real `fn` declaration / `let` binding / `return expression;` / `match expression {` still parse as contextual constructs
4. statement/tail/argument/nested match-scrutinee ordinary calls are not intercepted
5. ordinary bindings/parameters with contextual spellings remain reachable
6. `unit` remains inadmissible as ordinary lexical name and builtin semantics remain unchanged
7. member labels are not accidentally reserved
8. malformed contextual constructs do not fallback into a different valid ordinary-name parse

Only then should R6-01 be considered CLOSED.

## Disposition

Candidate Draft 17.14 is ready for Coordination review with an open, unmerged PR.
