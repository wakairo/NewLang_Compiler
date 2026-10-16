# P501: finite source-issued usize equality bool; backend / Pair join HOLD

Fixed main0d29d61c114f179af8e7300096fd85ec485ad2e7, canonical Draft17.30;
pre-code/pre-run freeze ac3589b2048f023f8c2ae9b36c15e2004a87316a. Source bytes,
SHA256 and expectations in frozen.json, unchanged. Full original P497 guards
copied byte-for-byte. Historical A–E / goal preflight: PREFLIGHT.md; measured
checkpoint, isolation reason and historical-access limitation: CHECKPOINT.md.
P501 reaches the primary comparison CHECK/public gate, not a native guard.

## Prediction -> measured first boundaries

Exact unchanged compiler ran all12 sources, separately --check and emission:
24 parser CLI3/P5-EXPRESSION-UNSUPPORTED, stdout0, at first =. Minimal true/false
span4:14–4:15, original full Pair5:16–5:17. This matched the prediction.
Baseline binary SHA and complete diagnostics: baseline.json.

Experimental candidate admission (default OFF):

| Source | CHECK | Public source-owned evidence | C17/native |
|---|---|---|---|
|equal_true|0|actual2==2, bool1|CLI4/no C/native not reached|
|equal_false|0|actual2==3, bool0|CLI4/no C/native not reached|
|copied_renamed|0|source Copy17==17, bool1|CLI4/no C/native not reached|
|if_true / if_false|0|real bool1/0 consumed by IF; two owned unit arms|CLI4/no C/native not reached|
|wrong_type|semantic CLI3/USIZE-EQUALITY-TYPE4:14–4:16|no complete artifact|no C|
|bool_as_ref|semantic CLI3/P3-WRITE-REF-REQUIRED5:11–5:16|no complete artifact|no C|
|unknown formals|precision CLI3/USIZE-EQUALITY-PRECISION1:33–1:35|no complete artifact|no C|
|full P497 true / false|precision CLI3/P6-JOIN-PRECISION7:5–8:70|no complete artifact|not reached|
|chain / split = =|parser unsupported CLI3|no artifact|not reached|

All successful CHECK programs still fail closed in the unchanged emitter with
V1-BACKEND-UNSUPPORTED, empty stdout. The full Pair guards now expose an ACTUAL
checker relational-field join blocker before that emitter; unlike P497 this is
not merely predicted by reading code. Stop here: no source_if, arm_frame,
Node join, issuer, effect or emitter widening. Original Pair old0 / selected7/8 /
sibling9 physical oracle is UNTESTED. Pure unit-arm IF does not demonstrate
runtime decision or physical mutation; both arms are checked hypothetical worlds.

## Implementation and proof boundary

Append syntax and checked equality kinds (old IDs preserved). Two adjacent
single-byte lexer = punctuation tokens represent exact ==; lexer unchanged.
One flat pair of names/typed literals only, no general infix/precedence grammar;
chain, mixed operator, block/call compound operands remain unsupported profiles,
not new normative prohibitions. u8 literal operand parses so its mismatch is
reported by semantic typing, not hidden behind syntax refusal.

Check actual left expression then right expression, require one exact core usize
result from each. Their genuine Copy value packages must have known scalar
values and no retained/unknown dependencies. Compare these mathematical native
quantity values, issue a fresh core bool scalar0/1, retain the two real checked
child IDs in LTR order and exact operator span. Source value names/constants
are not privileged. Unknown/dependent values fail precision; no algebraic or
runtime-symbolic reasoning is introduced. Consumed operand temporaries end using
existing Copy rules. New bool initializer has no Domain/Allocation/ref/owner
rights. Source checker/context cloning still supplies transactional commit.
No host seed, forged bool/formal, memory registry or pointer identity inference.

Public probe uses the real declaration source -> registration -> actual main()
owned child; original parser/tree/input have been destroyed by the source helper.
It follows each operand's source binding/Copy chain to a real typed literal,
checks local child/result IDs, package types/values, operator source span, bool
Copy and zero memory rights. Actual true/false: node10, children8/9, symbols1/2,
operand value IDs3/4 and fresh bool5; bool binding3. Renamed source: node13,
children11/12, symbols2/3, values4/5->6, binding4. Retained body-source operator
spans51:53 or73:75 are not original whole-file byte coordinates. Pure IF checks
its copied real bool condition and both distinct arm-owned contexts. No arm IDs
are imported into the caller. These are trusted public checked-view observations,
NOT a new independent public validator/theorem or tamper-proof certificate.
Wrong-world forged public observations / general dependency-sensitive source
are not independently validated here; IDs are scoped to their actual owner.

## Default isolation and compatibility cost

Adopted P497 tests explicitly pin full guards and negative sources to the old
parser boundary and cannot be edited in this task. To preserve that constraint,
NEWLANG_EXPERIMENTAL_USIZE_EQUALITY defaults OFF on normal newlangc. Dedicated
p501_newlangc/parser/semantic/checked_c_node targets reuse actual Production
source lists with only admission enabled; public probe/unit tests link that
matching graph, never both default/experimental semantic or parser libraries.
Every CI configuration exercises all new cases with the feature ON alongside
all old cases using default-OFF normal compiler. This is an unselected finite
candidate, not a claim main now accepts equality by default. Public kinds and
checker implementation are present; the parser gate controls new admission.

A separate BUILD_TESTING=OFF + experimental optionON production newlangc was
also built locally; raw true/false/negative/IF/Pair outputs agree with the
dedicated test compiler (evidence/optin-cli.json). Turning the global optionON
while running unchanged adopted golden frontiers would require separately
approved test-oracle maintenance; not done/claimed. No adopted test rewritten,
no changed workflow, no CI filter/skip/rerun. Extra test-only compiler builds are
a real build/maintenance cost, not a second handwritten compiler or authority.

## Regression / rollback / OOM

Five new CTests: parser, exact scalar values, rollback/negative, OOM, actual
source-owned frontier. Unit values cover0/1/17, leading zeros, same-variable
Copy and SIZE_MAX true/false; no host bool producer. Pointer-valued p==p is
semantic USIZE-EQUALITY-TYPE, and actual bool as write-ref is rejected. A known
true condition does not hide an invalid bool-as-ref in its else arm: that arm
is also rejected. Independent source-defined unknown usize formals precision-
reject even when a later actual call uses known literal values.

Semantic-error and definition-precision failures leave public snapshots
unchanged. malloc/realloc failure sweeps: parser16, source checker32,
registration101, actual main44 positions; every injected failure explicit OOM,
NULL artifact/tree and public rollback. Parser and equality node/package paths
included. No production allocator hook added.

Local GCC14.2 frontend294/294 PASS31.84s; after a test-only extra negative,
new five tests also PASS0.09s. Formatting PASS. Existing u8/P500 literal native,
P471 cross-root/P490, ref/borrow/nonCopy/EndRoot/Option regressions unchanged.
No new equality-generated C or native test is claimed. Default adopted C17
controls remain part of the executed suite.

## Reproduction / final investment boundary

Configure frontend-only as usual; build all and run ctest -R p501 -V (then all).
Dedicated test binary is p501_newlangc; default newlangc remains admissionOFF.
Or configure -DNEWLANG_FRONTEND_ONLY=ON -DBUILD_TESTING=OFF
-DNEWLANG_EXPERIMENTAL_USIZE_EQUALITY=ON and use normal newlangc --check INPUT.
frontier_test.py DEFAULT_COMPILER EXPERIMENTAL_COMPILER PUBLIC_PROBE TESTS_P501
checks all frozen hashes, exact stage/diagnostic distinctions and actual booleans.

Natural FULL5 + NO LLVM/PARTIAL CI is submitted exactly once for the PR HEAD;
actual SHA/merge/job results will be preserved in PR and final Issue report.
Canonical/toolchain/LLVM/workflows/API authority laws/emitter/adopted tests/
PR479/494 unchanged. One DRAFT/DO NOT MERGE; C decides acceptance/review.

Historical audit: N/A faithful canonical §5.2 single operator; §6/§7 preserved,
DI001/006/007/008/011 KEEP, broader grammar/effects/ownership/Array DEFER.
Actual Draft17.4 and17.30 read; indexed older Surface originals unavailable in
checkout, not claimed exhaustively audited. No ledger/spec selection changes.
Goal is bounded Exploration toward quantity decisions; quantity comparisons now
have a genuine typed result in this candidate. Runtime guards, bounds/index,
Array/byte/Ring, audit dependency-closure improvement, fair C/Zig/Rust burden,
H1/STB/Product GO are still unmeasured/HOLD. The next independent investment is
Pair relational current-state join and then separately faithful IF carrier
lowering; neither is automatically commissioned here. P stops after one final
Issue report, with no merge/Issue close/other Track.
