# NewLang Compiler

NewLang production compiler, written in C17 with LLVM as the primary backend.

**P0 through P7 are complete and merged.** P6 adds the Draft 17.10 bounded
registered closed-sum source slice, and P7 adds bounded dependency-aware
ref-valued match-result joins without changing canonical semantics.
P7 is reviewed/merged and CLOSED. P8 adds host-registered non-generic block bodies,
definition-time validation and body-sensitive direct known calls; **P8 is reviewed/merged and CLOSED**.
P9 implements the Draft 17.11 dedicated `return expression;` item in registered
ordinary-function lexical blocks, with bounded zero/one-normal-arm match flow.
[P9 review evidence](docs/P9_EXPLICIT_RETURN_SLICE_REPORT.md) records the scope and limits.
P10 enforces Draft 17.12's exact `unit` ordinary lexical name reservation while preserving builtin values and member labels.
P12 enforces Draft 17.14 ordinary-name reservation for `fn`, `let`, `return`, and `match`, with diagnostics distinct from core `unit` and member labels preserved.
P11 adds Draft 17.13's bounded top-level non-generic `fn` declarations, whole-unit signature visibility and acyclic body calls. Recursive body analysis remains an explicit precision limit.

P13 adds Draft 17.15's minimal core `bool` and exact `if (expression) { ... } else { ... }`, finite normal/return joins and if/else ordinary-name reservation. Distinct non-Copy conditional identities remain structured precision rejections. [P13 report](docs/P13_IF_SOURCE_SEMANTIC_SLICE_REPORT.md) records the review scope.

P14 adds exact Draft 17.17 `loop (...) { ... }`, `continue(...);` and `break expression;`
with unknown Copy-carried slots, an exact captured frame, all-edge closure and finite
Break/ref joins. Rich cyclic correlations remain structured precision rejections.
[P14 contract](docs/P14_LOOP_SOURCE_SEMANTIC_SLICE_CONTRACT.md) and
[P14 report](docs/P14_LOOP_SOURCE_SEMANTIC_SLICE_REPORT.md) define the bounded review scope.

## Authority

The canonical language authority is the Draft selected by
`docs/reference/CURRENT_SPEC.md` (currently **Draft 17.30**), followed by reviewed
process/backend contracts and merged implementation/formal evidence.
Historical reports, the task prompt and conversation do not override the Draft.
Backend Contract v0.4 defines backend obligations. The Charter, Handoff Manifest
and project process documents are policy/evidence, not language semantics.
[F0 / NewLang_FormalProof](https://github.com/wakairo/NewLang_FormalProof) and the
frozen M7.5 Python oracle are evidence; Lean is not a build dependency.
M7, P4/R1/F1.4/F1.5, P5, R2, M9.1/P6/R3/M9.2/P7 are reviewed/closed.
Semantic Sync checkpoints and later track starts are recorded through separate Coordination handoffs.
See the [P2 grammar audit](docs/P2_MINIMAL_SYNTAX_CONTRACT.md),
[P3 semantic contract](docs/P3_SEMANTIC_SLICE_CONTRACT.md),
[P4 raw-storage contract](docs/P4_RAW_STORAGE_SEMANTIC_SLICE_CONTRACT.md) and
[P5 source contract](docs/P5_SOURCE_FRONTEND_SLICE_CONTRACT.md) and
[P6 sum contract](docs/P6_CLOSED_SUM_SOURCE_SLICE_CONTRACT.md).

## Fresh setup — proposed fixed LLVM 23.1.3 full CI (Issue #449)

This section describes a **DRAFT, Coordination-gated toolchain migration**.
The Canonical remains Draft 17.30; the compiler/source/checked-owned/C17
semantics and Production tests are unchanged. Until its actual first five-job
CI and independent review pass, this is *not* proof of an adopted main toolchain.

Full curated LLVM SDK bootstrap is supported on **Ubuntu 24.04 x86_64**
(including WSL2 Ubuntu 24.04 userspace when prerequisites match). It does
**not** fetch the original 1.19 GB official archive, or the old 23.1.2
apt.llvm.org .deb packages. Requires Python >=3.12, CMake >=3.28,<4.0,
GCC13/G++13, make, zstd, and standard signed Ubuntu static-link developer
libraries (zlib/zstd/tinfo/XML as required by LLVM).

```sh
sudo apt-get update
sudo apt-get install --no-install-recommends -y \
  gcc-13 g++-13 make cmake python3 zstd \
  libstdc++6 zlib1g-dev libzstd-dev libtinfo-dev libxml2-dev
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake -S . -B build-gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build-gcc --parallel 2
ctest --test-dir build-gcc --output-on-failure
./build-gcc/newlangc --version
```

The committed `scripts/toolchain-lock.json` selects exact `llvmorg-23.1.3`
source commit `0d261d1ca552c95a8f007e061c787ac7132fbcbc` and a separate
**unofficial NewLang-curated GitHub prerelease** asset, exact SHA-256
`5369d292ef513d32758634f98ec7cf62c52f82a0a333eb330ecf9a325648b133`
(126,520,077 bytes). Both cached bytes and freshly downloaded GitHub Release
bytes are SHA-checked before bounded `zstd --long=30` tar validation and
rootless extraction. `.deps/activate.sh` activates exact SDK-local
`llvm-config-23`, `clang-23`, `clang-format-23`. It is static-only LLVM;
CMake links the unchanged `llvm.c_api` via static LLVM libraries and uses the
C++ linker **only for the infrastructure C API smoke**. The Production
compiler still uses strict C17. No `latest`, unreviewed mirror, binary
maintainer script, SDK-dependent frontend fallback or trust in cache hits.

The original LLVM GitHub tarball had a verified upstream GPG signature, but
**the curated archive is not separately GPG signed**. Its authenticity is
the *committed exact GitHub-hosted archive SHA*, plus its internal
`PROVENANCE.md` and `licenses/`. Its selection was compared locally against
496 original LLVM SDK entries. License/NOTICE inclusion is a bounded audit,
**not a legal certification or complete SBOM**. See
[toolchain P0 decisions](docs/P0_TOOLCHAIN_DECISIONS.md) and the
[unofficial prerelease](https://github.com/wakairo/NewLang_Compiler/releases/tag/llvm-23.1.3-newlang-sdk-linux-x64-v1)
for authorship, credits, limitations, and the disclosed Unicode-notice date
discrepancy.

`bash scripts/check-format.sh` uses the active `clang-format-23`; the
ordinary five-way workflow still executes GCC, GCC Release, Clang, ASan,
UBSan, every registered CTest, and the unchanged `llvm.c_api`. An
independent `NEWLANG_FRONTEND_ONLY=ON` **NO LLVM / PARTIAL** lane remains
available for Production source/checker/C17 reference work during SDK
infrastructure incidents; it never replaces full backend/native proof.

The older **Debian 13** snapshot .deb lock remains recorded as a legacy
fallback but is **not** the curated Ubuntu 24.04 full CI. The original
23.1.2 Debian/Noble cross-distribution guidance in historical P0 decisions
does not describe this curated install path. Once a toolchain migration is
approved, rerun bootstrap and use a **fresh** CMake build directory to avoid
persisted LLVM C API link targets. This is a candidate until Coordination
independently merges the PR.

## Other validated configurations

```sh
. .deps/activate.sh
CC=clang-23 cmake --fresh -S . -B build-clang -DCMAKE_BUILD_TYPE=Debug
cmake --build build-clang --parallel 2
ctest --test-dir build-clang --output-on-failure

CC=clang-23 cmake --fresh -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DNEWLANG_SANITIZER=address
cmake --build build-asan --parallel 2
ctest --test-dir build-asan --output-on-failure

CC=clang-23 cmake --fresh -S . -B build-ubsan -DCMAKE_BUILD_TYPE=Debug -DNEWLANG_SANITIZER=undefined
cmake --build build-ubsan --parallel 2
ctest --test-dir build-ubsan --output-on-failure
```

The historical P0–P14 inventory of **95 CTests** has expanded. The adopted-roadmap snapshot ran **263 CTests per job**; at exact merged PR420 main `6fc8ba26d34a2e2aff0aed92512859ecf40bd0a5`, all five jobs ran **265 tests per job**. Use `ctest --test-dir build-gcc -N`
to inspect the exact inventory of your checkout. The following is a historical
summary of the included source and lexer module unit
tests, six parser unit groups, file-source -> lexer/parser integration,
six P3 semantic unit groups and source -> parser -> checker integration,
ten raw-storage unit groups and the raw -> typed -> raw -> deallocate cycle,
six P5 source-slice unit groups and source -> checked lifetime/aggregate integration,
seven P6 sum-slice unit groups and source -> checked match integration,
five P7 ref-join unit groups and PW1–PW5 source -> checked integration,
four P8 body-registration/exit/ownership/rollback unit groups and body-sensitive direct-call integration,
a targeted R4-01 argument-compatibility regression,
six P9 return syntax/transfer/exit/failure/effects/availability unit groups and Result-like decoder integration,
three P10 unit-name admission/builtin/failure groups,
seven P11 declaration/visibility/body/failure/precision/resource groups,
four P12 structural-name ingress/header/member/failure groups (including if/else),
nine P13 bool/if grammar/outcome/identity/ref/effect/nesting/failure/resource groups,
forty P14 isolated loop source/control/header/join/ownership/OOM workloads,
diagnostic unit checks, five CLI option/help cases (each invoked twice), one
North Star V0 Checked-C source/check/emission/host-execution integration test,
valid LLVM C API module/IR, artifact integrity, and frozen-oracle smoke. ASan includes leak detection; UBSan stops on the first
failure. Imported LLVM binaries are not rebuilt with sanitizers; our C targets
are instrumented. Required checks are these configurations plus GCC Release/NDEBUG and PR CI
(see [P10 report](docs/P10_UNIT_NAME_RESERVATION_REPORT.md)).
The historical 466-test oracle suite and formal proofs are optional evidence;
P0 does not claim to reproduce them in C.

## CLI contract

`--version` prints `newlangc 0.1.0 (P0 bootstrap)`. `--help` (also no arguments)
prints usage; both exit 0. Invalid options / argument counts exit 2.

`newlangc --check SOURCE` runs the same production parser, function-unit
registration and semantic checking of the ordinary `main()` entry, then
destroys the source-generated checked-owned fragment without invoking codegen.
Success exits 0 with zero stdout/stderr bytes. Source I/O failures exit 1;
parser and semantic failures retain their diagnostics and exit 3. A missing
entry is a semantic failure. Backend-unsupported but semantically valid source
can pass `--check`; this does **not** establish C17, LLVM or native support.

For a source path, the North Star V0/V1/AVS path reads the actual source, parses the
bounded source function unit, registers/checks it with the production semantic
checker, then checks an ordinary zero-argument `main()` call selected only by
the driver. Only after those checks succeed does `newlangc` emit the bounded
Checked-C representation to stdout. Semantic rejection emits no C bytes.
Accepted checked constructs outside the V0/V1/AVS emitter report
`V1-BACKEND-UNSUPPORTED`.

The Checked-C path is a bounded bootstrap/reference execution vehicle for the
North Star V0/V1/AVS subset; it is not a general C backend and does not replace LLVM as
the planned primary backend.

### LLVM-independent frontend build

On a Linux host with GCC or Clang, make, CMake >=3.28,<4.0 and Python >=3.12:

```sh
CC=gcc cmake -S . -B build-frontend -DCMAKE_BUILD_TYPE=Debug \
  -DNEWLANG_FRONTEND_ONLY=ON
cmake --build build-frontend --parallel 2
ctest --test-dir build-frontend --output-on-failure
./build-frontend/newlangc --check tests/fixtures/node_join_same_locator.nl
```

These commands do not invoke bootstrap or fetch LLVM. The explicit option
skips LLVM discovery/import and only the `llvm.c_api` smoke test. All other
tests, including source/checker, negative semantics, OOM/rollback, public
validators and existing C17/native reference tests, remain enabled: that C17
emitter has no LLVM dependency and its ordinary CLI behavior is preserved.
`--check` is available in both builds; existing help/version output is unchanged.

The option defaults **OFF**. The normal build still requires exact LLVM 23.1.2,
and the normal five-configuration CI uses that default. Frontend-only results
are separately scoped partial validation and cannot replace the full gate or
justify merging new backend safety features. No native or Product PASS follows
from checking source alone.

Product/value strategy is recorded in
[`docs/NewLang_Product_Value_Strategy.md`](docs/NewLang_Product_Value_Strategy.md),
the current North Star contract in
[`docs/NewLang_North_Star_Product_Validation.md`](docs/NewLang_North_Star_Product_Validation.md),
and long-horizon C-replacement coverage in
[`docs/NewLang_C_Replacement_Coverage_Map.md`](docs/NewLang_C_Replacement_Coverage_Map.md).

The adopted, non-normative [evidence inventory and development roadmap](docs/NewLang_Development_Roadmap.md)
separates current source/checker/native capabilities from product and research investment gates.

The normal integration test compiles emitted C17
with the configured host C compiler and executes the resulting native program.


### First runnable source-to-C17 program on current main

After completing the `build-gcc` setup above, the following **real NewLang source**
uses the merged ordinary `u8` argument/result path, including **one
helper-to-helper call**. This source is the exact `nested9.nl` witness from
merged [PR420](https://github.com/wakairo/NewLang_Compiler/pull/420).
The compiler writes generated C17 to stdout:

```sh
cat > first-call.nl <<'NEWLANG'
fn echo(x:u8)->u8{x}
fn relay(y:u8)->u8{echo(y)}
fn main()->unit{
    let a=relay(u8(9));
    let b=relay(a);
    b;
    unit
}
NEWLANG
./build-gcc/newlangc first-call.nl > first-call.c
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror first-call.c -o first-call
./first-call
echo $?
ctest --test-dir build-gcc -R '^nested_u8_checked_c\.integration$' --output-on-failure
```

`echo $?` should print `0` on successful native execution. This is a **toolchain
smoke test**, not an observable guest `u8` output: `main` returns `unit`, and
this bounded emitter does not introduce guest I/O or an exit-value ABI.
The [merged nested-call integration test](tests/integration/checked_c_nested_u8_test.py)
independently observes the **actual emitted inner and outer argument/return C carriers**
for 9/255/renamed sources and detects corrupt return values. The
[P399 direct-call integration test](tests/integration/checked_c_u8_call_test.py)
remains part of the suite.

Accepted source constructs beyond this finite emitter profile still
fail closed with `V1-BACKEND-UNSUPPORTED` rather than silently producing C.
Two parameters, scoped-ref parameters, and richer callee bodies are not
covered by this example; the merged two-normal-arm `Node` checker admits
certain state-changing matches through checked-owned evidence but **cannot
emit C17 for the newly changed match**. See the
[adopted capability roadmap](docs/NewLang_Development_Roadmap.md) for the
separate SOURCE / CHECK / OWNED / C17 / NATIVE boundaries.

V1 adds only the positive-decimal `u8(DIGITS)` path and immutable u8 locals/use:

```newlang
fn main() -> unit {
    let x = u8(7);
    x;
    unit
}
```

The NewLang checker validates the mathematical value against 0..255 and stores
core u8/value evidence. Checked-C emits that checked value, never reparses the
literal. `u8(256)` and bare `7` reject before emission. `u8(-1)` remains
**Deferred / unchanged**, outside the V1 acceptance claim. No numeric semantics,
other integer families or C representation guarantees are added.
See [V1 contract](docs/NORTH_STAR_V1_TYPED_SCALAR_CONTRACT.md) and
[V1 report](docs/NORTH_STAR_V1_TYPED_SCALAR_REPORT.md).

The merged [PR404](https://github.com/wakairo/NewLang_Compiler/pull/404) implementation also lowers ordinary body-backed `fn echo(x:u8)->u8{x}`
with one runtime Copy u8 formal and one Copy u8 result. Repeated direct calls
and receiving/forwarding a result through a local use typed C17 carriers.
The finite callee body supports scalar Copy locals and a scalar tail/direct
return. Merged [PR420](https://github.com/wakairo/NewLang_Compiler/pull/420)
also supports **one authenticated nested `u8 -> u8` tail call** (relay + leaf).
A second nested level, two arguments, ref/nonCopy signatures, nested call
arguments, and general call graphs remain unsupported in this emitter.
This merged main implementation does not incorporate the unmerged G0/G1 branches;
zero-argument u8 returns retain the original unsupported backend boundary.
See [P399 evidence and limits](docs/P399_U8_COPY_CALL.md).

AVS adds one optional leading source declaration with exactly two core-u8 fields:

```newlang
struct Pair { left: u8, right: u8, }
fn main() -> unit {
    let p = Pair { left: u8(7), right: u8(9), };
    let Pair { left, right } = p;
    left;
    right;
    unit
}
```

This is the bounded D2 profile authorized by Issue #105, not general aggregate
declaration grammar. The existing checker registers the nominal shape, derives
Copy/Discardable and checks complete named-field construction/whole destructuring.
Checked nominal IDs, field indices and scalar values drive synthetic C struct
representation; source names and native layout do not become C ABI guarantees.
Only one leading declaration in one source unit is supported. Richer declaration
profiles and accepted constructs outside the emitter remain explicitly unsupported.
See [AVS contract](docs/NORTH_STAR_AVS_CONTRACT.md) and
[AVS report](docs/NORTH_STAR_AVS_REPORT.md).

The fixed-field gate uses Draft 17.22's one-level `p@left` /
`p@right` Copy reads on a direct source `Pair` local and an ordinary scoped
`loan_write(p@left){|w|replace(w,u8(11))}`. The checker tracks non-owning fixed
subplaces, refreshes target/parent current facts, and preserves the sibling.
Checked-C emits actual member reads/write and captures the old result before
mutation. Unknown dependencies and unsupported profiles stop explicitly.
See the [fixed-field contract](docs/FIXED_FIELD_PRODUCT_GATE_CONTRACT.md) and
[report](docs/FIXED_FIELD_PRODUCT_GATE_REPORT.md).

## Layout and review

Draft 17.21 catch-up adds one bounded recursive declaration category:
`struct Node { next: Option<ptr<Node>>, payload: u8 }` (names are not built-ins).
The checker creates a private incomplete nominal header, resolves the exact
Option/ptr link, rejects unbroken value-containment cycles and completes the same
identity once before checking bodies. None-link construction is semantically
accepted. Draft 17.23's semantic catch-up accepts the completed nominal's
declared link via `local@link`, its scoped ordinary write loan, and Node lexical-root
read / persistent-ptr read reacquisition. Payload/general fields and allocation
remain outside this profile. The bounded two-lexical-root Checked-C gate emits
real local Node addresses, tagged Option copies/replacements, and runtime
Some/None selection with checked read reacquisition. Its separate read-only
native observer asserts pointer identity, old packages and sibling preservation;
this is an intermediate product gate, not a general recursive backend or
North Star PASS. See the [native topology contract](docs/DRAFT_17_23_NODE_EXECUTABLE_CONTRACT.md)
and [report](docs/DRAFT_17_23_NODE_EXECUTABLE_REPORT.md).
The actual-source witness and rule/evidence mapping are in
[the Node link report](docs/DRAFT_17_23_NODE_LINK_SEMANTIC_REPORT.md).
See the [contract](docs/DRAFT_17_21_PRODUCTION_CONTRACT.md) and
[report](docs/DRAFT_17_21_PRODUCTION_REPORT.md).

The bounded Draft17.30 five-root/three-field **pre-detach** Checked-C adapter
emits the complete `tests/fixtures/five_root_three_field.nl` source through
the existing owned certificate revalidator. Native tests exercise all six
allocation worlds, actual heap links and explicit matched cleanup with an
independent read-only observer. See the
[contract](docs/FIVE_ROOT_NATIVE_C17_CONTRACT.md) and
[measured report](docs/FIVE_ROOT_NATIVE_C17_REPORT.md). This gate does not
implement B detach/adoption or claim cJSON product PASS.

Draft 17.22 separates bounded field `local@field` and closed-sum
`SumType::Variant` / `SumType::Variant(expr)` syntax. The two colons must be
adjacent. Exact recursive Option constructors use `Option<ptr<Node>>::None`
or `::Some(p)`; other generic forms remain outside the frontend profile.
Legacy dot routes and receiver/member calls are rejected without fallback.
Existing field/sum semantics and Checked-C evidence are preserved. See the
[migration contract](docs/DRAFT_17_22_SOURCE_PUNCTUATION_CONTRACT.md) and
[report](docs/DRAFT_17_22_SOURCE_PUNCTUATION_REPORT.md).

- `include/newlang/diagnostic.h`, `src/`: small borrowed-data diagnostic API and CLI.
- `include/newlang/{source,token,lexer}.h`, `src/{source,lexer}.c`: immutable
  owned source bytes/name, checked half-open spans, non-owning atom tokens and
  allocation-free streaming lexer. Structural ordinary-name admission is separate from word tokenization; broader comment/encoding rules remain bounded.
- `include/newlang/{syntax,parser}.h`, `src/{syntax,parser}.c`: owned immutable
  syntax trees borrowing source, one-token lookahead, checked failure cleanup,
  structured first diagnostics, four P2 fragment entries and the Draft 17.9 P5
  source-fragment entry for blocks/receiving/registered aggregate forms plus the
  Draft 17.10 constructor/match forms, Draft 17.13 fn-only function-unit entry and Draft 17.15 exact if. Loan
  bodies retain balanced byte regions; their contents are not parsed.
- `include/newlang/{semantic,checked}.h`, `src/{semantic,semantic_check,sum,checked}.c`:
  owning fixture context, canonical types/packages/places/scopes, transactional
  checker, bounded complete ref-result alternatives, conditional occurrence ownership and immutable checked fragments
  (including owned hypothetical arm snapshots); no LLVM linkage or syntax pointers.
- `src/function_body.c`: owned registered-body plans and function-exit dependency checking.
  Host registration supplies named exact signatures and existing lexical-block bodies;
  body-backed direct calls preserve actual alias/dependency relations and tail package identity.
  Source declaration units reuse those plans. Ordinary ref results, core-authority signatures and recursive body analysis remain limited.
- `include/newlang/raw_storage.h`, `src/raw_storage.c`: programmatic successful
  allocation/claim transitions, scalar byte observations and owned raw interval
  summaries. Compiler metadata only; no platform allocator or runtime bitmap.
- `tests/unit/`, `tests/integration/`: C unit / LLVM checks and CLI/artifact tests.
- `oracle/`, `tests/oracle/`: untouched M7.5 archive, identity, and isolated adapter.
- `scripts/`: reproducible bootstrap, dependency lock, formatter/fmt-check helpers.
- `.github/workflows/compiler-ci.yml`: PR-triggered GCC Debug/Release, Clang, ASan, UBSan validation plus
  pinned clang-format checking.
- `docs/NewLang_Aware_C_Guidelines.md`: production C implementation discipline.
- `docs/NewLang_Compiler_Testing_Strategy.md`: unit/integration/oracle/end-to-end test policy.
- `docs/NewLang_Compiler_Review_Guidelines.md`: shared review contract.
- `docs/PRE_P1_SOURCE_INPUT_AND_LOCATION_CONTRACT.md`: preserved-byte and canonical source-span
  foundation.
- `docs/PRE_P1_FOUNDATION_DECISIONS.md`: pre-P1 decisions and explicit deferrals.
- `docs/P1_LEXICAL_CONTRACT_AUDIT.md`: fixed/subset/deferred distinctions and
  lexical spec holes; the scanner subset is not normative NewLang grammar.
- `docs/P1_SOURCE_LEXICAL_FRONTEND_REPORT.md`: contracts, test evidence, findings
  and P2 readiness/adjudication requirements.
- `docs/P2_MINIMAL_SYNTAX_CONTRACT.md`: pre-implementation grammar audit,
  lexical/contextual-word profile, coverage/error distinctions and limits.
- `docs/P2_MINIMAL_SYNTAX_FRONTEND_REPORT.md`: ownership, tests and P3 handoff.
- `docs/P3_SEMANTIC_SLICE_CONTRACT.md`: authority audit, scoped reborrow rule,
  ownership/transaction contracts and conservative limits.
- `docs/P3_FIRST_SEMANTIC_VERTICAL_SLICE_REPORT.md`: operation/loan matrices,
  proved/unproved boundary, tests and P3 review handoff.
- `docs/P4_RAW_STORAGE_SEMANTIC_SLICE_CONTRACT.md`: backing identity, conservation,
  definedness/access, ownership/failure contracts and bounded deferrals.
- `docs/P4_RAW_STORAGE_SEMANTIC_SLICE_REPORT.md`: coverage, validation, findings
  and P4 review handoff.
- `docs/P5_SOURCE_FRONTEND_SLICE_CONTRACT.md`: Draft 17.9 selected source profile,
  receiving/block/aggregate ownership, transaction contracts and limits.
- `docs/P5_SOURCE_FRONTEND_SLICE_REPORT.md`: module inventory, tests and review handoff.
- `docs/P7_REF_VALUED_MATCH_RESULT_JOIN_CONTRACT.md`: public rebasing, complete ref alternatives,
  downstream uses, ownership/rollback and precision boundaries.
- `docs/P7_REF_VALUED_MATCH_RESULT_JOIN_REPORT.md`: PW1–PW5, destructive tests and P7 review handoff.
- `docs/P8_REGISTERED_FUNCTION_BODY_SLICE_CONTRACT.md`: registered body profile, formal/actual
  checking, alias/exit invariants and durable ownership/rollback contracts.
- `docs/P8_REGISTERED_FUNCTION_BODY_SLICE_REPORT.md`: W1–W4 disposition, tests, limits and review handoff.
- `docs/P9_EXPLICIT_RETURN_SLICE_CONTRACT.md`: terminating block items, function exit, bounded match flow, ownership and source limits.
- `docs/P9_EXPLICIT_RETURN_SLICE_REPORT.md`: Phase A, W1/W2, destructive/rollback tests and exact-head review handoff.
- `docs/P13_IF_SOURCE_SEMANTIC_SLICE_CONTRACT.md`: finite if outcome/identity/dependency, durable ownership and precision contract.
- `docs/P13_IF_SOURCE_SEMANTIC_SLICE_REPORT.md`: Phase A, W1–W17, rollback/OOM, validation and review handoff.
- `docs/P12_STRUCTURAL_NAME_RESERVATION_REPORT.md`: admission audit/classification, parser ordering, namespace regression and rollback/OOM evidence.
- `docs/P11_FUNCTION_DECLARATION_SOURCE_CONTRACT.md`: bounded source/unit API and durable body ownership/precision contracts.
- `docs/P11_FUNCTION_DECLARATION_SOURCE_REPORT.md`: Phase A, break-test evidence and review handoff.
- `docs/P10_UNIT_NAME_RESERVATION_REPORT.md`: name-admission audit/rule, builtin/member preservation, rollback/OOM and review evidence.
- `docs/R4_01_ARGUMENT_COMPATIBILITY_FIX_REPORT.md`: shared exclusive/ordinary argument fix,
  canonical test corrections and targeted validation.
- `docs/`: historical [P0 toolchain decisions](docs/P0_TOOLCHAIN_DECISIONS.md),
  [P0 architecture](docs/P0_ARCHITECTURE.md), and [P0 report](docs/P0_REPORT.md).

There is still no full-program parser or semantic checker, typed MIR, LLVM lowering,
optimization framework, broad FFI, concurrency, separate compilation,
self-hosting, public token/AST-dump mode, or a full-program frontend. The module slice supports canonical receiving/block/aggregate forms, registered closed-sum constructor/match, bounded dependency-aware ref results and registered body calls with P9 explicit-return lexical blocks. Only the fn-only declaration-unit profile is supported; broader frontend forms remain unsupported. The CLI compile path remains unsupported.

## Local-root ptr/ref product gate

The Draft 17.18 Provisional read-only profile now checks real lexical loan bodies:

```newlang
fn main() -> unit {
    let x = u8(7);
    let p = loan_read(x) { |r| ptr_from_ref(r) };
    loan_read_ptr(p) { |r2| ptr_from_ref(r2); unit };
    unit
}
```

The ptr package is received after the first loan ends. Reacquisition requires
provenance matching the still-live source local incarnation and its implicit
stability authority; a ptr token alone provides no access authority. Returning
the scoped ref rejects with `P8-EXIT-DEPENDENCY`; reacquiring after the local
ends rejects with `P3-STALE-POINTER`. Both reject before C emission.

Only simple-name u8 local roots and local `ptr<u8>` receiving/use enter this
Checked-C gate. Ordinary local names remain admissible; language-wide policy
for same-spelling declarations/calls remains Provisional. Profile-external
loans and richer accepted checked results remain explicitly unsupported.
C addresses are execution representation, never the semantic safety oracle.
See [gate contract](docs/NORTH_STAR_LOCAL_ROOT_CONTRACT.md) and
[gate evidence](docs/NORTH_STAR_LOCAL_ROOT_REPORT.md). Field mutation, raw storage,
recursive Node, cJSON and later slices are not enabled by this gate.
