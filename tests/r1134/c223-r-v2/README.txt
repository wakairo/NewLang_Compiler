C223 Track R V2 - SCOPE-CALIBRATED / implementation-fixture-blind

Authority: main 28726f933ff4885c000e0247cf4fd9ac1859c5b1; Draft17.34.
Fixed candidate: 0caf94894560d2f9a0b1f9404ca87055b0a21ee7.
This is an independent prediction freeze, not an execution result.

V1 is copied byte-for-byte, including its oracle and historical BLIND status.
V2 is a distinct cohort calibrated only to C223's minimum public profile.
C223's initial entry notation was corrected by C before this freeze:
canonical 4.9 and 18.1 require fn main() -> unit, not tuple notation.
No producer implementation, fixture, oracle, PR1137 body, P1133 body/comments,
P report, or F1138 result was read. No tooling reference was read before freeze.

Primary sources: six finite cases, ascending and descending, each with 0, 1,
and 3 real Continue/backedges. Rising true-arm guard gives phase <= 8 and
phase+3 <= 11. Falling false-arm guard gives phase >= 5, so phase-4 is
nonnegative. The precise concrete traces are in oracle.json. All usize values
fit the ordinary finite range. Break does not depend on phase. Continue leaves
two live two-u8 originals and all their fields unchanged. Break updates one
fixed field through an ordinary ref helper returning the old Copy. A later
caller performs a second independent write to the same original field.

Mapping from V1: p01/p02/p03 preserve the rising 3/0/1-backedge questions;
p04 preserves descending three-backedge and declaration-index-1 change.
p05/p06 add descending zero/one controls. Values, names, steps and bounds are
independent choices. V2 uses the published less-than guard, direct arithmetic
Continue argument, phase-independent B, and unit CLI entry. These are explicit
structural changes, not silent repairs to V1. No V1 acceptance requirement is
inferred. The reversed field initializer order in descending inputs tests
field-name identity separately from source initializer order.

Negative predictions: n07 violates subtraction at a later header, n08 adds on
the false arm with no upper bound, n09 subtracts in a true upper-bound arm and
later underflows, n10 has wrong operand type, n11 wrong carry type, n12 wrong
nearest-loop arity, and n13 escapes an ending loan dependency. They must be
rejected in checked use, and none may reach native execution. c14 is a legal
commuted-addition precision control outside the published finite profile;
conservative rejection is allowed and native execution is deliberately absent.

Canonical basis: 4.2/4.3 structural Copy/Discardable; 4.9 and 18.1 exact unit
source type; 5.2 quantity representability at every reachable header; 13.5a
current-value dependency and scope-exit compatibility; 13.8 exact-once loan
result forwarding; 16.2/17.1 named two-u8 fields; 17.4 lifetime-preserving
replace returning the old value; 18.2-18.8 ordinary explicit ref arguments,
Copy results and caller-visible state; 19.1 lexical/control boundary;
27.4-27.8 arm selection, cyclic header, exact carry/target, Break exit state.

Execution mode after this freeze: dedicated GitHub branch and Actions only.
No local shell, compiler, native, Lean, or P-equivalent environment. Initial
ref must point directly at a completed R commit whose parent is fixed P.
Natural FAST max 6; natural FULL5+PARTIAL max 2 pairs (12 actual jobs);
manual rerun/dispatch 0. Tooling-only repair remains within C223/Process7.6.
FIRST/raw and version identities must be preserved. Stop at first trustworthy
all-green outcome. Mechanical observer failures do not redefine the oracle.

Source legality, implementation profile acceptance, source-issued public owned,
C17/native, general soundness and Product value remain different judgments.
Finite capability does not measure library author plus ordinary safe caller
UB audit/re-audit locality. Added retention/harness burden is a separate
VALUE-ALIGNMENT-RISK. Audit locality UNMEASURED; GO624 PAUSED; H1 HOLD.
