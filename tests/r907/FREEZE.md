# R907 independent freeze — immutable reference

**Authority:** Issue #907 comment 6395495422 (freeze predates P implementation inspection and every semantic run). NEW NON-BLIND calibrated, never BLIND. Exact LF/UTF-8 bytes including final newline. No P/R898/R899 fixture or oracle reused.

| File | Bytes | SHA256 |
| --- | ---: | --- |
| r907_quartz.nl | 685 | `311929cb5ce3a06e0f44dfd994eb57becadb93ee9e1bada2534ea74cc388d64a` |
| r907_tide.nl | 724 | `ef0d58f77434dcb2b6f2d7e69fc86b012e97caa5e08ef86cda508f5618465b0f` |
| r907_return_incoming.nl | 682 | `62429becb619a77076f52774e3f2e37c0f4bff9aa396b40f2c25521619166860` |
| r907_return_literal_old.nl | 727 | `92d5769ea778145bb5972ccc4c64775b536aaea7a4c71af229b11088bd53f522` |
| r907_no_call.nl | 380 | `def328ab32a36a8713e509425edd329daf2c43727150b5f2268d7f70322f64dc` |

`r907_return_incoming.nl` = byte-exact `r907_quartz.nl` with one `    return displaced;\n` → `    return packet;\n`.
`r907_return_literal_old.nl` = byte-exact `r907_tide.nl` with one `    return echo;\n` → `    return u8(153);\n`.

All expected PRE and postvalues, foreign original/sibling, declared index and first semantic diagnostic are recorded in the pre-implementation Issue freeze. Any result earlier than expected P6 remains a STOP; source or oracle may not silently be changed. R-fast alone does not CHECK NewLang source. Fixed P HEAD `6e8be7c313d19b41ca2d45ae608b4dabb01b22e7`, source CHECK expected CHECK3/P6; no M8b C17/native claims.

Harness distinguishes real checked CALL-kind mutation from weaker view-only mutations and requires positive-before-negative and recovery. This bounded sample does not test actual external foreign context with equal IDs, all OOM branches or comparable P897/P905 builds; those are NOT TESTED unless separate Actions evidence establishes reach.
