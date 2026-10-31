# C161 local receipt and self-review

MERGE-CANDIDATE for this finite capability, subject to current-head CI and
Coordination review; DRAFT HOLD. No READY/merge/Product GO decision.

Parent main: `2cf49e44aebc20c62469f9ea113bbaf01f4e752f`, tree
`01a083b9e9020bfde425b51dce1d413653ead119`. Actual main push FULL41174463610
five jobs and PARTIAL41174463623 one job were checked completed/success,
exact main SHA, push/main/attempt1 before this branch or code was written.
Implementation/test candidate: `a00c65801f6977839f1d4c7211ce53095581e051`,
tree `b208226a4d29ad183de76496ba1aef745030df37`. Later changes are this
receipt, README and raw/summary data only; no retest result is invented for
changed compiler code. Final PR HEAD/tree and natural CI belong in Issue1071.

## First observations and repair history

Three new final-unit RETURN sources had CHECK0, genuine public Copy
GET/body/parameters/event current0, H origin/producer/terminal receipts, then
parent C17 backend4. The parent's closed-main planner rejected a terminating
root; its block serializer also did not mark main as an eligible final-unit
function body. These source-static guards explain the observed flattened CLI
diagnostic; no instrumented private trace is claimed. Named output stayed
unchanged. The source-only freeze predates both execution and Production edits.

New candidate first RED: targeted ASan CTest returned 122 PASS / 1 FAIL.
`p633_local_cli.integration` reached its unchanged P713 legacy helper, then
raised `FileNotFoundError` for `p713_public_probe`. The selective build plan
had omitted that indirect prerequisite. Original 123-test output, stderr and
LastTest log remain RED. Building only the missing target and running only
P633 on the same compiler/source/oracle produced PASS. This is a build-plan
repair, with no Production, old test, oracle or timeout edit. No real
sanitizer/ownership/current safety failure was observed.

## Local measurements

| Lane | Actual validation |
| --- | --- |
| GCC13.3 strict compiler | 595 distinct PASS: new3, existing590, CI-hook synthetic2; disjoint partitions |
| Clang23.1.3 strict compiler | 595/595 full CTest PASS, same named set as GCC |
| Clang ASan/LSan | 123 selected distinct PASS after the missing-prerequisite repair; original 122/1 retained |
| Clang UBSan | 123/123 selected PASS |
| New capability in all four lanes | Each has 3 genuine current/C17 passes, 18 physical O0/O2 guest0/1/2 traces, 1369 reached emitter OOM positions |
| Existing P1062 in all four lanes | All6 profiles, current/negative/no-clobber, 44 old C identities, 2750 reached emitter OOM positions per lane |
| Existing P965 | 494 original/foreign-child ownership trials and 502 ledger OOM positions; exact detailed counts extracted from Clang/ASan/UBSan logs |
| Formatting/history | Pinned clang-format23 PASS; unchanged historical lineage and prospective2530 input hash gate PASS |

New C17 hashes are exactly the parent tail-unit control hashes:

- terminal_without_probe: `f8c5647ee7b8d57127cb2a2832eaac90df49eb77e5812369bcb8f7717b23cd66`
- regression_reversed: `9ec3305e33723c41a8d276366555f79fd4153714b4a70627be935333aaf74720`
- producer_direct_result: `a3024c5674ffaa3e51d8fa8c0c5810ae3a3e78fa79b895e99af4019f66f65143`

Each new source exercises 14 actual Copy-current, 4 H and 7 world negative
cases, supplied foreign-view rejection, original A destruction without
restoring foreign B bodies/arms and B survival/exact use/destruction.
Reached emitter OOM positions are respectively455/459/455, with unpublished
output, unchanged semantic snapshot and original owner/current, exact retry.
Named-output quota failures preserve bytes/inode and leave no temporary files.
The new source negatives retain CHECK3 (wrong type), CHECK3 (existing
allocation-world precision limit), and CHECK0/C17=4 (nonliteral block result).

Summary: `evidence/local-results-v1.json`, SHA256
`fe976e109c97fe7226d987724d64e615fe66404f1b7e27626fcd9cb1abba4ca2`.
Raw ZIP: 2856 entries, 2236450 bytes, SHA256
`baf5fca89ceb27d93680409f90f16356dcd776d02698d1f686f2f1b924d965b9`.
CRC and every internal SHA were checked. It contains initial RED and repaired
epochs separately. GCC has all test summaries and new/P1062 raw commands;
its LastTest file was replaced by the later synthetic2 run, so a complete
GCC595 detailed LastTest capture is not claimed. Clang full and both selected
sanitizer detailed logs are retained. Full local sanitizer595 was not run;
the natural CI full matrix is a separate required receipt.

## Review and cost boundary

Self-review against Review Guidelines §§27–31: scope/authority, module
boundaries, ownership/lifetime, contract tests, integration evidence,
canonical/oracle fidelity, C safety/sanitizers and scope discipline PASS
within the stated envelope. CI/reproducibility remains pending publication.
The source delta is private2 files, +43/-13 (net30); classifier walks are
bounded and linear, with no heap allocation or ownership-state expansion.
The finite adapter is193 lines with3 CTest entries; old C witnesses are
reused. Existing v13 generated2530-key history map and complete source/raw
evidence dominate diff size; this is disclosed maintenance overhead, not a
new compiler abstraction or a substitute for implementation.

No general RETURN/CFG, third helper, broader used-read support, independent
F/R new-tree audit, human audit/re-audit locality comparison or performance
claim is established. R originals and PR1050 were not reused; original
protected source/oracle/first failures and v1–v12 are unchanged. Coordination
chooses proportionate review and, if PASS, timely user main merge. Capability
PASS does not establish general soundness or Product PASS/GO.
