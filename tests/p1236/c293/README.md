C293: Git-native v0 safety CI (Coordination #1245)
================================================

Production remains exactly PR1241's original 2ccbce8. Its original six jobs
(42438133813 / 42438133841), three timeouts, original FIRST/raw, and the abandoned
C291 v63/65 FIRST remain evidence; none are regraded. No v63/v10/v15 is adopted.

Normal builds retain all functional tests. The 27 explicitly identified SHA,
archive and versioned-history-only registrations in legacy-audit-selection.json
use newlang_add_legacy_audit(), enabled with -DNEWLANG_LEGACY_AUDIT=ON (default OFF).
This is explicit registration, not DISABLED/WILL_FAIL or a name-based exclusion.
C tests named evidence/history/proof/OOM still run: these exercise real compiler
ownership, graph authenticity, source/world rejection, teardown and rollback.
Full/front normal registration is 811/810; opt-in restores all 838/837 names.

The active P990→P1045/P1030 and P1045/P1055/P1058/P1062 runners condition only their final historical
hash-chain subprocesses. Source/CHECK, real public current/owner, raw and passive
native, physical H/T identity, guest0/1/2 cleanup, calibration, malformed graph,
foreign-B survival, reached OOM/no-clobber/retry, and old generated-C byte equality
stay unconditional. P1055 profile0 retains every substantive assertion and all38
old generated-C identities; its ten pure history mutation cases are opt-in.
The unchanged history scripts/manifests remain tracked at their original paths.

The P1188 signature adapter conditions only the global SHA-chain replay. All120
short/foreign allocation rejection, output preservation, B-survival and retry
trials still execute. P1208 C268 wrappers condition only inputs.verify() (SHA/
archive/current-generation replay); the original direct inputs.py remains intact.
Every C266 source/owned/current/C17 byte-equality/negative/census, native O0/O2,
and reached OOM/rollback assertion remains active, including per-binary census
and before-injection source/binary/plan digests. Safety oracle bytes are unchanged.

CTest explicitly exports NEWLANG_LEGACY_AUDIT=OFF/ON to mixed subprocesses.
For a direct mixed runner use NEWLANG_LEGACY_AUDIT=ON to request its old audit
substeps. The direct historical scripts themselves retain their original behavior.
Those old current-manifest audits can honestly refuse a later edited tree. To
audit original bytes, use a separate Git worktree at the reported original commit
and its original commands, e.g. git show 2ccbce8:tests/p1055/history_tests.py.
ON is a diagnostic request, not a promise that old hashes describe today's tree.
Do not copy old manifests, rehash them or reinterpret diagnostic failure as PASS.

Only p1055_old_origin_0.integration gets TIMEOUT180 through P1236's deferred
property; all other timeouts, run serialization and functional commands stay exact.
Git commit/parents/tree/diff plus real fixed-HEAD safety results are the normal
provenance. Neither Git nor green CI proves general soundness or Product GO.
Product Value Strategy1.1 author/caller audit-locality benefit vs well-factored C
remains UNMEASURED; this operational change removes administrative work only.

C294 closes P990's indirect P1045/P1030 call, not just direct CTest scripts.
P990 still executes its P1013 and P1020 functional helpers and the complete
P1030 current/native/owner/negative/H/OOM/rollback and29 old-C comparisons.
Only the latter helper's final five SHA-chain mutations become opt-in. No
P990 command, semantic check, physical observation or safety oracle changes.
The earlier local GCC811 run was not bound to its published tree because a
helper changed mid-run; that error and CI RED remain preserved. C294 results
are new executions on one sealed Git tree, immutable throughout testing.
