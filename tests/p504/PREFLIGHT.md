# P504 pre-code / pre-execution freeze

Authority2161115b5cc25c3ef0c8c2a729f0f0ffe1b7a287; canonical CURRENT_SPEC ->
Draft17.30. PR503/e561e8f189fd67961f5ac90560e20ecc45752abd is an immutable
reference, not branch base. No candidate run before this freeze commit.
The12 original P501 .nl files are byte copies from exact e561e8f;3 preselected
additional programs test real bool->saved Copy and direct literal operands.
All15 source bytes/hashes, operand/bool/native/negative/still-HOLD oracles and
P497 migration map fixed in frozen.json before implementation or execution.

Historical Gate A: fixed main, §§5.2/6/7/9/13/20/27; no new semantics.
B: actual Draft17.30 and historically normative Draft17.4 §5.2 both list
usize == usize -> bool. Ledger DI001/006/007/008/011 and Decision Procedure
A–E read; old indexed Surface originals absent in fixed checkout, not claimed
read. No numeric equality-specific ledger entry; no selected spelling conflict.
C: KEEP flat single selected operator/nominal exact types/Copy/LTR/dependencies;
KEEP existing field punctuation/loans/ptr != root authority/known-call contracts.
DEFER precedence/chains/overload/other numeric ops/unknown symbolic arithmetic,
IF physical effects/Pair join/CFG/effects/ABI/ownership/Array/loops.
D: N/A faithful existing §5.2/6 implementation, no ledger/spec selection change.
E: C reviews candidate/default-ON adoption; no self-merge or other Track.

Independent review plan for minimal PR503 hunks: parser has one flat binary
operator after current primary expression, contiguous ==, no precedence family.
Checker performs actual LTR Copy reads, exact usize types and known/dep-free
values, fresh bool with real child IDs/operator span, unknown precision-reject.
Review/reuse only syntax/checked/parser/checker hunks; REMOVE experimental guards
and do not inherit test-only duplicate compiler graph/flag. No PR503 modification.

Baseline prediction: all15 source cases parser CLI3 at first =, no CHECK/native.
Record binary SHA and all --check/emission results before editing compiler.
PhaseA default-ON expected source/owned bool1/0 (2/2 vs2/3), renamed Copy17/17,
direct17/19 false. Wrong type semantic vs unsupported grammar and unknown
precision separate. Pure-unit if CHECK only; full immutable Pair hits known
P6 relational join blocker and stays HOLD. No change to source_if/arm_frame.

PhaseB bounded scalar/binding/identifier/statement emitter only: authenticate
actual checked child IDs/types/Copy source bindings/source spans/fresh bool and
scalar consistency; emit real _Bool from C operand expression lhs == rhs, NOT
known checker bool0/1. Preserve size_t width assertion from P500. Iterative
source Copy chain checks, no arbitrary numeric ABI. Passive observer reads
already generated source locals/operand values/bool and saved Copy. Strict
GCC/Clang O0/O2 plain+observed, same observer must reject flipped comparison
and broken Copy carrier. No host staging or guest-state repair. Backend-private
mutations of test-owned genuine artifacts may test gate refusals, restored on
all normal test paths; no host-generated semantic worlds as positive evidence.

P497 migration: only relevant assertion codes/categories/output wording in
frontier_test.py, no .nl/hash/baseline.json changes. Map frozen.json records
old parser boundary -> Pair precision / mismatch semantic / bool-ref semantic.
Historical P497 frozen metadata stays historical; no broad test suppression.
Default CLI now admits finite equality. No alternate compiler, no new flag.

Budget120–210min MAX, one P branch/DRAFT and one natural FULL+PARTIAL CI pair,
no reruns/shopping. Scope first independent barrier: no IF emitter or Pair join
work. Semantic surprise, forged-origin acceptance, failed rollback/OOM, new
source selection or broad mechanism -> STOP preserve evidence and report.

Goal: Compass -> Product Strategy§1.1 / Coverage Map§§3.1–3.3 / Process§4.3.1:
quantity-to-bool physical result is reusable technical Exploration, not author+
ordinary caller UB-audit locality/Product GO. Alternatives are current literal
path, stopped PR503 source-only isolation, separately scoped IF/Copy selection.
Bounds/byte/Array/Ring/H1/STB remain unmeasured/HOLD; no fairness win inferred.
