# Draft 17.15 review resolution

Track: M

Compiler Issue #82 / M9.11 の targeted source-surface adjudication。
既存§27.3 / §27.4 finite branch/join semanticsを変更せず、
exact minimal value-producing `if` source profileだけを閉じる。

Draft 17.15 はcandidateであり、mainへmergeされるまではcanonicalではない。

## Authority

開始時:

- Compiler main: `a74184a2cd923e59fe7ad2f18fa3e8ed8b76073c`
- `CURRENT_SPEC.md` -> Draft 17.14
- Semantic Sync #80: CLOSED / PASS
- M9.10 #81: next = exact minimal value-producing `if` source profile
- P11/P12: CLOSED
- R6/R6-01: CLOSED
- FormalProof main: `a3a8c70becb3da6a9c2fc6f91ca578f49b19df32`
- F2: WAIT / not triggered

## Semantic floor retained

No new `if` ownership/lifetime/dependency state machine is introduced.

The candidate reuses:

- §19.1 lexical blocks / tail results / explicit return termination;
- §26.24 finite normal result rule shared with `if`;
- §27.3 normal result / availability / dependency / memory-state join;
- §27.4 value-producing `if` semantics;
- §27.9 terminating-edge exclusion.

Normal outgoing edge count 0 still does not synthesize bottom / never / `unit`.

## Exact grammar alternatives

### A — no condition parentheses

Conceptually:

```text
if expression lexical_block else lexical_block
```

Surface tokens are fewer, and prior conceptual examples used this style.

Rejected for the exact closed profile.

Reason: current expression grammar already includes:

```text
aggregate_expr := TypeName '{' ... '}'
```

so a brace immediately after a condition participates in both expression and arm-boundary parsing pressure.
A no-parentheses form therefore requires an if-specific rule for where top-level condition expression parsing stops, or equivalent contextual parser machinery.

Existing production `match expression {` uses a special scrutinee parsing fence as implementation evidence that this pressure is real; that implementation detail is not made normative.

M9.11 should not add a second expression-boundary special case merely to save two punctuation tokens.

### B — required condition parentheses

**Selected.**

```text
if_expression :=
    'if' '(' expression ')' lexical_block 'else' lexical_block
```

Benefits:

- exact condition boundary is syntactic;
- aggregate construction / future expression growth does not compete with arm-opening brace;
- no if-specific expression-subgrammar is needed;
- diagnostics have a clear expected `)` boundary;
- surface remains familiar and small.

The parentheses are part of `if` syntax only.
M9.11 does not add a general grouping expression.

### C — optional condition parentheses

Rejected.

It preserves the ambiguity/compatibility burden of A while adding a second accepted spelling.
There is no compensating semantic capability.

## Mandatory `else`

### A — mandatory `else`

**Selected.**

The closed `if` is always a two-branch value-producing expression.

This preserves:

- total explicit branch shape;
- exact normal result rule;
- no implicit false-path value;
- no statement-only `if` category.

### B — no-`else` form

Rejected from the current profile.

A no-`else` form would require choosing at least one new rule:

- implicit false `unit` arm;
- unit-only result restriction;
- special statement/control category;
- special behavior when the sole true arm terminates.

None is required by existing §27.4 semantics.

Effect-only conditional remains expressible explicitly:

```text
if (cond) {
    effect();
} else {
}
```

and can be used as an ordinary expression statement with trailing `;`.

Therefore:

```text
if (cond) {
    effect();
}
```

is outside the Draft 17.15 grammar.

## `else if`

Direct:

```text
else if (...)
```

is **outside the initial closed grammar**.

No dedicated sugar/flattening rule is fixed.

Equivalent finite structure is already expressible as:

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

because the nested `if` is the tail expression of the else-arm lexical block.

Future `else if` sugar remains addable without changing finite branch semantics.

## Structural ordinary-name policy

Compared:

### A — reserve both `if` and `else`

**Selected.**

Both are structural anchors of the exact closed construct.

They are added to the current §21.8 ordinary lexical reservation set:

```text
fn
let
return
match
if
else
```

This:

- does not require dedicated lexer keyword token kinds;
- does not globally reserve field / variant / member labels;
- does not reserve `loop` / `break` / `continue`;
- applies wherever a source form creates an ordinary lexical name.

### B — reserve only `if`, keep `else` ordinary/contextual

Rejected.

Although `else` only appears after the first arm, allowing ordinary lexical `else` gives little useful naming capacity and creates avoidable human/parser/diagnostic asymmetry inside one fundamental construct.

### C — contextual ordinary-name disambiguation

Rejected for the same M9.9 reason:
saving two ordinary identifier spellings is not worth permanent fallback/lookahead/diagnostic/future-grammar complexity.

Malformed structural `if` / `else` syntax therefore does not fall back to ordinary same-spelling lookup.

## W1 — simple value

```text
let y =
    if (cond) {
        a
    } else {
        b
    };
```

Disposition: **PASS**.

- condition exactly once;
- condition static type `bool`;
- exactly one arm evaluated;
- each arm exact lexical block;
- normal result types exactly equal;
- no implicit conversion/ranking;
- `if` result is the selected normal arm result with its semantic package/dependencies.

The final semicolon above belongs to the surrounding binding, not the `if` expression.

## W2 — one arm returns

```text
if (cond) {
    return x;
} else {
    y
}
```

Disposition: **PASS**.

The return edge terminates and contributes nothing to the current normal result / availability / dependency / memory-state join.

The else normal edge alone determines the normal `if` result/post-state.

## W3 — both arms return

```text
if (cond) {
    return x;
} else {
    return y;
}
```

Disposition: **PASS**.

There are zero normal outgoing edges.

No:
- bottom;
- never;
- synthetic normal `unit`;
- fabricated normal availability/state

is introduced.

## W4 — non-Copy availability mismatch

```text
if (cond) {
    consume(owner);
    unit
} else {
    unit
}
```

Both arms complete normally, with:
- true edge: owner Consumed;
- false edge: owner Available.

§27.3 therefore rejects any common normal continuation.

No MaybeConsumed state is introduced.

If the consuming arm instead terminates by `return`, that edge is excluded; it does not force a mismatch on the remaining normal edge.

## W5 — Copy/non-Copy result transfer

Copy result:
- follows ordinary Copy value semantics;
- no extra if-specific implicit copy.

Non-Copy result:
- selected arm's result responsibility transfers into the `if` result;
- unselected arm does not execute;
- no duplicate result responsibility is created.

A branch-local semantic value can survive the lexical arm only if existing scope/dependency exit compatibility permits it.

## W6 — ref/dependency result

Two normal arms may return the same static ref type with different valid incoming provenance/dependency facts.

Disposition: **allowed by the language** subject to §27.3 sound join.

The candidate does not require facts to be identical merely because the static type is equal.

The compiler must preserve a sound joined semantic package / may-set, or precision-reject if it cannot represent the join.

P7's concrete C alternative representation is evidence, not normative source semantics.

## W7 — one normal ref / one return

```text
if (cond) {
    return x;
} else {
    ref_value
}
```

Only the else normal edge contributes to the normal ref result.

No phantom alternative is added for the return edge.

## W8 — nested match

```text
if (cond) {
    match x {
        A => { a },
        B => { return r; },
    }
} else {
    b
}
```

Composition:

- inner B return edge terminates and is excluded from inner match normal join;
- inner A normal result becomes the true-arm lexical-block result;
- that normal result joins with outer else result under §27.3;
- the return edge is not reintroduced at the outer join.

No new nested-control semantic state is required.

## W9 — nested `if`

Nested finite ifs compose recursively as ordinary expression + lexical-block evaluation.

No backedge exists.
No loop/fixpoint rule is introduced.

This is also the chosen replacement for direct `else if` sugar.

## W10 — source/name attacks

Rejected as ordinary lexical names:

```text
fn if() -> unit { unit }
fn else() -> unit { unit }

let if = value;
let else = value;
```

Likewise:
- parameters;
- multi-result receivers;
- aggregate-destructuring fresh locals;
- match-payload fresh bindings

cannot use exact `if` / `else`.

But field / variant / member labels are not globally reserved by this rule.

For shorthand destructuring, a field label `if` may exist, but using it to create a same-named fresh ordinary local rejects at the local-name boundary.

## W11 — no-`else`

```text
if (cond) {
    effect();
}
```

Disposition: **not in Draft 17.15 closed grammar**.

No implicit `unit` false branch is inferred.

## W12 — direct `else if`

```text
if (a) {
    x
} else if (b) {
    y
} else {
    z
}
```

Disposition: **not in Draft 17.15 closed grammar**.

Use:

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

No source-level flattening identity is introduced yet.

## Caller-visible memory post-state

No new rule is needed.

If normal arms produce different caller-visible/current-value states, §27.3 joins the same CFG relation used for visible results/dependencies.

A sound may-set approximation is permitted.

If a first production slice cannot represent a particular correlated memory-state join, it may precision-reject; it may not select one arm's state as authoritative.

## Source integration

`if_expression` is an ordinary expression.

Therefore in §19.1:

- non-tail expression item:
  ```text
  if (...) { ... } else { ... };
  ```
- tail expression:
  ```text
  if (...) { ... } else { ... }
  ```

No dedicated `if` statement/block-item category is added.

Arm bodies are exact existing lexical blocks.

## Parentheses rationale vs current match

The existing exact `match expression { ... }` grammar is not retroactively changed.

Production currently carries an explicit match-scrutinee parsing fence around `{`/aggregate pressure.
That is implementation evidence, not an authority to copy the same boundary into every future construct.

M9.11 chooses explicit `(` / `)` for new `if` source so no new if-specific expression-stop contract is required.

This does not reopen `match`.

## External research

**Not required.**

The decisive fork is local:

- existing aggregate-expression brace syntax;
- existing lexical-block syntax;
- M9.9 structural-name policy;
- already-canonical value-producing `if` semantics.

No external-language fact is needed to resolve it responsibly.

## F2

**WAIT / NOT TRIGGERED.**

Draft 17.15 does not change:
- ownership;
- lifetime;
- dependency algebra;
- normal control-flow join invariant;
- terminating-edge semantics.

It closes source grammar + ordinary-name policy around those existing rules.

## Candidate normative changes

Draft 17.15 changes the smallest connected source/name surface:

1. Draft 17.15 summary.
2. §4.9 cross-reference to the enlarged structural reserved set.
3. §13.5a source examples only: exact new `if (...)` spelling.
4. §16.1 member-label/fresh-local explanation updated for `if` / `else`.
5. §18.1 ordinary function/parameter name admissibility follows enlarged set.
6. §19.1: `if` remains ordinary expression; statement/tail semicolon integration.
7. §21.8: current structural set adds exactly `if` / `else`.
8. §26.10 payload-binding ordinary-name admissibility follows enlarged set.
9. §27.1 / §27.1a ordinary receiver admissibility follows enlarged set.
10. §27.3 example only: exact new `if (...)` spelling.
11. §27.4: exact source grammar / mandatory else / direct else-if disposition / source integration, while preserving existing semantics.
12. §27.5 conceptual loop example only: nested `if` spelling updated for consistency.

No loop / continue / break semantic rule is changed.

## Bounded production handoff

Only after Draft 17.15 review/merge, a separate P task should:

1. add dedicated `if` syntax representation;
2. parse exactly:
   ```text
   if '(' expression ')' lexical_block else lexical_block
   ```
3. reject:
   - missing condition parentheses;
   - no-`else`;
   - direct `else if`;
4. extend central structural-name admission with exact `if` / `else`, while keeping member labels unreserved;
5. evaluate condition once and require `bool`;
6. reuse/generalize finite branch checking from match where sound;
7. reuse P9 terminating outcome;
8. preserve P7-style dependency/ref alternatives for W6/W7;
9. support nested `if` / match / return composition;
10. remain transactional/OOM-safe;
11. keep loop/backedge/fixpoint machinery completely out of scope.

The first P slice may retain explicit precision rejection for P7/P6-style areas not required to prove this bounded source:
- joined write mutation;
- ptr conversion from a multi-alternative joined ref;
- richer aggregate/sum-target ref joins;
- arbitrary correlated caller-visible memory-state phi beyond sound may-set support.

It must **not**:
- reject all same-typed dependency/ref results;
- erase dependencies/provenance;
- import terminated-edge alternatives;
- choose one branch's post-state unsoundly.

## Exact non-goals retained

No:
- loop;
- break;
- continue;
- while/for;
- if-let/pattern conditional;
- match guard;
- boolean short-circuit design;
- ternary;
- bottom/never;
- implicit result conversion;
- generic declaration;
- associated registration;
- requires fn;
- callable source;
- nominal declaration source;
- modules;
- production implementation;
- F/R;
- LLVM / relocation / FFI / concurrency.

## Disposition

Candidate Draft 17.15 is ready for Coordination review.
