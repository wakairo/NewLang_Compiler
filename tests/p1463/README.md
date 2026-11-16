P1463 covers one original two-u8 record with the same named write-u8 Copy
installer before and after a closed two-allocation H result. It uses the
existing source/CHECK/Copy/result seals; the backend ledger only schedules
their consumption. The record can place the written field at index 0 or 1.
The H result remains statically Unknown, and both real Bool selection results
are exercised. All H owners and loans close before the second installer.

This is a post-cleanup publication contract: allocation failures deliberately
perform an idempotent 0-to-0 final write. It does not implement P1459's in-H
publication, arbitrary extra calls, nested caller composition, or a general
owner/world join. Unsupported shapes must retain their existing refusal.

`probe.c` checks genuine public MACHINE, both Copy body/parameter/event
validators, exact original/incarnations, closed loans, scalar result seals,
foreign field/call/world/issuer rejection, and three semantic negatives with
same-context rollback/retry. Its four selected OOM points are CHECK realloc,
the actual post-H Copy Change replay allocation, and C17 text allocations 1/2.
The emitter faults compare the completed CHECK world immediately around
emission; the CHECK fault compares the pre-CHECK world.

`native.py` uses ordinary newlangc output without editing its bytes. GCC/Clang
CTest configurations compile it as strict C17. Passive macros observe actual
original pointers, member addresses, Domain tokens, ordered link Changes,
LiveTail handoff, old/current/sibling, and the real final free calls. Only
test allocation failure is injected; no source result is synthesized.
Wrapper-shared observer state is volatile because linker wrapping free may
otherwise invalidate the optimizer's library-call assumptions. The original
failed observer and the phase-mismatched OOM snapshot remain in `first/`.

The local receipt distinguishes successful finite evidence from preserved
apparatus REDs, untested profiles, and Product GO. No SHA-history generation
protocol or independent R/F fixture is introduced.
