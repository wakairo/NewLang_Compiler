# Track R #708 / BLIND first-pass independent candidate test

First immutable GitHub Issue freeze: https://github.com/wakairo/NewLang_Compiler/issues/708#issuecomment-6316589503

Execution model: ordinary Chat + GitHub connector, R-only branch **created from exactly** P candidate `1dbb75f4a326cd2e597689e4f7597d9627a5a4b0`, GitHub Actions. No P Work/Codex execution. **DRAFT / DO NOT MERGE**. Fixed base/main `45aa20cc1584a24c9d1a79ec99e034c7fb13c50e`, Draft17.33. This branch contains only tests/r708 and CMake registration.

Immutable original source UTF-8 SHA-256 (LF, terminal LF):

- `A_caller_original.nl`: `076142fd4555bae19933257517eab65fb818b2acb67694bf00b65bb994c6115f`; original index1 31→93, old31 feeds original independent index0 40→31.
- `B_two_invocations.nl`: `531bc4557b800c24940ccddca6d80a23eab16fbbed68ce1f1784db40eff786b3`; original index0 77→98 then different original index1 66→77, old returns 77 and 66.
- `N_read_amplification.nl`: `d998bd869873006e4f0b0c2a96ee1f44032b6f6d879a0df6f0f00b63f88b7a0b`; must reject actual read ref supplied to write formal.

Evidence layers:
1. `newlangc --check` on exact frozen bytes and negative semantic diagnostic.
2. Registered source→owned artifact via the public `node_checked_load` path (real `main()`, no host-created source authority), public historical parameter and event validation, before/after root/child facts, disjoint sibling, final loan scope, and copied-view mismatch rejection. Another independently loaded instance of the exact same source is used as a distinct issuer world.
3. Byte-for-byte strict emitted **unchanged C17** compiled/executed at O0/O2; additionally a passive native C observer reads the two original carriers through actual addresses before/after real emitted named helper calls. It checks caller actual ref equality with the real field address, scalar argument, old-return, old→new memory update, independent original and untouched siblings. Observer never writes guest state or source authority. Named output must match stdout.
4. No R test for private immutable issuer-record mutation, source loader OOM sweep or compiler-internal deliberate world corruption in this first pass. Public copied-view rejection is not private-record tamper proof. No heap, lifetime-domain mint, generic safety or product claims.

Use the one registered `r708_independent_m2.integration` test in existing FULL5 and NO LLVM PARTIAL CI. After initial results, keep logs and preserve original freeze if an authorized R-owned mechanical harness correction is necessary. Source/semantic/proof/native mismatch is a STOP, not a calibration invitation.
