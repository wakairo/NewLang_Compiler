# C247 local completion / publication hold

Validated code HEAD `b06533fdb95eb1420dc4bea348b106a376583286`; tree `ccc123ffc63292390fa81e31adbf09d4868514dc`.
Unadopted parent is `faad092b5b0deb608a048ff60bee0056082f9a59` / tree `01388ae82b1749453546933cb4ca6b311a25df77`.

- Clean detached GCC13 strict frontend: **741/741**, 395.62 seconds.
- ASan/LSan: **19/19** (15 previous verified passes plus 4 recovered tests).
- UBSan: **19/19** (17 previous verified passes plus 2 recovered tests).
- Strict GCC13 / exact Clang23.1.3 O0/O2 raw/passive native: **48**;
  ASan/LSan and UBSan each add **24**. Exploratory Clang23.1.2 is separate.
- New source cohort: 6 positives / 12 negatives / 2 byte-identical controls.
- Genuine current/Copy challenges: **752** including foreign-B survival and
  unrestored canonical destruction. Full representative OOM: **9,579** reached
  ordinals under GCC and both sanitizers; rollback/output-null/no-clobber/cleanup/retry.
- Same-candidate measured context retention: additional30, peak32, complete
  graphs7, six seal slots, cleanup live0/untracked0; no added retained graph/world.
- Exact original source/oracle/FIRST/RED preserved, anchored v1–v42 unchanged;
  prospective v43–v45 and P1107 CMake successors preserve each predecessor.
- Pinned format and diff/history checks pass. Final code/self-review and finite
  unsupported forms are documented in [README](README.md).

Raw packet (same P environment):
`/workspace/p1188-c247-checkpoint/local-measured-v1.zip`
SHA256 `ffa0b09c1227676737c35e43bd6e35c07c876a0fb662d73396c2b9e50331c186`.
Completion receipt SHA256 `971034062eec7549d8832daff3e4996eb889e7107b6189300a5fb596d45e1da3`.
The packet contains raw observations, original failures, generated C/passive
observers, census/PC logs, final test logs and relevant real compiler/probe ELFs.
The final documentation-only commit does not change validated Production/tests.

PR1186 is still open DRAFT at faad and remains unadopted (latest API check).
New remote push/PR/Actions=0; original PR push=0; GitHub manual actions=0;
Ready/merge=0; R input/report unread, duplicate P task=0. P1036/PR1050 STOP remains.
Publication requires actual PR1186 adoption at that same head and main tree01388,
then the already authorized forward merge / one DRAFT / at most12 natural jobs.
This is a publication wait, not a request for additional implementation approval.

Unverified: independent P1188 review, remote CI, local LLVM smoke/full LLVM,
general loops/arbitrary expressions/third Copy link/multiple helpers or writers/
heap H, and Product audit locality. Successful finite capability does not establish
general soundness or Product PASS.

VALUE-ALIGNMENT-RISK: ordinary variable factoring improves reusable v0 source;
source/lexical/value/world provenance and actual carrier mapping add audit work.
Versioned evidence/adapters are a larger maintenance cost than the Production
increment. Audit locality remains UNMEASURED; GO624 PAUSED and H1 HOLD continue.
