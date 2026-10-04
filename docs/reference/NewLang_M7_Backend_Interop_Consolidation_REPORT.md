# NewLang M7 backend / interop consolidation report

## 1. Scope

This pass consolidates the completed M7 backend/interop sequence:

```text
M7   intrusive owner recovery / opaque inverse projection
M7.1 address-sensitive values and place stability
M7.2 storage/layout/backend pressure
M7.3 LLVM alias/lifetime lowering contract
M7.4 C ABI / calling convention / aggregate boundary
M7.5 FFI / external backing / opaque-call boundary
```

The purpose is not to invent another mechanism. It is to determine which findings are language semantics, which are backend obligations, and which should remain outside v0.

## 2. Result

**PASS. M7 is closed.**

The M7.3–M7.5 findings fit the existing semantic architecture.  Two small normative clarifications are useful, while most executable evidence belongs in the backend contract.

The closure can be summarized as:

```text
source semantics state only target-independent safety facts
+
compatibility/backend layer owns C ABI and LLVM promises
+
foreign behavior is summarized narrowly or becomes Unknown
+
unsupported surface is Deferred rather than anticipated in the core
```

## 3. Normative delta

Draft 17.4 is generated from Draft 17.3 with no ordinary object/lifetime mechanism change.

The material additions are:

1. make the C aggregate boundary explicit: native aggregate/sum values have no implicit C ABI representation identity; interoperability uses an explicit compatibility representation and semantic marshalling;
2. state that foreign representation/signature/ABI information is not itself a semantic summary;
3. change persistent function pointer, normative FFI surface, and general external/static-backing API from unresolved v0 questions to explicit Deferred status.

Existing Draft 17.3 rules for `write != exclusive`, external-backing import, `Unknown`, foreign raw locations, and semantic `ValuePackage` remain unchanged.

## 4. What deliberately did not enter Draft 17.4

M7.3's LLVM lowering details do not belong in a source-language specification.  Neither do M7.4 target ABI details or M7.5 LLVM attribute spelling.

The following remain in `NewLang_Backend_Contract_v0_4.md`:

```text
noalias / readonly / capture restrictions
llvm.lifetime.* eligibility
dereferenceable / nonnull target policy
alias-scope metadata
sret / byval / register classification
foreign memory/capture/callback/unwind attribute mapping
generated C shim strategy
Checked MIR proof-origin separation
```

This separation is important: changing LLVM versions or adding a non-LLVM backend must not change NewLang source semantics.

## 5. Deferred scope after M7

The following are not prerequisites for the next milestone family:

```text
persistent function pointer syntax/type
normative FFI declarations/contracts
general external-backing ergonomic API
retained/asynchronous callback model
foreign nonlocal-control integration
general multi-view backing/MMIO model
safe partially-live/in-place aggregate construction
general source-visible effect system
```

Their extension boundaries are reserved; their source APIs are not frozen.

## 6. Production compiler consequence

The planned C-written production compiler can start with a deliberately conservative Checked MIR/backend spine.

It must preserve proof origins for at least:

```text
NewLang access/lifetime/dependency semantics
optional proven access exclusivity
capture/retention
known extent/alignment and target null policy
foreign ABI identity
foreign semantic effect
backend memory access
callback behavior
control transfer
construction certification
external-backing proof
```

A missing fact stays missing. It is not inferred from a stronger-sounding source type or ABI spelling.

Complex C aggregate ABI classification may initially be delegated to generated C shims. This is not a temporary semantic compromise; it is an implementation choice consistent with the language boundary.

## 7. Validation basis

This adjudication uses the canonical M7.5 integrated tree produced from the uploaded M7.4 archive:

```text
466 / 466 pytest PASS
100,000 M7.5 differential cases, 0 mismatch
M7–M7.4 published pressure outputs reproduced
M7.4 manifest verified before integration
Draft 17.3 bundle manifest verified
```

No source/runtime code change is required by consolidation itself. The canonical M7.5 code tree therefore remains the executable implementation baseline while Draft 17.4 becomes the semantic document baseline.

## 8. Final judgment

The important M7 outcome is not "NewLang now has an FFI". It is that the core does **not** need to know the final FFI shape in order to remain compatible with one.

Likewise, NewLang does not need to adopt LLVM's alias/lifetime vocabulary or C's aggregate layout as source semantics in order to compile efficiently and interoperate.

That is a useful stopping condition. Further work should now move forward rather than deepen the interop surface speculatively.
