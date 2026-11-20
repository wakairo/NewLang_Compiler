# FIRST/V1 FREEZE — R C1285 / Issue #1562 — NOT_RUN

**Bounded cohort: 2 positive complete units +4 negative complete units.** These are this actor's original source bytes and independently derived expectations, frozen before implementation interfaces, compiler/project execution, public issuance, C/native observation or any target-candidate discovery.

## Fixed authority and chronology

- Canonical Compiler/main: `be0ae7fa537b550ffdb8ad3640f49a504629062a`; tree fixed by the own Issue: `59142dcd7fa08820d57b7f17f870f1ed915a5e37`.
- [Compass](https://github.com/wakairo/NewLang_Compiler/blob/be0ae7fa537b550ffdb8ad3640f49a504629062a/docs/NewLang_Project_Compass.md), blob `0f9b170ce26504a16acec8e4b84beab11f9fbd4e`: navigation only.
- [Actual Value Strategy](https://github.com/wakairo/NewLang_Compiler/blob/be0ae7fa537b550ffdb8ad3640f49a504629062a/docs/NewLang_Product_Value_Strategy.md) §§1.1/1.3, blob `c75f1601f13070bc5a588bd78957b54662bdfd23`; [actual Process](https://github.com/wakairo/NewLang_Compiler/blob/be0ae7fa537b550ffdb8ad3640f49a504629062a/docs/NewLang_Project_Development_Process.md) §7, blob `92ecb166b5c2e4c4cbd8ebf2aba18e9d528c6d69`.
- [Actual CURRENT_SPEC](https://github.com/wakairo/NewLang_Compiler/blob/be0ae7fa537b550ffdb8ad3640f49a504629062a/docs/reference/CURRENT_SPEC.md), blob `5ff1e4a460848472e8ef832168e6f25d9c7b65e0`, selects [Draft17.38](https://github.com/wakairo/NewLang_Compiler/blob/be0ae7fa537b550ffdb8ad3640f49a504629062a/docs/reference/NewLang_v0_spec_Draft17_38.md), blob `25951270752c9e6499a1dcefc99ab7b17115bcee`.
- [ACTUAL START / exposure](https://github.com/wakairo/NewLang_Compiler/issues/1562#issuecomment-7032224447), individually fetched: nonempty and full equality verified.
- [Independent questions / canonical exposure](https://github.com/wakairo/NewLang_Compiler/issues/1562#issuecomment-7032251788), individually fetched: nonempty and full equality verified. Additional narrow canonical read after that comment: Draft §5.6 numeric conversion and §21.9 no implicit-conversion candidate ranking.

This remains **C-informed source-first independence**, not unrestricted blindness. Full canonical payloads were fetched in memory for section extraction; the model-read content is the documented Compass, Value sections, Process §7, full CURRENT_SPEC, selected Draft heading index and the relevant Draft sections. Historical actor links/examples incidentally present in those canonical documents were not followed or copied into this cohort. Names, values, case structure and oracle here were independently authored. No producer/candidate source, interface/header, fixture, runner, report, PR, other actor original, aggregate C, shared scratch or actor inventory was read. The fixed compiler tree is stated authority supplied by the own Issue, not an implementation-tree review.

## Exact source-byte contract and manifest

Each source code block below is the **complete unit**. Bytes are UTF-8 without BOM, all source characters are ASCII, each newline is one LF (`0A`), indentation is four ASCII spaces, blank lines are empty, and **exactly one LF follows the final closing brace**. Code-fence delimiters and headings are excluded. Each unit is 21 LF-terminated lines. The displayed complete bytes, not a future file reconstruction or a mutation recipe, are FIRST.

SHA256 was calculated only by administrative text hashing; no source was parsed, compiled or executed. The administrative hash function was checked against the standard empty-string and `abc` SHA256 values. Total source bytes across the six separately named units: **2841**.

| ID | Source identity label | UTF-8 bytes | Lines | SHA256 |
| --- | --- | ---: | ---: | --- |
| P1 | `r1562_first_p1_division_chain.nl` | 459 | 21 | `20c976aa377797cf230ce5793c4cd0992e076a66a05495cb795d6efaac888742` |
| P2 | `r1562_first_p2_equal_value_distinct_origin.nl` | 507 | 21 | `e3d863263c0dd8518519ea6231244d8676d1b27e1716bf2ccfd2a251e55f202b` |
| N1 | `r1562_first_n1_zero_division.nl` | 459 | 21 | `2bf51ab6f7f327a22d960abb17fe0f11ddb82c281e33598b32aedddd3b41ea0f` |
| N2 | `r1562_first_n2_zero_remainder.nl` | 507 | 21 | `f20498c5be7ca749088c0455bf75b5171748647c5d05869c87359d436836d5b2` |
| N3 | `r1562_first_n3_actual_type.nl` | 456 | 21 | `ef8a7763816695ace7400f0dca72740f85edd0b65df3b0a65857b2a87ef8302a` |
| N4 | `r1562_first_n4_return_type.nl` | 453 | 21 | `af1dca95315308fe990b2c324502f4d45fef3fc4007fb50455db07a243620100` |

The independent oracle is also text-frozen below: exact UTF-8 substring beginning at `## Independent pre-execution oracle — FIRST/V1` and ending at the end of this comment, **without a terminal LF**, is **10729 bytes**, SHA256 **`59d63e02a169e4fecd8e2916207c900d8757130d3355b069ee894733a7bb59b9`**. This hash covers the oracle and future-observation plan, not these metadata paragraphs.

## Complete sources

### P1 — r1562_first_p1_division_chain.nl

```text
fn main() -> unit {
    let load = usize(53);
    let first = sift_five(load);
    let saved = first;
    let second = pack_three(saved);
    let repeated = pack_three(first);
    let load_copy = load;
    let second_copy = second;
    unit
}

fn pack_three(parcel: usize) -> usize {
    let chunk = parcel / usize(3);
    let chunk_copy = chunk;
    return chunk_copy;
}

fn sift_five(parcel: usize) -> usize {
    let chunk = parcel / usize(5);
    chunk
}
```

### P2 — r1562_first_p2_equal_value_distinct_origin.nl

```text
fn residue_six(amount: usize) -> usize {
    let rest = amount % usize(6);
    let rest_copy = rest;
    rest_copy
}

fn main() -> unit {
    let cargo = usize(38);
    let quotient = split_seventeen(cargo);
    let residual = residue_six(cargo);
    let forwarded = residue_six(quotient);
    let quotient_copy = quotient;
    let residual_copy = residual;
    let forwarded_copy = forwarded;
    unit
}

fn split_seventeen(amount: usize) -> usize {
    let share = amount / usize(17);
    return share;
}
```

### N1 — r1562_first_n1_zero_division.nl

```text
fn main() -> unit {
    let load = usize(53);
    let first = sift_five(load);
    let saved = first;
    let second = pack_three(saved);
    let repeated = pack_three(first);
    let load_copy = load;
    let second_copy = second;
    unit
}

fn pack_three(parcel: usize) -> usize {
    let chunk = parcel / usize(3);
    let chunk_copy = chunk;
    return chunk_copy;
}

fn sift_five(parcel: usize) -> usize {
    let chunk = parcel / usize(0);
    chunk
}
```

### N2 — r1562_first_n2_zero_remainder.nl

```text
fn residue_six(amount: usize) -> usize {
    let rest = amount % usize(0);
    let rest_copy = rest;
    rest_copy
}

fn main() -> unit {
    let cargo = usize(38);
    let quotient = split_seventeen(cargo);
    let residual = residue_six(cargo);
    let forwarded = residue_six(quotient);
    let quotient_copy = quotient;
    let residual_copy = residual;
    let forwarded_copy = forwarded;
    unit
}

fn split_seventeen(amount: usize) -> usize {
    let share = amount / usize(17);
    return share;
}
```

### N3 — r1562_first_n3_actual_type.nl

```text
fn main() -> unit {
    let load = u8(53);
    let first = sift_five(load);
    let saved = first;
    let second = pack_three(saved);
    let repeated = pack_three(first);
    let load_copy = load;
    let second_copy = second;
    unit
}

fn pack_three(parcel: usize) -> usize {
    let chunk = parcel / usize(3);
    let chunk_copy = chunk;
    return chunk_copy;
}

fn sift_five(parcel: usize) -> usize {
    let chunk = parcel / usize(5);
    chunk
}
```

### N4 — r1562_first_n4_return_type.nl

```text
fn main() -> unit {
    let load = usize(53);
    let first = sift_five(load);
    let saved = first;
    let second = pack_three(saved);
    let repeated = pack_three(first);
    let load_copy = load;
    let second_copy = second;
    unit
}

fn pack_three(parcel: usize) -> usize {
    let chunk = parcel / usize(3);
    let chunk_copy = chunk;
    return unit;
}

fn sift_five(parcel: usize) -> usize {
    let chunk = parcel / usize(5);
    chunk
}
```

## Independent pre-execution oracle — FIRST/V1

### Authority, source legality and target fact

The own Issue declares the already-selected one-flat existing-name / or % typed-literal family. The fixed Draft supplies ordinary declaration visibility/signatures (§18.1), Copy/value-use (§§4.2/4.4/4.7), argument→fresh parameter transfer (§18.2), normal and explicit results (§§18.5/19.1/27.9), typed literal range checking (§6), unsigned quantity arithmetic (§§5.1/5.2/5.4), explicit numeric conversion (§5.6), and source-order evaluation (§7.1). Main and both helper declarations belong to one complete physical input / semantic unit (§28.1); physical declaration order is not the function visibility rule (§§18.1/28.2).

The cohort requires a later released target with **USIZE_MAX >= 53**. This is an explicit premise of this source cohort, **not** an inferred minimum width in the language. P1's largest literal is 53; P2's is 38. Every positive divisor (3, 5, 6, 17) is nonzero and representable under that premise. No execution target is known or certified here. If the later target does not satisfy the premise, preserve FIRST, report the literal-range boundary, and do not count that as the intended composition/zero/type measurement.

For any representable usize formal a and representable constant b>0, q=floor(a/b) satisfies 0<=q<=a, and r=a-q*b satisfies 0<=r<b. Thus the helpers have no additional overflow or hidden caller precondition. This mathematical derivation is an oracle, not a claim that any uninspected implementation proves it. There is no unchecked, heap, nested helper graph, dynamic divisor, recursion, branch, or loop.

P1 and P2 are **positive semantic/source expectations**, with expected successful semantic CHECK on an implementation admitting the selected source/profile. Present implementation acceptance, public-owned eligibility and C17/native support are **UNKNOWN / NOT_RUN**. In particular, this preparation does not adopt remainder or composition support. A lawful source rejected by a future parser/backend/profile must be classified at that actual boundary; it is not converted into a semantic-error negative.

### P1 — distinct division leaves, retained result and repeated caller use

Expected declaration associations (analyst labels only, not compiler identity values):
- P1-M: main, lines 1–10, () -> unit.
- P1-T: pack_three, lines 12–16, (parcel: usize) -> usize; division at line 13; post-operation Copy at line 14; final explicit return at line 15.
- P1-F: sift_five, lines 18–21, (parcel: usize) -> usize; division at line 19; normal tail at line 20.

All signatures must be available before resolving main. The two same-spelled formal names parcel belong to **different helper declarations**; repeated invocations create their own fresh formal bindings.

| Call occurrence | Actual binding/value | Exact callee and fresh formal | Exact operation result | Result path | Caller result |
| --- | --- | --- | --- | --- | --- |
| P1 line 3 | load = 53 | P1-F parcel = 53 | 53 / 5 = 10 | chunk -> normal tail | first = 10 |
| P1 line 5 | saved = 10 (Copy of first) | P1-T parcel = 10 | 10 / 3 = 3 | chunk -> chunk_copy -> final return | second = 3 |
| P1 line 6 | first = 10 (still Available) | P1-T new parcel = 10 | 10 / 3 = 3 | this call's chunk -> chunk_copy -> final return | repeated = 3 |

Expected completed-main bindings: load=53, first=10, saved=10, second=3, repeated=3, load_copy=53, second_copy=3. Main's normal result is unit. load remains usable after line 3; first remains usable after line 4 and after forwarding the copied result; second remains usable at line 8. Copy is value-use, not an explicit new Copy syntax or a move/consume of these integers.

The equal values of second and repeated do not permit substituting line 5's call/formal/result evidence for line 6's. P1-F and P1-T are distinct declared leaves with different operations and result paths; neither calls another helper.

### P2 — division and remainder with deliberately equal answers

Expected declaration associations:
- P2-R: residue_six, lines 1–5, (amount: usize) -> usize; remainder at line 2; post-operation Copy at line 3; normal tail at line 4.
- P2-M: main, lines 7–16, () -> unit.
- P2-D: split_seventeen, lines 18–21, (amount: usize) -> usize; division at line 19; final explicit return at line 20.

| Call occurrence | Actual binding/value | Exact callee and fresh formal | Exact operation result | Result path | Caller result |
| --- | --- | --- | --- | --- | --- |
| P2 line 9 | cargo = 38 | P2-D amount = 38 | 38 / 17 = 2 | share -> final return | quotient = 2 |
| P2 line 10 | cargo = 38 (still Available) | P2-R amount = 38 | 38 % 6 = 2 | rest -> rest_copy -> normal tail | residual = 2 |
| P2 line 11 | quotient = 2 | P2-R new amount = 2 | 2 % 6 = 2 | this call's rest -> rest_copy -> normal tail | forwarded = 2 |

Expected completed-main bindings: cargo=38; quotient, residual, forwarded, quotient_copy, residual_copy and forwarded_copy all equal 2. Main's normal result is unit. The forwarded input is the real quotient result from line 9, not cargo and not the independently equal residual from line 10.

This is a deliberate same-value/different-origin probe. Numeric equality is insufficient: P2-D's division result cannot stand in for P2-R's remainder, and P2-R's two invocations must retain the distinct actuals 38 and 2. Source-order calls are lines 9, 10, then 11; later copies do not rewrite that chronology. Correct source association and authentic evidence are separate from arithmetic equality.

### Negative complete units and earliest intended rejection

Each negative is a separate complete unit below. It changes exactly one source occurrence from its stated positive parent. They are intentionally source/type/precondition-negative controls, not claims that a backend limitation is a language error.

| Unit | Exact isolated change | Independent prediction | Earliest intended semantic layer |
| --- | --- | --- | --- |
| N1 from P1 | sift_five line 19 divisor usize(5) -> usize(0) | REJECT: the division's b != 0 precondition is false for every actual, including load=53 | Definition/body operation applicability / semantic CHECK; grammar, name and numeric types remain valid |
| N2 from P2 | residue_six line 2 divisor usize(6) -> usize(0) | REJECT: the remainder's b != 0 precondition is false, including actuals 38 and 2 | Definition/body operation applicability / semantic CHECK; grammar, name and numeric types remain valid |
| N3 from P1 | main line 2 usize(53) -> u8(53) | REJECT: line 3 supplies a core u8 value to a usize parameter; no implicit cross-type coercion was selected | Call/argument static typing, before successful whole-unit CHECK |
| N4 from P1 | pack_three line 15 return chunk_copy; -> return unit; | REJECT: the explicit returned expression has unit type while the function result is usize | Function result static typing / definition check |

These categories identify the first **intended language obligation**, not an unobserved compiler diagnostic code or promised traversal order. Under an unsupported future frontend/profile, an earlier source eligibility rejection leaves the intended semantic negative **NOT TESTED**. No output values, authentic successful whole-unit proof, or successful native run are expected for a semantically rejected complete unit. Some separately valid body must not be promoted into certification of the invalid whole input.

### Distinct evidence and future observation plan

The line-based labels above are source anchors for later comparison only. They do not define or replace authentic compiler function, operation, call, formal, result, body, context, issuer or source-unit identities. Later evidence must come from genuine registration/checking of these **whole immutable units**, selecting their actual main entry, with all real helper bodies and their defining contexts retained for the public path. The six different full-source hashes distinguish the unit variants even where names and unchanged body text coincide.

If separately released later, record the exact public candidate/base/tree, runtime target range, source hash, public entry path and actual diagnostics. Retain actual public bodies/contexts and map each authenticated declaration and call to its exact source occurrence. Check actual→fresh formal→that function's operation→that exit→that caller binding. Reject an argument that an equal answer, common formal spelling or identical-looking arithmetic permits exchanging evidence between callees, calls, units or contexts. No public schema, interface symbol, output-variable naming convention or producer implementation behavior is presumed now.

Keep **unmodified emitted C** as its own immutable artifact, and separately record its authentic compile/native outcome. A normal unit-returning main and an exit status alone do not expose all internal values or prove the provenance questions. If the released interfaces permit it, prepare a **separately labelled read-only observation** of already existing real emitted values at their actual execution points. Derive any emitter/source mapping from authentic retained output then; do not prescribe producer C names, synthesize a parallel guest calculation, replace main with independent leaf calls, seed answers/guest state, fabricate operations/results, or rewrite an oracle to match observations. If original values or provenance cannot be observed faithfully, mark that component NOT OBSERVED / INCONCLUSIVE.

The original unmodified-C/native result and any read-only observed derivative are distinct evidence. A future compiler rejection, observation limitation or discrepancy must be retained at its earliest actual layer; no later instrumentation or calibration overwrites these FIRST sources, predictions or original findings.

### Current status and boundaries

All tables are pre-execution predictions derived independently from the allowed canonical and declared scope. **SOURCE execution=0; compiler/native/Lean/build/project/SDK/toolchain/ref/branch/PR/push/CI=0; public-owned issuance/validation=0. NOT_RUN throughout.** Only administrative text generation, byte counting/SHA256 and own-Issue operations occurred.

Exposure remains **C-informed source-first**, with the incidental canonical-document exposure already recorded in the independent-questions comment. No forbidden original was opened, no candidate was searched/inferred, no designer case exchange occurred, and no implementation interface was needed. Fixed-main policy/spec finite evidence cannot establish general soundness, Product GO, or C-comparative author+ordinary-safe-caller audit/re-audit locality.