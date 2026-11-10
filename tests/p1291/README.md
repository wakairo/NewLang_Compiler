# P1291: a genuine scalar result after completed H matches

`sources/A-original.nl` retains the exact P1289 pre-execution source bytes.
`B-index-value-control.nl` changes the values, helper spelling and T field
declaration order. Each program keeps `main() -> unit`, two nested fallible
allocations of the same H, and an arm-local two-u8 T original. Only the
dependency-free u8 result reaches the ordinary continuation.

The checker verifies every normal arm against its unchanged or closed
ancestor frame and creates a fresh unknown parent scalar. The backend
serializes each actual arm tail once, then assigns its value to a common
scalar after that arm's cleanup. It does not select a success-world fact,
export T/ref/owner/Domain state, or substitute an expected result literal.

Expected runtime results are A: H0=11, H1=23, H2=old53 (current173,
sibling197), and B: H0=7, H1=19, H2=old61 (current131, sibling181).
The continuation bindings run after respectively zero, one or two frees.

The registered tests cover ordinary CLI CHECK, genuine public Copy/current
validators, six actual source-arm result edges, modified/foreign graph
refusal, canonical destruction with shortened/increased view cardinality,
strict unmodified C17 native before passive H0/1/2 observation, and actual
API/named-output OOM/rollback/no-clobber/retry. Native probes use both O0 and
O2 with the configured GCC/Clang and sanitizer. Observations qualify symbols
by their source frame and do not dereference an ended T or H object.

OOM plans are written before each sweep from a fresh source/binary census:
API CHECK samples every allocator PC plus first/last and spread ordinals;
the emitter sweeps every ordinal. Named CHECK samples first/last8 and a
midpoint, and named emission sweeps every ordinal. These are finite tests,
not claims of all possible allocation failures or general soundness.

Original source/oracle/FIRST and the first C17 refusal remain preserved.
The prospective native and failure oracles do not revise the original one.
Independent R inputs are not used. General results, nonCopy/owner/ref/T-root
phi, current-loop integration, new issuer/API/ABI and arbitrary CFG remain
outside this implementation.
