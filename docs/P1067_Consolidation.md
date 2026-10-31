# P1067 / C156: one main-based custody and H/T candidate

This is a bounded consolidation record, not a language or product authority.
Canonical main is `438170098d5b98096f1ad1f1f200dc1f326c75c3`;
CURRENT_SPEC selects Draft17.34. The fixed source tip is PR1063
`b49a5e24d9fa56d367e116ce037a067c7e1cca38`, tree
`5dd16c6582b9157bc7de5ffecb8818cab0b20955`.

## Goal and method

Make the repaired custody/current foundation and its dependent direct C17 H/T
consumer reviewable as one coherent candidate against current main. Preserve
real original H/T identity, Allocation/BackingRegion/LifetimeDomain, loan,
old-Copy return, all normal/failure cleanup and fail-closed unsupported behavior.
This task adds no Production behavior, source law, issuer, public API/ABI or IR.

The new branch starts at exact main. A local two-parent merge includes exact
PR1063 as its second parent. This is preparation on a new branch, not a merge of
any existing GitHub PR or a change to main. Keep main's Compass, Process and
B Charter byte-for-byte. All other inherited tracked inputs remain identical to
the fixed tip, including Production, CMake/workflows, every old source/oracle,
first RED, archive, external anchor and v1-v12 history. No history regeneration,
new prospective generation or test enrollment is needed for this docs-only
main delta. The stopped PR1050 sibling `9ed018db287e9e46cf17a2d570cf77de79a57f52`
must not be an ancestor.

The immutable 17-PR chain is in `tests/p1067/chain-v1.json`. Main and tip share
`fbbf8a0261669ce0e8920e5b019134268a6a58c1`; main has two governance commits,
the tip has 78 linear implementation/evidence commits. Retaining ancestry and
old blobs avoids adopting buggy intermediate states or rewriting their evidence.

## Two conceptual review packages, one final tree

| Package | Required implementation | Evidence and remaining trust |
| --- | --- | --- |
| Custody/current foundation, through PR976 | Private CHECK-issued H origin and per-parent canonical child ledger; transactional attach/reserve/commit/rollback; historical GET separated from current validation and canonical destruction; present callable and NULL-table guards | P965 original ownership494 / CHECK-ledger OOM502; P633 shallow observation, P817 historical GET/current reject; P974 callable/body/result/effect/owner/signature/world and V0 NULL guards. F76/F78 old unsafe findings are not approvals of their old PRs. |
| Dependent direct C17 and history, PR984-1063 | Postterminal T; same Change old-result forwarding; two helpers, two-normal UNIT structural join and dual field/root carriers; all-world closed H continuation and bounded H-body normalization; externally anchored history and T CTest registration | Retain original P982/P990 first RED, F81/F82 history repairs through PR1012, all subsequent P source/oracle epochs, original failed PR1063 CI and C152 repair. No R original fixture is imported. |

Actual implementation anchors: `src/h_origin_internal.c` (canonical custody,
origin and historical observation), `src/checked.c` (guarded borrowing and
canonical destroy), `src/copy_body.c` (historical/current proof split and current
callable), `src/main.c::v1_call_parameter` (NULL before indexing),
`src/if_field.c` / `copy_body.c` (two-normal actual/formal common poststate),
and `src/checked_c_copy*_internal.h` / `checked_c_node.c` (bounded carriers,
old-result def-use and H lifecycle consumer). The only public-header delta from
main is explanatory text in `include/newlang/copy_body.h`.

Do not adopt PR954/962/969 or the pre-PR1012 history state separately as if their
known findings were absent. F79/F83 and subsequent conditional static reviews
apply to their own fixed trees. Identical Production does not automatically
transfer an independent F disposition or establish source-to-C soundness.

## Finite supported boundary

The existing published P sources and oracles define measurements, not new laws.

| P-owned family | Expected observation on this candidate |
| --- | --- |
| P1062 original/reversed closed-H regressions | CHECK / genuine Copy GET / current / C17 succeed; two real H allocations have exact 0/1/2 cleanup before two independent T originals and two ordinary helpers |
| P1062 no terminal probe, explicit final terminal unit RETURN, producer direct result, unused failure-world read | Same existing-proof consumer succeeds without granting one successful world's authority to another |
| P1055 old-result forwarding | Pure bindings, tail or RETURN must trace to the same actual Change old Copy; incoming/equal constants/extra effects are not that result |
| P1020 UNIT | Two normal checked IF arms, actual/formal mapping and cleanup-normalized Unknown join; preserve the prior one-normal plus terminal RETURN family |
| Existing source/host negatives and OOM | Reject at their frozen layer; no output clobber, wrong owner/world/field admission, foreign B damage, duplicate destruction or leaked rollback |

The six P1062 positives and thirteen negatives remain their original complete
sources. Their original source-freeze has 22 inputs; no new NewLang source or
compiler oracle is invented for a behavior-identical merge. Historical oracle
corrections remain separate epochs and original RED stays RED.

Unsupported/unverified: arbitrary legal H/T compositions, general or early-return
CFG, a third helper, escaping observations, generalized owners/modules/privacy,
new IR/LLVM lowering, or universal source-to-C refinement. The closed-main
consumer still requires its original outer H match followed by two originals,
exactly two actual helper calls and independently proven all-world cleanup.
The aggregate later R1064 original POS-A/B native success strengthens the fixed
old tip only; a separately reported legal out-of-family C17 rejection remains
coverage debt with first private guard UNKNOWN. Do not open those R sources or
make a new shape-specific repair here.

## Pre-run integration checks and finite validation

`tests/p1067/plan-v1.json` and `pre-run-v1.json` are frozen before integration
execution. The release-specific opt-in audit checks actual Git ancestry,
main-authority blobs, all inherited tip blobs, frozen P source22 and a cache-free
clean checkout. Its six negative inventory/graph faults are in-memory tests;
they do not execute an unsafe compiler graph. The runner is not a permanent
freeze of future legitimate main development and is intentionally not a new
global CTest or workflow dependency.

Run from a full clean Git checkout, with build/evidence directories outside it:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -B tests/p1067/check_integration.py --self-test
PYTHONDONTWRITEBYTECODE=1 python3 -B tests/p1062/history_v12.py --check-git
PYTHONDONTWRITEBYTECODE=1 python3 -B tests/p1062/history_tests.py
```

Use existing pinned GCC13/curated Clang23.1.3/CMake, with genuine LLVM C API smoke
where locally available. Run semantic/public-current regressions before native,
then all existing CTests for GCC/Clang, strict native O0/O2 guest0/1/2 through the
unchanged P runners, owner/OOM/no-clobber, and ASan/LSan/UBSan. Build required old
probe targets, not only newlangc. On the two-CPU local quota, avoid concurrent
sanitizer/regression lanes and retain actual timeouts/first logs. No broad retest
of historical candidates or independent F/R launch is part of this task.

Only after clean local success and an honest scope/cost assessment, create one
DRAFT/HOLD PR targeting main. Maximum three distinct natural FULL5+PARTIAL sets
(six runs/eighteen jobs), mechanical packaging repairs only. Stop on first
credible all-green. No manual rerun/dispatch, old-branch rebase/force-push,
READY, main modification, existing PR merge or Product GO. A real
semantic/owner/loan/C17/native/host-UB defect, authority drift, old evidence
corruption or unreasonable implementation growth stops this task.

## Maintenance, alternatives and adoption debt

This consolidation removes repeated stacked adoption decisions, not the actual
dependency closure. The tip/main comparison covers 14 src/include paths
(+2162/-186, net1976), 2558 changed test paths and about 59.8MB test logical
bytes. These are snapshot/audit-size signals, not Git pack size or measured
engineering hours. Keep the existing T hook: it is pinned by old custody tests,
registers two synthetic tests and zero newly approved R cohort tests, and grants
no source authority. Removing it or historical archives during consolidation
would create a second semantic/provenance review problem.

Historical design audit: N/A, faithful integration only. KEEP Draft17.34
§16.3.8, §18.1a-b, §27.3 and DI-017/018; REPLACE NONE; DEFER public source,
issuer/API/ABI, privacy/IR redesign. Current main's product/process authority
remains exact.

VALUE-ALIGNMENT-RISK: one main-based review surface is useful preparation but
does not measure reduced UB audit/re-audit dependency closure for library authors
AND ordinary-safe callers against well-factored C. Compiler, observers, history
and support restrictions retain real maintenance cost. Comparative human effort,
performance, representation-hiding API value and adoption benefit are UNMEASURED.
Well-factored C and natural Rust safe facades remain practical alternatives;
honest NO INTEGRATION plus the fixed feature snapshot remains valid if safe
convergence fails. Product GO stays PAUSED. Durable original R raw Git custody
is separately outstanding; Library backup is not Git custody, and this P task
does not transport or re-run R's original evidence.

