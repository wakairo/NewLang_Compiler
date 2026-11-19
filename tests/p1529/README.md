# P1529 C1202: finite prefix-read consumer repair

Local parent: unadopted `7deabd1b1f44bc40862b6a28c89fde502832d8cb`.
Adopted main: `c97ce81c48a0c003b072c77c655bdeb10860ebf1`,
CURRENT_SPEC selects Draft17.38. Publication requires a separate C release.

The existing C17 planner required the allocation-match statement immediately
after the startup named Copy. It rejected otherwise legal current/sibling
observations of the original T. The private planner now recognizes each such
binding using the actual startup Copy handoff's original root, field,
incarnation, current facts and scalar value. It still emits those reads through
the existing field/carrier consumer. It never skips arbitrary effects, creates
authority, or replaces the whole-source, Copy, arm or typed-owner checks.

| Own FIRST source | Prefix reads | First H byte | Second H byte | Copy return | Unchanged7de C17 |
| --- | --- | --- | --- | --- | --- |
| A-prefix-zero | current0, sibling27 | literal83 | 0 | tail | UNSUPPORTED |
| B-prefix-nonzero | current0, sibling27 | literal83 | 37 | tail | UNSUPPORTED |
| C-oldcopy-zero | none | old Copy83 | 0 | tail | OK already |
| D-combined-nonzero | current0, sibling27 | old Copy83 | 37 | explicit | UNSUPPORTED |

All four initially passed ordinary CHECK and genuine public MACHINE/owner/Copy
observations. All four now produce identical CLI/public API C17 bytes with
GCC, pinned Clang23.1.3 ASan and UBSan frontends. Old-Copy initializer support
was already present; this change does not claim it as new implementation.

The real unit receiver retains exactly four teardown steps. In H2 it physically
ends original H2; the caller ends original H1, then Copy-updates the same T.
H0 ends no H; H1 ends original H1 once. Zero is successful H2. Passive hooks
check original addresses and Domains, donor/formal handoff, closed payload
loan, callee-before-caller free order, old83/old0 and final T0/37 with sibling27.
The original malloc/free wrapper observes only live pointers before real free.

Local evidence: A/B GCC+Clang O0/O2 x H0/H1/H2 (24), C/D GCC+Clang O2 x
H0/H1/H2 (12), Clang ASan D (3), Clang UBSan A (3), old inline field1 and
linked-zero H2 (2), final formatted observer zero H2 (1): **45 actual guests**.
The final observer changes only spaces on four macro lines relative to the
tested r3; token/line/literal preservation is recorded, plus the final recheck.
Four source authority/loan negatives and four artifact negatives reject;
pristine retries succeed. Selected ASan CTest units: 9/9. The full CTest sweep
and registered native integration driver were not run; its same helper was
used for the selected local native matrix. Six existing host OOM sites,
22 fault/retry cases, preserve output/state, retry and named no-clobber.

FIRST and REDs remain immutable: own source/probe FIRST6978592897;
verification r2 FIRST6978681858; native runner FIRST6978736426;
observer correction r3 FIRST6978751983; spacing r4 FIRST6978818724.
Baseline A/B/D refusal, the runner's unexpected C success, initial observer
compile RED (two stray '+' tokens) and formatter RED are retained separately.
Final counters: local181/192, native45/48, host6/8 sites and22/24 cases,
frontend/probe builds4/4, SDK acquisition0, Compiler remote/PR/CI0.

Raw, exact source/oracle/apparatus revisions, logs, ledger, patch and hashes:
[Issue1529](https://github.com/wakairo/NewLang_Compiler/issues/1529), Evidence
ref `archive/c1202-prefix-read-repair-20261118`. P1525/P1528 and their
FIRST/RED/counters stay COMPLETE/STOP. No independent F/R original was read.

Scope debt: optional receiver read/five-step support remains HOLD; no general
CFG, new source/checker/issuer/API/ABI, forwarding or producer/LiveTail profile.
Finite capability evidence is not a general soundness or safe-library result.
UB-prevention audit/re-audit locality versus factored C remains UNMEASURED;
sound Rust safe facades remain fair alternatives. Product GO624 PAUSED,
H1 HOLD. Changed-subject independent review and publication/adoption remain C.
