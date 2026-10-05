# NewLang v0 Draft 17.6 review resolution

## Status

**Minor normative raw-storage surface / validity extension.**

Draft 17.6 is based on the canonical `NewLang_v0_spec_Draft17_5.md` on `main`.
It integrates the M8.3R targeted reopen that followed the M8.6 representative systems workload validation.

Draft 17.5 exclusive-ref call-boundary reborrow semantics are preserved unchanged.

## R17.6-1 — Workload gap

M8.6 found one source-surface gap in the W7 raw protocol / serialization workload.

The closed raw `Storage` surface could:

```text
observe storage length/address
copy raw representation state
```

but could not express the two scalar codec directions:

```text
raw representation byte
    -> semantic byte scalar

semantic byte scalar
    -> raw representation byte
```

`copy_raw_bytes` cannot fill this role because it deliberately does not interpret representation state as a semantic value.

Using `Storage -> slot<byte> -> initialize` would also be wrong: it starts a typed object lifetime rather than observing/updating raw representation state.

Therefore the finding is a **source-surface gap**, not a new lifetime / authority / provenance mechanism.

## R17.6-2 — Canonical scalar bridge

Draft 17.6 adds:

```text
storage_read_byte(
    storage: ref<read, Storage>,
    offset: usize
) -> byte

storage_write_byte(
    storage: ref<read, Storage>,
    offset: usize,
    value: byte
) -> unit
```

Both are Storage-relative raw-representation operations.

They do not:

- consume or replace `Storage`;
- create a Storage subclaim;
- create a typed object lifetime;
- create a `ValuePackage`;
- mint `ptr` / `ref` provenance;
- expose a general raw pointer or pointer arithmetic.

Both use `ref<read,Storage>` because the ref capability applies to the **Storage semantic claim value**.
`storage_write_byte` mutates backing representation under a separate ordinary raw-write access property; it does not replace the Storage value.

## R17.6-3 — `byte` meaning

Draft 17.5 already has `byte` as a Copy + Discardable core type distinct from the integer families, but does not fully define its value domain.

Draft 17.6 completes the relevant v0 meaning:

```text
byte
    = one ordinary raw representation octet value
    = exactly 256 possible values
    Copy
    Discardable
    no arithmetic implied
```

For the v0 ordinary Storage target profile:

```text
one Storage byte = one 8-bit octet
sizeof(byte) == 1
alignof(byte) == 1
```

`byte` remains distinct from `u8`.

Explicit total bijective conversions are:

```text
u8(byte_value)
byte(u8_value)
```

This preserves the source-visible distinction between representation scalar and numeric computation without introducing a new primitive type.

## R17.6-4 — Defined vs Unspecified

Raw representation validity is conceptually:

```text
RawRepValidity =
    Defined
  | Unspecified
```

`Defined` means that the runtime byte has one well-defined value.
It does **not** mean that the compiler knows the exact constant.

Fresh allocation and raw bytes obtained after typed lifetime end may be `Unspecified`.

Ordinary-safe scalar read requires `Defined`.

```text
DefinedByte(b)
    -> storage_read_byte yields b

Unspecified
    -> ordinary-safe scalar read is not applicable
```

Draft 17.6 rejects the alternative where safe code observes `Unspecified` as an arbitrary byte, because that could expose stale allocator/object residue and would turn a validity error into a silent semantic value.

`storage_write_byte` does not require pre-existing Defined validity and establishes:

```text
RawRepState(offset) = DefinedByte(value)
```

for the selected byte.

## R17.6-5 — Hidden definedness facts

Raw-definedness is a semantic validity fact, not an authority value.

The compiler may represent it internally as something like:

```text
RawDefined(referent, [start,end))
```

and may approximate it as whole-range, prefix, interval, singleton, or Unknown.

The revision does **not** add:

- source-visible `RawRange`;
- `DefinedStorage`;
- a raw initialization typestate;
- a mandatory runtime initializedness bitmap;
- a mandatory exact per-byte compiler bitmap.

If proof is lost, the compiler must reject rather than assume Defined.
An explicit `unchecked` boundary may take responsibility when an external/runtime contract establishes the fact.

## R17.6-6 — Function boundaries

Ordinary codec functions must not require `unchecked` at every byte access.

The compiler may therefore propagate body-derived raw applicability requirements through internal semantic summaries.

For example a codec that reads two bytes may conceptually require:

```text
bounds for [offset, offset+2)
RawDefined(storage, [offset, offset+2))
ordinary raw-read access
```

This is not a new source-visible effect/precondition language.

## R17.6-7 — External byte-producing boundary

A future platform / I/O / FFI operation that actually writes defined external bytes may establish a `RawDefined` postcondition for the written range.

For example:

```text
successful input count
    -> RawDefined(destination, [0,count))
```

This does not mint Storage authority, typed lifetimes, `ValuePackage`, or provenance.

No general I/O or FFI source API is introduced by Draft 17.6.

## R17.6-8 — `copy_raw_bytes` remains distinct

`copy_raw_bytes` continues to copy raw representation state without interpreting it.

Normatively:

```text
source DefinedByte(b)
    -> destination DefinedByte(b)

source Unspecified
    -> destination Unspecified
```

Thus raw copy may legally propagate `Unspecified`.

Scalar semantic read cannot.

Therefore:

```text
storage_read_byte + storage_write_byte
```

is not generally equivalent to `copy_raw_bytes`.

## R17.6-9 — Preserved boundaries

Draft 17.6 does not add:

- pointer arithmetic;
- byte/raw pointers;
- persistent raw range/span;
- bytes-to-typed overlay;
- `repr(C)` / packed aggregate layout;
- general transmute;
- typed raw u16/u32/u64 loads/stores;
- native-endian raw access family;
- general `Layout<T>` / reflection;
- FFI subsystem.

MMIO / volatile / device-register memory remains outside ordinary raw-memory operations unless a target/platform contract explicitly defines compatible semantics.

## Workload consequence

The M8.3R validation demonstrated the selected surface on:

- big-endian scalar codec helpers;
- explicit header encode/decode;
- database/page fields;
- packet headers;
- boot-image magic;
- checksum preparation.

No native aggregate overlay is required.

## Classification

```text
new authority mechanism:          NO
new lifetime mechanism:           NO
new provenance mechanism:         NO
new core scalar type:             NO
existing byte semantics refined:  YES
new raw scalar operations:        YES (2)
raw validity rule refined:        YES
general raw-memory redesign:      NO
Draft-18-level revision:          NO
```

## Follow-up

After Draft 17.6 is reviewed and merged, M8.6 should resume only its targeted revalidation:

```text
W7 raw protocol / serialization
W11 ordinary platform raw-buffer codec portion
W12 MMIO/platform exclusion regression
raw-vs-typed boundary regression
copy_raw_bytes regression
```

Unrelated workloads do not need a full rerun unless this change exposes a new interaction.
