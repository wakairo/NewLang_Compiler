# NewLang Production Compiler Bootstrap Charter

**Status:** Production compiler track bootstrap charter  
**Scope:** P0 — Production Compiler Bootstrap  
**Language implementation:** C  
**Primary backend:** LLVM  
**Normative language source:** `NewLang_v0_spec_Draft17_4.md`  
**Relationship to other tracks:** Independent sibling of the semantic milestone track and formal-proof track

---

## 0. Purpose of this charter

This document defines the operating rules for beginning the NewLang production compiler implementation.

Its purpose is not to add or reinterpret NewLang language semantics. It exists to keep the production implementation disciplined while the language specification, formal proof, and compiler evolve in parallel.

The production compiler must be treated as one implementation of the normative NewLang specification, not as the source of truth for the language.

The initial production compiler is intentionally written in **C**, with the implementation style chosen to make comparison between C and NewLang useful during development and to preserve a plausible later migration/self-hosting path.

The primary backend is **LLVM**.

---

# 1. Project position

NewLang development now has three parallel tracks.

```text
Semantic / language track
    Draft 17.4 / M7 CLOSED
    M8+ continues language/API refinement

Formal proof track
    F0.x semantic-kernel proofs

Production compiler track
    P0 Production Compiler Bootstrap
    P1+ implementation milestones
```

These tracks are related but none is automatically authoritative over the others.

The intended relationship is:

```text
                 normative specification
                         |
            +------------+------------+
            |                         |
      reference/oracle           formal model
      implementation             and proofs
            |                         |
            +------------+------------+
                         |
                production compiler
```

If the specification, reference prototype, formalization, and production compiler disagree, do not silently choose one implementation as correct. Classify and adjudicate the discrepancy.

---

# 2. Authority hierarchy

The production compiler must use the following precedence order.

## 2.1 Normative source

`NewLang_v0_spec_Draft17_4.md`

This is the source of truth for language semantics covered by Draft 17.4.

## 2.2 Normative backend contract

`NewLang_Backend_Contract_v0_4.md`

This constrains lowering and backend behavior where the frontend/backend boundary has already been closed by M7.

## 2.3 Adjudication and closure records

`NewLang_M7_Adjudication_Resolution.md`

`NewLang_M7_Backend_Interop_Consolidation_REPORT.md`

These explain why decisions were made and what was intentionally Deferred or implementation-later. They must not override the normative specification or backend contract.

## 2.4 Reference/oracle implementation

The M7.5 Python prototype and its tests are a behavioral oracle and regression source.

They are not normative language specifications.

If the prototype disagrees with Draft 17.4, treat that as a possible implementation defect or specification ambiguity and report it.

## 2.5 Formalization

The F0 formal-kernel documents and `NewLang_FormalProof` repository provide machine-checked evidence for selected semantic properties.

Formal encoding choices are non-normative unless explicitly reflected in the normative specification.

Do not import proof-only representations such as ghost history sets into the production runtime/compiler merely because Lean uses them.

---

# 3. P0 objective

P0 is **not** the milestone in which the NewLang production compiler becomes feature-complete.

P0 establishes a trustworthy, reproducible production compiler development environment and the smallest executable compiler skeleton needed for later implementation.

P0 should prove that we can repeatedly perform the following loop:

```text
checkout
  -> bootstrap pinned toolchain
  -> build compiler written in C
  -> run unit/smoke tests
  -> exercise LLVM integration
  -> run sanitizers/static checks
  -> compare selected behavior with the reference/oracle
  -> run CI
```

P0 should make later language implementation safe and reviewable.

---

# 4. P0 non-goals

P0 must not prematurely implement large parts of the language.

The following are explicit non-goals for P0 unless required only as tiny stubs for infrastructure validation:

- full lexer
- full parser
- full type checker
- full semantic dependency analysis
- full ownership/value-flow checker
- generic instantiation
- aggregate/sum implementation
- dynamic container bridge
- full ABI classifier
- persistent function pointers
- broad normative FFI source surface
- retained/async callbacks
- foreign unwind/non-local transfer
- concurrency
- separate compilation
- optimization framework
- self-hosting
- cross-compilation matrix
- embedded/bare-metal target support
- stable compiler plugin architecture

P0 must not implement M8+ semantic ideas that are not yet normative.

---

# 5. Initial implementation language and coding policy

The production compiler implementation language is **C**.

The C code should be written in a deliberately disciplined style informed by NewLang design work.

This does not mean attempting to simulate NewLang mechanically in C. It means using the compiler implementation itself as pressure on both languages.

Prefer:

- explicit ownership of heap allocations
- explicit lifetime comments/contracts at subsystem boundaries
- clear distinction between borrowed and owned pointers
- immutable-by-default local design where practical
- small data structures with obvious invariants
- explicit error paths
- no hidden global mutable state unless justified
- narrow interfaces between frontend, semantic analysis, IR, and backend

Avoid:

- macro-heavy pseudo-language layers
- implicit ownership conventions that are not documented
- ad-hoc pointer aliasing assumptions
- large mutable singleton contexts unless architecture requires them
- C tricks that obscure the semantic correspondence being investigated

Do not force NewLang semantics into C where doing so makes the implementation unnatural. The comparison itself is useful evidence.

---

# 6. Initial host and portability policy

P0 primary host:

```text
Linux x86_64
Codex Cloud
```

Secondary validation host:

```text
WSL2 Ubuntu 24.04
```

The compiler implementation should remain portable C where reasonable.

P0 CI should validate host compilation with both:

```text
GCC
Clang
```

Do not assume compiler-specific extensions without documenting and isolating them.

Exact C language standard and build-system choice may be finalized during P0 after a small comparison, but once selected they must be pinned/documented and changed deliberately.

---

# 7. LLVM policy

LLVM is the primary backend.

The source semantics must remain LLVM-independent.

LLVM attributes, alias metadata, lifetime markers, captures, `noalias`, `readonly`, and related optimization facts may only be emitted when established by NewLang semantics/backend analysis.

Do not infer source-language rules from what LLVM would optimize well.

M7 backend conclusions remain binding for the initial implementation, including:

- `ref<write,T>` is not automatically LLVM `noalias`
- source-level exclusivity does not automatically imply backend `noalias`
- nonescaping source references do not automatically imply captureless behavior if persistent locator export is possible
- NewLang semantic lifetime is not identical to LLVM lifetime markers
- alias scopes require proven disjointness

The exact LLVM version should be pinned during P0 based on reproducibility in Codex Cloud and WSL2 and suitability of the LLVM C API.

Do not select an unpinned "latest LLVM" as an ongoing development dependency.

---

# 8. Frontend/backend architecture rule

The production compiler should preserve a visible semantic boundary between source-language analysis and LLVM lowering.

The intended high-level shape is:

```text
source
  -> syntax / AST
  -> semantic checking
  -> checked semantic IR / MIR
  -> backend contract
  -> LLVM lowering
```

The exact IR layering is not fixed by this charter.

However, LLVM-specific concepts should not leak backward into source semantics merely to simplify code generation.

Likewise, FFI ABI classification must remain distinct from NewLang semantic summaries.

---

# 9. Reference/oracle strategy

The M7.5 Python prototype remains useful as a reference/oracle during early production implementation.

P0 should establish a test harness capable of later differential validation such as:

```text
same NewLang input / semantic fixture
        |
   +----+----+
   |         |
Python      C production compiler
oracle      implementation
   |         |
   +----compare----+
```

P0 does not need to reproduce every Python prototype feature.

The important requirement is that the repository has a clean place and mechanism for oracle-backed tests before implementation volume grows.

A discrepancy must be classified rather than automatically treating the Python result as correct.

---

# 10. Formal-proof relationship

The production compiler and `NewLang_FormalProof` are sibling implementations of the specification.

The production compiler should use formal results as:

- design checks
- regression invariants
- guidance for targeted semantic tests
- evidence when adjudicating tricky transitions

It must not copy proof-only encoding details into runtime/compiler data structures without an independent implementation reason.

For example, a proof ghost structure such as historical identity sets may establish freshness properties without requiring the production compiler to retain unbounded runtime history.

Future compiler correctness proofs may connect production IR to the formal model, but this is not a P0 requirement.

---

# 11. Validation requirements

P0 should establish the following validation layers.

## 11.1 Host compiler warnings

Build with a strict warning profile using both GCC and Clang.

The exact flag set should be documented and kept practical. A likely baseline includes equivalents of:

```text
-Wall
-Wextra
-Wpedantic
-Werror
```

Additional useful warnings may be enabled after confirming portability.

## 11.2 Sanitizers

At minimum establish development/CI configurations for:

- AddressSanitizer
- UndefinedBehaviorSanitizer

Sanitizer support may be compiler/platform conditional, but the CI policy must be explicit.

## 11.3 Tests

P0 should establish:

- C unit tests for infrastructure
- compiler CLI smoke tests
- LLVM integration smoke test
- oracle/differential test harness skeleton
- deterministic test invocation

## 11.4 CI

PR-triggered CI is required.

The preferred workflow is:

```text
feature branch
  -> PR
  -> GCC build/test
  -> Clang build/test
  -> sanitizer checks
  -> oracle/integration smoke tests
  -> review
  -> merge only after approval
```

Codex must not merge its own production compiler PRs unless explicitly instructed later.

---

# 12. Reproducibility

The production compiler repository must document and pin the parts of the toolchain that materially affect reproducibility.

At minimum record:

- host OS baseline used in CI
- GCC version policy
- Clang version policy
- LLVM version
- build-system version/policy where relevant
- Python version if the oracle harness uses Python
- external dependencies and revisions

Prefer dependency mechanisms that allow a fresh Codex Cloud environment to reproduce the build without relying on accidental preinstalled state.

Do not update dependency pins implicitly during ordinary builds.

---

# 13. P0 repository architecture

The exact layout may change during bootstrap, but a small initial structure should look roughly like:

```text
NewLang_Compiler/
├── README.md
├── LICENSE / project metadata as appropriate
├── docs/
│   ├── NewLang_Production_Compiler_Bootstrap_Charter.md
│   ├── NewLang_Production_Compiler_Handoff_Manifest.md
│   └── ...
├── include/
├── src/
│   ├── main.c
│   ├── diagnostic.c
│   └── ...
├── tests/
│   ├── unit/
│   ├── integration/
│   └── oracle/
├── tools/ or scripts/
└── .github/workflows/
```

Do not create a deep architecture before code requires it.

P0 should prefer a few clear modules over a speculative full compiler directory hierarchy.

---

# 14. Minimum P0 executable capability

At the end of P0, the production compiler should minimally have:

- a C executable built reproducibly
- stable `--version` and/or equivalent smoke CLI
- diagnostics infrastructure sufficient for later compiler errors
- unit-test harness
- integration-test harness
- minimal LLVM link/API smoke test
- oracle harness connection or executable placeholder demonstrating the intended differential workflow
- GCC build
- Clang build
- ASan/UBSan validation where supported
- PR-triggered CI

A full parser or source-to-object pipeline is not required for P0.

---

# 15. Error and diagnostic policy

Diagnostics are a first-class compiler subsystem, but P0 should only establish the infrastructure.

Prefer diagnostics that can carry structured information such as:

- source location/range
- stable diagnostic category/code if useful
- primary message
- optional notes

Do not freeze final user-facing diagnostic wording in P0.

Do not entangle semantic analysis with direct printing to stderr if a small structured diagnostic representation can avoid it.

---

# 16. Compiler issue classification

When implementation pressure exposes a problem, classify it before changing semantics.

Use the following labels in notes/reports/issues.

## `COMPILER-SPEC-HOLE`

The normative specification lacks a rule required for implementation or permits contradictory outcomes.

Do not silently fill the hole in compiler code.

## `COMPILER-SPEC-AMBIGUITY`

Multiple materially different implementations are consistent with the current normative wording.

Report the minimal ambiguity and candidate resolutions.

## `COMPILER-LOWERING`

The source semantics are clear, but mapping them to checked IR/backend/LLVM requires a design choice.

## `COMPILER-IMPLEMENTATION`

Ordinary implementation defect or engineering decision that does not affect NewLang semantics.

## `COMPILER-DIAGNOSTIC`

Semantic behavior is clear but user-facing explanation, source attribution, or recovery is inadequate.

## `COMPILER-PERFORMANCE`

Correct implementation causes unacceptable compile-time, memory, code-size, or runtime cost.

## `COMPILER-PORTABILITY`

Behavior depends unexpectedly on host compiler, OS, LLVM version, target ABI, or toolchain environment.

These labels are implementation-track classifications, not language-spec status by themselves.

---

# 17. Rules for specification pressure

If implementation discovers a possible semantic problem:

1. Minimize the reproducer.
2. Identify the exact normative sections involved.
3. Determine whether the Python oracle and formal model agree or disagree.
4. Classify the issue.
5. Do not modify Draft 17.4 inside the compiler task unless explicitly authorized.
6. Report the issue back to the semantic track for adjudication.

Implementation convenience is not sufficient reason to change the language.

Conversely, if a semantically elegant rule causes disproportionate compiler complexity, record that as valuable evidence for M8+ review rather than hiding the cost.

---

# 18. M7 closure constraints carried into production

M7 is considered closed.

The production compiler may rely on the following closure outcomes without reopening M7 by default:

- source semantics remain LLVM-independent
- backend proof obligations are separate from source syntax categories
- native aggregate/sum layout remains opaque
- native aggregate types do not automatically acquire C ABI layout identity
- explicit C compatibility representation/marshalling is the intended boundary
- target ABI classification is backend-owned
- generated C shims are an acceptable initial fallback for difficult ABI classification
- foreign signatures/calling conventions are not semantic summaries
- missing semantic facts fall back conservatively rather than being guessed
- persistent function pointer API remains Deferred
- broad normative FFI source surface remains Deferred
- general external/static backing ergonomic API remains Deferred
- retained async callbacks and foreign nonlocal unwind remain Deferred
- direct C aggregate ABI lowering and aggregate varargs optimization are implementation-later

If production work exposes evidence that one of these closure decisions is untenable, report it as a new implementation pressure rather than silently reopening the design.

---

# 19. P0 review gate

P0 should stop for review once the development environment and minimal compiler skeleton are reproducible and green.

Do not automatically continue into substantial language implementation.

Before P1, review at least:

- repository structure
- build-system choice
- C coding discipline
- LLVM version and integration strategy
- diagnostic architecture
- test/oracle architecture
- CI matrix
- dependency pinning
- any COMPILER-* findings

P1 scope should be chosen only after this review.

---

# 20. P0 completion criteria

P0 is complete when all of the following are true:

```text
A. Dedicated production compiler repository exists.
B. This charter and the handoff manifest are committed.
C. Normative/non-normative source hierarchy is documented.
D. Compiler implementation is in C.
E. Fresh Codex Cloud environment can reproduce the build.
F. Exact LLVM version/policy is documented and pinned appropriately.
G. Compiler skeleton builds with GCC.
H. Compiler skeleton builds with Clang.
I. Strict warning policy is enabled.
J. ASan/UBSan configuration exists and passes where supported.
K. Minimal compiler executable/CLI works.
L. Minimal diagnostic infrastructure exists.
M. Minimal LLVM integration smoke test passes.
N. Unit/integration test harness exists.
O. Oracle/differential-test path exists.
P. PR-triggered CI is green.
Q. Codex works through branch + PR and does not self-merge.
R. No M8+ semantics are invented or implemented speculatively.
S. No unresolved COMPILER-SPEC-HOLE / AMBIGUITY is silently encoded.
T. P0 stops for architecture review before substantial P1 implementation.
```

---

# 21. Guiding principle

The production compiler has two jobs.

First, it must eventually compile NewLang correctly.

Second, during v0 design, it must act as an engineering pressure test on the language.

Therefore the preferred response to implementation difficulty is neither:

```text
"the specification must be right, make the compiler more complicated"
```

nor:

```text
"the compiler is hard, simplify the language immediately"
```

but:

```text
make the difficulty explicit
classify it
measure it
compare against the prototype/formal model
feed the evidence back into the semantic track
```

That feedback loop is part of the NewLang design process.
