# Draft 17.13 review resolution

Track: M

この文書は Compiler Issue #70 / M9.8 で行った、
minimal non-generic ordinary-function declaration source profile の targeted adjudication を記録する。

normative authority はこの文書ではなく、
`docs/reference/CURRENT_SPEC.md` が選択する canonical Draft である。
Draft 17.13 はreview前のcandidateであり、mainへmergeされるまではcanonicalではない。

## Scope

Draft 17.13 は Draft 17.12 の ownership / lifetime / dependency / ordinary-call / return / function-exit semantics を変更しない。

閉じるのは一つだけ:

> P8/P9/P10でhost-known / registeredとして与えていた ordinary non-generic function identity/signature/body を、
> exact top-level source declarationから与える最小profile。

generic function declaration、associated-function source registration、modules/separate compilation等は対象外。

## Authority audit

M9.8開始時:

- Compiler main: `bc6b75e5e31e010f2a0f01068bf69c654f38df30`
- `CURRENT_SPEC.md` -> Draft 17.12
- Semantic Sync #68: PASS / CLOSED
- M9.7 #69: ordinary non-generic declaration source closure selected
- FormalProof main: `a3a8c70becb3da6a9c2fc6f91ca578f49b19df32`
- F2: not triggered
- P8/P9/P10 and R4/R5 evidence already available for the underlying function semantics.

No production change is part of M9.8.

## Exact grammar decision

Selected closed grammar:

```text
ordinary_function_decl :=
    'fn' function_name '(' [parameter_list] ')' '->' result_type lexical_block

parameter_list :=
    parameter (',' parameter)*

parameter :=
    parameter_name ':' parameter_type
```

Rules:

- exact introducer is contextual source word `fn`;
- zero parameters are exactly `()`;
- one or more parameters use `name : Type`;
- comma separates parameters;
- trailing comma is **not admitted** in this minimal profile;
- `:` is ordinary punctuation and surrounding token whitespace is insignificant;
- `->` is an adjacent two-character punctuator represented by adjacent `-` then `>` tokens;
- `- >` is not the result arrow;
- result type is always explicit, including `-> unit`;
- body is the existing §19.1 lexical block;
- there is no declaration semicolon after the body;
- generic-looking `fn f<T>(...)` is outside this grammar.

### Why no trailing comma

Trailing comma is convenience, not required to express W1–W7.
Existing NewLang closed forms already deliberately choose trailing-comma policy per production.
Keeping parameter lists at `parameter (',' parameter)*` is the smaller first closure and can be extended compatibly later.

### Why result type is mandatory

P8 host registration always provides one exact result type and §18 already treats no-information result as ordinary `unit`.

Allowing omitted result syntax would add a second signature spelling for no semantic gain.
Therefore:

```text
fn ping() -> unit { ... }
```

is the exact minimal form.

## Placement decision

An ordinary source `fn` declaration is:

> a top-level ordinary declaration in the one semantic compilation unit.

It is not a §19.1 lexical block item.
Nested/local functions are not introduced.

M9.8 does **not** define a general-purpose grammar for every top-level declaration category.
A production frontend may have a bounded entry/container that recognizes repeated top-level ordinary function declarations.
A future broader:

```text
compilation_unit := item*
```

grammar can include this already-closed production without changing function declaration semantics.

Thus a general top-level grammar is frontend/general-source plumbing, not a canonical prerequisite for this profile.

## Name / duplicate decision

`function_name` and `parameter_name` are ordinary lexical source names.

Existing Draft 17.12 name admissibility applies:

- exact spelling `unit` cannot be introduced as a function name;
- exact spelling `unit` cannot be introduced as a parameter binding;
- parameter names within one signature must be pairwise distinct.

No additional general reserved-word table is introduced by M9.8.

The contextual spelling `fn` is fixed only as the declaration introducer.
M9.8 does not opportunistically reopen the broader question of contextual words in every ordinary binding form.

### duplicate ordinary functions

Within one semantic compilation unit:

```text
fn f(x: A) -> A { ... }
fn f(x: B) -> B { ... }
```

is a duplicate ordinary lexical declaration even though the signatures differ.

Reason:

- §28.2 gives one top-level ordinary lexical namespace;
- v0 has no overload ranking;
- signature difference must not silently create an overload set.

If another already-established top-level **ordinary declaration** occupies the same ordinary lexical name, adding the function is also a name collision.
M9.8 does not invent that other category's source grammar.

Associated functions are handled separately below.

## Declaration visibility/order comparison

Issue #70 explicitly required a fresh adjudication rather than treating M9.7's preference as settled.

### A — whole-unit signature collection / order-independent visibility

Characteristics:

- ordinary function names/signatures in one semantic unit are available before body checking;
- later textual declarations can be called earlier;
- self/mutual recursive identities resolve;
- physical-file split does not introduce a visibility order;
- duplicate detection is unit-wide;
- no prototype syntax is needed.

Cost:

- frontend/semantic implementation needs precollection, lazy declaration discovery, or an equivalent mechanism.

### B — declaration-before-use under semantic source order

Potential advantage:

- can fit a simple sequential frontend;
- resembles C/C++ declaration/prototype practice.

Problems in NewLang:

1. §28 permits multiple physical files in one semantic compilation unit.
2. file path/input order is not a semantic identity mechanism and physical file is not a visibility boundary.
3. B therefore needs a new **cross-file semantic source ordering** rule or some explicit ordering construct.
4. without such a rule, host input order can accidentally affect legality.
5. useful forward calls then need separate prototype/forward-declaration syntax, increasing source surface.
6. declaration order would become a new language concept that ordinary body semantics do not otherwise require.

B is therefore not smaller once multi-file one-unit semantics is included.

### C — restricted first profile: one function / no body-to-body calls

Rejected.

This would mostly mirror the current P8 production limit rather than define a coherent ordinary-function language surface.

A declared ordinary function whose name is in the lexical namespace should be usable as an ordinary known direct-call target according to existing §13.5c/§18 semantics.
Suppressing body-to-body calls at the language level creates a source restriction solely for current implementation convenience.

### Decision

**A selected.**

Normative rule:

> for bounded ordinary non-generic functions in the same semantic compilation unit,
> function names and exact signatures are semantically available before function-body ordinary name resolution / definition-time checking.

Implementation need not literally use two passes.

Equivalent strategies include:

- signature precollection;
- declaration indexing;
- lazy declaration discovery;
- another order-independent representation.

Observable visibility must not depend on textual declaration order, physical file path, or host input order.

## Forward-reference decision

Given:

```text
fn first(x: A) -> B {
    second(x)
}

fn second(x: A) -> B {
    ...
}
```

`second` **resolves** in `first`.

No prototype/forward declaration is required.

This decision is specific to the bounded ordinary-function declaration category.
M9.8 does not generalize order-independent lookup to every future top-level declaration category.

## Recursion: name resolution vs analysis

Given:

```text
fn f(x: Token) -> Token {
    f(x)
}
```

the callee name resolves to the same function declaration.

For mutual recursion, both function identities/signatures likewise resolve.

This does **not** claim current production can already analyze recursive body-backed SCCs.

Canonical §13.5c already provides the semantic implementation space:

- known callee summaries;
- transitive call chains;
- recursive SCC fixpoint over a finite monotone abstract domain.

Therefore M9.8 does not invent a source-level recursion prohibition.

A production compiler that has not implemented sufficient recursive summary analysis may conservatively issue an **analysis precision rejection**.
It must not reinterpret the source as “unknown function because declaration appears later” or as a new language-level recursion ban.

## Ordinary vs associated lookup

The M9.8 `fn` production creates an **ordinary lexical function declaration only**.

It does not create:

```text
AssociatedWith(function, nominal)
```

and does not add the function to a nominal associated-function set.

Therefore if:

- ordinary lexical function `compare` exists; and
- some nominal type owns an associated function also spelled `compare`;

the spelling alone does not merge candidate sets.

Existing §21.7–21.8 separation remains:

- non-dependent ordinary lookup -> ordinary lexical candidate;
- generic dependent associated lookup -> owning nominal's associated set.

Associated-function source registration syntax remains unresolved.

## Generic exclusion

This profile has no production after `function_name` for generic parameters.

Therefore:

```text
fn f<T>(x: T) -> T { ... }
```

is outside the Draft 17.13 closed declaration grammar.

M9.8 does not decide:

- generic parameter delimiters;
- generic declaration lookup;
- constraints;
- associated generic registration;
- specialization.

The conceptual generic examples elsewhere in the Draft retain their semantic role; Draft 17.13 only closes the exact **non-generic** declaration surface.

## Existing body semantics unchanged

The declaration layer supplies only:

```text
identity
name
ordered parameter names/types
result type
lexical body
```

The body still obeys existing §18/§19/§27 semantics.

No declaration-specific ownership, lifetime, borrow, dependency, result-transfer, or cleanup rule is added.

## W1 — zero parameters / unit result

```text
fn ping() -> unit {
    return unit;
}
```

Result:

- zero parameters use exact `()`;
- explicit result `-> unit`;
- body uses existing return-enabled ordinary-function lexical context;
- `unit` remains the core singleton.

**PASS.**

## W2 — one concrete parameter

```text
fn id(x: Token) -> Token {
    x
}
```

Result:

- one `name : Type` parameter;
- existing Copy/non-Copy semantics decide whether tail use copies or consumes;
- declaration introduces no new transfer rule.

**PASS.**

## W3 — multiple parameters

Accepted:

```text
fn exchange(a: ref<write,Token>, b: ref<write,Token>) -> unit {
    swap(a, b);
}
```

Rejected:

```text
fn f(a: A, a: B) -> unit { ... }
```

because parameter names are not distinct.

Rejected in this closed profile:

```text
fn f(a: A, b: B,) -> unit { ... }
```

because trailing parameter comma is not admitted.

**PASS.**

## W4 — forward call

```text
fn first(x: A) -> B {
    second(x)
}

fn second(x: A) -> B {
    ...
}
```

`second` resolves independent of textual order.

Whether a current production checker can then execute/validate the body-to-body call is a later compiler coverage/precision question.

**PASS at source/name-resolution level.**

## W5 — self and mutual recursion identity

Self and mutual names resolve under the same unit-wide signature visibility.

Recursive semantic analysis is governed by existing §13.5c.
Current production may precision-reject an SCC it cannot prove.

**PASS; name resolution and analysis precision are separated.**

## W6 — duplicates / collisions

### duplicate ordinary function

Rejected, regardless of signature.

### function named `unit`

Rejected by Draft 17.12 ordinary-name admissibility.

### parameter named `unit`

Rejected by the same rule.

### same spelling in ordinary and associated sets

Not a duplicate solely because associated candidates are not in the ordinary lexical candidate set.

### collision with another established top-level ordinary declaration

Rejected as an ordinary lexical name collision where that other declaration already exists semantically.
No source grammar for the other category is added.

**PASS.**

## W7 — physical-file split

Suppose `first` and `second` live in different physical files but both files are inputs to the same v0 semantic compilation unit.

Their bounded ordinary function signatures participate in the same unit-wide ordinary function visibility and duplicate check.

Changing host file input order does not change whether `first` can resolve `second`.

**PASS.**

## Targeted external comparison

A light comparison was performed only for declaration visibility/order.

### C / C++

C-family declaration/prototype practice makes a declaration available for subsequent use and uses prototypes/forward declarations to permit calls before a definition.
This keeps sequential parsing straightforward but introduces a separate declaration surface and declaration-order concerns.

Reference:
- cppreference, C function declarations: https://en.cppreference.com/c/language/function_declaration
- cppreference, C function definitions: https://en.cppreference.com/c/language/function_definition

### Rust

Rust item names are visible across the item scope and item name resolution permits references before or after the textual definition.
Items may be defined in any order.

Reference:
- Rust Reference, Items: https://doc.rust-lang.org/reference/items.html
- Rust Reference, Scopes: https://doc.rust-lang.org/reference/names/scopes.html

### Zig

Zig declarations are explicitly order-independent, with semantic analysis/discovery occurring lazily as declarations are referenced.

Reference:
- Zig Language Reference, File and Declaration Discovery:
  https://ziglang.org/documentation/master/#File-and-Declaration-Discovery

### Relevance to NewLang

This comparison is **non-binding**.

It exposes the central tradeoff:

- C-style ordering can simplify a sequential frontend but usually needs prototype/forward-declaration surface;
- Rust/Zig-style order independence shifts work to declaration collection/lazy analysis but keeps source smaller.

For NewLang specifically:

- one semantic compilation unit is already the v0 model;
- body-sensitive whole-unit/source access is already acceptable;
- recursive SCC semantics already exist conceptually;
- minimal source surface is a stated goal.

Therefore external evidence reinforces, but does not determine, the A decision.

## F2 decision

**F2 NOT TRIGGERED.**

Draft 17.13 adds source declaration/name-resolution rules around already-defined ordinary function semantics.

No new ownership/lifetime/dependency invariant is required.
Recursive SCC analysis was already present in §13.5c.

If later production work finds a semantic contradiction in summary recursion or function identity, that would be a new escalation; none was found here.

## Exact normative sections changed

Candidate Draft 17.13 changes only:

1. Draft 17.13 summary;
2. §18.1 ordinary function:
   - exact non-generic declaration grammar;
   - placement;
   - name/duplicate rules;
   - unit-wide declaration visibility;
   - forward reference;
   - recursion name-resolution distinction;
3. §19.1:
   - clarifies that source `fn` is top-level and is not a block item;
4. §21.8:
   - connects ordinary source `fn` to ordinary lexical lookup and preserves associated separation;
5. §28.2:
   - records the ordinary-function-category-specific order-independent visibility and duplicate rule.

No unrelated §18 ownership/call semantics, §21 generic semantics, or §28 declaration identity rules are rewritten.

## Remaining production precision / completeness after canonical closure

Expected implementation limits may remain:

- current parser is fragment-oriented, not a top-level declaration frontend;
- body-backed -> body-backed calls are currently bounded/rejected;
- recursive SCC analysis is not implemented in the P8/P9 slice;
- existing P9 match/operand precision fences remain.

These are not made into new language restrictions by Draft 17.13.

## Proposed bounded production handoff

Only after review/merge, a separate P task should implement the smallest source declaration bridge:

1. parse exact Draft 17.13 top-level `fn` production;
2. collect all bounded ordinary function names/signatures for the semantic unit before body checking, or implement equivalent order-independent discovery;
3. reject duplicate ordinary function names, duplicate parameter names, and `unit` names transactionally;
4. lower each source declaration into the existing durable P8/P9 registered body/signature representation rather than inventing parallel function semantics;
5. support W1/W2/W3;
6. support W4 ordinary forward resolution;
7. add at least one acyclic two-function body-to-body validation path if required to demonstrate forward-call semantics;
8. keep recursive SCC execution as an explicit analysis-precision boundary until implemented soundly;
9. preserve associated lookup separation;
10. preserve all P8/P9/P10/R regressions and OOM rollback.

That P task must not silently add generic declaration syntax or modules.

M9.8 does not start it.

## Findings / disposition

- **M9.8-SOURCE-SURFACE:** ordinary non-generic `fn` declaration can be closed without new function semantics.
- **M9.8-VISIBILITY:** whole-unit/order-independent bounded ordinary-function signature visibility selected after fresh A/B/C comparison.
- **M9.8-FORWARD:** later textual/other-file declaration resolves in an earlier body.
- **M9.8-RECURSION:** self/mutual name resolution permitted; production SCC analysis remains separable precision work.
- **M9.8-DUPLICATE:** same ordinary function name is error regardless of signature; no overload set.
- **M9.8-ASSOCIATED:** ordinary source `fn` does not register into associated candidate sets.
- **M9.8-GENERIC:** generic-looking declaration remains outside the closed grammar.
- **M9.8-TOPLEVEL:** no general all-item compilation-unit grammar required.
- **F2:** not triggered.
- **Deep Research:** not required; light targeted comparison completed.

Candidate Draft 17.13 is ready for Coordination review with an open, unmerged PR.
