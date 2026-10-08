# Draft17.28 live-tail RETURN native gate

Track: P — Issue #210. Candidate for independent Coordination review;
OPEN/unmerged PR, no self-acceptance or subsequent Track authorization.

## Authority / exact claim

Base main: `ff9098848dc6a826d6a8be84c2f33dce9e9b393c`.
CURRENT_SPEC selects Draft17.28 §§18.1b.1–6. Read Process, Testing Strategy,
Design Decision Procedure Gates A–E and DI-009–012. Historical design audit:
**N/A — faithful lowering, semantic delta 0**. KEEP adopted fixed LiveTail,
scoped head write-ref, original tail O/R/D and independent terminal receiver.
No Ledger, canonical Draft, source API, oracle or toolchain pin changes.

Reused independently accepted source/owned gate #209 and terminal native #202.
R #205 is bounded frozen Draft17.27 empirical negative evidence; V #204 is
frozen upstream C reference evidence. Neither is this native return proof.
No unmerged F result is used or required.

Primary actual source is unchanged `tests/fixtures/live_tail_return.nl`:
SHA-256 `fe6671d59794d3c96b935c97cf30753dc49a83ed7c3b560b2715afc0e8ba5e4f`.

The path now executed is:

```text
actual three-path NewLang source
  -> independent definition / every-call semantic check
  -> owned producer entry + live return worlds / retained checked operations
  -> separately emitted C17 producer / direct call / by-value LiveTail
  -> caller whole destructure / same-D scoped tail read
  -> separate terminal receiver / original tail free
  -> independent donor head cleanup
```

## Checked-to-C boundary

`nl_checked_c_node` borrows immutable owned artifacts. Preflight and call
lowering both require `nl_checked_producer_valid`: exact owned world tags,
independent definition obligations, original live tail root/incarnation/full
BackingRegion/Domain, disjoint live head and D_h-scoped write projection,
current Some(ptr_t), dependency/occupancy/access and returned correlation.
A foreign cloned world with identical numbers is not proof.

The closed backend accepts a two-item producer: checked local binding of
`replace(head_link,None)` then checked `return LiveTail {...};`. It validates
primitive operand identity, projection/root/child/incarnation, pre/post facts,
Change/Reset/occurrence, complete field indices and each original semantic
member. Fields are evaluated in checked source order, including reversed
constructor field order. No source names/text or AST is inspected for meaning.
The whole destructure checks each receiving binding against the original
checked return member identity; A/D are transferred into fresh carriers.

Generated `static nl_live_tail nl_producer_<checked function ID>(...)`
actually takes four source-selected carriers and writes the real field via
its checked write-ref parameter. It constructs and returns a private C struct
of original ptr/Allocation/Domain by value. It allocates/ends/frees nothing.
Caller receives the result while the head scope still exists, then the scope
ends before the whole destructure and terminal call. No fresh owner, Storage,
root or Domain is reconstructed. Existing terminal proof is independently
validated again; receiver body performs the real tail free before donor head.
C zeroing of consumed carrier variables does not create language authority.

Representation remains private Linux x86_64 C17 with existing H size 24 /
alignment 8. Neither this carrier nor C offsets define a NewLang ABI/layout
or supply provenance, dependency, lifetime or ownership proof.

## Independent execution observation

Separate **emitter-free** `handoff_checked_probe` loads the same actual source,
checks it, disposes the original syntax/source and reads owned evidence. Its
INIT / ROOT / FIELD / PRODUCER / CALL rows supply compiler-known expected
root/incarnation/R/D, payload, projection, scope and call IDs. Expectations
are not copied from generated constants. Native assertions compare actual
malloc addresses and live H bytes with these independent facts.

`live_return_observer.h` extends the existing terminal read-only observer.
Bookkeeping is its only mutable state: no H byte write, allocation, release,
link repair or safety grant. It observes cleared donor A/D, actual producer
entry, head Some(real tail) -> None, still-live original ptr/handle/Domain
at return and caller receipt, caller scoped safe read, separate terminal
EndRoot/full-range/raw/finalize/free, then donor head release. No object is
read after EndRoot. Existing NULL-only `two_heap_platform.c` delegates every
successful allocation/free to the real allocator; it is unchanged.

Native primary and renamed source variants each run the **same emitted C**
in all three worlds, observed and plain, C17 strict warnings, **-O2 -DNDEBUG**.
ASan/UBSan configurations also instrument host-generated binaries. Runtime
addresses vary and are observation only, never source authority.

| world | requests | frees | producer returns | terminal calls | free order |
|---|---:|---:|---:|---:|---|
| first NULL | 1 | 0 | 0 | 0 | none |
| second NULL | 2 | 1 | 0 | 0 | original head |
| both Some | 2 | 2 | 1 | 1 | original tail in receiver, then head |

Representative GCC measured trace (run-specific addresses):

```text
primary 1 OBSERVED head=0 tail=0 trials=1 free=0 roots=0 changes=0 copy=0 scopes=0 receiver=0 receiver_free=0 donor_free=0
LIVE_RETURN producer=0 returned=0 received=0 original_tail=0
primary 2 OBSERVED head=562df33442a0 tail=0 trials=2 free=1 roots=0 changes=0 copy=0 scopes=0 receiver=0 receiver_free=0 donor_free=1
LIVE_RETURN producer=0 returned=0 received=0 original_tail=0
primary both OBSERVED head=556109c762a0 tail=556109c762c0 trials=2 free=2 roots=6 changes=2 copy=1 scopes=6 receiver=1 receiver_free=1 donor_free=1
LIVE_RETURN producer=1 returned=1 received=1 original_tail=556109c762c0
```

## Falsification / atomicity / regression

14 independent negative execution controls are built successfully, then
**observer-detected**, separately from sanitizer findings: 13 emitted-C
mutations (wrong returned ptr/A/D, wrong callee/call ptr, forged Some,
missing unlink, wrong free, donor premature free, duplicate nonnull free,
second-NULL missing head free, skipped receiver free, bad caller receipt)
and one changed checked expectation. Observer rejection must report
`OBSERVER_REJECT` and SIGABRT; ASan error/UBSan runtime error is not accepted
as the detection mechanism. Duplicate free is observed while phase=9 and
aborts before the wrapper forwards a second free. No deliberate C UB or
freed-memory dereference is used. The observer does not repair any mutation.

Module tests reject 22 certificate/body/world/profile mutations with output
NULL and length sentinel unchanged, then restore byte-identical successful C.
Staging is transactional across main/producer/receiver buffers. Fault injection
enumerates every actual malloc path: three staging OOMs report OOM; the
existing bool certificate validator rejects scratch OOM fail-closed as
unsupported. In Debug this is 11 positions (3 staging + 8 scratch); GCC
Release optimizes scratch allocation paths to 7 (3 + 4). These counts are
observations, not a layout/allocator contract. Clean retry produces identical
C without semantic state mutation. A checker-accepted 600-statement source
hits the emitter resource fence with no partial C. Existing semantic OOM /
rollback, 41 owned-evidence corruptions and 27 source negatives remain.

Existing source test explicitly preclassifies six admitted source variants
as supported C profiles; four language-valid variants remain backend-unsupported
(local producer result, producer aliases, separate caller package binding,
absence of optional caller tail read). Classification is input-based and
fixed before execution; a failure does not classify overlap. The 10 semantic
positives still pass; no old negative or CI job is removed. Existing terminal
three-source native cases, two-H / one-H / lexical gates, oracle.adapter,
oracle.smoke and artifacts.integrity are retained.

## Validation / limits / handoff

Reproduce with checksum-verified bootstrap and existing CMake configurations:

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DNEWLANG_SANITIZER=none
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
ctest --test-dir build -R live_tail_return_native -V
bash scripts/check-format.sh
```

The same CTests run with GCC Release, Clang, address and undefined sanitizer
configurations. Exact PR head and five CI links/results are recorded in the
Issue/PR handback; documentation does not recursively embed its own commit SHA.

No new semantic blocker. **COMPILER-IMPLEMENTATION-LIMIT**: one completed H,
two static allocation sites, one exact two-item producer, one whole receiving
profile and existing terminal receiver; other valid profiles are explicit
backend-unsupported. **COMPILER-DIAGNOSTIC**: bool validator scratch OOM has
conservative unsupported status rather than a distinct validation OOM status.
No general ownership return/aggregate backend or effect system is claimed.
These are at most two residual bounded limitations, not language errors.

No third Node, general allocator, traversal, recursion, FFI, LLVM, cJSON,
new M/F/R/Q/V or follow-on task. Candidate stops at:

**DRAFT17.28 LIVE-TAIL RETURN NATIVE GATE READY FOR COORDINATION REVIEW**
