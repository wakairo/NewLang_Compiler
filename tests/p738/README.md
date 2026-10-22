# P738: authenticated no-pre-read owner-local RETURN backend lane

Track P capability evidence, DRAFT / DO NOT MERGE. This is not a new BLIND R
review, a general owner/alias API, Product GO, or H1 success.

## Fixed authority and preserved independent history

Original implementation base: `4eca3eb3bebd4069fc982fd94332970bfd7f15f0`;
repair base: immutable P735 `663a84916d3c7695f7c6accd7e42a4fa94e850e1`,
tree `621caeaad4b770da9e6750efe7ca52fc63bcf604`; canonical Draft17.33.
Main `5928ea7195339904e5f367f5c12ec6919591f386` is Roadmap-only beyond4eca.
No rebase or edit of P735, R737, canonical, Process, Roadmap or lock.

R736 original freeze `b91ebc7df77f98c3a0d33427472cc078a3239e8f` and R737 HEAD
`ccc4b4a95ca5ebadab4354df157ee33badd2b959` remain intact. R A is the original
3333-byte `tests/r736/positive_direct_local_return.nl`, SHA256
`676f662afca5254e3399ebb1195d4fe75cefc239180fe8602590a8c01bb320ee`.
P uses it from a separate detached, unchanged R checkout, without copying or
editing R's NewLang file. P's new native results are NON-BLIND P-calibrated
observations, never retroactive independent R evidence.

R's original A CHECK0/public-PASS/C174/no native, original B's reversed-layout
CHECK3/AVS-DECL-PROFILE and R's intentionally red FULL5 `39770638842` /
PARTIAL `39770638824` remain historical facts. No adjusted B is introduced.

## Freeze and the first actual refusal

P preimplementation freeze commit `bede168b0a2931ad2306d2cc06d279afbeabd3a8`
contains `freeze.json`, sources/oracles, pristine candidate observations and
read-only diagnostic instrumentation. Sprout uses17/149, different names,
Allocation/ptr/Domain member order and a terminal receiver read. Ticket uses
23/199, reordered constructor and a receiver without that read. Both preserve
real two-H initialization, initial link, producer-local whole RETURN, caller
read/terminal receipt and cleanup, without any pre-detach link read/match.
Eleven semantic negative sources were frozen before production edits.

Both P positives and untouched R A: pristine P735 CHECK0, authentic producer
and local-return public validators PASS, ordinary C17 CLI4 with0 bytes. The
first actual failing guard is **allocated_expr:1614 on old checked_c_node.c**:
second head `ref_from_ptr(write,...)` has `reloans=1`, reads0, projections1,
link_updates1; old role expects write only at loan indices0/3, so expects a
read at index1. R A's issuer-owned main span1778:1807 maps to original line72,
column35. Other producer/read/projection counters are later restrictions,
not the first refusal. `evidence/first-*.txt` retains actual traces/status.
`diagnostic-emitter.c.txt` is a separate trace-only copy; no production guard
was bypassed and its rejected output stayed empty. The pristine CLI/public
results are in `frozen-candidate-baseline.json` / `P-source-baseline.json`.

## Soundness and actual repair

Classification: finite backend shape restriction (A), with explicit backend
operand correspondence needed. Draft17.33 §18.1b.2–.3 already requires actual
head CURRENT Some(original tail), independent original R/O/A/D and a scoped
write, not a ceremonial read/match. The existing source-issued producer proof
and independently validated local receive/consume/RETURN seal provide this
relation. No issuer, public API or semantic authority changes are needed.

The only production edit is `src/checked_c_node.c`:

1. Remember the actual first emitted attach operation and its issuer.
2. At the no-read second write, independently validate one actual local-return
   producer call; tie its first projection's identifier to the exact scoped
   ref definition/binding/receiver in the same source graph. Compare current
   head place/incarnation/value/parent+child facts/Some occurrence with the
   first emitted Change and the public historical producer-entry certificate.
3. Validate the actual Some constructor and Copy operand against its issued
   semantic payload and the exact **initialized original tail C carrier**.
   The actual call's ptr operand must select that same original carrier.
   This is a direct-carrier gate; no arbitrary Copy-alias search is claimed.
4. Retain that exact issuer/call in transient Emit state and require it at
   the real producer call, where the original body/tuple/RETURN proof and
   physical argument/carrier/lifecycle checks still run. Only this certified
   lane accounts for the missing two read/match loans, one projection and one
   Option-copy binding; it has no link-read event. Successful whole emission
   still requires exactly one producer, one receiver, two Changes and cleanup.

This cache is backend admission bookkeeping, not a hidden owner registry or
ownership certificate. It adds no persistent proof/world, allocation, guest
Node, source operation, runtime rights or precomputed guest value. Existing
validators perform four extra scratch allocation positions in this lane;
no new authoritative fact is minted. Historical producer worlds establish
liveness; final-context pointer facts are only used for operand identity,
never to resurrect a dead root or create Domain stability.

All ordinary ref_from_ptr/scope/access checks remain. The generic second
replace read guard is unchanged; the certified callee's source-issued Change
is emitted by its existing producer consumer. Custody and the old read/match
lane retain their old conditions. P733 pending/cargo/direct-control generated
C is byte-identical to the fixed predecessor. General alias, direct-constructor
no-read and incomplete read-without-match shapes remain CHECK-legal but finite
backend unsupported where this lane cannot prove their carrier correspondence.
The separately labeled postimplementation `profile-freeze.json` records those
P-calibrated precision probes; they are not preimplementation independent oracles.

## Validation and failure boundaries

`native.py` runs normal `newlangc --check`, genuine source/public proof and
normal C17 emission, then compiles **unchanged** emitted C and a separate passive
observer with strict C17 warnings at O0/O2. `observer.h` reuses established
allocation/receiver/local-return observations with explicit no-read roles.
It never writes guest fields/branches, seeds values/rights, repairs allocations
or substitutes a fake main. Successful malloc/free delegate to the real host;
NULL-only failure selection tests allocation attempts1/2 and both-success.

For each compiler, two P sources produce24 native executions; including external
unchanged R A produces36. Success verifies two distinct original H addresses,
Allocation handles/Domain tokens, actual head None→Some(original tail)→None,
local consume, callee RETURN, caller receipt/read and exactly-once tail/head
cleanup with unchanged siblings. Failure1 has0 frees; failure2 only head is
freed once, with no producer/receiver. Root/ref counts are4 (Sprout/R A) or3
(Ticket), fields2, Changes2, link Copies0. These measured physical facts do
not replace the genuine source-issued authority validators.

The public probe is inherited unchanged P733: original/callee-local proof,
consumed/hidden/dead local place, original live tail unchanged at RETURN and
caller entry tuple. Generic CapturedClosureView remains absent on this legacy
profile; borrowed closed-arm resource observations are not a new generic
cleanup certificate. Public `nl_checked_c_node` validates the supported complete
producer/receiver/cleanup graph before returning C; native tests actual frees.

`gate_test.c` attacks attach operands, Some variants, omitted/duplicated Change,
wrong field/root/incarnation/parent+child facts/occurrence, wrong ref scope/Domain,
definition/projection edges and same-numeric foreign-world calls. Every refusal
must restore identical C, NULL output and length sentinel37. Existing P73379
local/world controls,22 backend controls and41 original-owner controls are
reused on **each new source**, not replaced. Their fail-at-N tests cover both
caller and historical producer-return public snapshots and successful recovery.
Registration/actual main, validator/staging and whole-driver OOM are bounded
10000 per stage; logs record actual positions. Old bool validators may return
unsupported on scratch OOM; CLI3 explicit-OOM versus inherited generic V0-CHECK
and CLI4 are counted separately, never called semantic violations.

Eleven source negatives preserve their pristine diagnostics, return no artifact/C
and preserve named output; the wrong initial target/None source refuses with
P208-CALL-HEAD-VALUE. Scoped read-as-write, active loan, wrong A/D/root/recipient,
double move/use-after-consume and outstanding obligations also reject. Larger
CHECK-legal precision probes yield backend4, zero C, preserved named output.

The first prototype reached the old global Option-binding-count guard; its
refinement is retained. A subsequent **experimental private-graph control**
changed the emitted Some operand to head while retaining semantic Some(tail).
The first prototype accepted this corruption; the final actual-operand/carrier
check rejects it. This is not a legal-source acceptance or a new independent R
runtime-unsoundness claim. First failures are preserved rather than relabeled.

## Reproduction and retained scope

```sh
cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build -R p738 --output-on-failure -V
python3 tests/p738/native.py build/newlangc build/handoff_checked_probe \
  build/p733_public_probe clang none tests/p738
# Optional final argument: untouched R737 checkout's original A source path.
```

Frontend-only is PARTIAL. Full builds use unchanged locked LLVM/Clang23.1.3.
Dedicated ASan/LSan and UBSan builds, full frontend regressions, native/compiler
logs and SHA inventory are in evidence/verification.json. Exact fixed-head
natural FULL5/PARTIAL IDs and candidate/virtual-merge identities are recorded
in the DRAFT PR and terminal Issue738 report; P never merges or starts R.

Historical gate: KEEP DI-011/012 and Draft17.33 §§3.2,13.5c,18.1b/c,24;
faithful compiler correspondence, no normative source selection or Ledger change.
Selected alternative authenticates the already sufficient source history.
The lower-cost fallback is the existing read/match spelling. Merely lowering
counts without binding original carriers is rejected. New generic owner/alias,
RAII/CFG/effect/heap joins or normative rules would require Coordination.

Goal Alignment: removes an artificial library read requirement without giving
up original topology/failure choices. Actual author+ordinary-safe caller audit
locality versus factored C is unmeasured. Product GO PAUSED624 and H1 HOLD268
remain; neither P-native success nor P-CI substitutes for future independent
NON-BLIND calibrated R, whose authorization belongs to Coordination.
