# P517 — calibrated R516 coverage repair candidate

DRAFT / DO NOT MERGE, existing PR515 only. P replay does not replace independent R516's first-pass FINDING / then-native-INCONCLUSIVE. Coordination owns calibrated R second-pass and adoption.

## Authority, immutable sources and reproduction

Fixed main9e6389388a44950d17e19bb134cbfde37ac75f97 / canonical Draft17.30; original candidate53b6883664f5f585ddd095ab357bc80e1e4018af. Follow-up sources/oracles frozen in fff6238 before new execution/instrumentation. Original independent R freeze19e9ed06a57ee822a569b777c1e2d9a8d51c21a8 is read-only; r516_*.nl are byte-identical derived replicas, not new R oracles. SHA256 TRUE c921596f72e64116630676b98d6ce43c529420a4bec26c437d01981cc39cc864; FALSE0dfc413cfc63fa379b6b4c1d20a6beea1421188add4e05b9d1bc660fc0ca1664; negative d9f6c09fdb63ba50e5beaeb26b6ada585cfe67c28883a53c53c7504ce6c0c272. All R and P514 hashes retained in frozen.json, checked before/after implementation. No tests/p514 source/oracle or R branch changed.

```
cmake -S . -B build-fe -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-fe --parallel 4
ctest --test-dir build-fe --output-on-failure
python3 tests/p517/native_test.py build-fe/newlangc build-fe/p517_public_probe \
  build-fe/p517_gate_test build-fe/p514_gate_test gcc tests/p517 none
# Same script with pinned clang; CI passes address/undefined for guest C too.
```

To reproduce the OLD diagnostic gate without changing Production: tracer.py reads git show of the exact53b6883 main.c and writes a temporary instrumented copy. Compile it with repository include/src include paths and unchanged semantic libraries (only main.c changed in this follow-up). Read-only trace generation inserts a line diagnostic before existing false returns, never changing predicates/authority. preedit_view.c is the source-driven internal/public observation helper used before edits, not a production hook or passing-C generator. Raw baseline.json/diagnosis.json preserve both stages and helper observations.

## Stage0 exact measured 2×2

Both TRUE/FALSE for every row SOURCE/CHECK0 on original candidate; native not presumed where C17 fails.

| Original Pair order | IF condition | Old C17 | First actual trusted failure |
| --- | --- | --- | --- |
| first binding | named bool | 0 | no refusal |
| first binding | inline a==b | 4 | copy_if_selection line511: bool_source_origin returns0 on genuine equality node, from==NULL |
| after two independent scalars | named bool | 4 | copy_if_pair_frame line997: declaration.symbol3!=1; root-first comparison is also false but short-circuited |
| after two independent scalars (exact R) | inline a==b | 4 | selection line511 from==NULL BEFORE Pair gate |

C17 CLI4 is V1-BACKEND-UNSUPPORTED, stdout0 bytes, path-only diagnostic with no backend source span. R exact owned IF18 spans142–198 in the retained body. P's Stage0 also measured FALSE emission4; R had stopped before FALSE emission, so this is additional calibrated P evidence, not retroactively an R result.

Actual owned entry for third-binding Pair: original binding3→place3/inc3/fact3/value5, declared children4/inc4/fact4 and5/inc5/fact5. Both arm worlds retain these original packages; left37/right211. Existing write/read evidence links these same identities (root3→9, left4→8, right5 stable for inline). Thus a genuine original aggregate/ref exists; the first refusal is an overrestrictive driver pattern, not unproven ownership or an invalid NewLang program. First+named uses the same R whitespace/values successfully: whitespace is not the first frontier.

Secondary isolated renderer observations (not misreported as the original first refusal): v1_emit_scalar rejects the actual equality node, existing emit_quantity_bool handles it; boolean_scalar expects a root binding/expression position and rejects the inline initializer. The ordinary loan/field renderer already uses the real source symbol, so no physical carrier representation change is needed.

## Bounded repair and trust

Production change only src/main.c,95 added/37 removed =58 net additional C LOC versus old53b6883, below150–200 net ceiling; CMake14 lines adds3 tests. Existing selected semantics, parser/checker/issuer/public API/validator, Canonical/docs/toolchain/workflows unchanged.

- Replace first-declaration/places[0]/bindings[0]/fixed child1/2 and later-binding position formulas with actual source declaration→checked binding→entry place→validated original fixed children. Bound/value/type/current package, parent/child relationship, incarnation and point-time write/read facts are cross-checked. Final ended retained package comes from that same binding. Numeric coincidences and raw addresses never confer rights.
- Existing both-arm unchanged-prefix checks, qualified IF parent/node/arm membership, literal/source/Copy lineage, pure arm structure, no authority from bool, one actual later scoped write, old/new Change/Reset and unchanged sibling checks remain. No scalar-known result is invented at IF join. Fresh fact-counter ordering is still a consistency check, not a root identity derivation.
- Existing bool operand validator is reused only when the source-issued equality is the initializer of THIS root-attached IF. Its normal entry wrapper retains prior root-only behavior. Existing equality C renderer now renders that already-validated IF condition; no new operator/grammar/type/IF effect semantics or proof family.
- No positional case table, R name/value whitelist, new scalar/Domain/Allocation grant, public Pair certificate, ownership registry, memory phi/SSA/effects/Array/ABI change. Independent private trust-consumer review is still needed before adoption.

## Genuine source-owned and physical result

All12 positive file paths (10 distinct source hashes; exact-R and late-inline matrix replicas intentionally alias bytes) now CHECK0/C170. Matrix all4 cells now succeed, plus middle declaration / source-selected Copy alias controls. Public probe uses only actual loading/register/check/accessors and destroys original AST/source before observing. It derives root/children from actual checked views and reports both original frames, unknown joined/input scalar, old/right packages and ended loan. Public generic Pair/IF validator is not claimed or introduced.

R exact TRUE/FALSE generated bytes are saved as evidence/r516_*.nl.c17.txt with hashes in candidate.json:

```
const size_t nl_local_1 = ...29;
const size_t nl_local_2 = ...29_or_30;
nl_type_10 nl_local_3 = { .f0 = 37, .f1 = 211, };
const uint8_t nl_local_4 = ((nl_local_1 == nl_local_2) ? 108 : 173);
/* actual scoped ref points to &nl_local_3.f0; capture *ref, write selected once */
```

Unmodified emitted C is first compiled/executed successfully. Then passive observations read the original Pair in its defining generated function, before stack frame exit; no host assignment/guest data injected. Same original physical carrier and field pointer, old37, TRUE left108/FALSE left173, unchanged right211. Plain/observer outputs exit0. Source-generated checked joined/input u8 remain unknown, while actual physical selected value is108/173. Original root/child incarnations3/4/5 and original implicit governing identity0 are retained; no heap Allocation/Domain authority exists in this lexical profile. Final function cleanup ends the root, not a claim of live final snapshot.

Local strict GCC14.2 and Clang23.1.2, C17 Wall/Wextra/Wpedantic/Werror O0/O2:96 plain/observed exit0 executions across the12 positive paths;340 corrupted-C exit91 refusals with the same passive observer. R's2 exact replicas alone contribute16 positive native runs and56 corrupted refusals. Corruptions: swapped arms/direction, selected/copy carrier, old value, sibling, wrong field/different original C carrier. The different C Pair exists ONLY in a deliberate corrupted-output control, never a passing guest substitute. CI uses repository-pinned Clang23.1.3 unchanged.

## Negatives, rollback and OOM

Original R invalid ELSE u8(256), despite known TRUE, still registration semantic CLI3 V1-U8-LITERAL-RANGE; parser accepted, no C/native, public state rolls back/artifact NULL. All R source bytes/expected values immutable. Wrong-field/disjoint-root/second-write follow-up cases remain CHECK0/C174 profile refusals, not unsafe-source findings. Existing P514 positives/native, invalid arm/condition, scoped ref rejection and P506 per-arm structural mutation P6-JOIN-PRECISION HOLD remain in the unchanged original test suite.

R TRUE/FALSE private test-owned artifacts each78 modified-evidence cases refuse, including both correlated cached truth+package corruption, actual operand source, mixed/foreign arms sharing the exact retained source plan, declaration binding/root/place/currentness/incarnation/old/new facts, wrong scope/Domain, ended-scope mismatch, and sibling. Original gate revalidates after each restoration. Hashes cover semantic context/arrays, nodes, scoped state and both worlds; unchanged snapshots are also checked for valid consumption. Gate allocations0 with first wrapped allocation forced to fail. Additional matrix forms have the same targeted controls; original P514572 control cases also pass unchanged.

Exhaustive OOM until first success for exact R source: registration233 allocations / actual main131 allocations. Every injected failure OUT_OF_MEMORY, unchanged public semantic snapshot, NULL output, retry; no allocation added by the changed consumer. Actual-source provenance retained, not host-created semantic fixtures.

Local complete partial suite303/303 PASS in41.38s. A later test-only cached-truth corruption was also executed in the GCC/Clang scripted runs, with no production behavior change. One natural FULL5 + NO LLVM/PARTIAL pair at the submitted final HEAD only; terminal CI results and exact checkout/tree recorded in PR/Issue after completion. No CI prediction is recorded as success here.

## Goal alignment, economics and STOP

Compass→Strategy1.1/Process4.2–4.3.1/Coverage, Draft17.30 and DI001/006/007/011 reviewed. KEEP the already-admitted original lexical Pair/scoped Change/pure Copy IF and usize equality semantics. Historical N/A for normative delta only: this corrects an emitter consumption constraint for source already CHECK-accepted; no new grant or source/semantic decision. Broader Pair branch mutation/effect/owner/Array remains DEFER/HOLD.

Preflight predicted two mechanical consumer contexts, not two new proof families. Actual first-predicate trace confirmed the two axes; repair adds58 net C LOC versus estimated<=100–150. No compiler/root proof redesign was necessary. Exact old→new head and wall time are recorded in terminal Issue report. Non-Production harness construction details (empty named-comparison string in direct-condition observer) were fixed before publication, source/oracle untouched.

VALUE / technical distinction for Coordination: false declaration-order rejection is removed while preserving the same original physical object. This is an Exploration capability/coverage correction, not demonstrated author+ordinary-safe-caller UB audit/re-audit dependency-closure reduction or Product GO. Fairly factored C/Zig/same-topology Rust can hoist the pure choice. Alternative: leave PR515 HOLD, direct existing scoped write without IF, or defer expensive P506 structural join/Array. No general safe library API or H1/STB product claim. Repeated source-specific gates could cost more than their value; this is the single authorized follow-up. Further investment requires a real burden/reuse case and Coordination decision. R first-pass FINDING/native-INCONCLUSIVE remains historical independent evidence; a P replay cannot label R SAFE. Stop after one terminal Issue517 report, no merge/main/canonical/other Track/Issue-close action.
