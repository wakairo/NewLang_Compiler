# Multi-original captured-closure certificate contract

Track: P — Compiler #244 substrate, extended for #237 source admission.
Canonical authority: Draft 17.30. This is bounded checker evidence, **not** native
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

The bounded lifecycle trace verifies explicit checked operands for
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

## Field Change evidence and ordinary source integration

The #237 ordinary registered-function/CLI path selects the same engine for the
exact three-link H profile. The source gate requires one parameterless unit
`main`, exactly five distinct strictly Some-nested allocation trials, and the
completed fields `next/prev/child:Option<ptr<H>>, payload:u8`. The existing
one-link public path remains bounded to two trials. Neither profile introduces
general allocator, owner, field or function applicability rules.

An actual checked `replace` on a three-link fixed subobject owns its pre/post
semantic snapshots, nominal world anchors and checked operation ID. At most
six such Changes are retained by an arm. `nl_checked_captured_change_view`
borrows these immutable snapshots; the last post-state exposes the still-live
pre-cleanup topology. Static ProjectionId is the opaque `(nominal,index)` key;
the current child place/incarnation identifies its particular root instance.
Sibling projections remain disjoint subobjects of one typed root, not new
BackingRegions or C offsets.

The public closure revalidator checks actual write-root acquisition and its
pointer/stability operands separately from mode-preserving field projection.
It requires valid current provenance, matching original LifetimeDomain, active
ordinary read-domain loan, available original domain owner, write permission,
and preserved dependency evidence. It replays existing fixed-field Change on
a private clone of each recorded pre-state and compares the entire recorded
post-state, including parent/child ValueFacts, payload occurrence and siblings.
This replay validates evidence; it executes no user cleanup and grants no
authority to the caller. The trace independently tracks all original R/O/D/A
and current field packages through the six operations and explicit cleanup.
Unknown/alias or extra operations outside this proof profile fail closed.

The immutable evidence remains readable after original source/AST teardown.
No source text is interpreted by the revalidator, no arm-local suffix is
imported into public state, and both None/Some arms must independently prove
the ancestor-derived closed post. A closed context alone still cannot replace
explicit operation evidence.

## Isolated integration and preserved gates

Private `nl_captured_closure_probe` uses the real source parser, independent
function registration/checking and known direct-call checker. Its isolated
profile permits up to five Some-nested successful-allocation trials and unit
cleanup results. It has no public CLI flag or host-seeded authority. Actual
`try_allocate_one` checked primitives grant the branch-specific packages.
The certificate profile rejects richer control/lifetime operations rather than
silently generalizing their proofs.

The canonical five-root/three-field fixture now passes the ordinary semantic
path and reaches explicit `V1-BACKEND-UNSUPPORTED` (exit 4), with no generated C.
The three-/five-site reduced one-link probes still reject publicly with
`ALLOCATED-CARDINALITY-PROFILE`. An isolated accepted probe is certificate
evidence only. No native lowering, implicit RAII, general allocator/owner system,
new spec or subsequent milestone is included. Historical #244/HOLD observations
remain unchanged; current source evidence is in the #237 resume report.
