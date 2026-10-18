# Track R #592 pre-candidate frozen independent BLIND oracle

Fixed baseline=12c2c8e3338201ec3bda00a63ddac841ed80e574; compiler candidate=54986f2b227fc4999204027ee54fc50c94b546e9; canonical Draft17.32. Source independently authored from #592 fixed source contract and canonical, before any PR590 description, diff, internal code, P/M reports or CI details. This commit is initial FROZEN test intent and should not be rewritten to fit candidate. Registered function source (not source-only fragment) for actual issuer/validator/C17.

## Source cases and phase oracle

### P01_literal_23
Prediction: CHECK_PASS; issued owned; native 5

```newlang
fn main() -> unit {
    let result = usize(2) + usize(3);
    unit
}
```

### P02_distinct_names
Prediction: CHECK_PASS; issued owned; native 96; preserve alpha/beta distinct original identities

```newlang
fn main() -> unit {
    let alpha = usize(37);
    let beta = usize(59);
    let answer = alpha + beta;
    unit
}
```

### P03_zero
Prediction: CHECK_PASS; issued owned; native 0

```newlang
fn main() -> unit {
    let a = usize(0);
    let b = usize(0);
    let result = a + b;
    unit
}
```

### P04_near_max_64
Prediction: CHECK_PASS only on target 64-bit usize, result SIZE_MAX-1; target guard must retain

```newlang
fn main() -> unit {
    let large = usize(18446744073709551608);
    let small = usize(6);
    let result = large + small;
    unit
}
```

### N01_overflow_64
Prediction: SOURCE accept; CHECK numeric overflow refuse, no C generated, only on target with 64-bit usize

```newlang
fn main() -> unit {
    let first = usize(18446744073709551615);
    let second = usize(1);
    let result = first + second;
    unit
}
```

### N02_literal_out_of_range_64
Prediction: SOURCE accept typed digits; literal range refusal BEFORE addition checking

```newlang
fn main() -> unit {
    let value = usize(18446744073709551616);
    let result = value + usize(0);
    unit
}
```

### N03_type_u8_plus_usize
Prediction: SOURCE accept; semantic TYPE error, no implicit conversion

```newlang
fn main() -> unit {
    let result = u8(7) + usize(1);
    unit
}
```

### N04_unknown_formal
Prediction: SOURCE accepts one flat binary; semantic PRECISION refusal (unless provable by actual known-call definition), no C

```newlang
fn add_one(value: usize) -> usize {
    let answer = value + usize(1);
    answer
}
fn main() -> unit {
    unit
}
```

### N05_chain
Prediction: SOURCE refusal

```newlang
fn main() -> unit {
    let answer = usize(1) + usize(2) + usize(3);
    unit
}
```

### N06_parenthesized
Prediction: SOURCE refusal

```newlang
fn main() -> unit {
    let answer = (usize(1) + usize(2));
    unit
}
```

### N07_mixed_comparison
Prediction: SOURCE refusal

```newlang
fn main() -> unit {
    let answer = usize(1) + usize(2) == usize(3);
    unit
}
```

### N08_bare_literal
Prediction: SOURCE refusal

```newlang
fn main() -> unit {
    let answer = 2 + usize(3);
    unit
}
```

### E01_existing_equality
Prediction: unchanged old equality true and false, no new addition privilege

```newlang
fn main() -> unit {
    let left = usize(13);
    let same = usize(13);
    let different = usize(14);
    let eq_true = left == same;
    let eq_false = left == different;
    unit
}
```

## FROZEN public certificate attack oracle
Use only real source-issued *registered-function* owned proof. After obtaining passing baseline: (a) copied public view swap L/R IDs while values differ; (b) mutate result / source operator span / full span; (c) change operand type or ValueFact or exact source operand identities; (d) stale place/incarnation or fake dependencies; (e) foreign source function with identical numbers or colliding numeric ID; (f) synchronized operand + result value edits; (g) false overflow flag; (h) genuine owned proof if publicly mutable, else NOT EXERCISED. Expected: every reachable modification rejected by *public* validator and, where exposed, C17 consumer; C-output sentinel remains intact, no partial emitted bytes. Baseline must still pass after each mutation. Copying a view is never evidence of actual certified issuer ability. No externally constructed host scalar roots.

## FROZEN native C17 observer oracle
Strict C17 GCC/Clang -std=c17 -Wall -Wextra -Werror at -O0 and -O2 with no sanitizer relaxation. During original guest function live range passive instrument generated C (no guest mutation): assert distinct named operand carriers 37 & 59, result 96, lexical source order and genuinely computed operands at site (not host substituted const 96). Literal 2+3=5, 0+0=0 and near-max safe if 64-bit. Check SIZE_MAX and exact 8-bit model guards in artifact. Independently mutate *separate observer copy* guest C operation to a wrong constant or wrong operand; observer must fail. No claim target beyond observed native target.

## FROZEN failure/rollback oracle
Bounded fail indices at parse/registration/issuance/validator/codegen where actual allocator and public API expose injection. On failed call no partial source-owned proof or changed output slot, no partial C text / unchanged sentinel. Record unexercisable and exact index rather than manufacturing expected PASS. Stop at first suspect checker/proof/native mismatch. First actual BLIND outcome must be preserved verbatim before inspecting PR590 developer evidence.

## Fixed bounds and separation
90-150m max; <=1 R-only DRAFT evidence PR (if CI needed), <=2 R-specific Actions attempts; no P repair, no PR590 edits, no main merge, no canonical changes, no reports from #591/#593 or P/M pre-freeze. First verification is fixed candidate only. Independent source is not allowed to be altered after this commit; add subsequent harness as separate files/commits with clear provenance.
