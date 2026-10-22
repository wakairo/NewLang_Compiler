# R720 frozen BLIND sources and tests

Issue: https://github.com/wakairo/NewLang_Compiler/issues/720
Immutable first source/oracle comment: https://github.com/wakairo/NewLang_Compiler/issues/720#issuecomment-6332163132
Candidate: daadd19d89b0c0274fb9490787a050867a07cbfb
Base canonical: Draft17.33, fixed original main 345e58be1bf805f2bc12cc802608f344351e7cd8

Five byte-exact original source files are retained: four positive cases in two original field layouts, and one wrong-return-type negative. SHA-256 is asserted in native.py. The R public probe observes actual-source checked-owned return receipt, validates all three event streams, checks root/field/sibling/currentness and rejects 13 copied-view mutations plus event mutations. The native observer is passive: original emitted C runs untouched as well, and observer build only checks real C condition/physical original fields/old Copy/branch/write/return-vs-postpath. GCC/Clang O0/O2 strict C17 are run via natural CI matrix; sanitizer variant uses configured suite. No private issuer tamper or R OOM sweep is claimed. Any failure remains a finding unless explicitly recorded as mechanical and retested. Never edit source/oracle to match producer.

R-only branch based on exact fixed candidate, not mergeable standalone without the producer PR; DRAFT / DO NOT MERGE.

## Initial CI retained (no source/oracle changes)

Initial R HEAD `6001892c27ec70e41562d3c52b21b66566a6ad0b`; [FULL5 #39719219145](https://github.com/wakairo/NewLang_Compiler/actions/runs/39719219145) and [PARTIAL #39719219225](https://github.com/wakairo/NewLang_Compiler/actions/runs/39719219225). First Clang job failed pinned clang-format on R-owned probe.c; exact correction in CI artifact 12400576490. Initial GCC/PARTIAL test failed the R passive observer `one actual terminal return`: incorrect R regex assumed `return 0;`, real generated unit return is `return;`. An unrelated redundant A_FALSE-only driver assertion was found and removed before that scenario first executed. This is **harness-only correction**, not a change to independent semantic oracle. Original SHA-bearing five source files and issue freeze never revised. First failures and workflow run URLs retained.

## Supplementary negative and second R-owned observer correction

Second R head `76cfe66a58ef4acc9ce25d922aaedd9b4816d5a5` natural FULL5 [#39719615251](https://github.com/wakairo/NewLang_Compiler/actions/runs/39719615251), PARTIAL [#39719615246](https://github.com/wakairo/NewLang_Compiler/actions/runs/39719615246); retain independently even if it fails. Before results from all four normal/terminal executions, a further **R-only observer metadata typo** was found: the preinspection original freeze says A_TRUE is a terminal return with no continuation; test metadata had `terminal=0`. Correct to `terminal=1` without changing the original source, semantic oracle or emitter. This is not candidate semantic calibration.

A separately designated **supplementary post-initial-freeze** negative source `scope_escape.nl` (not advertised as initial BLIND corpus) tests illegal survival of a scoped write-loan binder beyond its loan expression. UTF8 LF final newline; SHA256 `b536b80549881a49dc566746082b8aabe170635c0d168de3ef19d9fb38a6c6c2`. Oracle fixed before first execution: semantic rejection, no C, existing named output unchanged, no owned proof. Exact source:

```text
struct EscapeGuard { hot:u8, cold:u8 }
fn main()->unit {
 let object=EscapeGuard{cold:u8(7),hot:u8(3)};
 let dangling=loan_write(object@hot){|out|out};
 dangling;unit
}
```

Producer source/oracle was never copied for this additional negative. Scope and type negatives must not be conflated.

## Second initial repair run — mechanical Python observer failure

At second fixed R head `76cfe66a58ef4acc9ce25d922aaedd9b4816d5a5` PARTIAL [#39719615246](https://github.com/wakairo/NewLang_Compiler/actions/runs/39719615246), `r720_blind_return.integration` halted with `TypeError: 'int' object is not subscriptable` in observer construction, before any R-observed native results. The R script parameter `post` held an expected boolean flag, while the expression mistakenly indexed it as an expected field-value array (the original `end` input). Fix reference to `end[index]` and `end[1-index]` only. No guest source, physical values, branch oracle, producer compiler/proof or warnings changed. Third run from `ee07bcf93bc88ad9a658ec80bdd67c830d5e665c` may retain this same R-owned failure.
