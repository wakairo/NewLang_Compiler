# Draft 17.11 review resolution

Track: M

この文書は Compiler Issue #58 / M9.5 で行った、
host-known / registered ordinary-function body における explicit return の
exact lexical-block source closure に対する targeted adjudication を記録する。

normative authority はこの文書ではなく、
`docs/reference/CURRENT_SPEC.md` が選択する canonical Draft である。
Draft 17.11 はreview前のcandidateであり、mainへmergeされるまではcanonicalではない。

## Scope

Draft 17.11 は Draft 17.10 の ordinary-function semantics を変更しない。

閉じるのは一つだけ:

> semantic contextですでにidentity/signatureがknownなordinary function bodyで、
> existing `return expr` semanticsをexact lexical-block sourceとして書けるようにする。

source ordinary-function declaration grammarは閉じない。

対象外:

- `fn name(...)` declaration grammar
- top-level item grammar
- function declaration placement
- forward reference
- self/mutual recursion source policy
- duplicate/name-category rules
- generic function declaration syntax
- callable/loan source closure
- general statement language
- bottom / never type

## Current-authority audit

Draft 17.10 already fixes the semantics needed by this source closure:

- §18.5: explicit return is a control-flow terminator; return expression uses ordinary value-use; return itself has no normal block result.
- §18.6: non-Copy consume-out / source incarnation ending.
- §18.8: return-edge function-exit dependency/scope compatibility.
- §13.5a: terminating edges do not participate in normal result/state joins; scope-exit compatibility.
- §26.24–25: terminating match arms do not participate in normal match-result/availability joins.
- §27.3: normal joins use only normal incoming edges.
- §27.9: return terminates the enclosing named function.
- §4.9: `unit` is an ordinary singleton type/value.

The actual source gap was §19.1:
the closed block grammar admitted only binding/expression items and had no exact return item.

No semantic contradiction was found between these sections.

## Design comparison

### A — named dedicated return terminator / block-item category

Conceptually:

```text
return_item := 'return' expression ';'
block_item  := ... | return_item
```

Advantages:

- return stays outside ordinary expression typing;
- no bottom/result type is needed;
- semicolon/tail behavior is exact;
- control-flow termination is visible in the block grammar.

Cost:

- introduces a named grammar category that is not independently needed.

Semantically sound, but not the smallest grammar formulation.

### B — return as terminating expression

Rejected for this closure.

Making `return` an `expression` would force extra questions that the representative workloads do not need:

- what static result type does the expression have?
- may it appear in `let x = return y`?
- may it appear as a call argument/subexpression?
- is a bottom/never-like type needed?
- is semicolon-less tail `return y` an ordinary tail expression?

Restricting those positions again would create special expression-context rules.
That is larger than the exact block-item gap being closed.

No existing §18 semantics require return to be an ordinary expression.

### C — inline dedicated terminator alternative

Selected.

Exact grammar extension:

```text
block      := '{' block_item* [tail_expr] '}'
block_item := binding ';'
            | expression ';'
            | 'return' expression ';'
```

This keeps the semantics of A without adding a general statement language or a new reusable grammar category.

The source `return` is a dedicated terminating block item, not an ordinary expression.

## Semicolon / tail / bare-return adjudication

### Semicolon

Required.

Exact form:

```text
return expression;
```

Rationale:

- it is a non-tail block item;
- existing §19.1 uses semicolon for non-tail sequencing;
- requiring it avoids inventing newline termination or a second tail-like terminator form.

### Tail-position return

Semicolon-less tail return is not in the closed profile.

Rejected:

```text
{
    return value
}
```

Accepted exact form:

```text
{
    return value;
}
```

A return edge has no normal block result, so treating it as a tail expression would only blur the existing tail-result rule.

### Bare return

Not added.

```text
return;
```

is outside this closed profile.

For a unit-result function:

```text
return unit;
```

uses the existing ordinary singleton value and needs no second return production.

No systems workload in M9.5 requires bare return for expressiveness.

## Return-enabled source context

The source profile is enabled by an already-known ordinary-function semantic context.
Host registration is sufficient.

The ordinary function body is the return-enabled root.

Ordinary lexical descendant blocks inherit the context when no callable/loan boundary is crossed.
Therefore return can be written from:

- an ordinary nested lexical block;
- a match-arm lexical block;
- another ordinary lexical descendant supported by the surrounding source profile.

Entering a nonescaping callable block or loan body stops this M9.5 source-profile inheritance.

A `return expression;` inside those contexts is not admitted by this closed profile.
This does not permanently decide future non-local return semantics there.

Callable invocation termination remains `leave` under existing §19.4 semantics.
Loan final source syntax remains Provisional under §13.8.

## Control propagation / joins

On a return edge:

1. evaluate the return expression exactly once;
2. apply ordinary value-use;
3. transfer/copy the function result under existing §18 rules;
4. check function-exit compatibility;
5. perform no implicit cleanup;
6. do not execute remaining block items or the tail expression on that edge;
7. do not contribute that edge to normal block/match/availability joins.

If every reachable branch terminates, there is no normal outgoing edge.

No bottom/never value is synthesized.
No synthetic `unit` normal result is invented.
Existing result/join rules simply have no normal edge to join.

Unreachable-code warning/lint policy remains non-normative.

## Required break tests

### 1. return from nested lexical block — PASS

Within a return-enabled ordinary-function body:

```text
{
    {
        return result;
    };
    ...
}
```

the return edge terminates the enclosing ordinary function, not only the nested block.

The exact enclosing block expression syntax outside this return item remains governed by its existing source profile; M9.5 does not introduce a new block statement language.

### 2. return from a match arm — PASS

```text
match r {
    Ok(v) => {
        v
    },
    Err(e) => {
        return Result<Packet, Error>.Err(e);
    },
}
```

The Err arm has no normal match-result edge.
The Ok arm is the only normal contributor.

### 3. return followed textually by a non-Copy use — PASS / no false flow

A return edge does not flow into later text.

A later non-Copy use cannot create a normal availability requirement on the return edge.

M9.5 does not require unreachable source to be accepted solely because it is unreachable; ordinary syntax/static checking may still inspect it.
What is forbidden is fabricating a normal continuation state from the return edge.

### 4. one branch returns, another reaches continuation — PASS

Only the normal branch contributes result / availability / current-memory state to the continuation.
The return branch is excluded by existing §27.3 / §26.24 semantics.

### 5. all branches return — PASS

There are zero normal outgoing edges.

No bottom/never type is required.
No normal result is required merely to represent the terminated construct.

### 6. non-Copy return result — PASS

The return expression's ordinary non-Copy value-use consumes/transfers the responsibility exactly once into the function result.

No result is minted merely from its declared type.

### 7. Copy return result — PASS

Ordinary Copy value-use applies.
No forced-move special case is introduced.

### 8. attempted ordinary-ref return — REJECT by existing semantics

M9.5 does not relax §18.5 scope-bound capability escape rules.

The exact return spelling cannot be used to launder an ordinary ref across the function boundary.

### 9. caller-visible local-dependent state left live at return — REJECT

§18.8 / §13.5a function-exit compatibility rejects the edge if caller-visible surviving state still depends on an ending function-local scope/fact.

### 10. local-dependent state restored before return — PASS where otherwise well-formed

If the caller-visible dependent state is restored/ended before the return edge, and the candidate post-state contains no ending-local dependency, the return edge can satisfy existing exit compatibility.

No special return exception is required.

### 11. return outside ordinary function — REJECT in this source profile

A parsed return item is source-admissible only in the return-enabled ordinary-function lexical context.

M9.5 does not define top-level return or return in arbitrary block fragments.

### 12. return inside callable / loan context — OUTSIDE PROFILE / reject for current closed profile

Crossing a callable/loan boundary stops M9.5 return-source inheritance.

This avoids silently deciding non-local return behavior across 0..N callable invocation or provisional loan syntax.

### 13. ordinary tail after a possibly-returning item — PASS

Normal paths continue to the ordinary tail expression.
Return paths skip it and do not contribute to its normal join.

If no normal path reaches the tail, the tail is unreachable on those paths; no bottom type is introduced.

### 14. unreachable-code diagnostics — NOT A SEMANTIC REQUIREMENT

M9.5 does not make unreachable code a mandatory error or warning.

Implementations may diagnose it, but diagnostics must not alter the semantic fact that the return edge has no normal successor.

## W1 — decoder early failure

Representative source inside a host-known function body:

```text
let header =
    match parse(input) {
        Ok(h) => {
            h
        },
        Err(e) => {
            return Result<Packet, Error>.Err(e);
        },
    };
```

Pressure result:

- exact return spelling is unambiguous;
- `parse(input)` is evaluated under existing match semantics;
- non-Copy `e` can be transferred into the returned Err value;
- the Err arm terminates;
- Err contributes no normal match result / availability edge;
- success continuation receives only the Ok arm's normal result;
- no `?` sugar or function declaration syntax is needed.

**PASS.**

## W2 — restore before return

Use a host-known ordinary-function context and a caller-visible destination.
Assume the surrounding test fixture supplies an existing function-local dependency-bearing value; M9.5 does not add local/loan construction syntax.

Unsafe shape:

```text
let old = replace(dst, local_dependent);
return result;
```

The return-edge candidate post-state still contains caller-visible state depending on a function-local fact.

Expected: reject by §18.8 / §13.5a.

Safe shape:

```text
let old = replace(dst, local_dependent);
replace(dst, old);
return result;
```

After restoration, no ending-local dependency remains in caller-visible surviving state.

Expected: pass if all other ordinary value/Discardable obligations are satisfied.

The difference is entirely existing function-exit semantics; exact return source merely reaches that edge.

**PASS as a source/semantic pressure model.**

## Ownership / dependency semantics unchanged

Draft 17.11 does not change:

- argument passing;
- parameter lifetime;
- Copy/non-Copy result semantics;
- consume-out;
- ref nonescape;
- hidden dependency propagation;
- caller-visible post-state;
- function-exit compatibility;
- match result semantics;
- availability joins.

The only normative source addition is the exact explicit-return block item and its source-context/semicolon boundary.

## Function declaration syntax remains unresolved

M9.5 does not define:

- `fn name(...)`;
- parameter-list declaration grammar;
- result-arrow declaration grammar;
- top-level function item placement;
- forward references;
- self/mutual recursion source policy;
- duplicate function declaration rules;
- generic function declaration syntax.

Host registration remains a valid way to provide the ordinary function identity/signature for this source profile.

## F2 decision

**F2 NOT TRIGGERED.**

No new control-flow semantic rule was needed.
No contradiction among §13.5a / §18 / §19 / §26 / §27 was found.
The change is exact source integration of already-defined semantics.

## Deep Research decision

**Deep Research NOT REQUIRED.**

The rejected return-as-expression alternative would only be attractive if NewLang wanted a general bottom/result-expression model.
No representative workload requires that.

Callable/loan non-local return and declaration/name-resolution policy remain outside M9.5 rather than forcing an external literature survey.

## Findings

- **M9.5-SOURCE-SURFACE:** real gap closed by exact `return expression;` lexical-block item.
- **M9.5-SMALLER-FORM:** inline block-item alternative is smaller than a new general statement category and smaller than return-as-expression machinery.
- **M9.5-BARE-RETURN:** convenience-only for current v0; omitted.
- **M9.5-DECLARATION:** ordinary-function declaration source remains unresolved/out of scope.
- **M9.5-SEMANTIC-CONTRADICTION:** none identified.
- **M9.5-TARGETED-PRECONDITION:** none required.
- **F2:** not triggered.
- **Deep Research:** not required.

## Proposed downstream production validation

After Draft 17.11 is reviewed/merged, a separate P task may validate only:

- parser/AST support for exact `return expression;` block item;
- host-registered P8 ordinary-function bodies;
- nested ordinary block propagation;
- match-arm early return W1;
- W2 explicit return-edge function-exit compatibility;
- one-normal-edge / all-terminated joins;
- non-Copy / Copy return transfer;
- ordinary-ref escape rejection;
- callable/loan/outside-function source rejection for this profile;
- rollback/OOM/diagnostics;
- prior P suites and exact-head CI.

That P work is not started by M9.5.

## Disposition

Candidate Draft 17.11 is ready for Coordination review with an open, unmerged PR.
