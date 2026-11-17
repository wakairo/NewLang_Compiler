# Track P local storage maintenance

The user prefers persistent `/workspace` HEAD repositories and periodic cleanup
during CI waits or after a successful PR, with development taking priority when
storage is healthy. Do not put the active HEAD repository under `/tmp`.

The data-only remote is
`wakairo/NewLang_Compiler:archive/track-p-evidence-20261117`.
It has no workflows or compiler tree. It must not be merged into main or used as
an implementation/CI approval. Production candidate and test budgets are separate.

Before a large build, inspect `df -h /workspace`. For a complete static-runtime UBSan/ASan matrix, reserve at least 4 GiB of byte space and 100000 inodes before building; feature-only builds need less. A 1 GiB reserve was insufficient in C1109. Remove
obsolete reproducible dependency binaries, CMake objects and static libraries
first; keep active tools, active HEAD, source, oracle, FIRST/RED, raw logs,
configurations and measured safety-stop ELFs. Record exact deletions and hashes.
Avoid independent F/R source/oracle/artifacts entirely.

Public-main-identical tracked archive duplicates can be removed only after both
the index and actual local bytes match an adopted public Git blob. Keep a restore
catalog and mark those exact paths skip-worktree; do not edit manifests or Git
history. The P1492 production worktree is unchanged by this maintenance.

Unique necessary old data belongs in Git/LFS before local retirement. Freeze a
lossless archive and per-file length/SHA256 manifest, upload on the data-only
branch, download to a fresh repository/cache, verify the entire archive and all
members, and recheck the original file set before removing it. Leave a restoration
pointer at its former path and save a receipt. Never substitute regenerated output
for original evidence. Do not remove data on upload, download or checksum failure.

Receipts are in `receipts/`; the original cleanup catalogs are in
`/workspace/p1492-c1103/apparatus/`. `verify_and_retire.py` records the exact three
historical directory relocations performed on 2026-11-17; it is not a general
recursive deletion utility. GitHub Release binary uploads were unavailable, so
the empty draft release was deleted and standard Git LFS was used instead.

Original P1492 C1103 implementation STOP was ad476afaf329bbdadbd49385a612c6fdfb5cc434. Its
host/CLI36, native28 and selected OOM16 budgets are exhausted. Storage maintenance
does not authorize another test, Production fix, PR, CI, READY or merge.

Subsequent Coordination C1109 separately authorized the same sole P to repair
only the historical test projection and complete regressions on the same branch.
R1 is c4997882f981db5ba89dfeb475fe6fc283db2cfb, tree
432c88701cb3cef799c8d6cc9518af2fdd836aa1; its standard Git bundle is stored here.
The separate recovery budget is host24 / direct native8 / selected OOM4. Original
usage and REDs remain immutable. Actual results and conditional DRAFT publication
belong to Issue1492/C1109 and its receipts, never to a storage-maintenance PASS.
