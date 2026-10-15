# P493 — finite qualified ordinary-call source proof candidate, UNSELECTED

Track: P. Experimental source-issued certificate/independent replay only.
No named-call C17/native, matched join extension, semantic selection or Product GO.

## Authority and immutable input

Compiler/main authority ad0604d2c766eac045273eba15170027f199d650, CURRENT_SPEC
Draft17.30. Candidate implementation starts EXACT unmerged PR479 head
03b1751b513e7aef2d7ba193eda671a556f54b47, new branch p493-qualified-ordinary-call.
PR479 remains DRAFT/DO NOT MERGE at that head; no main/P488/old-PR modifications.
Pre-code/pre-run source+oracles commit3f3eef2d6431d841a372ec42243b7d6e638cc7f1.
One positive byte-identical to P488:
SHA25606c96f6df6688167f2f9d8e2768bfc5bc2ea9f93e09838502400039d2de5f8aa.
Source/oracle unchanged; no easier positive or host-created authority.

Unchanged compiler first: --check positive CLI0/stdout0/diagnostic0; actual owned
main/callee observation from P488 was genuine but ordinary public proof absent.
Original P478 matched source remains CLI3 P9-CONTINUATION-PRECISION13:5–16:6;
ptr_as_ref remains CLI3 semantic P3-TYPE-MISMATCH15:66–15:67. Raw baseline and
opt-in diagnostics/hashes are preserved. No guard/signature/source admission fix.

## Genuine issuance and independent read-only validator

The new experimental diagnostic path retains FIVE actual source-point worlds:
1. Caller after source-order actual evaluation, BEFORE formal binding.
2. Immediately before executed primitive, after real formals and two Copy reads.
3. Immediately after existing nl_fixed_change, BEFORE argument/local cleanup.
4. Actual caller call-return, AFTER callee and call argument cleanup.
5. Actual outer scoped-loan close, BEFORE receiving old Copy into caller binding.

Owned child holds entry/post/closed; primitive's existing NLCapturedChange owns
before/after. Caller/call ID and child/operation ID remain artifact-qualified.
Owning graph retains the actual immutable body/source plan after original parser,
unit syntax and input source owners are released. Public borrowed snapshots are
historical source points, NOT final teardown liveness or newly usable authority.

nl_checked_ordinary_call_validate does NOT call the source checker or trust a
verified boolean. It verifies real parent loan/binding connectivity, actual args,
retained function/body identity, exact connected four-node child (block -> one
replace -> two formal reads), type/mode/nominal projection/original incarnation/
implicit-D/scope and locator provenance. On a PRIVATE clone of genuine entry it
replays formal binding, both Copy reads, original fixed Change/Reset, old-result
forwarding, argument/formal cleanup, function-exit compatibility and actual loan
close. Complete-world comparisons against each independently issued checkpoint
include all places, values, F/RF, occurrence history, scopes and dependency data;
other originals/siblings must match, no extra body event/branch/call permitted.
Shared trusted primitives are core semantic operations and complete-frame
comparison, not the issuer's source interpretation. No cloned state is committed
or exported as authority. Public copied-candidate world/body pointers are compared
to owned anchors before dereferencing; arbitrary host certificates do not grant
rights. Correct-view replay also runs when candidate=NULL.

Measured exact call: caller35, child replace2, caller loan33; actual tail root1/
inc1, next field2/inc2; formal9/10 bind actual ref/Option; implicit local D identity,
NOT heap Allocation/explicit LifetimeDomain. Actual scope3 active at entry/call
post and inactive at authentic loan close. Field F2->20, parent RF1->21, original
field occurrence0->4 (fresh/live), old value1 None Copy forwarded from actual
replace to body/call/loan. Formal Some occurrence3 ends at callee cleanup; original
head and all three non-target fields/siblings retain facts/values/incarnations.
Incoming Some locator still denotes original tail; it never creates write rights.
The real write-ref comes from original lexical tail.next loan, not Copy ptr.

## Public corruption, rollback and OOM

- Change only exported mapping tail.next -> same-typed head.next (root/field/ref
  target as a mapping), authentic source/artifacts unchanged: INVALID precision.
- Erase exported nonempty ref scope3->0: INVALID precision.
- Increment exported original reference incarnation: INVALID precision.
- Substitute post as exported entry: INVALID precision; correct view still VALID.
- Six public worlds (five checkpoints + final owning context) unchanged across
  positive/corruption validation and ALL 47 malloc/calloc/realloc failure positions
  before successful validator replay. OOM is explicit, never acceptance or mutation.
- Actual source main() check/issuance: ALL307 allocation-failure positions in opt-in
  return OOM, owned output NULL, complete public registration state unchanged;
  subsequent successful actual artifact has exactly one revalidating certificate.
  Default-off equivalent has132 positions and zero certificates. No private layout
  or seeded semantic original used in tests. Local snapshot helper is a const-only
  copy of established public-view comparison routines, not mutable access.
- Existing P9 and P3 registration controls also prove public rollback unchanged
  in BOTH default and explicit opt-in test builds.

Limits are explicit: this exact witness is old None -> Some(original locator), two
lexical original two-field Nodes, no opaque/exact VALUE dependencies; real ref
scope dependency is NONEMPTY and preserved. Broader hidden-value overlays,
Some->Some old-occurrence histories, heap/nonCopy, alias generality, extra body
operations or calls/branches are NOT certified. Unknown never becomes Some.
The conditional F62 model does not prove this C/source adapter or prefix issuer.
No complete compiler-soundness/formal-refinement claim.

## Isolation, tests and cost

NEWLANG_EXPERIMENTAL_ORDINARY_CALL_PROOF defaults OFF. Ordinary CLI/check behavior
and P9 fence unchanged; default test observes real CHECK/owned but no certificate.
Normal CI explicitly compiles a separate opt-in semantic test target using the
same production checker (no trusted host state). Two new CTests exercise opt-in
positive/corruption/OOM/expected P9/P3 and default-off boundary. Thus FULL/PARTIAL
will actually test candidate proof, not merely a disabled flag. Workflow/lock/
bootstrap/default LLVM gates remain untouched. Hosted outcomes are in PR/Issue;
only one natural FULL+PARTIAL attempt, no retry or post-CI implementation push.

Local GCC14.2 Debug frontend suite290/290 PASS52.43s; latest added targeted checks
GCC and Clang23.1.2 both2/2 PASS. Existing unmodified P471 asymmetric inline control
source/public validator/C17/native strict GCC O0/O2 plain+passive all4 PASS,2545C
bytes/SHA92359ec3c59d72c7286cd45fd28dd8f68302410d935d9417934609feeb123acc.
This native result belongs ONLY to inline control. Named helper C17/native is
OUT OF SCOPE / NOT ATTEMPTED, even after certificate success.

Added production/proof surface: one530-line module +38-line public diagnostic
header,19 checker-hook lines,11 internal ownership/declaration lines,2 destructor
lines,28 CMake lines (includes testing). This is finite deterministic replay,
not a general effect/CFG/SSA system. It duplicates a small local-cleanup verifier
and depends on existing core/frame comparison contracts: maintenance/audit cost
is real. Five owned context clones increase this trace's checked-call allocation
positions132->307 (+175); byte memory and native runtime overhead NOT measured.
20 Debug public validations measured roughly0.0011–0.0013s locally, diagnostic
only, not a benchmark/latency guarantee. Test library duplicates semantic build
work intentionally to keep Production default-OFF; adoption would need review.

Alternatives retained from preflight: inline P471 costs ZERO added proof/ABI
mechanism and already works natively; conservative named-call HOLD/higher-level
API remains reasonable given V484's no-demonstrated-product-win. P490 C2 Unknown
correlation remains DEFER; sweeping effects/CFG/SSA/ABI not selected. Compass ->
Product Strategy1.1 audit dependency closure goal retained; author+caller burden
or C/Zig/same-topology Rust advantage remains UNMEASURED/Product priority HOLD.

Prediction was missing temporal source/call correlation, not syntax or a guard.
Observed: authentic entry/primitive/post/loan-close snapshots plus bounded whole
replay establish this one public certificate with required corruptions rejected.
Recommended next gate is independent Track R on a separate C-issued Issue, NOT
merge or emission. If review finds an omitted essential premise, HOLD; don't
widen the profile. P9 matched-call, H1/STB/allocator, P490 C2 holds unchanged.
Historical4.2 N/A faithful diagnostic proof representation; DI006/007/011 and
Draft13.5c/17.4/18/20/26–27 KEEP. No new NewLang source/API semantics or Ledger edit.
No main/old PR merge, own Issue close or Track launch. Final Issue report once
then STOP; C owns independent review/investment/adoption.

## Reproduction

```
cmake -S . -B /tmp/p493 -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/p493 -j4
ctest --test-dir /tmp/p493 -R '^p493_' --output-on-failure -V
ctest --test-dir /tmp/p493 --output-on-failure
```

To request experimental issuance in the actual compiler build (still unselected):
```
cmake -S . -B /tmp/p493-opt -DNEWLANG_FRONTEND_ONLY=ON -DNEWLANG_EXPERIMENTAL_ORDINARY_CALL_PROOF=ON
cmake --build /tmp/p493-opt --target newlangc -j4
/tmp/p493-opt/newlangc --check tests/p493/nonmatch_link.nl
```
CLI CHECK0 alone is NOT a public proof result; use authentic owned API/test above.
