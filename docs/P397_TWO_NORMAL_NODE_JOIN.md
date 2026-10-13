# P397 — bounded two-normal Node link current-state join

DRAFT / DO NOT MERGE. Fixed base: `348ca7d48f6dd5d658706f4547ab7ef9a63af33d`.
Canonical: CURRENT_SPEC → Draft17.30 §§13.5a, 26, 27.3–27.4; the selected
Draft17.23 local Node/Option field surface is unchanged. This is a source/checker
candidate, not a product PASS, H1 result, general memory phi or native extension.

Historical design audit: **N/A — faithful implementation of selected join
semantics**, including may-state approximation and surviving dependencies.
P7's complete reference facts, P9's independent normal/return worlds, P13's
prefix-only widening and P14's origin/closure discipline remain constraints.
DI-007/010/014 are source/projection context, not authorization for new syntax.
KEEP canonical modes, source grammar, original liveness and nonCopy duties;
DEFER general owner/effect/ABI joins. No specification, ledger or public API
change. Any later semantic widening returns to Process §4.2/§4.3.

## Admission and proof

The existing unchanged-frame `node_unit_arm_frame` remains the first fast path.
The added path handles two normal unit arms of a closed Option<ptr<Node>> match,
exactly two original live lexical local completed two-field Node roots and at
most one changed original `next` place. Each changed arm contains exactly one
checked scoped field replace. Nested match/IF/loop, body calls, allocated roots,
new Node roots or additional changed places are rejected by this path. Creation,
ending or movement of owner authority and different nonCopy availability are
not joined. All incoming link values in this path must be concrete and free of
hidden/ValueFact dependencies, and possible pointers must name a live original
root/incarnation with valid provenance and no scoped/occurrence dependency.

Every arm remains a separately owned checked source snapshot. The guard checks
its actual replace metadata against the common entry, unchanged original place
frames, terminated new loan scopes and the current field/parent post-facts.
A scratch comparison clone restores only the ownership/fact bookkeeping of
that authenticated update and verifies the rest of the complete prefix with
the existing exact comparator. The comparison clone is **not a valid
continuation or committed semantic world**; branch-private values and history
remain in the separately owned arm. No private IDs are rebased into the parent.

The common continuation creates a fresh abstract Option in the PARENT context
and applies ordinary `nl_fixed_change`/Reset: fresh child AND aggregate
ValueFacts, stable root/fixed child incarnations, preserved sibling fact,
ended old conditional occurrence. Old result/payload ownership is discharged
only as the existing Copy/Discardable rules permit. Existing value/occurrence
blockers reject mutation; the join does not erase a surviving dependency.

`variant == 0, sum_payload == 0` is content top, **not None** and not an invented
pointer. Empty dependencies are justified by checking all incoming concrete
packages and pointers; UNKNOWN/HIDDEN dependency knowledge is never converted
to free. The sum invariant admits this coarse shape only for the Copy,
Discardable completed two-field Node Option, with no authority/reference facts
or unconditional payload occurrence. Reading/copying this abstract value is
representable. Consuming or borrowed match/extraction rejects with
`NODE-JOIN-POST-PRECISION`; it cannot issue a safe reference or claim a concrete
Some/None result. Subsequent writes can replace content with a concrete Option
using existing scoped authority; this is separate from recovering a pointer
from the unknown old result.

## Evidence and limits

`tests/fixtures/node_two_normal_join.nl` preserves the exact PR396 Node pattern,
adds a post-join field Copy read and a fresh safe reloan of the previously
issued ORIGINAL tail ptr. Other full source fixtures exercise actual None,
a pointer to the other original root, both normal arms updating the same
field, Some→None occurrence ending use of an independently copied old locator, same-locator Some→Some
replacement and a subsequent original scoped whole write after the join. No successful case seeds values/rights through a host API.

`node_join_test evidence` inspects real source-generated owned artifacts with
public getters: distinct arm contexts, original root/incarnations, incoming
concrete variants, per-arm Change/Reset/loan termination, preserved payload
sibling and the common abstract field-read facts. The function's final lexical
scope ends its Nodes; field records are point-in-time evidence, not live caller
owners. These local roots have no heap Allocation/explicit Domain grants.
Their original lexical stability is not a five-root/Domain-transfer claim.

The exact unchanged compiler rejects the original state-changing match with
CLI3 `P9-CONTINUATION-PRECISION`. Candidate sources register, then an actual
`main()` check owns their evidence; CLI4 `V1-BACKEND-UNSUPPORTED`, C0 remains
because the existing Node emitter requires unchanged match frames. No native
execution of the new join is claimed, and no emitter guard is removed. The
original no-effect Node fixture still emits C; existing native tests cover it.

`node_join_test negatives` checks post-join extraction, two different changed
roots, arm-local root substitution, scoped ref escape, stale original ptr,
multiple updates and attempted owner creation outside the local source profile,
with public state rollback. `owner` uses only a trusted **type/signature**
registry for a negative affine formal; it issues no host-seeded owner value and
is not full-CLI positive evidence. `private-ids` first observes the unchanged
fast path on source-created arm-private Nodes: distinct worlds have coincident
PlaceId/incarnation (12/12). Storing those private tokens into the original link
is rejected; numeric equality is not original-root authority.

Registration and real-call malloc/realloc failure injection covers **384 and
235** allocation positions for the main witness, stopping at the first
success after exhausting all allocation failure positions and checking public snapshots/output rollback on every
failure. Existing P7/P9/P13/P14, dependency/occurrence, same-place/new-incarnation,
heap wrong-D/Allocation and native regressions are part of ordinary five-job CI.
The evidence JSON is diagnostic data, not an ownership or Domain certificate.

Reproduce with the README's existing checksum-locked toolchain:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
build/node_join_test evidence tests/fixtures/node_two_normal_join.nl
build/node_join_test oom tests/fixtures/node_two_normal_join.nl
python3 tests/integration/node_join_source_test.py build/newlangc \
    build/node_join_test tests/fixtures
```

The reusable operation acquired is a conservative one-place memory-currentness
join with explicit original-frame checking and parent-owned fresh facts. It
moves the measured first barrier to finite Node backend lowering or, for safe
extraction of the new current link, post-join provenance precision. It does not
rank or authorize a general backend/effect engine or change H1/STB priorities.

## O6 handoff / independent P qualification

O398/Coordination's Class O/H handoff is advisory, not source authority or proof.
Keep A: conservative content top and fresh field **and parent** currentness.
`node_join_same_locator.nl` observes real source-issued F17→F25, P2→P4,
RF18→RF26 in the changed arm despite identical Copy locator facts; the common
parent RF24 is freshly issued in the parent context, not imported from that arm.
Sibling payload fact F3 and original root/incarnations remain unchanged. Numeric
facts in different owned worlds are not compared as identity merely by number.

A hidden field/root-dependent ref is currently not expressible in this selected
Node source profile. The dedicated **trusted negative-only** dependency test
therefore starts from source-issued Nodes, reads their real field/root facts,
and supplies a dependent ref through the existing test boundary. Direct same-
locator replace returns semantic `FIELD-VALUE-DEPENDENCY`; the two-normal
function-block test rejects earlier at existing
`FIELD-DEPENDENCY-CONTROL-PRECISION`, with unchanged public state. These are C
regressions, not full-source positive authority, nor an O oracle transplanted
into production. Ordinary root refs do not implicitly depend on current facts;
only actual hidden/occurrence dependencies must block the corresponding change.

`node_join_post_write.nl` independently verifies source-owned scoped whole
replace after reading the joined top, returning the unknown OLD Copy Option
and installing concrete None with fresh field/parent facts and stable
incarnations. The C implementation can prove this typed-place operation without
extracting old contents. O6's coarse Python model precision-stopped that query;
that limitation does not prescribe a C rejection or supply C acceptance.
Some/pointer extraction from top is still precisely rejected. No finite
alternative engine or broadening of ownership, syntax or native ABI is added.
