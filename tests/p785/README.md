# P785 — Stage2B3a actual completed allocation-arm Copy proof

P Work/Codex, new branch from main77d83dfc00850b14fd7c3bc95e7e3c12e34161fb,
Draft17.34. Gate0 comment6364289220 precedes code; freezeee0f6f2 records sources,
oracles, expected first failure and K=5. R776 FIRST-PASS BLIND COVERAGE FAIL and
R780 calibrated A-only acceptance are historical facts, not replaced by this P
NON-BLIND implementation. Original external sources/oracle remain byte-identical
in tests/p778/external_r776. No backend, public API, grammar or normative edits.

## Actual first predicate / refined publisher

Baseline R-B CHECK0, public Copy body+parameters3, ordinary/named C174, zero C
and named sentinel preserved. A (the second allocation Some arm) owns body B at
call74; B's proof is sealed and B.copy_body.caller==A, but caller_nodes is NULL.
The existing function-final seal visits direct bodies, not separately owned arms.

Important refinement of M/Gate0's general function_match line references:
function_match dispatches try_allocate_one to allocated_match. The real arm root
and save_arm are in allocated_match. An initial normal-function-match-only pilot
left the SAME missing caller_nodes predicate; its result is preserved and Issue
supplement6364337059 records the correction. No next replay failure was hidden.
Only allocated_match's actual completion point is changed, no general recursive
seal or normal match/IF/loop expansion. An earlier mistaken text insertion at a
repeated source_if exit-union site failed compilation before source execution;
its raw compiler errors and static-review correction are also retained.

## Implementation / transaction / lifetime

copy_body.c extracts the old direct-child snapshot loop into the private helper
nl_copy_body_seal_calls. Each eligible sealed formal-write proof must already
have child->caller==the actual completed fragment. All direct-child node copies
are staged first, then caller_nodes/origin/root/count/source/body are committed
together. On any allocation/identity error, all temporary copies are freed and
no attachment changes. The existing body-count64 and graph-entry bounds remain.
The old function-final seal reuses this helper; its proof kind and public
validator code are unchanged. No source/world/entry/handoff/field fact is minted.

allocated_match calls it after the real MATCH_ARM root and checked closed-world
postcondition, before closure/parent arm publication. The original syntax-derived
match/variant/index, clone context and source ownership are retained. Nested
allocation arms complete through their own existing issuer lifecycle, not an
unqualified recursive walk importing ancestor graphs. Failure uses the existing
private-arm destructor and transaction rollback. Copies live in their existing
child proof until its destructor; borrowed node origin/source/body/world have
the same enclosing owned artifact lifetime as before.

The public proof authenticates the actual caller arm graph, not a new public
certificate for arbitrary containing-parent rewrites. Tests reject wrong parent,
sibling/index handles, alien same-number views and changed arm root variant.
No claim is made that passing the unchanged genuine A to its existing validator
certifies an unrelated host-rewired enclosing program; that larger parent receipt
is outside this schema and deferred. No test-only issuer can create positive rights.

## Source / public observations (not R-B native)

Unchanged R-B call74 now validates body0+parameters0. Actual arm root/count136,
original T root37/child39/index1, old184 -> new27, sibling7, loan72/scope5,
parentFact41/childFact43. H producer/local RETURN and terminal proofs remain
valid on original tail root16/inc19/backing2/domain2. H domains/backings close
in each semantic 0/1/2 allocation world; this is not a new physical B observation.

P-authored pressure_arm.nl was frozen before implementation testing, SHA
3cfd5ca0027f14c8ee2c1c064d679bcb8ddd27dae1339165347ca0ad4c50f82f.
It varies the author's prior source and puts two named T calls after owner RETURN
inside the H arm: index1 old237 ->97/sibling57; another original index0 old14 ->237/
sibling203, H payload105/249. Both actual issued calls74/81 validate0, full event
replay and caller/formal substitution unchanged. Two source tests verify same
incarnation, changed parent/child facts and unchanged sibling fact/value.

Both B and the new P source STILL have CLI4/zero C and named target preserved;
public Node emitter leaves output slots intact. B native NOT TESTED. R-A public0,
normal/named C17 and its old GCC/Clang native suite remain regression evidence.
Only the living P778 harness's B public3 expectation advances to0; old script
archived, frozen source/oracle/original first failure and CLI4 expectation unchanged.

## Reproduction / negative / OOM evidence

```
cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j2
ctest --test-dir build -R '^p785_' -V
```

K=5: p785_r_b_public.unit, p785_pressure_arm_public.unit,
p785_arm_integrity.unit, p785_issuer_oom.unit, p785_cli_boundaries.integration.
Baseline FULL5443/PARTIAL442 plus5 =>448/447; no removals. Exactly one natural
attempt1 FULL5+independent PARTIAL on stable candidate; no substantive CI rerun.

Integrity rejects30 graph/source/root/count/order/loan/field/ValueFact/Copy/formal
mutations with restoration to valid proof. Also rejects alien same-number views,
wrong parent/sibling/index handles, foreign entry/handoff and new-current/sibling
Copy values substituted for the true old result. Original R negative source
diagnostics are recorded, not relabeled: read-to-write first hits unsupported
LOCAL-LOAN-PROFILE; wrong backing, stale link, unknown field and consume reuse
hit their actual semantic errors. Supplemental P wrong-Domain/duplicate-owner
sources have a separate pre-execution freeze and leave the initial source intact.

The OOM target alone wraps the real private seal call and malloc. It NEVER calls
the issuer on host-generated evidence. The actual registered-source main check
reaches all newly used caller-snapshot allocations: R-B one position, P two.
Both first and second P snapshot failures are exercised; second fails AFTER one
temporary copy exists. Real arm and original registered public states unchanged,
all attachments unpublished, returned artifact NULL, status OUT_OF_MEMORY, clean
same-source retry and public validators0. This does not cover every compiler
allocation. ASan/LSan checks cleanup; no partial C or B native claimed from OOM.

Initial full frontend447/447 PASS (126.41s). Focused ASan/LSan17/17 and UBSan17/17
PASS, including R-A and p713/p718/p728 proof regressions. Additional explicit
current/sibling-as-old Copy checks subsequently pass regular+ASan+UBSan integrity
runs; Production code and inventory unchanged. All raw logs are retained.
Local Clang23.1.2 differs from unchanged Actions23.1.3 pin.

Initial harness-only build/configuration mistakes (probe field spelling, const
annotations, calloc wrapper symbol, destroyed context out-slot not reset) and
the Not-Run/first harness failure logs are retained, not called proof/OOM passes.
They were corrected before natural Actions; no compiler guard or oracle weakened.

Local LP64 node view1728bytes: new R-B snapshot235008bytes, new P two-call total
487296bytes; pending pointer stack512bytes. Per-call full-graph storage is the
old chosen representation and adds real memory cost. Sharing/deduplication or a
different parent receipt requires separate design, not silently added here.

## Historical / Goal Alignment / stop

Process4.2 N/A new selection: faithful completion of existing issuer-owned
graph snapshot under adopted16.3.8/DI-018 and H/T/affine/scope rules. KEEP source,
authority, validators and original ownership. DEFER new receipt schemas, generic
graph/CFG/effect/owner/MIR and Stage2B3b. Historical semantic_internal.h hash stays
identical; no historical-check exceptions. C chooses fresh independent R/adoption.

VALUE-ALIGNMENT-RISK: true public proof precision is compiler capability; author+
ordinary-safe caller audit/re-audit closure improvement versus well-factored C is
unmeasured. Memory/bookkeeping costs remain. Product GO624 PAUSED/H1268 HOLD.
No main merge, Issue close, R launch or B native implementation by P.
