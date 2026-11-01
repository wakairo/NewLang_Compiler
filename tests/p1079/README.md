# P1079: consume an existing effectful IF join in ordinary C17

C173 starts at actual main `626829e22cc3649a99bcbce998dda4aa0cfab22f`,
canonical CURRENT_SPEC → Draft17.34. The existing main push was independently
confirmed: FULL5 run41290053391, 598 named tests per job; PARTIAL41290053425,
597 plus real CLI CHECK. This is a source capability increment, not Product GO.

The selected next capability is a two-normal-arm conditional field update and
its returned old Copy, in main's continuation after every H outcome completes.
The alternative loop/break result has an earlier CHECK precision boundary
(`P6-JOIN-PRECISION`); general iteration additionally needs header/backedge/exit
correspondence. IF fork/join is the bounded control primitive taken first.
Existing straight-line split loans remain a usable alternative.

The private consumer uses the existing public IF-field GET/current validator.
Each arm opens and ends its own write loan and makes one direct replacement on
the same original child. Known u8 inputs are literals or immutable caller
Copy bindings. The predicate is an existing literal usize comparison (`<`/`==`).
Both actual arms are validated, including the unchosen arm; emission reads
the physical field before writing and forwards that actual old value. The
common continuation receives the validator's structural poststate, including
its sound Unknown value; no fork-local ID or chosen-arm fact becomes authority
in main. The original root, child and sibling stay in the physical Local ledger.
The returned Copy can feed a subsequent existing direct write on the other T.

Only `checked_c_node.c` and a backend-private header change Production.
There is no public issuer/API/ABI, source rule, semantic state, new IR, toolchain,
H ownership or canonical destructor change. Existing closed-H all-world checks,
two ordinary helpers, staged output and resource limits remain mandatory.
The old literal-only IF path and existing C outputs are retained.

## Boundaries and first observations

The original v1 source predicted a backend gap but first failed syntax
(`P13-IF-OPEN`, required condition parentheses). Original source/oracle/raw remain.
Version2 adds those parentheses: CHECK0, ordinary Copy current0, historical
IF-field GET1, but IF current3. Existing replay requires an arm-owned loan or
a real UNIT formal; the outer-loan ref shape has no current receipt accepted
by that validator. It remains unsupported. Version3 moves each loan into its
arm: genuine IF/event current0 and parent C17 exit4. No issuer was extended.
Version4 adds complementary child-index1 and equality coverage.

Sources and oracles were committed before each initial execution. Parent v1–v3
raw and SHA inventories are in `evidence/`; candidate and harness first REDs
are retained separately. Private include ordering, completed-context lexical
retirement, an extra observer initializer, and an old test's total-arm-count
assumption were repaired without replacing earlier predictions or first results.
The entry checkpoint authenticates scalar availability; the completed caller
only confirms the original binding identity after lexical retirement.

The new region excludes an IF using an enclosing loan, arm statements/multiple
Changes, distinct changed children/roots, nested calls/loans/branches, arm RETURN,
unknown predicates, arbitrary arithmetic, loop precision, and third helpers.
These are unsupported boundaries, not language prohibitions. The original
escaped-ref and missing H-failure-cleanup sources remain CHECK rejections.

## Verification and costs

`run-v4.py` uses three new independent P programs (true/false, both child indices)
and the same unchanged public validators, actual H/T carriers and strict C17
native O0/O2. Passive hooks check all fields and original addresses at every
write, actual caller old Copy results, guest0/1/2 and complete H teardown before
T starts. Unmodified generated C is also compiled and run.

New graph tests cover parent/arm node, field/fact/incarnation/world/source,
candidate views, cardinality and foreign B's entire IF arms. A is destroyed
without restoring B's arms; B must remain current, emit identical C and destroy.
Existing P-owned Copy/H/world probes are reused with a new adapter selecting
the actual H MATCH instead of assuming main has exactly two arms. Original
P1062 code is unchanged. Reached backend allocation faults require unpublished
output, unchanged length/context/owner and exact retry; conservative UNSUPPORTED
and OOM are counted separately. Named-output bytes/inode/temporary preservation
and inherited C-byte controls remain checked.

P990 v16 is a prospective successor of exact main v15; v1–v15 and old evidence
are unchanged. New inputs come from Git-managed files, excluding caches and
later result archives. No global timeout, RUN_SERIAL or CI workflow changes.
Three real integration tests and one bounded consumer add validation and
maintenance cost. No independent R source/oracle/raw/runner or stopped PR1050
was read or imported. P observations do not replace independent F/R review.

VALUE-ALIGNMENT-RISK: this advances selected control/value flow but does not
establish whole-language soundness or author-plus-ordinary-safe-caller audit
locality versus well-factored C. Dynamic general control and library audit
dependency closure remain unmeasured. Avoid expanding fixture-specific receipt
families as a substitute for required v0 mechanisms. Product GO PAUSED/H1 HOLD;
READY, merge and independent review remain separate decisions.
