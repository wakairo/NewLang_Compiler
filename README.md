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
The implementation remains stopped; regression is incomplete and no compiler PR
is published. See https://github.com/wakairo/NewLang_Compiler/issues/1492#issuecomment-6938425113 .

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
