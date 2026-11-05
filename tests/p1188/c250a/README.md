# C250a phase-discard repair

F1192-1 concerns the C17 carrier for a source-owned usize discarded in the
continuing arm of the original-T loop. A phase symbol belongs to its iteration
world. Its numeric ID also names the caller's result local, which is assigned
only on Break. The two roles must not share that C local.

The unchanged fixed8585 compiler was built in the same Debian 13.6 environment.
The four new sources, two unchanged controls, oracle, source hashes and mapping
equipment were frozen first. CHECK and emission succeed in all six cases.
The direct-helper and literal cases each emit one reference to the caller's
unassigned result local inside the loop. Their role traces and exact generated
bytes are in `evidence/FIRST-fixed8585-v1.zip`. No original generated guest was
compiled or run. Alias and control cases are retained as successful controls.

The repair changes only `src/main.c`. Its current-loop statement dispatcher
passes usize discards to the existing phase-aware quantity emitter, as it
already does for Copy bindings and helper actuals. The actual phase maps to
`nl_loop_phase_*`; aliases retain their own initialized `nl_local_*` carriers.
It does not initialize the result early, remove a proof check, or change CHECK,
root/header/guard acceptance, allocation, ownership, graph, or seal behavior.
The same dispatcher serves helper, inline-IF and literal changing-current
profiles. Loan statements remain u8; outer statements and stutter dispatch are
unchanged. This is a carrier repair, not additional loop semantics.

`run.py` authenticates the frozen inputs, obtains actual source-issued roles,
and rejects every pre-Break caller-result reference before preparing a native
compile command. It checks all direct/alias discard tokens against their
source roles. The four targeted sources cover direct phase, one/two-link alias
discard before/after the writer, and a literal profile. Two old controls must
keep their generated C17 bytes. Only repaired C passes to strict native builds
at O0/O2, raw and passive. Passive observations check actual discard values,
helper input/result, phase/Break values and the physical original's addresses,
old/current field and sibling. The observer does not supply guest values.

Existing P1188 source/oracles and signature/no-clobber/negative assertions are
retained. C250a permits reusing their reached OOM evidence: no allocation,
owner, graph or seal code changed here. The remaining one natural six-job CI
pair is the last of the original twelve-job publication budget; manual0.

Unsupported legal source families and the original finite Copy/call/loop
limits in the parent README remain. Independent repaired-head F/R review,
general soundness and audit locality are not established by these tests.
VALUE-ALIGNMENT-RISK: ordinary discard is reusable source, but separate C
carriers for world-local roles remain a maintenance boundary. This change
reuses the existing mapper and adds no retained proofs. Audit locality remains
UNMEASURED; GO624 PAUSED, H1 HOLD and P1036/PR1050 STOP remain.
