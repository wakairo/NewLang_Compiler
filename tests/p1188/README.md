# C247: ordinary usize Copy at three current-loop/helper boundaries

Base: repaired but unadopted PR1186 `faad092b5b0deb608a048ff60bee0056082f9a59`,
tree `01388ae82b1749453546933cb4ca6b311a25df77`. Canonical authority is
CURRENT_SPEC → Draft17.34. The original PR and all predecessor evidence stay fixed.

The bounded increment lets ordinary names separate three pure quantities:

- A source usize literal, optionally through its ordinary seed binding and at
  most two identifier-to-identifier Copy bindings, supplies the loop initializer.
- The actual current iteration phase, through at most two such Copy bindings,
  supplies the sole ordinary usize→u8 helper actual.
- That call's fresh formal, through at most two such Copy bindings, supplies the
  left operand of its one LT/EQ IF against a direct usize literal.

All existing constraints on unit main, one ordinary two-u8 original, one selected
fixed child/sibling, one carry, direct phase guard, representable positive
constant ADD, one Continue and actual-phase Break remain. The helper still has
two literal-u8 normal arms and a normal tail result; its existing u8 result
binding/Copy routes feed one scoped replacement. Names/literals and either field
are reusable; none of the new proof code tests fixture-specific names or numbers.
The new sources separately and jointly exercise all three boundaries, including
one and multiple actual edges. Existing P1184 coverage checks the zero-edge path.

Three Production-private files change. `current_loop_check_internal.h` shares
one allocation-free bounded checked-declaration origin walk across all three
boundaries. Every step checks the node, source/lexical name, exact world-local
symbol, source order, receiver, semantic value/type and dependency-free package.
Initializer provenance terminates at a real literal; actual/formal provenance
terminates at the exact phase/fresh-formal symbol. Bits or Unknown/type alone do
not establish origin. Binding and initializer IDs decrease, so the walk has no
cycle or recursive C-stack growth. Construction uses its actual `Check.context`;
published validation uses the artifact world after the existing origin/seal checks.

The source envelope selects a candidate named argument/initializer; it grants no
provenance or memory authority. The complete checked relation subsequently
rejects foreign producers, arithmetic, longer chains and incompatible values.
The iteration walk admits the authenticated usize Copy locals alongside existing
u8 locals, without changing the loan/new-value/current/frame rules. The private
consumer returns the authenticated caller initializer node. C17 emits that actual
initializer and each actual Copy local and helper argument; it emits neither a
host expectation nor a cached known value in place of a source Copy. Direct
literal/phase paths retain their original output bytes.

The original six seal slots, H/frame checks, complete prefix packages, fresh
root/child facts, sibling preservation, all leaf/union transfers, actual caller
Break projection and canonical child destruction remain. There is no additional
retained graph/context, public issuer/API/ABI, source law or general-loop method.

Unsupported legal forms include a third Copy link, aliases in the outer guard
or Continue arithmetic, arbitrary computed/dynamic initializers, arithmetic in
the actual/formal chain, an unrelated same-bits producer, an aliased RHS bound,
explicit helper RETURN in this composition, additional helpers/writers/carries/
loops, recursive/unknown/effectful/ref helpers, heap/nonCopy/dependency-bearing H.
A conservative status3 for such forms is a coverage limit, not an unsafety claim.

## Evidence and checks

Original `source-freeze-v1.json` / `oracle-v1.json`, the fixed-candidate FIRST,
target-runner correction, first local conservative rejection, census selection
error and equipment compile/format/history/timeout observations are retained in
Git and/or the evidence packets. Frozen source and expected semantic values have
not been changed to obtain a PASS. `evidence_test.py` verifies original records,
archived equipment versions and the current formatted successor.

The current native equipment is `native-v3.py` with the pre-execution
`native-oracle-v1.json`. It checks ordinary CLI CHECK/current/C17/named output,
strict C17 O0/O2 raw/passive native execution, exact actual helper input/result,
original field addresses, old/current/sibling values, every phase and final
Break result. Passive observations preserve guest values, stores and control.
The Copy-specific challenge matrix adds output-view no-clobber to the unchanged
P1184 ownership/destruction/foreign-B-survival suite. The test adapter renames
only the predecessor's alternative test entry points.

The new census reuses existing passive P-owned equipment on the new inputs. All
six positives and both direct controls retain additional30 contexts, peak32,
complete graphs7, cleanup live0/untracked0. New positive node counts are
80/77/77/86/101/101; both controls have 74/80. New allocation logs, not predecessor
numbers, identify 51 direct GCC PCs for each profile. Representative all-one and
all-two profiles reach respectively 1053/1167/2359 and 1159/1264/2577 allocation
ordinals at registration/actual/named stages: 9579 total locations. Full injection
checks OOM status/output-null, state/pointer/fact rollback, disposal, retry and
named bytes/inode/mtime/temp no-clobber. Context/graph retention is independent
of runtime iteration count in this family; work and package size depend on source.

P1184's fixed signature-origin freeze remains unchanged. The forward adapter
checks its original header against an archived exact faad blob and every original
source/oracle/probe digest, then requires the complete externally anchored current
history before running the same 120 rejection/no-clobber/foreign-survival trials.
It does not execute the original unsafe candidate or waive the old header pin.
Prospective v43–v45 externally anchor their complete predecessors and inherit
all coverage; original v1–v42 remain byte-identical. P1107 only adds legitimate
CMake successor freezes. Final local results and limitations are reported on #1188.

## Self-review / value and remaining authority

The direct existing source form remains the low-cost alternative. A bounded
checked-declaration relation is preferable here to duplicating a syntax provenance
walker, introducing a general IR, retaining another world, accepting same bits,
or minting an issuer. The existing pure body/call contract still validates the
complete callee. Canonical current/loan/owner and destruction remain separate
from observing a Copy value. Inspection found no new ownership or signature
allocation read before its existing origin authentication.

VALUE-ALIGNMENT-RISK: ordinary variable factoring becomes usable in the existing
v0 capability, while declaration/source/value/world checks, carrier mapping,
versioned evidence and regression adapters increase maintenance and review work.
The history/test equipment is larger than the Production increment and remains
an explicit cost, not a product success metric. Audit locality is UNMEASURED;
no general soundness or Product PASS follows from this finite capability.
GO624 PAUSED, H1 HOLD and P1036/PR1050 STOP continue.

Publication is conditional on actual PR1186 adoption at exact faad and main tree
01388ae. Until verified, this remains local with push/new PR/Actions0. Authorized
publication is one normal forward merge, one DRAFT/HOLD PR and at most twelve
natural jobs, manual0, stop at first reliable all-green. P Ready/merge is always0.
