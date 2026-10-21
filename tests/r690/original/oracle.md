# R690 original BLIND oracle (immutable, authored before examining PR #689 or P #688)

Authority: Compiler/main 92367144da5c672282b91fd291962e3b923492b3; Draft17.33. Target candidate 10561f0a252a39502bf1382083e2511250c0f911. Execution: Chat + GitHub connector + separate GitHub R branch + GitHub Actions; NO P/Codex local environment.

Source A: `river.nl` — ordinary zero-arg non-main `river()->u8` with typed tail `u8(29)`; unrelated legitimate `main()->unit` should NOT generate a C main or contaminate exported result. `river_consumer.c` independently includes target versioned header and calls external `uint8_t newlang_c17_u8_export_v0(void)`; expected actual value exactly 29, status 0 (otherwise 21).

Source B: `lantern.nl` — `lantern()->u8` with source Copy local `let ember = u8(61);` followed by explicit `return ember;`; independent `lantern_consumer.c` expected actual uint8 value exactly 61, status 0 (otherwise 31). Values are independent R selections, not copied from producer fixtures.

Predicted source CHECK for both A/B: success, no diagnostic. Export `newlangc --experimental-c17-u8-export river river.nl` and `... lantern lantern.nl`: status 0 and nonempty C17 bytes on stdout; one external exactly-versioned fixed symbol, uint8_t result from genuine checked user source; no generated `main`, with all other helper symbols internal if present. Header inclusion and independently compiled C17 producer/consumer objects must compile/link/run at GCC and Clang, `-std=c17 -Wall -Wextra -Wpedantic -Werror`, O0 and O2; exit 0 without host-side result injection. Plain CLI and `--check` expected to retain normal mode policy (plain may be normal C17 program with main, `--check` quiet success; do not substitute plain C output for export output).

R-authored negatives predicted BEFORE execution:
- valid A with unknown selector `absent_function`: export CLI 4, stdout byte count exactly 0.
- `non_u8.nl` selected `not_scalar` of `()->unit`: export CLI 4, stdout 0.
- `wrong_arity.nl` selected `parameterized` of `(u8)->u8`: export CLI 4, stdout 0.
- `source_invalid.nl` selected `invalid_scalar` with out-of-range `u8(256)`: source semantic failure CLI 3, stdout 0; normal CHECK fails.
- C consumer predeclaration `extern int newlang_c17_u8_export_v0(void);` before versioned header: strict C compilation must fail with conflicting types.
- two emitted producer TUs linked together against the same fixed export name: duplicate symbol link fails, not silent selection.
- request from separate TU for a private/static emitter helper (if identifiable in emitted C): link must fail. If no private helper exists, NOT TESTED / not applicable.
- no-header caller using a wrong manual C prototype is caller-side C UB responsibility, NOT a required compiler refusal.

Failure reporting: first unexpected real CHECK/export/ABI/value/refusal classified first-pass finding and STOP; source syntax/registration or R-owned harness preparation error preserved and may receive §7.6 syntax-only repair without changing semantic oracle and with separate new SHA. Do not read P source, oracle, implementation/PR diff or rationale before this freeze. No producer edits, main merge, or other Track launch.
