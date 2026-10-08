# Draft 17.24 — allocated Node native implementation report

Track: P / Issue #173. Base main:
`25c3fe260ce17b27d1d75b45004511a96ad9308d`; CURRENT_SPEC → Draft 17.24.
Branch: `draft-17-24-allocated-node-native`. Exact candidate SHA and PR-triggered
CI links are recorded in the Issue/PR handoff, avoiding an evidence-only commit
that would invalidate the tested head.

## Authorized prerequisite result

Coordination comment 6052322435 authorized the localized checked-operand fix.
`NL_CHECKED_REF_FROM_PTR` now preserves the selected ptr/stability argument
chain using existing Identifier/Copy evidence; the borrowed stability remains
scope-bound, with no consuming domain transfer. Actual-source regressions
distinguish `q` from `tail`, and an alternate `borrowed = stable` binder, while
the referent/incarnation/domain remain equal where appropriate. Registration
retains owned source/tree data after the original source/tree is destroyed.
Checker registration and actual-call malloc/realloc fault sweeps still pass.

Classification: **COMPILER-IMPLEMENTATION — operand provenance omission repaired**.
No semantic delta, new canonical ambiguity or new source/API decision. Historical
design audit: N/A faithful implementation, with DI-001/002/006/007/008/009 checked.

## Native result, separate from static safety

The unchanged `tests/fixtures/allocated_node_semantic.nl` now generates C17.
Its native success path uses a real malloc block as H, a separate lexical head,
actual Some link and independent Copy, selected Some ptr and explicit-domain
reloan, unlink, checked typed EndRoot, raw reclaim/domain finalization, and one
matching real free. Both checked allocation arms are emitted; controlled platform
NULL selects None without success grants/initialization/end/free.

Three evidence layers are exercised:

- Checked artifact: independently read scalar values and selected input symbols,
  conditional arm contexts, region/range/domain/current-incarnation evidence.
- Lowering: deterministic output, selected Identifier Copy temps feed reloan;
  no root-address substitution/source reparsing; private 24/8 and member-plan
  C assertions; separate affine Allocation/Storage/slot carriers.
- Execution: strict C17, NDEBUG-active read-only oracle and actual platform
  allocation/free, for success and failure; generated C without observation also
  executes. Six witnesses cover the unchanged source, renamed H/fields, changed
  scalar values, direct-tail ptr, alternate stability binder, and Option arm order.

The observer validates real values/identity rather than exit zero alone. Seven
focused generated-code corruptions independently fail observation: omitted
free, extra free request, omitted unlink, wrong tag, wrong Some pointer, wrong
initialized payload, and actual release before EndRoot. Observer and platform
interception are separate; neither repairs source ownership/topology. No freed
memory is read. Native sanitizers corroborate these tests; they do not decide
source safety. EndRoot for this all-Discardable H is a checked lifetime boundary,
not a C destructor or a runtime ownership proof.

## Static rejection and robustness

`allocated_node_source.integration` covers 22 source cases: canonical/renamed
positives, existing unsafe cleanup/domain/ref/stale-pointer/new-incarnation
controls, incorrect ptr/stability types, scoped ref escape, and a source-valid
extra-domain lifecycle or retained-Some link rejected explicitly by the bounded
backend (the native gate requires unlink, even though a dangling ptr token can be
source-valid). Semantic
failure returns code 3/empty stdout; backend unsupported returns code 4/empty
stdout. No C/object/executable is produced for rejected source. Existing scoped
escape reports through the prior `P3-INTERNAL` exit wrapper; diagnostic redesign
was not included. Programmatic raw/storage invalid-size/alignment/range/region/
occupancy tests are supporting evidence, not newly claimed source constructs.

New backend unit tests prove emitter OOM and malformed/missing ref operand/domain
evidence return no partial C, preserving caller length; restoring metadata yields
success. Existing transactional source registration/body checking fault injection
continues to pass. Fixed output/local/step/depth fences remain explicit. Richer
language-valid operations outside the admitted native profile remain unsupported.

## Reproduce / validation

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake -S . -B build-allocated -DCMAKE_BUILD_TYPE=Debug
cmake --build build-allocated --parallel 4
ctest --test-dir build-allocated --output-on-failure
bash scripts/check-format.sh
```

Repeat with GCC Release; Clang Debug; Clang Debug with
`-DNEWLANG_SANITIZER=address`; and Clang Debug with
`-DNEWLANG_SANITIZER=undefined`, using separate build directories. The native
integration test also instruments the **generated executable** with the selected
sanitizer. Ordinary full CTest includes prior V0/scalar/Pair/lexical Node native
tests, `oracle.adapter`, `oracle.smoke` and `artifacts.integrity` unchanged.

Local validation: **192/192** in each of GCC Debug, GCC Release/NDEBUG, Clang,
ASan and UBSan. Pinned bootstrap verified LLVM/Clang/clang-format 23.1.2;
local Debian 13 x86_64 GCC 14.2.0, CMake 3.31.6, Python 3.12.14. Existing CI uses
Ubuntu 24.04/GCC 13 and the same locked Clang/LLVM; no workflow/pin/oracle changes.
Review handoff is conditional on **all five PR-triggered jobs at the exact
candidate head** passing, including Clang format.

No new semantic/spec blocker found. These are P implementation/test results,
not independent Coordination ACCEPT or North Star PASS. No allocation/lifecycle
API expansion, FFI, second heap Node, detach, recursive delete, cJSON or next
gate was implemented. Stop for independent review with PR OPEN/unmerged:

`DRAFT 17.24 ALLOCATED NODE NATIVE LIFECYCLE READY FOR REVIEW`
