# P475 — pre-execution single source/oracle freeze

Fixed Compiler/main7139b8c64bf001001e4319c9a54bf9114e130f51, CURRENT_SPEC
Draft17.30. No compiler execution for this candidate/control yet in this task.
Compass -> Product Strategy1–6,9; Roadmap; Process4.2/4.3.1 read. Canonical
13.5c and18.7 preserve actual referent substitution/caller-visible state;
11/13 scoped ref nonescape,17.4 Change,26 Reset and20.4 safe API obligations
remain. Historical Gate4.2 N/A: unchanged selected semantics, no new syntax/API.

Goal: first safe named-call boundary after P471's actual inline q cross-root
write; necessary technical step toward libraries, not proof of a safe private
unchecked wrapper, product value, Ring, heap ownership or general function ABI.
Evidence: fixed-main P471 A source control hash c578...050 (frozen.json),
main FULL38718479149/PARTIAL38718479062 success/attempt1; public join probe and
native passive observer are adopted. Historical G1 PR384 at8e80af60... used
ref<write,u8>,u8->u8 on unmerged draft, CLOSED NOT MERGED. It is not current
main ref<write,Option<ptr<Node>>> function support.

Read-only source inspection: parser.c function_declaration uses type(parser)
for formals/result, ref recursively takes a type target, exact Option<ptr<Node>>
is already parsed. No new syntax is invented. semantic_check.c body_plain_type
only allows zero-field plain nominal/bool/unit/byte/u8/usize/addr; body_signature_type
allows sum only with plain payload, ref/ptr only with plain target (ref parameters
only). Thus Option<ptr<Node>> result and ref<write,Option<ptr<Node>>> parameter
appear outside current ordinary known-body signature profile. Registration checks
all signatures before definition bodies or actual main() invocation. Existing
function_source tests cover generic host CopyT ref calls but do not prove this
actual-source Node/Option signature. Existing Node C17 contract also lists calls
as outside its bounded emitter; backend not assumed reached.

Prediction: existing grammar likely parses; semantic registration likely first
fails P8-SIGNATURE-PRECISION. Alternative later barriers: independent definition
formal referent precision, actual write-ref substitution/currentness/loan exit,
public source-issued call-effect certificate, then backend. Predictions are not
measurements and valid semantics does not imply current source-profile support.

One candidate only: add ordinary source-defined set_link(ref<write,Option<ptr<Node>>>,
Option<ptr<Node>>)->Option<ptr<Node>> with exactly one replace(field,next),
replace ONLY selected Some-arm inline replace by set_link(w,Some(q)). Genuine
q from head.next, original two distinct tail/head, implicit local authority,
payload2/1, same scoped loan and returned old None, final head reset unchanged.
Expected physical result on Some path: original tail self-link; unchanged payloads,
old Some head Copy preserved. No refs escape; no ownership created from ptr.
Full source and frozen.json hash precede execution and will not be corrected to
force success. No source/oracle spelling corrections so far.

Alternatives: assume unmerged G1 supports Node calls (invalid), repeat inline P471
(lower information), add Ring/unchecked/general mechanism (unauthorized), chosen
single known-safe-helper probe (bounded). Goal alignment: technical Exploration
only; ordinary caller/private-library benefit remains unproved, H1/STB/V holds.

First run exact unchanged control source/check/public/C17/strict native (or cite
actual latest fixed-main CI). Then candidate --check ONCE; first diagnostic or
required rewrite stops. CHECK0 alone is not public call-effect proof; absent
precise adapter means PUBLIC EVIDENCE NOT ESTABLISHED and no native inference.
Only genuine proof permits ordinary C17 then strict physical observation. Any
safety/authority mismatch stops entire task. No private checker structs, semantic
seeds, fake Domain/Allocation or patched C. No Production/spec/toolchain/adopted
test/doc/workflow changes; no feature flags, prior PR merge or other Track.
Prefer one frontend-only local build, no LLVM download. First CHECK barrier needs
no large harness/PR/CI. At most one test-only branch/PR and one natural FULL+
PARTIAL if truly needed; no retries. One final Issue475 report then stop; C owns
next map improvement/investment, no automatic new issue.
