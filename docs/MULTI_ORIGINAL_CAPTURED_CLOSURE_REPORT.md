# Draft 17.30 multi-original captured-closure report

Track: P — Compiler #244. Candidate review evidence; no merge authorization.
本報告はcertificate substrateの実装証拠です。#237の完全なsource admissionを
達成した報告ではありません。

## Authority and scope

Startup Compiler/main: `0dcb5c837d70a392503a6163ae0a1f67f515ebc7`.
`CURRENT_SPEC.md` points to Draft 17.30. FormalProof/main:
`608d99505737e011006f902768478430f32c83b8`; F #41 / PR #42 independently
accepted, merged and closed. P #237 remains HOLD/open; M #243 research is closed.
Process §§3–6, Design Decision Procedure, DI-009–014 and Compiler Testing
Strategy were reviewed. Historical audit is N/A: no normative/design decision
was introduced. Formal finite conservation evidence was not imported as fixed
production IDs or claimed as implementation correctness.

The implementation and proof boundaries are detailed in
[the certificate contract](MULTI_ORIGINAL_CAPTURED_CLOSURE_CONTRACT.md).
This result addresses the certificate substrate only. It does not resume #237.

## Implementation

- `captured_closure.h` exposes borrowed immutable tuples and a read-only
  revalidator; `captured_closure.c` owns 0–4 ancestor originals, derives the
  common post before branches and validates both independent real arm states
  and explicit original release traces.
- `allocated_join.c` adapts the legacy one-original target builder to the same
  engine. Existing scalar checked fields and two-Node native consumers remain.
- `semantic_check.c` seals certificates before committing ancestor-derived
  state and provides an isolated real-source checker probe. Ordinary source
  admission gates remain unchanged. `checked.c` destroys the owned certificate
  with the checked artifact, including partially constructed objects.
- Tests use actual source artifacts, then destroy original source/AST storage
  before validating or attacking the certificate. No positive fabricated
  certificate, seeded owner, source cleanup substitution or native probe exists.

## Source observations

Two-site, minimum three-site and five-site ownership-only probes are accepted
by the isolated checker. Five-site terminal worlds independently prove exactly
0, 1, 2, 3, 4 and 5 original releases, one world each. Some-first permutations
at the outer and third match, plus alpha-renaming, retain those results.
The caller context imports no hypothetical branch-local region/domain suffix.

Fixture SHA-256:

- Canonical full fixture, unchanged:
  `812042833072145f761ddc21cbf774d81b64da3a34f6d466ba7db01a6d704d7d`.
- New reduced ownership-only five-site probe:
  `e1f3eb9b9823d7c1d6c6544e2bdb193faa979d8554a727a70edefe39df610687`.

True public CLI observations remain: canonical full fixture exit 3
`AVS-DECL-PROFILE`; reduced three/five-site exit 3
`ALLOCATED-CARDINALITY-PROFILE`; existing two-site semantic acceptance followed
by exit 4 `V1-BACKEND-UNSUPPORTED`. None emits C. Machine-readable input-based
observations are retained under `docs/evidence/`.

Actual-source negatives produce semantic refusal for cross-Allocation/full
Storage, cross-Domain EndRoot, duplicate consumption, missing cleanup in each
captured None world, active domain loan and scoped-ref escape. The sixth-site
control is classified separately as early profile rejection. The scope-escape
control retains an existing diagnostic-quality limitation: semantic-error
status with legacy `P3-INTERNAL` code. This is reported, not counted as a host
failure or canonical defect.

## Poison, OOM and rollback

Post-teardown attacks cover missing/duplicate/reordered originals, cross-owner
and domain, wrong full extent/incarnation, foreign worlds with coincident IDs,
chosen-arm post, live root/loan, forged availability, Unknown and conditional
dependencies, deleted explicit cleanup despite closed final state, duplicate
EndRoot, copied nonCopy operand, loan escape and a forged Some grant in None.
Restoring each mutation restores successful read-only revalidation.

Fault injection exhaustively fails all 137 constructor and 268 public-validator
allocation positions, followed by successful retries; snapshots remain equal
and failure publishes no output. Full five-site checker construction samples
184 deterministic distributed failures among 7,254 allocation opportunities
(first/last 64 plus every 128th). Each failure is OOM/no artifact, followed by a
clean successful retry. This full-checker sampling is explicitly **not** an
exhaustive claim. Existing exhaustive two-site rollback regressions remain.

## Validation and disposition

Seven new CTests extend the previous 234 tests to 241. Local validation passed
241/241 in each of GCC Debug, GCC Release/NDEBUG, Clang, ASan and UBSan;
tracked C/H formatting and `git diff --check` also passed. Fixed-head PR CI
requires the same five configurations, including oracle.adapter, oracle.smoke,
artifacts.integrity and existing two-Node native regressions. Exact head and
CI run are recorded in the PR and the single final #244 report, avoiding a
self-referential commit SHA in this file.

Findings: `COMPILER-IMPLEMENTATION-LIMIT` / `COMPILER-PRECISION` remain for
ordinary five-root/three-field source admission and richer certificate traces;
`COMPILER-DIAGNOSTIC` records the legacy scope-escape code. No normative
counterexample or spec change. #237 is **NOT ADMITTED / NOT READY** here.
Candidate remains OPEN/unmerged for independent Coordination review; no
native/LLVM, detach/adoption, cJSON, subsequent Issue or Track is started.
