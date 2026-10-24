# P814 Gate0 — preserved STOP evidence

Authority: Compiler/main `d04341f816853b241e8f2b0dc4a4cbcc14a14dc6`,
canonical Draft17.34. New branch `p814-completed-if-arm`.
Freeze commit `476a80de12f04373004580cbf49721b0f8106d32` predates all
new-source execution and any implementation. No production implementation was
changed. No R812/R809 private input was consulted.

The four new PumpLimits/ChargeState TRUE/FALSE sources all CHECK0. Their real
IF-arm calls report public Copy body/parameters 3/3; separately projected parent
and later calls report 0/0. ChargeState has two direct calls per actual arm, for
a future real two-stage OOM test. These results reproduce the authorized primary
boundary on adopted main, rather than importing old-main P793 measurements.
All four C17 attempts fail closed with CLI4/V1-BACKEND-UNSUPPORTED and zero output.
No native result is claimed. Scalar observations are semantic entry/handoff
values, not executed runtime values.

STOP: `read_only.nl` was frozen with a semantic type-mismatch oracle. Its actual
first rejection is earlier `error(unsupported)[LOCAL-LOAN-PROFILE]`, CLI3,
10:37–10:38. P chose an unsupported field read-loan spelling. This is a P-authored
negative calibration mistake, not an issuer soundness finding or proof that the
authorized minimal private IF snapshot application cannot work. The Issue's
unsupported-source-form STOP and no frozen-oracle tuning conditions are honored:
the source and its incorrect predicted oracle remain unchanged. No implementation,
new CTests, PR, Actions attempt, tamper/OOM or sanitizer work followed this STOP.
The batch also recorded the other frozen inputs before its results were inspected;
wrong field/return and escape reject at their predicted semantic layers.

`baseline.json` records actual CLI statuses, diagnostic classes/spans and output
lengths. Four `*-baseline.txt` files record original arm vs projected/later public
observations after parsed input teardown. The observer is diagnostic text, not a
compiler feature or a host-generated semantic world. `freeze.json` contains full
source SHA256 and pre-execution oracles, including the mistaken negative oracle;
`stop.json` separates that mistake from actual compiler results. Build logs are
losslessly gzip-compressed (mtime 0); `manifest.json` hashes captured evidence.

Reproduce from this branch using the repository's activated toolchain:

```sh
cmake -S . -B /tmp/p814-replay -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/p814-replay --target newlangc -j4
/tmp/p814-replay/newlangc --check tests/p813/sources/pump_false.nl
/tmp/p814-replay/newlangc --check tests/p813/sources/read_only.nl
cc -std=c17 -Wall -Wextra -Werror -I include -I tests/support \
  -x c evidence/p814/baseline-observer.c.txt -x none \
  -Wl,--start-group /tmp/p814-replay/lib*.a -Wl,--end-group \
  -o /tmp/p814-public-replay
/tmp/p814-public-replay tests/p813/sources/pump_false.nl
/tmp/p814-public-replay tests/p813/sources/charge_false.nl
```

Historical design audit: N/A—no implementation or normative change. KEEP
Draft17.34 §§13.5a/c, 17.4, 18, 20, 24, 27 and DI006/007/017, consistent with
M810 and C release. DEFER two-normal known-call join, backend and new proof schema.
Goal: reusable actual IF-arm proof completion. Evidence: baseline only, no new
authority. Alternatives: existing direct field paths or conservative rejection;
do not substitute parent projection for an arm receipt. Value alignment risk:
two-u8 compiler examples do not measure author/caller UB audit-locality improvement
over well-factored C. Product GO PAUSED and H1 HOLD remain. Coordination owns any
separate continuation authorization and fresh negative freeze.
