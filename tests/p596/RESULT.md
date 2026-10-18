# P596 — FINITE SOURCE-ISSUED USIZE SUBTRACTION CANDIDATE / DO NOT MERGE

Base/main fdae44e62e39bc521fc4bcab70909d4d7e1d6155, CURRENT_SPEC → Draft17.32.
M593/C selected exactly one flat atom-atom (comment6248105142); plus limited
adoption R592/C comment6248370657. No new semantics or general precedence.
Pre-edit gates/forecast: PREFLIGHT.md. All30 source hashes and original oracles
are frozen in frozen.json; unchanged-main failures are evidence/baseline.json.
Historical P589 files and public plus header stay byte-identical and are checked
against historical-hashes.json. No PR590/R594 evidence or canonical change.

## Reuse and compatibility

Route B: a small PRIVATE two-operation core, explicit ADD/SUBTRACT tag, one
existing source-issued event list/destructor/capture and one validator walk.
The public addition header, signatures, layout and .sum meaning are unchanged.
New subtraction-only NLUsizeSubtractionView uses .difference. Thin wrappers
explicitly convert metadata to a private .answer field; no public structure
casts, union punning, negative-addition encoding or silent sum reinterpretation.
Both view/validator families refuse the other operation, even with NULL candidate
and when the mathematical results coincide (zero case). Existing archived plus
and equality C17 output is byte-for-byte identical.

Only one expression-local infix minus is admitted alongside existing plus/==.
Name/typed literal atoms retain ordered child/value IDs, full/operand/operator
spans and current binding/Place/incarnation/ValueFact provenance. No global lexer
change; function result -> still parses. Chained/mixed/parenthesized/unary/bare,
call/field/loan/ptr operands remain SOURCE-profile unsupported. uintptr-usize
is a separate canonical operation outside this source slice, not declared
intrinsically invalid by this work.

Semantic CHECK requires exact core usize, known and dependency-free operands.
It proves lhs>=rhs BEFORE performing lhs-rhs; plus separately retains its own
rhs<=SIZE_MAX-lhs precondition. Unknown dynamic operands refuse precision.
Result is a fresh Copy usize, with no ref/Domain/Allocation authority.

The shared public validator authenticates retained registered source, exact
operation tag/AST/checked kind/token, original ordered operands and packages,
binding identity/currentness receipt, type/dependency exclusion and the specific
guarded mathematical relation. Event issuance remains trusted and historical
facts do not revive currently live access. Public observations are borrowed
local evidence, not a cryptographic certificate, untrusted whole-program replay,
serialized cross-build proof or ownership grant. The original input source/parser
AST is destroyed before public observations; the owned registered plan remains.

C17 validation consumes the correct public validator before emission and computes
actual original guest carriers, e.g. const size_t nl_local_3 = (nl_local_1 -
nl_local_2). It does not emit the checker-known 74/3 as a result substitute.
The admitted pure atoms have no observable evaluation side effects, preserving
source LTR semantics. Existing exact-u8/CHAR_BIT and SIZE_MAX guards are unchanged.

## Actual source outcomes

- Seven arithmetic positives: 7-4=3; named91-17=74; 0-0=0; MAX-0=MAX;
  MAX-MAX=0; Copy chain52-13=39; result consumed by existing ==.
  SOURCE/CHECK0, genuine registered public owned/validator true, normal C17 CLI0.
  Strict GCC14.2 / Clang23.1.2 O0/O2 plain and passive-observer native all exit0.
- 0-1 and 4-7: semantic USIZE-SUBTRACTION-UNDERFLOW, CLI3 at original operator
  spans (1:35–1:36). The typed operands themselves are individually valid.
- Oversized single literal: USIZE-LITERAL-RANGE, CLI3 (1:27–1:54), distinct from
  binary precondition failure. u8(2)-usize(1): USIZE-SUBTRACTION-TYPE, CLI3
  (1:32–1:33). Unknown parameter minus1: USIZE-SUBTRACTION-PRECISION, CLI3
  (1:29–1:30). Scalar-as-write-authority: P3-WRITE-REF-REQUIRED.
- Fourteen excluded grammar forms refuse at parser/SOURCE. No subtraction
  CHECK or underflow proof claimed for them. Semantic/parser failures publish
  no partial artifact, emit zero C bytes and preserve public-state rollback.
- Plus/equality controls and all original P589 tests/archived C17 remain valid.
- The arrow-control native0 frozen prediction was too broad: the BASELINE
  already has SOURCE/CHECK0 followed by CLI4/V1-BACKEND-UNSUPPORTED, zero C bytes.
  Candidate preserves exactly that behavior. This verifies arrow parsing, not
  new no-argument u8-return native lowering. Source/original oracle retained;
  calibration is explicit in integration_test.py and candidate.json.

## Negative proof, OOM and publication evidence

Per subtraction source: 52 public-copy/actual checked artifact refusals for
literal cases, 59 for named cases. Foreign registered same-number/value origin,
missing/foreign receipt, wrong type/children/result/operator/span, old
binding/place/incarnation/fact, dependency/ref/Allocation contamination,
truncated artifact and wrong retained AST/token all refuse. Backend gate refuses
actual corrupted checked artifacts; observed fragment/context arrays stay
bytewise unchanged; restored originals pass. No production fault hook added.

A separate private test compiles the real shared proof implementation and issues
all input worlds through real registered source. For subtraction, addition and
zero it refuses 11 actual receipt corruptions each, including wrong/unknown
operation tag, foreign matching numeric IDs/source worlds and false wrapped
underflow/overflow. BOTH public validators fail on every corrupt event. Valid
old plus .sum stays11. Pure math boundary checks separately refuse wrapped
underflow and overflow; these are unit negatives, not source-success evidence.
Failed getters preserve output sentinels; nonempty checked output slots remain
untouched; provisional unregistered fragments are never certified.

New parser malloc/calloc/realloc sweep:19 fail indices, partial treeNULL,
reusable parser. Real-source registration/main fail-at-N sweeps: names84/39,
literals68/29, every prior index OOM with public-state rollback and outputNULL.
Public minus validator allocates zero bytes/calls under fail-at-zero injection;
old plus OOM/rollback suite remains green. No new exhaustive stdio/C17 I/O-fault
injection claim. R592's unrun foreign-context/OOM publication checks remain
independently unrun: these additional results are P-side, not retroactive R PASS.

Passive native observers read existing guest locals only; wrong C plus operator,
reversed operands and wrong result retain the same observers and exit91 at O0/O2
where mathematically different. Such unsigned-wrap C mutants are deliberately
synthetic negatives, not accepted NewLang arithmetic or cross-target evidence.
Actual source, typed IDs, original carriers, generated C hashes and native
statuses are in evidence/{gcc,clang}-native.jsonl and the retained .c17.txt files.

## Regression, cost and stop

Local GCC frontend suite325/325, Clang focused15/15, ASan/LSan focused10/10;
final shared-core unit also rerun on Clang/ASan after strengthening zero/tag
attacks. Exact final-head GitHub FULL5 + PARTIAL run IDs/results are recorded on
Issue596/PR after this commit; local Clang23.1.2 is not CI's pinned23.1.3.

Measured 64-bit host: one tagged receipt264 bytes vs256 (+8 per operation),
fragment552 bytes unchanged (+0), old plus view216 and new minus view216.
One validator module373 lines vs270 (+103), not a second270-line validator/list.
No new validation heap allocation or world clone; existing bounded event,
registered-body/source/world scans remain. Many-operation scaling and allocator
bookkeeping are unmeasured. Explicit metadata conversions/three flat view
layouts are a compatibility maintenance cost, not a new public generic schema.

Forecast small private sharing proved feasible within the 90–150-minute ceiling.
The new reusable rung is known checked subtraction/Copy quantity computation;
first next blocker is unproved dynamic range, not new runtime arithmetic or a
capacity/container API. General arithmetic, precedence/CFG, dynamic proof,
uintptr source, owner/effect/Array mechanisms and arbitrary ABI stay deferred.
Goal/Evidence/Alternatives/Stop: reusable selected quantity foundation / real
source plus authentic public proof and native evidence / tiny sharing chosen
instead of duplicate validator, with no-code HOLD fallback / bounded profile
tested, stop before broadening. No goal deviation identified; no Product/H1 GO.
A new shared public trust boundary deserves independent R at C's discretion;
P did not launch R or any other Track. DRAFT, UNMERGED; stop after one final
Issue596 report. Main/canonical unchanged.

One supplemental, explicitly P-calibrated regression (supplemental.json, frozen
before its execution after initial implementation) tests three separate complete
flat expressions in ONE artifact, + / - / + = 15 / 11 / 19. Both public APIs
validate only their own nodes in the shared list; native original carriers and
wrong-minus-to-plus mutants are observed. The same three-event source has its
own registration/main OOM sweep (95/46 fail indices), including partial-chain destruction. This is
not mixed operators within one expression or an original pre-edit blind case.

Reproduce:
```
cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build -R 'p596|p589|c17_target_model|p572_' --output-on-failure
build/newlangc --check tests/p596/names.nl
build/newlangc tests/p596/names.nl > /tmp/p596.c
build/p596_public_probe tests/p596/names.nl
build/p596_gate_test tests/p596/names.nl
build/p596_proof_core_test tests/p596/literals.nl tests/p596/plus.nl tests/p596/zero.nl tests/p596/shared_sequence.nl
```

During validation, live main advanced to262f4bddb9055a6cf9067a639cbe44fa72b703a4
through Docs595 only (C Replacement Coverage Map and Development Roadmap).
The implementation branch remains based on fixed fdae44e; no rebase or code
change was taken from that docs commit. PR Actions may check the merge tree
including those two docs; report its exact relationship to the frozen head.
