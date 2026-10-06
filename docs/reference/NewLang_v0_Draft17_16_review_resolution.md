# Draft 17.16 review resolution

Track: M

Compiler Issue #89 / M9.13 の targeted semantic-contract adjudication。
exact loop/continue/break source grammarを固定せず、
loop header hybrid cyclic-state semanticsだけをF2へ渡せる形で閉じる。

Draft 17.16 はcandidateであり、mainへmergeされるまではcanonicalではない。

## Authority

開始時:

- Compiler main: `5e2382b1db24eb35f49cdbd4a1a30fe8c226bde9`
- `CURRENT_SPEC.md` -> Draft 17.15
- Semantic Sync #87: CLOSED / PASS
- M9.12 #88: HYBRID selected
- P13: CLOSED
- R7: CLEAN / CLOSED
- FormalProof main: `a3a8c70becb3da6a9c2fc6f91ca578f49b19df32`
- F2 sequencing: should follow M9.13

## Decision summary

**HYBRID CLOSED.**

Loop semantics now distinguish:

### Exact edge invariants

- loop parameter count;
- static parameter types;
- fresh iteration binding identity;
- ordinary Copy/non-Copy transfer;
- affine responsibility conservation;
- iteration-scope dependency compatibility;
- captured outer non-Copy availability equality on every reachable continue edge.

### Cyclic abstract state

A conceptual header state `H` may soundly abstract:

- current carried semantic-package identity/origin;
- hidden dependencies of carried values;
- caller-visible / outer Copy current-value facts;
- current-value dependencies;
- finite ref/dependency alternatives;
- correlations required for safety.

The language semantics remains concrete-path semantics; `H` is a proof/checking abstraction.

## Reference semantics vs abstract checking

Reference semantics is the set of concrete loop-header states reachable from initial entry after any finite number of reachable continue steps.

A compiler need not enumerate this set.

It may instead establish a sound inductive abstract state `H`.

Conceptually, if `⊑` means “is soundly represented by”:

```text
EntryState ⊑ H

for every statically reachable continue edge i:
    ContinueTransfer_i(H) ⊑ H
```

equivalently:

```text
Join(EntryState, ContinueTransfers(H)) ⊑ H
```

This is a post-fixpoint obligation.

The specification does **not** require:
- exact least-fixed-point iteration;
- a particular lattice;
- a particular worklist/widening algorithm;
- fixed iteration count;
- concrete identity enumeration.

A stronger inductive post-fixpoint is acceptable.

## First vs later iteration

First iteration receives initial argument packages.

Every later iteration receives packages transferred by a continue edge.

Every iteration has fresh parameter **binding identities**.

Therefore checking one body only under the concrete first-entry state is unsound unless it is separately proved that all backedges return to that same invariant.

One symbolic body analysis is sound when its input `H` already subsumes entry and all reachable backedges.

## Symbolic affine package

Key distinction:

```text
fresh iteration binding identity
!=
dynamic semantic-package identity
```

For each non-Copy parameter slot, the header may conceptually track:

> the current affine package carried by this loop-parameter slot

without enumerating unbounded concrete package IDs.

Every represented concrete header state still has exactly one responsibility for that slot.

### Unchanged carry

Current package `P` is consumed/transferred to continue.

Next iteration has a fresh binding owning the same dynamic package `P`.

Exactly one responsibility survives.

### Transformed carry

Current package `P` is consumed and fresh same-type package `Q` is created and continued.

This is language-legal when ordinary ownership/dependency rules pass.

Next iteration carries `Q`, not `P`.

Static type equality must not be used as identity equality.

If a compiler cannot preserve conditional/changing affine identity soundly, it may issue a structured analysis-precision rejection; it may not mint, choose, duplicate, or erase responsibility.

## Dependency recurrence

Carried dependencies and caller-visible current-value dependencies may recur through backedges.

Conceptually:

```text
D_header
  >= Join(
       D_entry,
       F_continue_1(D_header),
       F_continue_2(D_header),
       ...
     )
```

Exact notation is non-normative.

Rules:

- iteration-local dependency is rejected before it reaches the next header;
- surviving external/public dependencies may differ across continues;
- literal equality is not required;
- finite alternatives / may-set / Unknown / widening are allowed;
- widening must be conservative;
- Unknown is **not** dependency-free;
- an unresolved possible blocker must continue to block conflicting mutation/lifetime-end operations unless proved absent;
- loss of necessary correlation may cause precision rejection, never unsafe acceptance.

## Outer Copy/current-value recurrence

Outer Copy/current-value state may change every iteration.

Later iterations therefore do not generally see the original entry fact.

The header abstraction may contain:
- exact state when invariant;
- finite alternatives;
- larger may-set;
- Unknown;
- other sound inductive abstraction.

Break post-state is computed from a break transfer starting from sound `H`; it is not required to restore the entry state.

No general relational heap analysis is required.

## Captured outer non-Copy availability

Existing strong rule is retained exactly.

Every reachable continue edge must re-establish the loop-entry availability of captured outer non-Copy bindings.

No MaybeConsumed lattice is introduced.

Edge classes differ:

- continue -> header invariant;
- break -> finite loop-exit join;
- return -> enclosing function exit.

Changing affine state across iterations should be explicit loop-carried package state.

## Multiple continue edges

All reachable continues:
- satisfy exact arity/type/affine/scope/outer-availability rules separately;
- feed the same `H`;
- may contribute different dependency/current-value/package-origin alternatives.

No single edge may be chosen as canonical.

A sound join/widening is permitted.

If correlation loss makes safety unprovable, precision rejection is required.

## Break / return separation

```text
continue -> loop header recurrence
break    -> loop normal exit
return   -> enclosing function exit
```

Break is a finite exit join once `H` is established.

Reachable breaks require:
- equal static result type;
- equal outer non-Copy availability at the loop exit;
- scope/dependency compatibility;
- ordinary finite Copy/ref/dependency/current-state joining.

Different sound non-Copy result identities from different break edges are legal language behavior.
A compiler may precision-reject if it lacks a conditional affine result representation.

Return:
- never feeds `H`;
- never feeds break result join;
- contributes to enclosing function exit;
- is analyzed from the sound header abstraction.

No infinite unrolling of return evidence is required.

## Zero-break adjudication

Compared:

### A — allow zero reachable breaks

**SELECTED.**

If a loop has zero statically reachable break edges:

- it is not a static language error merely for that reason;
- it has zero normal outgoing loop edges;
- it has no normal loop result / post-state join;
- no bottom type is invented;
- no never type is invented;
- no synthetic `unit` is invented;
- reachable returns still exit the function;
- remaining execution may diverge via continue.

This matches the existing zero-normal-edge control-flow model.

### B — require at least one break

Rejected.

No ownership/dependency invariant requires such a restriction.
It would be compiler convenience, not semantic necessity.

## Static reachability

A continue/break/return edge contributes unless ordinary static control-flow semantics proves it cannot be reached.

Incidental host/test knowledge of a runtime bool/value is not sufficient.

M9.13 does not add constant propagation or symbolic execution.

This preserves the P13 discipline that branch checking is not pruned merely because a fixture knows a value.

## Nested loops

Each loop owns its own header abstraction.

- inner continue -> inner header only;
- inner break -> inner loop normal result/post-state;
- inner return -> function exit;
- outer continue sees the normal post-state produced after the inner loop.

Nested fixpoints may exist in an implementation.

First F2 may model one loop only.
First production loop slice may precision/resource-limit nesting.

Neither is a language-level nested-loop ban.

## Body-sensitive calls

A known acyclic body-sensitive call is an ordinary iteration transfer.

Its caller-visible post-state/dependencies feed the resulting:
- continue;
- break;
- return

edge.

Repeated calls across iterations are summarized through the loop cyclic abstraction; literal infinite inlining is not required.

Recursive call SCC remains a separate compiler/formal precision problem and is not solved by Draft 17.16.

# W1–W15

## W1 — pure Copy counter

A Copy counter slot has invariant static type but changing runtime value.

`H` may represent the current counter abstractly.
`continue(next(i))` feeds the successor back into the same slot abstraction.
`break i` reads the current represented value.

No persistent concrete binding/ValueId identity is required.

**PASS.**

## W2 — unchanged non-Copy carry

One non-Copy package is consumed/transferred through continue and eventually break.

Each iteration binding is fresh, but the same package responsibility may move through all iterations.

At every step there is exactly one responsibility.

**PASS.**

## W3 — transformed non-Copy carry

Each iteration consumes current package and creates a new same-type package.

Header abstraction denotes “current affine package in this slot” rather than one eternal package ID.

Identity may change while uniqueness/type remain exact.

**PASS.**

## W4 — two continue edges with different dependencies

Both edges must satisfy exact type/affine/scope/availability rules.

Dependency facts may join into finite alternatives/may-set/Unknown.

If correlation is needed for a later safety proof and cannot be retained, compiler precision-rejects.

Literal dependency equality is not required.

**PASS.**

## W5 — two continue edges with different affine origins

Different origins are not equated by static type.

A conditional/symbolic affine carrier may represent “one of these possible current packages” while preserving exactly one runtime responsibility.

If the implementation lacks such a sound abstraction, precision rejection is allowed.

**PASS.**

## W6 — outer Copy mutation

An iteration writes outer Copy/current-value state and continues.

Later iteration header cannot be assumed equal to the initial entry fact.

That post-state enters `ContinueTransfer(H)` and must be covered by `H`.

**PASS; decisive cyclic witness.**

## W7 — outer non-Copy availability mismatch

One continue consumes captured outer owner; another keeps it Available.

At least the consuming edge fails the exact “same as loop-entry availability” rule.

**REJECT.**

No widening/Unknown may rescue it.

## W8 — iteration-local dependency escape

A continue argument, break result, or surviving outer current-value state depends on the ending iteration-local scope/place.

Reject on that edge before header/exit joining.

**REJECT.**

## W9 — multiple break edges

Copy:
- ordinary finite result/state join.

Ref/dependency:
- sound finite alternative join.

Non-Copy:
- distinct dynamic identities are language-legal when each path has one sound responsibility;
- compiler may precision-reject conditional identity it cannot represent.

**PASS at language level subject to ordinary edge obligations.**

## W10 — return + continue + break

The three edge classes remain disjoint:

- continue feeds `H`;
- break feeds loop normal exit;
- return feeds function exit.

No edge is double-counted.

**PASS.**

## W11 — zero-break loop

All reachable loop-local exits are continue and/or return.

Loop has zero normal outgoing edge/result.

No bottom/never/unit synthesis.

**PASS / legal control-flow shape.**

## W12 — outer Copy write + later dependent read

An iteration installs a current value carrying dependency `D`, then continues.
A later iteration may read/use that state.

If widening yields Unknown, Unknown must conservatively include the possibility of `D`.

A later operation that could invalidate `D` must be rejected unless the analysis proves the dependency absent.

Treating Unknown as empty would be unsound and is explicitly forbidden.

**PASS only with dependency-preserving over-approximation.**

## W13 — nested loops

Inner and outer headers are independent semantic owners.

Inner normal break post-state can flow through the remainder of outer body and into outer continue.

Inner return bypasses both loop exits and reaches function exit.

**PASS compositionally.**

## W14 — body-sensitive call in loop

Known acyclic callee mutation/dependency result is part of the iteration transfer.

Its post-state feeds the relevant continue/break/return edge.

Header recurrence accounts for repetition.

**PASS.**

## W15 — condition-known fixture pressure

A test harness may know a bool value.

Unless normal static language semantics proves an edge unreachable, the checker still includes the syntactically reachable continue/break/return alternatives.

Fixture knowledge alone cannot shrink the loop recurrence/exit set.

**PASS.**

## Implementation freedom / precision rule

Permitted:
- finite may-set;
- monotone finite abstract domain;
- Unknown;
- widening;
- worklist iteration;
- memoized transfer;
- inductive post-fixpoint;
- conservative analysis-precision rejection.

Not required:
- least fixed point;
- exact trace enumeration;
- exact identity enumeration;
- one SSA representation;
- one widening algorithm;
- fixed iteration bound;
- source-visible loop invariant.

Required:
- over-approximate all reachable relevant state;
- preserve possible safety dependencies;
- preserve affine one-responsibility invariant;
- never infer identity equality from static type alone;
- never use first-iteration state for later iterations without proving inductiveness;
- never turn precision loss into unsafe acceptance.

## Targeted research decision

**NOT REQUIRED.**

The least-fixpoint vs inductive-post-fixpoint fork is resolved by separating:
- concrete reachable executions as language reference semantics;
- any sound inductive post-fixpoint as checker evidence.

No external language majority decision is needed.

## F2 handoff

**F2 TRIGGERED — bounded cyclic loop-header kernel next.**

Recommended F2 scope:

1. one loop header;
2. one concrete/abstract entry edge plus finite continue backedges;
3. abstract-state representation relation / post-fixpoint;
4. symbolic affine parameter carrier;
5. unchanged and transformed affine carry;
6. exact captured outer non-Copy availability invariant;
7. finite hidden dependency/current-value abstraction;
8. monotone join and conservative widening/Unknown safety property;
9. iteration-local dependency nonescape;
10. disjoint continue / break / return edge classes;
11. finite break-exit abstraction;
12. zero-normal-loop-exit case.

Required counterexamples/properties:

- first-iteration-only checking is unsound when outer Copy/current-value state mutates;
- transformed affine carry changes concrete package identity without duplication/loss;
- iteration-local dependency cannot cross continue or break;
- widening/Unknown cannot erase a blocker and make an unsafe transition accepted;
- break and return edges do not feed the header;
- a valid inductive over-approximation need not equal the least fixed point.

F2 does not need:
- exact loop source grammar;
- nested loops initially;
- generic/callable;
- recursive call SCC;
- production compiler;
- full language soundness.

After F2, perform Semantic Sync before exact loop/continue/break source closure.

## Candidate normative changes

Draft 17.16 is localized to:

1. Draft 17.16 summary.
2. §13.5a loop-carried dependency / cyclic header wording.
3. §27.5 header/reference semantics, hybrid decomposition, first/later iteration, symbolic affine package, reachability, composition.
4. §27.6 exact continue invariants and cyclic recurrence.
5. §27.7 finite break-exit semantics.
6. §27.8 edge separation and zero-break semantics.
7. §27.11 implementation freedom / safety constraints.

No source grammar or structural reservation changes.

## Explicit non-goals

No:
- exact loop source grammar;
- exact continue/break grammar;
- reservation of loop/continue/break;
- while/for;
- mutable local syntax;
- source-visible invariants/effects/lifetimes;
- mandatory least fixed point;
- fixed widening thresholds;
- production implementation;
- FormalProof implementation inside M9.13;
- nested-loop production support;
- recursive call SCC;
- generic/associated/requires/callable/nominal source work;
- new Red Team;
- LLVM / relocation / FFI / modules / concurrency.

## Disposition

Candidate Draft 17.16 closes the semantic contract and is ready for Coordination review.

**F2 TRIGGERED — bounded cyclic loop-header kernel next.**
