# R1197 C259 V2 independent pre-execution predictions and byte rationale

Authority: main 53c125b8cd66523651a21e803827eff6d2a06428; CURRENT_SPEC Draft17.34; compiler/base 0fd24ad3ed08c51845819aab48506c9d43fdccf6; original R V1 parent a22824b9541666983b32be044f29015ff3a47a47. Post-FIRST calibrated V2, not fresh BLIND.

Only the seven named source expressions have changed. A V1 positive prerequisite failed earlier at precision, before helper RETURN. V2's nested IF chooses literal next usize values: phase0->1, phase1->2, phase2->3, then the existing guard produces Break at phase3. Three actual Continues and one Break remain. Helper definition, RETURN placements, writer loan/replace, old Copy, sibling and original Gauge are byte-identical within each changed source. There is no change motivated by candidate output values.

| Case | Changed | Predicted helper result / CHECK status |
|---|---|---|
| P01 normal IF control | literal-phase only | LT [41,41,173]; legal positive CHECK |
| P02 then RETURN | literal-phase only | LT [41,41,173]; legal positive CHECK |
| P03 else RETURN | literal-phase only | LT [41,41,173]; legal positive CHECK |
| P04 both RETURN | literal-phase only | LT [41,41,173]; legal positive CHECK |
| P05 final RETURN | literal-phase only | LT [41,41,173]; legal positive CHECK |
| P06 Copy EQ both RETURN | literal-phase only | EQ [173,41,173]; legal positive CHECK |
| N01 bad RETURN type | V1 identical | invalid-source type refusal |
| N02 callee BREAK | V1 identical | invalid lexical exit refusal |
| N03 callee CONTINUE | V1 identical | invalid lexical exit refusal |
| N04 caller fallthrough | V1 identical | invalid loop fallthrough refusal |
| N05 wrong CONTINUE type | V1 identical | invalid-source type refusal |
| N06 out-of-profile mixed IF binding | literal-phase only | canonical legal; CHECK may accept, but out-of-admission owned/emission must fail closed |

P01-P05 replaced old sample [7,41,41]; P06 [7,173,41]; all post-loop sample173, unchanged sibling211, stored old Copy7, main unit. Genuinely source-issued proof must establish source-to-actual-root identity and issuer/currentness; addresses and numeric equality alone never suffice. Expect CHECK then public proof/C17/AST/passive map only if actually reached. Pending unbound-role gate means native0 until independently bound and verified. Different semantic, authority/proof, unsafe C or native/sanitizer outcome is STOP, never an oracle adjustment.
