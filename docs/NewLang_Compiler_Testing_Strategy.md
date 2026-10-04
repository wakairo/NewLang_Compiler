# NewLang Compiler Testing Strategy

**Status:** production compiler testing policy  
**Applies to:** P1 and later  
**Authority:** Class I project policy; tests do not override normative NewLang semantics

## 0. Purpose

These are different questions:

```text
Does the production compiler match a reviewed Python-oracle semantic case?

Does this C compiler module satisfy its own software contract?
```

Oracle/differential testing does not replace unit testing.

The compiler is both a collection of testable software components and an implementation of
NewLang semantics.

## 1. Primary test layers

Use four primary layers:

```text
1. module unit tests
2. subsystem integration tests
3. oracle / semantic differential tests
4. end-to-end compiler tests
```

Sanitizers, negative tests, invariants, fuzzing, and performance tests cut across those layers.

Use the lowest layer that directly owns the property.

## 2. Module unit tests

A unit is normally the smallest meaningful compiler module or pure component with a stable
contract, not necessarily one function.

Examples:

```text
source buffer
lexer
token buffer
diagnostic sink
identifier/symbol table
parser cursor
small IR container
```

A stateful module test should normally:

```text
construct
 -> operate through module API
 -> observe contract-level result
 -> verify meaningful invariant-visible behavior
 -> deinitialize
```

Do not publish private fields solely for tests. Do not include production `.c` files from tests
to reach static internals.

## 3. New stateful module expectation

A new nontrivial stateful compiler module requires module-level unit tests unless the PR
documents a concrete reason that it is intrinsically integration-only.

As applicable test:

- normal success,
- empty/minimal input,
- boundary conditions,
- documented failure paths,
- init/deinit or create/destroy lifecycle,
- state after failure,
- ownership/consumption behavior observable via the API,
- important invariant-preserving transitions,
- determinism.

Not every trivial accessor requires its own test.

## 4. Pure-function tests

Nontrivial pure/stateless logic should receive focused tests when it has meaningful boundary
behavior: identifier classification, numeric parsing, escape decoding, source-span arithmetic,
hash/equality helpers, checked arithmetic, and similar code.

Do not create ceremonial tests for trivial forwarding wrappers.

## 5. Contract, not private layout

Avoid tests whose only assertion is an internal implementation choice such as vector capacity,
bucket count, or helper call count unless that detail is explicitly contractual.

A representation refactor that preserves the module contract should not rewrite most tests.

## 6. Testability is architecture

A stateful module should normally be constructible without initializing the entire compiler,
take small explicit dependencies, operate deterministically, expose meaningful results, and
release owned resources.

If it requires ambient global state, ask whether the dependency is too implicit.

Do not introduce a dependency-injection framework merely for tests.

## 7. Test layout

Prefer discoverable mappings such as:

```text
src/source.c
include/newlang/source.h
tests/unit/source_test.c
```

One module test binary may contain table-driven cases. Avoid one executable per assertion when it
adds build overhead without value.

## 8. Harness policy

Keep the C unit-test harness small and transparent. P0 already uses CTest as the top-level
runner.

Do not add a large third-party unit-test framework unless it has demonstrated benefit.

Any harness must fail reliably, work under GCC/Clang, run under ASan/UBSan, produce useful
failure context, and remain deterministic.

## 9. Subsystem integration tests

Use integration tests for contracts that genuinely span modules:

```text
source -> lexer
lexer -> parser
parser -> semantic representation
semantic checker -> diagnostics
checked representation -> backend boundary
```

Do not move a local module property to integration level merely because the full pipeline also
exercises it.

## 10. Oracle / semantic differential tests

The frozen M7.5 Python prototype is a behavioral oracle, not normative semantics.

Differential tests should have a reviewed normative basis and compare normalized semantic
dimensions such as:

```text
supported / unsupported
accepted / rejected
diagnostic category/code when semantically meaningful
selected semantic facts when an explicit adapter exists
```

Do not compare incidental implementation details.

A mismatch can be a production bug, oracle bug, formal extraction issue, normal implementation
difference, or normative ambiguity. Classify before changing behavior.

## 11. Oracle tests do not replace component tests

A lexer bug can leave accept/reject unchanged for many programs. An ownership bug may show only
under ASan. Parser recovery may fail while a semantic oracle only checks final acceptance.

Therefore:

```text
module tests      -> local software defects
integration       -> contract mismatches
oracle tests      -> semantic divergence
end-to-end        -> user-visible pipeline failures
```

## 12. End-to-end tests

As features arrive, exercise the compiler as a user would.

Verify stable user-visible contracts: exit status, artifact existence/type, semantic result,
diagnostic category where stable, and deterministic behavior.

Do not freeze exact diagnostic prose unnecessarily during early development.

## 13. Negative/destructive tests

Compiler correctness depends heavily on rejection.

Add focused negative tests for malformed/truncated input, unexpected EOF, duplicates, overflow,
out-of-range operations, allocation failure when testable, and invalid state transitions where
appropriate.

FormalProof counterexamples are useful sources of semantic destructive-test ideas. Lean encoding
details are not production requirements.

## 14. Ownership/lifetime tests

C modules with ownership should test failure cleanup, lifecycle correctness, documented borrowed
view validity, consumption/transfer behavior, and mutation invalidation rules.

Sanitizers supplement those tests but do not define API contracts.

## 15. Sanitizers

Ordinary C unit/integration tests should remain runnable under P0's required configurations:

```text
AddressSanitizer
UndefinedBehaviorSanitizer
```

A module is not adequately validated if its direct tests pass normally but fail required
sanitizer CI.

## 16. Determinism and isolation

Tests must not depend accidentally on heap addresses, hash randomization, filesystem enumeration,
wall-clock time, network access, uninitialized data, or process-global leftovers.

Each fixture should construct/destroy its own module state where practical.

Tests must not mutate frozen normative/reference/oracle artifacts.

## 17. Source fixtures

Keep fixtures small enough that one failure points toward one primary rule.

Large all-feature fixtures are useful for end-to-end smoke tests but are poor substitutes for
focused semantic tests.

## 18. Diagnostic testing

Separate:

```text
semantic rejection
diagnostic category/code
source attribution
human-facing prose
```

Early tests should prefer stable category/code/range assertions. Exact renderer formatting may
be tested by the diagnostics module because formatting is that module's contract.

## 19. Properties/invariants

Where practical test invariants, not only examples:

```text
insert then lookup returns equivalent value
init then deinit leaks nothing
token spans remain within source bounds
round-trip operations preserve contractual state
```

P1 does not need a property-testing framework; deterministic table/loop cases are sufficient.

## 20. Fuzzing

Lexer/parser/source-decoding are strong future fuzz candidates, but broad fuzz infrastructure is
not a P1 prerequisite.

First establish stable APIs and deterministic tests, then add narrow fuzz targets when useful.

## 21. Coverage

Do not optimize for a line-coverage percentage.

Coverage may expose obviously untested paths, but review asks whether contracts, failure paths,
boundaries, invariants, and semantic rules are tested.

## 22. Regression rule

A bug fix should normally add the smallest test that would have failed before the fix.

Choose the lowest appropriate layer:

```text
lexer bug          -> lexer unit
module seam        -> integration
semantic mismatch  -> oracle/semantic
CLI artifact bug   -> end-to-end
```

## 23. New-module PR expectation

For each meaningful stateful module identify:

```text
responsibility
public contract
state/invariant
ownership/lifetime
failure behavior
unit tests
integration tests added or deferred
```

This can be concise.

## 24. Test omission

A stateful module introduced without module-level unit tests requires explanation.

“Oracle tests pass” and “end-to-end tests happen to exercise it” are not sufficient reasons for
a nontrivial module.

## 25. Formatting/static hygiene

Mechanical formatting is not semantic testing, but it belongs in reproducible PR validation.

The repository provides:

```bash
bash scripts/format.sh        # automatic fix
bash scripts/check-format.sh  # non-modifying check
```

CI runs the check-only path using the exact pinned `clang-format-23`.

Formatting checks do not count as unit/integration/oracle coverage.

If a formatter policy change rewrites many files, isolate the mechanical diff rather than
mixing it casually into a semantic feature change.

## 26. CI expectations

P0 established GCC, Clang, ASan, and UBSan PR validation.

P1+ should keep new unit/integration tests in the normal CTest path so all configurations execute
them by default unless a documented platform-specific reason exists.

Do not create a separate test command that required CI silently omits.

Current-head CI is merge evidence.

## 27. FormalProof relationship

Formal proofs provide useful legal witnesses, negative lemmas, and semantic counterexamples.

Use them to derive production tests, but do not treat C tests as formal proofs and do not copy
proof-only runtime representations without an implementation reason.

## 28. Normative relationship

Tests are evidence about implementation, not the language definition.

If an old test conflicts with normative specification, classify the discrepancy and correct the
lower-authority artifact after adjudication.

## 29. Review gate

A feature PR is not ready merely because all existing tests pass.

Review also asks:

- is the owning module directly testable,
- are failure/lifecycle paths covered,
- are new seams integration-tested,
- is semantic oracle evidence present where appropriate,
- do sanitizer jobs execute the new code?

See `NewLang_Compiler_Review_Guidelines.md`.

## 30. Summary

Default shape:

```text
stateful module
    -> module unit tests

module seam
    -> subsystem integration tests

NewLang semantic behavior
    -> reviewed oracle/differential tests

user-visible behavior
    -> end-to-end tests

all ordinary C paths
    -> GCC + Clang + ASan + UBSan

mechanical style
    -> clang-format check
```

Test the smallest contract that actually owns the property, then add higher-level evidence where
the property crosses boundaries.
