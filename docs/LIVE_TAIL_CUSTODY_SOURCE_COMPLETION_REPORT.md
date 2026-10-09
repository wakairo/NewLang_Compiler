# Draft17.29 durable LiveTail custody — complete bounded source gate

Track: P — [Issue #217](https://github.com/wakairo/NewLang_Compiler/issues/217).

Candidate disposition: **P DRAFT17.29 DURABLE CUSTODY SOURCE GATE READY FOR COORDINATION REVIEW**.
Independent Coordination review and merge remain pending. This is source/semantic
acceptance and owned checked evidence; it is not native custody execution.

## Authority / exact scope

Base main: `75897e9b108185c530229c774c9d6c9951756922`.
`CURRENT_SPEC.md` points to Draft17.29; adopted §18.1c is the normative rule.
The pointer's older explanatory PROPOSED prose does not override the adopted
canonical section or the Coordination continuation/commit-update instructions.
Process, Compiler Testing Strategy, Design Decision Procedure, DI-009–013 and
merged #218/#220/#221/#223 were checked. Historical design-selection gate: N/A
for implementation of this adopted bounded profile. Semantic delta: **0**.
Canonical Draft, CURRENT_SPEC, Ledger, frozen oracle and source fixture are unchanged.

The complete §18.1c.4 fixture is still
`tests/fixtures/live_tail_custody.nl`, SHA-256
`a30bbc37ff817020165d44a1436cb98ac934568f557068b378bf3118227d5644`.
The existing canonical-comparison test removes only comment-only lines, because
comments remain outside the current lexer. No executable statement is removed.
Earlier partial/HOLD reports describe their cited older candidates; this report
records the complete source gate on the new base.

## Actual transfer and source checking

The existing independently checked `NLCustodyDefinition` remains conditional.
Each actual recipient call first uses the read-only admission path to prove the
inherited, world-qualified original packet and its live O/R/D/Allocation,
separate caller-owned exact-None root, ordinary scoped write permission,
source loan operand, and absence of unresolved dependencies or live aliases.
This happens before argument evaluation/consumption. Body shape alone grants
no caller authority.

The same production body checker then replays the retained source plan with the
actual arguments. It consumes the original packet into `Some(packet)`, performs
ordinary `replace`, and explicitly consumes its old None at the first special
site. The original tail stays live; no EndRoot/free is added. The sink's fresh
payload occurrence and current fact, original packet, actual consumed donor and
formals, physical root incarnation and live region/domain are retained in owned
entry/post snapshots. Source/callee loans close through the existing scope rules.
Both Option variants remain statically nonCopy and nonDiscardable.

## Differential continuation and common final closure

| Independently checked policy world | Custody / original tail | Same later source suffix |
|---|---|---|
| Adoption | Some(original packet), fresh occurrence; tail live | fresh loan/replace returns Some; whole saved packet received and terminally released |
| Pre-consume refusal | None; original packet received/released by refusal arm | fresh loan/replace returns None; no owner is constructed in unreachable Some |

The policy match retains **both** arm worlds. `normal_frame_unchanged` is false;
common retained/closed packet flags are not substituted for this differential
state. The remaining lexical block is checked separately in each owned world,
under the existing shared finite work/resource budgets. The actual policy
scrutinee does not select the favorable alternative.

The later recovered match is syntactically exhaustive in both continuations.
Its possible tag is source-proven separately in each correlated world. Only the
reachable arm executes there, so the refusal world never gains a hypothetical
Some owner. Across the two policy worlds both recovery paths are checked.
Whole receiving retains the original packet field identities; the existing
terminal receiver independently re-proves the O/R/D/Allocation relation.
The second one-arm consuming match is limited to the original custody binding,
after the one actual extraction, when its current value is exact None and loans
have ended. Unrelated None values and other incomplete matches gain no exception.

A parent-derived closed-prefix target is constructed from the existing
parent closed-tail proof plus explicit head/custody closure obligations. **It is
not committed until both complete source suffixes prove it**. No arm's state is
chosen as the public result. The comparison for adoption projects only the
proved ownership edge from the original packet local to C's new payload; all
other inherited facts must match. For final closure, only the historical,
consumed, payload-free None ID in C's binding is normalized in a temporary
comparison clone. No live owner, root, region, domain or occurrence identity is
rebased across worlds. No new source owner/effect contract is introduced.

## Owned checked evidence / module contract

`NLCheckedFragment` owns recipient entry/post snapshots, both policy arm worlds,
both source suffix artifacts with their exact starting worlds, and the common
final target. Parent/world pointers are borrowed only within the owning tree.
Destruction releases all worlds/body evidence, including partially built OOM
paths. The caller context changes only through the enclosing successful
transaction.

`NLCheckedNodeView` records actual recipient operands and donor/formals, original
packet/world, sink incarnation, displaced None, new Some/occurrence, later
replace's old/new sums and occurrence, the two special match sites and selected
recovery variant. `nl_checked_custody_continuation` returns a borrowed per-arm
suffix; its numeric IDs belong to that suffix's owned world.
`nl_checked_custody_valid` reads this evidence, reconstructs the parent-only
closure target and verifies both alternative certificates. It does not replay
source/AST or mint/consume authority; validation OOM returns false.

Tests destroy original input AST/source and entry source before validation.
**64 poison controls per positive** reject foreign/sibling cloned worlds,
missing normal alternatives, fake unchanged/retained flags, wrong ancestor,
unknown/may-Some sink, stale sink incarnation/occurrence, changed original
packet fields/dependencies, missing first/second None evidence, corrupted
extraction, whole receiving fields and terminal donor/parameter/owner tuples.
No numerical coincidence between sibling worlds supplies an origin proof.

## Actual-source tests and accurate rejection stages

`custody_source.integration` tests six complete admissions: primary, refusal,
recipient rename, header/local rename, renamed refusal and declaration order.
Each must reach **exit 4, V1-BACKEND-UNSUPPORTED, empty C output**, then pass the
owned-evidence test on an actual checked `main()` call. Native success is not
claimed. The old integration also retains five independent-definition positives,
16 definition-shape negatives, seven pre-consume guard negatives, three producer
identity negatives, and both old closed/retained join prerequisites.

The new integration independently checks 25 destructive actual sources:

- Post-adoption donor reuse and double terminal release: `P3-USE-AFTER-CONSUME`.
- Wrong later ptr/domain and Allocation: `P193-CALL-DOMAIN/BACKING`.
- Missing tail/head release or final custody consumption: `P5-SCOPE-OBLIGATION`.
- Missing recovery arms, premature/unrelated one-arm None: `P6-EXHAUSTIVENESS`;
  Some wildcard: `P6-PAYLOAD-DISCARD`.
- Double extraction, recovery alias or extra scope: structured custody precision
  rejection. Read-mode recovery and consumption during a live loan use the
  existing type/reference-conflict rules.
- Retained-vs-closed policy without actual adoption cannot be forced into this
  custody continuation: `CUSTODY-CONDITIONAL-CONTINUATION`.
- Preexisting Some construction is an **early bounded constructor guard refusal**,
  not evidence of late occupied-sink transfer rejection. Unknown/may-Some current
  sink and stale occurrence are separately attacked in owned evidence.

Registration fault injection covers **3447** allocation sites; actual checked
`main()` replay covers **2968**. Every injected failure preserves the existing
source-created u8 carrier and complete snapshot and exposes no artifact.
A source which fails at the later wrong-Allocation terminal call covers
**2696** registration fault sites before the genuine `P193-CALL-BACKING` failure;
rollback preserves the pre-existing context. Completion/retry works afterward.
A raw-invariant allocation failure must propagate as OUT_OF_MEMORY rather than
a precision rejection; the same exhaustive injection covers that distinction.
The old independent-definition OOM test is preserved and now expects successful
complete source registration rather than the superseded transfer fence.

## Validation / boundaries / handoff

Checksum-locked bootstrap verified LLVM/Clang/format **23.1.2**, CMake **3.31.6**,
Python **3.12.14**, local GCC **14.2.0**. C17/warning policy and all dependency pins
are unchanged. Standard commands remain `cmake --build <build>` and
`ctest --test-dir <build> --output-on-failure`; CI additionally checks formatting.

All previous **229 CTests** remain. Three new tests bring the suite to **232**:
owned source evidence, exhaustive OOM and complete-source integration. Full GCC,
GCC Release/NDEBUG, Clang, ASan and UBSan validation includes old 2-root native
RETURN tests, oracle.adapter, oracle.smoke and artifacts.integrity. Exact candidate
head, PR URL and the five PR-triggered CI results are recorded on Issue #217 and
the candidate PR after CI completion, avoiding a self-referential SHA in this file.

Findings: no new spec hole/ambiguity or contradiction. Conservative
COMPILER-PRECISION fences remain for additional aliases, unknown states, other
conditional ownership profiles, extra custody transfers and non-unit/terminating
recovery suffixes. This is the adopted finite two-root source gate, not a general
owner/effect or symbolic memory solver. No backend, native custody, extra root,
new field, API spelling, RAII, LLVM, cJSON or other Track work is added.

The candidate is **OPEN / unmerged** for independent Coordination review.
No merge, Issue closure or next task is authorized by this report.
