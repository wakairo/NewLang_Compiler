# P752 — Draft17.34 mixed-unit Stage1

Track P. Fixed base `01151032569ad52ac50d164918b5de4cf90d1d7b`, canonical
Draft17.34 §16.3.8. Explicit Coordination RELEASE: Issue752 comment6354635019.
Base differs from implementation02884b7 only in four docs paths. Exact natural
push FULL5 39878130055 (5/5) and NO LLVM PARTIAL39878129512 succeeded on attempt1;
`evidence/base-ci.json` records the independent API observations.

## Scope and design

- The parser collects a complete unit before justifying a deferred T position.
  Only one one-link H plus one two-u8 T permits functions before/between/after.
  Two T declarations and mixed multi-H/three-link H are still fenced. H-only
  and leading-T-only admission retain their old behavior.
- Registration first collects both nominal declarations on the existing clone.
  It registers T and creates/completes H through the existing once-completion,
  self-target and cycle validator before installing **any** signatures or checking
  definitions. Both source orders use distinct deterministic nominal identities.
  Completion, names, signatures, retained body plans and definition failures all
  share the existing final commit/whole-candidate destruction boundary.
- The mixed category requires the specified ordinary `main()->unit`. Existing
  standalone categories and multi-input restrictions stay independent.
- The driver has an explicit negative capability gate after genuine CHECK and
  before selecting either independent emitter. Mixed C17 generation is CLI4
  `V1-BACKEND-UNSUPPORTED`, zero output, including u8 export mode. No emitter,
  issuer, public API or validator has been extended. The small `src/main.c`
  change prevents accidental emission of just the Copy portion of a mixed unit.

Historical audit: faithful implementation of adopted §16.3.8 / DI-018, not a new
source decision. KEEP DI-001/006/007, H completion and original lifetime rules,
DI-017 field nominal/index identity. DEFER other units, multiple H/T, general
ABI/CFG/owner/effects and joined proof/backend. No normative documents changed.
Compass -> Value Strategy §§1.1/1.3, Process §§4.2/4.3.1 and current Roadmap were
checked. Split units or replacing a record with a scalar do not meet the goal.

## Frozen input and real results

Initial preimplementation freeze commit
`9cf115a528e1896db608b917decd00a816d0bb18` contains `freeze.json` and17 sources:
3 new positives and14 rejection attempts. `evidence/unchanged-compiler.json`
preserves actual original compiler refusals (all CLI3). The compiler was built
from the exact code-equivalent fixed implementation, not a speculative spec PR.
`additional-freeze.json` labels four later calibrated negatives separately.
No frozen input bytes or predicted scalar/cleanup results were changed.

- `order_h_t.nl`: H, function, T, functions/main; normal named `retune` updates
  Controls.trim28→93, returning old28, then another lexical original receives28
  in its ceiling177→28. Sibling144 is preserved.
- `order_t_h.nl`: main before both types, T, named helper, H, requested function.
  The same types/signatures/body operations resolve before their source positions.
- `two_original_hook.nl`: function before both types and another between them;
  two actual static fallible allocation sites of the same Packet, payload46/182,
  distinct original root/incarnation/BackingRegion/Domain; explicit 0/1/2-success
  failure-path cleanup. This is checked source, **not physical execution**.
- `history/` contains byte-identical P746 A/B, original freeze and initial parser3
  observations. SHA256 A7197abc253ceac6839ab51c4ed72e5ee1f56c95f5e36a6991214fd95bbad84ce;
  B9fbdc2594e54ba2587fb951e7ed38af2e058311f00b70f321a5499d40d399bc7.
  These are inherited P controls, not fresh BLIND R or new independent sources.

All three new positives and both inherited P746 programs now give normal CLI
CHECK0. Actual source-generated owned fragments exist; original syntax/source
owners can be destroyed after registration because retained source plans own the
body input. Both H and T are registered before signature/body checking, H TypeId11
and T TypeId10 in these fresh contexts; H's committed ptr target is11, not10.
Type identity is observed in its original context, never inferred from ID numbers
in unrelated worlds or equal C layout.

For the first two programs, existing independent Copy-body and actual→formal
validators succeed. Tests observe actual old Copy28, new child93, unchanged
sibling144, root/child identity and loan active-at-handoff/closed-after-caller.
No host seed, guessed result or fabricated semantic object is used.

**First remaining proof boundary, deliberately NOT repaired here:** in the
real two-allocation hook, the existing Copy observations are present but both
existing Copy validators return `NL_CHECK_ANALYSIS_PRECISION_LIMIT` (3). Read-only
inspection found the existing `nl_copy_same_types(c,p->exit)` fence refuses H's
legitimate later layout refinement: TypeId11's prior Copy receipt has
`layout_known=false,size=0,alignment=0`, while the later allocated caller world
has `true,24,8`. Both worlds individually pass semantic validation, graph seal
and body/caller world identity match. This is **not a validated mixed proof**.
The typed owned entry and actual semantic CHECK0 are distinct from its finite
public certificate's validation precision. No validator was weakened and no
joined world or authority issued. C must separately scope any Stage2 handling of
layout/type epochs and dual carriers.

The inherited P746 actual producer local-return receipts/terminal owner receipts
and closed legacy cleanup-arm observations still validate through unchanged public
APIs. This validates those finite **H receipts**, not a combined Copy+heap proof.

All five positive programs stop before C17 emission with CLI4/zero bytes. New
mixed C17/native, physical addresses/stores/frees and allocation-failure native
execution are NOT TESTED / NOT AUTHORIZED. Old M1–M6 native controls remain
independent regressions, not mixed native evidence.

## Rejection, transaction and recovery tests

`unit_test.c` and `cli.py` check:

- six H/T/function permutations; both field declaration indices remain distinct;
  helpers/signatures visible forward and definition-time checking independent of
  whether the bad helper is called;
- extra H/T, three-link H+T, reversed H fields/wrong T field types: parser profile
  rejection; wrong H self target: semantic unsupported, not minted foreign ptr;
- H/T/function namespace collisions, duplicate function or field, unknown type,
  late wrong return, wrong u8 actual, real read ref into write formal, foreign
  H aggregate destructured as T: semantic rejection;
- the original read-only-field attempt still hits its earlier source loan profile;
  it is not relabeled as a permission-check PASS;
- mixed without ordinary main: bounded registration-profile rejection;
- two distinct physical inputs: rejection with correct input_index and full
  rollback; previous single-nominal/multi-input tests remain unchanged;
- every semantic rejection is repeated on the same context, observed by the
  public state snapshot, then the legal unit registers successfully. A preexisting
  **source-defined** `intact()->u8` and its retained plan are preserved;
- fail-at-N malloc/calloc/realloc at **every** successful parser/registration
  allocation up to completion, maximum10000. Parser output stays NULL; clone,
  T registration, H header/completion, signatures and all definition walks roll
  back without partial types/functions/facts. The same original context retries;
- real driver `compile_v0_to` for both CHECK and emission:974 injected positions
  each, CLI1=28 / CLI3=946, internal0, partial C0; uninjected endpoints are
  CHECK0 / backend4. Injection status is not relabeled as a source rejection;
- all 23 frozen/new/inherited CLI inputs preserve an existing named destination,
  leave a nonexistent destination absent, and leave no temporary sibling on
  refusal. Original stdout is empty. There is no mixed native oracle injection.

Registration OOM counts with the preserved preexisting function:517/517/1226;
parser82/82/229. Failures are exhaustive within these actual finite traces,
not universal allocation/allocator guarantees. See `source-observations.json`.

## Reproduce

```
cmake -S . -B /tmp/p752-build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/p752-build -j4
ctest --test-dir /tmp/p752-build -R '^p752_' --output-on-failure
ctest --test-dir /tmp/p752-build --output-on-failure -j4
```

Local GCC13.3 frontend regression424/424, new focused tests20/20; focused tests
also run on Clang23.1.2 ASan/LSan and UBSan. Natural candidate FULL5 uses the
repository's locked LLVM/Clang23.1.3 and includes its ordinary LLVM smoke; local
frontend evidence never substitutes for that required CI.

## First failures preserved

- `first-build.log`: local member-access typo (`NLFunctionEntry.view.result`),
  corrected to the existing `result` field; no design/semantic change.
- `first-focused-tests.log`: initial test incorrectly demanded a valid scalar
  public proof even after heap type refinement. Kept as the actual first failure;
  final tests report/require the existing precision refusal and defer Stage2.
- `first-cli-test.log`: calibrated negative expected the wrong semantic code;
  actual `P5-DESTRUCTURE-TYPE` was correct. Only the test's diagnostic expectation
  changed, not frozen source or its predicted refusal.

These local revisions preceded the fixed candidate PR/CI. No old P746 source,
old oracle or historical first refusal was rewritten to look successful.

## Goal alignment / limits

Stage1 unlocks selected same-file source/type/definition composition and exposes
its **next authenticated-proof boundary**. It does not establish an ordinary-safe
library, a general nominal ABI or physical mixed execution. Current explicit
resource ceremony and author+ordinary-caller UB audit locality remain unmeasured.
Product GO active scoring PAUSED624 and H1 HOLD268 stay in force. No Stage2,
new issuer/ownership/proof authority, main merge, canonical edit or other Track
launch is authorized by this candidate; independent review belongs to C.
