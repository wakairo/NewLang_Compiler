The original new CLI OOM oracle expected exit1 everywhere. Actual existing
render_parse_failure/render_check_failure return3 for parser/CHECK failures,
including their explicit OUT_OF_MEMORY status. The prior P1107 probe already
observes API phases and requires that exact mapping. No production code changes.

The prospective OOM harness now passively wraps the same real API boundaries.
Setup/backend OOM must return1; parser/CHECK OOM must return3 AND the observed
API must return the exact OUT_OF_MEMORY enum. A broad nonzero/rc3 allowance is
not used. API rollback/NULL publication and named bytes/inode/mtime/no-temp and
exact disabled-fault retry stay mandatory. All initial source/oracles and the
real first RED remain exact. Each reached allocation index/phase is logged.

History26 fixes the complete old-v25 SHA/Git OID; all original history stays
immutable. P1107 appends harness11 for the legitimate CMake successor only.
