# P746 — M7 single-unit admission boundary (NO PRODUCTION CODE)

Track: P. Result: **HOLD — SOURCE PROFILE / MIXED DECLARATION ADMISSION**.
This is compiler feasibility evidence, not a new source rule, CHECK/owned/native
success, memory-unsoundness finding, or Product GO. The first blocker was found
before the substantial implementation gate, so the bounded stop takes priority
rather than spending the rest of the night bypassing it.

## Authority and start gate

- Implementation/compiler `02884b759c86272c0f97c6ab74d3230c49ae5e27`, tree
  `0a6ddaabd800044bb54ca6f308046ec79b322f05`; CURRENT_SPEC -> Draft17.33.
- Exact main push attempt1 FULL5 **39790920491**: five jobs SUCCESS;
  NO LLVM PARTIAL **39790920334**: SUCCESS. API observations in `authority.json`.
- `origin/main` was rechecked and remained the exact fixed SHA. No base drift.
- Work/Codex local checkout `/workspace/p746`, fresh frontend-only Debug build
  `/workspace/p746-build`, GCC13.3.0. Build is not a new FULL5/CI run; normal
  `newlangc`, no experimental admission option. Configuration log preserved.
- Compass -> Value Strategy §§1.1/1.3, Process §§4.2/4.3.1 and Roadmap checked.
  Historical audit: KEEP DI-001 punctuation, DI-006/007 scopes/projection,
  DI-017 exact two-u8 nominal identity; keep existing two-H/LiveTail rules.
  No source/API/semantic change selected here. Mixed category admission must
  not silently be inferred from two separately implemented finite profiles.

## Before execution

Initial immutable freeze commit **59f4300a2be9d7be4a520f9ff03f50a577127d92**
contains two independently authored programs and five negative attempts.
`freeze.json` records exact SHA256, roles, expected dataflow, all three failure
worlds and the pre-execution prediction. Lifecycle spellings were calibrated
against the canonical/adopted implementation; this is P exploration, not BLIND R.
The programs are freshly authored composition workloads, not renamed claims of
old fixture success. No original source or oracle was adjusted after execution.

| Program | Intended use | Predicted result if admitted |
| --- | --- | --- |
| `calibration_batch.nl` | Calibrate a local device, archive the old offset, then hand off a linked batch's tail | sensor `[26,118] -> [63,118]`; old26 actually written into log `[4,202] -> [4,26]`; displaced202; heap payloads37/151 |
| `archive_window.nl` | After both allocations and attachment, adjust a high threshold and save it in a separate record before returning the tail | active `[12,193] -> [12,71]`; old193 written into backup `[88,207] -> [193,207]`; displaced88; heap payloads59/211 |

A has lexical operations before allocation; B only in the both-success arm.
B also uses a receiver-side scoped read. Both contain an actual owner local and
explicit RETURN; actual cleanup should be zero frees on first NULL, one on
second NULL, two after both successes. These are **predictions, not observations**.
The two lexical records have no heap Allocation/Domain handles. The two proposed
heap originals would derive solely from the two source allocation sites.
No original IDs, regions, domains, carriers, physical values or cleanup were
actually issued/executed because admission stopped earlier.

Source hashes:

- A: `7197abc253ceac6839ab51c4ed72e5ee1f56c95f5e36a6991214fd95bbad84ce`
- B: `9fbdc2594e54ba2587fb951e7ed38af2e058311f00b70f321a5499d40d399bc7`

A is3905 bytes/84 lines, B4103 bytes/85 lines. Each has main + three helpers,
two heap allocation sites, two lexical record constructions, one stateful Copy
helper call and one subsequent store of its old Copy into a separate original.
`source-cost.json` records this ceremony; runtime cost is NOT MEASURED.

## Actual first boundary

Both ordinary `newlangc --check SOURCE` and `newlangc SOURCE` exit **3**, zero
stdout, at the second declaration:

```
A: 3:1-3:45, bytes [148,192)
B: 3:1-3:40, bytes [147,186)
error(unsupported)[AVS-DECL-PROFILE]:
AVS remains one leading struct; recursive category is separate
```

This diagnostic is a **parser source-profile restriction**, not a semantic
ownership error. Public parser status is `NL_PARSE_SYNTAX_UNSUPPORTED` (2);
public source lexer reaches EOF for the entire input (735/758 tokens).
Both repeated calls on the same parser return the same span and leave the
owned syntax-tree slot NULL. Registration and semantic checking are NOT REACHED;
checked-owned artifact/public validator/C17/native/allocation-failure execution
are therefore NOT TESTED. There is no fabricated public world to fill the gap.

Each family was then calibrated once by swapping only the two declaration lines;
those sources and their hashes were separately frozen before execution in commit
`1d7fda4b00f31f35b3bfc6bc377f75f4f35a3263`. Both orders fail identically in
`function_unit`, so declaration reordering is not a source-only remedy.
The two three-line minimal examples show the same restriction without allocation,
helper calls, loan scopes or ownership operations at all. Their second-declaration
spans are [53,89) for heap-first and [37,89) for Copy-first.

All five frozen safety attempts (read-only authority, duplicate consumed owner,
wrong original Domain, missing allocation cleanup, escaped lexical loan binding)
are **masked by the same parser restriction**. They are NOT semantic-negative
PASSes and NOT evidence of unsafe acceptance. There is no valid current artifact
on which to run wrong-fact/world validator mutation tests. Semantic OOM/rollback
and native failure worlds remain NOT TESTED. The actual rollback observation is
limited to parser partial-tree disposal/reusability and CLI output management:
all seven original inputs preserve an existing `--emit-c17-to` destination, emit
zero stdout and leave no temporary sibling; the reproduction additionally checks
this for all eleven inputs.

Machine-readable observations: `baseline/cli.json`, `calibration.json`,
`public-phase.json`, `named-output.json`; original stdout/stderr files retained.

## Cause, scope decision, next bridges

**Observed:** `src/parser.c:function_unit`, lines1845-1865: `avs` and `recursive`
are explicitly mutually exclusive regardless of declaration order. Each individual
declaration has already parsed successfully; the failure is at unit composition,
not numeric/field/LiveTail/RETURN syntax. No source rewrite using two compilation
units, a host-seeded nominal, scalar in place of the record, or removed heap work
would meet this Issue's same-program goal.

**Read-only architecture audit; not later runtime findings:**
`register_function_unit` in `src/semantic_check.c` already stages registration on
a clone, but AVS still requires one source tree and its leading declaration.
`src/main.c` selects the Copy emitter or the independent Node fallback; it does
not compose their staged outputs. `src/checked_c_node.c:expr` dispatches ordinary
registered calls only to owner/producer handling; its aggregate `shape` requires
the exact recursive H, not a distinct two-u8 nominal. Therefore deleting the
parser exclusion is not a demonstrated safe end-to-end bridge. No claim is made
that these *anticipated* later barriers have actually been reached by our source.

Two concrete staged follow-ups, subject to Coordination's source/authority gate:

1. **Bounded mixed-unit admission:** decide/document whether exactly one
   completed H plus one exact two-u8 Copy nominal is already selected source
   composition. If authorized, admit just that combination with exact nominal
   identity, namespace collisions, order-independent registration and transactional
   negative/OOM tests. Do not infer arbitrary multiple nominal ABI support.
   Initial engineering estimate: roughly half a day including focused tests;
   confidence limited because definition/captured-world checking is not reached.
2. **Existing-proof dual-carrier emission:** after real CHECK/owned succeeds,
   map existing `NLCopyBodyParameters`/event/field proofs and original lexical
   carriers alongside the existing producer local-return/heap proof. Keep
   world-qualified symbols, scope closure, old Copy origin and source order;
   validate live failure-world capture and physical stores before emitting.
   No new registry or Allocation/Domain authority. Static estimate:1-3 workdays
   including native/OOM and FULL5/PARTIAL, not authorization or guaranteed cost.
   Stop at any missing authenticated cross-world correspondence.

This is more than a proven single backend-only bridge, and mixed source
eligibility is not unambiguously established by the existing category-local
wording (Draft17.33 §17.1 and the completed-H profile). Hence **NO production
modifications**, no speculative parser bypass, new issuer, PR, or new Actions.
No existing tests, fixtures, compiler, canonical, toolchain, Process or Roadmap
were changed. Frozen main CI remains the control, not M7 proof.

## Reproduce without overwriting this evidence

```
cmake -S . -B /tmp/p746-build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/p746-build -j4
cc -std=c17 -Wall -Wextra -Wpedantic -Werror -Iinclude \
  evidence/p746/phase_probe.c -L/tmp/p746-build \
  -lnewlang_parser -lnewlang_lexer -lnewlang_syntax \
  -lnewlang_source -lnewlang_diagnostic -o /tmp/p746-phase
python3 evidence/p746/reproduce.py --compiler /tmp/p746-build/newlangc \
  --probe /tmp/p746-phase --out /tmp/p746-new-observations
```

The phase probe is host test instrumentation, not generated guest C17/native.
It uses public source/lexer/parser APIs only, creates no guest values/rights,
and does not invoke registration after failure. The script verifies all source
hashes and preserves a separate output directory; no tests/CI are auto-launched.

## Goal alignment

Technical finding: the first ordinary single-unit cross-feature checkpoint is
**not reached**, despite adoption of the two isolated abilities. This updates the
exploration map earlier than the predicted backend barrier. Explicit resource
ceremony and category isolation are user-facing costs; no ordinary-safe-library
UB audit-locality improvement or runtime zero-cost claim has been measured.
Coordination should use this evidence to choose the next source-composition and
emission investment, not resume Product GO scoring on a fictional native PASS.
Product GO active scoring PAUSED #624 and H1 HOLD #268 remain unchanged.
