# P866 — selected equality Copy prefix candidate, STOP at local OOM harness

Base: fixed P822 b3a07d89871ba3bd8441821ba4656d7c73a2fd22, Draft17.34.
New P-only branch p866-copy-equality-prefix; no newer-main rebase. All source
bytes/oracles and the intended footprint were frozen in commit a902e68 before
production edits. No R source/oracle is a P fixture.

Production changes only unit_prefix_walk in src/copy_body.c: equality is a bool
comparison, reuses the exact two source-owned usize operands and recursive
literal/identifier checks, and recomputes == instead of the existing < predicate.
No issuer, schema, dedicated/generic IF validator, backend, loan/owner or caller
postcall authority change. LESS_THAN retains its existing operation.

Six P positives: two declared field indices and normal-arm ordinals, true/false
equality, literal and bound-identifier operands, plus two < controls. Before code:
all CHECK0/dedicated IF0, equality unit/parameters3, less unit/parameters0. After
the repair: all six unit/parameters/dedicated/event0; old Copy, incoming, sibling,
root/child incarnation and conservative unknown caller postcall checks pass.
Existing unchanged emitter produces C17 on all six; native is NOT TESTED yet.

Four preregistered CTests, expected FULL5 457/PARTIAL456 if C later releases CI:
- p866_equality_public.unit: PASS on first local execution, six sources.
- p866_prefix_integrity.unit: PASS, 16 mutations per case plus foreign views,
  wrong root/formal/old Copy. Test-private inclusion of copy_body.c calls the
  actual static prefix on genuine source-issued owners so operation-specific
  negatives cannot pass merely because an outer graph seal rejected first.
- p866_reached_validator_oom.unit: first case reaches both real prefix calloc
  failures, did_fail=1, OOM, unchanged world/public retry0. SECOND CASE BASELINE
  FAILS at oom.c baseline CHECK because the static test-only hit flag remains
  true from the first case; it is not reset before the next baseline. No fix or
  rerun performed. This is a P test harness setup defect, not evidence of a
  production memory/authority failure. Second-case faults NOT TESTED.
- p866_source_cli.integration: NOT RUN after --stop-on-failure. The independent
  two semantic negatives were measured before implementation; native/passive
  observer/negative/named-output regression remains pending.

First complete local sequence: public PASS, integrity PASS, OOM harness FAIL;
CTest exit8. No candidate-wide frontend/sanitizer/native or GitHub Actions run,
no DRAFT PR created after the STOP, no merge. A DRAFT PR would start the natural
CI pair, so it is deferred rather than silently spending CI on the known failure.

The first raw focused log and source/check/public observations are retained at
/tmp/p866-evidence. The preservation commit message embeds lossless gzip/base64
of that log with SHA256 and the exact failure account. Sources/oracles and the
failing harness are left unchanged for Coordination review.

Next smallest remedy, only if C separately authorizes: reset hit=false before
each exercise baseline, rerun bounded local tests, then consider the one allowed
natural fixed-HEAD FULL5/PARTIAL pair. Do not expand proof/backend semantics or
infer success of unexecuted cases. The P822 and all older R evidence remain fixed.

Historical audit: N/A faithful Draft17.34 §§5.2/13.5c/17.4/27.3–27.4, DI017/018
implementation; KEEP source and conservative join, DEFER general CFG/effects,
owner/FFI and new public rights. Goal is reusable selected compiler proof
coverage, not a measured author/safe-caller UB audit-locality advantage over
factored C. Product GO PAUSED and H1 HOLD remain. STOP pending C.
