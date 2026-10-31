# P1071: final unit RETURN in the closed-main consumer

This is one ordinary function-result lowering step on main2cf49e4, under C161.
Draft17.34 §§18.5/19.1 already permit a final `return unit;`. The existing
closed-H → two independent T originals / two ordinary Copy helpers consumer
accepted a tail `unit` but rejected this equivalent final RETURN after CHECK
and genuine public Copy current validation succeeded.

The shared private predicate classifies a finite BLOCK ending in a literal
unit RETURN. It creates no evidence and performs no allocation. `block` still
emits every earlier operation. Main receives the function-body marker only
after the existing `copy_closed_plan` selects that consumer. The existing H
allocation-world completion, T/field/loan/current and final ownership checks
remain mandatory. The previously supported terminal-owner result uses the
same predicate. Neither a runtime jump nor an early cleanup exit is introduced.

Supported: the already authenticated closed-main source family, ending in
literal unit RETURN, across different H producer/cleanup and helper/field
orders. The three P-owned profiles change only main's final result spelling
from their unchanged P1062 controls. Generated C must equal both the current
control and its independently measured parent C hash, exactly.

Legal but outside this increment: nonliteral `return { unit };` (CHECK0,
C17 unsupported4), arbitrary early RETURN/CFG, arbitrary nested result
forwarding, a third helper, general used-read loans, or source families which
the existing closed-main planner cannot authenticate. The early failure-arm
RETURN fixture hits CHECK's existing allocation-world precision limit; it
does not demonstrate a source safety violation. Wrong result type is a
semantic error. These boundaries are coverage limits, not new source rules.

## Contract and evidence

`oracle-v1.json` and six complete sources were frozen in source-only commit
e4790f4 before Production edits and execution. Their immutable map is
`source-freeze-v1.json`; Issue1071 comment6540193052 records the pre-run hashes.
`evidence/parent-FIRST.json` preserves the original CLI/current observations,
including raw outputs, commands and parent control C hashes. It identifies the
source-equivalent historical baseline build rather than claiming it was
rebuilt from new main. `harness-freeze-v1.json` pins the new adapter and the
unchanged existing P-owned witnesses before its first execution.

Three CTests reuse P1062's actual CHECK/public-view observer, malformed-current
consumer, foreign-B destruction checks, reached-allocation OOM injection and
passive H/T physical observer. The adapter checks literal RETURN acceptance,
old-result forwarding, distinct originals and field/sibling values, all H
cleanup in guest0/1/2, GCC/Clang strict C17 O0/O2, named-output error/inode
preservation, exact retry, and conservative rejection of three source
negatives. The inherited witnesses remain unchanged.

There is no new global checker or CI framework. The existing prospective
history protocol needs v13 because v12 correctly pins the old serializer.
Original v1–v12 bytes and externally anchored first failures remain intact;
the current selector advances and the generated map adds only its predecessor
and the new finite input set (2530 total, +12). This inherited hash-table
maintenance is a cost of the current protocol, not evidence of compiler
capability or a reason to enlarge future features.

No R original source/oracle/raw/runner or stopped PR1050 code was used.
No public API/ABI, issuer, canonical semantics or private IR was added.
The runtime C is unchanged for the three measured pairs; performance on
unmeasured sources is not established. The classifier adds bounded linear
node walks, no heap object or ownership state.

## Value boundary

This reuses function result handling on v0's ordinary source→native path.
Using the already supported tail unit is an available alternative; a general
CFG mechanism would be a separate, larger increment. Reuse here reduces
duplicate private terminal predicates, without claiming a generic RETURN
implementation. Human author plus ordinary-safe-caller UB-prevention audit
and re-audit locality versus well-factored C, performance and product value
remain unmeasured. Capability success is neither general soundness nor
Product PASS/GO. See the Issue for the final local/CI and self-review receipt.
