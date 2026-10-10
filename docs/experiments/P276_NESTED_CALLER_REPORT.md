# P #276 nested original values and direct caller experiment

Track: P

**EXPERIMENTAL / DO NOT MERGE.** Source/checker evidence only. Draft17.30 remains
canonical; Draft17.31 is unselected. There is no native cJSON, detach/adopt,
F0/F1 refinement, actor independence or North Star PASS claim.

## Authority and bounded scope

- Compiler/main: `b75baea96a644e68634baee383299b66981b3c62`, CURRENT_SPEC Draft17.30.
- FormalProof/main: `08c8b8da4b9dbe5e125e4be0643bffb28294bfad`, research HOLD.
- Draft candidate #273: `e6b4e98d4f19ed03bdba9a63ff0e86d6b40dc0a2`, DRAFT / UNMERGED.
- Fixed predecessor #275: `059b85497862d1461688ec5c6212e5ee87adf94c`, DRAFT / DO NOT MERGE.
- Current branch is a fresh child of that exact predecessor. #275 is not updated.
  #276 and Coordination #268 comment 6094203206 authorize this experiment only.
- Two independent original B/dst triads in one two-member ordinary nested record,
  one pure direct whole-result call per allocation arm, optional independently
  inferred single-record terminal. Same five genuinely fallible original sites;
  no sixth H, allocation, runtime registry, Matched bit or external grant.

`NEWLANG_EXPERIMENTAL_NESTED_CALLER=ON` is a second opt-in, default OFF, requiring
`NEWLANG_EXPERIMENTAL_ORIGINAL_GRANT=ON`. The old/default and #275-only routes
keep their previous source fences. This is an implementation experiment, not a
selection of proposed §3.2c's entire source example.

## Actual source and independent inference

[Fixture](../../tests/fixtures/experimental_nested_caller.nl) declares ordinary
`LiveRoot {p:ptr<Node>,a:Allocation,d:LifetimeDomain}` and
`TreeTwo {root:LiveRoot,child:LiveRoot}`. Names do not establish authority:
renamed nominals/functions/locals, reordered fields and different ordinary
record field labels have independent positive and negative source controls.

```newlang
fn assemble(first:LiveRoot, second:LiveRoot)->TreeTwo {
    let combined = TreeTwo { root:first, child:second };
    return combined;
}
fn finish_root(ticket:LiveRoot)->unit {
    let LiveRoot {p,a,d} = ticket;
    let empty = loan_exclusive_read(d) { |ending| destroy(p,ending) };
    let full = erase_slot<Node>(empty);
    finalize_domain(d);
    deallocate(a,full);
    unit
}
```

Definition checking for ordinary packaging recursively supplies **type-only**
formal constituents. It checks complete construction, consuming use, return
and all nonDiscardable exits without allocating any R/O/D or assuming any
matching. Every actual call instead executes the owned source body with actual
caller values. Three owned snapshots retain pre-call/current donor custody,
returned value/formal consumption, and fresh caller result placement.
`NLWholeValueCallView` is a read-only custody view, not matched entitlement.
The public original closure validator independently checks source/body/world
anchors, actual consumed inputs, unchanged constituent identities, the complete
returned member paths, original affine inventory and fresh placement.

For the one-record unit terminal, the existing typed-owner symbolic engine is
extended to a complete field-labelled destructure. Its actual destroy / erase /
finalize / deallocate protocol must prove the same eight existing conditional
requirements, independently of whether any favorable call exists. A definition
that omits release or releases before domain finalization fails. At every actual
caller the existing `nl_owner_relations` checks original ptr/current full typed
root/incarnation/BackingRegion, governing D, full recoverable range, Allocation
backing and dependencies. No Unknown formal can satisfy those checks.
The owned closure validator replays the actual callee's four primitive steps
in the original five-root release machine. A closed final world alone is not a
release proof. Removing a callee deallocation invalidates that certificate.

The caller has a complete B destructure, existing local D loan for B.prev,
loan close, complete repack, actual two-packet call/return, nested whole
destructure, another D-scoped B.next write/repack and two named terminals.
The post-A/C/src variant reaches named B/dst terminal after release order
`[2,4,1,3,5]`; it is still not a library detach/adopt implementation.

## Public observed values

Every ID below is qualified by its own actual Some^5 world/snapshot. They are
not global addresses or host-supplied identities. The public read-only observer
uses source → parser → transactional unit registration → checked `main()` →
owned original closure validation. It uses no private header/probe/seed.

| Witness | dst member | B member |
|---|---:|---:|
| original ptr region | 5 | 3 |
| original heap root / incarnation | 57 / 60 | 31 / 34 |
| Allocation value / original region | 57 / 5 | 29 / 3 |
| Domain value / identity | 62 / 5 | 34 / 3 |
| current returned packet value | 112 | 110 |
| copied ptr constituent value | 111 | 109 |

Caller donor bindings 58/59 and callee formals 60/61 become consumed.
Callee local result binding 62 has incarnation 91; caller result binding 63
receives value 113 in fresh local incarnation 92. Both original heap
incarnations and exact A/D constituent value IDs are unchanged. All five
original A and D values have exactly one available current carrier through
checked parent/member edges, including owners outside the selected parameters.
No heap EndRoot occurs at move/return. Actual terminal releases end all originals
once, with failure worlds 0..5 validated.

## Decisive contrasted sources and classifications

[38-case JSON](P276_SOURCE_OBSERVATIONS.json) includes exact source SHA256,
CLI exit/category/diagnostic, repeat identity, no output artifacts, actual
public snapshot/validator evidence and source byte spans. Full sources are
reconstructed deterministically by
[nested_caller_test.py](../../tests/integration/nested_caller_test.py).

| Case | Measured result |
|---|---|
| Actual original dst+B result, later named terminals | semantic ACCEPT; CLI exit 4, `V1-BACKEND-UNSUPPORTED` |
| B ptr/D + actual C Allocation, same whole-result call | B.prev and ordinary return are possible; exit 3 semantic `P193-CALL-BACKING` at `finish_root(tail)` |
| Same mixed result, then redistribute original Allocations correctly | semantic ACCEPT / backend unsupported; ptr regions `[5,3]`, Allocation regions `[5,4]`, D `[5,3]`; all five originals discharged |
| Genuine B ptr/A, C D supplied only at terminal | exit 3 semantic `P193-CALL-DOMAIN` at `finish_root(wrong)` |
| Wrong D before B.prev, independently | semantic `ALLOCATED-DOMAIN-MISMATCH` at root-ref use |
| C ptr with B A/D at terminal | semantic `P193-CALL-DOMAIN`; wrong root cannot get B's D authority |
| Duplicate A/D, repeated actual argument, old caller packet/Allocation reuse, repeated result/terminal, duplicate callee formal/local | semantic `P3-USE-AFTER-CONSUME` |
| Partial nested destructure, forgotten second owner | semantic field-count / scope-obligation errors |
| Live local D or derived H loan across whole repack | semantic `P3-REF-CONFLICT` |
| Whole packet loan around call | `LOCAL-LOAN-PROFILE` **unsupported**, not semantic borrow rejection |
| Named `finish_two(whole)` calling `finish_root` twice | `P276-TERMINAL-SUMMARY-PRECISION` **unsupported**; no invented original formal grant |
| Third nested declaration / second result call in arm | explicit finite profile unsupported |
| Terminal using explicit `return unit` | owned trace precision unsupported; normal unit fallthrough is the measured profile |
| Partial root declaration | parser `AVS-DECL-PROFILE` unsupported; no ownership verdict |

Totals: **11 semantic accepted/backend unsupported, 21 genuine semantic errors,
3 semantic profile unsupported, 2 precision unsupported, 1 parser unsupported**.
All actual CLI results are deterministic on repeated invocation, stdout empty,
and publish no native/Checked-C artifact. All public registration rejections
leave the caller snapshot unchanged and produce no owned artifact.

The B→C Allocation counterexample differs by one original record initializer;
C has not been deallocated before the rejected named B call. Its source span
is bytes `[13560,13577)`, line 219 columns 45–62, exactly `finish_root(tail)`.
The corrected source's named call passes; the mixed-repaired control proves
ordinary mismatched custody and actual B field access remain legal.

## Product/core separation and targeted M finding

The observer field `h1_dst_B_result` tests only the necessary original-member
result predicate, not detach/adopt graph policy or the full North Star score.
The swapped-result source returns B+dst instead of dst+B, then correctly
consumes both. It is core-safe and lacks the particular ordered H1 result.
The H0 source initializes `dst.child=Some(ptr_B)`, keeps B's complete current
packet outside the call and returns dst+C. The public observer verifies that
Copy pointer's original B root, B's separately unique current A/D carrier,
and all five matched terminals. It accepts semantics but **does not** establish
H1's result-contained original dst+B entitlement. All mixed and H0 predicates
are read-only observations; they never feed facts into the compiler.

Candidate §3.2c.2 already distinguishes ordinary mixed construction. In
§3.2c.3 the phrases "At every actual known-direct call ... AllocationBacking(a)==R"
and "For an owner-bearing complete result, require every original grant to
remain live/matched" need a precise distinction between **ordinary complete
custody**, **body-inferred owner-using preconditions**, and **the particular H1
postcondition**. The pure assembler never uses Allocation; the actual safe mixed
return cannot infer matchedness from this body/name. This is a targeted
ambiguity for M/Coordination, not a canonical contradiction or unsafe checker
bypass. No Draft/ledger is edited or presumed adopted.

## Regression and smallest remaining blockers

[Fixed controls](P276_FIXED_CONTROLS.json): the same correct, wrong-A and repaired
mixed sources are actually run on fixed main, #275 default and #275 opt-in.
All nine controls stop at parser `AVS-DECL-PROFILE`; none is credited as an
ownership rejection. #275's complete 22-case regression runs unchanged except
that its previously unsupported *unused* nested declaration is now explicitly
accepted under the separate #276 flag; all genuine safety errors remain errors.

Local and remote validation cover GCC Debug/Release, Clang, ASan/LSan and UBSan,
each in default, #275-only and #276 opt-in configurations. Exact local results
are recorded in the Issue report; remote Actions is tied to this PR's exact
head and its temporary merge commit in that report. The new opt-in suite has
254 tests, including the 38 actual source contrasts, **28 actual owned-call
poison attacks**, and allocation-failure sweeps through registration, actual
checking, call snapshots and public validator; failures roll back/no artifact.
C formatting and diff checks pass. No C layout/emitter changes are included.

Smallest next source blocker: independently propagate two member-path
conditional original-root requirements through a nested `finish_two` wrapper
before substituting an actual caller, then retain an owned transitive call trace.
The type-only formal currently cannot prove these conditions and correctly
stops at precision, rather than treating Unknown as matched. The existing
local D-destructure/borrow/repack path works; whole ordinary record loans and
an arbitrary list of result calls/return exits are separate unsupported shapes.
Full four-detach/two-adopt Change/Reset source and native backend remain absent.
No product scoring, actor/thread independence or F source/refinement follows.

## Historical design gate and reproduction

Process §4.2 / Design Decision Procedure were checked against canonical
§§13.5,14.5,16.1–2,18.1a–c,18.6–8,26–27; DI-011/012/013/014 and candidate
DI-015/#272/#273; predecessor #274 evidence and independent #275/#268 review.
**KEEP** explicit original A/Storage/slot/D laws, independent source-defined
consumer applicability, nonCopy/whole-value scope/current-incarnation semantics,
old compiler-known LiveTail gates and no name-based product policy.
**DEFER** normative ordinary-record source expansion, universal matched-result
assertion, transitive two-root inference, owner graphs, generic contracts,
field loans, visibility/modules, allocator/layout/native, RAII and concurrency.
No prior adopted rule is intentionally replaced. Historical source sketches
and M0 omitted the required Allocation/Domain/destroy evidence; they are not
positive source proofs. Full old M9 conversation is not exhaustively audited.
Ledger update is not appropriate before independent normative selection.

```sh
. .deps/activate.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DNEWLANG_EXPERIMENTAL_ORIGINAL_GRANT=ON \
  -DNEWLANG_EXPERIMENTAL_NESTED_CALLER=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python3 tests/integration/nested_caller_test.py build/newlangc \
  build/nested_caller_evidence build/captured_closure_test /tmp/p276-evidence
```

**STOP for independent Coordination review; Issue #276 stays OPEN.** Compiler
main/CURRENT_SPEC/Draft candidate/predecessor heads/FormalProof are unchanged.

`P CJSON-B-STATE-1 TWO-ROOT CALLER RESULT: EMPIRICAL CANDIDATE — SOURCE-CURRENT WHOLE RESULT AND ORIGINAL-GRANT TESTED, DRAFT UNSELECTED`
