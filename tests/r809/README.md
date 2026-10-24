# R809 calibrated evidence manifest (NO MERGE)

Track: R — Independent Red Team. GitHub branch + Actions only, **not** the P-equivalent environment.

- Authority: Compiler/main `d04341f816853b241e8f2b0dc4a4cbcc14a14dc6`, Draft17.34, fixed P #799 backend `7863e4fc2196273f3a33f5a1b33d1958f9b73b51`.
- Gate0 source, six negative inputs, expected oracle and SHA-256 are immutable at `695e03943bc945afd4c97297a2ab07df7a964d7b`, documented with full bytes [Issue #809 Gate0](https://github.com/wakairo/NewLang_Compiler/issues/809#issuecomment-6376673246). This is **NON-BLIND CALIBRATED**, not a correction of R806's independent first pass.
- Original R806 FIRST-PASS BLIND **INCONCLUSIVE** and its frozen source/oracle, DRAFT PR #808 and first failure Actions 39994390301 / 39994390297 remain unmodified.
- New 4 R-only CTests: `r809_source.source.integration`, `r809_public.public.integration`, `r809_c17.c17.integration`, `r809_native.native.integration`. Expected one natural first FULL5 5×456 and PARTIAL 455; do not repeat after first substantive failure.
- Public tests require the real issuer's body **and** parameters, same-numeric foreign-world refusal, stale/incorrect fact and loan refusal, source-original identities and lexical producer-local owner proof. 0/1/2 guest allocation failures are NOT issuer-internal OOM proof.
- **PLAIN_UNMODIFIED** native compiles/runs the unchanged generated C17 under GCC and Clang O0/O2, observing process status. It deliberately does **not** claim physical malloc/free or T bytes without an observation hook.
- **PASSIVE_WRAPPER_OBSERVER_ONLY** separately compiles the same emitted C17 with a read-only observer and linker malloc/free wraps, explicitly using `-fno-builtin-malloc -fno-builtin-free` at O2. Observed original address/physical free order, lexical producer move, T field bytes are evidence only for that **distinct instrumented binary**. The observer does not write guest state. Unrecognized observer hooks stop rather than fake a pass.

## Pre-CI PR base correction

The initial DRAFT PR metadata accidentally targeted `main`, where P799's stacked branch could not form a merge commit (GitHub `mergeable_state=dirty`) and no Actions were created. Prior R806 DRAFT PR #808 correctly targets the **fixed P799 candidate branch**. R809 PR #811 was retargeted to `p790-h-arm-native` at the same SHA `7863e4fc...` without any P branch or test/oracle mutation; GitHub then reported `mergeable_state=clean`. This README is a single non-semantic synchronization commit recording that metadata correction before any R809 Actions. Keep exactly one initial natural FULL5/PARTIAL attempt1 pair after this synchronization; no retry-to-green.
