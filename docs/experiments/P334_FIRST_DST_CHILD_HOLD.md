# P334 — first original dst.child adoption / exact source-profile HOLD

Track: P. **HOLD — FIRST NINTH ADOPTION SOURCE/OWNED PROFILE BARRIER**.
Evidence only, DRAFT / DO NOT MERGE. No compiler, header, CMake, public API,
existing input, canonical document or previous PR modification. No new
flag, capacity/predicate addition, second adoption or native work.

## Start gate and fixed authority

Issue334's initial blocked gate was explicitly released by Coordination
[comment6113375396](https://github.com/wakairo/NewLang_Compiler/issues/334#issuecomment-6113375396),
also #268 comment6113384652, AFTER V332 GO FIRST and R333 independently
composed source-owned CLEAN(LIMITED NONBLIND) were separately accepted and
closed. Release read before any compiler run/branch/source change; user
then explicitly launched this task. See `evidence/p334/preflight.md` for
completed historical Gates A–C and KEEP/DEFER comparison.

Compiler/main `a5343bf70004b30a8b026e52aa2fde93b5920be0`, CURRENT_SPEC
canonical Draft17.30. FormalProof/main
`a8de4ebb4099f4742fc161d26867740dcac48414`, F59 proof-only, no source grants.
Frozen actual compiler PR326 `de9c4b175784f484675b670a109f62edeaf35de1`;
P329 input/observer pinned from evidence-only PR331
`46a02e60cabce1b63cc3db4bc9035fe06f5f8009`. Both original heads and live main
reverified at activation, both PRs DRAFT/DO NOT MERGE unchanged. New branch
starts exact PR326; all ten inherited experimental flags ON only for the
isolated evidence build, original defaults OFF unchanged. Draft17.31/DI015
remains HOLD; H1 PASS/FAIL undecided.

## Fresh unchanged compiler calibration

Rebuilt frozen PR326 as-is, GCC14.2 Debug. Compiler binary SHA in
`authority.json`. Byte-identical P329 calibration source
`calibration_returned_owner.nl`, SHA256
`bd7692f8118c8e4de2b7662b0f90fcf503a548d9948bfded27bd430ad127716d`:
parser0/registration0/real-main checker0, actual nonNULL owned artifact,
public FIVE captured +TWO scoped +ONE requiring pair validators0,
actual original B/dst pure returned whole and donor-first/receiver-late
release order `[2,4,1,3,5]`; CLI4 `V1-BACKEND-UNSUPPORTED`, stdout0/no
NewLang C. Five original unique A values `[1,15,29,43,57]`, D values
`[6,20,34,48,62]`; B root31/inc34/R3/D3 and dst root57/inc60/R5/D5;
all original roots/20 fixed siblings preserved through pure return.
Static source release worlds0..5 checked, no native fault execution.

The reused P329 observer is public-only and immutable, checksum
`6cb44a88f43c7f844f243aabca8e11ced74536d7876a8fbed0604dd2656f3619`.
Reproduction reads its exact Git object, relocates only its assertion-header
include, and compiles against the freshly built unchanged compiler. It
validates the existing baseline and SAME-context clean retry artifacts,
never claims a positive ninth-Change artifact. No host-created root/domain
facts, private state mutation or made-up standalone whole-call validator.

## Two faithful first-adoption source shapes, both refused

Each source differs from the calibrated original solely by one extra
original-dst loan immediately after the fourth C.prev detach and BEFORE
Holder repack / PURE returned-owner assembly. All five allocation sites,
all allocation-failure arms, six initial link writes, ordinary nonCopy B
whole Holder transfer, four real detach changes in bounded order
B.prev→B.next→A.next→C.prev, and donor-first/returned-only terminal source
remain byte-identical. `observe.py` asserts this exact insertion relation.
Original B.prev and B.next remain None; neither is updated. No sixth H or
extra owner component, no topology trick or altered update order.

Shape1 returns the old Copy Option from the D-scoped block into a caller
binding, as with the two existing direct detach operations:

```newlang
let old_dst_child = loan_read(life_dst) { |stable_dst_adopt|
    let write_dst_adopt=ref_from_ptr(write,ptr_dst,stable_dst_adopt);
    replace(write_dst_adopt@child,Option<ptr<Node>>::Some(moved_ptr_B))
};
```

Shape2 consumes the loan scope with unit after binding the old Copy Option
inside it. Both acquire write permission from the correct original dst
Domain, project the fixed `child` field, and use the moved Copy pointer to
original B. Neither loan or ref is intended to escape into whole return.
These are SOURCE INTENT, not evidence of an admitted ninth effect.

| Source | SHA256 | First measured boundary |
| --- | --- | --- |
| `dst_child_return_old.nl` | `bef64179aca653b04a3fe20bc3d03c81906cc7a2fd7639ffb8bb1757b3c82ffc` | parser0, registration3/CLI3, `error(precision)[CAPTURED-CLOSURE-PRECISION]`, bytes `[14661,14728)`,232:49–232:116 |
| `dst_child_scope_unit.nl` | `b7968600287220138486878c63c1bf5044126052fdaeaf15ae559cabbde0773d` | parser0, registration3/CLI3, same precision diagnostic, bytes `[14659,14726)`,232:67–232:134 |

Exact message: `complete original-owner fork/cleanup proof is unavailable`.
Both CLI results repeated identically. No parser error, no semantic-invalid
ownership judgment, no backend attempt, no partial owned entry artifact.
Both public semantic snapshots unchanged after refused registration; each
same semantic context can then register/check actual calibrated main,
publish nonNULL owned and pass all its public validators. Full source SHA,
byte/line spans and real baseline/retry observations in `observations.json`.

## Exact first blocker and limits of this measurement

Frozen `src/semantic_internal.h:131–137` selects
`NLCapturedChange *field_changes[8]` under the tenth flag.
`src/captured_closure.c:22–36`, `nl_captured_change_begin`, refuses
`field_change_count >= CHANGE_CAPACITY` with ANALYSIS_PRECISION_LIMIT.
`src/semantic_check.c:1260–1268` requests the Change certificate BEFORE
`nl_fixed_change`, and `closure_status` at99–105 classifies that result
as `CAPTURED-CLOSURE-PRECISION` at the actual replace span.

The caller has SIX recorded initial direct field Changes plus direct
A.next and C.prev: EIGHT. TWO B helper Changes are separately owned by
callee fragments, not extra caller slots. The requested dst.child would
need the NINTH caller snapshot (eleventh actual Change overall). Its
certificate allocation fails before the field mutation is admitted.
Changing where the old Copy return is bound cannot reduce those eight
preserved events. This is a finite certificate/profile limit, not evidence
that a correct Domain/ownership relation or canonical None→Some is invalid.

No capacity experiment or transient patch was made. Merely permitting
slot9 would not establish a reusable multi-event/effect proof contract or
public validator coverage; separate M/Coordination history/no-foreclosure
authorization is needed before such compiler work. This P run does not
start that work or any other Track.

Because BOTH candidates stop at registration, there is NO actual new
source-generated owned dst.child Change, fresh Some occurrence, ninth
Projection/currentness/sibling effect or post-adoption owner-return/late
terminal evidence. Those facts are measured only on the unchanged
calibration without adoption. No 8–12 post-adoption safety corpus, private
artifact poison, new OOM campaign, normal-build regression expansion,
second B.prev self-link, general soundness or native result is claimed.
This stops at the first material barrier as authorized; semantic attacks
beyond it cannot be credited from precision refusal.

Correct later cJSON second update would be `B.prev None→Some(original B)`
self-link, NEVER Some(dst), but neither second update is performed here.
Even first-adoption success would remain 1/2 adoption, not full H1 topology.

## Reproduce and diagnostic-only CI

Activate checksum-locked repo toolchain and run
`bash evidence/p334/run.sh /tmp/p334-fixed`. The fail-closed harness rebuilds
exact unchanged implementation, verifies pinned observer/source SHA,
checks the real baseline owned artifact, asserts BOTH precise refusal
status/code/span and unchanged snapshots, then checks actual SAME-context
baseline owned retries. It fails if an adoption is unexpectedly admitted,
if refusals become another classification, or if retry validators fail.
No green collector that merely ignores nonzero exit codes.

Own Actions `p334-first-adoption-diagnostic.yml` pins exact PR326 parent,
whitelists only added evidence/doc/workflow paths, checks source SHAs,
fetches pinned P329 evidence read-only, and runs the same assertions with
GCC13 Debug. Exact run/head/log results are added to the sole terminal
Issue334 report after completion. Green diagnostic CI proves the expected
HOLD reproduction/rollback, NOT successful adoption. Standard PR CI is
separate; no compiler change or new feature means no new flag matrix.

Exactly one terminal Issue334 report then STOP. No self-close, merge,
existing PR edit, canonical selection, default flag change or other Track.
