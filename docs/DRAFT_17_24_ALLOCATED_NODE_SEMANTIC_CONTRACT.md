# Draft 17.24 allocated Node source-semantic gate

Track: P — Issue #171. This is the pre-backend gate authorized after #169/#170
and independent Coordination ACCEPT. Authority: live base main
`38696f024166ffb4e0434df2719a00714bbc0764`, CURRENT_SPEC → Draft 17.24 §3.2,
with the existing raw, domain, typed lifetime, closed sum and dependency rules.
Historical design audit: N/A, faithful canonical implementation; semantic delta 0.

## Source and conditional evidence

Only a direct consuming `match try_allocate_one<H>()` is currently supported.
H must be the existing completed two-field recursive nominal: an
`Option<ptr<H>>` link and u8 sibling. OneBacking and Option<OneBacking> are
compiler-registered non-Copy/non-Discardable types. Whole destructuring moves
Allocation and Storage. Their labels are fixed; H and its field names are not.

The common trial has **no result ValuePackage or published backing identity**.
Both None and Some have independently owned checked arm contexts. None creates
only its temporary sum value; no allocation, raw claim, slot or domain. Some
creates an abstract, branch-conditional fresh region and the unique full-range
Allocation/Storage obligations, then checks the actual source body. Neither
arm is a runtime allocator event or a host-seeded source prelude. A body error
in either arm rejects the whole source.

Each arm must complete normally with unit, close newly created live regions,
domains, places and non-Discardable values, and preserve the captured public
frame exactly. No arm-owned ID is imported into the caller. New facts remain
in their owned arm artifact for inspection. Richer result/state/control joins,
indirectly stored allocation outcomes and nested allocation worlds receive
precision/profile rejection rather than a success-only representative.

## Lifetime and occupancy

The bounded source routes delegate to existing semantic primitives:

| Source | Checked obligation/evidence |
| --- | --- |
| `into_slot<H>(raw)` | Exact target length/alignment; consume raw; same-region empty occupancy |
| `lifetime_domain()` | Fresh source-created domain owner in Some only |
| `loan_read(life)` | Active ordinary scoped domain ref; exactly-once body, noescape |
| `initialize(empty,H{...},stable)` | Same domain, fresh live incarnation, same backing occupancy; returned ptr facts |
| `ref_from_ptr(read,p,stable)` | Current live ptr, readable access, explicit same governing domain, concrete dependency proof |
| `loan_exclusive_read(life)` | Actual exclusive ref; no conflicting live domain/target refs; operation-local exclusive child reborrow |
| `destroy(p,ending)` | Current incarnation and matching exclusive ending domain; EndRoot returns same-range empty slot |
| `erase_slot<H>(empty)` | Consume slot, return raw occupancy without starting lifetime |
| `finalize_domain(life)` | Consume domain only when no governed root remains |
| `deallocate(allocation,raw)` | Same live region, exact full-range raw occupancy; consume both, end backing |

Existing lexical head field replacement, Option Copy/match and unit-frame join
are reused. Nested Node Option arms must preserve the live backing/domain frame
exactly. Unknown dependencies cannot become dependency-free reloan evidence.
Persistent Copy ptr tokens may remain after cleanup, but stale/deallocated or
superseded-incarnation safe ref acquisition rejects. Unlink is exercised in the
witness; mere persistence of a ptr token is not invented as a lifetime owner.

## Target facts and boundaries

On the existing Linux x86_64 execution target, the compiler-private H plan is
size 24 / alignment 8 (tag word, pointer word, u8 and padding). These are checked
allocation/slot target facts, **not** a NewLang ABI/layout/offset guarantee, a
host pointer or a native allocator call. Other targets and differing preexisting
layout facts are explicitly unsupported. A later separately authorized backend
must validate/honor its representation plan before native allocation lowering.

Every registration/check runs within the existing clone/commit transaction.
Registry strings, type metadata, grants, loans and arm artifacts belong to that
candidate or its owned snapshots; any semantic/OOM/resource error discards it.
No hidden global state, runtime safety check or implicit authority cleanup is
introduced. Scope obligations reject forgotten non-Discardable owners.

Backend admission is unchanged and fail-closed. The accepted lifecycle source
returns CLI exit 4 with `V1-BACKEND-UNSUPPORTED`, empty stdout and no C/object/
executable. Existing lexical topology native support remains independently
available. No dynamic native execution, allocator lowering, detach/free graph,
cJSON, LLVM/FFI, general recursive backend or spec amendment is included.
