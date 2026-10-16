# O482 PRE-FLIGHT (frozen before implementation/testing)

Track: O, Class O/H synthetic semantic algorithm hypotheses ONLY. Issue #482 at 2026-10-15 JST. Frozen Compiler/main d6e603e12591c2ff39e3e1b77c784681fec66033; CURRENT_SPEC Draft17.30 (Git blob db8fe08f811acc8286f2d37e9d3229b5d7a67067). P478 PR479 head 03b1751b513e7aef2d7ba193eda671a556f54b47 is a measured P9 call-in-arm precision stop, NOT a source acceptance oracle; historic O5 is no CFG/call solution.

## Experimental IR (hypotheses, not minted source authority)

- Exactly two independent lexical originals `head`, `tail`, with distinct original R, O+incarnation, Allocation, D and owner. Each has `next`, `prev`, `child: Option<ptr<Node>>` and `payload:u8`; all have separate stable fixed-subobject places. Initial `head.next=Some(ptr(tail))` and `tail.next=None` unless a test explicitly changes tail's old value. `head.next` is the source of a Copy matcher binder q.
- Two NORMAL match arms None (no operation) and Some (single body-visible `replace` either inline or inside known `set_link(wref:ref<write,Option<ptr<Node>>>,next:Option<ptr<Node>>) -> Option<ptr<Node>>`). Call formals map to actual field `tail.next`, Copy `q`, and READ-stable D_tail; some tests use a second unused alias formal. Scoped WRITE field reference derives from the correct D authority; Copy ptr cannot mint it. `replace` performs Change/Reset with fresh field+enclosing-root ValueFact and Some occurrence even if pointer bits equal; old Option value returned under a new value package, old memory occurrence ended. Field siblings stay unchanged. Call exits and loan scope must close.
- Each branch is an alternative world; join must cover both normal paths. Terminated branches are not relevant to this experiment. Before/after field/root facts, original D/A provenance, input mapping, q origin and returned-old packages remain separate. `Unknown` is never dependency-free. On invalid surviving dependency -> SEMANTIC_REJECT; insufficient alias/effect/budget proof -> PRECISION/UNSUPPORTED. This is limited modeling, not a claim of canonical source admission.

## Independent algorithms

A: concrete/full-snapshot reference computes full world and maps actual arguments into formal environment, checks callee's single Change/Reset in that environment, returns and merges two full successor world alternatives. No dependency on B.
B: independently check typed formal-to-actual mapping + effect atoms, store Change/Reset + old/new value/occurrence and q-dependent facts in an immutable event/delta ledger with base state and derived sparse frontier; join two deltas while keeping conditional old package and explicit dependencies. B never queries A's result.

## Pre-registered expected cases

Positive: inline vs named call, None/Some different poststates with unchanged original owners; Some(q) points to the same tail Copy locator; old None and old Some correct return; unchanged sibling; correct READ-D -> WRITE ref and scope close; extra unused alias formal to same place if no additional effect.

Semantic reject: mismatched original root/Allocation/Domain, stale q/target original incarnation, stale borrowed ref/invalid lifetime or scope escape, missing copy locator dependencies, duplicate conflicting binding for *same* formal (ambiguous mapping), post-join surviving old field/root-dependent package, fabricated old-value returned, mutated sibling/fact. Distinct formals sharing actual place are not automatically illegal; rules must operate after mapping.

Precision/unsupported: unresolved (Unknown) effect or unmappable actual place, >2 candidate/budget, unmodeled independent heap root and general CFG, arbitrary runtime/FFI.

Comparison: full successor set inclusion, first rejected operation/rule, accepted post-join read/whole Change when all alternatives safe; count state copies vs delta records, record timings and tracemalloc peaks, deterministic seed & min counterexample. Never claim Python succeeds at authentic named-call P9; no Product PASS / Allocation-Domain issuance.

## Bounded stop

One small stdlib Python experiment and tests, no general SSA/call graph/owner transfer, no P changes, no CI unless one natural PR event, stop on first genuine model limit. Final Issue comment exactly once. Optional DRAFT/DO NOT MERGE dedicated branch only; no change to source, canonical, docs, tests, CI or main.
