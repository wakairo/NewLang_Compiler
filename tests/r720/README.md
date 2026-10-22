# R720 frozen BLIND sources and tests

Issue: https://github.com/wakairo/NewLang_Compiler/issues/720
Immutable first source/oracle comment: https://github.com/wakairo/NewLang_Compiler/issues/720#issuecomment-6332163132
Candidate: daadd19d89b0c0274fb9490787a050867a07cbfb
Base canonical: Draft17.33, fixed original main 345e58be1bf805f2bc12cc802608f344351e7cd8

Five byte-exact original source files are retained: four positive cases in two original field layouts, and one wrong-return-type negative. SHA-256 is asserted in native.py. The R public probe observes actual-source checked-owned return receipt, validates all three event streams, checks root/field/sibling/currentness and rejects 13 copied-view mutations plus event mutations. The native observer is passive: original emitted C runs untouched as well, and observer build only checks real C condition/physical original fields/old Copy/branch/write/return-vs-postpath. GCC/Clang O0/O2 strict C17 are run via natural CI matrix; sanitizer variant uses configured suite. No private issuer tamper or R OOM sweep is claimed. Any failure remains a finding unless explicitly recorded as mechanical and retested. Never edit source/oracle to match producer.

R-only branch based on exact fixed candidate, not mergeable standalone without the producer PR; DRAFT / DO NOT MERGE.
