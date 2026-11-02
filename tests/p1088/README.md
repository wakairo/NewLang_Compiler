# P1088 / C180 Production takeover

P takes over M's existing PR1086 by forward commits from failed75d9f7c.
Original M96be1419 and75d9f7c, two RED CI pairs, source/oracles, v1-v17,
and independent P/R evidence remain historical. Canonical Draft17.34 and
main93219b4 are unchanged. This is source/Break-edge provenance only.

## Diagnosis and repair

The inherited p14_w40 failure is t2,n357, nested unit Break then outer Copy
Break. No wrapped allocation failed at that index: CHECK and disabled-injection
retry both return INTERNAL=6/P3-INTERNAL. The single-direct-Break observer's
cohort restriction had been imposed on otherwise valid nested-loop CHECK.
Remove that new rejection; actual source-specific append still validates the
checked node and operand before issuing each owned edge. P6 remains mandatory.

A separately frozen P negative also showed foreign same-layout exit-state
substitution passing the old boolean. Private exit records now bind original
source/body/parent/context/node storage, operand/node seals, target, result and owned
state. Union preserves this binding and rejects modified observations without
publication. Destruction follows private state/target handles. The validator
checks those identities before dereferencing challenged state/target pointers.
It remains allocation-free and grants no current, owner or caller result.

Unchanged p14_w40 and M provenance tests pass locally. New P tests cover
independent same-ID graphs/worlds, NULL/foreign state, coedited foreign targets,
source/body/graph/count/kind/result/item tampering, duplicate edge, shallow
parent/body, nested nearest targets, wrong scope and control profiles. A is
destroyed with B's whole observed edge installed; B must remain usable.

The extended OOM sweep requires exact OOM=4, NULL artifact, diagnostic,
original TestState no-clobber, cleanup and successful same-context retry at
each reached failure. Its eighth source grows the checked-node array from16 to32.
Actual reached allocation sites and per-source counts are recorded in evidence/oom-reached-summary.json. Correction: that
eighth source grows at identifier17; its actual Break is24. The original
prediction and mistaken node17 observer RED are retained, including its
early-return LSan report; a cleanup-preserving observer confirms actual24
and genuine provenance TRUE without leaks. The ninth source (four bindings,
one unit statement, then Break) actually has Break17 and forces its own
realloc. It adds221 reached OOMs, including Break add itself, validates real
provenance on every retry, and keeps every prior source/oracle unchanged.
Its first C harness-generation compile RED is also retained. See the separate
actual-break-add-oom.json for exact sites. The old strict test
and rollback checks are unchanged.

The first P diagnostic v1 incorrectly annotated numeric OOM as2; the named
status expectation was OOM throughout. Actual enum is4. Original v1, retry
RED and later status-first diagnostic are archived with their pre-run freezes.
Candidate1's wrong-scope negative used a bare top-level Break item and failed
in the parser. Its source and RED remain archived; candidate2 uses the required
lexical block and still requires semantic wrong-context rejection.
Clang23's strict build flagged the deliberately concatenated eighth C string
in the array. Its first build RED and pre-fix inputs are retained; explicit
parentheses preserve the complete NewLang source bytes and all OOM oracles.
The first local prospective-v18 candidate is archived before its forward
mechanical update; failed commits and evidence are never overwritten.
Self-review's unowned shallow-parent false acceptance is retained separately.
The actual source-checking parent address is now a private issuance identity,
including for nested loops; an observation cannot supply a new parent namespace.

## Forward history and evidence

Original v17 is401990 bytes, SHA256
3fc39dc0cae85a051e77e814d4d54c3d5c33d2bea955ea5d9db1c2d900b9a0b6.
The unchanged renderer produces401993 bytes: escaping U+2194 **and sorting
file keys** are both required for exact bytes. Parsed objects are equal.
Prospective v18 preserves v17 and all2754 keys, adds Git-managed P evidence,
uses ASCII stage text and the existing renderer. No history checker is weakened.
Archived pre-run input zips validate old source/oracle hashes without freezing
future legitimate Production changes. The original failed v17 SHA/blob are
externally pinned by the new evidence test.

Original M logs are retained in both the initially reported transport copy
(one extra EOF newline per log) and exact decoded-response byte archive. The
exact map records every job's SHA. GitHub's original runs are unchanged.

## Boundary, costs and next gate

No field-current replay, normal-break caller result/current projection, generic
cyclic state, C17 loop lowering, public current issuer, owner/ABI or source law
is added. Original P1081 effectful source still CHECK3/P6. Existing native,
owner, H/T, history and sanitizer regression gates must pass before any push.
Private seal storage replaces the former borrowed-view seals; bounded exit
records add owner/state/identity fields and three small tests plus evidence check.
There is no new IR or source-shape backend workaround.

VALUE-ALIGNMENT-RISK: this is a reusable v0 prerequisite, not an end-to-end loop
capability or Product PASS. NewLang versus well-factored C library-author and
ordinary-safe caller UB audit/re-audit dependency closure remains unmeasured.
Product GO #624 PAUSED and H1 #268 HOLD remain. Defer caller projection and
native loops until their genuine trust contract is reviewed. PR stays DRAFT/HOLD
for independent F exact-HEAD review and Coordination acceptance.
