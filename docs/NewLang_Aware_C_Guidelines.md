# NewLang-Aware C Guidelines

**Status:** production compiler implementation policy  
**Applies to:** P1 and later unless a milestone explicitly overrides it  
**Implementation baseline:** C17  
**Authority:** Class I project policy; not normative NewLang semantics

## 0. Purpose

The production compiler is intentionally written in C both as an implementation choice and as
a comparison surface for NewLang.

“NewLang-aware C” does not mean emulating NewLang mechanically in C. It means writing ordinary,
maintainable C with ownership, lifetime, mutation, module boundaries, and failure behavior
explicit enough that the implementation can pressure-test both languages.

The target is:

```text
good C implementation
+
visible ownership/lifetime/module contracts
+
useful evidence for NewLang design
```

Avoid creating a C-hosted pseudo-NewLang.

## 1. Explicit contracts over cleverness

At a meaningful module boundary a reviewer should be able to answer:

- who owns each object,
- which pointers are borrowed, transferred, consumed, or retained,
- what may mutate state,
- what state is valid after failure,
- what must be destroyed/released,
- what invariant the module maintains.

If those answers exist only as convention in the implementer's head, the interface is incomplete.

## 2. Stateful modules

A compiler subsystem that owns mutable state, resources, or a nontrivial invariant should
normally be represented as a module object plus functions operating on it.

Typical shape:

```c
typedef struct NLLexer NLLexer;

bool nl_lexer_init(NLLexer *lexer, NLSource source);
void nl_lexer_deinit(NLLexer *lexer);
NLToken nl_lexer_next(NLLexer *lexer);
```

Method-like operations should normally take the module object as the first argument.

This is a default convention, not a law. Pure/stateless functionality should remain ordinary
functions when that is clearer.

Do not introduce meaningless context structs merely to imitate object-oriented syntax.

## 3. Module boundaries localize invariants

Prefer modules whose public API validates inputs and maintains a small invariant.

Avoid exposing struct fields when doing so forces every caller to know and preserve hidden
invariants. Use opaque structs where representation hiding materially improves correctness or
changeability; do not make every trivial value opaque.

A module is justified by a real invariant, resource, lifecycle, or semantic responsibility.

## 4. Ownership vocabulary

Public and non-obvious internal APIs should use consistent ownership terms where ownership
matters:

```text
owned by caller
owned by module
borrowed for the call
borrowed until next mutation
transferred on success
consumed
retained
returned owned
returned borrowed
```

The exact contract may vary, but it must be explicit.

C `const` is useful where accurate, but it is not NewLang `ref<read,T>`, lifetime stability,
provenance, or semantic immutability.

## 5. Mutation and dependencies

Avoid mutable global state and ambient singleton contexts.

Prefer explicit module/dependency parameters. Do not introduce one giant mutable
`NLCompilerContext` simply to avoid parameter passing.

A larger context is appropriate only when it corresponds to a real ownership/lifetime boundary.

## 6. Initialization, destruction, and partial state

A stateful module should have a clear lifecycle:

```text
uninitialized
 -> init/create succeeds
valid module
 -> operations
deinit/destroy
```

An init/create API must define failure behavior. Prefer either:

- failure leaves a documented inert/deinit-safe state, or
- failure leaves the destination untouched/uninitialized and deinit is not allowed.

Do not leak partially initialized ownership across undocumented fields.

## 7. Fallible API contracts

Do not design only the success path.

For a nontrivial fallible API specify, as applicable:

```text
success:
    input ownership
    output validity/ownership
    resulting module state

failure:
    input ownership
    which outputs are valid
    whether the module remains reusable
    whether deinit is required/allowed
```

Use `bool` for predicates or simple success/failure contracts. Introduce a small status enum
when callers must distinguish materially different outcomes. Do not build a universal Result
framework before a real need exists.

Invalid NewLang source is an ordinary compiler error/diagnostic, not an assertion failure.

Assertions are for compiler-internal invariant violations.

## 8. Allocation

Start with ordinary checked C allocation where allocation is actually needed:

```text
malloc / calloc / realloc / free
```

Every allocation has one clear owner and matching release path. Allocation failure must be
checked and must not turn into a NULL dereference or other undefined behavior.

Do not introduce a universal allocator API, AST arena, scratch framework, or compiler-wide
ownership runtime before concrete lifetime/performance evidence justifies it.

OOM must not be misdiagnosed as invalid NewLang source.

## 9. Borrowed views

Borrowed string/source/token/array views must document the validity interval of their backing
storage and which operations invalidate them.

Do not return pointers into growable buffers without documenting the invalidation rule.

Stable identity should be provided by representation, not assumed from current allocation
behavior.

## 10. Representation boundaries

Public compiler interfaces should expose compiler concepts rather than accidental storage
choices.

LLVM handles belong behind LLVM-facing backend boundaries. LLVM representation choices must not
leak backward into frontend semantics merely for convenience.

Likewise, Lean proof-only representations are evidence, not required C runtime structures.

## 11. No macro-hosted pseudo-language

Use macros for narrow conventional C purposes only.

Avoid macro systems that hide:

- ownership transfer,
- cleanup/control flow,
- object systems,
- constructors/destructors,
- evaluation order,
- exception-like behavior.

If a mechanism is important enough to need a private language layer, first ask whether normal C
functions and data make it clearer.

## 12. Cleanup control flow

A single `goto cleanup` path is acceptable when it makes incremental resource acquisition and
release auditable.

Avoid duplicated cleanup and hidden cleanup macros.

Correct, locally understandable ownership release is more important than avoiding `goto`
categorically.

## 13. Integer and indexing discipline

Use types that reflect the domain. `size_t` is the default for source byte lengths/counts;
fixed-width types are appropriate for external fixed-width representations.

Check arithmetic when overflow would affect memory safety or compiler state. Do not rely on
signed overflow or other C undefined behavior.

## 14. Tagged state

Represent materially distinct semantic states explicitly rather than accumulating sentinel
conventions when those conventions permit invalid combinations.

Do not turn every small enum into a framework.

## 15. Assertions versus diagnostics

Use assertions for impossible internal states and implementation defects.

Use ordinary status/diagnostic handling for invalid user programs and expected external
failures.

Do not crash merely because source is invalid, and do not hide compiler bugs as user errors.

## 16. Determinism

Compiler behavior should be deterministic unless nondeterminism is deliberate.

Diagnostics ordering, token/AST ordering, generated representations, and tests must not depend
accidentally on pointer addresses, uninitialized bytes, filesystem iteration, or hidden global
state.

## 17. Source representation foundation

Source ingestion preserves input bytes exactly. Canonical frontend locations are half-open byte
spans over those preserved bytes.

See `PRE_P1_SOURCE_INPUT_AND_LOCATION_CONTRACT.md`.

Do not make line/column coordinates the sole semantic source identity and do not depend on
C-string termination for source contents.

## 18. NewLang-aware observations

When implementation friction matters, distinguish:

### C-specific friction

Example: manual cleanup bookkeeping caused by C's weak ownership tracking.

### Language-independent compiler engineering

Example: a lexer needs a clear source-buffer lifetime regardless of implementation language.

### Possible NewLang design pressure

Example: a common systems/compiler pattern is unexpectedly awkward under current NewLang rules.

Minimize and classify real semantic/lowering/portability/performance pressure. Do not redesign
NewLang from an anecdote.

## 19. Intentional divergence is allowed

C code should not be contorted merely to resemble NewLang.

Local mutation, cleanup `goto`, host-specific defensive checks, and other idiomatic C choices
are acceptable where clearer.

The objective is comparison, not mimicry.

## 20. Module testability

A new nontrivial stateful compiler module should normally be testable through its meaningful
module API without running the whole compiler.

Its dependencies should be explicit enough to construct in a test; teardown should be possible;
success/failure should be observable through contract-level results.

Do not expose private fields solely for tests and do not include production `.c` files from
tests to bypass module boundaries.

If a stateful module cannot be unit-tested without the whole compiler, treat that as an
architecture smell unless it is intrinsically an integration boundary.

## 21. Mechanical formatting is delegated to clang-format

Mechanical C/H formatting is not a NewLang-aware semantic design question.

The committed `.clang-format` is the single source of truth for indentation, braces, pointer
spacing, wrapping, initializer layout, and similar mechanical formatting.

The formatter version is pinned with the project toolchain. With LLVM 23.1.2 the formatter is
`clang-format-23` 23.1.2.

Expected workflows:

```bash
bash scripts/format.sh
bash scripts/check-format.sh
```

The first automatically rewrites tracked C/H files. The second is non-modifying and CI-enforced.

The responsibility split is:

```text
mechanical whitespace/layout
    -> clang-format + .clang-format + CI

API / ownership / lifetime / module structure
    -> NewLang-aware C policy + review
```

Reviewers should not request hand formatting that conflicts with formatter output.

If formatter output should change project-wide, change `.clang-format` deliberately and review
that policy change. Avoid broad `clang-format off/on` regions.

## 22. Naming and API consistency

Use stable project prefixes such as `NL` / `nl_` where appropriate. Prefer names that make
module ownership and semantic role clear.

Consistency matters more than forcing every API into one rigid signature pattern.

## 23. Review expectation

A meaningful stateful module PR should make these reviewable:

```text
module responsibility
state/invariant
ownership/lifetime
failure behavior
unit-test coverage
integration boundary
```

Clear headers, tests, and concise PR notes are sufficient when the design is simple.

See `NewLang_Compiler_Review_Guidelines.md`.

## 24. Anti-goals

NewLang-aware C is not an excuse for:

- speculative framework construction,
- excessive wrappers,
- making every function a method,
- eliminating all local mutation,
- mirroring Lean ghost state,
- NewLang-like macros,
- freezing future NewLang semantics into C architecture,
- treating C coding convention as normative NewLang behavior.

## 25. Summary

Prefer:

```text
small explicit modules
clear state and invariants
visible ownership
local mutation
explicit failure behavior
testable boundaries
minimal hidden mutable global state
ordinary readable C
mechanical formatting delegated to clang-format
```

while continuously asking:

```text
Which difficulty is C-specific?
Which is compiler architecture?
Which is genuine evidence about NewLang?
```
