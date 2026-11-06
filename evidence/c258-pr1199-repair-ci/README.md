# C257 PR1199 repaired natural CI audit

**The repaired six natural jobs pass this C data audit. Adoption remains HOLD; this packet is no C adoption PASS.** Independent F re-review of fixed0fd24 was released in [comment6677472782](https://github.com/wakairo/NewLang_Compiler/issues/1198#issuecomment-6677472782). Its technical disposition is outside this CI audit. Original24339 and its F-1198-01 CHANGES REQUIRED remain preserved.

## Fixed identity

| Item | SHA |
|---|---|
| Repaired P HEAD | 0fd24ad3ed08c51845819aab48506c9d43fdccf6 |
| Sole parent / original candidate | 24339b077ed0e0a3e847f23aed170c67d2b3d20a |
| Candidate and test-merge tree | 121ca6c13b81768c9c1b854a5d72d76841f6df10 |
| PR base / canonical main | 53c125b8cd66523651a21e803827eff6d2a06428 |
| Actual PR test-merge checkout in every job | 84278f92b8949349b8d47dae8a677ddf60c49a89 |

Ordered test-merge parents are main then repaired P HEAD. Git API metadata authenticates tree/parents and every whole log records the actual checkout SHA. The final API snapshot records the same fixed PR HEAD, base and test-merge.

## Observed execution

| Job | Run | Job ID | Actual CTests PASS |
|---|---:|---:|---:|
| GCC Release | 42165762426 | 127721762812 | 780/780 |
| ASan | 42165762426 | 127721763267 | 780/780 |
| UBSan | 42165762426 | 127721763018 | 780/780 |
| GCC | 42165762426 | 127721763162 | 780/780 |
| Clang | 42165762426 | 127721763311 | 780/780 |
| PARTIAL | 42165762344 | 127721763037 | 779/779 |

The six-job aggregate is **5 x 780 + 779 = 4679 passing CTest executions**. The original candidate's exact776 FULL/775 PARTIAL names are all retained and PASS. The fixed CMake adds exactly these four names, each actually reached and PASS in every job:

- p1193_signature_census.unit
- p1193_signature_normal_auth.unit
- p1193_signature_oom.unit
- p1193_signature_return_auth.unit

All original new20, required12 and C25410 names remain PASS in every configuration. Actual name/ID/ordinal sets are unique and complete, and every row total agrees with the actual final CTest summary. FULL and PARTIAL differ only by llvm.c_api. PARTIAL's registered inventory matches its executed779 names; its actual post-CTest Production CLI step returns0 with empty stdout/stderr.

No failing, skipped or timed-out CTest or sanitizer diagnostic appears in the complete logs. ASan/UBSan configuration is actually present in the corresponding workflow command; the other FULL jobs configure none. Required execution and post-CTest steps succeed. The workflow's explicitly conditional non-Clang formatting/artifact and trusted-main cache-save steps may be skipped as designed. Public workflows are byte-identical to the original candidate and match the repaired candidate's Git tree. The independent sidecar hook is present in every FULL configuration command. This audit does not execute CMake or tests locally.

## All-history budget and FIRST

The whole named-branch history contains exactly4 natural pull_request runs and12 distinct actual jobs: original6 plus repaired6. All run/job attempts are1, all statuses are terminal success, manual dispatch0, rerun0, queued/cancelled extras0. The authorized branch ceiling is12 and remaining allowance is0. First trustworthy final green stops additional pushes/CI. [C repair release](https://github.com/wakairo/NewLang_Compiler/issues/1193#issuecomment-6676808814) remains the authority; no earlier P/F/R budget is reset.

Original evidence is referenced at [66e222e275e7bac9c478eaba8b50262803badb46](https://github.com/wakairo/NewLang_Compiler/blob/66e222e275e7bac9c478eaba8b50262803badb46/evidence/c257-pr1199-original-ci/README.md), with original packet575920 bytes / SHA256 fef63e29018229f1765a74a8527ca5a52608222dc87dd4b3b4017ee1f79d6da6. No original file was changed and no old log was fetched again. FIRST-PRESERVATION.json distinguishes these original records from this repair's first-attempt success.

## Data custody and limits

This packet contains all6 new whole job logs (1536214 bytes), their single-download length/hash receipts, final audit/summary, fixed CMake and unchanged workflows with Git-blob checks, complete API snapshots, the C repair and F re-review releases, and data-only audit/finalization scripts. Each new terminal log was fetched once. Connector-decoded UTF-8 BOM/newline bytes were preserved; original GitHub binary log-ZIP bytes are not claimed.

The manifest fixes every payload member's length and SHA256 plus8 original baseline references (receipt, audit and six old whole logs), using their immutable archive/ZIP locators. The external FINAL receipt is outside the ZIP to avoid circular hashing. Reconstructible local JSON transport segments are excluded; exact whole log bytes and capture receipts are included.

Verify only as data: ZIP SHA256, CRC, exact member set and every manifest payload length/hash. This work ran no compiler, build, native program, AST tool, observer or semantic decoder and made no GitHub mutation or new experiment. Counts describe executions across configurations, not unique source cases, a general safety theorem or a Product result. F/R and adoption decisions remain separate.
