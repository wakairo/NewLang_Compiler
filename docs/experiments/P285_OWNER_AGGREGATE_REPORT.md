# P #285 — finite ordinary owner aggregate source substrate

Track: P

EXPERIMENTAL / UNSELECTED / DO NOT MERGE. This tests complete acyclic nonCopy
value transport, not the full original five-H detach/adopt operation or native H1.
The new default-OFF `NEWLANG_EXPERIMENTAL_OWNER_AGGREGATES` requires the three
predecessor opt-ins. Their old source profiles are unchanged with the new flag OFF.

## Historical design audit: Gates A–C COMPLETED, bounded experiment only

Gate A: Compiler/main `b75baea96a644e68634baee383299b66981b3c62`,
CURRENT_SPEC -> canonical Draft17.30 §§3.2b,4.5,13–18,26–27 remain authoritative.
FormalProof/main `f7dcac614394a526ea9c33848cafd324f2993d19` is F49 auxiliary
HOLD, not a source-to-rich proof. Candidate PR273 `f778eba116d1834cc79c0bef6f6c17051102049f`
Draft17.31 §3.2c / DI-015 remains UNADOPTED. Issue285 and Coordination268
authorize this one targeted widening from frozen PR284
`b4db42c4b87146e130faf1feed2cf29266fda8ba` on a fresh branch; stop at the next
exact boundary. Process §§2/4.2/6/9 and Design Decision Procedure apply;
this audit is explicitly **not N/A**. Direct-authoring North Star is the product
input; S1 migration, generic owner contracts, adoption and native emission are excluded.

Gate B: opened retained Design-Intent Ledger DI-001/002/007/008/011–014,
canonical/current source sections, candidate-only DI-015 at the above SHA,
Process/Decision Procedure, North Star/Testing Strategy and P274/276/278/281
source reports; compared their experimental source alternatives. Searched
workspace and git history for old Surface Draft1/1.1, module-private receiver
breaktest and M0 report. Their raw files are absent from the accessible checkout
and refs; comparison uses the retained DI entries and reports, **not a claim of
raw full-document or full M9-chat audit**. Missing access does not establish that
an old alternative did not exist. No material conflict is silently decided here.

Gate C:

| Current / earlier option | Evidence class | Decision and reason / impact |
|---|---|---|
| Draft17.30 whole-value nonCopy/nonDiscardable rules, same D identity under move, explicit matched release | normative-current | KEEP: consume every member, preserve actual IDs; ordinary packaging never mints authority. |
| DI-001 `@` field / `::` sum / reserved `.` receiver separation; DI-007 mode-preserving bounded ref projection | normative-current / deferred extension | KEEP: no implicit ptr dereference or new partial nonCopy field extraction. |
| DI-002 defining-module-private representation and transparent escape | Deferred direction | DEFER: one existing visibility domain; constructors/patterns do not foreclose future shared privacy rules. |
| DI-008 whole `consume_array` alternative | experimental / Deferred | DEFER general arrays; whole consume is an analogy, not root entitlement. |
| Surface Draft1/1.1 general nominal fields, complete construction/destructuring and nonCopy argument/return | experimental, retained DI-011/012/013/014 citations | PROPOSED finite opt-in instance only: 1–4 nominal members, multiple types, heterogeneous result; no general grammar adoption. Wider alternative remains DEFER. |
| M0 / private receiver source experiment | experimental, retained Ledger evidence | KEEP representation/operator distinctions; M0 excludes Allocation/Domain/destroy and supplies no owner grant. DEFER modules/lookup/receiver API. |
| DI-011 independent definition proof and actual caller obligations; DI-012/013 two-H live return/custody | normative-current, bounded | KEEP release obligations. Do not generalize old LiveTail or favorable caller facts to field-edit bodies. |
| DI-014 actual five original H roots, three Copy links, initial six Changes and explicit failure cleanup | normative-current, bounded | KEEP exactly five static sites and existing Change/Reset substrate. No sixth H or owner-bearing Node field. |
| Candidate DI-015 ordinary mixed-but-complete carriers; five-H detach/adopt | proposed/unmerged | Preserve mixed pure transport experimentally; actual release must match. Full detach/adopt DEFER at measured next gate. |

The implementation budget is not a language rule: at most16 nominal declarations,
1–4 composite fields and at most4 triad leaves. Flat root carriers retain the
predecessor exact ptr<H>/Allocation/LifetimeDomain layout. A topological register
pass completes actual component types before their owner aggregate; unknown
components/cycles reject transactionally. Structural Copy/Discardable are derived
from constituents. No special meaning attaches to nominal or function names.
Complete owner patterns may use `field:local` renaming; nested/partial patterns
remain outside the slice. One pure named identity call transports a complete
supported owner value. General structs/generics/traits/modules/privacy/contracts,
ABI/FFI, field-edit function summaries and symbolic loans remain DEFER.
No normative/DI selection or Ledger edit: Issue285 explicitly authorizes an
unselected compiler experiment, not an adopted design change. Gate E HOLD remains.

## Immutable full input: old -> new first gate

`experimental_five_root_detach_attach.nl` is unchanged from PR284.
SHA256 `8763eb98f68d502b8a26b49d8a023ba38dcce45090253bf2c6695fc5391d3512`.
Five original fallible sites, all None cleanup, twelve source replace operations
(six initial + four detach + two adopt), original A/C/src-first release intent retained.
Frozen predecessor binary: parser unsupported `AVS-DECL-PROFILE`, [319,327),
TreeFour's third LiveRoot. New opt-in: parser OK; registration **precision limit**
(status3), `P8-SIGNATURE-PRECISION`, [1868,2595), complete `attach_whole` definition
(lines49–65). Its `(LiveRoot,LiveRoot,ptr<Node>)->TreeTwo` signature exceeds the
bounded body/call adapter. Context rolls back, no owned entry, backend not reached,
CLI exit3 and no C. This is neither a semantic counterexample rejection nor a
proved detach/adopt. Do not skip this signature and credit the composition.

Minimal Four/Three, two owner declarations and heterogeneous result now parse and
complete type registration; their deliberately empty main stops at the unchanged
`FIVE-ROOT-SOURCE-PROFILE` (needs five actual sites). An independent admitted-shape
whole edit helper still stops at `ALLOCATED-DOMAIN-LOAN`, not a root authority grant.
These later gates are reported separately. The inherited `if true` refusal mutation
now reveals `P13-IF-OPEN` syntax error; it is not an ownership safety rejection.

## Actual positive substrate and attacks

`experimental_owner_aggregate_transport.nl` is a **separate substrate input**:
five actual original allocations, unchanged six initial Changes / None cleanup,
TreeFour(src,A,B,C) construction and complete consume, TreeThree(src,A,C), and
heterogeneous DetachResult(TreeThree,B), followed by a pure whole identity call.
Caller fully destructures and finishes all five originals in [2,4,1,3,5] order.
No four detach or two adopt operations occur; this does not implement cJSON H1.

The public read-only observer registers actual source, checks actual `main()`,
validates all nested captured closure/failure worlds, and follows actual owned
entry/returned/received worlds and actual callee release primitives. It neither
injects grants nor uses private headers. It checks structural nonCopy and
nonDiscardable types, one current A/D per original, conserved component/value,
root/incarnation/region/domain IDs, and distinct donor/formal/caller placements.
Pure mixed Allocation return is observed before repair, without a matched flag.

Fixed source contrasts include TreeFour identity itself, heterogeneous result
identity, alpha renamed types/functions and forward declaration order. A B/C
Allocation swap survives ordinary return, then whole decomposition repairs the
original Allocation pairing and all five explicit releases pass. The same swap
without repair rejects actual B `finish_root(leaf)` with `P193-CALL-BACKING`;
a wrong original C Domain rejects there with `P193-CALL-DOMAIN`. Parser/profile
admission is separate from these genuine semantic failures. Duplicate A_B/D_B,
reused donor/receiver/formal, duplicate member/local alias, missing complete field,
missing normal-exit cleanup, cycle/unknown type and a D_B loan crossing repack
reject semantically. Partial nonCopy field projection remains `FIELD-PROFILE`
unsupported; a fifth member and >4-leaf aggregate are implementation profile
limits, not semantic safety proofs.

Owned artifact validation additionally checks complete constructor/destructure
field bijections, recursive pure-identity component fact conservation, and all
still-live original A/D carriers across call worlds (including the separate dst).
This checks conservation without imposing ptr/A/D matchedness on ordinary values.
Poison attacks occur only after real public source checking; OOM checks require
no partial publication, context rollback and clean retry.

## Validation and limits

Exact observations, source SHA256, parser/registration/CLI diagnostics/spans and
public owned ledgers are in `P285_SOURCE_OBSERVATIONS.json` and
`P285_PREDECESSOR_OBSERVATIONS.json`; runners regenerate them. Fixed predecessor
controls remain default-OFF and original/nested/transitive 22/38/36 cases; the
new opt-in deliberately admits the previously unsupported unused `Outer` shape
and moves immutable P281 controls to their newly measured gates. Assertions are
profile-specific; previous profiles retain their exact original expectations.
New CLI positives exit4 at `V1-BACKEND-UNSUPPORTED`, output NULL, no generated C.
Native cJSON, full 6+4+2, refusal/refund whole function composition, nested field
Changes, richer conditional applicability and formal source-to-rich refinement
remain unproved/unimplemented. Captured Change capacity remains6; no caller facts
are injected into definition formals. Canonical source/emitter is unchanged.

Build/test counts, toolchains, poison/OOM numbers, exact PR head and CI run are
recorded in the single Issue285 Track P report after verification. This document
is included in that head; no circular self-referential commit hash is inserted.

P CJSON ORIGINAL OWNER AGGREGATE SOURCE: FINITE SUBSTRATE CANDIDATE — COMPLETE NONCOPY TRANSPORT EMPIRICALLY CHECKED, UNSELECTED
