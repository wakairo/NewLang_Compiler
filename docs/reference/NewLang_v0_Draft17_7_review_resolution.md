# Draft 17.7 — R1 targeted closure resolution

## Scope

This resolution records the adjudication of the independent R1 first-pass in Issue #11 and the targeted reopen in Issue #12.

Draft 17.7 is intentionally small. It does not redesign NewLang's lifetime, dependency, occurrence, relocation, or raw-byte model.

## Accepted findings

### R1-01 — exact split partition

Accepted as a core semantic gap.

Draft 17.6 required same-BackingRegion disjoint results but did not normatively state that the results exactly cover the consumed source range. Draft 17.7 defines the exact range equations and conservation law.

### R1-02 — zero-length Storage

Accepted as a core semantic gap.

For v0, `Storage` claims are now explicitly non-empty. `split` therefore requires an interior cut. A zero-size allocator request is a library/platform surface concern and must not be represented by minting a zero-length `Storage`.

This deliberately chooses the smaller affine model rather than adding empty-claim multiplicity / merge semantics.

### R1-03 — initialize write access

Accepted as a core semantic gap.

Starting a typed lifetime from `slot<T>` establishes a `T` representation in the destination backing. Ordinary-safe `initialize` therefore requires the target-defined destination write access needed for that representation.

This rule is semantic and is not removed by an optimization that happens to elide a physical store.

The merged P4 production checker requires targeted revalidation because explicit read-only BackingRegion + slot + initialize was not previously rejected.

### R1-04 — implicit local stability evidence

Accepted as a connection gap between §10.1 and §13.7.

The explicit `ref_from_ptr(p, stable)` shape remains the representative explicit-domain form. For compiler-managed lexical locals, the existing implicit local stability loan may satisfy the same semantic evidence obligation without materializing a programmer-visible `LifetimeDomain` value/ref.

This does not allow ptr-only reacquisition: current incarnation + governing-identity correspondence must still be proven.

### R1-05 — zero-length span backing

Accepted as a minor semantic ambiguity.

A zero-length span covers no live object and no backing byte range, so v0 does not require a BackingRegion identity, anchor pointer, dummy object, or dummy allocation solely to represent it.

Non-empty spans retain the single-live-BackingRegion rule.

## Findings kept without semantic change

### R1-06 — type-wide sum Discardable

No change.

`Discardable(Sum)` remains a static type-wide property. A compiler-known payloadless current variant does not make `store` / `destroy` variant-sensitive in v0. Programs that need to recover and handle the old value can use `replace`.

This is an intentional conservative source restriction, not a soundness gap.

### R1-07 — compiler proof precision

No change.

The canonical semantics define legality, while the v0 compiler may conservatively reject when permitted widening or missing proof precision prevents establishing legality. Draft 17.7 does not define a mandatory minimum theorem-proving precision.

This remains a compiler-quality / portability concern to track separately from language unsoundness.

## Downstream revalidation

After Draft 17.7 review/merge:

1. production P4 must add a read-only-backing initialize negative test and reject it;
2. exact split behavior already implemented by P4 should be rechecked against the now-canonical rule;
3. formal evidence should be revalidated only where these clauses intersect its modeled scope;
4. R1 may perform targeted post-fix revalidation with access to the adjudication evidence.

M9 / F1.3 / P5 remain waiting until this bounded correction closes.
