C232 / P1158: genuine runtime loop-result condition into unit early RETURN

The independent source/oracle/runner/probe bytes were frozen before fixed
PR1155 af16bba execution. Four normal cases passed CHECK but failed backend4.
Actual public loop and IF-return current validators succeeded. The first
private refusal was the IF-return binding's known-only condition consumer.
Known one-RETURN and two-normal runtime controls emitted successfully. An
additional final RETURN is outside the existing single-RETURN proof contract
and remains backend4. Negative earlier CHECK refusals are preserved as such.

The implementation shares the existing authenticated runtime condition
consumer after each respective public IF current validator. It creates no
new authority. The existing normal replay, return target/ended state, and
one RETURN exit checks remain intact. Known-only scalar/Bool consumers and
other profiles retain their own contracts. The emitter is unchanged.

The four sources distinguish RETURN left/right, true/false, early/normal,
field 0/1, ADD/SUB and zero/one/multiple backedges. Each ends with unit tail,
not another RETURN. The original T remains unchanged during the loop. Only
the normal continuation uses the displaced Copy and updated child/sibling.

`safety.c` checks genuine normal replay and world-qualified RETURN-live /
RETURN-ended / normal poststate, rejecting cross-world substitution. It
also rejects wrong target/source/type/current/callable and malformed views,
and disposes A with shortened/enlarged/foreign B arm/body displays before
reusing B. `run.py` compiles/runs raw generated C before its passive version.
The observers only read actual operands/Bool, chosen arm, live original
field addresses, RETURN, store and postreads. Early RETURN requires no
normal store/read or uninitialized IF-result access; normal requires one
store and old/current/sibling reads. Only counters are inspected after T's
lifetime. No guest expression, condition, branch, store or result is replaced.

OOM CTests reuse unchanged `tests/p1152/safety.c` via `p1152_oom`. Its generic
registration/invocation/named-failure machinery accepts each new source path,
measures reached ordinals dynamically and checks status/rollback/retry and
no-clobber. Its printed C228 labels identify the reused equipment; C232 test
names and raw argv identify these new inputs. No duplicate OOM framework or
old fixture/oracle alteration is needed. `reused-equipment.json` pins it.

Scope excludes new source semantics, public issuer/API/ABI, bridge/snapshot,
new RETURN payload/ABI, nested/general IF, T-changing or general loops/heap.
No independent R inputs/reports were consulted. Finite success is not general
soundness or UB-audit-locality evidence. Publication remains conditional on
Issue1158 section6; new push/PR/CI stay zero before predecessor adoption.
