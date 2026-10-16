# P501 measured checkpoint / verification-only continuation

Initial candidate real runs are preserved in candidate-initial.json, all twelve
frozen .nl bytes unchanged. TRUE/FALSE/copy and unit-arm IF CHECK0; normal C17
CLI4. Mismatch and bool-as-ref semantic rejects. Full Pair guards reach existing
P6-JOIN-PRECISION7:5–8:70: no join or emitter implementation beyond this point.

Compatibility discovered by read-only inspection before broad regression:
adopted tests/p497/frontier_test.py intentionally pins original guard/wrong-type
sources to their old parser failures. P501 explicitly forbids adopted test edits.
Therefore keep normal source admission default-OFF via
NEWLANG_EXPERIMENTAL_USIZE_EQUALITY, and exercise real production source files
in p501_newlangc/p501_parser/p501_semantic test targets with gate ON. The new
feature is fully tested in CI, while old adopted tests/default CLI stay literal
only. This is an experimental admission gate, not syntax/semantics selection
or a source-name exception. C would need separate adopted-frontier maintenance
before considering default activation. Build-source lists are reused from real
targets, not maintained as a second compiler implementation. No new backend.

Historical search correction/limitation: literal Surface Draft1/1.1 files are
not present under those indexed names in the fixed checkout (rg --files -uu).
The Ledger's references were inspected; no claim that the absent full originals
were read. Actual Draft17.4 and17.30 §5.2 both contain the exact operator list.
No conflicting selected single-operator spelling found, no general precedence
rule inferred. Current and historical normative lists plus C6205546443 support
faithful flat == implementation; no new user-visible eq API or overload.
