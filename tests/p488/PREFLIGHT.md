# P488 pre-execution source freeze

Track: P. Only new tests/p488/**; fixed unmodified PR479 head
03b1751b513e7aef2d7ba193eda671a556f54b47, main d6e603e12591c2ff39e3e1b77c784681fec66033,
canonical CURRENT_SPEC -> Draft17.30. This commit precedes every candidate run.
Exactly one candidate: original P478 source with only the match removed and
replaced by one straight-line actual set_link(w,Some(tail_ptr)) plus old Copy use.
Original head/tail, ptr_from_ref loan, initial head store, observed read and final
head reset retained byte-for-byte. Source/oracle SHA frozen in frozen.json.

Compass -> Product Strategy §1.1: audit dependency closure matters; this is only
an implementation frontier measurement, no library/user burden or Product GO.
Process §4.2 N/A: no source selection, normative change, checker/API/emitter edit;
KEEP existing body-visible call, ref/Copy/Change/Reset and original-domain rules.
Historical P475 P8 stop, P478 P9 match-arm stop and P471 inline success remain
distinct; no old G1 transplant, no fabricated match binder q. Definition-only
registration is not a positive main proof. Existing call-body accessor is not an
independent call effect validator. No reading parallel M/O/F research outcomes.

Prediction: outside match the actual-context known-call body may CHECK0, while
independently validating argument/formal/caller post-state evidence is absent.
Potential later direct-REPLACE C17 profile limit is code inspection, not a result.
Oracle: two original lexical implicit-local roots; head payload1/tail2; initial
head Some(tail), authentic Copy tail_ptr; call tail.next None -> Some(tail), old
None Copy; head final None; no reference escapes or heap owner minted.

Run LLVM-free exact compiler --check once. CHECK failure stops the case. If CHECK
passes, inspect actual source-owned graph and public API only. No host-seeded
worlds, fake field snapshots or treating child accessor as a complete certificate.
If independent public call proof is absent, report PUBLIC PROOF NOT ESTABLISHED
and stop without native claims. No need to spend CI on a known proof gap. Frozen
P478 ptr_as_ref negative may be referenced as historical evidence only. No other
positive, oracle rewrite, code changes, CI rerun, PR479 modification or merge.
Final Issue488 report once, then stop this task; Track C owns next investment.
