# NewLang M7 backend / interop adjudication resolution

## Decision

M7.3–M7.5 close without a new semantic mechanism.

The adjudication produces **Draft 17.4**, but Draft 17.4 is a closure/status revision rather than a Draft-18-class semantic redesign.  Its purpose is to keep stable source-semantic rules in the language specification, keep optimizer/ABI details in the backend contract, and explicitly defer interfaces that M7 proved can be added later.

## 1. Promoted or retained as normative Draft 17.x wording

### A. `write != exclusive`

**Status: already normative; no new rule.**

M7.3 supplied destructive LLVM evidence for an existing language rule. `ref<write,T>` remains mutation authority, not restrict-style access exclusivity.  The LLVM consequences stay backend-only.

### B. Native aggregate layout is not C ABI identity

**Status: clarify in Draft 17.4 §31.**

Draft 17.3 already made native layout opaque and rejected C-compatible-layout guarantees. M7.4 demonstrated that the same C aggregate receives materially different target ABI classification and that semantic field marshalling works while raw native bitcopy does not.

Draft 17.4 therefore states explicitly:

```text
native aggregate/sum
    != implicit C ABI representation

C aggregate interoperability
    = explicit compatibility representation
    + semantic field/variant marshalling
    + target-specific ABI lowering
```

This does not introduce an FFI surface or native stable ABI.

### C. Foreign signature is not a semantic summary

**Status: clarify in Draft 17.4 §13.5c.**

A foreign signature, calling-convention identity, pointer type, or aggregate parameter shape alone does not prove semantic non-mutation, no-retain, synchronous callback behavior, or ordinary control return.  Missing preservation facts continue to use the existing `Unknown` rule.

### D. External backing import obligation

**Status: already normative; unchanged.**

The existing Draft 17.3 rule remains sufficient: an ordinary-safe `BackingRegion` may be exposed only while the boundary guarantees backing liveness, required range/alignment/access properties, and the ordinary-safe non-alias invariant.  Otherwise the view remains opaque/platform/unchecked.

M7.5's distinct-virtual-address/shared-backing probe strengthens confidence in the rule but does not change it.

### E. Foreign bytes / lifetime-start boundary

**Status: already normative reservation; unchanged.**

Raw foreign representation does not manufacture `Allocation`, `Storage`, `LifetimeDomain`, provenance, or hidden dependencies.  A future privileged foreign lifetime-start may expose the first safe `ptr<T>` only after complete representation and complete semantic `ValuePackage<T>` are established.

## 2. Backend-contract-only findings

The following are intentionally excluded from normative NewLang source semantics:

- `ref<read>` may lower with LLVM `readonly`, but ordinary refs do not gain `noalias` by type;
- `exclusive ref` does not imply LLVM `noalias` without a separate dynamic access-exclusivity proof;
- source ref nonescape does not imply LLVM pointer-provenance nocapture;
- automatic `dereferenceable` / `nonnull` requires target/null-address facts;
- NewLang typed lifetime is not generally `llvm.lifetime.*`;
- alias-scope metadata requires separately proved disjoint access sets;
- C aggregate `sret` / `byval` / register splitting / hidden-result lowering is target ABI classification;
- ABI-originated alias facts have ABI proof origin and are not reflected into NewLang permissions;
- exact LLVM `memory(...)`, capture, `nounwind`, `nocallback`, or equivalent attributes are emitted only from matching independent proofs;
- Checked MIR should preserve semantic effect, backend memory behavior, retention, callback, control-transfer, construction, and external-backing facts independently;
- generated C shims are an acceptable initial ABI implementation strategy.

These rules are consolidated in `NewLang_Backend_Contract_v0_4.md`.

## 3. Explicitly Deferred from v0 core/API

M7 evidence is strong enough to stop treating the following as questions that must be answered before the next compiler milestone:

1. persistent function-pointer source/API facility;
2. normative basic FFI declaration and contract syntax;
3. general external/static-backing ergonomic API;
4. retained/asynchronous callback semantics;
5. foreign exception/unwind/`longjmp` integration with NewLang scope-exit obligations;
6. general multi-view physical-alias / MMIO model;
7. safe partial / field-by-field / in-place construction;
8. source-visible general effect/lifetime/post-state system.

Their semantic extension points have been reserved sufficiently to permit later addition without reopening the ordinary v0 core.

## 4. Implementation-later, not language-design blockers

These do not justify delaying the next milestone family:

- direct target-specific C aggregate ABI classification;
- direct by-value aggregate varargs lowering;
- richer LLVM alias/capture/dereferenceability optimizations;
- foreign-contract inference and better diagnostics;
- richer precision for unknown/indirect calls.

The first production compiler may use conservative lowering and generated C shims, then replace individual fallbacks with direct implementations backed by differential tests.

## 5. Why not promote the M7.5 multi-axis record into source syntax?

M7.5 showed that the axes are semantically distinct; it did **not** show that users need to spell every axis in NewLang source.

Freezing a source contract now would convert a successful implementation separation into a language feature prematurely.  The correct commitment is smaller:

```text
compiler must not infer one axis from another
```

The source surface can remain Deferred until real FFI workloads reveal which facts users actually need to express and which can be inferred or supplied by compatibility wrappers.

## 6. Versioning judgment

Use **Draft 17.4**, not Draft 18.

Reason:

- no object/value/occupancy/dependency invariant changes;
- no new capability type;
- no new ordinary-safe lifetime transition;
- no new general effect system;
- only two clarification rules are added, and three open surface questions are resolved to Deferred.

Draft 18 remains reserved for a substantive semantic redesign.

## 7. Gate to the next milestone family

M7 is closed.  The next milestone should not reopen FFI/ABI semantics by default.

Reopen an M7 item only if one of these occurs:

- a concrete NewLang systems workload cannot be expressed without it;
- the production C/LLVM spine exposes a contradiction with Draft 17.4;
- a destructive test demonstrates that the current conservative boundary is unsound;
- conservative rejection becomes materially obstructive in representative v0 code.

Otherwise implementation should prefer the current small semantics plus localized backend/compatibility mechanisms.
