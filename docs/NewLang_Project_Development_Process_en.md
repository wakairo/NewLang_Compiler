# NewLang Project Development Process

> English companion to the primary Japanese document: `NewLang_Project_Development_Process.md`
>
> Status: Initial operating policy
>
> This document describes the current cross-track development process for NewLang.
> It is intentionally lightweight and may be revised as the project learns.
> Git commit history is the source of truth for change history; this document does not maintain a separate change log or version history.

## 1. Purpose

NewLang is developed through several partially independent tracks:

- **M — Specification / design**
- **F — Formal proof / semantic formalization**
- **P — Production compiler implementation**
- **R — Independent red-team review**
- **Coordination — cross-track adjudication and sequencing**

The goal is not to make the tracks agree by construction.
The goal is to let different forms of evidence challenge the same language design and feed concrete findings back into the canonical specification.

## 2. Canonical authority

The normative semantic specification is the Draft named by:

```text
NewLang_Compiler/main
docs/reference/CURRENT_SPEC.md
```

Design reports, proof reports, compiler reports, experiments, deep research, and conversation history are evidence.
They do not override the canonical Draft by themselves.

If an important decision exists only in a report or conversation, it should be promoted through a reviewed Draft revision before being treated as normative.

UTF-8-managed project documents should normally use Japanese as the primary language, with an English companion when useful.
For commit messages and other text that appears prominently in shell/toolchain workflows, prefer English ASCII for portability and operational simplicity.

## 3. Track responsibilities

### M — Specification / design

M explores and adjudicates language semantics and source surface.
It should prefer small, orthogonal mechanisms and targeted reopen over broad redesign.

M should not advance indefinitely ahead of implementation and formalization.
As a practical default, M should normally stay within roughly one major semantic milestone of P/F unless coordination records a reason to run further ahead.

### F — Formal proof

F formalizes selected semantic claims and searches for contradictions, missing invariants, and hidden assumptions.

Formalization is evidence about the specification, not an authority above it.
Do not preserve a language rule merely because it makes an existing proof convenient.

### P — Production compiler

P tests whether the canonical semantics can be represented, diagnosed, checked, and eventually lowered in a realistic compiler.

Implementation difficulty is valuable feedback, but backend convenience alone should not redefine source semantics.

### Coordination

Coordination owns:

- authority/version synchronization;
- milestone sequencing;
- classification and severity of findings;
- deciding KEEP / CLOSE / TARGETED REOPEN;
- deciding when a Draft revision is required;
- deciding when M should pause for P/F catch-up.

## 4. Standard feedback loop

Specification changes should normally follow:

```text
Finding
  -> classification
  -> severity
  -> minimal reopen target
  -> design/adjudication
  -> candidate Draft
  -> review / CI
  -> merge to main
  -> targeted downstream revalidation
```

A report is not a substitute for this process.

Prefer **targeted reopen** when a concrete problem can be isolated.
Do not reopen a whole milestone merely because one workload exposes one local surface gap.


### 4.1 Cross-track communication

Durable, referenceable communication between tracks should normally use **GitHub Issues as the primary channel**.
Prompts and chat may be used to launch or control work, but the full handoff should not live only in conversation history. Prefer a thin launch instruction that points the receiving track to the corresponding Issue.

As a default, use one Issue for one bounded task, finding, or revalidation scope.
When relevant, keep the following together in that Issue:

- authority such as canonical repository, main SHA, and `CURRENT_SPEC.md`;
- scope and non-goals;
- questions, requested work, and stop conditions for the receiving track;
- findings, counterexamples, and CI / proof / implementation evidence;
- Coordination adjudication;
- closure, reopen, and downstream revalidation state.

Actual repository changes are reviewed in PRs, and the Issue should link to the relevant PR.
Issue/PR discussion, reports, and prompts are evidence and coordination records; they do not override the canonical specification by themselves.

Because multiple tracks currently use the same GitHub account, substantive Issue bodies and comments written on behalf of a track should identify the speaker near the beginning:

```text
Track: Coordination
Track: M
Track: F
Track: P
Track: R
```

The goal is later traceability of whose judgment, finding, or question is being recorded; no strict machine-readable format is required.

Questions, answers, additional evidence, and adjudication within the same scope should normally be appended to the existing Issue rather than relying on references such as “that earlier prompt” or conversation history.
If the scope materially changes, create a new Issue and link it from the original one.

Do not create an Issue for every trivial exchange.
Small implementation discussion or follow-up within one bounded task should remain in the existing Issue or PR.

For Red Team work, the independence rules in §7 take precedence.
A first-pass attack Issue should contain only the deliberately narrow input allowed at that stage and should not pre-seed M/F/P rationale or conclusions.
After first-pass findings are recorded, additional evidence may be appended to the same Issue or supplied through an explicitly linked follow-up Issue.

## 5. Finding classification

Use categories such as:

```text
CORE-SEMANTIC-GAP
SOURCE-SURFACE-GAP
COMPILER-PRECISION
DIAGNOSTIC-QUALITY
LIBRARY-DESIGN
RUNTIME-IMPLEMENTATION
LOCALIZED-UNCHECKED-BOUNDARY
DEFERRED-BUT-ADDABLE
CONVENIENCE-ONLY
FORMAL-HOLE
FORMAL-AMBIGUITY
```

Severity should be separated from classification.
A compiler-precision problem is not automatically a language-design problem.

## 6. Synchronization points

At meaningful boundaries, pause new semantic invention and run a **Semantic Sync Review**.

A sync review should record at least:

- canonical Draft and main SHA;
- M milestone state;
- P implementation coverage;
- F proof coverage;
- unresolved contradictions or precision gaps;
- Deferred features relevant to the next milestone;
- whether M is too far ahead of P/F;
- recommended next cross-track sequence.

Typical sync points include closure of a major M milestone or before starting another major semantic family.

## 7. Independent Red Team

The Red Team exists to reduce common-mode failure across M/F/P, especially when the same people or AI systems participate in multiple tracks.

### 7.1 Independence principle

The Red Team should begin from a deliberately narrow input set:

```text
canonical Draft
+
selected representative workloads / attack questions
+
minimal necessary repository context
```

It should **not normally begin by reading M/F/P conclusions, rationale, closure reports, or prior debates**.
The point is to avoid inheriting the same framing before forming an independent attack.

### 7.2 Communication during the attack phase

During the initial attack phase:

- Red Team should not participate in continuous design discussion with M/F/P.
- It may ask factual clarification questions when the Draft is genuinely ambiguous.
- Such questions should normally go through Coordination rather than becoming an iterative design conversation with the originating track.
- Coordination should answer with canonical references where possible, not with persuasive design rationale.

This is not strict information isolation.
It is a way to delay exposure to the project's preferred explanation until after the reviewer has formed its own model.

### 7.3 After initial findings

After the Red Team records its first-pass findings:

- it may inspect M/F/P reports and experiments;
- it may test whether an apparent problem is already covered by evidence;
- M/F/P may rebut findings with concrete specification text, proof, or implementation evidence;
- Coordination adjudicates the result.

Red Team findings are evidence, not normative decisions.

### 7.4 When to run Red Team review

Do not run a full Red Team pass on every minor edit.
Use it at higher-value boundaries, for example:

- before a major semantic milestone begins;
- after a large closure such as M8;
- before promoting a substantially broader Draft;
- when M/F/P all agree unusually easily on a risky mechanism;
- when a mechanism has high blast radius across lifetime, authority, provenance, or raw memory.

### 7.5 Red Team prompt style

Prefer prompts such as:

> Read the canonical Draft as an independent systems-language reviewer. Do not assume prior project decisions are correct. Find concrete workloads or invariants that break the current design. Distinguish specification gaps from compiler/library/runtime limitations. Do not redesign the language unless a concrete failure requires it.

Do not ask the Red Team to confirm that the current design is good.

## 8. Avoiding correlated validation

Different tracks should use different questions:

```text
M: Is this the smallest practical rule?
F: Is the rule internally coherent and provable where expected?
P: Can a production compiler represent and diagnose it faithfully?
R: What breaks if we ignore the project's preferred interpretation?
```

Passing all tracks increases confidence, but does not prove the design is complete.

## 9. Closure discipline

A milestone may close when its stated representative workloads and invariants pass and remaining issues are correctly classified outside the reopen threshold.

Closure means:

> the mechanism is adequate for the current v0 target and evidence set.

It does not mean:

> the API or syntax can never change again.

After closure, new issues should reopen only the smallest affected scope unless a deeper contradiction is demonstrated.

## 10. Current operating preference

For the current phase:

1. review and merge pending P/F milestones before starting another large M semantic milestone;
2. let P and F catch up enough to pressure-test the recently closed specification;
3. perform a Semantic Sync Review before substantial M9 work;
4. use an independent Red Team pass around that sync point or another similarly high-value boundary;
5. revise this process document when actual project experience shows a better operating model.
