# P817 — completed actual IF-arm Copy caller snapshots

Authority: adopted main `d04341f816853b241e8f2b0dc4a4cbcc14a14dc6`,
canonical Draft17.34. Exact new-branch base is P814 evidence-only
`82496eeeede9e82eb24ab1ff9186e7a0071744a6`. Historical P814 files under
`evidence/p814` and `tests/p813/sources` are byte-identical to that base.
No R809/R812/R803 private input/oracle was used.

## Gate0 and historical provenance

New negative freeze commit: `dad6c16` (before its first execution and any code
change). `freeze.json` records the full SHA, semantic/no-output/sentinel oracle,
and K=5 successor test names. `wrong_copy_actual.nl` uses the already admitted
`loan_write(meter@quota)` form and a real named `ref<write,u8>` helper; its second
actual is an independently source-issued `usize` Copy binding rather than the
required `u8`. Initial CHECK rejects at semantic P3-TYPE-MISMATCH, 8:64–8:73,
CLI3, zero output. Named output retains its original sentinel and no temporary.

An initial diagnostic harness invocation used unsupported `--output`, yielding
CLI2 before opening the source. That invocation mistake is retained separately
in `initial-cli-invocation.json`. Only the harness command was corrected to the
documented `--emit-c17-to`; neither source nor frozen expected diagnostic changed.
This is not a compiler semantic/test predicate failure or an Actions rerun.

The old P814 `read_only.nl` remains SOURCE-UNSUPPORTED / LOCAL-LOAN-PROFILE,
including its original mistaken semantic prediction; it is NOT counted as a
semantic wrong-mode PASS. The other old wrong-field, return-type and scope-escape
sources retain their respective semantic refusals.

All four old positive baseline observations exactly matched P814 before changes:
CHECK0; original IF-arm calls body/parameters3/3, parent projections/later0/0.
The baseline executable was built from unchanged fixed-main production files;
its digest and actual results are in `gate0.json`.

## Minimal implementation and trust boundary

Only production change: `source_if` invokes existing private
`nl_copy_body_seal_calls(branch.artifact)` after checking the actual body and
constructing its IF_ARM root, immediately before `save_arm` publishes it.
The arm's source-owned parent, IF node, index and cloned world were established
by the existing issuer. Each callee is its actual direct child, with exact caller
pointer; the helper stages all completed caller graph copies before attaching any.
On failure, its pending allocations are freed, the branch is destroyed and the
surrounding semantic transaction rolls back. RETURN flags/exits and sole-normal
parent replay are unchanged. A projected parent proof is never used for an arm.

Public API/schema, sealer/validator implementation, source syntax/semantics,
normal joins, C17/LLVM, H/Array code and workflows/toolchain are unchanged.
`semantic_internal.h` retains historical SHA256
`92766597c230e2e54ccd938f7eefb373dc67faaf330c6c0791846f01a3558a67`.
No new Allocation/Domain/ref authority is issued. The cost is the existing bounded
per-child full caller graph snapshot, O(caller nodes × direct calls), not a new
registry or public receipt mechanism.

## Validation

Five new tests (`p817_*`, not the old preregistered-but-unimplemented p813 names):

- Pump and Charge TRUE/FALSE: actual original IF-arm public body AND parameters0,
  exact parent/index/world/source/root/count snapshots, preserved root/child
  incarnation, changed child AND parent ValueFacts, original old Copy, sibling
  unchanged, genuine Change event; projected/later proofs separately0.
- Integrity: same-number separately issued worlds, parent/wrong sibling/arm/node,
  graph/source/root/count, terminal/normal flag, actual/formal/old Copy, wrong
  field/current ValueFact/scope and ended loan refusals; duplicate snapshot
  attachment refused without damaging the original. 72 restored graph/state
  mutations plus foreign-view and forged-result/parameter checks.
- Issuer OOM: every reached actual IF-arm snapshot allocation (Pump2, Charge4).
  Includes the second child after one allocation has been staged. No partial
  child receipt validates; no child snapshot attachment survives; actual arm and
  registered caller state remain unchanged, output artifact NULL; clean retry
  validates genuine original-arm proofs. Both terminal and normal arms reached.
- CLI: immutable source SHA, correct semantic layers, historical unsupported
  negative classified separately, all four positives CHECK0/C17 CLI4/zero output,
  old named output preserved and temporary cleanup.

Initial local test/build logs are losslessly gzip-compressed, not rewritten.
`local-results.json` records actual inventories and statuses. GCC13's additional
local ASan build did not reach executable tests: that toolchain's static libasan
link failed on `signgam`. Its original log is retained. CI uses Clang for ASan;
the separate pinned Clang23.1.3 ASan/LSan run is recorded distinctly, without
changing code, link flags or test oracles. No failure is backdated as a PASS.
One natural fixed-HEAD FULL5/PARTIAL Actions pair is reported on Issue817.

```sh
cmake -S . -B /tmp/p817-replay -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/p817-replay -j4
ctest --test-dir /tmp/p817-replay --output-on-failure
ctest --test-dir /tmp/p817-replay -R '^p817_' -V
```

## Historical intent, goal alignment and limits

Historical §4.2: N/A—faithful private issuance precision improvement under M810/C
release, KEEP Draft17.34 §§13.5a/c,17.4,18,20,24,27 and DI006/007/017. No normative
selection or Ledger update. Alternatives are existing direct scoped replace and
conservative refusal; defer two-normal known-call poststate join (M8b), backend
(M8c), generic CFG/effects and new public proof schemas. Stop on any new authority
need or first unexpected source/public/fact/OOM/format/CI mismatch.

Goal is reusable normal-unit T-only helper proof completion. Technical success
does not prove global soundness, native execution or reduced author+ordinary-safe
caller UB audit/re-audit dependency closure against well-factored C. This toy's
product value and Rust/locally-unchecked alternatives are unmeasured; Product GO
#624 PAUSED and H1 #268 HOLD remain. Fresh independent R and candidate adoption
are Coordination's decisions. No merge or Track launch by P.
