# R1185 execution equipment freeze E3

C247 explicitly releases only compiler faad092b5b0deb608a048ff60bee0056082f9a59,
tree 01388ae82b1749453546933cb4ca6b311a25df77, direct parent
6be8c524b7a745d0d67a082e4cc2f794f90a3cd7. Release:
https://github.com/wakairo/NewLang_Compiler/issues/1185#issuecomment-6656900527

This is the complete initial R execution equipment, frozen before publication
and before any R FAST, ordinary CI, compiler/native or AST generation. Its text
is complete; formatting, compilation, registration, CLI and actual AST shape
are still untested. No successful observation is claimed by this freeze.

## Preserved inputs and independence

V1 remains NOT_RUN. V2 source/oracle/calibration are byte-identical to the
previous server-readback freeze. The same six positive, one control and eight
negative cases are used. No new input, expected value or safety question was
selected. V1's removed alias-specific claims remain removed in V2.
Independence: pre-execution frozen independent V1 / scope-calibrated V2;
C-supplied finite-profile boundaries.

E0/E1/E2 snapshots, receipts, source bytes and the old d743 STOP remain retained
outside this prospective delta. Their old target fields are not rewritten.
The new target.json is a separate explicit execution binding. Snapshot and
driver require GitHub Actions. Local compiler/native/build/AST generation
remain prohibited.

## Equipment and finite evidence contract

- public_probe.c: genuine source load, function-unit parse/registration,
  ordinary source main() CHECK and traversal of actual source-issued owned
  public views. Raw IDs are qualified by artifact/world. No private header,
  hand-built AST, scalar/root seed or manufactured authority. Whole-loop
  validator is NOT_RUN / NO_APPLICABLE_PUBLIC_API.
- observe.py: only actual emitted C and its authentic Clang AST are inputs.
  The observer inserts logging at existing compound boundaries. Original bytes
  remain in order, and restoration is checked. It adds no guest call, condition,
  argument computation, store or replacement main. Repeated variable spellings
  are unobservable. Nested calls are never hoisted. A call-site marker requires
  one direct call initializer and only safe already-initialized direct arguments
  or literals. Pre-store logging is address-only; existing guest old-load values
  are logged only after the guest computes them.
- compare.py: role decoding receives only the actual observation map and raw
  trace. Frozen oracle comparison is a later operation. No expected value selects
  a variable, field or event. The initial finite shape includes an inline normal
  u8 IF join, one plain loop, direct fixed field stores and immutable pointer /
  scalar Copy paths. Unknown or ambiguous forms, including currently unsupported
  out-of-line helper layout, are R_DECODER_INCOMPLETE. See DECODER-CONTRACT.md.
- driver.py: whole raw argv/rc/stdout/stderr and all-FIRST retention; ordinary
  CHECK, applicable public views, C17, authentic AST and original/passive diff;
  strict GCC/Clang O0/O2 and separate guest ASan/UBSan, plain/passive. Native
  binaries are hashed before and after execution, with at most 84 runs per
  host job. Invalid negatives never reach native. Abnormal host exits or
  sanitizer reports cannot be counted as language rejection.
- snapshot.py: the permitted fixed-context plumbing adapted to the explicit new
  binding. It recreates authentic objects, HEAD, stage-0 index, paths and modes
  from the fixed target tree and verifies clean worktree. No online fetch or
  shared object store. All original fixed P bytes are mechanically carried,
  never inspected as source/oracle design input.
- register.cmake: original test argv use the authentic fixed source/repository
  context; original tests/assertions/timeouts remain unchanged. Exactly one
  r1185.helper_result_current.integration CTest and one public probe target are
  added. Only the new test gets its own timeout and baseline environment.
- export.py: full member index and bounded raw gzip/base64 export through natural
  CTest logs; transient binaries are hash-recorded and excluded. No workflow,
  toolchain or production changes. Root CMake changes by one registration include.

C assignment boundaries do not prove optimized physical STORE survival,
especially for same-bits replacement. Inline semantic flow does not prove a
machine CALL, physical callee frame or ghost freshness. Those machine facts
remain NOT_MEASURED. Run-local pointer relationships never become absolute or
cross-run address equality. Missing observability is not semantic PASS.

## Authority, allowed inputs and publication sequence

Compass, Value Strategy 1.1, Process 7 and CURRENT_SPEC -> Draft17.34 remain
anchored to authority main 63658cd546be8b99599baf19476486a475adb320.
Only the four explicitly allowed old plumbing files and necessary public
headers/root CMake/build context/three workflows/format config were used.
At the new target all public header/workflow/format/hook blobs equal the prior
allowed versions; the new root CMake and fixed metadata are fetched at faad and
Git-object-hash verified. P/F implementation, source, fixture, oracle and reports,
and other old-R equipment/input/report remain unread.

The whole E3 delta is hashed and recoverably recorded on Issue #1185, with
complete server readback before publication. The first ref will point to a
complete R commit whose sole parent is faad. No empty P-head R ref is created.
R rechecks the prerequisite P natural FULL42010094182 + PARTIAL42010094219
metadata and fixed branch HEAD before that ref. Same R HEAD actual FAST success
(format/GCC build/unique CTest) precedes one Draft NEVERMERGE PR to
p-c245-helper-current and natural ordinary CI.

FAST maximum 6 actual jobs; ordinary maximum 12 actual jobs; manual 0.
No budget is reset by this edition. First trustworthy all-green stops further
push/CI. Pure-text replay of identical Actions-generated C/AST is allowed for
safe finite equipment repair, with original versions/raw retained; it does not
authorize local compiler/native/AST generation. Any material or boundary doubt
is affected STOP to C.

No runtime result or audit-locality improvement is established here.
audit locality UNMEASURED / GO624 PAUSED / H1#268 HOLD / P1036-PR1050 STOP.
