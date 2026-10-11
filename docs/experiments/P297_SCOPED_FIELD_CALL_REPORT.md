# P297 — one original D-scoped fixed-field source helper

Track: P. Authorized NewLang source-checker development and regression testing.
EXPERIMENTAL / UNSELECTED / DRAFT / DO NOT MERGE. One A.next update only.

## Authority and mandatory historical Gates A–C

Gate A: Compiler/main `a5343bf70004b30a8b026e52aa2fde93b5920be0`;
CURRENT_SPEC canonical Draft17.30 §§3.1/3.2b,4,10–14,17.4,18.7/18.8,26–27.
The separate branch starts exactly at PR295
`f6f79161f6d6f1b83d4c80275379982bceb9fa9e`. PR298
`cb7074bb71bf9fca4a953ae9e9d0064d094e3ace` is not imported and is not an
ancestor. No existing PR/main/spec/DI/evidence is edited. FormalProof/main
`e2e540a4a6bfd23c6a5ba30fc19039e7a3323b0c` is a proof-only reference;
no F theorem or pregrant is a source acceptance oracle. Issue297's latest
Coordination comment and user launch supersede its older authority SHAs.
Draft17.31 / DI-015 / PR273 remain unadopted / merge HOLD.

Gate B: read Process at fixed current main §§4.1.1/4.2, Design Decision
Procedure A–C, Ledger DI-001/006/007/009–014, canonical sections above,
M290 design report and independent Coordination review, P289 implementation
and retained tests/report, and R301's source-generated three-formal identity
review (CLEAN LIMITED, not a scoped-field proof). Fresh R296 first-pass material
was not read. Searched tracked paths and accessible git history for old
Surface Draft1/1.1, private-receiver breaktest and M0 report; original raw
files/transcript are not available here. Retained Ledger citations are evidence
of prior alternatives, not a full raw-document or M9 transcript audit. Absence
from this checkout does not prove nonexistence.

Gate C is mandatory for this finite admission widening; it is NOT N/A:

| Evidence / earlier alternative | Class | Disposition and concrete effect |
| --- | --- | --- |
| Draft17.30 DI-009/014 five original fallible H allocations and manual 0–5 cleanup | normative-current | KEEP source-origin-only A/D/R, exact fixed fields, six initial Changes and failure cleanup. No sixth root or automatic drop. |
| DI-010/007 explicit D stability, mode-preserving ref-from-ptr / `@` projection | normative-current / broader Deferred | KEEP permissions, original D and fixed projection. No Copy ptr promotion, read→write, implicit dereference or exclusivity grant. |
| §13.5c,18.7/18.8 independent body and caller substitution; DI-011 typed receiver precedent | normative-current, bounded | KEEP independent type-only formal checks and every actual caller's proof. A favorable caller is not a definition proof. This helper consumes no A/D. |
| §17.4 / §26 old current/occurrence Reset and fixed sibling identity | normative-current | KEEP exact replay and old occurrence conflict checks, including same-pointer writes. Independent prev stays current. |
| M290 Option A caller supplies real scoped field ref; Option B callee obtains p/D loan | experimental proposal | Investigate A as one UNSELECTED finite implementation candidate. DEFER B: actual full source still stops at its in-body `loan_read(dd)` applicability. No implied normative selection. |
| Surface Draft1/1.1 wider loans/ordinary calls; private receiver/M0 field experiments | experimental, retained citations only | KEEP mode separation intent; DEFER general grammar, privacy, receiver/module/ABI semantics. M0 gives no Allocation or Domain grant. |
| DI-006 unified general loan surface and M290 general effects/full4+2 | Deferred | DEFER. Existing targeted spelling is not selected as a final generalized framework. |
| Copy-pointer-only safe-ref shortcut or assumed matched tuple | rejected alternative | KEEP the rejection. No fabricated provenance, stable D, Allocation or Matched permission. |

No Ledger/spec update: Issue297 authorizes an UNSELECTED compiler experiment,
not normative source/API/semantic selection. Existing core invariants are
unchanged. Future ordinary signatures, loan spelling, ref/ptr location
projection, privacy, general effects and ownership interfaces are not foreclosed.
Further Gate D/E adjudication is external to this task.

## Frozen source and exact first obstacle

Immutable `experimental_five_root_detach_attach.nl` SHA256
`8763eb98f68d502b8a26b49d8a023ba38dcce45090253bf2c6695fc5391d3512`.
Frozen PR295 repeated twice: CLI exit3; parser OK; registration status2
**UNSUPPORTED**, `ALLOCATED-DOMAIN-LOAN` [2140,2282), **53:25–56:6**.
The actual `attach_whole` definition has `loan_read(dd)`, then
`ref_from_ptr(write,pd,s_dst)` and `replace(w_dst@child,Some(ptr_B))`.
Transaction rolls back; entry/backend are not attempted and no owned entry/C
is produced. It is not a parser refusal or semantic rejection of an invalid D.
The new candidate retains that exact first obstacle and byte-identical fixture.
`P297_FROZEN_FULL_SOURCE.json` records the two CLI runs and public boundary.

The new independent actual source `experimental_scoped_field_call.nl` SHA256
`e83854ba2b37f8c522fa119c282ce48d5b4914e8868ebe7f51f96ad2e96f5096` retains
five original H allocation sites, all failure cleanup, six initial field writes
and five successful frees. It adds only one A.next write Some(ptr_B) → Some(ptr_C)
inside a caller loan of actual life_A. It does not detach B or transfer its
release responsibility. On frozen PR295, the same source parses but registration
status2 `FIVE-ROOT-SOURCE-PROFILE` [148,268), 3:1–8:2, CLI exit3, rollback/no entry.
`P297_FROZEN_SCOPED_SOURCE.json` records this exact-source comparison.

## Finite implementation

New explicit `NEWLANG_EXPERIMENTAL_SCOPED_FIELD_CALL` defaults OFF and requires
all five predecessor opt-ins. New helper profile: one same-unit ordinary known
source definition, two formals `(ref<write,Option<ptr<H>>>,Option<ptr<H>>)`,
unit result; exactly `let old=replace(formal_ref,formal_new); unit`.
The full old Copy/Discardable Option is disposed at scope exit. Names do not
grant ownership. No owner, loan acquisition, nested call, branch or escape in
this body is admitted. One explicit caller `@next` and one helper call only.
Other link fields, a second helper call and a second replace remain precision
HOLD. The standalone host body-registration API is not widened.

Before checking main, registry-only shadow worlds check all four old/new
Option variants (None/Some × None/Some). The formal referent is a symbolic local
Option and Some contains an unknown Copy ptr token; there is NO original H,
R, Allocation, Domain identity, matched relation or caller-proven safe pointer.
The ref's ordinary write contract suffices for one replace/disposal; the Copy
token is never dereferenced or promoted. These private definition worlds are
discarded. Definition order and nominal/function/formal renaming do not matter.

Actual call copies its scoped ref and Copy Option using existing call mechanics.
It checks the real caller projection, active same-D stability, current physical
root/incarnation, original backing write permission and current field fact.
The callee owns ONE existing Change before/after capsule. Caller retains SIX
initial capsules; the global captured-change budget is not increased. Shared
validation rederives caller ref-from-ptr/loan/projection and exact existing
`nl_fixed_change` replay. Finite closure trace integrates this one body effect
while preserving all five original release responsibilities. No fake FIELD_REF,
owner grant, extra root/domain or new Change/Reset law is generated.

The new public read-only view/validator reports actual owned caller/callee
artifact state. Normal builds expose no accepted helper through this API.
The new loan escape diagnostic is scoped to the opt-in helper path:
`P297-LOAN-ESCAPE` is semantic, replacing the historical generic P3-INTERNAL
classification there; predecessor profiles retain their diagnostics.

## Actual owned artifact evidence

All five positive sources generate a non-NULL actual owned `main()` artifact
through public parse/register/check APIs. Five captured closures and the new
scoped-call validator pass. CLI exit4 `V1-BACKEND-UNSUPPORTED` is separate:
source acceptance does not imply native generation/execution.
The public observer includes no private headers or seeded authority.

| Evidence in baseline source | Observed |
| --- | --- |
| Original A current root / incarnation / D / R | root18 / incarnation21 / D2 / R2 |
| Opaque ProjectionId representation / child incarnation | nominal10,index0 (`next`); child22 / incarnation22 |
| A.next current fact | 80 → 107; root fact81 → 108; same fixed child incarnation |
| Old/new Some occurrence | 3 ENDED → fresh8 LIVE, B/R3 → C/R4 pointer token; old payload place76 noncurrent (value/fact0) |
| A.prev | entire public current place view unchanged; separate fixed child/occurrence |
| Original Allocation/Domain values | A[1,15,29,43,57], D[6,20,34,48,62], unchanged and each one live owner in both worlds |
| Scoped reference | scope active before Change, ended in final caller world |
| Successful source primitive frees | five exact full-R/Allocation matches, each original once |
| Caller/callee Changes | 6 / 1 |

Some(B)→Some(B) separately proves a fresh occurrence despite equal Copy pointer
facts. Some→None ends old occurrence without a new conditional payload. This is
static checked-world evidence, not native memory/runtime output or full4+2.

## Source controls and exact classifications

`P297_SOURCE_OBSERVATIONS.json` records all **27** source SHA256s, repeated CLI
exit/diagnostic, byte span and excerpt, public parse/registration rollback, and
positive owned views. CLI runs twice per source; rejected units publish no entry
and backend is not attempted. None produces C/native output.

- **5 accepted source / backend unsupported:** baseline, alpha rename, definition
  after caller, same-pointer/fresh-occurrence, Some→None. Public observer exit0.
- **11 semantic refusals**, parser0/registration1/CLI3: wrong original D
  ALLOCATED-DOMAIN-MISMATCH; read ref→write formal P3-TYPE-MISMATCH; Copy ptr
  as stability ALLOCATED-REF-TYPE; Copy ptr as safe formal P3-TYPE-MISMATCH;
  active-loan D move and EndRoot P3-REF-CONFLICT; callee ref escape P8-BODY-RESULT;
  caller scoped escape P297-LOAN-ESCAPE; orphan A P5-SCOPE-OBLIGATION;
  duplicate D P3-USE-AFTER-CONSUME; Allocation as Copy payload P6-PAYLOAD-TYPE.
- **6 precision HOLD**, parser0/registration3/CLI3: borrowed old Some payload
  P9-MATCH-PRECISION **before replace**; extra Unknown formal P8-SIGNATURE-PRECISION;
  another link or second call P297-CALL-PRECISION; additional same-field alias
  CAPTURED-CLOSURE-PRECISION; second replace P297-BODY-PRECISION.
- **3 unsupported profiles**, parser0/registration2/CLI3: payload projection
  HEAP-LINK-FIELD-PROFILE; ordinary field reloan FIELD-PROFILE; immutable full
  source ALLOCATED-DOMAIN-LOAN.
- **2 parser gates**, no semantic registration: absent stability argument
  ALLOCATED-CALL-ARITY parse3; exclusive-field-loan spelling
  P5-EXPRESSION-UNSUPPORTED parse2. Both CLI3.

The old-occurrence source control is NOT a demonstrated semantic rejection:
borrowed match on this ref-valued sum lacks current source precision. Likewise
additional nonexclusive aliases are not intrinsically unsafe; their post-effects
are outside this capsule profile. Competing field reloan/exclusive source forms
stop before a meaningful effect proof. These controls remain explicit HOLD,
not safety successes or generalized borrow support.

## Validator inconsistency controls and sampled OOM

Ten controlled mutations of an actual owned source artifact reject through
BOTH public scoped-call and captured-closure validators: wrong ProjectionId,
stale child current fact, wrong post occurrence, stale root incarnation,
capability result, old occurrence left live, changed sibling fact, duplicate/
wrong Allocation region, wrong governing D, and a retained old-occurrence ref.
Every mutation is restored; the original public validators then pass. The
existing backend consistently refuses without output; no backend was expanded.
The private test mutator does not construct or seed any accepted source state.

A replay-only new validator initially missed the deliberately retained
old-occurrence ref. This was a validation gap in the new unpublished candidate,
not source acceptance of the old-payload-ref program (that source still stops
at P9 precision). The final new validator rederives the primitive's live
old-occurrence and exclusive-overlap conflicts before replay. The new control
rejects; legacy primitive rules and general borrow framework are unchanged.

Sampled malloc/calloc/realloc injection: **7 points at each of 3 stages = 21**
failures. Baseline allocation-call denominators: registration9321, actual entry
check8324, public scoped-call validator109. Points: 0,1,2,floor(n/4),floor(n/2),
floor(3n/4),n−1. Each fails explicitly OUT_OF_MEMORY, keeps caller snapshot
unchanged, exposes no partial owned entry where applicable and permits a clean
same-context retry. The validator remains read-only. This is sampling, NOT an
exhaustive sweep of all 17,754 allocation calls.

## Validation and final limits

Local GCC14 Debug: normal OFF **249/249**, all five predecessor flags ON/new
OFF **269/269**, all six flags ON **271/271**, plus the final expanded public
observer/source tests and inconsistency tests **2/2**. Exact-head remote run
results are recorded in the final PR/Issue297 report. CI adds the bounded Issue297 profile
to all five existing jobs: GCC Debug, GCC Release, Clang, ASan/LSan, UBSan,
while retaining all six previous profile steps. No CI failure is treated as an
accepted source or a platform bypass. Formatting and diff checks are required.

STOP after one field and finite controls. Full source's callee-internal D loan,
borrowed old payload match and alias/effect precision remain HOLD. No actual
4 detach+2 adopt, TreeFour physical transfer success, native backend/cJSON
implementation, H1 PASS/FAIL, canonical adoption, self-merge, issue self-close or
other Track launch. The terminal disposition, only after verification, is
reported on Issue297; this file itself does not select language semantics.
