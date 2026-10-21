# R708 immutable first-attempt and mechanical repair history

First HEAD: `bc727808fe2e457803bd12240969ea77d171b2bc` (before fixes). Independently frozen source and numerical oracles remain byte-for-byte unchanged, Issue #708 comment 6316589503. No producer/source/numerical oracle changes.

- FULL5 first natural PR pair: [run 39604888558](https://github.com/wakairo/NewLang_Compiler/actions/runs/39604888558), attempt1, 5 jobs failure. Clang job `119238427428` failed **before build** on pinned clang-format of `tests/r708/public_probe.c`, original exact suggestions are retained in that job log/artifact 12347672311.
- GCC job `119238427504` and GCC Release job `119238427644`: original `r708_independent_m2.integration` failed at Python `witness` with `KeyError: 'old'` in R-owned observer at line 152. GCC and GCC Release respectively 383/384 other tests passed. Native observer generation/execution **not reached** in this attempt.
- UBSan job `119238427136` and ASan job `119238427140`: identical R-only `KeyError: 'old'` and other existing tests passed. Initial source CHECK and public proof/passive metadata probe for case A reached before this failure; case B and negative N **NOT TESTED** at that initial point.
- NO LLVM PARTIAL first natural pair: [run 39604888747](https://github.com/wakairo/NewLang_Compiler/actions/runs/39604888747), job `119238427521`, 382/383 tests passed; exactly the same `KeyError: 'old'`, not a compiler rejection. No rediagnostic oracle change.

The observer code erroneously dereferenced a non-existent `old` property on public `direct` metadata. The old u8 value is **only** from the original independent pre-execution oracle, `oracle["direct"]["old"]` (40), so both uses now reference that frozen oracle; metadata identity remains required. P value/source/report was not imported to determine expected values. Pin formatter exact changes are solely whitespace/layout/comments; sanitizer/warnings/semantic assertions unchanged.

This bounded §7.6 machinery correction receives a **new natural** FULL5/PARTIAL pair; the initial failures remain historical and cannot be overwritten by the corrected run.
