# C222 guarded computed carry (P-owned)

Fixed main: `28726f933ff4885c000e0247cf4fd9ac1859c5b1`, Draft17.34
§§5.2, 27.5–27.7. This is finite compiler capability evidence. Independent
F/R review, general loop soundness and Product GO are separate decisions.

## Supported family and inductive argument

One literal-initialized usize loop slot; the complete body is one IF with
condition `phase < usize(L)`, one direct `continue(phase +/- usize(k))`, and
one phase-independent normal Break. The Break uses the existing scoped
fixed-field Change/ordinary Copy helper kernel and real caller replay.

`H = exact persistent E × arbitrary dependency-free usize`. Entry is in H.
For ADD on the actual true arm, `L>0` and `k<=SIZE_MAX-(L-1)` imply the
mathematical result is representable for every `phase<L`. For SUB on the
actual false arm, `L>0` and `k<=L` imply representability for every
`phase>=L`. Each successor is again a free usize in H. The exhaustive
Continue grammar cannot write E. Fresh binding/scope exit and exact current,
dependency and responsibility checks remain mandatory. B is checked from H
and does not feed back to it. No termination or least fixed point is claimed.

The private arithmetic event is issued only during the actual nearest-loop
walk: same source node, target, IF parent and arm, and the same fresh binding's
place/incarnation/current fact. It is retained inside the existing loop
record and rechecked with the complete graph grammar and existing canonical
3 graph/7 world seals. There is no additional world snapshot, public receipt,
authority registry or issuer. Unknown operands/results do not receive the
known-number addition/subtraction receipts. The C17 consumer computes the
next value from the real current phase, rather than replaying a fixed literal.

Excluded: T changes on Continue, multiple/nested C/B, variable guards/steps,
commuted operands, equality-derived arithmetic, general range inference,
L=0 for computed carry, H heap and new source/API/ABI/issuer semantics.
Some excluded sources are legal; they remain precision/coverage work.

## Evidence and tests

`evidence/gate0-v1.zip` preserves complete independent source/oracle/runner
freeze and the fixed-main FIRST: ADD/SUB precision status3. `oracle.json`
and the original 18 source files remain byte exact. No R input was used.

`safety.c` uses genuine registration and an actual main CHECK. It verifies
private induction, public Copy GET/current/parameters/Change, absent public
known-number receipts, modified source/guard/operand/arm/target/current
refusals, canonical A destruction while foreign B survives, and shortened/
enlarged/foreign child view arrays. Altered graphs are never emitted or run.
Its OOM mode enumerates every reached registration/invocation/named CLI
allocation, with exact status, rollback, no-publication, cleanup and retry.

`run.py` checks eight cohorts and ten negatives. Unmodified generated C and
read-only observed C are distinct artifacts. GCC/Clang O0/O2 observation
checks 0/1/2/3 actual backedges and the full phase trace, same original field
addresses, sibling/ambient storage, one helper Change, old/postcurrent and
caller reuse. No observer writes storage or injects checker authority.

`bounds/` independently freezes +0 static-only (infinite native NOT RUN),
L=0 refusal, unsafe all-H transfers even when entry takes B, and an actual
ending lexical loan dependency refusal. The original source99 only reaches
unknown-callee rejection and is not counted as that dependency test.

`evidence/inputs-v1.zip` and `native-FIRST-v1.zip` retain the original strict
observer build RED (`unsigned < 0` for the zero-backedge control). The v2
observer expresses the same bound via `phases<=count` together with the
existing `phases==continues+1`; it does not weaken the native oracle. Original
freeze v1, source/oracle and Production remain intact. Prospective v34 pins
adopted v33 SHA256 and Git blob and inherits all old protected evidence.

Audit locality: UNMEASURED. GO624 PAUSED / H1 HOLD. No READY/merge/Product GO.
