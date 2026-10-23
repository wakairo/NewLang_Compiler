# P790 — speculative H-success-arm T Copy backend consumer

Base: unadopted P786 `5cd860e844703b6b778299689399b9d7fab64b44`.
Canonical Draft17.34; original main remains `77d83dfc00850b14fd7c3bc95e7e3c12e34161fb`.
This is a backend candidate, not issuer adoption, general soundness or Product GO.

`freeze.json` and the two new sources were committed before compiler edits.
`evidence/baseline.json` preserves CHECK0, actual source-issued public body and
parameters validators0, C174/zero output. The original emitter discovers Copy
calls only in the top-level function fragment; the first T aggregate in the H
second-success arm therefore falls through to the H aggregate shape gate.

The consumer discovers calls only through the existing two allocation matches,
validates both existing public views, and requires all T operations to use the
same actual caller artifact and its entry/handoff original/field facts. Nested
carriers are allowed only after the one H producer returned and before terminal
receipt. The existing physical record, helper old-load/new-store, H lifecycle,
caller-local owner zeroing and cleanup emitters are reused. One or two lexical
T originals are counted; no semantic state/authority is created. Unsupported
shapes still fail closed without publishing partial C.

K=4 new CTest names: `p790_native.integration`, `p790_arm_integrity.unit`,
`p790_arm_oom.unit`, `p790_driver_oom.unit`. Expected FULL5 452 each; PARTIAL451.
The native test observes actual fields before their lexical lifetime ends and
retains only observation numbers for program-exit assertions. It delegates all
successful heap allocation/free to libc, injecting only allocation failures.
No host supplies guest initial values or mutates guest records/ownership.

Reproduce with frontend CMake build then `ctest -R '^p790_' -V`.
`native.py COMPILER P778_PROBE CC SANITIZER tests/p790` tests each frozen program
at O0/O2, strict C17, unchanged generated file with and without passive observer,
all 0/1/2 allocation-success/failure worlds, tail-before-head frees and named output.
Integrity/OOM tests reuse actual source-owned graph tests; no synthetic owned
world is substituted for source success. Driver OOM samples sixteen reached
allocation positions, not exhaustive compiler OOM. Issuer OOM reaches every new
arm caller-snapshot allocation in the one/two-call programs.

`evidence/previous-tests` preserves exact previous live regression files before
migrating only their obsolete C17-unsupported expectations. Original source,
oracles and first-failure records in p778/p785 remain byte-identical. R776's
historical FIRST-PASS COVERAGE FAIL and R789 INCONCLUSIVE are not upgraded.
Independent R for this changed backend remains required before adoption.

Historical Gate: KEEP DI-001/006/007/009–013/017/018 and Draft17.34. N/A new
source/semantic selection: this consumes the existing exact public proof only.
DEFER new issuer, general CFG/effects, MIR, generic records, owner rights, FFI.
Goal alignment: reusable selected compiler capability; library author plus
ordinary-safe caller UB-prevention audit/re-audit closure versus factored C is
unmeasured. Product GO PAUSED and H1 HOLD.
