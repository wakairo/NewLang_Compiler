# P506: Stage 1 source-owned structural candidate / Stage 2 HOLD

DRAFT / DO NOT MERGE. Base PR503 `e561e8f189fd67961f5ac90560e20ecc45752abd`;
source freeze `501e918`. No P504 change imported, no source/semantic selection.
See frozen.json for exact base tree, sources, SHA256 and pre-execution oracles.

## Actual boundary

Unchanged opt-in PR503 compiler: both original full Pair inputs CHECK CLI3,
`error(precision)[P6-JOIN-PRECISION]`, source 7:5–8:70. No owned result issued.
All eight baseline outputs and binary digest are in baseline.json.

Candidate requires BOTH `NEWLANG_EXPERIMENTAL_USIZE_EQUALITY=ON` and the new
`NEWLANG_EXPERIMENTAL_PAIR_IF_JOIN=ON` (each default OFF). Original frozen true
and false source now CHECK CLI0 through the ordinary newlangc `--check` entry.
No stdout/stderr. candidate.json records CLI results/binary digest. In normal
CI only a dedicated semantic test variant enables Pair joining; original
P501 precision expectations and every adopted source/oracle remain byte-identical.

Exactly one original implicit-local Pair, original `@left`, two normal unit
arms, each one actual scoped replace, closed scope, no other persistent effect.
Both independently checked arms authenticate same original place/incarnation,
old Copy, before/after field and parent ValueFacts. After restoring ONLY the
transition's ownership metadata in temporary comparison clones, the existing
complete persistent-frame comparator rejects every unrelated change. These
comparison clones are never committed or exposed. Both checks finish before
the parent candidate creates anything. No blank Pair TOP/flat_copy relaxation.

Parent uses existing `nl_fixed_change` to attach a fresh unknown-but-valid u8
left and unchanged right to a fresh complete aggregate, with fresh left/root
facts. No arm-private ID imported. Calls `nl_sem_validate` before continuation.
Public actual-source artifact observations after AST/input destruction:
root place/inc4, left place/inc5; both arms root fact4→9, child5→8, old0,
incoming7/8, scope1 ended. Parent root fact8, child7, left Copy unknown,
right Copy9. Fact numbers can coincide across worlds: always qualify by the
actual arm/context, never infer identity from equal numbers. Function cleanup
then ends original lexical places; field-read nodes retain point-in-time
facts and the ended aggregate retains correct member IDs. Final state is NOT
misrepresented as a live post-IF world. owned.log records both actual outcomes.
No Option field exists here: no conditional payload occurrence/Reset is invented.

## Negatives and transactions

wrong_root, wrong_field, extra_change: P6-JOIN-PRECISION; canonical legality of
broader different-target joins is not denied. Invalid else, despite true guard:
P3-WRITE-REF-REQUIRED. Escaping write ref: P8-EXIT-DEPENDENCY. The attempted
whole-Pair read loan fails earlier LOCAL-LOAN-PROFILE (unsupported), so no live
Value(root)-dependent-ref source test was reached. All six registration errors
leave the entire observed public semantic snapshot unchanged. Parser succeeds
on all eight frozen sources. Captured candidate diagnostics: candidate.json;
public transactions: rollback.log.

Every malloc/realloc/calloc failure before first success: registration317,
actual main()200; OUT_OF_MEMORY, NULL checked result where applicable, exact
public rollback, then identical source succeeds. No validator OOM result claimed.
Existing Node MATCH validator refuses this IF; it is NOT reused as Pair proof.

## Exact Stage 2 blocker and STOP

**HOLD — SOURCE-ISSUED PAIR JOIN PUBLIC VALIDATION NOT IMPLEMENTED.**
Existing public IF views expose two owned final arm contexts and operation
facts, but no qualified common-entry / immediate parent-joined-post witness.
In particular lexical cleanup zeros current root/member facts and detaches
fixed children. The source-connected read node and ended packages observed
above are not independently replayable public proof of that intermediate live
aggregate/carrier/scope state. Existing Node proof requires a different MATCH
and Node profile; accepting its labels here would be false authorization.

A trustworthy next Stage 2 needs issuer-bound point-in-time common entry and
post, actual connected LOAN→REPLACE→old Copy→scope close in each qualified arm,
and an independent read-only full-frame/aggregate validator plus corruption
and OOM controls. This is a separate estimated 400–800 implementation/test LOC,
120–180 minutes of additional trust-boundary work; not a thin existing-public-API
adapter. Do not enlarge this candidate to force proof within the bounded task.
No new public API, witness, validator, approved flag or extra retained proof-world
clones were introduced. Planned wrong-root/fact/private-world/old-Copy/sibling
public-view poison cases are **NOT RUN / no Pair validator**, not claimed rejected.
Stage 1 is an experimental CHECK candidate only, not independently validated
soundness, native success or adoption readiness. Independent R launch remains C.

## Reproduce and scope

```
cmake -S . -B /tmp/p506 -DNEWLANG_FRONTEND_ONLY=ON \
  -DNEWLANG_EXPERIMENTAL_USIZE_EQUALITY=ON \
  -DNEWLANG_EXPERIMENTAL_PAIR_IF_JOIN=ON -DBUILD_TESTING=OFF
cmake --build /tmp/p506
/tmp/p506/newlangc --check tests/p506/full_guard_true.nl
/tmp/p506/newlangc --check tests/p506/full_guard_false.nl

cmake -S . -B /tmp/p506-tests -DNEWLANG_FRONTEND_ONLY=ON
cmake --build /tmp/p506-tests
ctest --test-dir /tmp/p506-tests --output-on-failure
```

Local GCC14.2 frontend-only: 298/298 tests passed; targeted owned/rollback/OOM
and frozen hashes passed. Natural CI results are recorded in final PR/Issue
report, not predicted here. No C17 emission/native execution attempted for Pair;
backend, public API, Node proof, source syntax, canonical, ledger, toolchain,
workflows, PR503/504/479/494 untouched. No merge, self-close or other Track.

Goal: compiler-checked original guarded object currentness. Capability gained:
finite structural source join, unknown left Copy / stable right after two arms.
Safety: six frozen source controls and exhaustive issuer OOM. Evidence missing:
independent public proof and physical IF lowering. Product/audit-locality
improvement vs factored C/Zig/same-topology Rust remains UNMEASURED. Alternative:
existing direct scoped writes/scalar selection, or separately budget the above
public witness before any original-Pair physical IF. Coordination owns next gate.
