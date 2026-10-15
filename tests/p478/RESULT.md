# P478 — partial checker spine, CHECK/JOIN PROOF GAP (HOLD)

Fixed baseline d6e603e12591c2ff39e3e1b77c784681fec66033, Draft17.30.
Pre-execution freeze 464cf9500387bef4bbf27a70e596f82ea70c1069.
Exact original P475 target SHA444cb2c22ec51d7c4cb991fe53398beac59553e9357c91d3b7a016103796c73f;
P471 A control SHA c578ebdce71ffd46e30758903c28f69227876c7904264b3eec1914c7f6bdd050.
No frozen source/oracle edits or substitute positive.

## Local implementation / measured first frontier

Production changes only semantic_check.c (+32/-7). Structural exact signature
predicate accepts a registered completed two-field Node Copy Option plus its
ordinary write ref, with the same Option argument/result. No function-name or
source-pattern match. The existing Unknown Option abstraction is used for
independent formals instead of a favorable None; it has no Some payload, root,
provenance or liveness authority. New-profile definitions are checked before
ordinary caller walks. Existing actual-context run_body/primitive argument
substitution, type/scope/conflict/Change/Reset, old return and rollback stay intact.
This is **not a completed named-call feature** and not an allow-list-only patch.

| frozen case | baseline CHECK | changed CHECK |
| --- | --- | --- |
| original named_link | CLI3 P8-SIGNATURE-PRECISION | CLI3 P9-CONTINUATION-PRECISION, match span13:5–16:6 |
| ptr_as_ref | not separately measured | CLI3 semantic P3-TYPE-MISMATCH15:66–15:67 |
| live_read | not separately measured | CLI3 precision P9-CONTINUATION-PRECISION13:5–16:6 |

All changed failures have stdout0. No ordinary C17 candidate execution after
CHECK failure, no complete target-owned certificate/artifact or native result.

Read-only root cause: nl_node_join_arm_check explicitly rejects REGISTERED_CALL;
Node proof issuer/validator operates on a direct arm REPLACE with incoming
constructor, arm-local lexical loan and arm-owned before/after snapshots.
run_body retains a separate owned child body with actual-context IDs, but no
ordinary call entry/argument/formal/return/effect validator is issued. Child
REPLACE annotations alone cannot establish the enclosing arm's entire state or
prove there are no unrecorded effects. The public join operation ID is currently
arm-local, while the actual operation is child-local. The C17 loan consumer also
requires a direct REPLACE and has no matching ref/Option function ABI.

A faithful continuation requires a qualified source-issued call/body effect
certificate, independent revalidation of real argument-to-formal relation,
complete body connectivity/hidden effects, Change/Reset and old Copy/nonescape,
then Node join consumption and a matching emitter. Existing public call-body
accessor alone supplies no such proof. This is a new trust boundary, not a field
copy or boolean bypass. Stop at the pre-registered proof frontier; no flattening,
forged authority, relaxed join guard or speculative C emission was added.
A finite future solution may be possible; general CFG/SSA necessity is NOT proved.

## Evidence and limits

Baseline unchanged main: original inline control CHECK/public issued join and
validator/C17 all0, 2545C bytes/SHA92359ec3c59d72c7286cd45fd28dd8f68302410d935d9417934609feeb123acc.
GCC14.2 strict C17 O0/O2 plain/unchanged passive observation all4 native PASS,
real q/original tail self-link, separate head, old Copy and payload2/1. Existing
P471 tests stay unchanged and run in the full regression suite after the patch.
That evidence is not proof of the named target.

Five new public-only unit tests: helper definition prefix (same frozen declared
helper, without caller); complete positive-gap and ptr-type failure rollback;
read-case precision classification; allocation-failure sweep over independent
registration. Helper-only definition succeeds with Unknown formals and exports
NO formal values/bindings/places/domains/scopes/occurrences/backing. This is a
unit test of definition registration, NOT a substitute positive main/native.
96 malloc/calloc/realloc fail positions return OOM with the full public snapshot
unchanged before success. Complete target and both rejected sources also retain
registration public state. No private checker layouts, host-seeded roots or fake
source-owned call artifact are used. Local new tests5/5, full PARTIAL288/288
passed in58.15s. Hosted single FULL/PARTIAL results are reported in PR/Issue.

The frozen read example did not yield a semantic safety rejection. Its held
reference is ordinary/nonexclusive and does not by itself promise immutable
current value, so the initial expectation of a conflict was too strong. It is
preserved unchanged, classified precision-only, and NOT counted as safety PASS
or unsafe acceptance. Pointer-as-write-ref is the one demonstrated semantic
negative. No replacement negative was invented to repair the oracle.

An offline test harness initially assumed semantic_create had zero prelude
functions; the definition-count assertion was corrected to baseline+1. Source,
semantic expectations and authority checks were unchanged. No CI retry used.

## Scope, cost and next frontier

No edits to canonical/docs/adopted tests/workflows/toolchain/public API/emitter.
No historical G1 transplant, new flags, general effects or owner mechanism.
Code and tests are a DRAFT/DO NOT MERGE partial exploration, not integration-ready.
The pre-registered next P9 guard was reached; a simple emitter fix is not the
remaining task. Further investment needs its own C decision on the finite call
certificate/Node join proof boundary and adversarial/OOM audit, then ABI/native.
Low-confidence follow-on estimate: 1–2 engineer days including proof/mapping
review and tests; not a measured implementation quote or a claim general effects
are mandatory. This slice's actual cost is recorded as the small diff, fixed
three-source measurements, five new tests, one local preflight and one hosted
FULL/PARTIAL attempt; no product-audit benefit is inferred.
Canonical17.30/H1/STB/V473/M474/Redis holds unchanged. No merge/Issue close/other
Track. One terminal Issue478 report then STOP.
