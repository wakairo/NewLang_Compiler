# P466 pre-execution freeze

Fixed compiler/main: `55f4f0feaf5976648a97fbd7190bbf954ed6647e`.
CURRENT_SPEC -> Draft 17.30. No compiler execution has occurred for these sources.

Goal: one bounded existing-feature composition test, not a product gate.
Evidence read: Compass -> Product Strategy §§1–6,9; Roadmap current checkpoint;
Development Process §§4.2,4.3,4.3.1; canonical §10/13 local scoped loan,
§17.4 Change, §26 Option Reset, §27.3 normal join; merged initial-Some and
node_join_native_none fixtures and public checked/node_join APIs.

Own candidate set is frozen in some.nl / none.nl and frozen.json before execution.
Both are expected semantically valid: two original lexical roots, no heap or
nonCopy ownership, implicit local domains/incarnations, independent payloads.
Anchor is constructed first at None, payload 29; peer payload 61. Direct scoped
anchor loan issues the ptr used at peer initial Some (or matched None control).
Match observes peer.next. Both arms are normal/unit, exactly one replace each:
None arm: old None -> Some(original anchor); Some arm: old Some(original anchor)
-> None. Arm-local old is the real pre-write Copy. Old Some occurrence must end
and field/parent facts refresh; peer.payload and anchor remain unchanged.
Post-join Copy is allowed, but its abstract Unknown is not refined to Some.
Exactly one later reloan uses the retained original anchor_ptr, not Unknown.
No host authority or value creation is permitted. Runtime pointer expectation is
exact original anchor address, distinct from peer. See frozen.json for values.

Compile-event boundary: --check must return 0 with no output; then public-only
source-owned inspection and issued join validator must pass before invoking C17
emitter. Expect two qualified per-arm Change/Reset records, not one selected
concrete public post-state. Then ordinary CLI C17/native may succeed or reveal
an existing finite coverage boundary; no layer is assumed proved by another.
Predicted first risk is compositional owned proof or backend carrier restriction,
not new language meaning. Source hash/expected values will not change to rescue it.

Alternatives: infer composability from separate green tests (insufficient),
add general Node backend/heap (unauthorized), or run this exact-source probe
(chosen). Stop at first real source/profile/precision/proof/backend barrier,
wrong native topology, unreliable observer, compiler drift or CI budget.
One frontend-only local preflight, no LLVM downloads; at most one naturally
triggered FULL + matching PARTIAL run, no reruns or iterative CI repairs.
Changes only in new standalone tests and test registration; no docs/production.
§4.2 N/A: unchanged selected semantics, no new source/API selection. If one is
required, stop. No spelling/API corrections have been made after execution.
Technical compatibility is distinct from Product GO; H1/STB/V holds unchanged.
Coordination owns further investment. One terminal Issue466 report, then stop.
