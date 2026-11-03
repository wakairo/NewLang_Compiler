# R1120 C219 V3 repair evidence

This is an evidence-only, unreferenced Git commit object. It was never made an execution ref and never tested as a new candidate. The tested R branch remains at baf7c90a1b84de08adfcbe3407361efdcb32013a, tree d2def57c8549a7791787a09affdb41f4fcc9d277; PR1128 remains DRAFT / NEVER MERGE.

## Result and scope

C219 V3 NON-BLIND calibrated. The repaired natural FULL5 run 41643129722 and PARTIAL run 41643129684 completed successfully, attempt 1. Each FULL job passed 653/653 CTests; PARTIAL passed 652/652. FAST run 41643118343 passed formatter/build/registration only.

C01-C06 reached CHECK, genuine retained public views, emitted C17, strict GCC/Clang O0/O2 original and observed guest executions. All 33 frozen call rows matched their return, Bool and selected-arm expectations in each observed optimization. There were 24 guest executions per job (12 unmodified original, 12 instrumented copies), 144 across six jobs. ASan/UBSan guest flags and finite diagnostic-free executions are retained in the raw records.

Unmodified original C was compiled and executed with exit 0 and empty output. Argument/result/Bool/arm values were measured in a separate passive C copy; this is not direct branch instrumentation of the unmodified original. Public observations concern genuine retained known-call views, not an observed definition-time unknown-value proof.

C07 explicit RETURN/alias and C08 IF-result/tail alias passed CHECK but the backend refused them with V1-BACKEND-UNSUPPORTED, exit 4; their forecast emitter exit 0 remains unchanged. The same backend coverage refusal occurred for the three W wrappers. These five cases have no C17/native PASS. Their output was empty and named sentinels were preserved.

Nine negative adapters rejected at their prospectively frozen first diagnostics, exit 3, with empty stdout and preserved named sentinels. They were never passed to native. Earlier refusal does not establish downstream rules.

## Immutable history and budget

The source/oracle freeze is issue1120 comment6604296170. Oracle SHA256: 77219031e4a02354079d300a60507df4a2a1245bdddc314ec845a6bf155be791.

Initial FIRST/RED is comment6604707684. The original failure was R observer requiring an if/else statement for generated C using a lawful conditional expression; guest native was NOT_REACHED. All six original raw ZIPs and job logs are preserved in evidence/r1120/c219/v3-first in the tested branch. The repair freeze is comment6604857005. All 20 source files and the oracle stayed byte-identical. Original V1/V2 and old refs remain unchanged.

Final C219 consumption: FAST2/6, natural FULL5+PARTIAL pairs2/2, all attempts1, manual/rerun0, preservation CI0. No additional ref/push/CI is authorized by unused FAST slots.

## Archive format

REPAIR_MANIFEST.json lists the exact run/job/attempt, checkout SHA, bytes, SHA256 and Git blob identity. Six R-owned original ZIPs are stored without recompression, alongside six complete decoded job logs and the final FAST log. These ZIPs are R-owned log-transported archives, not GitHub-native artifacts; GitHub artifact lists were empty. Every ZIP CRC32 and every RAW_FILE_MANIFEST entry SHA256/size was checked using pure JavaScript receipt processing. Native executable bytes are not in the ZIPs; their SHA256/size and compile/run commands are in the summaries.

The actual Actions merge checkout was 8a8d0513283c60f4ca65c94826bafbf8221e516b with exactly the tested R tree. The fixed P target is a1091b2672b0a4b1d37e2571ef97bf100882b60b; main is 65055be55934416c911453463b5f26fa461e8d6a. No Production/workflow/root CMake/old guard edits occurred.

64-bit target only. No 32-bit execution, foreign graph/owner mutation, OOM campaign or new LOOP/Continue validation. Audit locality remains UNMEASURED; GO624 PAUSED/H1 HOLD. No READY, merge or Product GO.
