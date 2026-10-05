# P3 semantic slice contract

Date: 2026-10-05 (Asia/Tokyo). Status: pre-implementation contract; the authority
audit gate is resolved by the user's explicit clarification below.
Branch: `p3-first-semantic-vertical-slice`. Base/main:
`cd229877d550e913260ad5a79627b65c1eb405d0` (merged P2 PR #4, verified remotely).

## Authority and scope

Draft 17.5 (17.4 + normative reborrow clarification) > Backend Contract v0.4 > adjudicated M8.1–M8.4 decisions >
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

## Implementation requirements

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

## ENDING-ARG-01 audit and resolution

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

The audit requested an explicit source mapping with child/parent extent, outer
binding post-state and nested argument evaluation lifetime before implementation.
The resolution below provides that mapping; this is no longer a pending gate.

The user supplied the explicit resolution on 2026-10-05: for an already-selected
compatible reference parameter/primitive operand, an existing exclusive-ref
binding is operation/call-locally reborrowed under §11.4 + §12 before the ordinary
non-Copy-transfer rule (§4.2 / §18.2) is considered. This is clarification of the
existing rule, not a new normative semantics change. The original M8 artifact source wording
gap is **DOCUMENTATION-GAP, no ambiguity blocker**. The later Draft 17.5 sync
described below supplies the explicit normative wording.

Implementation: a fresh child scope and child authority package reference the
same referent/incarnation/domain; the parent is suspended only during the child
extent. Later argument evaluation cannot conflict with that live child. At
operation return the child ends, the outer binding remains Available and can be
used sequentially. Selected exclusive/ordinary ref parameters can reborrow
compatible exclusive authority with the same access mode; this is not
ordinary Copy weakening or an implicit T -> ref<T> borrow. Core take/destroy
expect exclusive read-domain authority. Artifact records the scoped reborrow;
returned user-call ref/affine-core-authority summaries remain outside P3.
No historical Draft/oracle snapshot is rewritten to hide the gap.

During final validation upstream PR #6 added Draft 17.5 and its review resolution
at `10f4ffe5a02c846e4079b56e9254fc4b4e38f0de`. These byte-preserved documents
were incorporated from main and inspected against Draft 17.4: the delta is
explicit call-boundary clarification (§12.1, §14.2–3, §18.2), not new semantics.
Draft 17.5 is now the normative baseline for this review packet. Its reborrow
rule does not create otherwise-unestablished exclusive mode compatibility. P3
therefore returns unsupported for exclusive write-to-read argument mode changes
rather than copying the ordinary weakening rule onto exclusive types. Compatible
same-mode exclusive/ordinary child reborrows and ordinary Copy write-to-read
remain supported. Ordinary `let e2 = ending` still consumes/transfers ending.

## Concrete P3 state/ownership boundary

Use one small owning semantic context with append-only bounded tables and copied
host-registration names. Candidate checks deep-copy concrete context tables;
complete success swaps candidate state into the original context address.
Failure releases candidate and artifact. Registrations also follow this rule.
This intentionally favors explicit rollback over optimization for the first
slice; a general DB/allocator/effect framework is unnecessary.

Bindings have compiler-managed local carrier places (implicit governing domain
represented by zero), separate from ref/ptr referents. Scalar nominal/unit
fixtures, Domain values, slots, pointers and references have explicit package
identity/carrier roles. Root transitions currently support flat user-nominal
payloads. Nested primitive-authority payloads and returned core capability
summaries are semantic unsupported; never guessed from a type-only signature.
Hidden/unknown dependency markers on any surviving package reject the whole
fragment with ANALYSIS_PRECISION_LIMIT. Explicit ref scopes/liveness/conflicts
are checked separately; no full Value/Occurrence dependency algebra is claimed.

Checked artifacts own a bounded flat array of views with artifact-local child
indices, snapshot result IDs and canonical spans. They hold no syntax-node
pointers and survive tree destruction. Context IDs need the original live
context for interpretation; optional text access needs the live source.
Destruction needs neither and does not discard semantic result responsibility.
Expression results remain loose packages until explicitly forwarded/bound.

Limits: 4096 entries per context table/checked artifact, 128 registered parameters
and recursive checking levels. These are implementation budgets. Names are
copied with checked length/allocation; IDs/facts never wrap/revive on successful
history. Loan scopes are inactive plans; body contents remain opaque and their
nonescape is unproved. Pointer exclusive-write loan and exclusive input to
ptr_from_ref stay explicitly unsupported without additional adjudicated evidence.
Exclusive stability in a loan header needs a body-extent reborrow plan and
returns unsupported, rather than treating the compatible authority as illegal;
ordinary stability headers and call-local compatible reborrows remain supported.

Diagnostics keep static borrowed code/category/message plus canonical primary
span. Unknown names, consumed values, incompatible types, false known evidence
and violated closed preconditions are semantic errors. Missing analysis facts
are precision limits; unimplemented summaries/receiving/forms are unsupported;
OOM/resource failures are host errors. Invalid API owner slots leave all outputs
unchanged. Fixed semantic prelude names cannot be re-registered by the flat
function registry; local binding names remain a separate value namespace.
