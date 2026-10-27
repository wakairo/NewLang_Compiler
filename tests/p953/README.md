# P953 — authenticated pre-producer Copy originals

Stacked on P946 `9aaf352f34d66aa3b99a10fff056ddfd41c3eb9b`, Draft17.34.
`echo.nl` is the exact independent R949 frozen source, SHA256
`005a09a8fa2d16aa0de012faea2b2b1cef1ed91a9ed50f0c1f326d39a2f2d1f9`.
Its original R INCONCLUSIVE/CI0 is unchanged. Other files are separately frozen
P controls; `window_true.nl` also reverses the T declaration's field indices.

The new backend lane validates the actual completed Copy caller graph, exact
constructor/binding/IF/loan forward chain, literal initializers, original H
initialization and current Env/affine carriers. It borrows existing entry facts
only within their owner's lifetime. It does not issue a capability or borrow a
future producer's return to authorize an earlier operation. Existing producer,
owner, Copy validator and legacy admission lanes remain mandatory.

Five tests cover actual public Copy proof and tamper reject/restore, selected
reached driver OOM plus retry, ordinary CLI bytes, strict raw C17/native and a
passive original-carrier observer. The extra observer check requires both heap
originals initialized, no producer and no H field Change when T binds. All reads
of original C locals occur while those objects are live; finish uses saved data.
Real malloc NULL worlds yield allocations/frees0/0,1/1,2/2. T executes only in
both-Some. A regular-file write/flush failure is reached using RLIMIT_FSIZE;
the existing output bytes/dev/ino survive and healthy retry reproduces raw C.
Separate close/rename fault sites are not tested.

Reproduce with frontend-only CMake and `ctest -R '^p953_' -V`.
`run.py CLI CC none|address|undefined tests/p953` accepts explicit GCC13 or
locked Clang23.1.3 and runs O0/O2. Expected numbers are observer assertions only;
the observer never initializes guest state. Full public APIs, arbitrary H/T
interleavings, source semantics and Product GO are outside this candidate.
