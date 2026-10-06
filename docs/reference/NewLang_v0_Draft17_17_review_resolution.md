# Draft 17.17 review resolution

Track: M

Compiler Issue #92 / M9.14 の targeted source/control-context adjudication。
Draft 17.16 / F2で閉じたHYBRID cyclic loop semanticsを変更せず、
exact minimal `loop` / `continue` / `break` source profileだけを閉じる。

Draft 17.17 はcandidateであり、mainへmergeされるまではcanonicalではない。

## Authority

開始時:

- Compiler main: `2a3643449ae5d9fa619909c9fa16d21b8d2ac5e6`
- `CURRENT_SPEC.md` -> Draft 17.16
- Semantic Sync #91: PASS / CLOSED
- M9.13 #89: CLOSED
- FormalProof main: `e591e53f295d4ab76d00d3bc1a2cdc443e06b7b4`
- FormalProof F2: CLOSED
- F2 finding: FORMAL-HOLE / FORMAL-AMBIGUITY / FORMAL-EXTRACTION none

No semantic reopen was required.

# 1. Exact loop grammar

Compared:

## A — mandatory parameter parentheses

```text
loop_expression :=
    'loop' '(' [loop_parameter_list] ')' lexical_block
```

**SELECTED.**

Reasons:

- zero and nonzero parameter forms share one grammar;
- parameter boundary is always explicit;
- consistent with mandatory `if (...)`;
- future source growth does not require retrofitting a special zero-param form;
- the parameter list is semantically important because it names the explicit loop-carried state.

## B — `loop { ... }` for zero parameters

Rejected.

It saves only `()` while creating two source shapes for the same construct and a later compatibility burden if parameter-related syntax grows.

## C — other forms

No smaller/coherent alternative identified.

Exact zero-param source is:

```text
loop () {
    ...
}
```

# 2. Zero loop parameters

**SUPPORTED.**

A loop may have zero explicit carried parameters.

This is useful for:

- pure divergence/retry loops;
- loops driven by outer Copy/current state;
- loops whose only exits are return/break.

Corresponding zero-argument continue is:

```text
continue();
```

No dummy `unit` parameter is required.

# 3. Loop parameter declaration / static type

Compared:

## A — inferred from initializer

```text
loop (i = initial, owner = initial_owner) {
    ...
}
```

**SELECTED.**

Each initializer normal completion must produce exactly one value.
The static type of that value is the static type of the corresponding loop parameter.

## B — explicit annotation

```text
loop (i: u32 = initial) {
    ...
}
```

Rejected from the minimal profile.

No type ambiguity requires it because continue already requires exact static type equality with the parameter slot.
Annotation would duplicate information without adding a new v0 capability.

## C — reuse another binding syntax

Rejected.

Loop parameters are a dedicated explicit loop-carried-state list, but their names obey ordinary lexical binding rules.

# 4. Initializer evaluation order and scope

**Selected rule:**

- initializer expressions evaluate left-to-right;
- every initializer performs name lookup in the **outer pre-loop lexical environment**;
- no new loop parameter binding is in scope in any initializer;
- effects/consumes performed by earlier initializer evaluation are visible to later initializer evaluation;
- after all normal initializer evaluations and receiving succeed, first-iteration parameter bindings are introduced together.

Example:

```text
loop (a = outer_a, b = a) {
    ...
}
```

The spelling `a` in `b = a`:

- resolves to an outer lexical `a`, if such a binding exists;
- otherwise follows ordinary unknown-name behavior;
- never resolves to the newly declared loop parameter `a`.

This deliberately avoids accidental sequential-`let*` scope.

# 5. Parameter name rules

Loop parameter names are fresh ordinary lexical binding names.

Rules:

- duplicate names in one loop parameter list: reject;
- exact `unit`: reject by §4.9;
- any current §21.8 structural reserved spelling: reject;
- no loop-specific identifier namespace;
- an admissible parameter name may shadow an outer ordinary name according to ordinary lexical shadowing;
- that shadow begins in the loop body, not in the initializer list.

# 6. Trailing comma

**Rejected in the minimal profile.**

Rejected:

```text
loop (i = x,) { ... }
continue(next_i,);
```

This matches the ordinary function parameter-list smallness policy.

No loop-specific pressure justifies trailing-comma syntax now.

# 7. Exact continue syntax

Selected:

```text
continue_item :=
    'continue' '(' [continue_argument_list] ')' ';'

continue_argument_list :=
    expression (',' expression)*
```

Decisions:

- dedicated terminator;
- mandatory parentheses;
- mandatory semicolon;
- zero argument allowed as `continue();`;
- comma-separated expressions;
- no trailing comma;
- no ordinary-expression result;
- no tail-expression form;
- no same-spelling ordinary call fallback.

The argument count must equal nearest target loop parameter count.

Arguments evaluate left-to-right and obey existing Copy/consume/dependency/availability rules.

# 8. Exact break syntax

Compared:

## A — `break expression;`

**SELECTED.**

## B — `break(expression);`

Rejected as a break-specific form.

Current closed expression grammar has no general grouping expression, and M9.14 does not add one.

## C — bare `break;`

Rejected.

The loop is value-producing; an explicit result is required.

Unit result is:

```text
break unit;
```

Exact decisions:

- result expression mandatory;
- semicolon mandatory;
- expression evaluated exactly once;
- dedicated terminator;
- not an expression/tail;
- no ordinary call fallback.

# 9. Block grammar integration

Draft 17.17 closes:

```text
block      := '{' block_item* [tail_expr] '}'
block_item := binding ';'
            | expression ';'
            | return_item
            | continue_item
            | break_item
```

Classification:

- `loop ...` = ordinary expression;
- `continue(...);` = dedicated terminating block item;
- `break expression;` = dedicated terminating block item;
- `return expression;` remains dedicated terminating block item.

Therefore:

- non-tail loop expression requires ordinary expression-item semicolon after the whole loop;
- tail loop expression has no outer semicolon;
- continue/break always contain their own mandatory semicolon;
- continue/break can never be tail expressions.

No implicit cleanup/destructor/fall-through is added.

# 10. Loop-control target

Selected source rule:

> `continue` / `break` target the **nearest enclosing active loop** in the same control context.

Ordinary nested lexical blocks inherit the loop target.

`if` and `match` arms are lexical blocks and inherit it as well.

Example:

```text
loop (x = initial) {
    if (done(x)) {
        break x;
    } else {
        continue(next(x));
    }
}
```

Both terminators target that loop.

# 11. Nested loop target shadowing

Entering an inner loop establishes a new nearest target.

Thus:

- inner continue -> inner header;
- inner break -> inner exit;
- after inner normal exit, outer loop target resumes in the surrounding outer lexical context.

No labels or multi-level control are added.

Nested loops remain language-legal.

# 12. Callable / loan boundary

Outer loop-control target does **not** cross:

- nonescaping callable block boundary;
- loan body boundary;
- ordinary named-function call/body boundary.

Inside such a boundary:

- `continue` / `break` cannot control an outer source loop;
- a new loop created inside the boundary establishes a new local loop target normally.

This is source control-context isolation only.
No callable/loan source redesign is performed.

# 13. Continue semantic connection

M9.14 changes no Draft 17.16 semantics.

For source continue:

- arity = parameter count;
- corresponding type exact equality;
- left-to-right evaluation;
- one value per argument;
- ordinary Copy/consume;
- affine responsibility conservation;
- hidden dependency transfer;
- iteration-local dependency nonescape;
- exact captured outer non-Copy availability invariant;
- HYBRID recurrence feeds the same header H.

# 14. Break semantic connection

M9.14 changes no Draft 17.16 break semantics.

For source break:

- result expression exactly once;
- ordinary value-use;
- all reachable break result static types agree;
- outer non-Copy availability agrees across break exits;
- ending loop/iteration dependency cannot escape;
- break does not feed the header;
- finite break result/current-state join remains Draft 17.16;
- conditional non-Copy identities remain legal language behavior even if future P precision-rejects.

# 15. Zero-break source behavior

Exact source grammar admits zero-break loops.

Example:

```text
fn spin() -> unit {
    loop () {
        continue();
    }
}
```

The loop has:

- zero normal outgoing loop edges;
- no normal result/post-state join;
- no synthetic unit/never/bottom.

No unreachable-code policy is added for source text after such a loop.

Reachable return edges still exit the function normally.

# 16. Structural ordinary-name reservation

Compared:

## A — reserve all three

```text
loop
continue
break
```

**SELECTED.**

They are fundamental expression/terminator structural words.

The §21.8 set becomes:

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

This is ordinary lexical source-name reservation only.

It does not require dedicated lexer keyword tokens.

## B — reserve only some

Rejected.

There is no meaningful ordinary-name value in preserving one of these exact spellings while reserving the others, and asymmetry worsens parser/diagnostic rules.

## C — contextual disambiguation

Rejected consistently with M9.9/M9.11.

Minimizing keyword count is not a goal.
No concrete benefit justifies fallback/lookahead complexity.

# 17. Member namespace

M9.14 does **not** globally forbid:

- field label `loop`;
- variant label `continue`;
- member label `break`.

Reservation applies to ordinary lexical bindings/declarations.

If shorthand uses such a member spelling to introduce a fresh ordinary local with the same spelling, ordinary-name admissibility rejects the local binding.

# 18. Malformed structural syntax / fallback

Malformed structural forms do not become ordinary same-spelling calls/names.

Examples:

```text
loop(...)
continue(...)
break(...)
```

are interpreted only according to the closed structural grammar available in that source position/context.

If malformed, they are rejected.
Parser backtracking to an ordinary function named `loop` / `continue` / `break` is not required or allowed by this profile.

# 19. Interaction with if / return

Representative:

```text
loop (x = initial) {
    if (done(x)) {
        break x;
    } else {
        continue(next(x));
    }
}
```

and:

```text
loop () {
    if (fatal) {
        return err;
    } else {
        continue();
    }
}
```

remain ordinary finite-control composition around Draft 17.16 cyclic semantics.

No implicit fall-through is added.

# 20. Source / semantic boundary

Source exposes only:

- loop initializer/parameter list;
- iteration lexical block;
- dedicated continue/break terminators.

It does not expose:

- H;
- phi;
- post-fixpoint;
- widening;
- invariant annotation;
- effect/lifetime annotation;
- mutable loop variables.

All Draft 17.16/F2-reviewed cyclic analysis stays compiler-internal.

# W1–W20

## W1 — zero-parameter infinite loop

```text
loop () {
    continue();
}
```

**ACCEPT.**
Zero parameters and zero-argument continue align exactly.
Zero break gives zero normal exit.

## W2 — single Copy carry

```text
loop (i = initial) {
    if (done(i)) {
        break i;
    } else {
        continue(next(i));
    }
}
```

**ACCEPT**, subject to ordinary type/dependency semantics.

Parameter type = static type of `initial`.

## W3 — two parameters

```text
loop (i = first(), acc = second()) {
    ...
}
```

- `first()` then `second()`;
- both resolved in outer pre-loop lexical environment;
- distinct parameter names required;
- continue requires exactly two values in the same parameter order and exact corresponding types.

**ACCEPT.**

## W4 — unchanged non-Copy carry

Same source form as ordinary continue.

Current package can be consumed/transferred:

```text
continue(owner);
```

No source-level move annotation is added.

**ACCEPT** if Draft 17.16 affine obligations pass.

## W5 — transformed non-Copy carry

Consume current owner, construct a fresh same-type owner, continue it.

Source needs no identity annotation.

**ACCEPT** under Draft 17.16 symbolic affine package semantics.

## W6 — duplicate parameter name

```text
loop (x = a, x = b) { ... }
```

**REJECT.**

## W7 — initializer scope

```text
loop (a = x, b = a) { ... }
```

New parameter `a` is not visible in `b` initializer.

- outer `a` exists -> use outer;
- otherwise unknown name.

**CLOSED.**

## W8 — continue arity mismatch

Parameter count and argument count differ.

**REJECT.**

Zero params require exactly zero continue args.

## W9 — break type mismatch

Two reachable break expressions with different static types.

**REJECT** by unchanged Draft 17.16 finite break semantics.

## W10 — explicit unit break

```text
break unit;
```

**ACCEPT** where loop result type is unit.

Bare `break;` rejects.

## W11 — nested lexical block / if / match

continue/break inside nested ordinary block / if arm / match arm inherits current loop target.

**ACCEPT** and targets nearest loop.

## W12 — nested loop

Inner terminators target inner loop.

Outer target resumes after inner normal exit.

**ACCEPT.**

## W13 — return inside loop

`return expression;` targets enclosing named function, never loop.

It does not feed break/header.

**ACCEPT** under existing return context.

## W14 — zero-break with return

Loop may have continue + return but no break.

No normal loop result/outgoing edge.
Return still function-exit edge.

**ACCEPT.**

## W15 — missing semicolon

```text
continue()
break x
```

as terminating items:

**REJECT.**

Semicolon is mandatory.

They are not tail expressions.

## W16 — direct call/name attacks

Rejected as ordinary function/parameter/local/loop-parameter names:

```text
loop
continue
break
```

Same-spelling ordinary calls are not preserved.

## W17 — member labels

Member/field/variant spellings `loop` / `continue` / `break` are not globally reserved by this rule.

**NOT OVER-RESERVED.**

## W18 — trailing comma

```text
loop (i = x,) { ... }
continue(next_i,);
```

**REJECT.**

## W19 — zero-argument continue

```text
continue();
```

**ACCEPT** exactly when nearest target loop parameter count is 0.

Otherwise arity error.

## W20 — outer same-name binding

```text
let x = outer;
loop (x = x) {
    ...
}
```

Initializer RHS `x` resolves in outer environment.

After initializer receiving, fresh loop parameter `x` shadows that outer name inside the loop body according to ordinary lexical scope.

**ACCEPT** if the outer use/ownership rules permit the initializer transfer.

# Formal implication

**NO NEW F TASK TRIGGERED.**

Reason:

- M9.14 changes source grammar/control targeting only;
- Draft 17.16 HYBRID semantics are unchanged;
- F2 already proves the relevant cyclic semantic kernel;
- no new semantic invariant or contradiction was introduced.

FormalProof does not need to model punctuation/name reservation to validate M9.14.

# Bounded production handoff

Only after Draft 17.17 review/merge, a separate bounded P task should implement:

1. central ordinary-name structural classifier additions:
   - loop
   - continue
   - break
2. syntax nodes for loop / loop parameter / continue / break;
3. parser for exact loop grammar;
4. outer-pre-loop initializer lookup and left-to-right evaluation;
5. duplicate/admissibility/trailing-comma checks;
6. dedicated terminating block items;
7. nearest-loop control-context stack;
8. inheritance through ordinary lexical block / if / match;
9. nested-loop target shadowing;
10. no target inheritance across callable/loan/function boundary;
11. continue arity/type/value transfer;
12. Draft 17.16 cyclic header analysis;
13. finite break exit join;
14. zero-normal-exit representation;
15. transactional publication / OOM / resource-limit behavior.

The first P slice may retain explicit COMPILER-PRECISION / resource limits for:

- nested loops;
- changing affine identity correlation;
- rich outer-memory correlation;
- body-sensitive cyclic effects.

Those limits must not become language bans or cause unsound acceptance.

# Targeted research

**NOT REQUIRED.**

All source choices resolve locally from existing grammar/control-context policy and the already-closed semantic model.

# Candidate normative changes

Draft 17.17 changes only the connected source/name surface:

1. Draft 17.17 summary.
2. §4.9 current structural-set cross-reference.
3. §16.1 member-label non-reservation example set.
4. §18.1 ordinary function/parameter reserved-name set.
5. §19.1 block grammar + loop-control source context.
6. §21.8 structural reserved set.
7. §26.10 payload ordinary-name admissibility.
8. §27.1 / §27.1a ordinary binding/receiver admissibility.
9. §27.5 exact loop source grammar / initializer scope/type.
10. §27.6 exact continue source grammar.
11. §27.7 exact break source grammar.
12. §27.8 source-control integration note.

Draft 17.16 cyclic semantics remain intact.

# Explicit non-goals

No:

- while;
- for;
- labels;
- multi-level break/continue;
- bare break;
- implicit continue;
- normal loop-body fall-through;
- mutable loop variables;
- assignment/reassignment;
- source-visible loop invariants;
- source-visible effects/lifetimes;
- general grouping expression;
- new bool/operator syntax;
- production implementation;
- new F;
- new R;
- recursive function SCC;
- generic declarations;
- associated source;
- requires fn;
- callable source redesign;
- nominal source declarations;
- LLVM;
- relocation;
- FFI;
- modules;
- concurrency.

## Disposition

Candidate Draft 17.17 is ready for Coordination review.
