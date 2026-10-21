# R675 first BLIND run preservation / mechanical repair

Original R HEAD: cca81903f880bee455bf18b9f3dd29e905808fbe
Direct parent: compiler candidate 7189475f6aec9e7ccaa1edda4db3ebf6b9b58665
Original source and semantic oracle SHA: see untouched BLIND_FREEZE.md

FULL5 initial attempt1: https://github.com/wakairo/NewLang_Compiler/actions/runs/39458253019
PARTIAL initial attempt1: https://github.com/wakairo/NewLang_Compiler/actions/runs/39458252999
Clang job 118750668001: R-owned public_probe.c clang-format failure; exact Actions patch generated.
PARTIAL job 118750657613: initial test #53 failed only in R observer regex: original C carrier count 0 != 1.
Initial index0 source CHECK rc=0, stderr/stdout empty; public owned rc=0, original root ID1, distinct children2/3 indices0/1, meaningful post-Change ValueFacts; emitted C17 rc=0, 1272 bytes.
No strict C17/native at first R HEAD (observer failed before compilation). Rejection cases NOT REACHED.
Other 364/365 PARTIAL tests passed; full GCC/GCC Release/UBSan each 365/366, with R-only test failure; first Clang format fail prebuild.

R mechanical-only repairs:
- Apply exact pinned clang-format suggestions to public_probe.c (unchanged observer logic).
- Correct blind_test.py C initializer regex for possible const qualifier and .f1 then .f0 named initializer order, preserving two unique field-index numeric oracle and real original carrier requirement.
- Original seven .nl source blob SHA and original numerical/semantic/physical oracles remain frozen. No producer test/oracle/rationale consulted, even during repair.
- Candidate emission source code was narrowly read AFTER original source-oracle freeze for harness diagnosis, not producer test data.
- New HEAD/new run are separately attributed, never retroactively called first-run PASS. No compiler/issuer/validator/canonical or CI workflow modification.
