# R690 NON-BLIND CALIBRATED-ABI-FINISH — pre-CI freeze

Authorization: Issue #690, Coordination "R690 CALIBRATED-ABI-FINISH" (2026-10-21 JST).
This note was committed together with the strictly test-only corrections **before**
the one authorized new natural FULL5 + PARTIAL run pair. It does not supersede or
retroactively revise the independent initial BLIND INCONCLUSIVE finding.

## Immutable first-pass record (unchanged)
- Frozen compiler/main: `92367144da5c672282b91fd291962e3b923492b3`;
  Draft17.33. Candidate PR #689: `10561f0a252a39502bf1382083e2511250c0f911`.
- Original, independently authored R freeze commit (ancestor):
  `91bce404fc56c6a87224cadeaefc23b6ccb5c3e7`.
  First CI R HEAD: `bd793ac51a273a5accff76e9ce6cbe58f3efea28`.
- Original `tests/r690/original/oracle.md` still predicts CHECK0 for both
  `river.nl` and `lantern.nl`. This prediction was **falsified**, not fixed
  retroactively. Both original NewLang source files remain byte-identical.
- Original BLIND source SHA256: river
  `0d69d7da2e98836057aaeba2724ef2dcee53c33d26be67479fce08f1c6a22b7a`;
  lantern `85e04fa52a755281b1644ec78c737ee85ffdc8ddd62079ab9e8939e4af0f67a7`.
- Original consumer SHA256 at frozen original commit:
  river `beeb8356af410689e7f4f1ae033696a81c6d717a204db01f7d1b7a2920af707a`;
  lantern `06903001c2c51345b033cf088f6277ba96fa43aaaf66b42c091cbc13e7b3ec18`.
- First FULL5: run `39547814572`, attempt1. GCC `119049868901`,
  Release `119049869116`, ASan `119049868969`, UBSan
  `119049869098` each hit 1/379 CTest failure at the original lantern CHECK;
  Clang `119049869007` stopped before CTest on pinned clang-format.
  Original format-suggestion artifact `12322226512` is preserved.
- First NO-LLVM PARTIAL: run `39547814523`, job `119049868402`,
  attempt1, 1/378 failure at lantern CHECK. River 29 cross-TU was actually
  observed native exit0 under strict GCC O0/O2; wrong expectation returned21.

## New calibrated expectation, separate from initial BLIND
The **ordinary** `newlangc --check tests/r690/original/lantern.nl` synthesizes
`main()` and MUST return **CLI3** with `P3-UNKNOWN-CALLEE` and
**zero stdout C bytes**, because this original helper-only source has no main.
This is not a source semantic change and does not predict export failure.

The separately invoked
`newlangc --experimental-c17-u8-export lantern tests/r690/original/lantern.nl`
selects the zero-argument helper directly and MUST still return **CLI0** with
real C17 producer bytes. The **independent** `lantern_consumer.c` includes
the published header, calls the actual external u8 result, and requires
**61/exit0** when compiled as a distinct C17 TU and linked with the producer
under both GCC/Clang, O0/O2, strict C17 warnings and existing sanitizers.
Its wrong-expectation control still returns **31**. The river ordinary
CHECK0/export true result29/exit0 and wrong-expectation21 are unchanged.

All original source-denial (CLI3), target profile/selector-denial (CLI4),
header-type conflict, duplicate public symbol link, private static symbol
link, and ordinary default mode oracles remain unchanged. Another substantive
divergence requires STOP rather than further calibration.

## Exact mechanical scope
1. `tests/r690/independent_native.py`: special-case only the above
   original lantern **ordinary CHECK** preflight (required CLI3, zero stdout,
   diagnostic containing P3-UNKNOWN-CALLEE), retain river CHECK0. Print a
   distinct CALIBRATED label and a distinct NON-BLIND terminal success label.
   Do not weaken the export/native/link/denial checks.
2. `tests/r690/original/lantern_consumer.c`: exact pinned clang-format-23.1.3
   suggestions, brace placement of two functions only.
3. `tests/r690/original/river_consumer.c`: exact pinned clang-format-23.1.3
   suggestions, include ordering and one function brace placement only.
4. This documentation file. No compiler, P test, canonical, CMake, workflow,
   toolchain, NewLang source or frozen oracle mutation. Original consumer
   bytes are recoverable exactly from the original freeze commit. Calibrated
   file SHA256 and actual CI verdict are to be recorded from GitHub Actions
   in the distinct final calibrated Issue #690 report.

**Method**: normal Chat + GitHub connector + existing dedicated R GitHub
branch + GitHub Actions; NO P-equivalent environment. At most one new natural
FULL5/PARTIAL pair; no manual reruns, no merge/self-close/other Tracks.
