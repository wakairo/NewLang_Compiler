# P489 final — SOURCE/CHECK PRECISION GAP; None native NOT REACHED

Track: P. Fixed main d6e603e12591c2ff39e3e1b77c784681fec66033, Draft17.30.
Pre-run immutable freeze 2fd441c5cafa68f8a462ae1e5265572eac7dd91c.
Only the first head.next initializer changed Some(tail_ptr) -> None relative to
P471; dormant Some(q) code, authentic stored tail_ptr, roots and all other source
bytes retained. No source/oracle edits after first run; exactly two candidates.

| frozen source | SHA256 | --check |
|---|---|---|
| asymmetric_none.nl | 7b95794a3eb145de1a786ccfb68e929003fd12c851fc4721de58a4ad26fd81db | CLI3 P9-CONTINUATION-PRECISION |
| symmetric_none.nl | f049777361399fbc28ed8ad333a433260751a18f25b510791c116ae0d1bbe7fd | CLI3 P9-CONTINUATION-PRECISION |

Both diagnostics: tests/p489/<case>.nl:10:5-13:6:
error(precision)[P9-CONTINUATION-PRECISION]: multiple normal function-match states
require a richer join. stdout0, no complete target-owned artifact. Parser did not
reject; the source has reached normal-match semantic checking/definition
registration. This is a finite-analysis precision refusal, NOT evidence of a
semantic safety error or an unsafe issued certificate. Genuine public-issued
entry/arms/post, C17 generation and native branch execution are NOT REACHED for
both candidates; stopped immediately, without a source rewrite or emitter probe.

Read-only code location: semantic_check.c function_match tries frame preservation
then node_memory_join and emits P9 at6094-6096 when neither establishes the two
normal outgoing states. node_memory_join requires both actual arm frames be
validated by nl_node_join_arm_check; any new Some pointer must resolve to a live
original in the incoming world (node_join_pointer). A known None scrutinee does
not itself supply q's concrete original pointer in the dormant Some arm. This is
a candidate explanation of the failed finite join, NOT an instrumented claim of
which internal subpredicate first returned false. No debug hooks/compiler edits
or fabricated q/ref were used to refine the diagnosis after the public first-stop.
The actual exact measured first barrier is the P9 match span above.

## Control and honest coverage

One unchanged adopted control tests/p471/asymmetric.nl SHA256
c578ebdce71ffd46e30758903c28f69227876c7904264b3eec1914c7f6bdd050:
fresh same compiler CHECK0, genuine source public node join and independent
validator0 (including copied wrong projection rejection), C17 CLI0;2545 bytes,
SHA256 92359ec3c59d72c7286cd45fd28dd8f68302410d935d9417934609feeb123acc.
No fresh native control invocation needed after both candidate CHECK stops.
P471's previously executed Some native path remains valid historical evidence;
P466's different initial-None workload is NOT the target None-path proof.
This task has NOT filled that physical runtime coverage gap.

Prediction: known None may limit dormant Some precision/public issuance. Actual:
CHECK stops before complete owned issuance on BOTH A and S. The two-candidate
comparison localizes a current finite join boundary; no broad language/CFG
change necessity is established. Next selection for C: investigate whether a
bounded checked unreachable-arm or original-locator correlation contract can
support these exact frozen inputs soundly. Do not rewrite the dormant arm or
reuse the Some-only observer as a None oracle. No implementation is authorized
by this result. No safety negatives or mutation-resistance runs beyond the
unchanged control public validator; no new safety PASS claims.

## Reproduce

```
cmake -S . -B /tmp/p489-build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/p489-build --target newlangc p471_public_probe -j4
/tmp/p489-build/newlangc --check tests/p489/asymmetric_none.nl
/tmp/p489-build/newlangc --check tests/p489/symmetric_none.nl
/tmp/p489-build/newlangc --check tests/p471/asymmetric.nl
/tmp/p489-build/p471_public_probe tests/p471/asymmetric.nl
/tmp/p489-build/newlangc tests/p471/asymmetric.nl
```

Local GCC14.2 LLVM-free Debug, exact production tree unchanged; binary/build
identity and raw diagnostics/exit/stdout in evidence/checks.json. Evidence branch
only new tests/p489/**; no code/include/CMake/existing tests/docs/workflow/lock/
PR479 edits. No PR or CI spent on a known CHECK stop, no retries or Track launch,
no main merge or Issue close. §4.2 N/A test-only, Compass/Product Strategy1.1
exploration vs product distinction retained; no measurable audit-locality gain
or Product GO. H1/STB HOLD unchanged. Final Issue report once then STOP task.
