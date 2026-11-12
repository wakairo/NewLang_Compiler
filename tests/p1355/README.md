# One input through a current/Copy wrapper (P1355)

A one-u8 ordinary wrapper can keep its own two-u8 original alive while calling
the existing one-u8 current-loop function. The inner function returns its real
terminal old Copy; the wrapper uses it to update its own original, closes the
loan, and returns the Copy to its caller. This implements selected Draft17.35
ordinary calls, fresh formals, Change/current and normal-tail Copy semantics.

The source constructor filter captures the existing readonly entry for this
forwarding frame. The same current/Copy/custody record authenticates the exact
program root, main/wrapper/inner BODY and call slots, fresh formals, canonical
graphs, original fields, current signatures and final result seals. Signature
allocation authentication precedes reading current signature elements. The
readonly entry check does not call the full current/Copy validator, avoiding a
validator cycle. No public API, issuer, proof structure or emitter changes are
needed.

## Measured support and limits

`A-input83.nl` preserves P1354's source bytes. `B-input137-index1.nl` uses a
different input, opposite field and three backedges. In both cases there is one
main call, one one-u8 wrapper with one local original, and one one-u8 inner call
with its own original. Both end their loans and originals before their free
Copy escapes.

| Source | Inner old/current/sibling | Wrapper old/current/sibling | Caller Copy |
| --- | --- | --- | --- |
| A | 83 / 227 / 191 | 47 / 83 / 83 | 83 |
| B | 137 / 239 / 179 | 61 / 137 / 137 | 137 |

Two inner calls and another wrapper depth remain legal-source precision gaps;
their tests expect a precision refusal with complete rollback. Wrong argument
types and an escaping ended loan remain semantic rejections. Multiple formals,
reference writes across calls, main-local aggregates/H, general recursion and
CFG are outside this increment. Zero-argument and direct current paths remain
covered by their existing tests.

## Validation

Candidate `3167b7b90e59767dca7600a1ce4c6d82ca9d93a3` passed the seven new
unit tests and the 70 selected related unit tests plus nine failure/compatibility
integrations under GCC Release, ASan+LSan and UBSan. The selected 70 include
the seven new tests; they are not 77 distinct tests.

For each source/host, CHECK's fresh census was 1598 allocations. Sixteen selected
points include both readonly entry/anchor intervals, original initialization,
Copy/custody seals and the last allocation. Every reached allocation in the
outer Copy validator (43) and inner validator (3) was injected. Nil/out/context
preservation, body ownership rollback, cleanup and retry passed. Named-output
malloc/write/flush failures preserve bytes, inode and mtime, remove temporary
outputs and retry. Twelve older generated C files remain byte-identical.

Twenty local guest runs passed: raw and passive GCC/Clang C17 O0/O2 for both
sources (16), plus passive guest ASan+LSan/UBSan (4). The observer compares
distinct originals only while both are alive and clears pointers before the
function ends. After return it examines only counters and copied scalar values.
The native runner also checks generated actual/formal and binding uses before
execution. CHECK retains an Unknown result; sampled native values are not
promoted to fabricated Known facts.

`evidence/local-raw.zip` preserves both pre-execution freezes, the first
candidate's enum-spelling compile RED, the apparatus metadata-selection error,
raw successful tests, build settings, C17, command manifests and guest attempt
records. The prior P1354 source/oracle/FIRST and C636 evidence packet are kept
as immutable copies. The tested source/oracle bytes are unchanged. The final
evidence commit only adds this report/archive and corrects a stale comment.

Example focused checks (using an appropriately configured build):

```sh
cmake --build build --target newlangc p1355_wrapper p1332_cli_oom
ctest --test-dir build -R '^p1355_' --output-on-failure
```

## Value and remaining review

This extends ordinary function decomposition without a second proof mechanism
or a new finite-depth proof type. Maintaining another constructor filter and
the shared three-file validation dependency is still a cost; wider call shapes
remain incomplete. Inline/direct or zero-argument forms remain alternatives.
Finite capability tests and CI do not prove general soundness, safe library
encapsulation or the library-author plus ordinary-caller UB audit/re-audit
locality benefit. Product GO remains PAUSED; H1 remains HOLD. Independent review
and Coordination adoption are separate from these local results.
