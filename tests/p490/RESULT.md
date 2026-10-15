# P490 final local evidence — C1 physical read PASS; C2 precision boundary

Track: P. Exact production main d6e603e12591c2ff39e3e1b77c784681fec66033,
CURRENT_SPEC -> Draft17.30. Pre-execution complete two-source/oracle freeze
f673cdd3b1717d249a99fe8fbf716178a847e33f. No candidate/oracle rewrite, no third
source. Only adopted P471 A plus exact preselected post-match continuation(s).

| case | source SHA256 | CHECK / first boundary |
|---|---|---|
| C1 read | 9221eb7bb0a09b35e38db87c3e70658cc00819d9ebfbfd3e9587577f79e75b69 | CLI0, public/validator0, C17/native physical PASS |
| C2 reloan | c5c94d2ed32a5d60b01d009673ba182bb8be6c0a0aaba05193b34f56cd2e60b0 | CLI3 NODE-JOIN-POST-PRECISION16:5–19:6 |

C2 exact diagnostic: error(precision)[NODE-JOIN-POST-PRECISION]: joined Node link
has unknown variant and pointer provenance. stdout0. Parser/source spelling is
supported; normal join and read succeed internally but the NEXT match on joined
Unknown cannot extract a Some pointer. No complete C2-owned output, C17/native
or actual p reloan; stopped without trying to recover via another source. This
is conservative precision refusal, not a semantic-safety negative PASS.

## C1 public source-issued proof and currentness

Real source unit registration -> actual main() -> immutable owned main graph.
NLNodeJoinView match34 validates both issued and copied view through independent
public validator. Wrong changed-projection view rejected; original still valid.
Original lexical tail1/inc1 and head6/inc6, original implicit local identities
(Domain0/backing0, NOT explicit heap Allocation/Domain rights). Entry next field2,
sibling payload3. Entry input Some(original tail); actual Some binder q8 is a
Copy locator to original tail, not new liveness or owner authority. None arm no
mutation. Some arm actual replace4: F2->18, parent RF1->19, occurrence0->3,
old None/new Some(original tail). Sibling payload fact/value unchanged.

Joined post is intentionally **Unknown**, field F16/value19, parent RF17;
original root/incarnation remain live. C1 subsequent FIELD_READ36 authenticates
original parent1/inc1, child2/inc2 and EXACT post RF17/F16/value19/occurrence.
Its result Copy21 remains variant0/payload0 with no invented ptr provenance,
Allocation or Domain. The checker has NOT selected the runtime Some arm as a
concrete semantic continuation. Point-in-time public fields are compared to
owned public post snapshots, not final context liveness after lexical teardown.
Public source-issued graph/read assertions and independent join validation are
distinct checks; no fabricated read certificate or host-generated world.

## C17/native

C1 ordinary unmodified newlangc emission CLI0, **2728 bytes**, SHA256
**daac2db7d1d0db0475819f6b8448f9fed0cd0a97255bab4c3f257999d9c79705**.
Exact bytes preserved as evidence/C1.generated.c17.txt (extension prevents a
formatter from altering emitted evidence). No C patch or constant injection.
Control P471 A unchanged SHA c578ebdce71ffd46e30758903c28f69227876c7904264b3eec1914c7f6bdd050
CHECK/public/C17/native PASS;2545 bytes/SHA92359ec3c59d72c7286cd45fd28dd8f68302410d935d9417934609feeb123acc.

Local GCC14.2 and Clang23.1.2, strict -std=c17 -Wall -Wextra -Wpedantic -Werror,
O0/O2 plain and passive-observed, 8 native runs per host (C1+control), all PASS.
Local SDK is pre-existing; no bootstrap/lock edits. Hosted CI keeps its own fixed
current toolchain (23.1.3) and requirements. The same emitted C bytes are compiled
with/without existing test observer macro; guest code is never changed.

C1 passive const-only observer uses public-source-derived binding roles and
pre-frozen oracle. It reads actual original distinct head/tail addresses, original
stored tail_ptr, actual Some(q) binder, branch old-tail None, real tail self-link,
then actual joined_tail Copy Some(original tail) BEFORE final head reset; head
ends None, tail remains self-link, old head Copy retains Some(tail), payloads2/1
unchanged. No guest writes, synthetic runtime pointers or host semantic seeds.
C1 has no second p binder: that C2 operation is NOT REACHED. No unobserved p
identity or reloan PASS inferred from C1 or native process exit alone.
Raw public records, manifest, emitted C and host/native JSON are under evidence/.

## Safety, cost and scope

No independent new source negative (stale EndRoot/old Copy liveness/known None
as Some) was added: UNTESTED in this exact two-source scope, not silently counted
as PASS. Existing regression suite contains relevant tests. Copied wrong public
projection is rejected; read facts retain Unknown instead of laundering ptr.
No new OOM/rollback mechanism or tests needed for test-only change; established
main validator/OOM regressions remain in normal CI, not a new OOM proof claim.

Prediction->actual: C1 copies post Unknown safely and reads the genuine original
carrier natively; C2 stops before Unknown pointer extraction/reloan as predicted.
Reusable capability measured: post-asymmetric-join original-field read. Remaining
first gap: post-join Option variant/provenance sufficient for a new source match
and scoped reloan. One next step for C: decide whether preserving finite
correlated pointer alternatives or a sound refinement contract is worth its
proof/validator/backend cost; no general CFG/SSA necessity is proved here.
Product audit closure/author+caller burden benefit, heap/nonCopy owners, H1/STB,
Ring safe API are not measured or changed.

Only new tests/p490/** plus 9 CMake lines (one public probe and ONE CTest).
No compiler/include/issuer/validator/backend/adopted test/docs/workflow/toolchain
changes. §4.2 N/A test-only; canonical rules KEEP, no new selected semantics.
DRAFT/DO NOT MERGE evidence PR, at most one natural FULL+PARTIAL attempt; final
hosted results belong to PR description and terminal Issue490 report. No retry,
merge, self-close or other Track. Stop after final report; C owns next investment.

## Reproduction

```
cmake -S . -B /tmp/p490-build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/p490-build --target newlangc p490_public_probe p471_public_probe node_join_native_probe -j4
ctest --test-dir /tmp/p490-build -R '^p490_' --output-on-failure -V
```

The new CTest checks frozen hashes; C2's expected exact precision boundary is
explicitly labelled RELOAN NOT REACHED. Control and C1 each independently traverse
CHECK/public proof/runtime manifest/unmodified C17/strict O0/O2 plain+observed
native. Sanitizer CMake builds apply the configured sanitizer to native code too.
Green overall test means **C1 physical read PASS + expected C2 precision stop**,
not a reloan feature PASS. No CI state is predicted as already passed here.
