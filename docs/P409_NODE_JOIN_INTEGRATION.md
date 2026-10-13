# P409 — existing-v0 Node join and scalar call integration

Track: P. [Issue #409](https://github.com/wakairo/NewLang_Compiler/issues/409).
**DRAFT / DO NOT MERGE.** This is integration of already-tested implementations,
not a new Node backend, language rule, general ABI/CFG/owner mechanism or product
GO. Canonical CURRENT_SPEC remains Draft17.30. FormalProof/main
`a8de4ebb4099f4742fc161d26867740dcac48414` is not proof of the combined code.

## Immutable inputs and path audit

At task start live Compiler/main exactly matched
`37b03455b78c1bcc79826d59a5f42972a81d351b` (merged PR404).
Original PR401 HEAD remains `36611db41cc2be37fdd85007981ce3630c4e282b`,
parent `348ca7d48f6dd5d658706f4547ab7ef9a63af33d`; its branch was not modified.
Original PR404 code head is `bb6d8b58447b7d61f5fe2243e430a9aa1ef4d8f2`;
its squash merge supplies current main. The new branch is
`p409-node-join-integration`, based on that current main.

`git cherry-pick --no-commit 36611db41cc2be37fdd85007981ce3630c4e282b`
applied all 16 original paths cleanly. Git automatically combined CMake; there
was **no conflict and no manual resolution**. The source patch was committed
separately from this integration-only diagnostic evidence.

[Path audit](evidence/p409/path-audit.json) checks all paths of both immutable
diffs from their common original parent:

- PR401: 16 paths. All 15 non-CMake files are byte-identical to fixed PR401,
  including production `semantic_check.c`, `semantic_internal.h`, `sum.c`, its
  tests, fixtures and historical evidence/report.
- PR404: 29 paths. All 28 non-CMake files are byte-identical to fixed PR404,
  including `main.c`, scalar call tests, README, reports and evidence.
- Only shared path is `CMakeLists.txt`. The result equals current main CMake
  with **exactly** PR401's original 16-line registration block inserted at its
  original Node marker. No duplicate targets/tests; PR404's 6-line block remains.
- No normative Draft/public header/workflow/flag modifications; no R-only
  PR403/407/408 tests or O Python imported. Integration-only files under
  `docs/evidence/p409` and this document add no compiler functionality.

## Fresh combined-tree observations

Local clean GCC Debug build: **263/263 tests PASS, 33.58 seconds**. Registration
contains all 14 original PR401 tests (8 source-owned variants, 5 controls,
1 source/CLI suite) and PR404's one source-owned/native suite, without duplicate
names. This is new combined-code evidence, not a reuse of either prior CI.

[Node observations](evidence/p409/node-source-owned.json) come from the original
P401 test/probe applied to the combined compiler. All eight unchanged sources
register/check and expose real source-generated owned arm/continuation evidence.
Both original local Node identities/incarnations, field/parent ValueFact updates,
Some occurrence ending and sibling currentness are retained. Same Copy locator
still refreshes field F17→F25, occurrence P2→P4, parent RF18→RF26; the common
parent RF24 is parent-issued. Every new changed-match source remains
**CLI4 / V1-BACKEND-UNSUPPORTED / C0**. No Node join native execution is claimed.
The unchanged-frame baseline still emits 2209 C bytes.

The added test-only [scalar source](evidence/p409/scalar_edges.nl), SHA256
`38202e8d8df56e01a1aba78356b9ba6038dc2617eaef6a2d04bc8fda081ce86c`,
uses the same ordinary callee with 0,255 and caller-received Copy values.
CLI0 emits 920 exact C bytes, SHA256
`7213285045495110cc72cb74d02a3eedaf261b38afaae3db71509836f8f0f901`.
[Verbatim C](evidence/p409/scalar.generated-c.txt), [GCC](evidence/p409/scalar-gcc.json)
and [Clang](evidence/p409/scalar-clang.json) record strict C17 native O0/O2,
both plain and test-copy read-only post-call observations **0,255,0,255**.
The source-issued owned probe reports four distinct fresh formals and eight
fail-closed artifact controls. No callee, runtime result or argument is replaced.
This supplemental local test reuses unchanged P404 helpers and is not added to
CMake; the original 263-test registration is preserved exactly.

[OOM observation](evidence/p409/oom.txt): original P401 injection covers
registration=384 and call=235 allocation positions with rollback on all failures.
The full suite includes original negative/dependency/private-ID/owner and
scalar-call diagnostic/mutant/snapshot controls. No error or safety guard changed.
Standard fresh combined-head five-job CI is linked in the PR and final Issue
report after it completes. Local results are not substitutes for those CI jobs.

## Reproduce

Use README's existing checksum-locked toolchain, without workflow/pin changes:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python3 tests/integration/node_join_source_test.py \
  build/newlangc build/node_join_test tests/fixtures
python3 docs/evidence/p409/recheck_scalar.py \
  build/newlangc build/u8_call_checked_probe gcc
python3 docs/evidence/p409/recheck_scalar.py \
  build/newlangc build/u8_call_checked_probe clang-23
build/node_join_test oom tests/fixtures/node_two_normal_join.nl
```

## Gates and independent trust boundaries

Historical §4.2: **N/A — faithful mechanical integration**, selected
Draft17.30 §§13.5a/27.3 and scalar §§18.1/19/24/27 unchanged. Compass → Product
Strategy/Coverage Map and Process §§4.1–4.3 checked; §4.3.1 Exploration GO is the
authorized composability test, not new Product investment. No design selection
or Ledger update is made. Stop on a non-CMake implementation conflict or any
need for semantic/emitter/API changes; neither arose in the mechanical apply.

R402 fresh BLIND INCONCLUSIVE remains historical. R405 NON-BLIND calibrated
owned PASS BOUNDED and R406 BLIND native PASS BOUNDED qualify their separate
frozen trees/scopes. They are **not independent R validation of this combined
tree**, nor proof of hidden dependency/heap-owner/general join/native Node
safety. The new test-only local observation is P evidence, not another R phase.

Coordination decides adoption. Do not merge old PR401 and this same patch twice;
old PR401 remains frozen OPEN DRAFT during this task. No merge, self-close,
extra Track launch or scope expansion. H1 original five fallible roots,
6+4+2/B.prev=B/full B+dst nonCopy/0–5/B1–B7 remain undecided/H1-only deferred;
STB six allocator conditions remain HOLD.
