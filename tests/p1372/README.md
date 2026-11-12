P1372: ordinary callee-local ending in a current-u8 call
=====================================================

This bounded increment reuses CopyBody's existing INITIALIZE/READ/CHANGE
replay, entry/caller seals and EndLocals. The callee has one readonly u8
formal, one local two-u8 original of the caller's nominal, one scoped field
Change, and a normal identifier Copy tail from that Change's real old value.
It contains no inner loop, wrapper, call, IF or explicit RETURN.

The outer current relation authenticates the original current-field read,
actual/formal handoff, returned free Copy, caller Change and Continue closure.
The local profile validates its Copy replay directly rather than inventing
an inner current loop or recursing through the outer current validator.
Signature allocation origins/extents are checked before dependent reads.
The CLI consumes this same replay and retains its existing one-nominal C17
carrier, ordinary aggregate/loan/load/store and scalar return emitters.

`L-straight-local-old.nl` is the unchanged 1098-byte P1370 source. The second
positive reverses the field index, changes all constants and carries aliases
on the input, local initializer, old-result tail and caller return. The
original oracle and baseline predictions are immutable; the baseline's
observed exact type diagnostic and the distinct-world negative boundary are
explained prospectively in `runtime-expectations.json`.

The ten units cover genuine owned/current/Copy/ending, malformed retained
metadata and source, restoration, selected registration/actual/Copy replay
OOM, rollback and retry. The three integrations cover strict unedited
raw/passive native, semantic rejection and named-output malloc/write/flush
faults, and 22 previously measured C17 outputs. The passive observer compares
caller/local addresses only while both objects are live and clears local
pointers before the real return. Static C def-use checks separately trace
field read -> actual -> formal -> local initializer -> old load -> Copy tail
-> return -> caller store; equal numeric bits alone do not establish lineage.

This is finite capability/safety evidence. It does not implement W, nested
wrappers/loops, extra originals, multiple calls, callee conditionals/RETURN,
general owners/H/CFG or a new public source/API/ABI/issuer. Selected OOM points
do not prove all allocations. Native success is not general soundness or a
Product PASS for UB-prevention audit locality. Product GO PAUSED / H1 HOLD.
