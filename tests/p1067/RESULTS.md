# P1067 / C156 local integration results

DRAFT HOLD, preparation only. Fixed main
`438170098d5b98096f1ad1f1f200dc1f326c75c3`, fixed source tip PR1063
`b49a5e24d9fa56d367e116ce037a067c7e1cca38`.
The tested local merge is `4dcd417a7edba5fc5d2cd461955c5530646c62bd`, tree
`b3a2d583dff6be49aa052438b4d10c6758d08405`, parents exact main + fixed tip.
Later publication changes add this result and raw evidence only. Final
publication HEAD/tree and natural CI are recorded in Issue1067 and PR metadata.

## Converged scope

The automatic three-way merge had no conflict. Main's Compass, Process and B
Charter are retained exactly. All inherited tip blobs except the explicit main
governance overlay are unchanged, including every Production file, old test,
source/oracle, first RED, archive, CMake/workflow, external anchor and v1-v12.
The stopped PR1050 commit is not an ancestor. One new P branch is used; existing
branches/PRs and main remain unchanged. No new NewLang source, issuer/API/ABI,
language law, IR or shape-specific lowering is introduced.

The integration audit verified 5038 inherited entries (tip5037 plus the main B
Charter), actual main-first-parent/tip ancestry, excluded sibling, source22,
clean/cache-free checkout, and six in-memory inventory/graph negatives. Original
v12 current freeze and five original history negatives plus the C152 ambient
input negative pass without regeneration or a new generation. The pre-run
Issue comment6536819832 predates branch creation, integration and execution;
all four frozen inputs and their manifest remain byte-identical.

`evidence/production-identity-v1.json` records all14 main-to-tip src/include
paths and their exact SHA256/Git blobs: +2162/-186, net1976. Every one is
identical in this integration. The only public header difference from main is
comment text; its non-comment tokens match. Actual full recursive Git inventories
also confirm 2581 main-to-tip changed paths, 2558 tests, 59,795,844 tip-side test
logical bytes. This is audit/data size, not Git pack size or engineering time.

## New local measurements on the integrated tree

| Tool profile | Semantic/current/owner preflight | Remaining existing tests | Total |
| --- | ---: | ---: | ---: |
| GCC13.3 Debug | 64/64 | 528/528 | 592/592 PASS |
| Curated Clang23.1.3 Debug | 64/64 | 528/528 | 592/592 PASS |
| Clang23.1.3 ASan/LSan | 64/64 | 528/528 | 592/592 PASS |
| Clang23.1.3 UBSan | 64/64 | 528/528 | 592/592 PASS |

The batches are disjoint; all four complete named sets agree. Every profile
includes the real curated LLVM23.1.3 static C API smoke, not just frontend-only
registration. This infrastructure smoke is not a NewLang-to-LLVM backend.
P633/P817/P785/P790/P974/P965 and history safety tests run before native. All
required old targets are built. Native tests retain strict C17/Werror/pedantic
GCC/Clang O0/O2; guest budgets0/1/2 are actual allocation/failure observations.
There are two existing synthetic T tests and zero newly enrolled R tests.

Per compiler/sanitizer profile, actual full logs record:

- 494 original P965 ownership trials, including unrecovered A destruction and
  foreign B survival, plus502 reached canonical-ledger OOM positions across
  CHECK and definition paths. Exactly-once ownership and rollback assertions pass.
- Six independently published P1062 source positives with genuine GET/current,
  36 physical traces (two optimizations x guest0/1/2 x six sources), 144 total.
  Original H allocation/free identity and complete teardown precede two original
  T carriers; old returns, updates, sibling and caller use assertions pass.
- 2750 reached emitter injection positions:2634 OUT_OF_MEMORY and116 conservative
  UNSUPPORTED outcomes. Output remains unpublished, snapshots/original ownership
  are preserved, and exact retry succeeds. These are host faults, not guest
  allocation failure counts. Final no-allocation close is not called reached OOM.
- All13 frozen source negatives, current/owner/world/foreign-body/arm negatives,
  named output/inode/no-temp preservation, and44 old C byte identities pass.

No actual ASan/LSan/UBSan runtime diagnostic occurs in the saved full logs.
Pinned clang-format23.1.3 passes. The same source/runner/oracle is used across
all profiles; completed batches are not rerun. One regression lane runs at a
time on the two-CPU quota, with the original per-test timeouts unchanged.

## First diagnostics and mechanical environment repair

The first GCC full build exited2 because the local host lacked the unversioned
`libxml2.so` development link for the existing LLVM smoke. The installed
SONAME2 runtime was present. A task-local symlink plus per-command LIBRARY_PATH
resolved that existing runtime, SHA256
`8f2643af68a9eba917929046806dabd1848867068d0eee0f6799288a8e886963`.
The same candidate then built; no source/CMake/oracle/toolchain version changed.
The original failed build remains RED in the raw archive. GCC13/Clang23 compile
the C17 compiler/native tests; system g++14.2 links the LLVM C API infrastructure
only. CI's existing Ubuntu prerequisites provide libxml2-dev normally.

A broad Git whitespace diagnostic also reports inherited frozen raw whitespace
and final blank lines in two new pre-frozen non-C files. Its first console output
was truncated, not presented as a complete saved FIRST. Full same-tree
reproductions are explicitly labelled REPRODUCED. Original raw and pre-run bytes
are preserved, not normalized to silence a diagnostic. This is separate from
the passing pinned C formatter and compiler/safety tests.

All earlier P/F/R first failures stay historical. In particular the original
PR1063 HEAD0502751 first six CI FAILs and the C152 Git inventory repair are both
retained unchanged; their results are not regraded or inherited as this tree's
PASS. The new complete raw ZIP is `evidence/local-raw-v1.zip`, 1,090,463 bytes,
95 entries, SHA256
`d4689a2ad3b9ae96025ef50aa32a77ab98d2c21c12a19944c097c46d741e016c`.
Its internal hash manifest is verified; results JSON SHA256 is
`e4381fa366f4f7c7b7ef625e7a7c1b1d5d974ca70754ebb6c2bbcd3f9704d0f1`.

## Adoption boundary and residual work

The support matrix in `docs/P1067_Consolidation.md` matches the unchanged P
fixtures/implementation. This is a useful bounded candidate, not general H/T
composition or source-to-C soundness. F76/F78 custody/current repairs and
F81/F82 historical anchors are carried together with their dependent consumers,
not adopted as known-broken intermediate PRs. F91's conditional static result
and R1064's aggregate native milestone remain results on their fixed old tip;
no new F/R disposition or execution is claimed for this integrated tree.

Legal out-of-family rejection, arbitrary/early RETURN CFG, third helper,
escaping observations, generalized owners/modules/privacy and new IR remain
unsupported/unverified. R original sources/oracles/runners/raw were not opened
or copied. Separate R raw Git custody is not certified or repaired by this P task.
READY, existing PR merge, main modification and Product GO are not performed.

VALUE-ALIGNMENT-RISK: one main-based review target reduces fragmented adoption
work, but retains the real compiler/observer/history/audit maintenance closure.
Author AND ordinary-safe caller UB audit/re-audit burden versus well-factored C,
natural Rust alternatives, performance and product adoption value remain
UNMEASURED. Preserve the honest NO INTEGRATION/snapshot alternative if adoption
cost or an independent safety finding overturns this finite technical result.
