# P930: genuine pre-IF diagnostic observations, no new authority

Fixed Compiler/main38b0954be6b1466a2982e7c58e1070ac76a9fafc,
Draft17.34; FormalProof4a293a94e3d2028b92902f87e0d197b66467ac23/F72 B only.
P-owned NON-BLIND finite evidence, not independent R or C refinement.

Gate A was posted before branch/CTest changes/CI. An unchanged ordinary CLI
Debug binary was observed with a scratch-only GDB. Both original P929 inputs
produce real pre-IF snapshots, during definition checking and actual main()
checking. Actual nl_body_create_span input/returned plan proves whole-unit
slice/body identity operationally while all owners are live. Debugger JSON
records actual pointers as historical scalar labels, not usable authorities.
Borrowing ends before observed original-fragment/snapshot destruction.

An initial debugger assertion incorrectly equated fragment.context with the
current source-if Check.context. The source intentionally uses the caller
artifact context for a synchronous body. First log is preserved; the refined
probe logs both worlds. Definition matches, actual main call differs. Context
addresses are also reused across lifetimes. Source/oracle unchanged; no
Compiler change or guest semantic/lifetime defect was inferred. Subsequent
CTest explicitly handles this distinction instead of conflating equal IDs.

## Minimal test-only implementation

Only additive CMake and new P930 tests/evidence change. Production source,
public headers, public IF gate, normal CLI, parser, backend, canonical,
workflow, dependencies and all historical tests remain byte-identical to base.
No Production callback or new observation API was necessary.

`observe.c` linker wrappers call REAL nl_body_create_span and nl_if_field_prepare.
After successful prepare, synchronously inspect the ACTUAL owner-held entry,
with original source/tree/registration context still owned by the client.
The plan hook records only the borrowed actual creation token and raw span;
no parser/reparse engine, owned-copy session or artificial world is built.
Existing public semantic snapshot/binding/place/value/aggregate-field views
supply data. Private header access locates the existing unsealed snapshot and
checks real plan/owner linkage; it does not make a public certificate.

The real source context may differ from fragment.context, so the actual
prepare argument qualifies the historical snapshot. Session and phase qualify
copied identities; no cross-world equality is inferred from counters or host
addresses. Original unit bytes -> body span -> actual retained plan -> fragment
source -> pre-IF node/cut are checked while their owners remain live. Only
scalar observations are copied. Borrowed unit/plan tokens are cleared before
client owner cleanup; no snapshot pointer escapes the callback.

Four sources exercise the two original P929 SHA-frozen programs, an independent
initializer-order-only perturbation with equal declared values, and a whole
first/second root-value-origin permutation. New freeze.json was written before
Stage B executions. P929's original freeze (including incorrect exit2) and its
first seven results are preserved unchanged; P930 CLI negative expectations3
are separately labeled calibration, not a repair/regrading of P929 history.

- `p930_live_pre_if.unit`: all four sources, two actual phases each; typed
  nominal/package/field values, declaration-vs-initializer order, root/child
  currentness/unique incarnation/facts, literal dependencies/member carrier,
  condition TRUE/FALSE and three ended temporaries, before any arm creation.
  Old public field-proof getter correctly remains unavailable at this cut.
- `p930_order_root_negative.unit`: 26 mismatched comparisons reject: request a
  different actual root for the first binding, wrong declared field, wrong
  source order, and compare genuine scalar copies from independent order/root
  variants. These are observational-oracle refusals, NOT semantic rejection
  of those legitimate variant programs or a production validator theorem.
  Actual contexts/snapshots are never tampered with.
- `p930_prepare_oom.unit`: actually fault proof calloc and entry-clone context
  malloc within REAL prepare; assert hit1/OOM/no observations, original public
  context inventory unchanged, then SAME context/source healthy retry. This
  is two finite reached fault sites, not exhaustive memory/host proof coverage.
- `p930_ordinary_cli.integration`: source SHA custody and normal --check,
  stdout C17, named output for four legal/two malformed sources. Preserve
  CHECK0 / backend4, and semantic3 codes plus sentinel/no-temp on failure.

Run `ctest -V -R '^p930_'` after a frontend-only build. The test target's wrapper
is not linked into normal newlangc. No no-hit OOM is classified as a pass.
ASan/LSan/UBSan exercise callback/owner cleanup; Gate A debugger independently
records actual original tree/snapshot destruction without post-free reads.

## Evidence and remaining boundaries

Original body offsets are not whole-unit offsets: true body[65,248),
IF body[132,171) -> whole[197,236); false body[65,250), IF body[133,173) ->
whole[198,238). GDB and CTest observe roots1/4, four children2/3/5/6,
unique local inc/facts1..6, values9; these are observations in qualified
instances, not required native identity constants. Tests derive IDs from
actual named bindings and do not seed them.

This is finite observed correspondence for typed lexical C memory, not raw
parser/registration refinement A, proof of F72 Represents C, or accepted-rich
D. No active-block/support history is exposed as a source authority by C;
implicit_local and a scoped cut observation do not themselves prove the F72
lexical event history or F1 lifetime/access. Explicit domains/backing/loan
scopes remain absent, governing_domain0 is a marker. No Domain/Allocation,
child ownership, ref/stability or public Change is invented. No source rules,
CHECK0 admission, C17 unit-IF/native support or general effects are added.

VALUE-ALIGNMENT-RISK: this closes a finite compiler observation blind spot,
not a demonstrated reduction in library-author plus ordinary-safe-caller UB
re-audit dependency closure versus well-factored C. Avoid treating additional
private machinery as product value. Test-only reuse of an existing owner is
preferred to a second parser or mutation seam. Product GO PAUSED/H1 HOLD;
Coordination decides adoption and any later independent R or proof gate.
