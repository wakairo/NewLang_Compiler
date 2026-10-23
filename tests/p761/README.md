# P761 Stage2A: partial Copy-proof result, regression HOLD

Track P / fixed Compiler main `0eb528e6f3bc1dc2ce3f606690dc67be78fecbd4`,
CURRENT_SPEC -> canonical Draft17.34, C RELEASE6356093556.
**STOPPED on first unexpected full-regression failure. DO NOT MERGE.**
This directory preserves an incomplete implementation candidate and actual
measurements, not an adopted feature, all-green CI or mixed native result.

## Before production edits

- `53476d5c53239648409dc68992c67d51c867f239`: independent H-first/T-first
  sources, six negatives, independent oracle and SHA256 freeze (`freeze.json`).
- `330760f4865dfff6043a3ec2d31aa33b407abec9`: baseline measurements and
  A/B/C safety decision (`DESIGN.md`), still no production edits.
- `evidence/baseline-cli.json`: both positives ordinary CHECK0, ordinary C17
  CLI4 / zero bytes. Six negatives CLI3, individually classified by diagnostic.
- Baseline public Copy-body/parameters validator status3 = ANALYSIS_PRECISION,
  with real H type11 unknown/size0/align0 -> known/size24/align8 in source worlds.
  H/T distinct nominal IDs11/10; genuine source graph and original T facts exist.
- Integrated-main natural first-push FULL5 39889500192 (5/5) and distinct
  PARTIAL39889500624 were independently checked successful for this exact base.
  Base/main was rechecked unchanged at STOP. No R source is claimed fresh here.

## Minimal candidate, not fully validated

Choice A stages the **existing** supported-target H representation plan before
mixed Copy bodies snapshot types. `nl_allocated_prepare_layout` extracts the
old allocator metadata calculation; the actual H completion returns its exact
nominal TypeId to mixed transactional registration. It does not register an
allocation result type or create a value, place, backing, Allocation or Domain.
Unsupported host targets gain no layout facts; actual allocation's old platform
fence remains. Single-nominal staging is unchanged. B (new trusted equivalence
bridge) and C (new snapshot/type-epoch model) were rejected as unnecessary scope.

Production diff: `src/allocated_node.c`, `src/semantic_check.c`, one private
prototype in `src/semantic_internal.h`. Public Copy issuer/validator/API,
`nl_copy_same_types`, driver, emitter, toolchain, workflows and normative docs
are unchanged. `tests/p752/unit_test.c` migrates only the targeted heap-hook
precision expectation to require the unchanged existing validators to succeed;
its sources/oracles/first-failure evidence remain unchanged.

## Measured positive boundary

Both frozen new positives CHECK0 and validate genuine existing public Copy-body
and actual->formal receipts: type prefix identical, H layout1/24/8 in each world.
The actual source caller contains a binding of the old Copy returned by the
helper, followed by exactly one use as incoming data to a different lexical
original; this is checked in the owned source graph, not inferred from numbers.

| Source | Caller original field | Old -> new | Sibling | Returned Copy consumer |
| --- | --- | --- | --- | --- |
| limits_h_first | root1/child2/index0/inc1 | 32 -> 91 |168| separate saved.cap becomes32, sibling4, displaced219 |
| tuning_t_first | root1/child3/index1/inc1 |204 ->75|17| separate root4/child5/index0/inc4 becomes204, sibling199, returns6 |

Both real H allocation sites remain independently checked, with distinct
original root/incarnation/domain/backing; successful semantic MATCH worlds close
one and two originals as appropriate, and first-allocation failure has zero.
These are **semantic** 0/1/2 allocation cleanup arms, not guest allocator trials
or physical frees. The certificate is **T Copy only**, not a simultaneous H
ownership / dual-carrier certificate. `p752_two_original_heap_hook` now passes.

Read-only historical comparison also found calibration_batch's Copy receipt
valid, but archive_window's call59/root27/index1 remains public body/parameters
status3. That source's later heap-world placement was not diagnosed further
because of STOP. Do not generalize the new positive result to all mixed sources.
Raw receipts and the exact inspection source are preserved in `evidence/`.

## Negative and allocation-failure measurements

- Ordinary CLI negatives: wrong Domain P3-DOMAIN-MISMATCH; mismatched Allocation
  P4-ALLOCATION-MISMATCH; consumed Domain P3-USE-AFTER-CONSUME; escaped lexical
  ref P3-UNKNOWN-BINDING; Copy-as-ref P3-TYPE-MISMATCH. These are semantic errors,
  not parser absence. Wrong H self target is unsupported REC-SELF-TARGET.
- Supplementary ended-original loan source refuses CLI3/P3-STALE-POINTER.
- Supplementary foreign same-shape T and H controls have equal numeric IDs but
  distinct source owners/worlds. 32 proof/state/source mutations per foreign
  control refuse; genuine receipt restores. This is intentionally NON-BLIND
  calibrated internal tamper testing, not an independent R result.
- Wrong source/world/root/child/index/nominal/incarnation/fact/old Copy/loan scope,
  layout-known/size/alignment, H ptr target and source graph are challenged.
- Public body/parameter/event validators: 36 fail-at-N allocation positions each
  per foreign control, explicit OOM, unchanged current/handoff public snapshots
  and valid retry. Parser237 positions each; transactional registration1183 and
  1305 positions with preserved preexisting function/context and retry.
- Registration metadata inspection finds values/places/domains/backing and
  OneBacking/allocated-targets zero: no allocation authority minted by layout.
- Uncalled helper yields no caller Copy receipt. Mixed ordinary emission and
  named emission remain CLI4/zero C; preexisting sentinel survives, new target
  stays absent. No mixed C17 compilation or guest native run was attempted.
- `controls-freeze.json` is explicitly a calibrated supplement frozen **after**
  the first implementation; it is not relabeled as the preimplementation freeze.

## First unexpected red and mandated STOP

Full local frontend regression: **431/433 PASS, 2 FAIL**, first run77.30s.
`p634_historical_hash.unit` and `p605_arithmetic_comparison.integration` both
fail at `tests/p605/integration_test.py:56`, the frozen normalized whole-private-
header SHA assertion, before downstream arithmetic/native execution in those
particular tests. The single new private declaration changes that header.
Read-only inspection proves removing exactly that declaration from a copy
restores the entire exact base header (see `stop-header-inspection.json`).
This is a real candidate regression of the existing historical-integrity gate;
it is **not** reclassified PASS, a demonstrated arithmetic miscompile, or proof
that all historical behavior remains correct. No gate, manifest, historical
source/oracle or expected digest was modified. No implementation repair or
regression rerun followed the red.

Earlier/final-in-flight focused evidence: nine new tests9/9 PASS; local
Clang23.1.2 ASan/LSan9/9 PASS and UBSan9/9 PASS. Those were already launched
with the full regression; they are focused frontend results, **not** FULL5.
Full local GCC was13.3.0 frontend-only; repository Actions lock23.1.3 unchanged.
A transient Work execution transport disconnection during initial build was
recovered with the original files/build retained; it was not a test failure.

The Issue says “On first unexpected failure preserve first red, STOP and
report.” Therefore candidate Actions were not started (zero attempts). Existing
workflows run on every pull_request, including a DRAFT: creating the requested
DRAFT now would launch CI after the STOP. Preserve the candidate on the dedicated
P branch without opening a PR, and ask C to adjudicate the historical-header
integration boundary before any repair, PR or CI. No skip-CI marker, workflow
change, coverage reduction or retry was used. This is an incomplete deliverable,
not a terminal all-green Stage2A candidate. Main/PR755/R756 remain untouched.

## Reproduction (for a future separately released session)

The test commands below document what ran; they were not rerun after STOP.

```sh
CC=gcc-13 cmake -S . -B build-frontend -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-frontend --parallel 2
ctest --test-dir build-frontend -R '^p761_' --output-on-failure
ctest --test-dir build-frontend --output-on-failure --parallel 2
```

Local exact paths/configure/build/test output are archived. For sanitizers the
existing NEWLANG_SANITIZER=address/undefined frontend policy was used with
Clang23; no compiler flags or trust conditions were relaxed. The source/public
probes load actual source via the Production parser and semantic registration.
`probe.c` uses private type-prefix inspection only diagnostically; acceptance
and Copy obligations use the unchanged public validators. Tamper tests access
private state solely in their test executable and restore it before reuse.

## Goal alignment and next decision

A reusable existing-layout scheduling correction yields a real, narrow public
T Copy receipt in a mixed semantic program without weaker authority. This is
technical progress toward v0, not a measured ordinary-safe author+caller
UB-prevention audit-locality benefit or Product GO. Process4.2 N/A for a faithful
implementation of adopted Draft17.34/DI-018, with no selected source rule or new
proof authority. Product GO624 PAUSED and H1268 HOLD remain.

C should first decide whether to authorize a minimal header integration repair
(e.g. a private declaration placement that avoids touching the frozen header,
or an exact historical exception with full negative preservation). Neither was
attempted here. The archived historical-b precision refusal also needs separate
bounded diagnosis before broader coverage claims. After a future clean Stage2A
candidate and C-selected independent R, Stage2B would still need a genuine
source-issued two-carrier receipt tying T and H original ownership/cleanup
worlds to the same caller plus fail-closed emission, native0/1/2 failure paths
and new independent backend scrutiny. Stage2B was not released or implemented.
