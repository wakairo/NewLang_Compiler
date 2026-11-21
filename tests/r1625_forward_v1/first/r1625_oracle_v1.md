# R1625 FIRST source-law oracle v1

Authority: own [C1311 control](https://github.com/wakairo/NewLang_Compiler/issues/1625); canonical main43745d03f6f763b5a017e597bbb6825b1f3fadd9, supplied tree e479cfb4b7760973e79a078c0dfbde7eb07fa4f6, Draft17.38. This is the same original R1608/R1620 actor with disclosed prior exposure. Future target/BASE/candidate NOT_RELEASED. This document selects no new language rule and predicts no implementation outcome.

## Canonical basis

Compass navigation and Value1.1/1.3 separate reusable selected semantics from comparative product evidence. Process7 requires independent oracle preservation and finite failure reporting. [Draft17.38](https://github.com/wakairo/NewLang_Compiler/blob/43745d03f6f763b5a017e597bbb6825b1f3fadd9/docs/reference/NewLang_v0_spec_Draft17_38.md) clauses used:

- Sections4.2,4.4,4.7 and27.2: scalar bool and integer Copy uses preserve availability; Copy implies Discardable. A copied value can be used or discarded without consuming the original binding.
- Sections5.1,5.2: bool is its own type; integer comparison produces bool. No usize truthiness or numeric conversion supplies a bool argument.
- Sections5.4 and6: typed usize literals are range checked; division/remainder require a nonzero denominator. Quotient is floor division and remainder is Euclidean. Numerator zero is lawful.
- Sections7.1,18.1,18.2: evaluate actual operands completely in source order before invocation; correspond actual position to declared formal position/type; initialize fresh formal bindings for each invocation. An unused formal does not cancel evaluation/preconditions of its actual.
- Section13.5c: known-call meaning uses actuals and genuine body semantics, with invocation-appropriate identities/substitution. This is no mandate to inline text, expose a fresh integer ID, allocate a fresh pointer, or retain one runtime operation for each Copy use.
- Sections18.5,19.1: a helper's tail expression supplies its result. A semicolon discards an expression result when Discardable; an earlier discarded value is not the function result.
- Section27.4: the condition has bool type and is evaluated once; exactly one arm executes. Every normally completing arm must have the same result type, including an arm not selected for a particular runtime value.
- Sections18.1,28.1/28.2: same-unit ordinary helper declarations and explicit parameter/result types; source order of top-level declarations does not require moving the helper above main.

Exact inspected ranges and document hashes are in READ_EXPOSURE_v1.json. These are source-law conclusions, not observations of a new compiler.

## Immutable original six

The eight tests/r1608/first payloads in this packet are byte-for-byte copies of the frozen original six sources, r1608_oracle.md and SHA256SUMS. Their own oracle remains controlling for their original claims.

| Case | Source-law expectation | Typed actual/formal and quantity |
|---|---|---|
| P1 main_zero_then | Lawful | No helper arguments. Named Copy numerator0, true condition, selected0/7=0; hypothetical remainder0. main returns unit. |
| P2 helper_else | Lawful | actual0 seed:usize -> input:usize; actual1 flag:bool -> take_quotient:bool. 53,false selects53%7=4; quotient7 is hypothetical. |
| C1 selected_div_zero | Invalid | Selected53/0 violates nonzero denominator; no normal quantity assigned. |
| C2 selected_rem_zero | Invalid | Selected53%0 violates nonzero denominator; no normal quantity assigned. |
| C3 unselected_unit_arm | Invalid | Normal then usize versus normal else unit; a true condition does not erase the static arm-type conflict. |
| C4 hoisted_alternative | Lawful alternative | Same usize,bool formal order as P2. Both53/7=7 and53%7=4 occur before the IF; false selects4. This source is not substituted for P2. |

The original R1608 six-lane P1 emission STOP, later NOT_RUN cases, formatting RED, and raw records remain unchanged. The later R1620 record remains separate: P1 reached native process exit0; P2/C4 retained CHECK_OK emission4 NONPASS with backend health UNPROVED and native NOT_RUN; C1-C3 retained original semantic rejection and matching CLI3 diagnostics whose intended law reason was untyped. These old observations do not supply a new-target result. Preserve the [R1608 FINAL](https://github.com/wakairo/NewLang_Compiler/issues/1608#issuecomment-7061764412) and [R1620 FINAL](https://github.com/wakairo/NewLang_Compiler/issues/1620#issuecomment-7062788552).

## Supplement S1: bool-first, two distinct invocations

Source: r1625_s1_bool_first_two_calls_v1.nl. This separately named lawful control fills two concrete gaps: reversed mixed formal order and two invocations of one body with different actual values.

| Invocation | Actual position0 -> formal0 | Actual position1 -> formal1 | Selected source quantity |
|---|---|---|---|
| first | first_flag:bool=true -> take_quotient:bool | first_input:usize=43 -> input:usize | quotient43/7=6 |
| second | second_flag:bool=false -> take_quotient:bool | second_input:usize=26 -> input:usize | remainder26%7=5 |

The first unselected remainder is1; the second unselected quotient is3. These are hypothetical arm arithmetic quantities, not evidence that both arms execute. The selected helper results are6 and5; main's result is unit. Existing Copy bindings remain available when later named/discarded.

For each call the bool actual is evaluated before the usize actual. Each formal binding belongs to that invocation; flag_copy and copied use the corresponding typed values. Repeated uses and repeated invocations must not substitute a stale43 for26, a prior true flag forfalse, or the usize argument for the bool formal. Both normal arms return usize and use typed nonzero divisor7. The flag_copy/copy expression statements after the IF discard copied scalar results; result_copy is the helper tail result.

Fresh semantic per-use/invocation correspondence is required where the language demands it. Public numeric IDs/pointers are qualified display handles, not values or a specified unique-identity representation. Equal scalar values, shared immutable body fragments or repeated local IDs alone prove neither a binding error nor correctness. Source spans, declared types/order, call-site association and genuine body/world context must accompany any public inference.

## Supplement S2: unused usize actual still evaluated

Source: r1625_s2_unused_usize_actual_zero_v1.nl. The declared order is bool,usize: actual0 flag:bool=false -> take_quotient:bool; actual1 usize(9)/usize(0):usize -> ignored:usize. The second formal is deliberately unused. The helper definition itself is lawful: copied29, both normal arms usize, divisor7, tail selected.

The whole source is invalid because actual1 division has a zero denominator. Discardability of the formal and later discard of value cannot erase actual evaluation or its precondition. No normal invocation result or runtime selected quantity is assigned to this invalid source. The unused formal is not itself a source error. A compiler/parser unsupported refusal is not evidence for the intended law reason. Expected rejection is semantic, without a promised diagnostic spelling/code beyond a separately observed public contract.

Original admission of this invalid source is an unconditional STOP before adapter rescue or emission/native. CLI CHECK0 is also an unconditional unsafe-admission STOP. A nonzero result alone does not prove the intended reason; retain statuses and rendered diagnostic bytes and keep the reason unproved if public typing is insufficient.

## Arrival layers and existing observation limits

For every case at the future target: SOURCE/PARSE NOT_RUN is the earliest unproved arrival; then original registration/admission or rejection, adapter association/check, public graph/quantity evidence, CLI CHECK, emission, C compilation, native process. Untouched stages remain NOT_RUN. Canonical lawful/invalid expectations never upgrade these execution fields.

The immutable observer first records the original function unit's parse/register result, separately from the exact7-byte main() newline adapter. It records genuine original/body/adapter sources and spans, node type and argument edges, qualified local fragment/world/value handles, scalar-known flags, genuine body-backed calls, and then/else hypothetical fragments. Its arithmetic audit records original, copied and wrong/foreign candidate validations. These are finite public observations.

It does not print a complete declared formal-position/type list or a typed actual-to-formal substitution certificate. It deduplicates repeated fragment pointers. It does not establish whole-call custody, runtime IF selection, internal runtime arithmetic values/order, or a world theorem. Native unit exit0 establishes only that bounded emitted process completed with that status. Pure internal arithmetic may be unobservable or optimized; arithmetic agreement and native exit are not a runtime value/order proof.

The accepted driver assesses every arithmetic record against one numerator per case. Reusing that classifier unchanged for S1 would conflate43 and26. No source is rewritten to fit it. If a future public-only apparatus cannot expose the necessary call-site/type/source/body/world correspondence, report PUBLIC_OBSERVABILITY_BOUNDARY / NOT_ESTABLISHED and STOP C. No new public source, issuer, world, schema, guarantee or unique-ID rule is presumed.

All source bytes are UTF-8 without BOM, LF line endings and one final LF. Sources, this oracle and the original oracle are separately hashed in the full publication manifest.
