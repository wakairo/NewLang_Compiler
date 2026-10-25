# R912 PRE-BRANCH immutable V2 harness / calibrated freeze

**Class:** NEW NON-BLIND calibrated correction of R910 only. NEVER a retroactive R910 PASS or new first-pass BLIND result.

## Fixed baseline / read-only authority
- Compiler/P905 candidate parent: `6e8be7c313d19b41ca2d45ae608b4dabb01b22e7` (tree `509c5954aac3c2a65af8e86af8ba8ace6625281e`).
- Old CLI comparator P897: `b65c39fac1cf56e276c03a53fff802fb0c243957`.
- Main docs HEAD `eb66367cc865fa9520b909e6af25b6f6236bca6f`; CURRENT_SPEC -> Draft17.34; FormalProof `5437f92d099e228773d31e2323e8ee6228ca21bf`.
- Old R910 PR911 HEAD `107b172b39b558bbec1a2ebdbcc7dced292925a5`, first failure and FULL/PARTIAL INCONCLUSIVE/STOP remain immutable; no edits to old branch.
- Preexisting R907 independently authored LF sources *reused as inherited*, never relabeled as newly authored: Quartz SHA256 `311929cb5ce3a06e0f44dfd994eb57becadb93ee9e1bada2534ea74cc388d64a`; Tide `ef0d58f77434dcb2b6f2d7e69fc86b012e97caa5e08ef86cda508f5618465b0f`; wrong incoming `62429becb619a77076f52774e3f2e37c0f4bff9aa396b40f2c25521619166860`; numeric-old literal `92d5769ea778145bb5972ccc4c64775b536aaea7a4c71af229b11088bd53f522`.

## New full UTF-8 bytes pinned as Git object blob SHA-1 (before Git branch or runtime)
- `tests/r912/probe.c`: `0e18b874b2a31727c703bbe2574997331fa98c30`; descended from R910 R-owned formatted V2 `74da767a01de7882a19c34e8356ac89535795957`; no P fixtures imported.
- `tests/r912/cli_diff.py`: `0c2798a8438b9b27e915c6dfd299959fca74c7e4`; descended from R910 R-owned `b5beeb10768dd58c87be26da6c297e26b153db4b`, correcting `['--emit-c17', SOURCE]` to `[SOURCE]` and test labels only.
- Full parent CMake with additive R912 target and five tests: blob `4793080c79263cfb3abbc4598f7b7482bc381205`.
- Original R910 blob/fixture/oracle/CI and first R910 false validator scratch calloc remain historical, NOT fixed or regraded.

## PRE / capture oracle; BEFORE first semantic run
- `src/copy_body.c` calls `p=calloc` then `nl_sem_clone(c,&p->pre)` in `nl_definition_call_begin`; `src/semantic.c` `nl_sem_clone` first allocation is real `malloc(sizeof(*copy))`. Both external symbols occupy distinct translation units at fixed P905. R linker wraps beginning call, clone symbol and malloc, allowing gating `active==PRE && in_begin && in_pre_clone && begins==1`, `source==begin_context`, allocation request `sizeof(NLSemanticContext)`, hit EXACTLY 1, no other allocation injection. Actual failure must return NULL, clone `NL_CHECK_OUT_OF_MEMORY`, callback `NL_CHECK_OUT_OF_MEMORY`; no valid PRE published, no partial guest change or dangling borrow; first pass other NORMAL arm preserves original old/incoming/sibling values. Only disable injection and register same unchanged `NLSemanticContext` again: expect both genuine NORMAL arms validated with old/incoming/sibling, `pre_verified`, `return_old_verified`, `loan_closed`, replacements=1 and healthy distinct other root. If clone wrapper is bypassed, requests are not uniquely scoped, site does not reach or state corrupts: STOP, no acceptance.
- Existing separate capture `calloc` injection expected 1 genuine hit, OOM callback, one normal positive then same context retry 2 arms. This is NOT the PRE clone malloc hit.
- Quartz expected old181 / incoming57,96 / sibling43; Tide old153 / incoming217,102 / sibling41.
- Existing independent foreign second checked context must have distinct owner/source/world/callee but colliding numeric NodeId, actual `parent->bodies[0]` edge substitution rejected and original restored authentic positive; retry preserves original source/owner; no use of expired borrowed pointers. Borrowed test only asserts primitive-snapshot lifetime, NOT comprehensive private loan/retained-ownership proof.

## CLI oracle: twelve legal-mode source tests
- Four frozen LF source bytes above × modes `newlangc --check SOURCE`, `newlangc SOURCE` (stdout C17), `newlangc --emit-c17-to OUTPUT SOURCE` (preexisting named sentinel). Build P897 and P905 independently inside natural R-only GitHub Actions CTest and verify exact Git full SHA before comparison.
- For EACH candidate and EACH source/mode expected exact `exit=3`, empty stdout, first diagnostic `P6-JOIN-PRECISION`, preexisting sentinel unchanged, zero residual temp files; P897 vs P905 exact return code, stdout and stderr bytes. `LC_ALL=C`, inherited Actions PATH (unchanged), same scripts/env for both. Each of the 12 pairs must execute; first unexpected output fails closed.
- Historical R910 `--emit-c17 SOURCE` exit2 `P0-CLI-ARGUMENT` and first-Quartz CHECK pair PASS are retained as OLD, not retroactive evidence. No new emitted ordinary M8b C17/native claimed.

## CI gate / STOP
Branch MUST be created once, directly from the first completed R-only commit (parent fixed P905) at `r-fast/r912-true-pre-malloc-cli`. Natural push R-FAST max2 including branch creation; second only §7.6 mechanical. Real exact SHA R-FAST SUCCESS formatter/GCC frontend-only/CTest-N new tests BEFORE one DRAFT PR against immutable P905. Separate PR FULL5+PARTIAL at most 1 natural attempt pair; first unexpected unsafe result => STOP, NO push/retry/rebase/merge. New result can remain overall INCONCLUSIVE especially for private borrowed lifetime.