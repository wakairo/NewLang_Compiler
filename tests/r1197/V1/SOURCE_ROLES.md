# R1197 V1 independent source roles and derivation

All cases declare one nonrecursive two-u8 Copy/Discardable Gauge and one original local in unit main, with a known-direct usize -> u8 helper. The common caller selects sample (declaration index 0), leaves guard (index 1) alone, advances phase from 0 by 1 under phase < 3, and uses explicit Continue and Break except the deliberate invalid controls.

## Positive/control cohort

| Case | Helper | Independent purpose |
|---|---|---|
| P01 | LT normal IF tail result | Control for the shared caller/field behavior |
| P02 | LT then RETURN, else normal | Callee early exit on phases 0 and 1 must resume caller |
| P03 | LT then normal, else RETURN | Callee exit on phase 2 must resume caller |
| P04 | Both LT arms RETURN | Zero normal IF edges must not synthesize unit or exit caller |
| P05 | Normal IF -> chosen -> final RETURN | The evaluated normal result is exactly the value returned |
| P06 | One Copy alias of phase, EQ, both RETURN | False/true/false condition order and short Copy envelope |

P01-P05 deliver [41,41,173] and return old selected values [7,41,41]. P06 delivers [173,41,173] and returns old selected values [7,173,41]. All three writes preserve sibling 211. Every normal source has loop result 173, final selected 173, final sibling 211 and retained old Copy 7. Main's result is unit. Full literal vectors and exact per-case hashes are in oracle.json.

Draft §§18.5/19.1/27.9 make RETURN terminate its enclosing named function only. The helper has no main/loop exit authority. §§27.4/27.3 exclude terminating arms from the normal IF join, including the zero-normal-edge both-return case. §§27.5-27.8 require fresh loop parameter receiving, exact Continue types, explicit Break result and no normal fall-through. §§16.2/17.1/17.33/17.4 distinguish named field identity, old Copy result, lifetime-preserving Change, original/child identity and unchanged sibling.

The source trace has three helper calls, three Continue edges, one Break edge and four loop guard evaluations. Those are predictions, not an assertion that optimized C/public interfaces expose separate events. Actual observed events and inferred source semantics must remain separate.

## Negative/boundary cohort

| Case | Sole intended challenge | Frozen disposition |
|---|---|---|
| N01 | usize RETURN in u8 helper | Type/source CHECK refusal; no proof/C/native |
| N02 | callee Break without its own loop | Lexical loop-target refusal; caller loop cannot authorize it |
| N03 | callee Continue without its own loop | Lexical loop-target refusal; caller loop cannot authorize it |
| N04 | caller true loop path falls through after both-return helper | Must reject fall-through; helper exits do not terminate caller |
| N05 | Continue supplies u8 to usize phase | Exact loop argument-type refusal |
| N06 | mixed-return IF result binding followed by chosen tail | Canonical legal but outside Issue admission family; CHECK may accept; unsupported owned/C17 must fail closed; unexpected admitted emission returns to C before native |

A safe early unsupported/parser refusal does not prove the intended later negative obligation. A legal positive safely refused as unsupported is not language-invalid. No invalid/boundary source is authorized for guest execution. Targeted invalid acceptance, fabricated/foreign authority, wrong exit, source-to-C mismatch, native/sanitizer anomaly or failure not explained by R-owned apparatus requires STOP. First results remain immutable; source/mechanical repair requires preserved original bytes and a new pre-execution prediction.
