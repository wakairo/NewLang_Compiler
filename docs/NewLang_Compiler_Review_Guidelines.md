# NewLang Compiler Review Guidelines

**Status:** production compiler review policy  
**Applies to:** P1 and later  
**Audience:** Codex implementer, human reviewer, ChatGPT review/adjudication  
**Authority:** Class I review policy; not normative NewLang semantics

## 0. Purpose

Review works best when the implementer knows the review model before writing code.

This document is a shared contract between reviewee and reviewer. Its goals are to reduce
avoidable rework, make review criteria predictable, distinguish blockers from preferences,
encourage testable module boundaries, prevent scope creep, and preserve the project's authority
hierarchy.

Do not code to the checklist by building unnecessary frameworks.

## 1. Review philosophy

Judge production PRs primarily on:

```text
correctness
faithfulness to normative semantics
clear module contracts
C ownership/lifetime safety
testability
failure behavior
scope discipline
maintainability
reproducibility
```

Ask:

> Is this the smallest clear implementation that satisfies the current contract and leaves the
> next step understandable?

Do not reward speculative abstraction.

## 2. Authority during review

Use the existing hierarchy:

```text
N  normative specification/backend contract
A  adjudication/closure
O  Python oracle/reference
F  formal evidence
I  production implementation
```

Production code/tests do not override higher authority.

When artifacts disagree, classify the discrepancy rather than fixing it silently in C.

## 3. Scope first

Before line review confirm:

- milestone,
- explicit scope/non-goals,
- normative basis,
- which modules/contracts are being introduced.

A correct implementation can still be rejected for prematurely implementing deferred semantics
or creating a framework outside the milestone.

Likewise reviewers should not demand explicitly deferred work unless the current design makes
future work impossible.

## 4. Review packet

A substantial PR should normally state:

```text
scope / non-goals
normative basis
new/changed modules
important ownership/lifetime contracts
test evidence
oracle/formal evidence if relevant
COMPILER-* findings
current-head CI
known limitations
```

Keep it concise; do not restate code line by line.

## 5. Module-level review

For each meaningful stateful module ask:

### Responsibility
Can the purpose be stated in one or two sentences?

### State/invariant
What state is owned? Which combinations are valid? Which operations mutate it?

### Ownership/lifetime
Which inputs are owned, borrowed, transferred, consumed, or retained? How long do returned views
remain valid? Who releases resources?

### Failure
What state remains on failure? Which outputs are valid? Who owns inputs/outputs?

### Testability
Can the module be tested without running the whole compiler? Does it have expected module-level
unit tests?

A stateful module impossible to test independently without real integration necessity is an
architecture concern.

## 6. NewLang-aware C review

Use `NewLang_Aware_C_Guidelines.md`.

Look for:

- hidden ownership conventions,
- ambient mutable global state,
- partial initialization leaking across interfaces,
- borrowed pointers with undocumented validity,
- unnecessary representation exposure,
- macro-hosted pseudo-language,
- LLVM concepts leaking into frontend semantics,
- unjustified giant context objects.

Do not reject idiomatic C merely because it does not resemble NewLang syntax.

## 7. Testing review

Use `NewLang_Compiler_Testing_Strategy.md`.

Review module-unit, subsystem-integration, oracle/differential, end-to-end, and sanitizer evidence
at the appropriate levels.

A new stateful module without direct unit tests needs a reason. Oracle agreement does not excuse
missing component tests. Unit tests do not replace semantic/end-to-end evidence where observable
NewLang semantics are implemented.

## 8. Evidence over assertion

Prefer:

```text
"no leak"             -> ASan
"both host compilers" -> GCC + Clang CI
"oracle agrees"       -> differential test
"rejects this case"   -> regression fixture
"recovers after fail" -> failure-path unit test
```

Documentation explains what was established and why; tests/CI establish that it holds.

## 9. Tool-assisted review and formatting

Human review should not spend time on formatting a deterministic tool can settle.

The repository uses:

```text
.clang-format
pinned clang-format-23
bash scripts/format.sh
bash scripts/check-format.sh
CI enforcement
```

The formatter is authoritative for indentation, braces, wrapping, pointer spacing, continuation
indentation, initializer/switch layout, and similar mechanical C/H style.

Reviewers should not request manual formatting that conflicts with committed formatter output.

If formatter output should change project-wide, review `.clang-format` itself rather than
hand-formatting one PR.

Automatic correction is encouraged before/update of a PR. CI only checks and fails; it does not
silently rewrite source.

This policy currently adopts clang-format only. Do not infer that clang-tidy is required.

Reviewer attention should instead focus on semantics, architecture, ownership/lifetime, failure
behavior, tests, diagnostics, performance, and portability.

## 10. Review severity

### BLOCKER
Must resolve before merge.

Examples:

- normative semantic conflict,
- credible C UB/memory unsafety,
- materially ambiguous ownership transfer,
- incorrect failure cleanup,
- unjustifiably untestable nontrivial stateful module,
- core contract left untested,
- silently encoded spec hole/ambiguity,
- material milestone scope violation,
- current-head required CI not green,
- weakened closed M7 backend constraint.

### REQUESTED CHANGE
Substantial improvement expected before merge but not necessarily semantic incorrectness.

Examples: unnecessarily coupled module state, only end-to-end tests for a direct unit contract,
underspecified failure state, duplication obscuring ownership/error reasoning.

### NON-BLOCKING
Optional improvement: naming, equivalent representation, small refactor, prose polish.

Do not turn preference into a blocker.

### QUESTION
Reviewer understanding is incomplete. A question is not automatically a requested change.

## 11. Reviewer obligations

Reviewers should:

- cite the relevant guideline/semantic rule for blockers when practical,
- distinguish correctness from preference,
- avoid speculative abstraction demands,
- respect milestone non-goals,
- review the current implementation rather than an imagined future compiler,
- state what evidence would resolve uncertainty.

Review should improve the implementation, not reward complexity.

## 12. Reviewee obligations

Codex/implementer should:

- explain non-obvious architecture decisions,
- surface uncertainty,
- classify specification/lowering pressure,
- add the smallest appropriate tests,
- avoid unrelated broad refactors,
- address the underlying review concern rather than literal comment wording,
- leave the PR open/unmerged until approval.

If a review request conflicts with normative/project policy, report the conflict rather than
comply blindly.

## 13. No speculative abstraction for review

Do not introduce generic object frameworks, universal allocators, generic transition engines,
visitor/plugin frameworks, universal result types, or large dependency-injection systems merely
to anticipate future review.

Prefer a small concrete module until repeated use cases justify generalization.

## 14. Architecture review questions

Ask:

```text
Is dependency direction correct?
Is mutable state localized?
Does this module know about LLVM too early?
Does backend convenience freeze frontend semantics?
Can representation later change without unrelated rewrites?
Does the interface expose storage rather than responsibility?
Is there one clear owner for each allocation?
```

These questions do not automatically imply more layers.

## 15. Semantic review questions

For NewLang behavior ask:

```text
What normative rule is implemented?
What is accepted/rejected?
What state changes?
What identities/lifetimes are preserved/ended?
What behavior is conservative because facts are unknown?
Did the implementation accidentally inherit Python behavior rather than the spec?
```

Use formal evidence where useful.

## 16. Test review questions

Ask:

```text
Is the smallest owning contract directly tested?
Are failure paths and boundaries tested?
Are ownership/lifecycle transitions tested?
Are module seams integration-tested?
Is oracle evidence present where appropriate?
Do GCC/Clang/ASan/UBSan exercise the tests?
Would the test remain useful after an internal refactor?
```

Do not demand one test per function.

## 17. C safety review

Explicitly inspect bounds, overflow affecting memory/state, use-after-free, double free,
uninitialized reads, pointer lifetime, alias assumptions, cleanup, NULL handling, and
string/byte length handling.

Sanitizers are required evidence but not a substitute for interface reasoning.

## 18. Diagnostics review

Distinguish semantic correctness, diagnostic classification/code, source attribution, and prose.

Early work should not be blocked merely because wording can improve when semantic cause and
location are correct and prose is not frozen.

Misleading attribution or hidden semantic cause may require change.

## 19. Performance review

Do not prematurely optimize P1.

Note obvious asymptotic traps. Use `COMPILER-PERFORMANCE` when semantics create material cost.
Prefer measurement before adding caching, arenas, interning, or incremental infrastructure.

## 20. Portability review

The current baseline is Linux x86_64, C17, GCC+Clang, exact reviewed LLVM, ASan/UBSan.

Do not block P1 for unrequested Windows/macOS/embedded/cross support. Do block accidental
compiler extensions/platform assumptions outside the stated baseline.

## 21. Documentation review

Document decisions future implementers cannot reliably infer from code: ownership/lifetime
contracts, semantic basis, non-obvious safety rationale, portability boundaries, conservative
behavior, and reviewed deferrals.

Do not create a second implementation record that merely restates transient code details.

## 22. Change locality

Prefer one coherent milestone per PR.

Small related cleanup may stay. Large unrelated refactors/toolchain upgrades/mechanical rewrites
should be separated where practical.

## 23. Current-head validation

Merge evidence corresponds to the current PR head, not an older green run or pre-refactor local
test.

## 24. Python oracle review

Ask what the oracle demonstrated, not whether production copied Python.

A mismatch may indicate production/oracle/formal/spec issues. Classify first.

## 25. FormalProof review

Use formal legal witnesses, negative lemmas, and invariants as evidence.

Do not demand Lean ghost structures in production. The question is whether production establishes
the required semantic property.

## 26. NewLang-aware feedback

When review finds recurring friction, distinguish C-specific friction, compiler architecture
requirements, and possible NewLang design pressure.

Do not derail an implementation PR into language redesign unless faithful implementation is
blocked.

## 27. Merge rule

Codex does not self-merge unless explicitly instructed.

Normal flow:

```text
feature branch
 -> PR
 -> current-head CI green
 -> review/adjudication
 -> user merge after PASS
```

Review outcome is `PASS`, `CONDITIONAL PASS`, or `CHANGES REQUIRED`.

## 28. Definition of review-ready

A PR is review-ready when scope is evaluable, failures are not hidden, new stateful modules have
expected unit tests, appropriate higher-level evidence exists, local checks pass, material design
decisions are explained, COMPILER-* issues are surfaced, and current-head CI has been requested.

## 29. Avoiding review churn

Useful development sequence:

```text
1. define module responsibility
2. define ownership/failure contract
3. define directly testable API
4. implement smallest correct behavior
5. add module unit tests
6. add integration/semantic evidence
7. run formatter + test/sanitizer matrix
8. document non-obvious decisions
9. open/update PR
```

Testability and ownership are design inputs, not cleanup after coding.

## 30. What review should not maximize

Do not maximize abstraction count, design patterns, line coverage percentage, hypothetical
extensibility, similarity to another compiler, similarity to Lean, or documentation volume.

Maximize clarity, correctness, local reasoning, testability, semantic fidelity, and ability to
change later.

## 31. Suggested review summary

```text
Scope / authority       PASS | ISSUE
Architecture/modules    PASS | ISSUE
Ownership/lifetime      PASS | ISSUE
Unit tests              PASS | ISSUE
Integration tests       PASS | ISSUE
Semantic/oracle         PASS | ISSUE | N/A
C safety/sanitizers     PASS | ISSUE
CI/reproducibility      PASS | ISSUE
Scope discipline        PASS | ISSUE

Overall:
    PASS
    CONDITIONAL PASS
    CHANGES REQUIRED
```

Only material issues need commentary.

## 32. Summary

The shared contract is:

```text
Codex knows what matters before coding.
Reviewer distinguishes blockers from preferences.
Stateful modules are designed for unit testing.
Oracle tests complement component tests.
Normative semantics outrank executable precedent.
C ownership/failure behavior is explicit.
Mechanical formatting comes from clang-format.
Evidence comes from tests and current-head CI.
Scope stays small.
```

The goal is not to eliminate disagreement; it is to make disagreement occur at the real design
boundary rather than because review expectations were hidden.
