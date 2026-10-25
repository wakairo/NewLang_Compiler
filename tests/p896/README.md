# P896 — private, non-publishing definition CALL diagnostics

Fixed base0981a87db7121fd357d88b964d6b0040a961f41b; Draft17.34.
M895's construction contract and F66's unproved source adapter are inputs,
not public proof or a formal soundness result. Gate A and pre-code independent
ChannelProfile source/oracle/SHA are preserved in freeze.json/evidence.
Original P793 Router and P892 TransferBudget are unchanged source bytes.

The private thread-local observer is OFF by default, has no CLI/public API,
and grants no current authority. It is a synchronous test/diagnostic channel,
not a hidden owner registry or public IF event. No new capture allocations run
in ordinary newlangc. With an observer, run_body captures an owned PRE after
ordered argument checking/reference_live and BEFORE fresh formal binding.
After real body attachment it captures observed call-end data. At completed
arm/save_arm it invokes the observer while graph/source/contexts still live,
then destroys both snapshots and the pending list, including on errors.
Callbacks may not retain/dereference these borrowed pointers after return.

The independent verifier derives the exact scoped loan/ref, ordered arguments
and typed literal from registered caller AST + completed checked arm and
source-owned IF entry. It replays PRE on a separate clone, binds fresh formals,
interprets registered callee AST + every checked executing node, applies one
real fixed-field Change, and tracks old-result def-use through Copy bindings.
It verifies actual callee RETURN edge/target/qualified state, formal teardown,
observed call end, original/sibling/other-root frame, then enclosing loan close.
Observed POST/expected numerical oracle is NOT an assumed precondition.
Candidate-view equality is only an identity check; real graph/world/RETURN
mutations are independently replay-rejected. Unknown scalar flags remain
explicit; historical old Copy is not a live old-ValueFact dependency.

The finite profile has two lexical originals of one exact two-u8 nominal,
one call in each normal arm, ordinary scoped write and plain u8 actual1 literal,
no H/Array/heap ownership, no general effects/CFG. Formal storage is not the
caller referent. Caller/callee graphs are separate namespaces while run_body
shares a semantic context; fork contexts remain distinct. No public Copy
capture is enabled for definition instances (!body.definition is unchanged).
No if_field/main/public header/schema or admission guard is modified.

Run CTest p896_definition_capture.unit (six unchanged/frozen inputs),
p896_private_negative.unit (wrong metadata and actual graph/world/RETURN;
RETURN-incoming, RETURN-literal73 despite old numeric73; no-call),
p896_capture_oom.unit, p896_unchanged_cli.integration.
The OOM test reaches capture calloc and PRE-clone calloc separately; diagnostic
OOM is reported without changing the existing source rejection. It checks
transactional original public state and retries the SAME context/input, then
checks a clean independent run. This is host diagnostic OOM, not NewLang heap
allocation failure. Verifier-wide exhaustive fault injection is NOT claimed.

Every effectful positive still returns CHECK3 P6-JOIN-PRECISION and zero C,
with a named-output sentinel preserved. Public IF and M8b native remain
UNREACHED. The no-call control CHECK0 does not constitute a call observation.
Full ordinary CLI before/after outputs and diagnostics are byte compared in
27 mode/input comparisons. No independent R or Product GO claim.

Initial local failures are retained: missing private-header stdint include,
harness diagnostic/const types, replay omitted implicit_local on u8 source
locals, and a harness assumption that the prelude had zero functions. The
replay was corrected to match actual source operations, and rollback now
compares the complete original public snapshot. A later RETURN-state audit
hit the legacy non-const control-state getter signature; the following CTest
used the previous executable because that shell lacked set -e. That log is
explicitly NOT CANDIDATE evidence. The getter is used read-only with the same
private cast pattern as existing consumers; all final build/test chains use
set -e and freshly rebuilt executables. No source/oracle/expected CHECK3 was
retuned. All remote CI, if launched, is one natural FULL5+PARTIAL attempt only.

This is reusable Stage0 technical observation, not IF composition/public
issuance/currentness join or native execution. A next Stage1 needs separate
C authorization and independent scrutiny. Additional replay/maintenance
closure may harm audit locality; benefit vs well-factored C is unmeasured.
Product GO PAUSED / H1 HOLD remain.
