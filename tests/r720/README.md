# R720 frozen BLIND sources and tests

Issue: https://github.com/wakairo/NewLang_Compiler/issues/720
Immutable first source/oracle comment: https://github.com/wakairo/NewLang_Compiler/issues/720#issuecomment-6332163132
Candidate: daadd19d89b0c0274fb9490787a050867a07cbfb
Base canonical: Draft17.33, fixed original main 345e58be1bf805f2bc12cc802608f344351e7cd8

Five byte-exact original source files are retained: four positive cases in two original field layouts, and one wrong-return-type negative. SHA-256 is asserted in native.py. The R public probe observes actual-source checked-owned return receipt, validates all three event streams, checks root/field/sibling/currentness and rejects 13 copied-view mutations plus event mutations. The native observer is passive: original emitted C runs untouched as well, and observer build only checks real C condition/physical original fields/old Copy/branch/write/return-vs-postpath. GCC/Clang O0/O2 strict C17 are run via natural CI matrix; sanitizer variant uses configured suite. No private issuer tamper or R OOM sweep is claimed. Any failure remains a finding unless explicitly recorded as mechanical and retested. Never edit source/oracle to match producer.

R-only branch based on exact fixed candidate, not mergeable standalone without the producer PR; DRAFT / DO NOT MERGE.

## Initial CI retained (no source/oracle changes)

Initial R HEAD `6001892c27ec70e41562d3c52b21b66566a6ad0b`; [FULL5 #39719219145](https://github.com/wakairo/NewLang_Compiler/actions/runs/39719219145) and [PARTIAL #39719219225](https://github.com/wakairo/NewLang_Compiler/actions/runs/39719219225). First Clang job failed pinned clang-format on R-owned probe.c; exact correction in CI artifact 12400576490. Initial GCC/PARTIAL test failed the R passive observer `one actual terminal return`: incorrect R regex assumed `return 0;`, real generated unit return is `return;`. An unrelated redundant A_FALSE-only driver assertion was found and removed before that scenario first executed. This is **harness-only correction**, not a change to independent semantic oracle. Original SHA-bearing five source files and issue freeze never revised. First failures and workflow run URLs retained.
