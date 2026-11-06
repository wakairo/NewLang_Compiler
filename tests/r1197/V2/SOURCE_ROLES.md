# R1197 C259 independently frozen V2 source roles (post-FIRST calibrated)

This is a new V2 prediction, **not** a retrospective correction of the original blind V1. All exact V1 files, oracle, FIRST and precision refusal remain immutable. No P/F/producer source, oracle or result was read.

## Minimal source-only replacement

The seven selected V1 sources P01-P06 and N06 had an additive `continue(phase + usize(1));` each. V2 replaces only that caller-loop expression with:

```newlang
            let next_phase = if (phase < usize(1)) {
                usize(1)
            } else {
                if (phase < usize(2)) {
                    usize(2)
                } else {
                    usize(3)
                }
            };
            continue(next_phase);
```

The nested IF tests only the current phase and yields a literal usize in each arm. At phase0 it yields1, at phase1 it yields2, at phase2 it yields3. The outer guard remains phase<3: phases0/1/2 each call the unchanged helper, replace the selected field and take a real Continue, then phase3 Breaks. This avoids dependencyful usize addition. The five invalid negatives N01-N05 retain their exact V1 bytes, including earlier unrelated addition where present; they had already reached their intended invalid-source refusals.

## Source and caller roles, independent prediction

P01 LT normal IF result; P02 LT true-arm explicit RETURN; P03 LT false-arm explicit RETURN; P04 both LT arms explicit RETURN; P05 normal IF result then final RETURN; P06 phase Copy alias EQ both arms RETURN. Caller always resumes after callee RETURN. P01-P05 deliver [41,41,173] and return old selected Copy values [7,41,41]. P06 delivers [173,41,173] and replaces [7,173,41]. Same-value second LT write is not an issuer/currentness shortcut.

Every positive source has three actual Continue edges, one later Break, main unit, loop/final sample173, guard211 unchanged and old snapshot Copy7. The original live Gauge and both fields retain identity; proof of root/currentness and role correspondence cannot come from observed values or addresses.

N01 wrong RETURN type; N02 callee Break without callee loop; N03 callee Continue without callee loop; N04 caller fallthrough after helper RETURN; N05 wrong Continue argument type. All five are deliberately invalid and byte-identical V1. N06 is canonical-legal but outside the admitted mixed IF-binding continuation family; unsupported proof/emission must fail closed and is not a language-invalid result.

## V2 expected layers and stop

Corrected positive source is expected to reach ordinary CHECK without the V1 addition precision error, and then may independently reach genuine source-issued owned probe, strict C17, actual AST, passive observer, linked map and role binding. An unsupported precision refusal is a bounded candidate result, not a helper RETURN correctness finding. The actual-source role bridge is unresolved, so no native launch is authorized without fresh owner-qualified, unique mapping and verified observer lifetime. Invalid/boundary native launches are forbidden. Any unexpected semantic/proof/authority/native/sanitizer behavior STOPS with raw evidence. No P/F inputs, oracle repair, or extra normal jobs.
