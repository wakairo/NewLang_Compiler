This bounded increment composes the existing current/Copy/custody relation
through main -> zero-argument u8 wrapper -> one readonly-u8 current-loop call.
The wrapper keeps its own original across the inner invocation, applies its
real Change using the returned old Copy, and returns a normal-tail plain Copy.
The inner original and program root retain their true identities and custody.
No issuer, public API/ABI, source law or independent proof is added.

The Copy record's borrowed current_call identifies the canonical existing
child. Origin-only path checking avoids validator cycles; full Copy replay
consumes the inner current/Copy evidence before replaying the wrapper Change
and result. C17 traverses that same authenticated child to register metadata;
only that validated wrapper permits scalar current depth two.

All original source/oracle/FIRST data remain in Git and the evidence packet.
The original provisional oracle predicted too few outer validator allocations
and the wrong diagnostic name for a declared-result mismatch. The CURRENT
successor records actual 40 allocations (including existing Change replay)
and the unchanged baseline P8-BODY-RESULT contract. Every actual allocation
is still fault-injected with strict OOM, rollback, out preservation and retry.
The C-origin collector is scoped to the caller function; its old false
positive, which also collected a later callee's store, is retained.

The passive observer reads both physical originals only while alive and clears
their pointers before return. Only copied scalars are used after wrapper exit.
Legacy generated-C hashes and first-Copy binding/use origins protect existing
direct one/two-call behavior. Coverage is finite; multiple wrappers/inner calls,
H composition, extra main aggregates, reference parameters and general CFG
remain outside this increment. Product GO PAUSED and H1 HOLD remain.
