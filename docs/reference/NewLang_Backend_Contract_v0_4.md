# NewLang Backend Contract v0.4 — M7 consolidated LLVM / C ABI / foreign-boundary lowering

Status: **M7 backend/interop closure contract**. This document consolidates M7.3–M7.5 lowering obligations after adjudication against NewLang Draft 17.4. It is not a NewLang source-language mechanism.

## 1. Governing rule

NewLang capabilities and LLVM optimizer promises are not isomorphic.

```text
write permission          != access exclusivity
source exclusive authority != LLVM noalias by spelling alone
ref nonescape             != pointer-provenance nocapture
NewLang object lifetime   != llvm.lifetime.* in general
```

The backend may erase NewLang proof/capability state only after translating it to LLVM facts that are **no stronger** than the checked NewLang semantics.

## 2. Baseline matrix

| NewLang fact | Baseline LLVM lowering | M7.3 decision |
|---|---|---|
| `ptr<T>` | plain `ptr` locator | no `noalias`, `readonly`, `dereferenceable`, or capture claim by type alone |
| `ref<read,T>` | pointer access; parameter may be `readonly` | **not** `noalias`; capture is body-sensitive |
| `ref<write,T>` | ordinary read/write pointer access | **not** `noalias`; **not** `writeonly` because write refs may be read/weakened |
| `exclusive ref<...>` | same access mode as the ref | source `exclusive` alone does **not** imply LLVM `noalias` |
| checked access-exclusive range fact | `noalias` or scoped alias metadata may be considered | only if checker proves the LLVM dynamic-extent/access-set condition |
| stable read | statically prevents conflicting NewLang update | `readonly` is sufficient baseline; `llvm.invariant.*` is deferred optimization |
| lifetime-ending authority | compiler semantic authority | no direct LLVM attribute |
| NewLang typed lifetime start/end in heap/raw `Storage` | semantic only | **do not** emit `llvm.lifetime.start/end` |
| compiler stack-allocation storage lifetime | stack storage marker | may emit `llvm.lifetime.start/end` when exact alloca storage lifetime matches |
| ref capture | infer from body | `captures(none)` only if body analysis proves no address/provenance capture |
| positive extent | target/layout fact | no baseline `dereferenceable(N)` until null-address/ABI policy is owned by target lowering |

## 3. `ref<write,T>` must not lower to parameter `noalias`

NewLang intentionally permits aliasing write references.  Existing source fixture `13_semantic_aliased_actuals_conflict.nl` passes the same write reference as two arguments and is accepted.

LLVM parameter `noalias` is a substantially stronger promise: for memory modified during the function, conflicting accesses through pointers not based on the noalias argument make the IR behavior undefined.

M7.3's destructive probe passes the same pointer to two LLVM functions:

```text
ordinary pointers:
    O0 result component = 2
    O2 result component = 2

incorrect `noalias` parameters:
    O0 result component = 2 (optimizer did not exploit the invalid promise)
    O2 result component = 1 (optimizer validly exploits `noalias`)
```

The combined process exit changes from `202` at `-O0` to `201` at `-O2`, and optimized IR reduces the incorrectly annotated function to `ret i32 1`.

This is not an LLVM miscompile.  The frontend would have produced IR whose `noalias` precondition is false for a NewLang-defined program.  Therefore the lowering itself would be wrong.

## 4. `ref<read,T>` may use `readonly`, but not `noalias`

LLVM `readonly` only says the function does not write *through that pointer argument*; it does not say other aliases cannot modify the same memory.

The M7.3 probe uses one `readonly` pointer and one ordinary writer pointer that alias.  At `-O2` the load correctly observes the value written through the other pointer (`2`).

This is a good one-way lowering fact for a NewLang read ref: NewLang promises at least that code cannot write through the read ref.  NewLang's stronger stability semantics remain a checker fact and do not require a stronger LLVM alias promise.

## 5. `exclusive ref` and LLVM `noalias`

`exclusive` in NewLang is an authority/resource property.  LLVM `noalias` is an access-set promise over a dynamic execution extent.

M7.3 therefore requires a separate checked-MIR fact such as conceptually:

```text
AccessExclusive(range R, dynamic extent E)
```

before emitting LLVM `noalias` or equivalent `alias.scope`/`noalias` metadata.

The source spelling `exclusive ref<...>` alone is insufficient.  This keeps lifetime-ending authority, aliasing locators, and memory-access exclusivity as distinct axes.

## 6. source ref nonescape does not imply LLVM nocapture

NewLang refs themselves are scoped/nonescaping, but a persistent locator may be derived from a ref:

```text
fn save_locator(value: ref<read,Box>) -> ptr<Box> {
    ptr_from_ref(value)
}
```

M7.3 fixture `241_ref_locator_capture_ok.nl` is accepted.  Therefore a ref parameter can cause pointer provenance to escape even though the ref capability itself does not.

Do not emit LLVM `captures(none)` from the source type alone.  Capture attributes are body-sensitive optimization facts.

## 7. `dereferenceable` / `nonnull`

A NewLang ref denotes valid access in the source semantics, but LLVM `dereferenceable(N)` has target-level consequences.  In default address space it normally implies non-null unless `null_pointer_is_valid` is present.

Because NewLang targets embedded/bare-metal code and should not accidentally rule out a platform where address zero is usable storage, M7.3 does not emit `dereferenceable` or `nonnull` from ref type alone.

A target-aware lowering phase may add them after it owns:

- concrete positive extent;
- address-space rules;
- target null-address policy;
- the relevant dynamic program point.

## 8. NewLang lifetime versus `llvm.lifetime.*`

Current LLVM LangRef restricts `llvm.lifetime.start/end` operands to stack objects defined by `alloca` or `llvm.structured.alloca`.

Therefore:

```text
NewLang initialize(slot<T> on heap/raw Storage)
NewLang take/destroy of typed object
CurrentValueVersion reset
```

must **not** be lowered as general `llvm.lifetime.start/end` operations.

The intrinsics may be used when the compiler has an actual stack allocation and the intrinsic describes that stack storage lifetime exactly.  This is an optimization/storage-lifetime mapping, not the normative representation of a NewLang typed-object incarnation.

## 9. alias-scope metadata

`!alias.scope` / `!noalias` metadata can also introduce undefined behavior if the promised access sets alias.  They are not a workaround for the ordinary-ref rule.

They may be generated only from a separate proven disjoint-access fact in checked MIR, for example one derived from genuinely disjoint BackingRegions/ranges and valid for the exact instruction scope.

## 10. Deferred optimizations

M7.3 intentionally does not rely on:

- `llvm.invariant.start/end` for read stability;
- `nofree` for stability/lifetime loans;
- `writable` for write refs;
- automatic `dereferenceable`/`nonnull`;
- TBAA as a representation of NewLang ownership;
- automatic alias scopes from source ref kinds.

These may be revisited as performance optimizations after a production Checked MIR exists.  Omitting them is safe; emitting an unjustified one can make a valid NewLang program LLVM-UB.

## 11. Production compiler consequence

The future compiler should preserve at least these independent backend facts until Checked MIR:

```text
access mode
proven access exclusivity (optional)
capture behavior
known extent/alignment
target null-address policy
storage kind (stack allocation vs heap/external/subobject)
NewLang semantic lifetime state
```

Only the proven LLVM-relevant subset is emitted.  This supports the planned C-written production compiler without forcing LLVM's memory model back into the NewLang source semantics.

## 12. References used for M7.3

- LLVM Language Reference Manual, `noalias`, pointer capture, `readonly`, object lifetime, and memory-use markers: https://llvm.org/docs/LangRef.html
- NewLang Draft 17.3 and the M7.2 prototype/test corpus.

---

# M7.4 addendum — C ABI / calling convention / by-value aggregate boundary

## A. Native aggregate layout is not a C ABI representation

NewLang ordinary/native aggregate layout remains opaque.  Crossing an `extern C`-style boundary does not make a native aggregate layout-compatible with a C `struct` or `union`.

Conceptually:

```text
NewLang native value
    -> semantic field/variant marshalling
explicit C compatibility representation
    -> target C ABI classification
target machine call/return
```

The exact source spelling of the compatibility representation is not fixed by M7.4.  It may initially be produced by a compatibility frontend or generated C shim.

A raw/native bitcopy is not the default marshalling rule.  Padding bytes are representation details, not NewLang semantic value.

## B. ABI classification is a backend result

The following are not source-language ownership/lifetime properties:

```text
LLVM sret
LLVM byval
LLVM inreg
register coercion/splitting
hidden result pointer
caller-created aggregate copy
```

They arise from the selected target ABI and calling convention.

The same source-level C aggregate may have materially different LLVM signatures on x86-64 SysV, Windows x64, AArch64, and RISC-V.  Checked MIR therefore preserves an explicit foreign ABI identity/profile, while the target backend or generated C shim owns classification.

## C. ABI-originated alias facts remain distinct from NewLang refs

A target C ABI may impose a strong hidden-result-storage contract.  Clang may therefore represent an ABI-lowered hidden result as an `sret ... noalias` pointer.

This does not weaken M7.3:

```text
NewLang ref/exclusive spelling
    !=
LLVM noalias
```

Instead the `noalias` fact has a different proof origin:

```text
foreign ABI hidden-result rule
    -> backend ABI fact
    -> LLVM noalias where valid
```

Proof origin must be retained in Checked MIR/backend planning; ABI facts must not be reflected back into NewLang source permission semantics.

## D. v0 boundary policy

Baseline policy:

```text
C scalar / explicit C pointer
    -> direct C ABI lowering is permitted

explicit C record/union
    -> direct only when a target ABI classifier is available
    -> otherwise generated C shim

native NewLang aggregate/sum
    -> no direct by-value ABI identity
    -> explicit C representation + semantic marshalling

NewLang semantic-resource-bearing value
    -> no automatic aggregate-byte ABI conversion
    -> dedicated privileged FFI contract required

by-value aggregate varargs
    -> generated C shim in v0 baseline
```

A native sum is not automatically a C union: active-variant semantics and representation must be mapped explicitly.

## E. Initial production compiler strategy

The first production compiler need not reimplement every platform aggregate classifier.

A conservative path is:

```text
NewLang frontend / Checked MIR
    -> generate narrow C ABI shim
    -> host/target C compiler performs exact C aggregate ABI lowering
    -> link with NewLang-generated objects
```

This is especially attractive for the planned C implementation of the production compiler.  Direct ABI classification can be added target-by-target later and differential-tested against the C shim/Clang oracle.

## F. Calling convention contract

A foreign declaration records its ABI/calling-convention identity explicitly in semantic/backend state.  LLVM declaration and call-site calling conventions and all ABI-impacting attributes must match the target contract.

NewLang native calling convention is independent and may evolve without changing the C ABI compatibility representation.

## G. M7.4 executable evidence

M7.4 cross-target Clang probes observe at least three different LLVM signatures for the same C `PairD { double, double }` by-value identity across:

```text
x86_64 SysV
Windows x64
AArch64
RISC-V 64
```

A 20,000-case randomized layout pressure test reorders native fields independently from the C declaration and explicitly marshals semantic field values.  Result:

```text
roundtrip mismatch: 0
native bitcopy not equivalent: 17,187 cases
```

A separately compiled host C API plus generated shim also passes while the simulated native record deliberately has a different size and field order from the C record.
---

# M7.5 addendum — FFI / external backing / opaque-call summary boundary

## H. Boundary rule

M7.5 does not add a general source-visible FFI/effect system.  It fixes the compiler/backend boundary rule:

```text
foreign behavior known by an explicit checked contract
    -> preserve the narrow facts independently
    -> feed semantic effects to the NewLang checker
    -> feed only matching backend facts to LLVM

foreign behavior not known
    -> Unknown on the affected semantic axis
    -> omit unsupported LLVM promises
    -> reject only when the unknown behavior could invalidate a live NewLang obligation
```

A foreign function signature, pointer type, `ref` spelling, or C ABI identity does not by itself prove semantic non-mutation, no-retain, no-callback, or no-nonlocal-transfer behavior.

## I. Foreign summary dimensions stay separate

Checked MIR/backend planning must not collapse the foreign boundary into a single `pure/impure` bit.  At minimum the following facts have independent proof origins:

```text
semantic state effect
    none / Change(P) / Reset(P) / EndRoot(P) / Unknown

backend memory access
    none / read / readwrite / Unknown

pointer/location retention
    no-retain / may-retain / Unknown

callback behavior
    none / synchronous-known / retained-or-escaping / Unknown

control transfer
    ordinary checked return / foreign exception-or-unwind / longjmp-or-other nonlocal transfer / Unknown

object construction
    representation completeness
    semantic ValuePackage completeness

external backing
    backing liveness
    range / alignment / access properties
    ordinary-safe non-alias proof
```

One axis must not be inferred from another.  In particular:

```text
NewLang semantic effect = none
    != LLVM memory(none)

NewLang call returns through the checked continuation
    != LLVM nounwind

source ref is block-scoped
    != foreign no-retain
```

## J. Opaque-call policy

For a call without an adequate foreign summary, the compiler uses `Unknown` only on the axes that can affect the exposed state.

Examples:

```text
opaque call receives only scalars and an opaque foreign handle
    -> mutation of NewLang safe places may be irrelevant

opaque call receives a safe NewLang place/location
    -> unknown semantic mutation/lifetime effect may invalidate live dependencies

opaque call receives a scoped ref/raw out location
    -> no-retain must be proved or the checked boundary rejects

opaque call receives a nonescaping callable
    -> synchronous/non-retained callback behavior must be proved
```

M7.5 deliberately keeps conservative rejection preferable to inventing a general source-visible effect language merely to make opaque calls easier to accept.

## K. External backing policy

Ordinary-safe `BackingRegion` import is permitted only while the platform/FFI boundary proves the existing invariant:

```text
distinct live ordinary-safe BackingRegion identities
    => disjoint abstract backing bytes
```

The boundary must additionally establish backing liveness, range, alignment, and access properties required by the imported storage.

Different numeric or virtual addresses do not prove different abstract backing.  Shared mappings, device windows, foreign aliases, and other views that cannot satisfy the ordinary-safe non-alias obligation remain as opaque external views outside the safe occupancy model.

This does not weaken the ordinary-safe BackingRegion invariant and does not require a general multi-view alias model in v0.

## L. Foreign out construction / lifetime start

M7.5 retains the Draft-17 reservation:

```text
slot<T>
  -> FFI/platform raw destination location   // not ptr<T>
  -> foreign writes
  -> validate complete representation
  -> establish complete semantic ValuePackage<T>
  -> privileged lifetime-start transition
  -> fresh root incarnation
  -> first safe ptr<T>
```

Partial writes do not create a partially-live typed object.  A raw location retained by foreign code blocks the safe lifetime-start transition unless a future ownership/retention mechanism models that relationship.

Foreign representation bytes never synthesize value-owned semantic authority such as `Allocation`, `Storage`, `LifetimeDomain`, pointer provenance, or hidden dependencies.

## M. LLVM lowering rule

LLVM attributes are emitted only from independently proved backend facts whose dynamic contract matches LLVM semantics.

Current LLVM semantics make omission the conservative representation for several axes:

```text
no memory attribute
    -> memory(readwrite) is implied

no captures restriction
    -> pointer provenance/address capture remains possible

no nounwind
    -> exception unwind is not ruled out

no nocallback
    -> callback/nonlocal reentry is not ruled out by that attribute
```

A checked foreign summary can justify an LLVM promise only when the proof is at least as strong as the LLVM promise.  The proof origin remains recorded in Checked MIR/backend planning.

Important non-equivalences:

```text
semantic Change/Reset/EndRoot summary
    -> not itself an LLVM memory attribute

foreign no-retain proof
    -> may justify captures(none)/version-equivalent capture restriction
    -> only for the exact pointer copy/argument covered by the proof

foreign no-exception-unwind proof
    -> may justify nounwind
    -> does not by itself prove absence of longjmp/nonlocal transfer

foreign no-callback/reentry proof
    -> may justify nocallback where LLVM's contract matches
    -> NewLang still retains its own boundary fact because backend attributes are not the source semantics
```

## N. Destructive optimizer evidence

M7.5 intentionally gives Clang a false foreign summary in one probe:

```text
actual implementation: writes *p += 5 and returns 7
caller declaration:    __attribute__((pure))
```

Observed with Clang 17:

```text
-O0: x=15 r=12, exit 0
-O2: x=15 r=7,  exit 1
```

The optimized caller receives LLVM `memory(read)` and eliminates the post-call reload.  The test is deliberately undefined by the false annotation; its purpose is to demonstrate that an unjustified foreign summary can change generated behavior.

The same caller with an unannotated external declaration retains both loads and produces the expected result at `-O2`.

## O. External-alias executable evidence

A separate probe maps the same shared file backing twice with `mmap` and observes:

```text
aliases=1
distinct_virtual_addresses=1
```

Thus external backing identity cannot be inferred from numeric/virtual address inequality.

## P. Production compiler consequence

The planned C-written production compiler does not need a general FFI theorem prover or source-visible effect system in its first version.

A conservative implementation can preserve a small internal contract object per foreign declaration/wrapper containing independent optional facts for:

```text
semantic effects
memory access
capture/retention
callback behavior
control transfer
construction certification
external-backing proof origin
```

Unknown/missing facts remain unknown.  Generated C shims remain useful for expressing platform ABI details, but a C declaration attribute is not automatically a NewLang semantic proof; wrapper generation must be driven by already-checked contract facts.

## Q. M7.5 closure

M7.5 supports the following closure:

```text
FFI stays a localized boundary
+
external aliases need not weaken ordinary-safe BackingRegion
+
opaque calls use narrow explicit summaries or conservative Unknown
+
retention / callback / control-flow are separate facts
+
foreign bytes do not manufacture semantic authority
+
LLVM attributes are downstream proof products, not source semantics
```

No Draft-18 semantic redesign or general source-visible effect system is required by this pressure point.


---

# M7 consolidation addendum

## R. Adjudicated ownership of rules

The M7.3–M7.5 findings are intentionally split across three layers.

### Normative NewLang semantics

Draft 17.4 owns only rules that affect whether a NewLang program is well-formed independent of LLVM or a particular C ABI:

```text
write permission != exclusivity
native aggregate/sum != implicit C ABI representation
foreign signature/ABI identity != semantic summary
external safe BackingRegion import requires liveness/range/alignment/access/nonalias proof
missing external-call preservation proof -> Unknown
foreign bytes != semantic ValuePackage
safe typed lifetime starts before the first safe ptr<T> is exposed
```

These rules must remain true if LLVM is replaced or a target uses a different foreign ABI.

### Backend contract

This document owns target/lowering facts such as:

```text
LLVM noalias / alias.scope / capture restrictions
readonly / dereferenceable / nonnull decisions
llvm.lifetime.* eligibility
sret / byval / inreg / register coercion
LLVM memory(...) / nounwind / nocallback mapping
target calling-convention identity
generated C shim strategy
proof-origin tracking in Checked MIR
```

These are not reflected back into the source permission/type model.

### Deferred surface / implementation-later

Draft 17.4 deliberately does not freeze:

```text
persistent function-pointer source API
normative basic FFI declaration/contract syntax
general external/static-backing ergonomic API
retained/asynchronous callback semantics
foreign unwind / longjmp integration with scope obligations
general multi-view backing / MMIO model
safe partial/in-place construction
direct target-specific aggregate classifier
by-value aggregate varargs direct lowering
```

The last two items are backend implementation-later rather than semantic blockers; generated C shims are an acceptable first production path.

## S. Minimum Checked MIR separation

A production Checked MIR must not encode the M7 closure as one "safe foreign" or "exclusive" boolean.  At minimum it needs enough independent state to avoid strengthening one proof into another.

Conceptually:

```text
SourceSemanticFacts {
    access_mode
    lifetime/root state
    semantic dependencies/effects
}

BackendProofFacts {
    proven_access_exclusivity?
    capture_behavior?
    known_extent/alignment?
    target_null_address_policy?
    storage_kind
}

ForeignBoundaryFacts {
    abi_identity
    semantic_effect_summary?
    backend_memory_access?
    retention/capture_contract?
    callback_contract?
    control_transfer_contract?
    construction_certification?
    external_backing_proof?
}
```

The exact C struct/IR representation is implementation-defined.  The required property is **proof-origin separation**: absence of one fact cannot be repaired by guessing from another.

## T. Conservative lowering default

For the first production compiler, omission is preferred to an unjustified optimizer promise.

```text
not proved noalias      -> emit no noalias promise
not proved nocapture    -> emit no capture restriction
not proved memory effect -> use conservative memory behavior
not proved nounwind     -> do not claim nounwind
not proved nocallback   -> do not claim nocallback
not classified aggregate ABI directly -> generated C shim
```

This may leave optimization performance on the table.  It does not change language semantics and can be improved target-by-target after Checked MIR facts are available.

## U. Closure gate for the next milestone family

M7 is considered closed when all of the following remain true:

1. Draft 17.4 contains no LLVM-specific source semantics.
2. The backend contract contains no rule that weakens Draft 17.4 safety obligations.
3. FFI/function-pointer/general external-backing surface remains Deferred unless a later workload demonstrates a concrete need.
4. New backend optimizations are justified from independent checked facts and have destructive/differential tests where a false promise could create UB.
5. Production compiler architecture may begin without solving every target ABI directly; a C shim oracle/fallback is permitted.

A later milestone may reopen one item only with concrete implementation or workload pressure, not merely because a richer abstraction is available.
