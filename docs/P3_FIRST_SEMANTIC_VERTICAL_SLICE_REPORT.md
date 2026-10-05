# P3 — First Semantic Vertical Slice

Date: 2026-10-05 (Asia/Tokyo). **P3 READY FOR REVIEW.**
Implementation, local matrix and PR validation evidence are recorded below.
This is a bounded fragment checker, not a full NewLang semantic frontend.

## Repository and authority

- Repository: https://github.com/wakairo/NewLang_Compiler
- Branch: `p3-first-semantic-vertical-slice`, based on merged P2/main
  `cd229877d550e913260ad5a79627b65c1eb405d0` (P2 PR #4).
- P3 PR: [#5](https://github.com/wakairo/NewLang_Compiler/pull/5), OPEN.
  No PR has been merged by this task. Current head and final run evidence live
  in the PR validation section / [current checks](https://github.com/wakairo/NewLang_Compiler/pull/5/checks).
- User P3 handoff SHA-256:
  `c55d2d35769144735098bc0d1e4210682fe84017a547494d8a761649323236a9`.
- Historical Draft 17.4 / Backend Contract v0.4 snapshots and input identity
  manifest are unchanged. Normative baseline is now the upstream-reviewed
  Draft 17.5 clarification, incorporated from main PR #6 at
  `10f4ffe5a02c846e4079b56e9254fc4b4e38f0de`. Its diff was inspected: no semantic
  delta; explicit §12.1 / §14.2–3 / §18.2 call-boundary wording only.
  Draft 17.5 SHA-256:
  `055b5383b5c5a749ba3c3e5709afb90565fa4e778c9939bac9533b74c94666e9`;
  review resolution SHA-256:
  `f77289b78c0f2bba24e6006071d49241c586a25eaf7807b2c8498118f2c6744c`. Relevant rules: Draft §§4, 11.4, 12, 13.4/13.7/13.8, 14.1–3,
  17.4, 18.2; selected M8.1/M8.2 forms are supplied by the user handoff.
- Authority: normative language > normative backend > adjudicated surfaces >
  reviewed formal evidence > historical M7/oracle evidence > implementation.
  Standalone M8 artifacts were not available for independent hash verification.
- The user's explicit ENDING-ARG-01 clarification applies existing §11.4 + §12
  before ordinary non-Copy transfer at selected compatible ref operands. This
  closes the semantic blocker; the original M8 wording gap is historical DOCUMENTATION-GAP. Draft 17.5
  now provides the explicit normative wording; no semantic blocker remains.
  The [contract and authority audit](P3_SEMANTIC_SLICE_CONTRACT.md) preserves the
  evidence, minimal two-root example and resolution. No historical spec is edited.

## Architecture, ownership and failure

`NLSource` -> P1 streaming lexer -> P2 immutable syntax tree -> P3 semantic
context/checker -> owned immutable checked fragment. P2's parser and syntax
representation are unchanged. The two new libraries do not link LLVM, Python or
Lean; the CLI still returns `P0-COMPILE-UNSUPPORTED` for source paths.

The caller owns `NLSemanticContext`. It owns copied registry names and bounded
append-only tables for types, bindings, packages, places, domains, scopes and
exact function signatures. IDs are context-local, zero-invalid and conceptually
distinct. Getter views are copies, not pointers into reallocatable tables.
Counters are context-local; two contexts do not interfere.

Types are canonical nominal/unit/ptr/ref/slot identities, not syntax strings.
Nominals register Copy/Discardable; Copy without Discardable is rejected. Ptr and
ordinary read/write refs are Copy+Discardable, exclusive refs are non-Copy+
Discardable, LifetimeDomain and slot are non-Copy+non-Discardable. Compound type
axis identity includes target, access and exclusivity. Nested ptr/ref type
construction is supported without implementing nested authority transitions.

Bindings are single-assignment, Available or Consumed. Ordinary Copy use creates
a fresh package; non-Copy use transfers the existing package. Local carrier
places are separate from pointer/ref referents. Packages identify current values;
places retain object incarnation and governing domain. Replacements update
current value facts, not object identity. Explicit ref scope/provenance facts
are checked separately from deferred hidden Value/Occurrence dependencies.

`NLCheckedFragment` owns one flat node array with artifact-local indices,
operation kinds, spans, resolved IDs, argument links, use/adaptation records,
result responsibilities and loan plans. It stores no syntax pointers and survives
syntax destruction. Context IDs require the original live context for meaning;
optional source text needs the live source. Neither owner is dereferenced by
artifact destruction. Freeing the artifact does not silently discard loose
semantic results. Unit has zero result responsibilities; take has two distinct
result IDs, never a tuple package. Host fixture APIs may bind each separately;
source multi-result receiving and single-name receiving of zero/two results are
explicitly unsupported.

Every check deep-clones candidate state, evaluates left-to-right, and swaps it
into the original context only after complete success and artifact construction.
On any failure, candidate/artifact are released and public context views, names,
counts, availability, carrier roles, domain/root/scope state and freshness history
remain unchanged. Registration APIs use the same boundary; ending a fixture
scope validates all preconditions before its one-bit update. This is deliberately
small and conservative, without a general transaction/allocator framework.

Diagnostics distinguish semantic error, semantic unsupported, analysis precision
limit, OOM, implementation resource limit and invalid/internal API state. Primary
canonical spans identify callee, argument, binding name, loan source or stability
operand. The renderer projects spans through preserved source bytes into the
existing structured diagnostic sink; checker subsystems do not print directly.
Limits of 4096 entries/table/checked artifact and 128 parameters/traversal levels
are host budgets, not language limits. P2 also bounds syntax depth at 128.

## Core operation matrix

All root payload transitions currently require flat user-nominal T and known
free hidden dependencies. In this matrix, ordinary ref/ptr arguments are copied;
exclusive arguments at compatible selected operands are scoped reborrows.
Copy packages and argument temporaries are distinct from referent transitions.

| Operation | Argument requirements | Copy/consume behavior | Authority | Result shape | Semantic state change | P3 limitation |
|---|---|---|---|---|---|---|
| `LifetimeDomain()` | No arguments | Fresh non-Copy/non-Discardable package | Constructor | One LifetimeDomain | Fresh live DomainId; binding transfers preserve it | No source declarations/domain aggregates |
| `ptr_from_ref(r)` | Live ordinary read/write ref, valid incarnation/provenance | Copy ref argument | Existing ref; no new borrow | One ptr<T> | Same referent/incarnation, scope-independent token; no new referent/domain | Exclusive input unsupported pending adjudicated evidence |
| `finalize_domain(d)` | Live domain; no governed live roots or conflicting refs | Consume d | Owned LifetimeDomain | Unit/zero | End domain identity; no implicit destructor | Hidden dependencies rejected; no general dependency solver |
| `initialize(s,v,stable)` | Empty slot<T>, exact T, live matching ordinary read-domain stability | Consume slot; copy/consume v by capability; copy stable or scoped compatible exclusive reborrow | Stability keeps selected live domain | One ptr<T> | Install package; fresh incarnation/current fact; root governed by stable domain | No raw storage allocation or core-authority nesting |
| `take(p,ending)` | Valid ptr to current independent root, matching live domain | Copy p; operation-local exclusive child; outer ending remains Available | Exclusive read-domain ending authority | Separate T + slot<T> | End root/incarnation liveness, detach package, return occupancy | No tuple/direct multi-result source receiver |
| `destroy(p,ending)` | As take, plus Discardable(T) | Copy p; exclusive child, outer reusable; end old package | Same ending authority | One slot<T> | End root and package; return occupancy | No implicit discard of non-Discardable T |
| `replace(r,v)` | Live write ref, exact T, no conflicting exclusive ref | Copy ordinary r / compatible exclusive reborrow; copy/consume v | Write access; no lifetime-ending authority | One old T | New current package/fact, old package loose; incarnation/domain unchanged | Core payload/dependency summaries deferred |
| `store(r,v)` | As replace, plus Discardable(T) | As replace; old package ends | Write access | Unit/zero | New current package/fact; incarnation/domain unchanged | Non-Discardable store rejected |
| `swap(a,b)` | Live write refs, same T, known identical or disjoint fixture sites | Ordinary refs Copy, not exclusive; compatible exclusive reborrow permitted | Write access; no Copy/Discardable(T) requirement | Unit/zero | Same-place: exact referent state/fact no-op. Distinct: exchange packages, fresh facts; each incarnation/domain stays with its place | No fixed-subobject/partial overlap analysis |

Operation-local children stay live across later argument evaluation and the
operation, then end on return. Parent suspension is tied to the exact authority
package; sequential take/take/destroy is tested. A later nested reuse while the
child is live is rejected and rolls back. Supported registered calls cannot
return scope-dependent/core-authority capabilities, so child escape is not
silently permitted by a type-only signature.

## Loan acquisition matrix

Only the header is checked. Body regions remain opaque, including names/calls
inside them. Each accepted header records a fresh inactive scope plan, referent
place/incarnation, source/stability symbols, canonical body spans and future
lifetime/conflicting-access obligations. No inner binding or permanent outer
borrow is installed. Source bindings remain Available. **Scope acquisition is
checked; body compatibility and nonescape are not proved.**

| Form | Verdict with valid facts | Authority / conditions |
|---|---|---|
| Local read | Accepted | Existing compiler-managed live local; no conflicting exclusive ref, Draft §§11–12 |
| Local write | Accepted | Ordinary write is not exclusive; ordinary aliases allowed, §§11–12 |
| Local exclusive read | Accepted | No conflicting live capability, §12 |
| Local exclusive write | Accepted | No conflicting live capability, §12 |
| Ptr read using stable | Accepted | Current incarnation, proven provenance/read access, matching ordinary live read-domain stability; §13.4 and supplied M8.1 form |
| Ptr write using stable | Accepted | Above plus write access; ordinary write aliases allowed |
| Ptr exclusive read using stable | Accepted | Above plus no conflicting referent capabilities; §12/§13.4 and supplied M8.1 form |
| Ptr exclusive write using stable | Unsupported/deferred | Source grammar acceptance does not establish closed safe acquisition semantics; no verified standalone M8 rule, task §227 |

Local `using`, missing ptr stability, wrong domain, dead/stale or known invalid
facts and proven conflicts are semantic errors. Missing provenance/scope/alias
knowledge is an analysis precision limit. Stability's ordinary write -> read is
recorded compatibility, not a new source borrow. Nested ref/slot local-source
loans and exclusive stability forms return explicit unsupported results. The
exclusive-stability header needs a body-extent reborrow plan; it is not declared
semantically illegal and is separate from supported call-local reborrows.

## Registered calls and proved boundary

The host context registers one exact signature per name; there is no overload
ranking, declaration/body checking or module shadowing. A fixed prelude uses a
separate callee namespace from local bindings. Unknown callee/type/binding names
are semantic errors. Registered calls support nominal/unit results, ordinary
value use and nonescaping ref operands with no caller-visible effects/hidden
dependency summary. Marked effects return unsupported; hidden summaries return
precision limit; affine domain/slot transfers and returned core capabilities
need richer summaries and are unsupported.

Ordinary `ref<write,T>` -> selected `ref<read,T>` is allowed and recorded; the
reverse is rejected. Existing exclusive authority uses its scoped reborrow
mechanism first at compatible same-mode selected ref operands. The new §12.1
clarification does not establish new exclusive type compatibility: exclusive
write-to-read argument mode changes are explicitly unsupported in P3, rather
than being assumed from ordinary Copy weakening. Ordinary exclusive-ref binding
transfer (`let e2 = ending`) still consumes the old binding and preserves package
identity; both distinctions are regression-tested. Standalone uses are not
Copy weakening; T -> ref<T> is never synthesized. Any surviving package marked
hidden/unknown dependency rejects the whole fragment, including a nominally
no-op swap. This deliberately avoids proving safety from incomplete summaries.

Established: name/type identity, availability, Copy/consume/reborrow use,
Discardable applicability, basic root/domain matching, incarnation/provenance,
result type/arity and supported loan-header acquisition applicability, relative
to explicitly seeded context facts.

Not established: physical allocation/storage facts, loan body nonescape,
function-body post-state, hidden Value/Occurrence dependency compatibility,
fixed-subobject structural transition, sum occurrence, Storage/BackingRegion
byte authority, relocation, LLVM optimizer promises or whole-program soundness.
There is no automatic noalias, captureless, lifetime marker or C aggregate ABI
claim. These backend constraints remain backend-owned and conservative.

## Evidence and tests

The frozen M7.5 oracle archive and hashes are unchanged; existing oracle smoke
still runs in every configuration. Static inspection finds direct ending-name
consumption in its historical take/destroy branch. Oracle differential for P3 is
**N/A**: its complete-program environment differs from the seeded fragment API,
and that historical argument mapping is not the clarified rule. Direct C
contract fixtures provide evidence; production never shells out to the oracle.

Formal repository inspected read-only at
`fdda0d99d3de961f782f465fa2033b5554790ba5`. F0.1 replace, F0.2 store, F0.3 swap,
F0.4 lifetime, F0.5 ref/ptr and F0.6 domain specifications corroborate abstract
package/root transitions. `Reference.lean`, `Lifetime.lean`, `Domain.lean`
explicitly abstract/defer scoped applicability; they are not a proof of this C
checker. Ghost Finset histories and Lean representation choices were not adopted.
Lean was not built and is not a production dependency.

The existing **20 CTests are preserved**, plus six unit groups and one integration
entry (**27 total**). Tests use public views rather than private storage layout;
checks remain active with NDEBUG.

- `semantic_type`: all required type forms, nesting/canonical identities,
  capabilities/invalid Copy and unknown names.
- `semantic_value_use`: Copy twice/fresh packages, non-Copy transfer/use-after,
  duplicate binding, exact calls/arity/names, no implicit borrow, ordinary mode
  weakening/reverse rejection, exclusive reborrow/sequential reuse/nested
  suspension, ordinary exclusive transfer, conservative unsupported exclusive
  mode changes and dead child/parent scope rules.
- `semantic_domain`: constructor/move identity/finalization/live-root refusal,
  read/write-ref ptr derivation, scope-independent ptr and unchanged referent identity.
- `semantic_transition`: initialize/typed slot/stability/rollback, take/destroy
  including non-Discardable take and refusal of non-Discardable destroy, wrong
  domain/nonroot/stale pointer, sequential ending reuse, separate result
  responsibility receiving/reinitialize fresh incarnation, replace/store,
  ordinary write aliases, same-place/distinct/non-Discardable swap, fresh facts
  and preserved incarnations/domains.
- `semantic_loan`: four local/three ptr positive forms, ptr exclusive-write
  unsupported, matching stability/currentness/access, local/missing using,
  active exclusive conflict, ordinary-write alias, ended stability and explicit
  unsupported exclusive-stability header.
- `semantic_failure`: hidden/unknown markers, unsupported summaries, unknown/
  invalid provenance/access, left-to-right rollback, fresh domain/root/fact
  rollback, repeated failure, allocation failure and host entry/parameter limits.
  Malloc/realloc fault injection walks every encountered allocation on eight
  checker paths (including checked-array growth) plus constructor/seven owning
  registration paths. The injected allocator exists only in tests.
- `source_semantic.integration`: preserved bytes -> lexer -> P2 -> P3 across two
  independent contexts, deterministic checked structures, tree-before-artifact
  destruction, domain transfer/finalize, loan opaque body, diagnostic role spans
  and renderer, and destruction after borrowed owners are gone.

Baseline main: bootstrap/format PASS, GCC and Clang **20/20 PASS** before code.
Local host: Debian 13 Linux x86_64; GCC 14.2.0, Clang/LLVM/clang-format 23.1.2,
C17, CMake/CTest 3.31.6, Python 3.12.14. Ubuntu 24.04 PR jobs report
GCC 13.3.0 and Python 3.12.3, with the same locked LLVM/Clang and CMake.
Toolchain/bootstrap locks are unchanged.

| Final configuration | Local result |
|---|---|
| GCC Debug, strict warnings | 27/27 PASS |
| Clang Debug, strict warnings | 27/27 PASS |
| Clang ASan, leak detection | 27/27 PASS |
| Clang UBSan, halt-on-error | 27/27 PASS |
| GCC Release, `-DNDEBUG` | 27/27 PASS |
| Locked clang-format check | PASS |

The same CTest suite includes diagnostics, CLI unsupported, LLVM C API, oracle
smoke and artifact integrity. Linux linker allocation wrapping is test-only.
New C targets use existing `-Wall -Wextra -Wpedantic -Werror`; sanitizer flags
instrument both semantic/checked libraries and their tests.

## Reproduction and CI

Use the README's existing rootless bootstrap/activation and GCC/Clang/ASan/UBSan
commands. No new application/service credentials or environment setup is needed.

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake --fresh -S . -B build-gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build-gcc --parallel 2
ctest --test-dir build-gcc --output-on-failure
bash scripts/check-format.sh

CC=gcc cmake --fresh -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel 2
ctest --test-dir build-release --output-on-failure
```

Existing Ubuntu 24.04 PR workflow discovers the new tests automatically in GCC,
Clang, ASan and UBSan jobs. Its pinned actions, exact LLVM bootstrap, sanitizer
options and original tests remain intact. Local Release/NDEBUG is additional
validation. Implementation run [37251168999](https://github.com/wakairo/NewLang_Compiler/actions/runs/37251168999)
passed all four Ubuntu 24.04 PR jobs at
`337aaeacc75f22d5d61dee76f206b022ebff08fc`, each 27/27. This is explicitly
pre-final-documentation evidence. Run [37251445160](https://github.com/wakairo/NewLang_Compiler/actions/runs/37251445160)
also passed all four jobs at `41d65bf60ef82ea230182eaf32b350c27d87a6d1`.
Both precede the final Draft 17.5 reference/compatibility sync. After that final
commit, the full local matrix and PR-triggered workflow are rerun. The final
current-head SHA/run URL and green job results are recorded in the PR body and
completion response, with live current checks linked above; earlier green runs
alone are not final-head evidence.

## Findings and review gate

| Classification | Result |
|---|---|
| COMPILER-SPEC-HOLE | No new blocker; inherited lexical/open surfaces unchanged |
| COMPILER-SPEC-AMBIGUITY | ENDING-ARG-01 resolved by explicit user clarification, no blocker; original M8 DOCUMENTATION-GAP recorded; Draft 17.5 normative sync supplies explicit wording |
| COMPILER-IMPLEMENTATION | No known unresolved defect after validation |
| COMPILER-IMPLEMENTATION-LIMIT | Header-only loans, free dependencies, flat nominal root payloads, limited call summaries/receiving; ptr exclusive-write and exclusive ptr_from_ref inputs, exclusive mode-changing arguments explicitly unsupported |
| COMPILER-DIAGNOSTIC | Role-based first diagnostics tested; no new blocker |
| COMPILER-PERFORMANCE | Deep copy / linear registry scans favor rollback clarity; bounded fixtures, no whole-program scalability claim or optimization work |
| COMPILER-LOWERING | No new finding/lowering; M7 backend constraints preserved |
| COMPILER-PORTABILITY | Existing Linux x86_64 toolchain and test linker wrapping boundary unchanged; WSL2 not separately executed |

P3 scope is implemented; stop at **P3 READY FOR REVIEW** after final current-head
checks are green, for human/ChatGPT review. P2 remains syntax-only; no full program/body checker, M8.3/M8.4 grammar,
effect/dependency solver, raw storage/FFI/relocation, MIR or LLVM lowering was
added. Next: review this slice and its limitations before selecting a separately
reviewed P4 scope. This task does not start P4 or merge its PR.
