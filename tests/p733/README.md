# P733 v0-M6: ordinary original-owner local → RETURN

This is P-owned finite capability evidence for Issue #733, not a BLIND review,
formal proof, Product GO, or a general owner-return implementation.

## Authority and unchanged baseline

Implementation base: `4eca3eb3bebd4069fc982fd94332970bfd7f15f0`, tree
`7f81471300049065a34311b40e9ba5071cacb423`, canonical Draft17.33.
Coordination subsequently advanced main to
`5928ea7195339904e5f367f5c12ec6919591f386` for Roadmap-only PR734;
this work does not rebase or modify the Roadmap, Process, specification or lock.

Freeze commit `0163d781e1fc6d7ba468e0bddcb0273baef6b67e` precedes production
changes. `freeze.json` records exact UTF-8 source hashes and independent physical
oracles. `pending.nl` uses Node/pending_tail with head/tail payloads 11/67;
`cargo.nl` uses Segment/cargo, renamed fields/functions, reordered constructor
members, and payloads 29/211. Both allocate the real original two H objects,
set head.next to the original tail, detach in the actual producer, bind and
consume its complete nonCopy owner, receive/destructure in the caller, read the
returned original and terminally release it; head is released separately.
No pointer, Allocation or Domain is seeded by a test host.

`control.nl` is byte-identical to the existing direct-constructor fixture.
`inherited/producer-result-local.nl` preserves the previous P700 witness; it is
inherited prior P evidence, not a fresh independent oracle. The pristine fixed
compiler gave CHECK0 and original producer validator true for both new programs
and this inherited witness, but CLI4/V1-BACKEND-UNSUPPORTED with zero C.
The direct control gave CHECK0/C170 and its existing real-platform/native test
passed. `evidence/baseline.json` and `pristine-control-native.txt` retain these
actual results, including all ten negative diagnostics. No old oracle is edited.

## Issued proof and independent validation

The completed actual callee check seals its issuer-owned 17-node graph, original
caller/call, source plan, body/context, return target and exit-state identity.
The seal is not an ownership registry and issues no new R/O/A/D authority.
It is attached only after successful checking and destroyed with the artifact;
allocation failure follows existing atomic checker rollback.

Public `NLProducerLocalReturnView` exposes borrowed declaration/construction,
whole consumed identifier, RETURN, result, body, and historical entry/return
worlds. `nl_checked_producer_local_return_valid` checks this against the private
issued record, reconstructs the complete source-corresponding graph and checks
exactly one nonCopy local receive/consume/RETURN. It independently combines the
existing original-owner producer certificate, package fields, original live
root/incarnation/region/Domain, local hidden+consumed+dead place, and callee
RETURN target/state. The old local place is distinct from the original H root.
Constructor field order and spelling do not establish ownership.

The ingestion source/tree is destroyed by `node_checked_load`; the existing
immutable issuer-owned function-body source plan remains retained. This is not
a claim that all source information has disappeared. Every graph node and edge
is accounted for; a duplicate edge, orphan, foreign issuer or extra operation
fails closed. Checking the byte seal alone is insufficient: changing both the
sealed and observed consume mode still fails structural consumption validation.

The independent public probe checks actual producer and receiver observations.
The legacy two-H cleanup profile does **not** provide the newer generic
`NLCapturedClosureView`: the probe records zero generic views, inspects existing
`captured_frame_closed`/captured-post and both checked arm resource states, and
requires all present BackingRegions/Domains closed. The existing public
`nl_checked_c_node` independently validates the complete supported producer,
receiver and cleanup graph before exposing C; native verifies actual frees.
Do not mislabel borrowed cleanup observations as a new generic cleanup proof.
The new local-return API uses the existing bool producer validator; false may
mean scratch OOM, not necessarily invalid authority. It never grants authority
or updates public state. A diagnostic dump is not a capability.

## Actual C and physical observations

Normal `newlangc --check source` and `newlangc source` operate on the frozen
source. The emitter lowers actual argument operands and constructor members,
binds a real C packet local, moves its real ptr/Allocation/Domain to the return
carrier and clears consumed packet carriers. The original H never moves.
Raw C temporary bit patterns are not additional semantic owning responsibilities.
No extra guest heap allocation is added; source allocation count remains two.
No zero runtime-cost or broad ABI optimization claim is made.

`native.py` compiles unchanged normal emitted C and a **separate** passive
observer variant with strict C17 warnings, GCC/Clang O0/O2. It reuses established
real malloc/free platform and return observer functions without editing them;
`local_observer.h` additionally observes source-local bind and whole consume.
The observer cannot provide guest values, refs, addresses or authority. Expected
payloads come from the preimplementation oracle, identity is compared to real
observed allocation addresses and original Domain tokens. Real producer/receiver,
head Some(original tail)→None, original tail still live at RETURN, consumed local,
caller receive/read, siblings and exactly-once tail/head cleanup are checked.

Each compiler executes 2 sources × 2 optimizations × plain/passive × 3 outcomes
= 24 native executions. Failure at allocation attempt1: no allocation/frees;
attempt2: original head allocated and freed once, no producer/receiver;
both successful: original tail returned/received/released once and head freed
once. These are real host malloc outcomes with controlled NULL failure selection,
not fabricated semantic worlds. `evidence/*.c17.txt` are unmodified generated C.

## Rejections, rollback and resource boundaries

Ten frozen sources retain pristine semantic/definition diagnostics: duplicate
consume, dropped responsibility, missing member, wrong backing/Domain/root,
active loan, wrong receiver, donor reuse and missing final cleanup. Definition
return-profile refusal is distinguished from an ownership violation. All yield
CLI3, zero C and preserve a preexisting named output. Registration or actual-main
failures return no artifact and leave full public TestState unchanged.

The private gate also reuses existing producer/backend controls on both new
sources. It attacks original identity, incarnation, field/facts, package, donor,
caller, source, local graph/order/mode/Copy origin, foreign worlds with equal
numeric IDs, exit target/state and missing/foreign private seal. Restoration
must recover identical C; rejected C returns NULL and preserves length sentinel.
History tests reuse existing owner correlation controls and malloc/realloc plus
calloc fail-at-N. New explicit bound is 10000 per stage; no fault budget is
silently expanded. Driver OOM injects all three allocators across the entire
ordinary compile path and requires no partial C and eventual successful retry.
Some inherited bool validation scratch OOM returns CLI4; this is counted
separately from CLI1 and CLI3 failures, not called a semantic rejection.
The supplementary diagnostic audit classifies CLI3 into 3107 explicit OOM and
12 inherited generic V0-CHECK diagnostics, with no internal diagnostic. All
3163 failures still expose zero C; no driver diagnostic policy is changed. Dedicated validators/semantic checks also assert snapshot rollback.

First local failures are preserved separately: an incorrect harness assumption
that this legacy route exported a generic closure view; a Clang-only unused
probe counter compilation error; and a **real validator completeness finding**:
changing a copied ref result ValueID passed a type/permission-only check. The
last led to exact source-issued graph sealing, rather than accepting arbitrary
same-shaped ValueIDs. The final negative rejects that mutation. Tests started
after the first Clang build failure also lacked not-yet-built executables; these
are invalid local test attempts, not evidence of a production sanitizer failure.
No CI failure is retroactively overwritten.

P605's frozen entire-header hash remains unchanged. Its normalizer removes only
the exact unique, correctly positioned two-line new artifact attachment before
its already selected IF/copy normalization. P634 adds missing/duplicate/misplaced/
wrong-type owner-attachment attacks; all historical tests remain. Arithmetic
issuer/validator/source/oracle bytes and original manifest digests are unchanged.

## Reproduce and limits

```sh
cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build -R p733 --output-on-failure -V
./build/newlangc --check tests/p733/pending.nl
./build/newlangc tests/p733/pending.nl > /tmp/p733.c
./build/p733_public_probe tests/p733/pending.nl
# Repeat with cargo.nl; native.py runs both real programs and all failure paths.
python3 tests/p733/native.py build/newlangc build/handoff_checked_probe \
  build/p733_public_probe clang none tests/p733
```

The frontend-only route is PARTIAL, not the LLVM full gate. Use the normal
locked toolchain/full CMake build for FULL5. Dedicated sanitizer builds use
`-DNEWLANG_SANITIZER=address` or `undefined`; CTests enable ASan leak checking.
CI and fixed candidate SHA/tree are recorded in the PR/terminal Issue report;
local results and hashes are retained in `evidence/verification.json`.

Historical Design-Intent Gate: KEEP DI-011/012 body-sensitive known-call and
original resource correspondence, canonical §§3.2,13.5c,18.1b/c,24. This adds no
source syntax or semantic selection; no Ledger change is required. DEFER
additional locals/aliases, arbitrary producer shapes, general owner storage/phi,
RAII, more roots, CFG/effect framework, escaping refs and general FFI.

Predicted first barrier was emitter shape, confirmed CHECK0/emit4. Implementing
it safely also required proving the actual local consumption/RETURN origin;
the new ID-mutation test revealed why type/permission-only checking was too weak.
Maintenance cost is one bounded private node seal plus source/graph/return
validator and finite C packet transport, not a new owner semantic system. The
measured flat proof size is printed by the gate; no new semantic world is cloned
for this attachment. No runtime performance benchmark or independent R/F verdict
is claimed. Paused/resumed wall time is not active engineering-time measurement.

Goal Alignment: this removes an unnatural direct-constructor-only backend
restriction while retaining real topology/failure choices. It does not establish
ordinary-safe library abstraction, general nonCopy APIs, author+caller audit
locality improvement vs factored C, Product GO, or H1. Product scoring remains
PAUSED (#624), H1 HOLD (#268). Adoption and any independent review belong to
Coordination; this DRAFT is DO NOT MERGE.
