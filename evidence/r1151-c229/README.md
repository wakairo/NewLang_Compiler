# R1151 C229 custody archive

This directory preserves completed R measurements; it does not schedule or authorize execution. The measurement branch is stopped at `8a124b4aaa2ba9f3f22ab4fa5e9b586c6d12073c`. This custody commit has that exact parent and adds only this directory. PR #1153 remains Draft / NEVERMERGE.

## Contents and byte contract

- `logs/`: 15 complete decoded GitHub job logs, saved as UTF-8 (including any leading BOM) and losslessly gzip-compressed. The hashes for both compressed and decoded bytes are in `manifest.json`.
- `archives/`: 12 original R tar.gz byte streams decoded from the matching job log chunks. These archives are not regenerated. Each contains its original manifest plus 205 payload files; every payload hash was verified.
- `ledger.json`: the complete 15-job ledger, original FIRST failures and NOT_RUN layers, immutable measurement history, source manifest, limitations and receipt identifiers.
- `manifest.json`: hashes and sizes for the other 29 files. It explicitly excludes its own digest to avoid a recursive claim. The external Issue custody receipt records its hash, completed commit/tree and GitHub byte readback.

The archive's internal identity alone is not unique: GCC and GCC Release share that field. Paths use the actual run and job ID to preserve each separately. Whole job logs necessarily contain earlier repository suites; those bytes are retained as data, not additional producer evidence for this R oracle.

V1, V2, V3, all original numerical oracles, `tests/r1151/source-manifest.json`, and apparatus sources remain in the inherited parent tree. They are not duplicated here. V1/V2 positive versions remain NOT_RUN. No source, test, workflow or old evidence file is changed by custody publication.

## Custody verification method

The complete tree and commit are created before the first archive branch ref. All 30 files are fetched from that immutable commit with base64 encoding and compared byte-for-byte by size and SHA-256 with retained originals. The recursive tree is compared with the parent: every one of 5,954 existing leaves and all three workflow blobs must be identical. Only this prefix may be added. The external receipt checks the archive branch Actions count is zero, the measurement ref remains fixed, and PR #1153 remains Draft. These are text/hash/archive checks, not local compiler/native/build/test/formatter execution.

Authorization: [C custody release](https://github.com/wakairo/NewLang_Compiler/issues/1151#issuecomment-6627552258). Technical result: [R FINAL receipt](https://github.com/wakairo/NewLang_Compiler/issues/1151#issuecomment-6627899703). Final publication/readback is recorded separately in the same Issue.

## Technical result and evidence limits

### R1151 FINAL finite result: PASS after one historical-connection repair; measurement stopped

**Final HEAD 8a124b4aaa2ba9f3f22ab4fa5e9b586c6d12073c**, tree **d6bf6e116dfa4a0c2381d09220068955c29ee7b8**. [FULL41808183650](https://github.com/wakairo/NewLang_Compiler/actions/runs/41808183650) and [PARTIAL41808184014](https://github.com/wakairo/NewLang_Compiler/actions/runs/41808184014) are completed/success, attempt1, with all six actual jobs successful. The final ASan job completed at2026-11-04 09:47:16 UTC (18:47:16 JST); its raw and all205 payload hashes were checked. All six FINAL R archives are intact and PASS. **Further measurement-branch push/CI is stopped at this first reliable all-green.**

The PR checkout is **152540e962e60a691f9fa0854f04c5e8264f5ce1**, whose tree equals the R HEAD tree above. Fixed P remains d66fdf751aa5823b5775cbaf346a6c02aba38182; authority main c120dd1accfefd3e8d3d7ae7a2f7f17b54e3f89a / Draft17.34. [PR1153](https://github.com/wakairo/NewLang_Compiler/pull/1153) remains **DRAFT / NEVERMERGE**.

### Actual finite observations

All six active V3 positives accepted CHECK, produced genuine source-issued owned artifacts through the public probe, and were accepted by the ordinary CLI's private C17 consumer. Unmodified generated C and a separate passive observer version each compiled/executed at O0 and O2. The matrix covers GCC and Clang; the dedicated Clang ASan/UBSan jobs instrument the guest as well as the host build.

In each observed execution, the original's two live field addresses remained the same through the recorded loop phases, predicate call and scoped write/read interval. The actual writer and target reads used the original target address, while the sibling read used the other original field address. The original field values stayed unchanged until the one observed store; old Copy, stored/updated target, sibling and terminal Copy matched the frozen numerical oracle.

| Case | Recorded phase sequence | Result = Copy = callee actual | Target old -> actual return/store/updated | Unchanged sibling | Terminal Copy |
|---|---|---:|---|---:|---|
| p01_add_zero_first | 6 | 6 | upper: 29 -> 83 | 197 | old: 29 |
| p02_add_one_second | 4 -> 7 | 7 | lower: 193 -> 142 | 37 | updated: 142 |
| p03_add_many_first | 1 -> 4 -> 7 -> 10 | 10 | upper: 53 -> 142 | 181 | updated: 142 |
| p04_sub_zero_second | 9 | 9 | lower: 211 -> 142 | 61 | sibling: 61 |
| p05_sub_one_first | 8 -> 6 | 6 | upper: 109 -> 83 | 227 | updated: 83 |
| p06_sub_many_second | 17 -> 13 -> 9 -> 5 | 5 | lower: 151 -> 142 | 43 | old: 151 |

These values were already observed in FIRST and are reproduced by FINAL on the same frozen cohort. Per-case generated guest and observer C hashes are identical across all12 normal jobs. A data-only comparison of the six observed C files against their generated guest counterparts confirms that removing the added observer statements restores the guest bytes exactly.

### Negative FIRST diagnostics, not presumed downstream reach

Every negative returned semantic exit3 with the following actual diagnostics in all12 normal jobs. Rejected negative sources were never emitted or native-run; public probe/private C17 stages are NOT_RUN for them.

| Negative | Actual diagnostic | Exit | C17/native |
|---|---|---:|---|
| n01_nonbool_guard | P13-IF-CONDITION | 3 | NOT_RUN |
| n02_wrong_continue_type | P14-CONTINUE-TYPE | 3 | NOT_RUN |
| n03_foreign_field | FIELD-UNKNOWN-FIELD | 3 | NOT_RUN |
| n04_read_to_write | P3-TYPE-MISMATCH | 3 | NOT_RUN |
| n05_ref_escape | P8-EXIT-DEPENDENCY | 3 | NOT_RUN |
| n06_ended_root | P3-STALE-POINTER | 3 | NOT_RUN |

The stale-pointer diagnostic is source CHECK evidence for that particular ending-root case. It does not establish downstream private-validator coverage. Likewise the read-to-write case reports its actual static type mismatch; no later mode validator is claimed.

### FIRST / repair / FINAL ledger

| Matrix job | FIRST run/job | FINAL run/job | CTests per job | R native FIRST / FINAL |
|---|---|---|---:|---:|
| GCC | [126491941303](https://github.com/wakairo/NewLang_Compiler/actions/runs/41805670734/job/126491941303): RED, 9 failures | [126500384607](https://github.com/wakairo/NewLang_Compiler/actions/runs/41808183650/job/126500384607): PASS, 0 failures | 682 | 24 / 24 |
| GCC Release | [126491941774](https://github.com/wakairo/NewLang_Compiler/actions/runs/41805670734/job/126491941774): RED, 9 failures | [126500384583](https://github.com/wakairo/NewLang_Compiler/actions/runs/41808183650/job/126500384583): PASS, 0 failures | 682 | 24 / 24 |
| Clang | [126491941557](https://github.com/wakairo/NewLang_Compiler/actions/runs/41805670734/job/126491941557): RED, 9 failures | [126500384331](https://github.com/wakairo/NewLang_Compiler/actions/runs/41808183650/job/126500384331): PASS, 0 failures | 682 | 24 / 24 |
| ASan | [126491941034](https://github.com/wakairo/NewLang_Compiler/actions/runs/41805670734/job/126491941034): RED, 9 failures | [126500384679](https://github.com/wakairo/NewLang_Compiler/actions/runs/41808183650/job/126500384679): PASS, 0 failures | 682 | 24 / 24 |
| UBSan | [126491941378](https://github.com/wakairo/NewLang_Compiler/actions/runs/41805670734/job/126491941378): RED, 9 failures | [126500384805](https://github.com/wakairo/NewLang_Compiler/actions/runs/41808183650/job/126500384805): PASS, 0 failures | 682 | 24 / 24 |
| PARTIAL | [126491941798](https://github.com/wakairo/NewLang_Compiler/actions/runs/41805671104/job/126491941798): RED, 9 failures | [126500385124](https://github.com/wakairo/NewLang_Compiler/actions/runs/41808184014/job/126500385124): PASS, 0 failures | 681 | 24 / 24 |

Each normal job's R cohort contains six positives with four native executions each (unmodified guest/observer × O0/O2), and six rejected negatives. **FIRST144 native = guest72 + observer72; FINAL144 = guest72 + observer72; total288.** Negative checks total72; their native count is0. No normal job or R CTest was skipped. FIRST PARTIAL's later standalone Production CLI step remained NOT_RUN because CTest failed; FINAL PARTIAL actually ran it and reported exit0 with empty stdout/stderr.

FIRST all six overall jobs remain RED on exactly the same nine CMake current-freeze connections. The single forward repair changed only CMake routing and R-owned freeze_bridge.py. Original scripts, expected hashes, assertions, archive bytes and compiler sources were unchanged. Eleven reached historical registrations now run through the exact-fixed-P snapshot; actual R-built CLI/CC/sanitizer/profile remain unchanged, with only p990_new_native's declared source-directory path mapped into the snapshot. All fixed-P tree checks remain enforced.

FAST ledger: [41804041756](https://github.com/wakairo/NewLang_Compiler/actions/runs/41804041756) FIRST formatting RED /71cc570c; [41805323896](https://github.com/wakairo/NewLang_Compiler/actions/runs/41805323896) PASS /b40261c; [41808173874](https://github.com/wakairo/NewLang_Compiler/actions/runs/41808173874) PASS /8a124b4. **FAST3/6, normal12/12 actual jobs, manual rerun/dispatch0.** All attempts1. FAST is only formatter/build/registration evidence. First FAST build/registration and all semantic/native layers remain NOT_RUN; the later success does not rewrite FIRST.

### Source versions, independence and limits

**pre-execution frozen independent cohort / post-freeze interface-calibrated V3 / incidental legacy registration exposure after freeze / incidental old-P formatter exposure after freeze / incidental legacy native-report exposure after FIRST execution**.

V1/V2 positive versions remain NOT_RUN. V3's unit-main terminal Copy is not V1's u8 function return or evidence for V2's added helper ABI. SUB guard normalization preserves the pre-frozen finite numerical paths but does not test the original greater-than spelling. The active negatives are the six unchanged V1 negative files. All26 source/oracle manifest rows remain unchanged.

- Phase and carried-value subsequences, a Break value and actual dataflow observations are recorded. There is no separate direct guard-Boolean counter or independent proof of complete event interleaving.
- Detailed addresses and value traces are evidence from the passive version. Unmodified guest execution establishes strict compilation, zero exit/empty output and applicable sanitizer observations; it does not expose its internal addresses or prove every O2 operation survives optimization.
- The owned dump is authentic public data from real source checking, not an independently complete proof of currentness, Unknown handling or custody soundness. Ordinary private C17 acceptance is a separate observed layer.
- This R cohort adds no independent OOM, rollback or no-clobber experiment, and establishes no general range/termination, changing/nested/multiple-loop, or general heap/H result. No pointer/field observation occurs after the original's lifetime.

### Value and custody

No material semantic/C17/native/sanitizer discrepancy was found in this finite question. **VALUE-ALIGNMENT-RISK is separate:** the nine historical-entry connections to one global CMake digest made an otherwise successful small R experiment incur a full RED matrix and a snapshot adapter. This is concrete maintenance/audit coupling, not a measured audit-locality improvement or a product adoption decision. audit locality **UNMEASURED**, GO624 **PAUSED**, H1 **HOLD**, P1036-PR1050 **STOP** remain unchanged.


The FIRST logs also have 30 gzip/base64 Issue receipts recorded in `ledger.json`. The formatter artifact ZIP binary was not acquired or independently hash-verified; its GitHub-reported digest and size are metadata only. The complete decoded FIRST FAST log is independently retained. The old lineage.c deletion suggestion was never applied.
