# R1049 C139 — CALIBRATED source-only, NOT EXECUTED

This is a new R-owned, **post-FIRST NON-BLIND CALIBRATED** observation source for the
exact fixed direct PR1048 HEAD `ade11ebec808f5a95e8831a7ae6ee6f6da2b0dbe`, tree `70305183f8951038d296ac9a9ecae1f8e8750e4f`.
The original R1016 2 positives / 4 negatives and oracle seven are immutable.
It imports only the existing R1022 independent helper and physical observer/probe,
not P/T inputs, runners or oracle, and never reruns FIRST or old v1/v2.

- Fixed R-review parent: `e161666b611a9fe6b501cb132b7c7e916c5f5d2b`, tree `6eccec9eb3914dc9687dd2bd98f5e3558878f3a0`.
- Future exact opt-in ref **`refs/heads/r-calibrated/r1016-pr1048`** (ABSENT at C139 preflight).
- Only new files: this README, manifest, `r_calibrated.py`, and the new
  `.github/workflows/r-calibrated-r1016-pr1048.yml`; no old file changes.
- Old FIRST run `40849018312/attempt1`: 27685-byte
  `evidence/r1028/original-first-40849018312-attempt1.zip`,
  Git blob `7467df2023c84fe86409f815136df779de728bae`,
  SHA256 `0d074152c822d1b45a71ccee37a91fcfb1fea2eae2ce8fff8c27a35f3c3daac9`.
- Old v1 run `40869364041/attempt1`: artifact upload failed; RAW LOST.
- Old v2 run `40874742688/attempt1`: 33200-byte
  `evidence/r1035/calibrated-v2-40874742688-attempt1.zip`,
  Git blob `b85e5e794b03d1d623e71d3a5fa6d36cccf18060`,
  SHA256 `6baa3c9ef24b472d58c936abe9e414c81d8c11978f8f8ff48ec7f3bbbe6c0aa9`.
- All prior run/ref/source/receipt/journals are unchanged and not reclassified as PASS.

On a **later, separately authorized exact C-review one-shot push only**, the opt-in
workflow obtains the fixed physical candidate checkout, captures setup
stage stdout/stderr, exit and hashes in an exclusive per-run temp directory,
builds frontend-only with GCC13/curated LLVM 23.1.3 (NO P suite or synthetic
merge), and runs the original R-owned cases. Runner records original inventory,
Git blobs, both ZIP hashes, per-command stdout/stderr/exit hashes, all named
output snapshots and generated C17 bytes, native/guest evidence if reached.
Workflow uploads RAW with `if: always()`, unique slash-free
`r1016-pr1048` run/attempt/SHA name, missing-file error, and no overwrite.
An upload failure is **RAW EVIDENCE LOST** and must never be reported green.

A positive that still fails CHECK/C17/probe/native is **CALIBRATED RED** or
**NOT OBSERVABLE**; not an oracle correction. Public current/validator is
source-owned when reached but does not prove original H allocation/region/domain
or T old/new/sibling identity. The existing observer supplies limited callbacks,
not complete physical equivalence. Guest budgets 0/1/2 are **NOT host OOM**;
host OOM, ASan, LSan and UBSan are NOT IMPLEMENTED / NOT OBSERVABLE.
Negative earliest refusal and SHA/bytes/dev/ino no-clobber are raw-observed,
while a precise intended guard requires independent diagnostic interpretation.

**Static review only here**: no Python compile/syntax test, Actions, CI, compiler,
C17/native, fixtures/tests, formatter, PR, dedicated ref, GitHub Environment or
P-equivalent local execution. Executor source has no machine attestation of C
approval; C must independently inspect/pin the published HEAD/tree/blobs and
explicitly release a distinct one-shot action. No READY, merge or Product GO.
