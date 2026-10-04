# NewLang Production Compiler Handoff Manifest

**Purpose:** Define the input artifacts, their authority, and the reading/usage rules for Codex tasks working on the NewLang production compiler.  
**Initial compiler milestone:** P0 — Production Compiler Bootstrap  
**Implementation language:** C  
**Primary backend:** LLVM

---

## 0. Why this manifest exists

The NewLang project now has multiple kinds of artifacts:

- normative specification
- backend contract
- adjudication records
- historical research/closure reports
- Python prototype/oracle
- prototype tests
- formalization bridge documents
- Lean proofs
- production compiler source

These artifacts do not have equal authority.

Codex must not infer authority from recency, file size, amount of detail, or whether an artifact contains executable code.

This manifest defines how they are to be interpreted.

---

# 1. Authority classes

Use the following classes.

## Class N — Normative

Defines required NewLang semantics or an explicitly normative compiler/backend contract.

Conflict rule: Class N overrides all lower classes.

## Class A — Adjudication / closure

Records decisions, rationale, rejected alternatives, Deferred items, and milestone closure.

Useful for interpretation, but does not override Class N text.

## Class O — Oracle / reference implementation

Executable reference behavior and regression corpus.

Useful for differential testing. Not normative.

## Class F — Formalization

Machine-oriented semantic extraction and proofs.

Useful for invariants and counterexamples. Encoding choices are non-normative unless promoted into Class N.

## Class I — Production implementation

The current C compiler implementation, tests, and implementation notes.

Must conform to Class N. Existing code is not specification.

## Class H — Historical/supporting

Research, experiments, benchmark results, and contextual material.

Useful evidence only.

---

# 2. Required initial input set

The following artifacts should be available to the initial production compiler Codex environment/task.

## 2.1 `NewLang_v0_spec_Draft17_4.md`

**Class:** N — Normative  
**Role:** Primary language semantic source of truth.

Rules:

- Read this before implementing semantic behavior.
- Do not modify it in ordinary production compiler tasks.
- If implementation exposes a hole or ambiguity, report it rather than silently editing the specification.

---

## 2.2 `NewLang_Backend_Contract_v0_4.md`

**Class:** N — Normative backend contract  
**Role:** Defines backend/lowering constraints established by M7.

Rules:

- Treat this as authoritative for frontend/backend separation and LLVM lowering where covered.
- Do not add stronger LLVM assumptions simply because they optimize better.

---

## 2.3 `NewLang_M7_Adjudication_Resolution.md`

**Class:** A — Adjudication  
**Role:** Records accepted/rejected/deferred M7 decisions.

Rules:

- Use it to understand intent and avoid reopening closed questions accidentally.
- If its explanatory prose appears to conflict with Class N, stop and report the conflict.

---

## 2.4 `NewLang_M7_Backend_Interop_Consolidation_REPORT.md`

**Class:** A/H — Closure report / evidence  
**Role:** Explains M7 pressure tests, backend/interop conclusions, and Deferred scope.

Rules:

- Use as rationale and test-planning evidence.
- Do not copy implementation details into normative semantics automatically.

---

## 2.5 M7.5 Python prototype

**Class:** O — Oracle/reference  
**Role:** Behavioral oracle and source of known-good implementation examples.

Rules:

- Keep it operational where practical.
- Use it for differential tests.
- A mismatch with the production compiler is not automatically a production bug; classify first.
- A mismatch with Draft 17.4 means the prototype must not override the specification.

Recommended artifact/bundle:

```text
NewLang_FrontEnd_Prototype_M7_5.zip
```

or its unpacked repository-equivalent content.

---

## 2.6 M7.5 tests / semantic corpus

**Class:** O  
**Role:** Regression and differential test corpus.

Rules:

- Preserve test intent where moving/adapting tests into the production compiler repository.
- Do not assume every Python-test implementation detail is a language rule.

Known M7.5 closure baseline:

```text
466 / 466 tests PASS
```

This baseline is historical evidence for the reference prototype, not a claim that P0 must immediately reproduce all 466 behaviors.

---

## 2.7 `NewLang_F0_Formal_Kernel_Specification_Draft0.md` or current formal bridge

**Class:** F — Formalization bridge  
**Role:** Explicit extraction of selected semantic kernel concepts into proof-oriented form.

Rules:

- Useful for understanding precise distinctions such as surviving dependency, current-value fact, occupancy, and freshness.
- It is non-normative.
- If it conflicts with Draft 17.4, Draft 17.4 wins and the formalization discrepancy must be reported.

---

## 2.8 `NewLang_FormalProof` repository

**Class:** F — Machine-checked formalization  
**Role:** Selected machine-checked invariants and destructive tests.

Repository:

```text
https://github.com/wakairo/NewLang_FormalProof
```

Rules:

- Use theorem statements and counterexamples as semantic test guidance.
- Do not copy Lean proof-state representations directly into production data structures without an implementation reason.
- Formal proofs do not automatically prove production compiler correctness.

At production bootstrap time, formal proof work is a parallel track and may continue advancing independently.

---

## 2.9 `NewLang_Production_Compiler_Bootstrap_Charter.md`

**Class:** I-policy  
**Role:** Operational policy for the production compiler project.

Rules:

- Read this before creating the repository architecture or build environment.
- It governs P0 scope and production-development discipline.
- It does not override normative NewLang semantics.

---

# 3. Optional supporting artifacts

These may be supplied when useful but are not required to bootstrap P0.

Examples:

- earlier Draft 17.x versions for history
- M7 intermediate experiment logs
- Deep Research reports
- language-comparison research
- LLVM experiments
- ABI probe programs
- prototype performance measurements

**Class:** H unless explicitly marked otherwise.

Do not allow older drafts to override Draft 17.4.

---

# 4. Explicit conflict-resolution procedure

When two artifacts disagree, use this procedure.

## Case 1 — Normative vs non-normative

Follow normative text.

Create a note/issue describing the stale lower-authority artifact if the disagreement could cause future confusion.

## Case 2 — Normative vs normative

Do not guess.

Classify as:

```text
COMPILER-SPEC-HOLE
or
COMPILER-SPEC-AMBIGUITY
```

Provide:

- exact files/sections
- minimal example
- observable implementation difference
- possible resolutions

Stop only the affected semantic feature, not unrelated P0 infrastructure.

## Case 3 — Prototype vs formal proof

Check both against the normative specification.

Possible outcomes include:

- prototype bug
- formal extraction bug
- both are acceptable because they encode different non-normative details
- normative ambiguity

Do not pick the executable implementation merely because it runs.

## Case 4 — Production implementation vs any higher-authority artifact

Treat production code as suspect until shown otherwise.

Do not update the specification merely to preserve existing production code.

---

# 5. M7 closure status for production work

M7 is **closed**.

Production compiler P0 may begin without waiting for M8.

The production compiler track is intentionally parallel to M8+ semantic/API work.

P0 must limit itself to stable M7-era foundations and infrastructure. It must not infer that beginning production implementation freezes all future language semantics.

The following M7 statuses must be respected:

## Closed/usable foundations

- Draft 17.4 semantic baseline
- backend/frontend semantic separation
- LLVM lowering constraints in Backend Contract v0.4
- opaque native aggregate/sum layout policy
- generated C shim as acceptable initial ABI fallback
- semantic summary != C/foreign signature
- conservative fallback for missing facts

## Deferred from v0 core/API

- persistent function pointer
- broad normative FFI source surface
- general external/static backing ergonomic API
- retained async callbacks
- foreign unwind / non-local transfer

## Implementation-later

- direct target-specific C aggregate ABI lowering where a generated C shim suffices initially
- aggregate varargs optimization

P0 should not reopen or implement Deferred features unless explicitly requested.

---

# 6. What Codex should read first

For P0 setup, recommended reading order:

```text
1. NewLang_Production_Compiler_Bootstrap_Charter.md
2. this Handoff Manifest
3. NewLang_v0_spec_Draft17_4.md
4. NewLang_Backend_Contract_v0_4.md
5. NewLang_M7_Adjudication_Resolution.md
6. NewLang_M7_Backend_Interop_Consolidation_REPORT.md
7. inspect M7.5 prototype/test structure
8. inspect formal proof repository only as needed for invariant/test ideas
```

Do not require Codex to fully understand every future/deferred language feature before P0 repository setup.

---

# 7. What should be copied into the production compiler repository

Recommended to commit under `docs/reference/` or equivalent:

- this Bootstrap Charter
- this Handoff Manifest
- a pinned copy or clearly versioned reference to Draft 17.4
- Backend Contract v0.4
- M7 adjudication/closure documents if repository size/policy allows

For the Python oracle, either:

- vendor a clearly identified frozen snapshot under an `oracle/` area, or
- use a pinned external repository/submodule/archive mechanism

The choice should be explicit and reproducible.

Do not silently copy the formal-proof repository into the compiler repository. Prefer linking/pinning it as a sibling evidence source unless direct test generation later requires otherwise.

---

# 8. Artifact immutability and updates

Pinned reference documents should not be casually edited inside production tasks.

When a new normative draft supersedes Draft 17.4:

1. add/update the normative artifact deliberately
2. record the old/new version transition
3. identify affected compiler behavior/tests
4. update oracle expectations if needed
5. update formalization references separately

Do not rewrite historical closure reports to make them look current.

---

# 9. Production compiler report expectations

Each production milestone/PR should state which input artifacts it relied on.

For semantic changes, the PR/report should include a short section like:

```text
Normative basis:
- Draft 17.4 §...
- Backend Contract v0.4 §...

Oracle evidence:
- prototype test ...

Formal evidence, if used:
- theorem ...

Implementation decision:
- ...
```

This makes later adjudication possible without reconstructing hidden context.

---

# 10. Initial repository setup checklist

Before substantial compiler code is written, verify:

```text
[ ] Charter committed
[ ] Handoff manifest committed
[ ] Draft 17.4 available and clearly marked normative
[ ] Backend Contract v0.4 available and clearly marked normative backend contract
[ ] M7 adjudication/closure material available
[ ] M7.5 oracle snapshot or reproducible reference available
[ ] test corpus location documented
[ ] formal-proof repository reference documented
[ ] artifact authority hierarchy documented in README
[ ] branch + PR workflow enabled
[ ] CI can run from a fresh environment
```

---

# 11. Initial data package recommendation

For the first Codex Cloud production compiler task, the ideal input package is:

```text
Production compiler policy
  NewLang_Production_Compiler_Bootstrap_Charter.md
  NewLang_Production_Compiler_Handoff_Manifest.md

Normative
  NewLang_v0_spec_Draft17_4.md
  NewLang_Backend_Contract_v0_4.md

M7 closure/adjudication
  NewLang_M7_Adjudication_Resolution.md
  NewLang_M7_Backend_Interop_Consolidation_REPORT.md
  NewLang_M7_Closure_MANIFEST.json        (if available)

Oracle
  NewLang_FrontEnd_Prototype_M7_5.zip
  or equivalent frozen prototype checkout

Formal evidence
  https://github.com/wakairo/NewLang_FormalProof
```

Optional convenience bundle:

```text
NewLang_M7_Backend_Interop_Closure_Bundle.zip
```

The exact packaging is less important than retaining the authority metadata in this manifest.

---

# 12. Handoff rule for Codex

Before implementing a semantic feature, Codex should be able to answer:

```text
What normative rule authorizes this behavior?
What lower-authority artifact is being used only as evidence/oracle?
What assumptions are implementation-only?
What would be reported if these disagree?
```

If those answers are unclear, do not hide the uncertainty in C code.

Classify and report it.

---

# 13. Summary

The production compiler begins from a closed M7 semantic/backend baseline while M8+ and formal proof continue in parallel.

The key discipline is:

```text
specification defines semantics
prototype provides oracle evidence
formalization provides machine-checked evidence
production compiler implements and pressure-tests
```

No one lower-authority artifact is allowed to silently become the language specification.
