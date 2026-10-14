# P432 public Node join proof exploration

DRAFT / DO NOT MERGE. Fixed start b4db6aa73c3dc730b860c8b3b6103be86ac31b58,
CURRENT_SPEC Draft17.30. Branch p432-public-node-join-proof.

Preflight GO (recorded before compiler edits): the existing finite original
Node arm comparison can be shared; separately owned pre-fork, pre-pattern and
immediate joined checkpoints can be retained, with a new read-only public view
and verifier that independently rederives the parent post using ordinary
Change/Reset. Predicted medium/high review risk, one finite proof surface,
roughly 400–700 implementation lines and 200–400 targeted test lines, no
CFG/SSA or heap owner join. First falsifiers: stale enclosing root RF despite
same Copy locator; foreign arm with identical numeric original IDs.

Unchanged baseline: same-locator and both-arm fixtures source/check/owned PASS.
Same-locator F17→F25, P2→P4, RF18→RF26; parent common RF24. CLI4,
V1-BACKEND-UNSUPPORTED, C0. The missing edge is an immutable common entry and
immediate parent post with qualified forks, not source semantics.

Historical gate N/A: faithful implementation of Draft17.30 §§13.5a/b,17.4,
26.6/7,27.3/4; KEEP DI006/007/009–014 original scope/projection/lifetime rules.
M414/P412 and merged PR410 checked. Additive diagnostic checked-owned contract
only, no source selection or ownership grant; Ledger update not needed.
Compass/Product Strategy: architecture preserved, mechanical proof bookkeeping
made consumable. Existing private trace/guard stays; native lowering, general
proof framework and Product GO remain deferred. Issue432's bounded Exploration
GO permits this falsifiable pilot; no user burden or product win is claimed.

STOP on first unresolvable lineage/post corruption, generalized effects/owner
mechanism or expanded semantic choice. No emitter/normal_frame_unchanged change,
workflow/bootstrap/lock change, merge or other Track launch.

## Issued contract and revalidation

`newlang/node_join.h` is an additive borrowed diagnostic contract, not authority
or backend permission. `validate(NULL candidate)` validates the issued proof;
a copied public view may be supplied to detect foreign/modified candidate data.
Candidate pointers are nominally matched before dereference. The owning fragment
and its context must remain live. There is no importer, setter or public issuer.

Issuer retains a common pre-fork entry, each exact pre-pattern fork, and immediate
parent post. Existing point-in-time captured Change snapshots are reused for the
one original scoped Replace per arm. Unchanged arms have zero Change snapshots.
A proof owns 4 + 2W context clones for W=1 or 2 writes; arm-world pointers borrow
owned checked arms. Partial issuance is destroyed on transaction failure. A
linked list associates proofs with match IDs without imposing a new source cap.

Validator rechecks owner/match/world anchors, exact fork-prefix equality,
checked parent/arm graph connectivity, actual scoped write-ref operand and
constructor/pointer evidence, fresh F/RF and P retirement, and the shared exact
original-frame/dependency kernel. It separately replays each concrete Replace
from its source-issued immediate before checkpoint and compares the complete
immediate after. It then creates dependency-free abstract Option on a clone of
common entry and replays the ordinary parent Change/Reset, binding refresh and
scrutinee end; complete equality against immediate parent post is mandatory.
No private arm suffix is imported, no concrete arm selected, no ptr liveness or
nonCopy authority created. The certificate's original roots are lexical:
BackingRegion=0, implicit domain (root/incarnation), not heap Allocation/Domain
packages. Existing heap-owner rejection remains unchanged.

## Evidence and limits

`docs/evidence/p432/source-owned.json` records unchanged source SHA256, real
public probe/CLI outputs and OOM/corruption evidence. All four required fixtures
validate; all eight old Node fixtures remain source-check positive. Required
four retain CLI4/V1-BACKEND-UNSUPPORTED/C0. No changed-arm C17/native success.

Public-only tests first validate intact issued proof, then reject 15 copied-view
poisons including foreign same-number world, stale RF, old P, wrong root,
truncated/duplicated/reordered arms, missing Change and post-function-as-entry.
Supplemental trusted unit corruption modifies the actual issued owned snapshots
or checked graph and calls the same public validator: stale joined RF, poisoned
entry, EndRoot, changed sibling, forged concrete None, correlated old P,
precise Change post RF, foreign arm context, disconnected operation/parent
match, wrong scoped Domain all reject; each member is restored and intact proof
revalidated. No test-side issuer or seeded ownership. Borrow lifetime is explicit:
validation after free is outside the C API contract, not a tested rejection.

Local strict GCC Debug whole suite: 274/274 PASS (51.87s), including 23 targeted
family tests. Public validator OOM sweeps: 112/164 failure positions for one/two
writes, nine/eleven public worlds unchanged (including precise Change checkpoints).
Existing source registration/call OOM sweeps now include calloc and report
661/489 (same locator), 695/530 (both arms) positions, all failures unchanged. Four proof
worlds plus concrete Change snapshots add real memory/allocation cost; this is
not an O(1) certificate. Normal five-configuration CI will recheck exact PR HEAD.

Prediction→scope: ~500-line verifier/issuer plus ~400 targeted test lines and a
small shared-kernel refactor; no general CFG/SSA/owner framework. Additional
precise Change checkpoints and finite graph connectivity were needed to prevent
an arbitrary fresh RF or disconnected operation from certifying a source effect.
Maintaining and independently auditing this new public trust boundary is still
medium/high risk. Coordination/R adoption review is required; Product HOLD,
H1/STB frozen contracts and existing normal_frame_unchanged guard remain.
