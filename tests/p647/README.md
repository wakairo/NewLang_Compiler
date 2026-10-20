# P647 — newly authored ordinary-source checkpoint (evidence only)

Fixed Production compiler: `f58f1b166780569704ace110d74110ede07b86d2`.
Canonical CURRENT_SPEC: Draft17.32. No Production, public API, specification,
workflow, toolchain or existing test/source changes. CMake adds test targets only.
P Work/Codex local environment was available; this is not the R-host route.

## Original experiment and pre-execution freeze

`handoff.nl` is a newly authored two-stage record update under two capacity
conditions, not a renamed P630/R644 fixture. Two independent original lexical
Pair values hold a status and a retained tag. A true comparison selects72 into
ingress.left; its returned original14 is retained. A false second comparison
selects that original14 into egress.left. Original egress.left33 is also retained.
The tags181/207 are independent siblings. These are intended source semantics,
not native observations: no guest C17 was produced.

`freeze.json` and all four source files were committed BEFORE executing them:
`aa47ef0cadbecf074efc1fe9057a270b54b4fb72`. Their hashes/oracles never changed.
The single optional `one_if_control.nl` keeps both roots and both writes, but
removes the second conditional. It diagnoses the boundary; it is not substituted
as success for the original. Wrong-quantity and additional-arm-local variants
are semantic-invalid and legal-but-unsupported controls, respectively.

| Input | SOURCE/CHECK | Ordinary C17 | Output bytes | Guest native |
|---|---|---|---|---|
| handoff.nl | 0 | 4 / V1-BACKEND-UNSUPPORTED | 0 | NOT REACHED |
| one_if_control.nl | 0 | 4 / same first frame limit | 0 | NOT REACHED |
| wrong_quantity.nl | 3 / USIZE-LESS-THAN-TYPE, 6:26–6:27 | 3 | 0 | NOT REACHED |
| extra_arm_local.nl | 0 | 4 | 0 | NOT REACHED |

The last control does not independently isolate the two-local limit: the earlier
frame rejection still applies. The first semantic-invalid input does not count
as a precision/backend failure. No expected semantic/native oracle was changed
to fit an observed result.

## Actual source-issued owned evidence and first boundary

`public_probe.c` uses public accessors on the actual registered `main()` artifact
after original syntax/parser/source teardown. It seeds no value, owner, Domain
or pointer. It observes/asserts:

- original lexical roots3/6, incarnations3/6, fields4/5 and7/8, in both first-IF
  worlds; two roots remain distinct and their children belong to their original
  parent/incarnation/index;
- first/second conditions true/false, actual arm Copy values72/8 and91/14;
  distinct owned arm worlds and conservative Unknown joined values;
- genuine source-issued changes on root3/field4 and root6/field7, old14/33,
  root facts3→13 and6→21, child facts4→12 and7→20;
- writable scoped refs to the exact children, genuine provenance and ended
  carriers; scopes1/2 both ended and nonescaping;
- sibling readback181/207 on the same children/incarnations and unchanged
  sibling ValueFacts. The main program's changed fields are conservatively
  Unknown, not a concrete72/14 falsely read from the semantic join;
- Allocation/Domain count0: lexical roots, not heap-allocated nonCopy owners.

This is genuine public **readback**, not a new public Pair/IF validator or a
claim that readback alone certifies arbitrary proof worlds. No such general
public validator is introduced or claimed. Existing backend private admission
still rejects the program.

`backend_locator.c` diagnostically calls the UNCHANGED Production predicates
on the same source-issued artifact. It finds the first rejected binding6,
initializer IF28; the selection-prefix predicates pass, while
`copy_if_pair_frame` fails its initial `src/main.c:1179` requirement
`c->scope_count == 1`. Actual count is2, both completed scopes. This is not an
active-loan semantic error. Static subsequent singleton checks also reject a
second aggregate, IF, loan or replace; simply deleting the scope guard would
not establish safe composition and is not proposed.

Checked source spans are relative to the retained function body. Body SHA256
`cabd816935d1842de35c1d40f1f3749716b82943f6bcb8feafc280ade1131497`
is824 bytes at original-unit offset51. IF span[218,266) in owned source maps to
original[269,317), line7:18–7:66. The owned bytes are read publicly and matched
exactly against the original source in `run.py`. An initial diagnostic metadata
file mistakenly mapped the body-relative span directly onto the whole source;
`evidence/initial-harness-span-mapping.json` preserves that SUPERSEDED harness
mistake (I), and `observations.json` plus final JSONL have the corrected mapping.
No source or compiler result changed. Initial unqualified `clang-format` was
also unavailable; retained `clang-format-23` was used before candidate CI.

## Reproduction and actual evidence

Build the normal frontend (`NEWLANG_FRONTEND_ONLY=ON`, no experimental compiler
opt-in), then targets `newlangc p647_public_probe p647_backend_locator` and:

```
ctest --test-dir build -R '^p647_' -V
python3 tests/p647/run.py /absolute/build/newlangc /absolute/build/p647_public_probe /absolute/build/p647_backend_locator /absolute/tests/p647
```

`evidence/local-gcc13*`, `local-clang23*`, `local-asan*` retain actual commands,
diagnostics, public observations and first rejection. GCC13, Clang23.1.2 and
ASan host runs PASS the boundary-observation test. Strict C17 host-test builds
are not generated NewLang guest/native success. The post-failure check of the
same source returns0; errors emitted no partial C. ASan covers actual cleanup
of these subprocesses, not an in-process OOM/rollback stress proof.

Local retained SDK23.1.2 / NO LLVM is PARTIAL. Remote ordinary CI keeps pinned
23.1.3 unchanged. Actual merged-main **push** FULL39347001923 (5×354/354) and
PARTIAL39347001925 (353/353), attempt1 SUCCESS, were separately verified through
Actions REST plus each actual job log; see main-push-ci/totals JSON. These are
not PR637/R644 candidate runs. Candidate evidence uses at most one natural
FULL5+PARTIAL pair; final outcomes are recorded once in Issue647, no HEAD repair.
Passing remote CTest output is normally suppressed, so local JSONL retains the
concrete observations; do not invent separately archived remote guest C17.

## Capability map / one proposed follow-up

Selected `usize <`, ordinary Copy IF, arm-local Copy, old-value return, two
lexical Pair roots, two scoped field Changes and sibling/currentness metadata
already compose through CHECK and owned generation here. Their **whole-body
backend frame consumer** does not compose them through ordinary C17/native.
Old PR637 / calibrated R644 one-root/one-closed-loan finite native success is
preserved and is not arbitrary-composition evidence.

Recommend exactly one next bounded P preflight/implementation candidate:
authenticate the two-original-record / two-closed-write frame of the frozen
**one_if_control** using existing root/field/ValueFact/scope evidence in a
reusable frame walk, without a new issuer or singleton-guard bypass. The second
sequential IF is deferred. Estimate an initial20–30min frame audit, then only
if local reuse is sound a separately authorized90–150min implementation/test
budget. This is an unmeasured estimate, not an assertion of a one-line fix.
Stop for M/C if general CFG, new authority/proof semantics or broad rewrites
are required. No implementation is attempted in this evidence task.

Compass→Strategy§§1.1/1.3, Process§§4.2/4.3/4.3.1/7 and current Roadmap reviewed.
KEEP selected semantics; Historical source-rule change gate N/A (no new rule).
This is a compiler precision boundary (D), not a demonstrated language semantic
contradiction or host memory fault(S). The hand-written program is synthetic
user-style evidence, not an independently R-validated entire program, real
customer validation or measured author+ordinary-safe-caller audit burden(V).
No new goal-deviation finding; no C/Rust value superiority claimed. Product GO
active scoring PAUSED, H1 HOLD. Evidence DRAFT / DO NOT MERGE, C owns next action.
