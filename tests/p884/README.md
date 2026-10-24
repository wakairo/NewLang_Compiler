# P884: controlled current-main integration

Fixed main/base:73ddf7b108ef5db690bfac98e1264901619b5ced,
tree37a255d33ce8d91a5f3911930e5fa6d21da94421, canonical Draft17.34.
New main-based branch p884-main-if-unit-integration. Historical P822/P872/P878
branches and R875/R881/R883 evidence are not rebased, edited or regraded.

Before edits, all3 permitted main production files exactly matched the old
stack base d04341f816853b241e8f2b0dc4a4cbcc14a14dc6. Only the selected per-file
patches were applied; there is no blanket merge or old-tree replacement:

| Path | Main blob | Integrated blob, identical to fixed P878 |
| --- | --- | --- |
| src/semantic_check.c | 1be7c4fea080c696e9152250df682d57523347ce | 0e6185b42f6675346677c2c79ee88f0d0c75ca51 |
| src/copy_body.c | 7a2da394d5e4fd8090e4dde735e5f25df5f13641 | c5711aa46f3759102bf655e59eb0cbc6b4d2d294 |
| src/main.c | 20205fa5dbe9e1487b973f69ed6edefbee2af73a | 2554972faf57f72e3a315699eb47d443befffc09 |

The5-line completed IF-arm seal,16-line selected equality prefix delta, and
27-line unit-loan binding delta preserve original source-issued worlds,
old Copy/Change/Reset, caller conservative Unknown, and once-only no-payload
unit emission. No new source grammar, issuer/schema/authority or ABI is added.

Crucial modern differences are deliberately retained: checked_c_copy_internal.h
21a680815276de81b0a14b98f4dfa19b5a6e3c40 keeps recursive H-arm traversal and
copy_carrier_world; checked_c_node.c and Array4 parser88951b36fe5258eeec2bd5295d7d2f6d8ee92c63
stay untouched. The R-FAST workflow9fea1afeeef948127c73e9ca86c476091036116e
and all41 audited modern H+T/Array4/toolchain/workflow paths retain exact blobs.
R-FAST is only an opt-in r-fast/** push workflow; it is not invoked here.

Reuse48 immutable P-owned test files from fixed P878
670f976d51cff52efe182d32fcb37aed3394063b. No new source/value oracle is needed.
Only2 essential historical freeze JSON manifests are copied (3829B total),
because unchanged p817 CLI tests reference them; no old raw evidence logs are
shipped as files. All50 imported blobs are identical. Current CMake registration
is retained byte-for-byte around an additive61-line block adding14 tests:
P8175 + P8664 + P8785. Historical P813/P817 selected CHECK/public positives
intentionally retain backend CLI4; their expectations are not relaxed.

Local baseline:456/456 actual frontend PASS. Initial integrated full frontend:
470/470 PASS, preserving all456 names +14. Selected H+T/Array4/IF/Copy/unit-loan:
23/23 PASS with GCC13, and complete host/guest Clang23.1.3 ASan and UBSan runs
23/23 each. Strict O0/O2 plain C17 and separate source-original observations
for P866/P878, retained H+T and normal Array4 pass with GCC13 and Clang23.1.3.
Negative/unsupported distinctions, named-output sentinels, temporary cleanup,
source/world/field/old Copy tamper and actual reached OOM are preserved.

First additional local execution mistakes are retained, not called compiler
failures: a manual Clang Array4 invocation passed p791_issuer_oom instead of
the registered p694_fault_test, which rejected its argv with exit2; the two
selected sanitizer builds also omitted p694_fault_test, so initial focused
runs stopped at FileNotFoundError after four Array tests. Original logs remain.
Correcting only invocation/build-target selection to actual CTest registration
completed all checks. No production or test-oracle fix was made:0 production
correction cycles,1 local execution-setup correction. No unexpected source,
public proof, guest-value, sanitizer or cleanup defect occurred.

Real OOM includes completed IF-arm sealing with no published attachments and
unchanged worlds/retry0, prefix calloc indices0/1 on each of two old/new P
sources, Array parser/issuer and atomic-output failure cleanup, H owner
allocation worlds0/1/2 with matching frees, and selected driver faults.
Only actually reached positions are claimed; this is not exhaustive soundness.

A separate producer NON-BLIND smoke of unchanged R875 sources on the integrated
compiler yields CHECK/public/C17 and strict GCC/Clang O0/O2 plain native0.
Their2117/2118B raw C17 hashes68382cb5.../bc26f120... match old P878 receipts.
These source copies/results stay outside the repo. This does not transfer R883's
old-base finite PASS to modern main or upgrade old INCONCLUSIVE grades.

Detailed audit, raw C17, separate P passive files and SHA manifests, actual
inventories, first failure and recovery logs are preserved outside tracked
files and in the integration commit's lossless evidence archive. Issue884 is
the result record. Only after local gates/pinned format PASS may one fixed-HEAD
DRAFT PR run a natural FULL5(5x471) + NO LLVM PARTIAL(470), attempt1 only.
No retries, after-PR pushes or merge are authorized by this candidate.

Historical design KEEP: Draft17.34 unit/Copy/call/loan/field/return identities,
DI017/018, current direct-C17 policy. Product Strategy1.1 author+ordinary-safe
caller UB audit/re-audit locality vs well-factored C is UNMEASURED. Finite P
integration is not independent modern-main R or Product GO. Product GO PAUSED,
H1 HOLD. Adoption/new R runtime RETURN coverage are Coordination decisions.
