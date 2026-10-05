# NewLang v0 Draft 17.5 review resolution

## Status

**Normative clarification only. Semantic delta: none.**

Draft 17.5 is based on `NewLang_v0_spec_Draft17_4.md`.
It resolves a production-compiler question discovered while preparing P3 semantic checking.

The question was whether the canonical:

```text
take(p, ending)
destroy(p, ending)
```

calls consume the caller's:

```text
ending : exclusive ref<read,LifetimeDomain>
```

binding under the ordinary non-Copy argument rule, or use an operation-local exclusive reborrow.

## R17.5-1 — Exclusive-ref argument use at call / primitive-operation boundaries

### Apparent ambiguity

Draft 17.4 already states both:

1. §12: an `exclusive ref` may be reborrowed into a shorter-lived exclusive child; while the child is live the parent's conflicting use is suspended; after the child ends the parent becomes usable again. The text explicitly says this supports safe sequential reuse of the same lifetime-ending authority.
2. §18.2: an existing non-Copy binding used as an argument is ordinarily consumed / ownership-transferred.

Read independently, §18.2 could be interpreted as consuming the outer `ending` binding at the first `take` / `destroy` call.

That interpretation contradicts the purpose already assigned to exclusive -> exclusive reborrow in §12 and would make sequential lifetime-ending operations with one lexical exclusive loan unnecessarily impossible.

### Resolution

When an **already-selected** callee parameter or primitive operand requires a compatible `exclusive ref<...,T>`, and the actual argument is an existing exclusive-ref binding, argument use applies the §12 exclusive-reborrow rule rather than the ordinary §18.2 non-Copy transfer rule.

Conceptually:

```text
parent exclusive binding E0
    -> operation/call-local child E1

while E1 is live:
    conflicting use of E0 suspended

E1 ends:
    E0 usable again
```

The child receives a fresh hidden scope identity whose extent is no longer than the call / primitive operation.

The parent binding remains available after the child ends.

### Canonical lifecycle consequence

This is valid:

```text
loan exclusive read life as ending {
    take(p1, ending)
    take(p2, ending)
}
```

Each `take` receives a separate operation-local child reborrow.
The first call does not consume the outer `ending`.

The same rule applies to:

```text
destroy(p, ending)
```

### Ordinary transfer remains ordinary transfer

The clarification does **not** change general non-Copy value-use.

For example:

```text
let e2 = ending
```

is an ordinary non-Copy transfer:

```text
ending -> Consumed
e2     -> Available
```

Likewise, `LifetimeDomain`, `Allocation`, `Storage`, and other affine/non-Copy authority values continue to use ordinary argument transfer unless their own semantics explicitly define a reborrow operation.

### Not implicit borrowing

This rule does not introduce:

- `T -> ref<T>` implicit borrowing;
- a general implicit-conversion system;
- overload ranking by conversion;
- copying of exclusive authority;
- a new exclusive write-to-read weakening;
- a source-visible lifetime parameter.

The actual value is already an exclusive-ref capability. The rule only creates the shorter child authority that §12 already permits.

### Compatibility / candidate selection

Exclusive reborrow is applied **after** the callee / primitive operation has been selected and ordinary compatibility is established under existing rules.

The reborrow rule does not make an otherwise-incompatible exclusive-ref type compatible.

## R17.5-2 — Documentation precedence

For argument use, the normative reading is:

```text
existing exclusive-ref binding
+
selected compatible exclusive-ref parameter/operand
    -> §12.1 exclusive reborrow

other ordinary non-Copy argument use
    -> §18.2 consume / ownership transfer
```

Thus §12.1 is the specific rule for this case; §18.2 remains the general rule.

## Implementation consequence

Production P3 should model `take` / `destroy` `ending` as:

```text
caller parent:
    Available before call

operation-local child:
    exclusive
    same underlying LifetimeDomain authority
    fresh child scope identity
    live only for operation extent

during operation:
    parent conflicting use suspended

after operation:
    child ended
    caller parent Available again
```

P3 must not route this operand through ordinary non-Copy binding consumption.

## Preserved Draft 17.4 decisions

Draft 17.5 does not revise:

- `exclusive ref` being non-Copy, Discardable, scope-bound, and non-escaping;
- ordinary non-Copy value transfer;
- `take` / `destroy` lifetime-ending authority requirements;
- `LifetimeDomain` identity semantics;
- ref/write/exclusive separation;
- hidden dependency rules;
- function body / call-boundary non-laundering;
- any Storage, aggregate, sum, relocation, FFI, or backend rule.

## Classification

```text
semantic contradiction:       NO
new semantic mechanism:       NO
normative clarification:      YES
production blocker after fix: NO
Draft-18-level revision:      NO
```

## Judgment

Draft 17.5 should become the normative semantic baseline for P3 and subsequent work.

Its only purpose is to make an already-adopted §12 capability rule explicit at the function / primitive-operation argument boundary so that the implementation does not accidentally consume reusable exclusive authority.
