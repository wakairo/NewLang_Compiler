# Track P / C82 P965 — STOP, local regression gate red

Tested candidate: `cf0b6dc66e50ba2d59c80e43871a8fa47e78aa10`, tree
`78f1cbc25e0b330faa43809068dd0916adcc28ab`, based exactly on frozen PR962
`24dac612697fcfcfdfcfabc35418c5e6f0a7b0b3`.

The candidate and full new test source/oracle/SHA were committed and reported in
Issue965 before any compiler/test execution. `freeze.json` SHA256 is
`cf3231f87e3d6f1a5c13eb29e3ab017a8ee7560c52c90ceb73c916ee5e5c1a49`.
No source, harness or oracle was changed after that execution began. This report
and evidence were added after STOP; they do not change the tested implementation.

## First failure and stop

GCC14.2.0 frontend build succeeded. The full 515-test frontend command used
`--parallel 1 --stop-on-failure --output-on-failure`. Tests1–45 passed; test46,
`p633_local_same_value.unit`, failed at unchanged `tests/p633/local_gate.c:154`:
`CHECK(validates(replay.artifact))`. CTest exit8. The remaining469 tests were not
run. Full first logs are retained under `evidence/`; there was no rerun or repair.

The old test initially validates the genuine original and replay CHECK entries.
Its failing positive control then substitutes a shallow arm shadow and an equal
copied node buffer. The old oracle expects this source-preserving view to remain
accepted before testing corruptions. Static inspection identifies a conflict
with the new exact canonical-child predicate in `h_origin_internal.c:200` and
published-root child-getter preflight in `checked.c:143/175`. This is an existing
replay-view compatibility gate failure; it is not an observed healthy source
CHECK/native failure or measured memory-safety violation. It remains a RED gate.
P does not weaken canonical custody, rewrite the old test, or treat it as waived.
Coordination must adjudicate the replay-view contract before further work.

## Finite completed evidence

- Clang23.1.3 ASan+LSan: new P96526/26 gates passed (two initial direct Window
  gates plus the other24 CTests). All new ownership probes passed492 trials.
  Window's first trial targets the real nested T IF whole arm. Restored trials
  emit identical healthy C; unrestored destruction frees original A fragments
  and array bases once, frees no B pointers, and B remains usable/destroys once.
  Window has11 actual fragments and7 watched original view-array allocations.
- Genuine foreign arm/body/continuation, arm_count0/1/SIZE_MAX, body_count0/
  SIZE_MAX, array-base swaps and combinations reject before child selection.
  CHECK-only ordinary T/scalar/LOOP sources keep the existing H-node emitter
  UNSUPPORTED result; their actual CLI is a separate existing consumer.
- Actual CHECK OOM positions320; definition registration positions182. All502
  selected source/mode/index failpoints were reached and returned OOM without
  publication, preserved the registry and succeeded on the same-context retry.
  New reservation records, first-arm-committed/second-pending, both BODY realloc
  phases and continuation reservation/early-clone cleanup were reached. Final
  close/publication allocates nothing. Full indices are in `reached-oom.json`.
  This is finite selected coverage, not exhaustive OOM or source soundness.
- Actual driver whole inner T IF replacement: CLI4/C bytes0, unrestored A
  cleanup/B healthy, named bytes/device/inode preserved; healthy retry equals
  raw generated C. No malformed graph output was native-executed.
- Pinned clang-format23.1.3 full tracked C/H check passed; new and historical
  file SHA checks passed. Existing `p634_historical_hash.unit` and
  `p633_local_cli.integration` passed in the frontend run before STOP.

Original Echo/Window strict GCC/Clang O0/O2 guest native0/1/2 observer, UBSan,
P605 historical integration and remaining frontend cases were NOT RUN after the
STOP. Some earlier existing-family native integrations passed in the first45;
they are not substitutes for those missing gates. The local host is Debian13,
GCC14.2.0, Python3.12.14, CMake3.31.10. Clang/formatter23.1.3 came from the exact
SHA-verified curated SDK. No claim of local GCC13 or full LLVM validation.

## Custody implementation and protected evidence

Only the five authorized implementation files, additive CMake and P965 files
changed. The existing opaque root attachment owns dynamically allocated parent
records and PENDING/COMMITTED/DETACHED ARM/BODY/CONTINUATION records. Producers
attach before nested work; child remains local during reserve. Each realloc
success immediately saves the real allocation base, including BODY phase1 before
phase2. Commit publishes the view and ownership without a fallible operation.
Teardown detaches canonical children and frees original buffer bases independent
of challengeable entries/counts. H issuance/currentness remains separate;
zero-site custody supplies no H witness. Published child getters preflight
canonical counts/bases to cover the ordinary scalar/T CLI too.

No edits to public headers, `semantic_internal.h`, canonical Draft, workflows,
bootstrap, existing sources or tests/oracles. Original P953 first FAIL, P961 C78
harness RED/C79 finite PASS and F76 old STATIC Gate B remain unchanged. R963
independent source/oracle freeze was not inspected, borrowed or modified.

Remote push0/PR0/Actions0; FULL5/PARTIAL0. Original PR962/954/946 remain DRAFT HOLD.
No READY, merge, or R/F launch. The fixed failed candidate remains local on the
separate P branch, with its original logs and source patch available for review.

**VALUE-ALIGNMENT-RISK:** the single private ownership protocol may replace
mutable-view obligations, but parent inventory and global getter checks add
compiler-maintainer audit and validation costs. The newly observed replay-oracle
conflict must be resolved explicitly. Additional metadata sizes observed in the
trace are104 bytes/parent and56 bytes/edge, plus the22992-byte root holder
(including existing bounded H records); no comparative performance claim.
Library-author plus ordinary-safe-caller UB audit/re-audit improvement versus
well-factored C is UNMEASURED. KEEP Draft17.34 / REPLACE NONE; technical exploration
only. Product GO PAUSED, H1 HOLD. STOP / returned to Coordination.
