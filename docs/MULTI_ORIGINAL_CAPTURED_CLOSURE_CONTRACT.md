# Multi-original captured-closure certificate contract

Track: P — Compiler #244. Canonical authority: Draft 17.30. This is a
bounded checker certificate substrate, **not** #237 source admission or native
execution. Historical design audit is N/A for this faithful implementation;
no source/API/core semantics or Design-Intent Ledger decision changes.

## Representation and construction

`NLCapturedClosure` owns an ancestor snapshot, its closed post-state, and two
pre-grant fork-entry snapshots. It borrows the two arm artifacts exclusively
owned by the enclosing checked fragment. The public `NLCapturedClosureView`
borrows immutable storage and exposes 0–4 original tuples: ancestor world,
BackingRegion/full extent, typed root/incarnation, governing LifetimeDomain,
Allocation/domain bindings and value packages, and exact availability.
The deterministic tuple order is original ancestor region order. Region,
root, domain and package IDs are interpreted only in their recorded world.
Nominal ownership anchors prevent substituting even byte-equivalent foreign
snapshots. There are no fixed fixture site IDs or address-based identities.

Construction accepts complete, disjoint typed occupancy with unique available
Allocation/domain custody. It checks semantic/raw well-formedness, layout and
full extent, and conservatively rejects active loans, Unknown or conditional
dependencies, and additional available nonCopy responsibilities. The bounded
profile uses one completed recursive-local H type and sequential distinct
allocations; it is not a general owner graph or range solver.

Before either arm is checked, construction derives the common closed post
solely from that ancestor: original roots ended, Allocation/domain bindings
consumed, regions/domains ended. This is a proof target, never an executable
cleanup or a grant of Storage, Domain or ownership. Each actual arm must prove
the target independently. None contributes no original; Some contributes its
one real primitive-granted original. Only a clone of the ancestor-derived post
is committed. Arm-local suffix places, regions, packages and domains remain
private. The existing one-original public path delegates target construction
to this engine while preserving its scalar evidence/native consumers.

## Read-only revalidation

`nl_checked_captured_closure_view` and
`nl_checked_captured_closure_validate` are public read-only APIs. Validation
recollects the tuples, rederives the closed post, compares the exact ancestor
prefix and both independently checked arm states, checks the recorded fork
origins, and recursively validates child certificates. It does not reparse
source or consult the original AST. Owned contexts and arm evidence remain
valid after the caller destroys source/parser/AST storage.

The bounded ownership-only trace verifies explicit checked operands for
`into_slot`, Domain creation, `initialize`, `destroy`, `erase_slot`, Domain
finalization and matching `deallocate`. It preserves original package identity,
full extent, ptr provenance/incarnation and scoped domain-ref authority.
Allocation/Storage are consumed, ptr is Copy, and ending authority is an
exclusive reborrow. Each terminal world must explicitly end and release every
original exactly once. A sealed histogram records independently verified
terminal worlds with 0–5 releases; it is not a runtime counter. Merely presenting
a well-formed closed final context without its operation evidence is rejected.

Mutation of tuple count/order, region/extent/incarnation/domain/binding/value,
world anchors, branch state, release counts, operand use, dependencies or scope
evidence fails closed. Invalid proof returns `ANALYSIS_PRECISION_LIMIT`; ordinary
allocation failure returns `OUT_OF_MEMORY`. Neither publishes an artifact nor
mutates the caller's context. Partial initialization is destructible, and
read-only validation leaves the certificate unchanged on success or failure.

## Isolated integration and preserved gates

Private `nl_captured_closure_probe` uses the real source parser, independent
function registration/checking and known direct-call checker. Its isolated
profile permits up to five Some-nested successful-allocation trials and unit
cleanup results. It has no public CLI flag or host-seeded authority. Actual
`try_allocate_one` checked primitives grant the branch-specific packages.
The certificate profile rejects richer control/lifetime operations rather than
silently generalizing their proofs.

The ordinary frontend remains limited to two allocation trials and the existing
one-link Node profile. The canonical five-root/three-field fixture still rejects
with `AVS-DECL-PROFILE`; the three-/five-site reduced probes still reject publicly
with `ALLOCATED-CARDINALITY-PROFILE`. An isolated accepted probe is certificate
evidence only. No three-field parser admission, native lowering, implicit RAII,
general allocator/owner system, new spec or subsequent milestone is included.
