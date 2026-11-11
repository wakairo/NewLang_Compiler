This is the CURRENT successor for sequential readonly-u8 current-loop calls.
The two call sites keep distinct actual bodies, fresh formal places, ended
callee originals, closed loans, normal-tail Copy results and custody seals.
The same live caller original may change between calls according to its actual
checked Change records. The first plain Copy may outlive its callee and remain
usable after the second call.

The finite supported boundary is two sequential calls to the same ordinary
one-readonly-u8 function, each with its own one-field/literal-terminal current
loop, and at most one preserved caller two-u8 original. Reference parameters,
cross-call loans, extra caller originals, third calls, H composition and
recursion are outside this increment. No public issuer or source rule changes.

`oracle.json` freezes all six sources, expected values, failures and test
budgets. `owned.c` checks genuine owner/current/Copy and call-slot custody,
including corrupted call/intervening Change evidence, OOM rollback and retry.
`native.py` compiles unmodified generated C under strict C17, both raw and
through existing passive observation hooks. `observer.h` clears callee
pointers while still live; it never compares or dereferences expired pointers.
Stack-address reuse and equal return bits across calls are legal. Distinct
evidence is checked by IDs and seals, not by unequal addresses or return bits.

`current-expectations.json` records the prospective migration of two historical
legal precision-negative sources to owned/current positives. The original
P1339/P1343 sources, oracles, runners and FIRST evidence remain unchanged.
`failure.py` preserves every other original semantic/scope rejection and all
original allocation/write/flush, output-byte/inode/mtime, no-clobber and retry
checks. Ordinary safety checks remain mandatory; historical SHA audits are
governed separately by the Git-native policy.

Finite successes demonstrate this compiler capability. They do not establish
general soundness, audit locality, Product GO or H1 adoption.
