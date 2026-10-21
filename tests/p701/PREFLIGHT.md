# P701 Stage A — implementation preflight

Base fe3e0b81d64246df14e2ec4bb76a137630582f17, canonical Draft17.33.
Compass -> Value Strategy 1.1/1.3 and Process 4.2/4.3.1/4.3.2/7: selected
ordinary internal functions, not Product GO or foreign export. KEEP sections
13/17/18/24; no syntax, capability or canonical change. P692/M693 cheap foreign
export relaxation is not reused. Existing Node replay remains independent.

The five sources/oracles in freeze.json were fixed before production edits.
All give CHECK0 and backend4/zero C on unchanged source-equivalent P696 binary.
The extra sources vary field/initializer order, old vs current return, declaration
order, and three changes: no exact count/name/field-index predicate is justified.

## Chosen mechanism and trust boundary

Use issuer-owned live checkpoints with ordered INIT/READ/CHANGE events, plus an
immutable copy of the completed source-issued checked graph. Capture root/child
incarnations/facts/current values at actual placement/read/change, BEFORE EndRoot.
Capture the live exit and Copy result before lexical teardown. Seal completed
nodes only after checker succeeds; sealing does not reconstruct lost liveness.
A registered zero-formal u8 body owns evidence, retained source/body and all
snapshot lifetimes. A certificate is bound to its exact actual caller/call and
result. Public views are borrowed observations, never authority.

Independent validation: authenticate source/body/call owner and complete graph;
walk only straight-line bindings, scoped writes, Copy reads/identifiers and typed
return/tail; match every temporal event in evaluation order; rederive each Change
using the existing fixed-field kernel on a private clone, check parent/target/
sibling and currentness, scopes and Copy-origin IDs against issuance anchors.
Reject all absent/extra/foreign/changed nodes, event omissions/duplicates/order,
stale/equal-scalar wrong-origin Copy, wrong field/root, active/escaped scopes.
No source reparse/fixture-specific interpreter or value/name/count whitelist.

Snapshots reuse existing semantic cloning/Change validation. Compared with a new
compact delta interpreter, this costs more bounded memory but needs fewer new
transition rules. Events/graph use existing node/resource bounds; OOM is explicit
and checker publication remains transactional. Public evidence never outlives
its artifact/context. New immutable receipts are a narrow authorized backend
trust-boundary expansion and require C's later independent R gate.

Only after validator/negative checks: existing C record, scoped reference and
replace emitters plus actual uint8_t callee return/caller carrier. Keep foreign
export Copy-only, M2 ref formals and branch/owner joins unsupported. Test real
GCC/Clang O0/O2 and passive reads, normal/named output failure, semantic and
unsupported negatives, fail-at-N registration/call/validator cleanup and sanitizers.
One stable candidate FULL5+PARTIAL pair, no unapproved retries/repair after CI.

Stop for semantic redesign, minted authority, unverifiable issuance or a required
broad CFG/effect/ownership mechanism; not for arbitrary LOC/cost thresholds.
