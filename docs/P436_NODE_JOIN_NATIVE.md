# P436 bounded Node join native proof consumer

DRAFT / DO NOT MERGE. Fixed start c6771a029062292c915ac2b8f5e66045661eec3f,
CURRENT_SPEC Draft17.30, branch p436-node-join-native.

## Preflight GO — before emitter edits

Exact main postmerge CI38505746891: attempt1, all five success. R435's independent
PASS covers selected source/public-view tests only, not this backend boundary.
P412/P432/M414 and the merged public header/validator examined. Unchanged actual
node_join_same_locator source parses/checks/owns and public-validates (probe0),
but CLI4/V1-BACKEND-UNSUPPORTED/C0. First missing edge is the inherited C-carrier
mapping of original roots in changed normal frames; switch/loan/Replace already
exist. Existing Local stores source place/incarnation at actual Node construction.

GO design: a separate changed-match consumer requires the exact public witness
and read-only validation, verifies each original entry/fork/post root against
already constructed parent Local plus fixed next/sibling identities, and permits
inherited lookup only for that authenticated prefix. Reuse the finite runtime
switch/Replace, preserving old Copy forwarding and physical root addresses.
Unknown semantic Option is runtime storage, not proven Some/None or liveness.
Original unchanged-frame guard stays. No issuer/API/semantic change needed.

Prediction: one emitter-local family, 150–250 implementation lines, one targeted
native/poison/OOM test family (~400–600 lines). First falsifier: public proof OK
but wrong C carrier incarnation must refuse before publishing C. Other tests:
foreign snapshot/RF/P poison, passive dynamic Some and None executions, generated
C wrong-root/tag/omitted-write/old-copy/sibling mutants, full five-job CI.
STOP on first unresolvable trust negative, additional broad issuer/accessor,
CFG/SSA/heap ownership or cost beyond the 2–4h session. No native claim yet.

Compass/Product Strategy: preserve intrusive topology and move mechanical
bookkeeping to compiler. Technical Exploration GO only, not Product GO; no
caller burden/C/Zig/Rust comparison established. Alternatives: keep CHECK-only,
unsafe guard removal (rejected), broad CFG (rejected), this finite consumer.
Historical gate N/A: faithful lowering of Draft17.30 §§13.5a/b,17.4,26.6/7,27.3/4;
KEEP DI006/007 existing source spellings/projection and original identities.
No grammar, canonical, Ledger or public API selection. General effects/owners,
H1/STB and Product holds remain. Coordination/R decides any adoption.
