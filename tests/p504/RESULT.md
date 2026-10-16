# P504: normal usize equality -> checked bool -> real C17/native candidate

Exact base2161115b5cc25c3ef0c8c2a729f0f0ffe1b7a287; CURRENT_SPEC -> Draft17.30.
Pre-code/pre-run freeze80d02e0 (full hash from git/PR),15 immutable sources and
oracles in frozen.json. Twelve files copied byte-for-byte from frozen unmerged
PR503/e561e8f189fd67961f5ac90560e20ecc45752abd, three additional Copy/direct
literal programs frozen before any execution. PR503 branch unchanged, not base.
Historical GatesA–E/goal rationale: PREFLIGHT.md. No candidate source rewrite.

## Predicted -> measured stages

Exact unchanged main ran --check and ordinary emission for all15 (30 runs),
all parser CLI3/stdout0. Fourteen sources stopped at first = with
P5-EXPRESSION-UNSUPPORTED. Additional direct-literal source stopped at the same
operator with P5-EXPECTED-ITEM-END instead: old typed literal primary returns
early. This is a baseline diagnostic calibration, not a changed source/oracle
or semantic safety finding. Binary SHA and all outputs: baseline.json.

Normal default-ON candidate, no experimental flag or alternate compiler graph:

| Source | CHECK/public owned | C17/native |
|---|---|---|
|P501 equal_true / equal_false|0, actual2==2 bool1 /2==3 bool0|CLI0 genuine comparison; GCC/Clang O0/O2 observed1/0|
|P501 copied_renamed|0, real Copy17==17 bool1|CLI0/native1|
|new copy_true / copy_false|0, original2/2 or2/3, actual bool Copy to saved|CLI0/native saved1/0|
|new direct_false|0, original literal17/19 bool0 -> saved Copy|CLI0/native0|
|P501 if_true / if_false|0, real bool1/0 and two checked unit arms|CLI4 V1-BACKEND-UNSUPPORTED, no C/native|
|unchanged full Pair true / false|CLI3 precision/P6-JOIN-PRECISION7:5–8:70|not reached|
|u8 vs usize|CLI3 semantic/USIZE-EQUALITY-TYPE4:14–4:16|no C|
|bool as write-ref|CLI3 semantic/P3-WRITE-REF-REQUIRED5:11–5:16|no C|
|unknown formal|CLI3 precision/USIZE-EQUALITY-PRECISION1:33–1:35|no C|
|chain / split operator|parser unsupported|not reached|

Full original Pair old0 / true7 / false8 / sibling9 is still an immutable
UNTESTED physical oracle. First independent boundaries remain IF emission CLI4
and Pair relational join CHECK precision. No source_if/arm_frame/normal-frame
proof/Node join/effect/ABI/owner extension or broad arithmetic work performed.
No runtime guard or author-visible IF mutation success claimed.

## Implementation provenance and narrow backend proof

Reviewed and reused only PR503 syntax/checked/parser/checker hunks. Removed the
experimental admission guards; no P501 duplicate parser/semantic/compiler targets
or flag imported. Same core usize/bool, exact flat ==, known dependency-free
values, actual LTR operand evaluation, fresh bool package0/1 and exact checked
child IDs/operator span. Unknown/dependent quantities still precision-reject;
no source-name/literal-value whitelist, new semantic selection or host bool.

Normal Checked-C now accepts only source-connected bool equality/identifier/
binding/statement scalar flow in a function's straight-line root block. New
consistency gate validates exact bool type/result, fresh package ID after real
operands, actual LTR child edge IDs, whole/operator/child span relationship,
original quantity scalar origins and static fact consistency. Copy chains use
strictly decreasing IDs. Root declarations and retained checked source spellings
confirm identifier-to-symbol resolution, including equal-valued distinct bindings;
source bytes are read only for validation, never emitted as C names or code.
No new ownership registry or public API. Existing usize-only lowering is kept;
the extra root-block restriction applies to the new bool path. Nested/aliased/
unknown/runtime formal quantity/boolean ABI is not promised.

Physical output is const size_t original locals followed by e.g.
const _Bool nl_local_3 = (nl_local_1 == nl_local_2);
const _Bool nl_local_4 = nl_local_3;
NOT a cached checker1/0. Literal operands emit their actual checked quantities.
Both operands are pure local Copy reads/literals, so C's operand evaluation
ordering has no observable effect; no effectful call/volatile/borrow operand is
admitted. Target SIZE_MAX static assertion from P500 retained even for direct
literal comparison. Only native64-bit Linux measured, no cross-target/ABI claim.

Public probe uses real source registration/actual main() after original parser/
tree/input destruction. It reads local equality child/operand/Copy/bool facts,
retained source spans and actual receiving symbols/values; zero Domain/
Allocation/ref rights in bool. Historical source-issued observations are not a
new independent public validator or theorem. Driver-private test controls include
actual main.c, mutate only test-owned source-generated checked nodes, then
restore before disposal; they do not manufacture positive semantic worlds.

## Physical native observation and bounded malformed-evidence controls

Six frozen positive programs on GCC14.2 and Clang23.1.2, strict C17
-Wall -Wextra -Wpedantic -Werror at O0/O2. Per compiler:24 plain/passive-observer
exit0 and18 controlled corrupted-carrier exit91; totals48 successes/36 refusals.
Observer reads only original emitted size_t and _Bool locals, including saved
Copy; no guest assignment, condition repair or foreign C equality helper.
The same observer rejects flipped == to != in all six programs and corrupted
Copy in the three extra receiving-local programs. Emitted expression structure
must use actual public operand symbols/literals; bool1/0 initializer prohibited.
All C bytes/digests and native statuses in evidence/{gcc,clang}-native.jsonl
and six .c17.txt files. New integration also propagates sanitizer configuration
to generated-C compilation for CI ASan/LSan and UBSan (not merely compiler tests).

Driver-private controls reject12 mutations for each of five named-operand
sources and10 for direct literals (70 per suite). Cases: missing/extra/invalid
child, wrong cached bool, missing scalar evidence, operand-as-bool package,
wrong operator/whole span, reordered child, earlier genuine same-valued quantity
as wrong child, and same-valued sibling symbol substitution. Original validates
again after every restored corruption, semantic snapshot unchanged. These are
bounded internal emitter-gate controls on trusted-issued artifacts, not a public
untrusted-IR validator or exhaustive tamper-proof/soundness claim. No unsafe
wrong-origin acceptance was observed in the tested cases.

## Adopted P497 migration and failure/regression evidence

Only relevant assertions/categories/result wording in tests/p497/frontier_test.py
changed. Guard true/false now expected precision/P6-JOIN-PRECISION; wrong_type
semantic/USIZE-EQUALITY-TYPE; false_authority semantic/P3-WRITE-REF-REQUIRED.
All original .nl bytes/hashes, frozen.json, prior baseline.json and old raw
results preserved as historical evidence. Migration map in new frozen.json;
no skipped/deleted failing test or weakened accepted safety meaning. Literal3/9
native and overflow still run unchanged. Comparison progress is now default-ON,
not hidden behind an opt-in test compiler.

Reused reviewed P501 unit coverage under ordinary mainline semantic/parser
libraries, source-backed public/driver probes only. Unit values0/1/17/SIZE_MAX,
leading zeros, Copy/renaming, mismatch, ptr input rejection, unknown definition,
bool-as-ref and invalid else despite known-true condition. Parser/checker/
registration/actual main malloc/realloc sweeps16/32/101/44 with explicit OOM,
NULL failed artifact/tree and public snapshot rollback. No production allocator
interface or new allocation inside emitter gate. Dependency-bearing source,
arbitrary private multi-field IR corruption and general runtime inputs are
not independently proven here.

Local GCC frontend294/294 PASS35.31s; formatting PASS. P471/P490/u8/Pair/Node,
ref/currentness/nonCopy/EndRoot/Option and P497 regressions remain in full suite.
One natural FULL5 + NO LLVM/PARTIAL CI pair on the final PR HEAD; definitive
run/job/SHA evidence in PR and once-only Issue report, no rerun/post-CI push.
No toolchain/workflow/LLVM lock change (full CI retains curated23.1.3).

## Reproduce / scope and cost

Configure default FRONTEND_ONLY=ON for local LLVM-free build, build all,
ctest -R p504 -V then all. No feature flag. frontier_test.py takes normal
newlangc, public probe, driver-gate probe, C compiler, tests/p504, sanitizer
(none/address/undefined). ASan/UBSan CI uses actual sanitized generated native C.
Two test probes reuse normal production libraries; gate probe includes actual
driver only to access static consistency gate, not a duplicate frontend graph.

Historical audit N/A faithful selected §5.2/6; Draft17.4 and17.30 exact operator
lists and DI001/006/007/008/011 KEEP. No general precedence/source rule change,
Ledger/Canonical/Compass/coverage/product strategy unchanged. Old indexed Surface
originals unavailable, no exhaustive archive claim. Known limitation: new bool
C17 path root-block/source-known only; widening requires separate review.

Exploration result: a real quantity equality bool is now physically computed
and copied through normal source/C17/native. Array/byte bounds, general arithmetic,
unknown runtime guards, physical IF, original Pair effect join, API audit
locality/C-Zig-Rust advantage and H1/STB/Ring Product GO are NOT demonstrated.
C decides review/adoption/independent validation and any next milestone. P stops
after once-only report; no merge, Issue close or other Track.
