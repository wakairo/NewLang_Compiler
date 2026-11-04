# R1151 independent original T observations

Issue: https://github.com/wakairo/NewLang_Compiler/issues/1151

Fixed P: d66fdf751aa5823b5775cbaf346a6c02aba38182
Fixed P tree: 6ebba6e90fc9e7a4db441663873a7780614647be
Authority main: c120dd1accfefd3e8d3d7ae7a2f7f17b54e3f89a
Canonical: CURRENT_SPEC -> Draft17.34

## Preserved source history

- V1: independent complete source/oracle freeze before P implementation access:
  https://github.com/wakairo/NewLang_Compiler/issues/1151#issuecomment-6626465949
- V2: post-freeze helper/CLI calibration, NOT_RUN, unsupported helper boundary:
  https://github.com/wakairo/NewLang_Compiler/issues/1151#issuecomment-6626554487
- V3: post-freeze direct unit entry and equivalent SUB guard calibration:
  https://github.com/wakairo/NewLang_Compiler/issues/1151#issuecomment-6626698575

V1 files retain their original frozen paths directly under tests/r1151.
V1/V2 positive bytes remain unchanged and are not executed by this test.
V3 positive sources and the six unchanged V1 negatives are the active cohort.
Numerical/path/original-identity oracle remains V1's, with its selected tail
value observed through a terminal Copy in V3 unit main. This is not a fresh
blind V3 experiment and does not erase the original FIRST prediction.

## Apparatus

One registered test: r1151_original_t_write.integration. The public probe
creates genuine parser/semantic/owned artifacts. Its node views and historical
IDs are observations, not minted proof or native pointers. The ordinary CLI
is the actual private C17 consumer. No production code, headers, workflows,
toolchain, locks or existing test assertions are changed.

The unmodified generated C is built/run separately from passive observer C.
Observer statements only read live values/addresses and print records. They do
not supply guest conditions, replacement values, result values or stores.
The true callee return is read from the compiler's existing call-result local;
no return expression is rewritten. The predicate decision itself is not an
extra directly instrumented Boolean; its actual return is recorded. Host and
applicable guest sanitizer settings are preserved in the evidence.

Original freeze guards use a private-index snapshot. Its non-R tree must equal
fixed P exactly after restoring the original CMake bytes. Original guards and
manifests run unchanged in that snapshot; measured newlangc is built from the
real R checkout, which contains no production changes.

All commands, diagnostics, generated guest/observer C, public owned data,
observations, environment/checkout/tree and SHA manifest are archived to the
natural Actions log even for successful CTest execution. No network tokens
or manual workflow dispatch/rerun are used. Evidence extraction is pure data
processing and requires no repeat semantic execution.

## Limits

Six positive and six negative sources; no general range/termination,
changing/nested/multiple-loop, general heap/H, or production repair claim.
No final pointer or field read occurs after the main-body original ends.
Material discrepancy or unclassified failure stops the affected experiment.
Safe unsupported-profile refusal is distinct from canonical invalidity.

DRAFT / NEVERMERGE. FAST at most 6 actual natural runs; normal CI first set
plus at most one mechanical repair set (12 jobs total). Stop extra push at
first reliable all-green with the finite observations. audit locality remains
UNMEASURED; GO624 PAUSED, H1 HOLD, P1036-PR1050 STOP.
