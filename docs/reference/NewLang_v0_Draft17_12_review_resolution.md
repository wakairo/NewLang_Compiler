# Draft 17.12 review resolution

Track: M

この文書は Compiler Issue #63 / M9.6 で行った、
R5-01 `unit` source-name reservation / shadowing ambiguity に対する targeted adjudication を記録する。

normative authority はこの文書ではなく `docs/reference/CURRENT_SPEC.md` が選択する canonical Draft である。
Draft 17.12 はreview前のcandidateであり、mainへmergeされるまではcanonicalではない。

## Scope

Draft 17.12 は Draft 17.11 の explicit-return control semantics、ownership、lifetime、dependency semanticsを変更しない。

閉じるのは一つだけ:

> source spelling `unit` が core singleton type/value と ordinary lexical name lookup / binding introduction のどちらに属するかを一意にする。

source ordinary-function declaration grammar、general keyword system、modules/qualification等はscope外のまま。

## R5-01 specification-level reproduction

Draft 17.11では:

- §4.9: `unit` はordinary singleton type/value;
- §19.1 / §27.9: unit returnは `return unit;`;
- §21.8: ordinary non-dependent nameはlexical lookup;
- §27.1: `let x = expression` がordinary binding;

と定義される一方、
`unit` がbindable / shadowableかどうかは未定義だった。

従って:

```text
let unit = x;
return unit;
```

について:

- local `unit` を参照するのか;
- builtin unit valueを参照するのか;
- binding自体が不受理なのか

をcanonical textだけで一意に決められなかった。

R5-01のprimary classificationである SOURCE-SURFACE-GAP / canonical ambiguity は成立する。

## Alternatives

### A — token-level/global reserved keyword

例:
- lexerが `unit` をidentifier/wordとは別tokenにする;
- field / variant / member spellingを含め、identifier位置で全面的に禁止する。

Rejected as too broad.

R5-01が必要とするのは ordinary lexical name と core unit spelling の衝突解消であり、
field labelやvariant labelまで禁止する必要はない。
general keyword table / lexer redesignも不要。

### B — ordinary predeclared name + lexical shadowing

`unit` をouter/prelude bindingとして扱い、local `let unit = x` がshadowする案。

Rejected.

この案は一見§21.8と自然だが、現在のv0には一般prelude/qualification modelがない。
shadow後にbuiltin singleton valueを直接spellする方法を別途必要とし、
type-position/value-positionのcore `unit` identityとordinary lexical bindingの関係も新たに定義する必要がある。

特に:

```text
let unit = token;
return unit;
```

をToken returnにする一方で、同scope内からcore unit valueを必要とする場合のsource surfaceが失われる。
それを救うqualificationや別literalを足す方がR5-01より大きい。

### C1 — contextual builtin-first lookup while binding remains legal

`let unit = x` は作れるが、expression `unit` は常にbuiltinへ解決する案。

Rejected.

これはbindingをsourceから到達不能にし、
ordinary lexical lookup ruleをspelling依存でsilent bypassする。
R5で観測されたproduction mismatchをcanonical化するだけで、coherent shadowing ruleにならない。

### C2 — distinguished core spelling + ordinary lexical name reservation

**Selected.**

Rule:

- `unit` is a distinguished core source spelling;
- type positionではcore unit type;
- value expression positionではcore singleton value;
- ordinary lexical lookup candidateではない;
- ordinary lexical namespaceへ `unit` をbinding/declaration nameとして導入できない;
- lexer token categoryは変更不要;
- separate member namespacesのfield/variant labelまでは一律予約しない.

これはAより小さく、Bのprelude/qualification machineryを不要にし、C1の到達不能bindingも作らない。

## Lexical token reservation vs name admissibility

Draft 17.12 does **not** require `unit` to become a special lexer token.

An implementation may still lex it as the same WORD/identifier-like token class as other names.

The normative distinction happens when source spelling is interpreted:

- core type/value position -> distinguished `unit`;
- ordinary lexical binding/declaration introducer -> exact spelling `unit` is inadmissible;
- nominal/member label namespace -> not automatically forbidden by M9.6.

Thus this is a local source-name reservation, not a general reserved-word system.

## Break tests

### 1. `let unit = x; return unit;`

**REJECT at binding-name admissibility.**

No local `unit` binding is created.
The later `return unit;`, in a program without the invalid binding, denotes the core singleton value.

For a Token-returning function, `return unit;` therefore remains a normal result-type error if reached; it never refers to a shadow binding.

### 2. `let unit = unit;`

**REJECT because the receiver name is inadmissible.**

If the RHS were considered independently, its `unit` spelling denotes the builtin singleton value.
No self-shadowing or special evaluation-order rule is introduced.

### 3. nested block shadowing

```text
{
    let unit = x;
}
```

**REJECT.**

The reservation is not scope-depth dependent.
No lexical descendant can shadow core `unit`.

### 4. ordinary function parameter named `unit`

The source `fn` declaration grammar remains unresolved/out of scope.

However, the name rule is independent of that grammar:

> any source form that introduces an ordinary lexical parameter binding cannot use exact spelling `unit`.

Thus a future/minimal source parameter `unit: T` is inadmissible.
This does not otherwise define parameter punctuation, declaration placement, lookup, or recursion.

### 5. multi-result receiver named `unit`

```text
let (unit, rest) = expression;
```

**REJECT.**

§27.1a receivers are fresh ordinary bindings and inherit §4.9 name admissibility.

### 6. aggregate destructuring receiver / field binding named `unit`

Current shorthand:

```text
let Type { unit } = value;
```

uses the field label as the same-named fresh ordinary local binding.

**REJECT because of the local binding name**, not because the field/member label spelling itself is globally illegal.

M9.6 does not prohibit a fixed-shape aggregate from having a member label `unit` in construction/member namespace.
The current destructuring shorthand simply cannot introduce a local with that reserved ordinary name.

No renaming destructure is added here.

### 7. builtin `unit` in ordinary non-return expression position

```text
consume_or_observe(unit)
```

where the context accepts unit:

**resolves directly to the core singleton value.**

It does not scan ordinary lexical candidates for a shadow.

### 8. `unit;` expression statement

The expression denotes the core singleton value.

Because unit is Copy + Discardable under §4.4, discarding that visible result under §19.1 is legal.

No special statement semantics are added.

### 9. block tail `unit`

```text
{
    unit
}
```

normal-completes with the core unit singleton value.

This is the same builtin identity as no-tail block's semantic unit result; no lexical binding lookup is involved.

### 10. single lexical namespace / builtin model

The ordinary single lexical namespace remains unchanged for ordinary names.

Core `unit` is **not modeled as a predeclared ordinary lexical entry**.
Therefore:

- no prelude insertion order;
- no lexical shadowing;
- no qualification to recover builtin unit;
- no collision-dependent lookup priority

is required.

This targeted exception does not establish a general builtin-first lookup rule for other names.

## Match payload binding

`Variant(unit)` would introduce a fresh ordinary arm binding under §26.10.

Therefore exact spelling `unit` is inadmissible as the payload binding name.

The variant/member label itself may independently contain the spelling `unit` only if its own nominal namespace rules permit it; M9.6 does not broaden into variant naming policy.

## Exact normative sections affected

Candidate Draft 17.12 edits only the rules needed to make the source-name model coherent:

- Draft summary: targeted M9.6 closure;
- §4.9 `unit`: distinguished core spelling / non-shadowable ordinary lexical name reservation;
- aggregate destructuring section (§16.1): shorthand fresh binding named `unit` is inadmissible, field label itself not globally reserved;
- §21.8: `unit` is not an ordinary lexical candidate/predeclared binding;
- §26.10: match payload binding name cannot be `unit`;
- §27.1: ordinary binding name cannot be `unit`;
- §27.1a: multi-result receiver cannot be `unit`.

No §18 / explicit-return control rule is changed.

## Local vs broader lexical rule

The fix is **local to the exact spelling `unit`**.

It does not imply:

- a general keyword/reserved-word registry;
- all core type names are automatically reserved by the same new rule;
- member/field/variant labels are globally barred from using the spelling;
- modules/qualification/prelude semantics;
- source `fn` declaration grammar.

Future source features that introduce an **ordinary lexical name** must respect the already-fixed `unit` reservation, but their syntax is not defined by M9.6.

## F2 decision

**F2 NOT TRIGGERED.**

This is source-name admissibility/name-resolution closure.
No ownership/lifetime/control-flow rule changes and no semantic contradiction was found.

## Deep Research decision

**Deep Research NOT REQUIRED.**

The alternatives are determined by current NewLang lookup/source constraints.
External language surveys would not improve the minimality decision.

## Proposed bounded P follow-up

Only after candidate Draft 17.12 is reviewed/merged, a separate P task should:

- reject `unit` at every currently supported ordinary binding introducer;
- include ordinary single binding, multi-result receiver, aggregate destructuring fresh local, match payload binding, and body/host parameter names where they model source-equivalent ordinary bindings;
- preserve field/variant label handling outside ordinary binding namespace;
- make expression `unit` consistently denote builtin core unit;
- test `unit;` and tail `unit`;
- keep failure transactional;
- retain all prior explicit-return tests;
- avoid general keyword/parser redesign unless implementation structure requires an internal helper.

M9.6 does not start that P work.

## Proposed R5-01 targeted revalidation condition

After canonical merge and the bounded P fix, targeted R5-01 revalidation should require at minimum:

1. original witness
   ```text
   let unit = x;
   return unit;
   ```
   is rejected because `unit` cannot be introduced as an ordinary binding;
2. `return unit;` with no illegal shadow still denotes the core unit value;
3. nested ordinary binding, multi-result receiver, aggregate destructuring local, and match payload binding named `unit` are consistently rejected;
4. ordinary non-return `unit`, `unit;`, and tail `unit` still denote the core singleton;
5. field/variant label spelling is not accidentally over-reserved beyond the canonical rule;
6. no builtin-first lookup path can coexist with a successfully created ordinary lexical `unit` binding.

Only then can R5-01 be considered CLOSED.

## Findings

- **M9.6-SOURCE-NAME-GAP:** R5-01 valid; Draft 17.11 was ambiguous.
- **M9.6-SELECTED:** distinguished core spelling + ordinary lexical name reservation.
- **M9.6-NOT-KEYWORD-REDESIGN:** token-level/global reservation rejected as broader than required.
- **M9.6-NO-SHADOW:** ordinary predeclared-name shadowing rejected because it would require extra builtin recovery/prelude machinery.
- **M9.6-MEMBER-NAMESPACE:** field/variant labels are not globally reserved by this targeted rule.
- **M9.6-SEMANTIC-CONTRADICTION:** none.
- **F2:** not triggered.
- **Deep Research:** not required.

## Disposition

Candidate Draft 17.12 is ready for Coordination review with an open, unmerged PR.
