# Local evidence capture qualification

The repaired full frontend verdict is the original complete872-line
`evidence/full-frontend-first-repair.log`: all433 test verdicts,433/433 PASS,
82.41s. It was captured directly during the one real test run.

The subsequent **inventory-only** `ctest -N` regenerated CTest's temporary
LastTest.log before its archival copy. Consequently the three-line file named
`full-frontend-first-repair-details.log` contains only the inventory's start/end
markers, NOT the full run's per-test stdout. It must not be cited as complete
full-suite detailed output. No such output was reconstructed and no tests were
rerun to fill this capture gap. The separate raw full verdict log remains intact.

The193-line ASan and UBSan focused detail logs were copied from their separate
build directories and retain actual source-issued Copy proof, tamper/OOM,
metadata and CLI output. The original pre-repair first-red detailed log remains
unchanged under ../evidence/, as checked by the preserved44-file manifest.
This qualification adds no compiler/test/expected-result changes.
