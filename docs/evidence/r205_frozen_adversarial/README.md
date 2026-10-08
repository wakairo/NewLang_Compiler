# Track R #205 — auxiliary GitHub Actions evidence only
This branch descends from frozen compiler aed9e7b7d49de338c570c8c6e89197f2251db134; no canonical files changed.
Workflow checks out a *second detached worktree* at frozen SHA and uses its exact pinned LLVM23.1.2 bootstrap.
All 56 inputs carry preregistered expectations in manifest.csv (15 A positive, 23 A negative, 18 B variants).
No NewLang emitted C is executed. Incorrect semantic admission of an expected-invalid source halts the corpus.
A parser/profile rejection is NOT counted as proof of the typed-owner R/D guard.
The execution job captures original stderr/stdout, exit status, source SHA256, diagnostic labels and frozen binary SHA256.
These are observations; independent R interpretation and any owned-state/rollback investigation happen after data collection.
Keep branch unmerged. Do not change compiler, Draft, tests, or compiler-ci.yml.
