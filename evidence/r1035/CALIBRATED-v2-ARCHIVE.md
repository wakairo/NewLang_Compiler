# R1035 CALIBRATED v2 — raw ZIP archive (C137 / #1046)

**ARCHIVE-ONLY, NOT A VALIDATION PASS.** This is the literal existing GitHub Actions artifact, not a recompressed ZIP or regenerated JSONL. No new R run.

## Provenance and immutable hashes
- Original Actions run **40874742688**, attempt **1**, job **123450547887**, branch `r-calibrated/r1016-pr1031-v2`.
- Original artifact **#12897102916**, name `r1032-CALIBRATED-40874742688-attempt1-r1016-pr1031-v2-1edc429ff22513db1b47bd1cbd3fcda31f573d53`; expiry **2026-11-12 12:37:45 UTC**.
- R executor HEAD **1edc429ff22513db1b47bd1cbd3fcda31f573d53**, tree **89ea046b9f23da26d2393530ab40113c6b81755e**, parent **68ee20aa286f1510fe62696a7fba1a9f9dff06f8**.
- Compiler target PR1031 HEAD **a56cd08c23548852504cf95e94eb6c9a56f3dd8f**, tree **a2cd1c9a9c66a8686d7e4b99f545b82865ef3877**.
- Archive `calibrated-v2-40874742688-attempt1.zip`: **33,200 bytes**, SHA256 **6baa3c9ef24b472d58c936abe9e414c81d8c11978f8f8ff48ec7f3bbbe6c0aa9**, Git blob **b85e5e794b03d1d623e71d3a5fa6d36cccf18060**.
- ZIP **43 entries, CRC 43/43**. `CALIBRATED-RAW.jsonl` **20,986 bytes**, SHA256 **bd6df18faf82e18e90f683c181277898c1de4855ce8a443632683541e89c7df8**, **73 parseable JSONL records**. 15 stages and 30 stdout/stderr streams: SHA256/length **30/30 exact matches**.

## Actual outcome / limits
- POS-A and POS-B: CHECK exit **0**, public owned-current exit **0** (partial API observation), C17 backend emit exit **4** `V1-BACKEND-UNSUPPORTED` each.
- NEG-1 through NEG-4: CHECK exit **3**, negative emit exit **3**, named-output **no-clobber 4/4**. NEG-1 intended deep missing-Some-link guard **INCONCLUSIVE** (earlier provenance refusal).
- Original H/T physical/native, success 0/1/2, OOM: **NOT OBSERVED**. Raw terminal verdict: `RAW_ONLY_NOT_A_PASS`. This archive grants no READY/merge/Product GO.
- Original frozen six sources and oracle **7/7 Git blobs unchanged** against initial publication **0d933749005f0a2d9ed5777627e91732bb568547**. Original FIRST run **40849018312/attempt1** is distinct, with unchanged existing `evidence/r1028/original-first-40849018312-attempt1.zip` Git blob **7467df2023c84fe86409f815136df779de728bae**, SHA256 **0d074152c822d1b45a71ccee37a91fcfb1fea2eae2ce8fff8c27a35f3c3daac9**; original FIRST RED remains. Calibrated v1 run **40869364041/attempt1** remains a distinct **RAW LOST** historical outcome.

## Custody / verification
Retrieved actual GitHub uploaded ZIP, verified raw bytes/CRC/JSONL and streams, and transferred literal base64 to authenticated Git `create_blob`; Git returned the exact locally computed Git object SHA1. Only the ZIP and this Markdown file are to be added in an archive-only child of safe `r-review/r1022-six-original-source-only`. No change to workflow, runner, source, oracle, FIRST/v1, Production, P/T or main. No new Actions/CI/compiler/native execution or PR.

To validate the stored evidence, retrieve the Git blob as **binary** and recheck ZIP SHA256, byte count, CRC, raw JSONL and stream checksums; never treat a rendered URL/text summary as proof. See [Issue #1046](https://github.com/wakairo/NewLang_Compiler/issues/1046) and [C137 queue](https://github.com/wakairo/NewLang_Compiler/issues/1044).
