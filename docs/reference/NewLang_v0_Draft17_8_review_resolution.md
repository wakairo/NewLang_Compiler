# Draft 17.8 — R1-03A targeted closure resolution

## Scope

This resolution closes the sole material residual found by the R1 post-fix targeted revalidation in Issue #15 and adjudicated in Issue #16.

Draft 17.8 changes no ownership, lifetime, occurrence, Storage, span, or dependency mechanism. It only makes the access requirement of `take` explicit and distinguishes it from `destroy`.

## R1-03A

A write-only explicit BackingRegion may legally support:

```text
Storage -> slot<T> -> initialize(...)
```

because initialization needs destination write access.

The returned ptr therefore may carry:

```text
readable = false
writable = true
```

Allowing:

```text
take(ptr, ending) -> (T, slot<T>)
```

from that ptr would expose the current `T` value to ordinary value flow despite the backing denying ordinary read access. This is an access amplification path.

Draft 17.8 therefore states that ordinary-safe `take` requires ordinary read access to the source current value. For explicit backing, both the backing access property and ptr access evidence must permit read.

## Why destroy differs

`destroy` does not return/materialize the old semantic value. Its conceptual “take + discard” description does not make it a literal applicability composition.

Therefore ordinary native `destroy` does not inherit `take`'s read requirement solely because of that conceptual equation. A target/platform extension may still impose additional access for lifetime end.

## Production follow-up

After this candidate becomes canonical:

- reject write-only `take`;
- preserve write-only `destroy` where other preconditions hold;
- add direct targeted tests for both;
- synchronize endpoint `split` diagnostics with Draft 17.7/17.8 by reporting invalid semantic input rather than deferred/unsupported behavior.

No FormalProof change is required by this clarification because current F1.2 does not formalize BackingRegion access properties.
