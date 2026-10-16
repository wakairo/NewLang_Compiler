# R516 independent first-pass pre-execution evidence freeze

Track: R — fresh BLIND. Frozen without reading P #514, PR #515 content, tests or candidate implementation.

## Fixed authority

- Canonical compiler/main: `9e6389388a44950d17e19bb134cbfde37ac75f97`.
- Canonical specification: `docs/reference/CURRENT_SPEC.md` at fixed main, Draft 17.30.
- Opaque compiler candidate (non-merged): `53b6883664f5f585ddd095ab357bc80e1e4018af`.
- FormalProof is not a candidate proof; no formal conclusions imported.
- Reference: Process §7.1–7.3, §7.6, Compass navigation, Product Strategy §1.1.
- Exactly one original live Pair aggregate named `original`, created before the if.
- Do not infer separate tested roots or allocation/lifetime rights from identical numeric values.

## Frozen source SHA256 (LF UTF-8; cryptographic digests fixed before execution)

```text
c921596f72e64116630676b98d6ce43c529420a4bec26c437d01981cc39cc864  true.nl
0dfc413cfc63fa379b6b4c1d20a6beea1421188add4e05b9d1bc660fc0ca1664  false.nl
d9f6c09fdb63ba50e5beaeb26b6ada585cfe67c28883a53c53c7504ce6c0c272  reject_arm.nl
```

## Oracle committed *before* target invocation

- Both original roots/fields and one scoped write are source-created; host never supplies guest values.
- TRUE: distinct source `usize` bindings 29 == 29; selected left replacement **108**, returned former original.left **37**, post original.left **108**, unchanged original.right **211**.
- FALSE: distinct source `usize` bindings 29 != 30; selected left replacement **173**, returned former original.left **37**, post original.left **173**, unchanged original.right **211**.
- The originals existed before evaluation of `if`, stay the same live root and same fixed left/right field incarnations after one `replace`; no second write and no escaping write ref. `Change` modifies current semantic value, not root incarnation.
- TRUE and FALSE sources differ only in right-side `usize` operand literal. Both arms require static checking even where runtime condition is statically obvious.
- Negative `reject_arm.nl`: true operand world but ELSE contains `u8(256)` (out of u8 range). Expected source can parse, semantic CHECK rejects before emission; unreachable branch must not bypass semantic validation.
- No arbitrary nonCopy or Array/general effects/nested-if assumption.
- Expected stage sequence for both positive files: PARSE success, CHECK exit 0, source-owned certificate/loan validation as implemented by public CLI, *ordinary* C17 emission exit 0, strict GCC/Clang C17 native exit 0. Actual capabilities must be measured rather than presumed. Public CLI does **not** separately export full provenance/ValueFact evidence; if unavailable classify partial/inconclusive, not forge facts.
- Native read-only debugger observer (GDB locals and addresses at guest main's final return) must distinguish `original` physical field from C value copies, show former/new/sibling and identity as far as symbols permit. It never writes guest memory, inserts a guest constant, or alters emitted C bytes. A native exit-code-only success is **not** sufficient for SAFE. Optimized O2 may erase dead locals; O0 observation is mandatory. Generated C checksummed before and after observation.
- Control rejection must be independently attributed to semantic range error (not parser/precision/backend or generic CLI failure).
- Technical verdict SAFE / FINDING / INCONCLUSIVE applies only to this exact bounded source contract. Native PASS cannot imply formal proof, Product GO, general heap/Array/owner authority, multi-root safety, or measurable author/caller audit advantage.
- Value alignment is separate: demonstrate same actual original topology without host substitution; evidence cannot yet show reduced UB-prevention audit/re-audit dependency closure versus factored C/Zig/Rust helpers.

## Execution / stop plan

- Initial freeze commit contains all guest text, SHA, expected oracle, passive observer and workflow.
- Candidate never checked out as moving PR ref; workflow checks exact candidate commit with full history, then records binary digest.
- At most one natural GitHub Actions run plus **one** additional run solely for demonstrated observer/harness/checkout failure. At most two narrow R-only repair commits beyond this freeze; frozen guest source/expected oracle must never change.
- Unexpected positive source/check, negative control, checked-owned/proof stage, emission, native value/identity, lost provenance, unusable observer that cannot independently be repaired, or budget exhaustion => STOP and report finding/inconclusive to C. No P review after verdict without separate approval.
