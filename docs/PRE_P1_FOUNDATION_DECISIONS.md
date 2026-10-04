# Pre-P1 Foundation Decisions

**Status:** production compiler foundation policy before P1 implementation  
**Authority:** Class I implementation policy  
**Purpose:** fix only high-leverage engineering conventions that become expensive to change after
frontend modules proliferate

## 1. What is fixed before P1

The following project-wide foundations are adopted before substantial P1 code:

- NewLang-aware C implementation guidelines,
- compiler testing strategy,
- shared review guidelines,
- clang-format as the mechanical C/H formatter,
- canonical source spans as half-open byte offsets,
- explicit failure/ownership contracts for fallible C APIs,
- assertions reserved for compiler-internal invariant failures,
- ordinary checked C allocation before any general allocator framework exists.

Everything else remains demand-driven.

## 2. Mechanical formatting

`.clang-format` is the single source of truth for mechanical C/H layout.

The formatter is pinned with the reviewed LLVM 23.1.2 toolchain as
`clang-format-23`.

Developer/Codex workflow:

```bash
bash scripts/format.sh
bash scripts/check-format.sh
```

The first command may rewrite files. The second is non-modifying and is enforced by CI.

Reviewers should not spend review bandwidth on formatting already determined by the committed
formatter configuration.

A project-wide formatter rule change should be deliberate and reviewed.

## 3. Fallible C API convention

Do not create a universal Result framework before there is a demonstrated need.

Use:

- `bool` for true predicates and simple success/failure APIs whose output/error contract is
  unambiguous,
- a small operation-specific or project status enum when callers must distinguish materially
  different outcomes,
- structured diagnostics for invalid NewLang programs,
- assertions for violated compiler-internal invariants.

Every nontrivial fallible public/module API must document, as applicable:

```text
success:
    input ownership after return
    output validity/ownership
    module state

failure:
    input ownership after return
    which outputs are valid
    whether module remains reusable
    whether deinit is required/allowed
```

Do not use assertion failure as ordinary invalid-source handling.

Do not convert an impossible internal state into a misleading user diagnostic merely to keep
running.

## 4. Allocation and OOM

Start with ordinary C allocation where allocation is actually needed:

```text
malloc / calloc / realloc / free
```

subject to explicit ownership and checked failure behavior.

Do not introduce before evidence requires them:

- a universal allocator interface,
- AST arena framework,
- compiler-wide ownership runtime,
- generic region system,
- allocation macros that hide ownership.

Project-owned allocation failures must be checked. A NULL result must not flow into undefined
behavior.

The exact user-facing policy for fatal host-memory exhaustion can be chosen when the first
allocating production subsystem requires it, but cleanup/ownership must remain correct and the
compiler must not misclassify OOM as invalid NewLang source.

Later arenas/interning/scratch allocators should be justified by concrete lifetime or measured
performance needs.

## 5. Stateful modules

A stateful subsystem should normally have a module object whose responsibility, invariant,
ownership, failure behavior, and lifecycle are reviewable and directly unit-testable.

A pure/stateless operation does not need a ceremonial module object.

The first argument convention for method-like functions is preferred, not mandatory.

No giant mutable `NLCompilerContext` should be introduced merely to avoid passing explicit
dependencies.

## 6. Testing

For each nontrivial stateful compiler module:

```text
module-level unit tests required
    unless a concrete integration-only reason is documented
```

Oracle/differential tests complement but never replace module tests.

P1 should grow the test support from real module needs rather than adopting or building a large
unit-test framework in advance.

All ordinary C unit/integration tests remain in the normal CTest path and therefore run under:

- GCC,
- Clang,
- ASan,
- UBSan.

## 7. Source location

The canonical internal representation is:

```text
[source start byte, source end byte)
```

over preserved source bytes.

See `PRE_P1_SOURCE_INPUT_AND_LOCATION_CONTRACT.md`.

Line/column values are derived presentation data.

## 8. Explicit deferrals

Pre-P1 deliberately does **not** choose:

- final AST representation,
- checked IR/MIR representation,
- hand-written parser vs parser generator,
- symbol interning representation,
- arena allocator architecture,
- generic container library,
- clang-tidy/static-analysis policy beyond current warnings/sanitizers,
- fuzz infrastructure,
- performance budgets,
- stable diagnostic wording/numbering,
- incremental compilation,
- separate compilation,
- native Windows/macOS abstractions,
- thread-safety architecture.

These should be decided from concrete implementation pressure rather than anticipation.

## 9. Review intent

The three policy documents form a shared contract:

```text
NewLang_Aware_C_Guidelines.md
    -> implementation style and explicit contracts

NewLang_Compiler_Testing_Strategy.md
    -> validation layers and module unit testing

NewLang_Compiler_Review_Guidelines.md
    -> review severity, evidence, and shared expectations
```

The objective is to reduce predictable review churn while preserving freedom to revise
architecture when real evidence appears.

## 10. P1 readiness

P1 may begin once this foundation PR is reviewed and merged with current-head CI green.

P1 should then implement only its chosen vertical slice and the modules genuinely required by
that slice.
