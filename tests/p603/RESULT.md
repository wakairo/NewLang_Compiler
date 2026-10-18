# P603 finite quantity less-than, source→owned Copy→C17/native

Fixed base262f4bddb9055a6cf9067a639cbe44fa72b703a4, CURRENT_SPEC Draft17.32.
Unmerged PR599 is NOT base and remains untouched. Existing public addition
header/issuer and semantic_internal.h remain byte-identical; no new privileged
issuer, receipt list, public arithmetic API or validator. Initial freeze:
2026-10-18T11:52:07.209320+00:00 (20:52 JST), 27 sources/oracles in frozen.json.
Baseline binary was built from the exact fixed checkout. Latest main independently
observed same262f4bd; push CI39118050844/39118050507 first-attempt success.

## Baseline and admitted finite rung

Before production edits, all27 < inputs refused at SOURCE/CLI3/stdout0:
literals P5-EXPECTED-ITEM-END, lexical names P5-EXPRESSION-UNSUPPORTED.
Original P504 == positive/TYPE/PRECISION and PR590 + positive/overflow controls
retain statuses and original emitted C SHA256. Nine selected related baseline
CTests passed. baseline.json keeps exact binary SHA, spans and phases.

Normal default-ON candidate uses exactly one flat typed/name atom < atom.
No precedence/chaining/mixed/parenthesized/unary/bare/call/field/loan/ptr family
or other operator added. Append-only syntax/checked tags preserve old enum IDs.
The existing equality checker evaluates real operands left-to-right, requires
exact known dependency-free core usize, creates one fresh core Copy bool with
0/1, and keeps real ordered child IDs and operator spans. It cannot create
Allocation, Domain, ref, root or indexed-access authority. Total < needs no
arithmetic range precondition; unknown quantities still precision-reject.

Existing driver-private scalar consistency gate is shared with equality. It
matches original quantity/Copy binding origins, ordered children/spans, fresh
bool and cached truth. It additionally validates exact source operator bytes
(< or ==), then normal C17 computes the ORIGINAL guest size_t lhs < rhs.
It does not emit a checker-known bool0/1. Bool Copy emits the original bool
carrier. Existing SIZE_MAX/exact-u8/CHAR_BIT guards remain unchanged. Pure
literal/local Copy operands have no observable evaluation-order ambiguity in C.
No IF consumer, physical branch profile, ABI or memory-effect mechanism changed.

## Measured stages and negative limits

Nine immutable positives reach SOURCE/CHECK0, real public owned observations,
normal C17 CLI0, strict -std=c17 -Wall -Wextra -Wpedantic -Werror and native:
2<3 true;3<2 false;7<7 false;0<0 false;0<MAX true;MAX<0 false;MAX<MAX false;
named/copy19<23 true;distinct equal-valued bindings19<19 false. Each includes
one real bool Copy. Public probe uses registered main() checked-owned evidence
AFTER original input/parser/AST teardown. It checks exact operand types/Copy,
source operator span and LTR edges, fresh real value IDs, static truth, saved
Copy and absence of memory rights. Retained function-body source spans are local
to that registered body, whereas CLI diagnostics use original file coordinates.
These are historical public observations, NOT an independent privileged public
validator or untrusted-IR theorem. The source checker/registered plan are trusted.

u8/usize and Copy-pointer ordering: semantic USIZE-LESS-THAN-TYPE CLI3;
unknown usize formals: precision USIZE-LESS-THAN-PRECISION CLI3;
bool-as-write-ref: semantic P3-WRITE-REF-REQUIRED CLI3. Thirteen excluded grammar
forms remain SOURCE refusals. All negative CLI paths produce zero C bytes.

The initial uintptr/usize TYPE oracle is NOT met: exact uintptr source type
resolution stops EARLIER at semantic P3-UNKNOWN-TYPE (1:14–1:21). The original
source and oracle are preserved, the harness explicitly records this calibration.
It is not evidence that uintptr comparison TYPE checking or ordering works.
No currently usable dependency-bearing usize source was found; unknown formal
precision and bounded malformed owned dependency negatives are separate evidence.
No new type/source mechanism was introduced to make a negative reach farther.

## Native, mutation and rollback evidence

GCC14.2 and local Clang23.1.2 at O0/O2: per compiler36 plain/original-passive
exit0 executions. Observers only read original guest operands/bool/Copy; they
never assign guest state or compute a substitute comparison. Synthetic C
mutants <→<=, <→> and corrupted Copy use the SAME observer. Per compiler36
mutants exit91,18 yield the same truth and correctly exit0. In particular equal
cases detect <= while unequal cases detect >; indistinguishable outputs are
not falsely claimed as detected. ASan/LSan focused tests also compile/run actual
sanitized generated C. No TI/cross-target claim. All C bytes/hashes in
*.c17.txt and generated-index.json, status rows gcc-/clang-native.jsonl.

Driver-private gate rejects22 malformed cases per literal and25 per named
source, restoring original acceptance. Cases include wrong checked operator tag,
actual source token, operand edges/count/order, operator/whole/overlap spans,
wrong fresh result, cached bool, missing scalar facts, same-number wrong child
or lexical binding, and actual result package dependency/Domain/Allocation/ref
contamination. A semantic snapshot before/after validation remains unchanged.
These are bounded private consistency checks, NOT a new public hostile-world
validator, exhaustive AST corruption or cross-context replay guarantee.

OOM sweeps: parser16, source semantic32, registration101, actual main44 failure
indices before success. Every prior injected failure returns explicit OOM,
partial tree/artifact NULL and semantic state rollback. Semantic rejection
snapshots roll back too; old equality OOM tests retained. No new backend gate
allocation; exhaustive stdio/output-I/O failure injection NOT RUN.

## Honest test migrations and local harness repairs

P504's exact historical a<b parser input now has an explicit parser-success
assertion; it was moved from the rejected list without changing the input.
All other equality positives and safety negatives remain active. No selected
canonical source/semantic rule or frozen comparison source was rewritten.

Array4 opt-in remains unchanged. Adding < causes normal default-OFF Array<...
SOURCE parsing to advance to the existing Option<u8> unsupported type guard:
P5-EXPRESSION-UNSUPPORTED→REC-TYPE-PROFILE, still CLI3/output0. A second exact
unchanged-main checkout was rebuilt for this supplementary diagnostic comparison
after implementation; this later measurement is NOT called pre-edit evidence.
All12 old Array .nl bytes/hashes stay unchanged (array-migration.json and exact
baseline/candidate logs). Only two default-OFF diagnostic assertions migrated;
all original opt-in public validator, roots/elements, semantic negatives,
physical placement/native and tampering tests stay unchanged and pass.

Initial focused harness had two test mistakes, retained in focused.log and
focused-final.log: adding one byte of harmless whitespace to a child span need
not be a rejection; and identifiers do not carry literal scalar_result metadata.
Tests now attack actual operator overlap and read the real semantic package for
same-number origin tests. No production weakening or source/oracle rewrite was
used. Initial full suite failed only the two old Array diagnostic assertions;
local-suite-initial.log retained. Final GCC frontend327/327, Clang focused10/10,
ASan/LSan focused5/5 PASS. Definitive Actions statuses are in final Issue
report and final logs; no failed run is erased or relabeled initial success. Stored CTest logs trim
trailing whitespace only (for git diff --check); diagnostics/statuses retained.

## Reproduce, cost and goal alignment

. /workspace/NewLang_Compiler/.deps/activate.sh  # local toolchain only
cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build -R 'p603|p504|p589|p534|p541' -V
ctest --test-dir build --output-on-failure

Local frontend-only is PARTIAL, not LLVM23.1.3 full evidence. DRAFT PR triggers
exactly one FULL5 and one NO LLVM/PARTIAL run, no reruns/post-CI head pushes;
final URLs and outcomes belong to once-only Issue603 report. No workflow/lock
or bootstrap changes. Local Clang23.1.2 differs from CI's pinned23.1.3.

No new event/list/world clone/public proof representation. Scalar comparison
adds the same one fresh bool package and two existing ordered operands as ==.
Extra validation checks source token bytes and one integer <; existing bounded
origin/Copy-chain scans reused. Only parser, appended syntax/checked tags,
shared checker and existing emitter, minimal CMake and tests changed. Compile/
memory cost of the new test targets and whole-program scaling are not benchmarked.

Historical audit: faithful selected Draft17.32§§4/5.2/6/7/27; old Draft17.4 also
lists total usize <. KEEP exact nominal quantity types/Copy/LTR/dependencies;
Ledger DI001/006/007/008/011 memory/source distinctions preserved. Indexed old
Surface sources unavailable, no exhaustive archive claim. DEFER precedence,
unknown/dynamic guards, privileged proof, pointer order, IF native, Array index,
owner/effect/CFG. Canonical/Ledger/PR599 unchanged.

Reusable technical result: known quantity order computed from actual original
source carriers into an ordinary copied bool. Next demonstrated finite obstacle:
unknown/runtime quantities remain precision-limited. Technical PASS is NOT a
real ordinary-safe library, safe indexed access or smaller UB-prevention author+
caller audit/re-audit dependency closure vs factored C. Such API invariant/error/
mutation/teardown and comparative maintenance evidence is unmeasured. No Product,
Ring or H1 GO; no target-specific workaround. No goal deviation in this bounded
Exploration; C decides adoption and any independent review. Stop after one final
Issue report; no merge, self-close or other Track.
