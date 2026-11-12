# Two calls of the same input wrapper

Issue #1359 / C653 extends the existing current/Copy relation to two ordinary
calls of the same one-u8 wrapper. Each invocation keeps its own wrapper original
alive through one inner current-loop call, then returns that call's free old-u8
Copy. Main can retain the first Copy after the second invocation completes.

The CHECK consumer borrows the existing wrapper/inner records and canonical
call children. It checks source/plan/call custody, fresh formals, the actual
EndLocals transition, the scalar caller prefix, result identity and final seals.
The previous Copy exit is a live return world; it must not be mistaken for the
post-teardown entry of the next call. Preparing an inner argument can add READ
events after INITIALIZE. Their exact node/field/result and event chain are
checked before transfer; the existing full Copy replay remains authoritative.
There is no new stored certificate, issuer, world or public interface, and the
C17 consumer is unchanged.

`A-two-inputs.nl` uses inputs 43/181 and field index 1; `B-reversed-field.nl`
uses 91/157 and index 0. The passive observer checks both actual inner old
results, terminal current and sibling, the wrapper's displaced value and its
current/sibling, and the first result again after the second call. Only live
objects are inspected. Sequential stack-address reuse is allowed.

The six unit registrations cover both CHECK/owned/current/Copy positives,
identity/signature/field/world rejection and restoration, selected CHECK OOM,
all reached Copy-validator OOM, ended-loan/type rejection and safe rejection of
a third wrapper call. Three integrations cover strict raw/passive native,
named-output allocation/write/flush failures and 14 legacy generated-C hashes.
Historical sources, oracles and FIRST/RED records are unchanged.

The fixed functional candidate is `58469f670567a183fd9a4bf49299731e001419e9`.
GCC Release, ASan+LSan and UBSan each passed the six new unit tests, 76 related
unit registrations and 11 failure/compatibility integrations. The local native
plan used 20 guests: GCC/Clang O0/O2 raw/passive for both sources, then passive
ASan+LSan/UBSan. The 3976-allocation CHECK census is not an exhaustive CHECK OOM
sweep: 28 distinct selected points per source were injected. Copy validation
was exhaustive over the reached 43+3 and 61+3 allocations of the two invocations.

`evidence/LOCAL-FINAL.json` binds results to tested inputs and `local-raw.zip`.
The archive preserves candidate1's rejected CHECK FIRST, frozen source/oracle,
patches/bundles, raw diagnostics (including a discarded GDB apparatus query),
candidate2 results, commands, generated C, configurations and original main CI
receipts. It also includes the original P1358 packet and C649 static answer.
Current GCC CLI/owned ELF bytes are archived. Earlier candidate1 ELF bytes were
not separately archived before incremental rebuild; their recorded hashes,
complete Git source, build configuration and original failure logs are retained.
Other binaries remain rebuildable from the recorded inputs; sanitizer build
outputs are also retained in the persistent local workspace.

Scope excludes three calls, multiple formals, extra wrapper depth or multiple
inner calls, caller aggregates/H and cross-call reference loans. The new tests
do not grant the private CLI driver an arbitrary mutable-host-world contract;
the separately reported C649 hardening boundary remains unchanged. This finite
capability does not demonstrate general soundness, library encapsulation or
the author plus ordinary-safe caller audit-locality Product PASS. Product GO
remains PAUSED and H1 HOLD; independent review and Coordination adoption are
separate decisions.
