Track P / C56 P934 **read-only Gate A: GO for one bounded backend consumer of pure unit IF, not a new proof issuer.** No branch, repo modification, PR or Actions at this report; CI0.

Fixed main `fbbf8a0261669ce0e8920e5b019134268a6a58c1`, tree `dea21a61ecc898eca2f7df3632ab5dfafc391a3f`, CURRENT_SPEC→Draft17.34. Remote main identical. C56's postmerge push runs40274245248/40274245276 also report exact head/attempt1/SUCCESS by API; C independently verified their six logs. Gate A uses a newly built unchanged GCC13 Debug frontend compiler (BUILD_TESTING OFF), SHA256 `8e15dbcfeee0bb7c366245db5c2e4d56a5265739c59c0c00637d21c946b9666b`.

### First actual refusal, not a two-root constructor/ABI failure

The four immutable source hashes remain:

| Source | SHA256 |
|---|---|
| budget_true | febac1afa9d58bb166740df1c47d640d2baeda8acae5f89a4b5b12dd691d3687 |
| budget_false | b4738981b36bf450a77cb1f593b2d8c8f8fd6f0c8703a6d6e8e66cc390a2deb1 |
| order_only | 62b1113c821fd6f7bc629ca6db8f4146a263d92c1b3a1ba1a933bf157f8854b8 |
| root_values_swapped | 687519610b6fb7a800c0557152774ca76c15150b651d14a7967fb25e206c1aee |

All four ordinary CHECK0; stdout and named C17 EXIT4 `V1-BACKEND-UNSUPPORTED`, Cbytes0, named sentinel intact, zero temporary leaks. Missing-field and wrong-condition controls remain semantic EXIT3. Full raw streams and SHA are saved in `/tmp/p934-gate-a/ordinary-cli-first.json`, separate from older histories.

Read-only debugger tracing of each actual CLI backend path gives the SAME first failing chain, with no debugger errors, writes or inferior calls:

1. Real registered main() call / body-owner identity passes the existing call gate. BOTH original BINDING→AGGREGATE paths pass `avs_shape`, `avs_construction` and `v0_validate_node` (return1). Thus neither nominal count nor two physical constructor carriers is the first barrier.
2. IF19 has `normal_arms=2`, exact unit type, result_count0, !terminates. `v0_validate_node`'s IF case (main.c:2326+) first tries `nl_checked_if_return_view` (false), then `if_field_selection` (main.c:2254). Its **first conditional `!nl_checked_if_field_view(f,id,&v)` is true**, so it returns false BEFORE any public field validator or carrier mapping.
3. The field receipt exists privately but has ready/complete/sealed=false, root0, zero events/changes in BOTH arms. `nl_if_field_plan` intentionally needs a field Change to select a root; without one it returns OK/not ready, finish/seal do not invent a field proof. `nl_checked_if_field_view` therefore correctly returns false at if_field.c:348. This is NOT a failed public validator or invalid actual ownership proof.
4. Refusal propagates IF→statement→block→registered call; Array4 fallback returns `NL_CHECK_SEMANTIC_UNSUPPORTED`(2), Node fallback `NL_NODE_C_UNSUPPORTED`(1); compile_v0_to reports EXIT4. No emitter runs and no C is produced.

### Genuine checked state and minimal safe composition

Actual owner-held arms are each precisely three checked nodes: BLOCK, UNIT, IF_ARM, with zero block items, unit tail, no result/return/loan/replace/call/nested control. Parent's real owned arm links have correct `match`, ordering and context_origin; child.if_parent/node/index and source agree with the parent. Each arm retains the SAME inherited prefix from that source IF fork (values9, facts6 here), not numeric equality between arbitrary worlds.

For budget_true, each arm's original binding1→PLACE1→package VALUE3 and binding2→PLACE4→package VALUE6 retains its own original incarnation and two dependent children, live typed u8 scalars [186,9]/[72,224], package/member carrier links and source declaration indices. Domain/scope/backing counts0 mean implicit lexical objects, **not minted rich authority**. Original root/type/incarnation identities and source initializer permutations were also inspected for false/order/root-swap cases. Other world pointers stay separately qualified. No separate host-seeded fragment is used.

Classification **(i) backend finite-profile composition gap**: unconditional use of effect-IF proof selection as the generic unit-IF path. There is no effect to justify in these two empty arms. A separate backend-only predicate can authenticate real owned-arm lineage, exact unit/normal/empty graph and identical inherited frames, check source-issued constructors against each original package/field and existing scalar/condition checks, then reuse current emission. It must NOT bypass an effect proof for a nonempty arm or treat an unsealed field view as a certificate.

Planned source-dependent lowering remains existing `nl_type_<actual nominal TypeId> { uint8_t f0; uint8_t f1; }` and two separate `nl_local_<actual binding symbol>` automatic C objects. Designated initializers preserve declaration index and source initializer order. Original arm prefix relates constructor result ValueIds to binding PlaceIds/packages/children; emitted names are not guessed numeric IDs or source field names. C storage lives in the original function/block and is never borrowed after teardown. Existing CHAR_BIT/exact-u8 and native SIZE_MAX guards remain unchanged; no new cross-target/foreign ABI is selected.

Existing `emit_if_field` can already emit the authenticated two empty unit branches and actual usize operand comparison; only its validation precondition/comment needs a pure-case alternative. No result constant or known checker truth may substitute for native condition evaluation. No issuer/API/semantic/Domain/Allocation/P927 change is needed. New pure-case helper must refuse nonempty/terminal/foreign-world/active-authority cases and leave existing field/return validators unchanged.

### Conditional Gate B plan, witness boundary and budget

Freeze TWO new independent P-owned sources/oracles before the changed backend: different nominal/field labels and declaration/initializer permutations, edge u8 values0/255, equality TRUE versus less-than FALSE, two genuinely separate lexical roots each. Preserve all existing source hashes and historical first oracle/result files. Source controls include original malformed input and unsupported stateful/authority profiles, separately categorized semantic vs precision/backend rejection.

Three implementation/test paths planned: `src/main.c` (roughly100–140 lines bounded pure consumer), `tests/p930/cli.py` (active regression migration for the now-supported output), new `tests/p934/native.py` (roughly200–250 lines CLI/native/failure regression). **No CMake change needed:** reuse the existing p930 CLI test registration, discover its real configured compiler/sanitizer from the existing build cache and reuse the existing p694 fault binary. New source/oracle/log data paths only. No new private proof or snapshot engine. STOP before a substantial expansion beyond this estimate.

**Required regression migration is prospective, not oracle retuning:** preserve P929/P930 frozen initial EXIT4/C55 histories and data byte-for-byte; active CLI test must test the newly implemented EXIT0 and actual named C bytes for the same positives, with a separately pre-frozen P934 current expectation. Negative controls and failure/sentinel contracts remain. It would be false-green to keep testing EXIT4 using an old hidden compiler or disable the old test. No historical initial result is regraded.

Native witness: strict GCC13 and pinned CI Clang23.1.3 O0/O2 compile/run the **unmodified generated C bytes**, stored with SHA. Bare unit return0 alone is not physical carrier evidence: optimizing compilers can erase unobserved objects. A separately labeled passive-observer copy will retain the SAME emitted original declarations/initializers and read addresses/volatile u8 fields immediately before/after the actual IF while their owning function remains live; branch marker changes only observer state. It compares independent source-oracle values, distinct originals, field addresses/offsets and unchanged values. It never initializes/mutates guest fields or fabricates permissions. Instrumented observation is not mislabeled as the bare O2 object's address evidence. This follows the existing scoped passive-witness approach without extending pointers past local lifetime.

Reuse p694's real flush/close/commit and allocator faults on the new ordinary named-output path; require hits, preserved sentinel/no temp and recovery, not a no-hit OOM claim. New pure validator allocates nothing. First source/carrier/native/OOM mismatch STOP, no fixture or expectation repair campaign. Local first full frontend/native/sanitizers before a DRAFT PR. At most two natural FULL5+PARTIAL pairs authorized; plan to consume ONLY ONE, no dispatch/retry/rebase/force-push/merge. Final report at the immutable candidate.

Alternatives: existing direct C17 validator+renderer is sufficient and lowest cost. Rewriting source into a different already-native family is not this workload; Array4/Node fallbacks legitimately have other contracts. A new public proof issuer/rich lifetime/history or private P927/reparser cannot fix this pure-code backend selector and is disallowed; MIR/LLVM DEFER.

Compass→Value Strategy1.1/1.3, Process4.1/4.2/4.3.1/4.3.2, Backend Execution Policy/Contract0.4 and DDP/DI006/007/011/017 read. Draft17.34 §27.4 already selects the exact unit condition/two normal arms; §4.2 N/A faithful backend implementation: KEEP semantics / REPLACE NONE / DEFER new source/authority/ABI/CFG. F72 B only and all A/C history/encoding/D gaps remain unchanged.

**VALUE-ALIGNMENT-RISK separately:** this can remove an actual ordinary CLI backend composition barrier, rather than add only prefix diagnostics. It still does not show a useful safe library API or lower author+ordinary-safe-caller UB audit closure than well-factored C. Passive test observability is not a new user output mode; native toy success is technical capability only. Product GO PAUSED/H1 HOLD, old independent R results unchanged. Conditional small Gate B only; no other Track started.
