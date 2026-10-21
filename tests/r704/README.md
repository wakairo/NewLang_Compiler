# Track R #704 — independent BLIND fixture and observation

The initial independent sources, expected guest values, and exact LF-byte SHA-256
are frozen in Issue #704 first comment (#issuecomment-6314440150), timestamped
**before** examination of candidate implementation or any P701 source/oracle.
The source fixtures here must remain unmodified in BLIND first pass. The script
checks all frozen hashes and fails on drift.

Two ordinary main() calls invoke a helper which changes a *different*
lexical original: `ledger_old.nl` returns 37 (old field value after
37->182), then caller stores 37 over 6; `switch_current.nl` returns 22
(current value after 147->22), then caller stores 22 over 5. Distinct field
indices, sibling values and identities are included. One read->write negative
must reject before C emission.

`source_probe.c` loads real source through the already integrated
`tests/support/node_checked.h` loader, reads authentic source-issued public
temporal event checkpoints, validates them and tests mutations on copied public
observations. It does not modify private issuer records or inject authority.

`blind_native.py` retains the byte-exact emitted C17, compiles it into a
different translation unit and executes via its own C17 consumer. Separately,
a passive copy adds only assertions on real C carriers and physical fields.
The original guest values are never seeded or rewritten by the observer.
Both run at O0 and O2 on the configured compiler.

CI: the preexisting five-config (GCC Debug/Release, Clang, ASan, UBSan) PR
matrix, plus independently runnable LLVM-independent frontend-only CTest.
No producer source fixture or oracle was consulted in this first pass.
A copied public view rejection alone is NOT proof that the private
issuer-history resists mutation. R first-pass findings remain in Issue #704.
