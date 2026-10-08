# Draft 17.24 — one allocated Node native contract

Track: P / Issue #173. Canonical base: main
`25c3fe260ce17b27d1d75b45004511a96ad9308d`, CURRENT_SPEC → Draft 17.24.
This implements §3.2 and the narrowly authorized Checked-C gate; it changes no
source spelling, semantic rule, or public layout/ABI. Historical design audit
§4.2 is N/A for faithful implementation; DI-006/007/008/009 remain authoritative.
The semantic-only #171/#172 contract remains historical pre-backend evidence.

## Prerequisite: selected reloan operands

`allocated_ref` now records two checked Identifier arguments in the **same owned
arm artifact**: the selected ptr and selected ordinary read stability ref. Both
follow the existing Copy capability convention. The stability input borrows
the already scoped authority; its owner/domain is neither consumed nor created.
Temporary argument packages end after checking. Their immutable symbol, type
and use evidence survives for lowering, along with the existing referent,
incarnation, domain and resulting-ref scope facts.

No admission changes: current incarnation, explicit matching readable domain,
scope/non-escape, conflict and dependency checks still precede success. Missing
or malformed operand evidence is unsupported at the backend. Lowering obtains
runtime operands from the checked argument chain and resolved symbol carriers;
it never guesses a Some binder or substitutes the allocation-root address.

## Exact native admission

The existing Node emitter handles one direct `match try_allocate_one<H>()` with
two owned checked arms. H is the exactly completed recursive nominal containing
`Option<ptr<H>>` and `u8`. The successful arm has one whole OneBacking
destructure, one slot/domain/initialize, one lexical H head, link replacement,
three Option bindings (old None, independent Copy, old Some), a finite Option
match and one explicit-domain read reloan, unlink, exclusive-domain destroy,
erase slot, domain finalization and matching deallocation. Both arms normally
complete with unit. Extra lifecycle/operation forms are explicitly backend
unsupported, even if the checker accepts them. No general allocator/backend
framework is introduced.

The common allocation-trial node mints no backing. One generated `malloc(24)`
produces the actual conditional handle. NULL selects the checked None body;
non-NULL selects the independently checked Some body. Only Some constructs
separate private Allocation and full-range Storage carriers. Consumption moves
the relevant C carrier and clears the old carrier; it does not copy affine
source authority. Slots retain exactly the checked region/range. Initialize
writes the checked H value **at that block**, producing its actual pointer.
An explicit domain token is a private carrier for this single checked domain,
not a runtime ownership adjudicator.

Lexical link, Copy and conditional Some binder use the prior Node representation
and checked symbol mapping. Domain loans retain their checked scope. Destroy
marks the statically proven EndRoot transition and yields the same empty slot.
For this exact H all fields are Discardable, so no C destructor action is needed;
ending the NewLang typed lifetime is not a claim that C bytes disappear. No H
read occurs afterward. Erase preserves the range; finalization consumes the
domain; deallocate emits exactly `free` of the matching Allocation handle.
There is no implicit cleanup, runtime provenance repair, or runtime replacement
for the semantic checker.

The private Linux x86_64 target plan is size 24/alignment 8. Generated C17 asserts
size/alignment, pointer size and member offsets (link 0, u8 16). A mismatch is
a target implementation failure, not a new NewLang C-layout guarantee. No native
layout is exposed to source.

## Identity and failure boundary

Runtime symbols are keyed by artifact/frame, resolved symbol and type. Crossing
a match-owned world requires the checked binding-prefix certificate. Heap-root
lookup additionally requires the certified parent pointer binding; coincident
post-fork numeric IDs cannot authorize importing a root. Dynamic types are read
from the current owned context. Neither hypothetical Some state nor machine
address equality is merged into common semantic state.

The existing emitter stages C in an owned bounded buffer (131072 bytes, 128
locals/frame, 512 steps, depth 16). Unsupported, resource limit or malloc failure
returns no output and leaves the caller's length unchanged. These are backend
limits, not language errors. Source/checker failures remain transactional and
emit no C. Existing unsafe source and raw/storage programmatic tests retain
their separate roles.

## Independent runtime evidence

`allocated_checked_probe.c` reads owned checked views only, after production
source checking: scalar values and selected ptr/stability symbols. The Python
harness uses those results for expected values and checks emitted operand use.
The emitter has no source AST/text input.

`allocated_observer.h` only observes actual native values/addresses and event
order. Its abort-on-failure checks remain active under NDEBUG. It never writes a
Node, initializes topology, allocates or frees. H memory reads occur before
EndRoot; after release only integer address observations remain. The observer
checks distinct lexical/heap objects, initialized scalar/tag values, linked and
copied Option values, Some binder, selected reloan pointer/token, actual unlink,
EndRoot/raw/finalize order and one matching platform release.

`allocated_platform.c` is a separate test-only platform-call interceptor.
Success delegates the generated malloc unchanged. Controlled failure returns
only NULL. Free instrumentation checks the exact generated request and delegates
that request to real free; it supplies no additional cleanup. Test-only
`-fno-builtin-malloc/free` preserves call interception. Unobserved generated C is
also compiled/executed. Instrumentation is inert in ordinary product output.

Corrupted generated-C controls must strict-compile and fail the observer: wrong
tag/pointer/payload, omitted unlink, missing/extra release, and release before
EndRoot. They demonstrate observation sensitivity, not independent Red Team
review or complete memory-safety proof.

## Stop / non-goals

This is one intermediate native product gate, not North Star PASS. No new Draft,
general recursive backend/allocator, second heap Node, detach/delete, FFI, LLVM,
cJSON, oracle policy or next slice. Candidate PR stays OPEN/unmerged for
independent Coordination ACCEPT/BLOCK; Issue #173 stays OPEN.
