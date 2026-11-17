# Track P evidence preservation

This independent, data-only branch preserves local Track P evidence. It contains
no workflow or buildable compiler tree, is not a Production candidate branch,
and does not authorize a release, merge, rerun, or Product GO. Large archives use
Git LFS. Small source-recovery bundles and manifests use ordinary Git.

The repository owner explicitly requested preservation on GitHub and safe local
cleanup on 2026-11-17. A local archive may be removed only after the remote file
has been independently downloaded and its SHA256 and length verified. Source,
oracle, FIRST/RED and actual raw evidence must not be edited during housekeeping.

## P1492 C1103 STOP

Authority: main f7c277af59a4c4281c07fd6d3cc1d98f8bf88f48, Draft17.38.
Saved candidate: ad476afaf329bbdadbd49385a612c6fdfb5cc434,
tree b77c1f20d0a235f994452130ff78e6d24141e007.
At that original C1103 STOP, regression was incomplete and no compiler PR had
been published. That failure remains historical evidence, unchanged by the
separate prospective C1109 recovery below. See
https://github.com/wakairo/NewLang_Compiler/issues/1492#issuecomment-6938425113 .

`P1492-C1103-STOP.bundle` is a standard Git bundle requiring the authority commit.
Fetch that existing public main commit, run `git bundle verify`, then fetch the
bundle's `p1492-guarded-usize-result` ref into an isolated local branch. It contains
the complete forward source history, fixtures and unchanged prospective oracle.

`P1492-C1103-STOP-evidence-20261117.tar.gz` contains the original execution
ledgers, raw stdout/stderr, generated C, native receipts, measured GCC/sanitizer
ELFs, configurations, FIRST/RED and a per-file SHA256 manifest. These are bounded
measurements, not complete CI evidence or a claim of general soundness.

## Historical measured evidence

The following archives contain a per-file `ARCHIVE-MANIFEST.json` with original
absolute root, relative path, exact length and SHA256. Each archived directory
is retained locally until its full remote Git LFS object has been independently
downloaded and the complete member manifest verified.

- `P1393-C795-preserved-C7-ELFs.tar.gz`: the original measured C7 host ELFs from
  `/workspace/p1393-c795/preserved-C7-ELFs`, including the preserved safety STOP.
- `P1260-C314-measured-binaries-and-raw.tar.gz`: measured GCC/Clang/sanitizer
  binaries, original raw and configurations from
  `/workspace/p1260-c314/actual-local-binaries-and-raw`.
- `P1253-C299-measured-local-results.tar.gz`: original baseline/prototype and
  measured host binaries/raw from `/workspace/p1253-c299/durable-local-results`.

Relocation does not change any source, oracle, expectation, FIRST/RED, original
status or historical CI claim. The manifests make restoration of exact bytes
possible; no SHA-only reconstruction or regenerated result is substituted.

## P1492 C1109 forward recovery

Fixed R1: c4997882f981db5ba89dfeb475fe6fc283db2cfb, tree
432c88701cb3cef799c8d6cc9518af2fdd836aa1. `P1492-C1109-R1-c499788.bundle`
preserves the complete forward source history against adopted f7 main.

`P1492-C1109-complete-local-evidence.tar.gz` is 216935389 bytes, SHA256
2f5f87b7374b1eac687b6f116f25331241bd2391d42f8a7a98fd46320f34ee5e.
It contains121 outer members including complete raw/config/XML receipts and
individually verified measured-output/feature-ELF suite archives. Actual results
are1124/1124, no fail/skip, for GCC Release, Clang23.1.2, ASan/LSan, UBSan and
NO-LLVM Debug. Earlier apparatus and old-driver REDs remain separate inside;
all local configurations are frontend-only and do not establish LLVM smoke or
general soundness. The original exhausted36/28/16 ledger is not reset.

Local acceptance: Issue1492 comment6943046427. Conditional ordinary DRAFT/HOLD
PR1495 is the Production review surface, separate from this data-only branch.
CI results must be read from their own same-head natural runs, not inferred
from storage or the local archive. Publication receipts are separate small
Git files. No READY/merge/Product GO is performed by this archive.
