# P901 — statement-wrapped IF in the private Stage0 diagnostic

Base P897 b65c39fac1cf56e276c03a53fff802fb0c243957, tree1721a4d1fbd75d0c6cb87ee33ce0a674dabfbdc2.
Compiler/main0981a87db7121fd357d88b964d6b0040a961f41b, Draft17.34.
This is P-owned NON-BLIND calibrated regression evidence, not independent R.

R899 V2/source-only inputs are copied byte-identically from frozen R900
270533d84109c9490afb7c0c13a89cdcc0fecc02, SHA7fbf208d.../8672a123... .
No R harness, frozen oracle, test expectations, labels or CI were changed.
R899 V2 finite private positive FAIL, old V1/R898 INCONCLUSIVE stay historical.
R's no-call assertion expected rollback after successful registration; that
was an R harness defect, not evidence of P corruption, and is not repaired here.

Read-only Gate A ran BEFORE branch/code. It showed all earlier private gates
true, exact registered body/source/checked owner, exactly one matching IF span
under STATEMENT.expression, and exactly one checked IF with that span. The old
walker could not find it; private status3/PRE0/failure_line0 in each arm. Source
registration still failed P6/CHECK3. Logs and the P-only read-only topology tool
are preserved under evidence; this did not mutate a semantic world or admit IF.

The sole production repair is two lines: dc_find_if descends the statement
expression at depth+1. Exact span/registered owner/depth16 and all later
arm/loan/callee/RETURN/replay guards remain unchanged. No public proof/schema,
currentness join, compiler admission, backend, parser or source semantics change.

New ScanPolicy TRUE/FALSE and RETURN-incoming/same-numeric-literal-old controls
were SHA/oracle frozen before code. A preceding unrelated pure IF tests exact
matching rather than selecting the first IF in a body. P901 probe reuses ONLY
the P896 harness, requires genuine PRE/RETURN-old/loan close first, asserts
independent source numerical/index oracles, then performs real owner/span and
unrelated-IF mutations plus inherited actual/formal/field/world/RETURN faults.
The P no-call control expects CHECK0 and a committed registration, not rollback.

Four new CTests: p901_statement_positive.unit, p901_statement_integrity.unit,
p901_statement_oom.unit, p901_cli_unchanged.integration. The OOM test reuses the
P896 P-owned capture/PRE-clone calloc injection on unchanged R Meridian bytes,
including healthy companion verification, original public-state rollback and
SAME-context retry. This is not R's malloc harness nor guest allocation failure.
No new allocation was added; exhaustive verifier/call-end OOM is NOT claimed.

Default CLI is observer-off. Twenty-one input/mode comparisons against fixed
P897 are byte-identical. All effectful programs retain CHECK3/P6/zero C/named
sentinel intact. New M8b public proof and emitted C17/native remain unreached.
Local first-run targeted/full/sanitizer/Release results are retained. No further
positive mismatch occurred. Any first CI failure requires STOP; no manual retry,
post-CI repair push, rebase or merge. New PR is stacked against fixed P897 only.
Product GO PAUSED/H1 HOLD and well-factored-C audit-locality UNMEASURED remain.
