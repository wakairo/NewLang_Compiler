# P466: existing initial Some + two-normal Node mutation composition

Test-only evidence against compiler/main
`55f4f0feaf5976648a97fbd7190bbf954ed6647e`, canonical Draft 17.30.
Sources, semantic expectations, hashes and stop conditions were committed in
`22e348fd36d9296aacee625d4831830eead67b84` before any candidate execution.
No source corrections were needed. Neither compiler nor adopted tests change.
This is finite compatibility evidence, **not Product GO**.

## Reproduce

Use the existing repository build instructions and locked toolchain. Alternatively,
for a NO LLVM / PARTIAL reproduction (no bootstrap download):

```sh
cmake -S . -B build-frontend -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-frontend --parallel 2
ctest --test-dir build-frontend -R '^p466_composition.integration$' -V
```

The new CTest takes the configured host compiler and unchanged sanitizer setting.
It checks the frozen hashes, calls `newlangc --check`, runs `p466_public_probe`,
then ordinary source-to-C17 CLI, then strict C17 O0/O2 plain and observed native.
The same unmodified emitted bytes are used for plain and observed builds.
The passive header reads actual original carrier addresses; it writes only its
own observation bookkeeping. Branch-local old Copy addresses are not retained.

## Local observations before hosted CI

Both sources: CHECK0/no diagnostic; public source-owned probe0; issuer view and
independent validator0; C17 CLI0/no diagnostic. Both arm records are validated,
including the statically checked arm not selected in that runtime input.

| source | C bytes | C SHA256 |
| --- | ---: | --- |
| some.nl | 2530 | `9981c13b1579d95f364b4be3c3948c8b320d5a8cce85bf63c379fb1260cc3367` |
| none.nl | 2491 | `901705433179e0631564380c608f6b527ff53e07ed4ef7d08dbf7749446634fa` |

Public original places/incarnations are anchor=1/1 and peer=6/6. Their governing
Domain is implicit local identity (public Domain ID0), BackingRegion0; these are
lexical Nodes, **not heap Allocation/LifetimeDomain/nonCopy custody evidence**.
Some input: field7 F7->F12, peer RF6->RF13, joined abstract Unknown; sibling9
unchanged. None arm F7->F13/RF6->RF14/occurrence1->3 (fresh Some even with same
pointer); Some arm F7->F14/RF6->RF15/occurrence1->0. All IDs are qualified by the
respective common-entry/arm/post world; equal numeric IDs do not unify worlds.
None control: joined F7->F10/RF6->RF11, sibling8 stable; arm None creates a fresh
Some occurrence, arm Some leaves None while refreshing both F and RF.
Copied witnesses with stale parent RF or forged old occurrence are rejected;
the original issued proof remains valid afterward.

GCC14.2.0 and Clang23.1.2 local native: each host, two sources, O0/O2, plain and
observed = 8 successful executions per host. Strict flags are
`-std=c17 -Wall -Wextra -Wpedantic -Werror`. Actual original addresses differ;
Some initial pointer/old Copy/reloan all target the original anchor. Some runtime
arm clears peer.next; None control runtime arm installs Some(original anchor).
Payloads29/61 and initial Option Copy remain intact. Machine addresses vary.
The single local NO LLVM/PARTIAL GCC preflight passed all 282/282 CTests in
55.24 seconds (281 existing + this composition test).
Raw public evidence and native observations are in evidence/*.json. These are
local unsanitized observations; hosted locked23.1.3 FULL/ASan/LSan/UBSan results
are separately reported in Issue466, not inferred from this local evidence.

A local test-target invocation initially preceded CMake regeneration and said
"No rule to make target"; regeneration resolved the build invocation. This was
not a compiler execution/diagnostic and did not change the frozen source/oracle.
There were no source rejection or observer repairs.

## Boundary and audit

Changes are new tests plus six CMake test-registration lines. No changes to
src/, include/, scripts/, docs/, workflows or existing test files. No new flag,
source rule, API, authority, public proof, runtime allocator or general mechanism.
Historical design gate N/A: existing selected semantics and existing validators.
One naturally triggered FULL + matching PARTIAL CI attempt is the budget;
no manual reruns. Any actual failure is reported rather than repaired in CI.
Goal alignment: this tests composability before a larger owner investment;
H1/STB/V product holds remain. Future investment/merge belongs to Coordination.
