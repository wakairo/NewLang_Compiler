# Track P / C85 NIGHTLY — STOP at additive observer precondition

C85 was executed against candidate `f03c4db6b335817359137ad64f381ededfa43546`,
tree `36a40ef3d9734296cec8e10ad61a3921bd27cc9d`, on the original isolated
`p965-canonical-custody-children` worktree. Resume checkpoint:
`d702109a14053bc2339d3e215301fbaaa01817a2`; PR962 base:
`24dac612697fcfcfdfcfabc35418c5e6f0a7b0b3`; main:
`d579c761a78e3c84765e35061f77936789788d82`; CURRENT_SPEC -> Draft17.34.
LOCAL ONLY. Any subsequent evidence-only checkpoint is not a new tested candidate.

## Confined implementation

Only `src/h_origin_internal.c/.h` and `src/copy_body.c` changed production behavior.
The const observer locates an actual parent's original COMMITTED BODY edge, without
consulting current world/source/node/body-view tables. Four historical Copy GETs
use it; validators retain separate current `nl_checked_call_body` authentication.
The unit validator checks its current proof before dereference. Existing canonical
ownership/destruction, H/C17 admission, generic getters and proof issuers were not
changed in C85. The exact patch and production/source hashes are in `evidence/`.

## Ordered results

| Stage | GCC14.2 | Clang23.1.3 | ASan/LSan | UBSan |
|---|---|---|---|---|
| Original P633 same/different, P817, P785, P790 (5/config) | 5 PASS | 5 PASS | 5 PASS | 5 PASS |
| Unit and selected neighbors (20/config) | 20 PASS | NOT RUN | NOT RUN | NOT RUN |
| New frozen historical observer probe | FIRST RED, first source | NOT RUN | NOT RUN | NOT RUN |

P633 preserves its equal-node shallow replay and 35 controls per source; P817
reports 72 integrity mutations with restore PASS; P785/P790 report 30 each.
The original unit/neighbor selection includes P866/P878 prefix integrity and
reached OOM, P706/P713/P718/P722/P728 proof/OOM, P774/P778, P817 public/issuer OOM
using P813 sources, and both P945 integrity cases. Complete CTest test names and
commands are preserved verbatim in the corresponding logs. These are 40 passing
existing CTest executions, not full frontend or full ownership coverage.

## First RED and mandatory STOP

Command:

```text
python3 tests/p965/c85/observe.py /workspace/p965-build-gcc /usr/bin/gcc none
```

The standalone strict C17 probe compiled successfully, then failed on the first
input `tests/p813/sources/charge_false.nl`, mode `u8`, at `observer.c:165`:

```c
o.caller != foreign.caller &&
o.caller->context != foreign.caller->context &&
o.caller->source != foreign.caller->source &&
o.caller->root != 1 && o.caller->count > 1
```

This is a compound fixture precondition before baseline gets/validates, bogus
shallow-parent checks and the 12-mutation loop. The log does not isolate which
conjunct failed. The assumptions `root != 1` and `count > 1` were not established
by selecting a genuine Copy caller. No production defect, permission acceptance,
or sanitizer memory error is inferred from this failure. The preregistered probe
is RED and is not fixed, waived, instrumented, or rerun. The source/oracle/freeze
remain exactly as executed. Probe exit1 propagated as CalledProcessError;
`observer-gcc-FIRST.log` SHA256:
`0748471e6e035252fa17317889cc88b7b63967e73328e5abe041a8ec50d37a70`.
The runner's original TemporaryDirectory cleanup removed its executable on unwind;
no diagnostic rebuild was performed.

All later build/test stages stopped. There is no C85 remeasurement of the original
494 ownership trials or 502 CHECK/registration OOM failpoints, no original
P961/P953 ownership/output matrix, no full515 frontend/history/formatter gate,
and no healthy raw C17 GCC/Clang O0/O2 guest0/1/2 or guest sanitizer matrix.
The previous 494/502 evidence remains historical, not C85 PASS.

The completed GCC neighbor OOM logs separately record P866/P878 prefix index0/1
on four source inputs with did_fail=1, unchanged=1, public retry healthy, and
P817 actual completed-arm snapshot indices0..1/2 and0..3/4 with reached=1,
unchanged worlds, no published artifact, and healthy retry. The detailed original
neighbor OOM results are preserved; they do not substitute for P965's 502 points.

## Preservation, remote budget, value

The original P633 RED, C83 P817 RED, P965 initial freeze, all old tests/oracles/SHA
manifests, original P953/P961 failures and independent R963 BLIND freeze remain
unchanged. R963 source/oracle inputs were not inspected. New evidence is copied
byte for byte from `/workspace/p965-c85-artifacts`; SHA256SUMS covers all copied
logs, the exact private patch and stop.json. stop.json includes all changed source
and new frozen test/input hashes.

Push0 / new PR0 / Actions0 / FULL5=0 / PARTIAL=0. No existing PR edit, READY,
merge, or R/F launch. No CI run ID or merge tree exists for this candidate.
Overall local release Gate NO; remaining ownership/native safety UNKNOWN.
Keep PR962/954/946 OPEN DRAFT HOLD. No automatic continuation.

VALUE-ALIGNMENT-RISK: the original getter/validator regressions now pass the
selected finite core checks, but the supplemental historical-observer contract
remains unverified and its fixture assumptions need a separately authorized
review. Private history/current authority separation adds audit burden; canonical
ownership, OOM, full regressions and guest cleanup were not remeasured on C85.
Human UB audit burden versus C remains UNMEASURED, Product GO PAUSED, H1 HOLD.
Return to Coordination with this first RED and no safety-proof upgrade.
