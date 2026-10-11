# Track P — Issue306 frozen-compiler evidence (UNSELECTED)

This is bounded NewLang compiler source/ownership validation, not a compiler repair or a new source rule.

- Compiler code and original tests are byte-identical to PR302 `b28a49755f1bc254b6a8f7c337e5e1866d9483b8` (six opt-ins ON).
- Canonical main `a5343bf70004b30a8b026e52aa2fde93b5920be0`, Draft17.30, particularly §14.5; FormalProof main `a8de4ebb4099f4742fc161d26867740dcac48414`.
- PR298 is not incorporated. PR302 remains DRAFT/DO NOT MERGE, Draft17.31/DI015 remains unadopted/HOLD.

`held_B_next_None.nl` allocates the original five physical roots, establishes the original six links and explicit allocation-failure cleanup, packages original B into ordinary complete nonCopy `DetachedHolder { held: LiveRoot }`, consumes/transfers/destructures into fresh locals, re-loans the same original Domain, calls the two-formal helper exactly once for B.next Some(C)→None, ends the scope, re-packs/destructures, and explicitly releases all five originals. Holder is a semantic stack value, never a sixth allocation. This smaller update is the explicitly authorized fallback: the preferred post-move helper B.prev Some(A)→None first reaches P297-CALL-PRECISION; the unchanged profile supports a single helper @next only. The earlier direct B.prev None preparation comparator additionally reaches CAPTURED-CLOSURE-PRECISION. No detach/adopt or H1 PASS is claimed.

`owned_observer.c` uses only public headers/accessors and actual parser/register/check-expression output. It validates all five captured allocation closures and the scoped call, compares the original captured B with the caller before/after Change snapshots, observes the old consumed/current new lexical Domain carriers, the new loan on the new place, unchanged original A/D/BackingRegion/root incarnation, fresh Some occurrence/ended old occurrence, unchanged B.prev, and scope closure/repack/exact terminal release. Source spans are read from each checked fragment's own retained source, not assumed to be offsets in the outer compilation unit.

`run.py` replays each CLI case twice, then observes registration/rollback independently through the existing public boundary tool. Positives require a non-NULL source-generated owned artifact/public validators; CLI exit4 means backend unsupported/no C. Semantic refusal, unsupported source, and precision are separate. Old borrowed payload source stops at P9-MATCH-PRECISION before mutation, so it is not an observed semantic rejection of stale payload use.

OOM sampling is 7 allocation indices per registration, entry check, and public validator (21 injections), verifies unchanged semantic snapshots/no failed owned result and successful same-context retry. It is not exhaustive. The existing controlled `full-scoped-poison` test examines a separately created artifact from the new source, applies 10 invalid evidence modifications, requires both public validators to refuse them, restores the data and verifies success; this never creates source admission or supplies owner facts.

## Reproduce

Build the unchanged compiler with all six experimental flags ON, then:

```bash
bash evidence/p306/build_observer.sh build none
python3 evidence/p306/run.py build/newlangc build/p306_owned_observer build/five_root_source_gate_evidence build/scoped_field_evidence build/p306-observations.json
build/captured_closure_test full-scoped-poison evidence/p306/held_B_next_None.nl
```

The independent evidence-only workflow runs GCC Debug/Release, Clang, ASan/LSan and UBSan, all 271 existing regressions and these observations. It asserts protected compiler/tests/docs/standard CI files match the immutable PR302 head. Final CI disposition belongs in Issue306.

## Historical comparison — Gates A–C, not N/A

Reviewed fixed canonical CURRENT_SPEC and development process §§4.1.1/4.2/6/9/10, Design Decision Procedure, Ledger, M266/M305 source routes and the updated Holder-first Coordination adjudication, P297/R304 actual source→owned limits, and F59's adjunct-only result. No new admission/API/semantic rule is introduced here.

| Prior intent / alternative | Disposition and reason |
|---|---|
| DI006/007/010: deferred general loan spelling, mode-preserving ref projection; older four-mode loans and ptr@field sketches | KEEP current explicit same-D scoped loan/ref projection; DEFER general syntax/implicit ptr dereference. Copy ptr cannot grant safe authority. |
| DI009/014: original physical grants, exact original A/R/D, three separate fixed fields and six initial links | KEEP five originals and every explicit cleanup. No extra physical root or implicit Drop. |
| DI011: independent known function check and actual caller obligations | KEEP existing two-formal one-replace helper and public body-sensitive caller substitution; no function/type-name authority. |
| DI012/013: original nonCopy whole carrier/return/custody and §14.5 identity | KEEP complete whole transport; ordinary TreeD logical value versus special DetachedHolder has no authority difference. DEFER generic containers, nonCopy Option handling and two-owner receiver join. |
| M266 Holder / M305 TreeD (design-only, UNSELECTED) | KEEP ordinary complete nonCopy staging without extra heap node. Holder-first follows updated user/Coordination priority; identical one-field TreeD cannot confer different authority or erase ceremony. DEFER full receiver ownership/H1 route. |
| Proposed DI015/Draft17.31 and M305 full 4+2 route | DEFER, unchanged merge HOLD. This single moved-D reloan does not select a whole H1 profile. |
| F59 Domain-value lexical loan adjunct | Research evidence only; KEEP selected core unchanged. No hidden/global Domain registry or new guard introduced. |

The Ledger references older Surface Drafts/module/M0 experiments; those raw legacy files were not present in this fixed worktree and no complete M9 chat history was recovered. M305's history classifications and accessible canonical/experimental source were compared; absence is not treated as proof that no older alternative exists. No intentional replacement or foreclosure is proposed; final normative selection remains Coordination's responsibility.

## Holder-first measured ceremony

The primary source is 248 physical lines versus 237 in the immutable P297 calibration (including comments/blank lines): **+11 lines**. Temporary ownership adds **2 ordinary nominal declarations**, **8 explicit whole-value statements**, **12 lexical receiving names**, **4 aggregate construction expressions** (including nested reconstruction), **4 whole destructures**, and **1 explicit whole binding move**. The read-loan and write-ref/helper sequence is the existing one-scope pattern, not an additional second helper. The allocation source still has exactly **5** original physical allocation sites: **0 additional heap H allocation sites**. This is source/checked-accounting evidence, not a measured runtime instruction/byte cost: the native backend refuses this source, so erasure/ABI/runtime zero cost remains unknown. The optional one-field TreeD comparator changes only the wrapper type name in the tested Holder source and produces the same public identity/currentness observations; it adds no authority or measurable ceremony advantage.

Preferred `held_B_prev_None_blocker.nl` is parsed but registration returns precision status3 / P297-CALL-PRECISION at the single helper invocation, not a semantic rejection of B.prev. The finite unchanged signature/caller gate requires index0 (@next), so the primary legal fallback clears **B.next Some(C)→None**. This proves moved-original-D reloan and one field update only. It does not establish B.prev, complete B detachment, later adoption, or H1.
