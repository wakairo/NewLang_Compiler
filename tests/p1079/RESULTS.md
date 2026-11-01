# P1079 bounded source capability: local result

Authority: C173 / Issue1079; actual main
`626829e22cc3649a99bcbce998dda4aa0cfab22f`, tree
`57ca1b9e9d4b8adb1b87fd130ed0a51e96fabf22`, CURRENT_SPEC → Draft17.34.
The real main push FULL41290053391/PARTIAL41290053425 was checked before editing.

The tested local cohort is HEAD `fc76bff14519b5ec322946bc0b8566faee96c056`,
tree `5fb6388a81ebac1d3374308b8eb1aca0d7ac70e6`. Production is the same as
`b923449` (one private header plus dispatcher; 194 header lines). Later results
and this document are outside the runtime freeze. Exact 2752 active input
bytes must remain identical at submission; no result is attributed to an
unexecuted changed implementation.

| Local gate | Actual result |
| --- | --- |
| Fresh full GCC / Clang, SDK23 LLVM smoke and existing T991 hook | 601/601 each; 383.35s / 375.03s |
| ASan + LSan, risk-selected frontend cohort | 151/151 of 600 eligible; 257.13s; detect_leaks=1 |
| UBSan, risk-selected cohort | first epoch 113 PASS + one preserved 90.01s TIMEOUT; only failed/unreached38 rerun serially, 38/38 in85.99s; union151 |
| Original P965 owner / ledger-OOM per lane | 494 / 502 actual completed cases in all four lanes |
| New true / false / child1-equality profiles | authentic CHECK, Copy and IF/event current; strict C17 GCC/Clang O0/O2; physical guest0/1/2 in every lane |
| Actual new native observations | 72; original H/T addresses, fields/siblings, selected old Copy and exact failure cleanup |
| Reached backend allocation calls per profile/lane | 631; 611 OOM +20 conservative UNSUPPORTED, all unpublished/context+owner preserved/exact retry |
| New IF negatives per profile | parent5 +arm14 +candidate-view4 +cardinality2; foreign B entire IF arms survive A's unrestored destruction |
| Existing Copy/H/world negative adapter | 14 /4 /7, original B body/H-arm after A destruction; original test unchanged |
| Named output | success bytes, rejected source/preflight, I/O-error inode/bytes/no-temp, exact retry |
| Historical/current freeze and formatting | clean detached Git checkout, all2752 Git-managed inputs, original v1–v15/P1077/P1071 bytes; full pinned formatter PASS |

The UBSan TIMEOUT was `p990_new_native.integration` during simultaneous local
GCC/Clang/ASan/UBSan workload. Its printed native observations passed before
TIMEOUT; no UB or value mismatch was reported. After other suites finished,
exactly the failed/unreached38 tests used the same candidate, source, oracle,
binary and timeout90 at `-j1`. The timed-out test passed in17.78s. The first
TIMEOUT remains RED in the archive; successful tests were not repeated.

The new C outputs are15257bytes each and agree across all four local builds:

- true: `ec3f8af9bd5105aa757f3e0ee51a9988435d9deb6664c15be52fe5a6799b9808`
- false: `b87e8bcaa72c2ad33a18a6dda5f73306a20d6783b0efe0c90793577e1b0dd440`
- child1/equality: `c796d8c0b0edd8f18bfa29b5ec4e641a01d15db27c7e9638f310282621e73422`

The plain split controls remain byte-identical to the parent (14915/14916bytes).
Two original helpers return47/83; the selected IF old211 then updates the other
original, yielding true `[155,211];[47,47]` or false `[155,211];[83,47]`.
The complementary child1 profile returns129/211/83/47 and ends
`[83,155];[129,129]`. The IF result and joined child are statically Unknown;
this is retained, rather than replaced by a chosen-arm numeric value.

Initial source-only commit `a1acc894bfd99ddca8b60e12add3188f295f55e0` preceded
execution. Its oracle predicted CHECK success/backend refusal but observed
`P13-IF-OPEN`: original condition parentheses were missing. That first source,
oracle and raw remain. v2 retains outer loans and observes CHECK0/GET1/current3.
v3 arm-local loans observe CHECK0/current0, then parent C17 exit4 at the old
literal-only IF consumer. v4 adds child1. Private include-order and entry-vs-ended
availability failures, two harness assumptions and the UBSan TIMEOUT also remain.
No original failure was retroactively counted as PASS.

`evidence/local-results-v1.json` records the actual commands/results/cohort.
`evidence/local-raw-v1.zip` contains861 members,1395931bytes, SHA256
`a10825acd109b5bf6d7dcc2d264cc1916b386f778c7ae0db7222f890142c5d92`;
all member SHA256 and CRC were checked. Parent/FIRST archives are separate.
Canonical prospective v16 protects2752 inputs, SHA256
`28abd3405854ae26cd7de5ff841f00e1df080cf8f3b363d26d36befed66c0bf5`,
with exact previous v15 SHA256
`ff9e1502d995892e689c8011b04b315a334de422c24503ab68aa438fe9199e63`.
Timeout90, p990 RUN_SERIAL, workflows, old fixture/source/oracle and independent
P/R evidence are unchanged. No independent R inputs or stopped PR1050 were used.

Self-review: PASS for this finite private consumer, DRAFT HOLD for Coordination.
Only current-validated, source-owned arms enter the path; arm worlds are never
substituted into caller Env. Entry authenticates availability; the completed
caller confirms immutable binding identity after lexical retirement. A real
field read/write supplies the result; only common poststate updates Local.
Both arms are checked before output; the existing staged publication and
canonical destruction remain in force. This does not approve new issuer rights.

Unsupported legal boundaries remain: an enclosing loan around the IF (GET1 /
current3 / C17 exit4), loop/break (first CHECK3 P6-JOIN-PRECISION), different
changed IF roots (CHECK precision), arbitrary predicates and general CFG,
multiple arm Changes/calls/nested scopes/RETURN and third helpers. Some of these
are static exclusions rather than newly executed failures. The next useful
increment is authenticated control/exit correspondence or a bounded extension
using existing receipts, with source-first evidence before any issuer change.

Maintenance: 3 real CTests; +173 active history inputs over2579; public validation
adds reached allocation/work cost (631 backend positions in the new programs).
One bounded consumer reuses existing receipts, physical ledger and output staging;
no new heap/evidence framework. Later raw/result archives are not runtime inputs.
CI at this document's creation is pending, budget unused; see Issue1079 and the
PR for exact final-head natural run/job IDs and remaining limitations.

VALUE-ALIGNMENT-RISK: reusable conditional mutation/value flow is progress toward
selected v0, not full soundness or Product PASS. General control and author plus
ordinary-safe-caller UB-prevention audit/re-audit dependency closure versus
well-factored C remain unmeasured. The cost of narrow proof consumers, duplicate
historical fixtures and growing provenance inputs needs continued review; it
must not become a substitute for necessary v0 mechanisms. Product GO PAUSED,
H1 HOLD. No READY, merge, Product GO or new F/R/Q run is authorized here.
