# C257 PR1199 natural CI audit

This packet records a C read-only audit of the already executed first six natural PR jobs. It is not independent F/R, an adoption decision, a general soundness proof or Product GO. **F-1198-01 is UNRESOLVED for this original candidate; adoption HOLD / no C PASS.** The original candidate remains CHANGES REQUIRED / DRAFT HOLD under [C repair authority](https://github.com/wakairo/NewLang_Compiler/issues/1193#issuecomment-6676808814). These CI successes do not clear that material static finding. This audit makes no runtime-reproducer claim.

## Fixed identity

P head: 24339b077ed0e0a3e847f23aed170c67d2b3d20a
P parent: 9ea08f360e53848f57009ec3e52139773aaceff6
P and test-merge tree: f18f59a6f10ee5d9564047a33a93f8915d7ba40f
PR base / observed main: 53c125b8cd66523651a21e803827eff6d2a06428
Actual checkout test-merge: 8fdf26ae564ea4b07049350a5eaf1f648c8678ed
Ordered merge parents: ['53c125b8cd66523651a21e803827eff6d2a06428', '24339b077ed0e0a3e847f23aed170c67d2b3d20a']

## Observed tests

| Job | Run | Job ID | Actual CTests PASS |
|---|---:|---:|---:|
| GCC Release | 42160307072 | 127704137981 | 776/776 |
| ASan | 42160307072 | 127704137920 | 776/776 |
| UBSan | 42160307072 | 127704137895 | 776/776 |
| GCC | 42160307072 | 127704137968 | 776/776 |
| Clang | 42160307072 | 127704137727 | 776/776 |
| PARTIAL | 42160307046 | 127704137670 | 775/775 |

The six-job aggregate is **5 x 776 + 775 = 4655 passing CTest executions**. Every old main test name is retained, and the exact 20 new names declared by the fixed CMake literal profile lists are present and PASS. Required12 and previous C25410 pass in every job. FULL/PARTIAL differ only by llvm.c_api. No failure, skip, timeout or sanitizer diagnostic was found. PARTIAL's actual post-CTest Production CLI step exited0 with empty stdout and stderr. Names, IDs, ordinal completeness, row totals and actual checkout were checked.

The original stage has two natural pull_request runs, all attempts1 and six actual jobs. The captured whole-branch inventory has 2 runs / 6 actual jobs; any later repair stage remains separate from these six logs. The individual publication ceiling stays12 jobs: original6 plus one remaining repaired6 explicitly authorized for F-1198-01. Unused allowance is not a target; first trustworthy final green stops extra pushes/CI. Original manual dispatch/rerun0, remote mutations0 and new experiments0. Earlier P/R/integration budgets are not reset.

## Data custody

The ZIP contains all six whole decoded job logs and capture receipts, the exact final audit/summary, fixed CMake/workflow texts and their Git-blob identity checks, API snapshots, audit scripts and the preserved C apparatus FIRST. Whole log total bytes: 1527434. Each job log was downloaded exactly once. BOM and newline bytes of the returned UTF-8 string were preserved. Original GitHub binary log-ZIP byte identity is not claimed.

The external FINAL receipt is outside the ZIP, avoiding a circular hash. The ZIP manifest fixes every payload byte count and SHA256. Local JSON transport segmentation copies are not duplicated in the ZIP; the exact reconstructed whole log bytes and capture length/hash receipts are included. No source/native evidence is stripped.

The eight baseline references are the existing [C256 main CI archive](https://github.com/wakairo/NewLang_Compiler/blob/7cd2fe94b994f4d7c01f47b88f1360d823939b7f/evidence/c256-pr1191-postmerge-ci/README.md): its external FINAL-RECEIPT.json, and audit-final.json plus six logs inside the restored ci-audit-packet.zip. The manifest gives exact original paths, ZIP-member locators, sizes and hashes. No baseline file was modified or downloaded again.

## Preserved apparatus FIRST and limits

The first attempt to construct the C audit script failed before writing a file because JavaScript interpolated a Python literal profile placeholder. It changed no CI, source or log. APPARATUS-FIRST.json retains that error; the corrected data-only script produced the final audit. This was not a compiler finding or a new semantic run.

Restore/verify only as data: verify ZIP SHA256 from the external receipt, ZIP CRC, exact manifest-member set and every payload bytes/SHA256. The fixed CMake/workflow files also match Git blob identities. Nothing in this packet requires executing an archived compiler, guest, AST tool, observer or semantic decoder.
