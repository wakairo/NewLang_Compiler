# P3 semantic slice contract — authority audit pending

Date: 2026-10-05 (Asia/Tokyo). Status: **BLOCKED at the exclusive-authority
argument adjudication gate; no P3 semantic implementation yet**.
Branch: `p3-first-semantic-vertical-slice`. Base/main:
`cd229877d550e913260ad5a79627b65c1eb405d0` (merged P2 PR #4, verified remotely).

## Authority and scope

Draft 17.4 > Backend Contract v0.4 > adjudicated M8.1–M8.4 decisions >
reviewed FormalProof > M7/M7.5 evidence > frozen Python oracle > production.
The P3 task supplies the scope and selected closed surfaces; its attachment is
not a replacement normative language specification. Standalone M8 adjudication
documents are not in the checkout or the supplied attachment. No independent
verification of those documents is claimed. No normative or historical document
has been changed.

The supported target is P2 syntax -> explicit context -> checked representation
for nominal/ptr/ref/slot/unit types, value use, registered calls, the nine selected
core operations and loan acquisition headers. P2 parser/nodes remain syntax-only.
No M8.3/M8.4 grammar expansion, loan body traversal, dependency/effect solver,
typed MIR, LLVM lowering, public dump mode or P4 belongs in this task.

## Agreed requirements; not implemented

- Context owns its semantic tables and state; multiple contexts must be
  independent. Type, symbol, value, domain, place/incarnation and scope identities
  must remain distinct concepts rather than source strings or LLVM objects.
- Nominals have registered capabilities; ptr and ordinary refs are Copy and
  Discardable, exclusive refs are non-Copy and Discardable, LifetimeDomain and
  slot are non-Copy and non-Discardable; Copy implies Discardable.
- Bindings are single-assignment in the fixture scope and Available/Consumed.
  Ordinary Copy use duplicates a package; ordinary non-Copy use transfers it.
  Place/incarnation/governing domain are distinct from the current value package.
- Hidden dependency-bearing/unknown state must return semantic unsupported or
  analysis precision limit; it cannot be silently treated as dependency-free.
  Modeled ref scope obligations still need explicit handling.
- Registered calls have one selected signature, left-to-right arguments and a
  deliberately small no-caller-visible-effect contract. Ordinary write-to-read
  weakening is parameter compatibility after selection, not implicit borrowing,
  overload ranking, reverse conversion or exclusive-ref weakening.
- Core semantic operation kinds belong in checked representation, not P2 nodes:
  LifetimeDomain creation, ptr_from_ref, finalize_domain, initialize, take,
  destroy, replace, store and swap.
- Checked results distinguish unit, one value and multiple responsibilities.
  take returns separate T and slot<T> results, never a tuple. Single-name
  binding of that result remains semantic unsupported in this P2/P3 subset.
- Loan checks only header acquisition applicability. Local headers preserve
  source availability; pointer headers require live exact incarnation,
  provenance/access evidence and matching domain stability. Scope and future
  obligations must be recorded without committing a permanent outer borrow or
  claiming body nonescape. Pointer exclusive-write acquisition is unsupported
  unless the adjudicated rule can be verified.
- Every failed check, including later argument failure and OOM, must leave
  observable context unchanged. A small candidate state/commit boundary is
  permitted; a general transaction framework is not needed. Owned artifacts,
  source/context borrows, destruction and syntax-tree independence must be
  documented before implementation.
- Semantic error, unsupported, precision limit, OOM, resource limit and internal
  error remain distinct. Canonical byte spans identify the affected source role.
  Neither semantic lifetime nor exclusivity is an LLVM optimizer promise.

## COMPILER-SPEC-AMBIGUITY: ENDING-ARG-01

The task explicitly requires auditing whether the ending argument is consumed
or operation-locally reborrowed before implementing take/destroy (§116–117,
§226). Its gate forbids a silent implementation choice or forced P3 completion.

Minimal distinguishing fixture: two live independently ending roots of T are
governed by the same live domain D. Available binding `ending` has type
`exclusive ref<read,LifetimeDomain>` referring to D. P2 parses both fragments:

```text
take(p1, ending)
take(p2, ending)
```

The same distinction applies to destroy when T is Discardable. Result
responsibilities can be inspected separately by the module test harness; no
tuple binding or full-program grammar is implied by this fixture.

Two source-to-semantic mappings produce different observable binding states:

1. The direct argument is ordinary non-Copy value use, consuming `ending` (or
   consuming a child ref explicitly prepared by the caller). The second direct
   use of the same binding fails. Reusing an outer ref would require an explicit
   child/reborrow step or another source rule.
2. This primitive argument position automatically creates a checked shorter
   exclusive child, uses it for the operation and ends it before returning.
   The named outer binding stays Available and the second operation can succeed
   subject to the remaining root/domain/conflict checks.

Evidence:

- **Draft §4.2 / §4.4:** ordinary non-Copy value use consumes its source;
  exclusive ref is non-Copy.
- **Draft §12:** shorter ordinary/exclusive reborrows are permitted; parent
  conflicting use is suspended while the child is live, then becomes usable.
  It explicitly says exclusive-to-exclusive reborrow is for sequential reuse
  of lifetime-ending authority. This establishes a legal mechanism, but the
  inspected text does not specify that bare `ending` in this canonical call
  automatically denotes that child rather than ordinary value use.
- **Draft §13.4 / §14.2–3:** take/destroy require the matching exclusive-read
  domain authority and describe lifetime/value/occupancy effects. Those effects
  alone do not select the argument binding's post-state.
- **Backend Contract v0.4 §§2, 5, 8:** authority/exclusivity/lifetime are semantic
  facts; LLVM attributes cannot adjudicate this source-level choice.
- **Frozen M7.5 oracle:** static inspection of
  `newlang_frontend/typecheck.py` lines 868–883 finds an explicit
  `consume_direct_name(ending, env)` in the take/destroy branch. This is reference
  evidence, not normative authority. No new source-level oracle case has been
  executed or represented as adjudication.
- **FormalProof** at `fdda0d99d3de961f782f465fa2033b5554790ba5`:
  `NewLang/F0/Lifetime.lean` abstracts caller `CanEndRoot`; F0.4 records the
  matching-domain/root/discardability transitions, not exclusive-binding
  availability. `Reference.lean`/F0.5 defer lexical ref scopes; `Domain.lean`/
  F0.6 abstract remaining scoped applicability. These proofs do not resolve the
  canonical argument mapping. Lean was not built and is not a dependency.

Resolution needed: the existing M8.1 decision that selects this mapping, or an
explicit source-semantic adjudication. If operation-local reborrow is selected,
it must define the child/parent extent, outer binding post-state and nested
argument evaluation lifetime; if ordinary consume is selected, it must describe
how the advertised sequential authority reuse is expressed in the supported
source subset. P3 must not invent this rule from implementation convenience.

Until that decision is supplied, the ending-argument transition remains
unimplemented. No new conflicting normative rule is alleged: this finding is
an unresolved application/mapping question, not a claim that §4.2 and §12
contradict each other.
