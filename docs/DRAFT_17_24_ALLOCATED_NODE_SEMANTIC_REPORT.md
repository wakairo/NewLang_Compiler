# Draft 17.24 allocated Node semantic gate report

Track: P — #171; base main `38696f024166ffb4e0434df2719a00714bbc0764`.
Canonical: CURRENT_SPEC → Draft 17.24, accepted #169/#170 design.
Scope/implementation obligations: [contract](DRAFT_17_24_ALLOCATED_NODE_SEMANTIC_CONTRACT.md).
Semantic delta 0. No verified specification hole/ambiguity or required amendment.

## Reproducible evidence

`tests/fixtures/allocated_node_semantic.nl` wraps the canonical §3.2 schematic
in an actual ordinary main function. No seeded context claims or target pointers.

`allocated_node_evidence.unit` loads that source, registers it, destroys the
original syntax/source, calls its retained main body, and inspects owned checked
artifacts. It asserts exactly one common claim-free trial and two separately
owned outcome grants; None has zero regions/domains; Some has same-region
Allocation/Storage, target layout, Initialize/Destroy and explicit same-domain
reloan, deallocation, consumed owners and ended backing. The caller has no
arm-region/domain imported. Semantic/raw validators check the closed success
world.

`allocated_node_source.integration` exercises the canonical source and a renamed
H/link/sibling equivalent. Both check successfully and stop before emission.
Source negatives check None-body errors, missing None, Some wildcard/dropped
bundle, omitted deallocation/finalization/slot cleanup, double deallocation,
ordinary ending authority, wrong reloan/ending domain, conflicting loans, stale
ptr after destroy/deallocation and same-range fresh incarnation. All reject
before emission with structured NewLang diagnostics; no C/native artifact.

`allocated_node_failures.unit` sweeps malloc/realloc failure positions separately
through source-unit registration and actual main-body checking. Registration
failures leave no hidden types/functions/claims; retry succeeds. Call failures
leave the original type/function/value counts and zero caller bindings/backings/
domains, no artifact, and valid state. Existing raw-storage tests independently
cover invalid size/alignment, partial/wrong-region deallocation, occupancy overlap
and fault/resource rollback as **programmatic supporting evidence**, not actual
source tests involving a fabricated second allocation.

## Limitations and handoff

COMPILER-IMPLEMENTATION-LIMIT / COMPILER-PRECISION: direct consuming allocation
match, closed normal-unit worlds, one selected H/one allocation per world;
Linux x86_64 private target plan only. Richer conditional ownership joins,
allocation outcome storage/return and nested allocations are conservatively
unsupported. These are implementation bounds, not source-language invalidity.
No semantic safety is delegated to C or a runtime. Dynamic native execution
remains explicitly unsupported; no claim of an observed runtime allocation.

Existing oracle adapter, frozen oracle, artifact locks and native lexical Node
observer/corruption controls are unchanged. Independent Coordination review is
required; candidate remains OPEN/unmerged and no next slice is authorized.

## Validation

Commands: activate `.deps/activate.sh`, `cmake --build <build> --parallel 4`,
`ctest --test-dir <build> --output-on-failure`, `bash scripts/check-format.sh`.
Five configurations and exact-head CI results are recorded in the Issue/PR
handoff, avoiding a self-referential checked-in head SHA.
