# P713 — original fixed-field IF join candidate (DO NOT MERGE)

Authority: released main `e917906f0416bf6132af3b5649a4f7fc5bdf47ba`,
Draft17.33, C's M3 RELEASE in Issue713. No historical PR510 code is imported.
This is finite compiler capability evidence, not independent R/formal proof or
Product GO. H1 HOLD and active Product scoring PAUSED remain unchanged.

## Before implementation

`freeze.json` freezes two independent source families, TRUE/FALSE variants,
numerical oracles, declaration index, sibling and changes per arm. The measured
unchanged compiler stops at CHECK3 `P6-JOIN-PRECISION` for all four; no C.
`evidence/baseline.json` records spans and classifications.

The very first authoring missed mandatory IF condition parentheses. Original
sources, freeze and parser diagnostics remain in `evidence/initial-syntax/`.
Only parentheses were corrected and then re-frozen **before Production edits**;
numerical expectations did not change. That syntax failure is not the join
barrier. Supplementary repeated-write/negative inputs in `extra-freeze.json`
were frozen before their first execution, after primary implementation; they
are not represented as original unchanged-main evidence.

## Implemented common mechanism and trust boundary

* The real source checker retains entry, both actual arm worlds and ordered
  direct fixed-field Change/read checkpoints, all owned by the registered IF.
  It admits the exact same original nominal root/child across two normal arms,
  including a genuinely unchanged arm. Names, field index, byte constants and
  number of writes are not admission keys. Existing availability checks precede
  this join; other scopes, places, domains and backing facts must be unchanged.
* In the **common** entry world, existing dependency validation and fixed-field
  Change create a fresh unknown typed u8 and structurally rebuild the nominal
  parent. Original root/child incarnations and sibling facts are preserved.
  No arm-private ValueID/ValueFact is copied into common state. Both real arms
  are checked even if the condition is statically known. Same final bytes still
  require new parent/child facts (`repeated_true`: 72→9→72).
* `newlang/if_field.h` exposes borrowed **historical observations**, not live
  ref/Allocation/Domain authority. Observation alone is not validation; NULL
  candidate validates the actual issued receipt. Output structs stay untouched
  on missing observations. Lifetime is the retained checked owner, not a wire
  serialization format.
* The public validator binds private source/body/owner identity, full sealed
  caller and arm graphs, original entry/post and arm worlds, normal-arm index,
  exact original field, scope, access, event order and old/incoming/result Copy.
  It independently walks all arm source nodes once and replays each Change and
  the common structural join with the core fixed-field rules. Missing, duplicate,
  reordered, stale or foreign evidence fails closed. Immediate subsequent reads
  are tied to common post until the next potentially changing source operation.
  Private issuer seals/anchors and host memory integrity remain trusted; hostile
  simultaneous rewriting of trusted internal issuer storage is not a theorem.
* The driver requires public validation and authentic constructor→entry original
  binding→physical C carrier mapping. A receipt cannot fall back to the old pure
  IF path if invalid or OOM. The emitter uses an actual C if/else and actual field
  addresses, forwards actual replace-old Copies and executes guest comparison
  operands. It does not pick an arm using cached checker truth.

The old M1/M2 snapshot/trio/plain/type-prefix helpers are shared **privately**;
public M1/M2 APIs and semantics are unchanged. The new private receipt attachment
requires a narrow P605 historical-header normalization: exact unique two-field
block at `NLCheckedFragment` start, then the unchanged original SHA. P634 tests
missing/duplicate/misplaced/wrong-type attachment attacks. No old hash manifest,
NewLang source, numerical oracle or historical evidence is rewritten.

## Measured cases

| Cases | Original declared fields | Changes then/else | Runtime current / old / sibling |
|---|---|---|---|
| both_true / both_false | coast(0), drive(1) | 1 / 1 | 205 or 17 / 111 / 42 |
| one_true / one_false | threshold(0), marker(1) | 1 / 0 | 64 or 8 / 8 / 193 |
| repeated_true / repeated_false | low(0), high(1) | 2 / 2 | 72 or 61 / 9 or 240 / 11 |

All six use ordinary `newlangc --check`, source-issued public validator, ordinary
C17 emission, strict GCC13/Clang23 O0/O2 native. Unmodified generated C is run
separately from passive observations. The observer checks original physical
field addresses, each executed old/store, IF result, postfield and sibling;
it supplies no guest state, branch result, ref or authority. Synthetic wrong
comparison/guest field/old capture controls use that same observer. An unchanged
runtime arm executes no store/capture, so no unexecuted corruption is claimed as
native rejection. The first mistaken mutation expectation and correction are
preserved in the evidence logs.

Negative source: wrong replacement type, incompatible arm result and escaping
loan are semantic rejections with rollback/null artifact/zero C. Different roots
or fields stop at conservative `P6-JOIN-PRECISION`, not semantic illegality.
The supplementary `noncopy_availability.nl` stops earlier at unsupported
`ALLOCATED-SOURCE-PROFILE`: it is **not** a successful source test of availability
join. Existing `if.unit` availability rejection regressions and actual-artifact
availability corruption tests remain separate evidence. `sole_normal.nl` is
CHECK0 but backend4/no C; the terminating edge is not counted as a two-normal
proof. No new heap/nonCopy source admission or return CFG lowering is claimed.

Public/producer tampering covers wrong root/child/index/incarnation/source world,
parent/child ValueFact, equal-byte different Copy IDs, sibling, missing/reordered
Change, cyclic trace, terminating arm, foreign scope/ref/access and stale reads.
Test-owned real source artifacts are mutated; no host semantic world seeds a
positive. Registration and actual-call fail-at-N OOM leave public snapshots
unchanged; validator/admission OOM propagates as resource failure; whole driver
OOM produces zero C, and semantic failure preserves prior named-output bytes.
The existing host parser/checker OOM diagnostic is CLI3, not misreported CLI1.
Emission allocates nothing after admission and `/dev/full` fails honestly.

Old P497/P504/P514/P633 legitimate structural-IF precision expectations advance
to genuine public validation and native regression. Their sources, old freezes
and old evidence remain unchanged. These are not relaxed malformed-source tests.

## Reproduce

```sh
cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
ctest --test-dir build -R '^p713' -V
./build/newlangc --check tests/p713/both_true.nl
./build/newlangc tests/p713/both_true.nl > both.c
cc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 both.c -o both
./both
./build/p713_public_probe tests/p713/both_true.nl
```

The normal full build/CI still requires exact curated LLVM23.1.3; frontend-only
runs are PARTIAL. Local Clang23.1.2 is identified separately from CI's pinned
Clang23.1.3. ASan includes leak detection; UBSan is fail-fast. No workflow,
bootstrap, lock, canonical, Process or public arithmetic proof is changed.

## Cost, historical audit, limits

This needed a complete issuer→public replay→emitter vertical addition rather
than deleting the flat-Copy guard. Reused core Change, structural reconstruction
and M1/M2 snapshot helpers; the new public proof and graph replay carry ongoing
maintenance obligations. Each IF records O(E·S + G) snapshot/seal memory and
replay work for E events, semantic world size S and checked graph G. Calling
individual event validators repeats full receipt validation. Existing entry/depth
budgets and OOM paths bound allocation; this is not a compact general IR.

Measured both-write receipt retains 8 historical worlds (2 pre-existing arm
worlds), 103648 bytes of flat anchors and 81216 graph-seal bytes. Asymmetric case:
7 worlds, 83992 / 74304 bytes. Repeated case: 13 worlds, 209192 / 127872 bytes.
These counts **exclude** nested semantic/registry/source allocations. Local
launch+exec timing/RSS samples are in `evidence/local-cost.json`, not an isolated
allocator measure or scalability promise. Native code has no receipt runtime.

Historical §4.2: **KEEP** Draft17.33 §§16.1,18.7,27.2–27.4 current structural
place/currentness/normal join semantics and DI-017 nominal metadata. **KEEP**
DI-001 projection/operator split and DI-006/007 mode/nonescape restrictions.
**DEFER** final unified loan spelling, general alias/CFG/SSA, heap custody,
nonCopy owner join and general effects. M709/C release establish this as faithful
implementation; historical unmerged PR510 CHECK-only and Node MATCH are not
adopted proof authority. No source rule or normative semantic choice is made.
Full inaccessible historic chats were not audited or declared nonexistent.

Goal Alignment: selected v0 branch state can now be expressed directly without
moving stores outside the IF merely to fit the old emitter. This is reusable
compiler capability, **not** proof of author+caller UB-audit locality improvement,
ordinary-safe library completeness, H1 or product adoption. Unchecked, C-side
manual update or pure-value-only workaround would not establish this selected
source capability. Major concern remains snapshot/proof maintenance cost and
limited composition: public replay covers direct scoped changes/reads and Copy
bindings in two normal arms, not nested calls/IF/heap operations or terminal CFG.
Independent BLIND R of this new issuer/backend boundary is for C to authorize;
P does not initiate it. Stop after the sole final Issue713 report.
