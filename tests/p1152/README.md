C228 / P1152: ordinary runtime condition into an existing original-T IF join

The independent source cohort was frozen before execution on PR1149's fixed
d66fdf751aa5823b5775cbaf346a6c02aba38182. `oracle.json`, `gate0-freeze.json`,
the full input archive, and `evidence/gate0-cli.zip` retain that first execution.
The four normal sources passed CHECK but failed emission with status 4 at the
root Unknown-Bool binding. Genuine loop and two-normal-arm IF current proofs
were already present. Neither a new issuer nor a state bridge was needed.

The four cases distinguish `<`/`==`, true/false, field 0/1, ADD/SUB, and
zero/one/multiple real backedges. Both arms use direct scoped replace of the
same original child, with different incoming bytes. After joining, the source
reads the updated child, unchanged sibling, and displaced old Copy.

`safety.c` uses actual source registration, actual `main()` CHECK, the public
loop/IF current validators, both issued Change events, and the private driver.
It also checks that the old known-only quantity/Bool consumers still reject
Unknown results; flags cannot manufacture an origin. Node/source/world,
callable, condition, operand, current, arm and ownership mutations must fail.
Canonical disposal is checked while A's loop/T-arm or body displays are
shortened, enlarged, or replaced by genuine B displays, followed by B reuse.

The OOM build records each reached allocation ordinal, callsite and status for
registration, invocation, and named emission. It requires rollback, no checked
publication, successful un-injected current retry, exact output bytes/inode/
mtime and no temporary-file leak. No production fault hooks are added.

`run.py` compiles/runs unmodified generated strict C17 before its passive
derivative. `observe.h` and inserted callbacks only read live original field
addresses, actual scalar phases/result, real comparison operands and Bool,
chosen arm, store operands/results, and old/new/sibling values. The original
comparison, branch, stores and results stay intact. Counters require exactly
one condition evaluation and one selected arm/store. After the T lifetime,
only counters are inspected. GCC/Clang and O0/O2 are selected by the runner.

The different-child source's original prediction was CHECK 0/backend 4, but
FIRST was the earlier CHECK 3 P6-JOIN-PRECISION. `observed-layers-v1.json`
separately records this boundary; the original prediction remains unchanged.
That backend case is NOT REACHED. Read-mode's first refusal is similarly the
earlier LOCAL-LOAN-PROFILE; it is not counted as reaching the mode validator.
Known-condition and existing-loop controls require byte-identical C17.

The successful initial harness is archived with its exact v1 manifest. V2
stores the immutable diagnostic C under `.c.source` to avoid treating archived
bytes as a live format target, and adds archive/lineage checks; no source,
oracle, original assertion or FIRST was rewritten. Prospective v37 externally
pins the fixed predecessor's v36 and inherits its protected history. Prior
v1-v36 data and old P1146 inputs remain unchanged.

Scope excludes new semantics, issuer/API/ABI, state bridges, T-changing or
nested loops, general range reasoning, nested IF and new helper grammars.
No general soundness or UB-audit-locality claim follows from this cohort.
Publication remains conditional on Issue1152 §6; local success does not adopt
PR1149. Audit locality is unmeasured, GO624 paused, H1 held, P1036/PR1050 stopped.
