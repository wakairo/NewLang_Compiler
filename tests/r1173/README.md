# R1173 independent changing-current cohort

C242 authorizes this R-owned cohort against compiler commit
`870e4f0972abb9725b7cf9f38d09178e51ff1280`, tree
`5db3576a09f2627f8975a21295419dee6433a629`. All compiler, build, native, and SDK
operations run only in the existing GitHub Actions workflows. The R branch is
`r-fast/r1173-changing-current`; its draft PR is NEVERMERGE.

## Independent input and evidence

`oracle-v1.json`, `source-freeze-v1.json`, and the fourteen source files were
chosen from canonical Draft17.34 before reading P/F implementation, fixtures,
oracles, reports, rationale, or PR1171's body. Six finite positives distinguish
zero, one, and multiple backedges, both selected fields, and a same-bits write.
Eight negative sources distinguish exact type/arity/field, write access, scoped
escape, representability, and the wrong guard arm. A precision rejection is
recorded at its actual layer, not credited as a later authority proof.

`execution-freeze-v1.json` separately freezes this execution machinery before
any semantic/native execution. The probe registers the actual source unit and
checks a real `main()` call. It neither seeds semantic roots/references nor
constructs private proof records. Public checked worlds are owner-qualified.
The probe records published loop/field facts and checks published freshness
when both before/post facts are available. It does not claim that native
addresses prove semantic identity or that same-bits values prove fresh facts.
No matching public validator for the four additional mutation intentions was
found in the inspected API; those mutations remain explicitly NOT_RUN.

The passive observer derives original record, declaration-order fields, actual
carry/result, loads, and store from generated C syntax, without oracle input.
It inserts only `fprintf` calls reading live lvalues at existing sequence
points. Unsupported spelling is a machinery NOT_RUN, with original generated C
retained. It requires separate actual before/old/after/sibling loads to complete
the observation. Native execution of unmodified C and passive C are separate.
GCC13 and Clang23 run strict C17 at O0/O2. The ASan and UBSan jobs additionally
instrument three meaningful multi-edge/same-bits guests, in both forms and both
optimization levels; host sanitizer success is not guest sanitizer evidence.

Every command, return code, stdout, stderr, source, oracle, generated C,
observer mapping, semantic views, native observation, and checkout identity is
retained under the build's `r1173-evidence`. FIRST is never retried or rewritten
within a job. `export.py` publishes a lossless tar.gz as bounded base64 lines to
the natural CTest log, including successful jobs via CTest's post-test log
observer. FAST configures/builds/enumerates only and does not run this cohort.

## Fixed-P Git machinery

The root CMake change is a single enrollment include. Inherited test commands
retain their arguments, assertions, and compiler targets; only source paths
under `tests/` resolve into the R-created fixed-P checkout. R's own test is added
once separately. No test is excluded and no shared workflow is changed.

`snapshot.py` verifies every fixed P blob and executable/symlink mode against
the canonical recursive tree. The sole root-CMake exception is restored from
the exact fixed-P byte copy. It materializes canonical Git blob/tree objects
and the exact hash-verified original commit, reads the canonical tree into a
real stage-0 index, then verifies the complete index and clean worktree. It does
not infer tracked state from a directory scan or `git add`. It does not mutate
the Actions checkout's index, refs, or worktree. The isolated repository has a
one-commit shallow boundary; ancestor commit history is not claimed present.

After the independent source/oracle freeze, the T1166 fixed Git prototype and
its mechanical README were used as equipment reference. The old R1163 CMake
registration was read only to understand how inherited history-test arguments
can retain fixed-P bytes; its source, oracle, driver, and reports were not read.
Public headers and shared CI/CMake registration are the other machinery inputs.
T1166's synthetic result is not credited as real P integration evidence; that
integration is checked by this branch's natural Actions.

## Budget and boundaries

Natural FAST maximum 6, including first branch creation. The completed first
R commit is created without a ref and only then receives the initial branch.
After same-HEAD FAST success and unique registration, one draft PR targets
`p-c238-changing-current-loop`. Natural FULL5 plus PARTIAL consumes 6 actual
jobs, with at most one mechanical repair set, total 12. Manual rerun/dispatch,
force, rebase, and amend are zero. Stop at the first trustworthy all-green.

Semantic/authority/original-identity/native/sanitizer anomalies are affected
STOP and returned to C. An unexplained failure is never relabeled machinery
merely to continue. The cohort does not establish general soundness, audit
locality, Product GO, or readiness for merge. Existing hold/stop decisions stay
with C.
