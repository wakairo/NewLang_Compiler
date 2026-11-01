# P1077: ordered direct writes inside one T field loan

C170/C171 starts at actual main `10c11e2031296aad73d6d11a06db596e642ddb14`,
canonical Draft17.34. CHECK and ordinary Copy call public current validators
accepted the new programs, but the private C17 `copy_loan` required an empty
body ending directly in a call/replace. Equivalent split-loan controls emitted C17.

Within the existing authenticated closed-H main continuation, a T field
write loan now accepts straight-line u8 bindings initialized by direct
replacements, pure u8 literals or Copy identifiers, and a corresponding u8 tail.
Replacements use the original scoped reference and the same child. Each step
must match the evolving original root/child incarnation and current facts.
Incoming batch operands are pure u8 identifiers/literals. The consumer creates
no proof, owner, reference, lifetime or semantic state. Local carriers are hidden
at exit while the original's updated ledger remains. The old empty-body route
retains its C bytes. Existing local/step/depth/resource limits remain.

The new region excludes nested loans/calls, extra roots, branches, reference
escape, arbitrary arithmetic and RETURN within the loan. Existing mixed-main
admission, two old-Copy helpers, H all-world completion, custody and final
cleanup remain mandatory. General CFG, third helpers, H-success rights in
failure worlds and new issuer/API/ABI are outside it.

Three P-owned profiles exercise both field indices and two original T carriers.
Each has two independently authenticated helper calls, a two-write loan and
use of the intermediate old Copy in a later write to the other original.
Split controls have the same physical effects; their parent C hashes are pinned.
The passive oracle checks each write, all other fields, original addresses,
actual caller Copy carriers and complete H cleanup in guest0/1/2 before T starts.
It reads generated carriers via existing passive hooks, replacing no operation.
Unmodified C17 is independently compiled and run.

`run-v2.py` checks real CHECK/GET/current, strict native O0/O2, seven batch graph
mutants with historical GET/current rejection separation, unchanged P-owned
Copy/H/world/foreign-B destruction probes, reached allocation failures, staged
output, named-output inode/bytes/no-temp preservation and exact retry. This is
finite P evidence, not independent R/F assessment. No independent R
source/oracle/raw/runner or stopped PR1050 was imported.

## Original evidence and versions

Complete sources and oracle-v1 were committed before execution in 9ca88f3.
Their duplicate H cleanup-local `raw` produced actual CHECK3; that original
prediction/source/oracle/FIRST remains unchanged. Source-v2 only renames those
cleanup receivers. Source-v3 adds field-index1 coverage, preserving both earlier
versions. First predictions are never retrospectively labeled PASS.

The environment PATH127, source CHECK3, observer ValueId-equality assumption and
first private compile failure are preserved in `evidence/FIRST-v1.json` and its
SHA/CRC-checked archive. Parent adapters are historical diagnostics, not current
CTests. Exact original C bytes remain as `.c.data` in
`evidence/parent-adapters`, with unchanged original maps and original Git commits;
the relocation map authenticates those bytes. The current observer traces
pure-binding origin instead of equating forwarded Copy ValueIds.
`run.py`/harness-v1 preserve the first successful two-case harness;
run-v2/harness-v2 add the third. No prior canonical fixture or oracle changed.

The first two profiles used a fresh actual-base Production build. The extra
index1 control used the existing P1071 GCC compiler with verified identical
Production/include/CMake tree to base10c, explicitly recorded as
source-equivalent rather than another fresh main build.

Prospective P990 v15 chains exact canonical main v14, preserving v1-v14.
Actual tracked new test/first-evidence inputs join the inherited cohort;
ambient caches and later result archives are excluded. RUN_SERIAL, TIMEOUT90
and workflows are unchanged. See Issue1077/RESULTS for final measured identity.

## Value and maintenance boundary

Combining scoped updates is a reusable selected-v0 value-flow step. Separate
loans remain a lower-cost workaround. One private header reuses the existing
bounded locals/steps/depth and staged output, with no new heap allocation or
global proof framework. Linear validation/emission and the inherited history
cohort remain costs. Current native timing proves no audit/performance gain.
Author plus ordinary-safe-caller UB-prevention audit/re-audit locality versus
well-factored C is unmeasured. Product GO PAUSED/H1 HOLD; READY/merge and
independent review remain separate.
