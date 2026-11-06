# P1208 / C263 local mixed helper continuation

Fixed local basis: adopted main7a9be, CURRENT_SPEC Draft17.34. This branch does
not import the separate P1203 literal Break feature. Publication remains HOLD
until an explicit Coordination rebind to the adopted main; local history here
is a successor of that fixed basis, not the current live main's generation.

The finite private plan borrows the actual one-normal/one-RETURN IF, sole-normal
replay and final RETURN node. It allocates nothing and never converts the
returning arm into a normal edge. Normal values can flow through ordinary
dependency-free u8 bindings, Copy and discard to a normal tail or final lexical
RETURN. The emitter evaluates the condition once, emits the real early return,
and initializes a carrier only on the genuine normal predecessor. The caller
gets the existing conservative Unknown result and original-T current proof.

Support: one ordinary known-direct pure usize-to-u8 helper, one LT/EQ choice
with literal u8 arms, exactly one lexical RETURN arm and one normal arm,
bounded pure continuation, and the adopted guarded ADD original-T caller with
phase-result Break. Existing normal/mixed-tail/both-RETURN/final-RETURN controls
have byte-identical C17. Nonliteral arms, multiple/nested choices/helpers,
effects/loans/nonCopy continuations, both-RETURN continuations, general CFG,
new issuers/world families/API/ABI and literal Break are outside this increment.

source-freeze-v1/v2 and oracle-v1/v2 precede execution. The v2 addition is one
discard source; v1 files remain exact. The preserved baseline has 28 CLI
observations: five new positives refused at CHECK P14-LOOP-PRECISION, four
controls reach C17, five refusals remain. The separate prior P1200 report and
boundary95 raw/source/oracle are archived. The prospective boundary95 wrapper
requires real CHECK/owned/current/C17 success on the same P-owned source and
preserves its original refusal source, oracle, checker and raw byte-for-byte.

Focused ownership tests mutate RETURN target/state/value/type/ordinal, source,
owner, current, loan, signatures including a short foreign allocation, normal
replay and arm count. Rejected output must remain unchanged; restore/retry and
canonical A destruction/B survival are checked. Foreign display arms/states
remain unrestored at destruction in two trials. Invalid graphs are never run.

The native cohort is capped at 48 new guest executions: four core mixed forms,
discard, both-RETURN control, GCC/Clang x O0/O2 x raw/passive. Structural owned
and carrier/CFG gates precede every guest. Passive hooks read the same live
original and payloads and verify that early-return calls skip continuation and
discard. Exit callbacks inspect counters only, after automatic storage ends.

OOM uses fresh census for explicit-final and discard. select_oom.py freezes
symbol-qualified direct control/exit/anchor sites, every allocation in the
RETURN record-array/clone/anchor prefix, and the signature ledger. It does not
run every old compiler allocation. In this cohort, per configuration:
explicit-final 64 registration +191 actual +255 named, discard 58 +155 +213,
total 936. All-object sanitizer and GCC total1872. Exact OOM, rollback, unchanged
types/functions/facts, no publication, same-context retry, named bytes/inode/
mtime/temporary preservation and partial cleanup are required. oom.py normally
re-censuses its own binary; its explicit audit-recorded mode checks the already
completed same-binary trials without repeating them.

Fresh normal/explicit census: records2/3, retained RETURN authentication
103454/160442 bytes, graphs8, proof4688 bytes, validator allocations0; peak
contexts37/39. The third record reuses an existing clone/anchor slot within the
unchanged maximum4. Final tracked bytes/blocks/contexts and unknown destroys
are zero. These are finite observations, not general memory bounds.

Build local frontend targets with NEWLANG_FRONTEND_ONLY=ON. Focused registered
tests are p1208_*; native has24 guests per configured compiler. Do not rerun it
after consuming the48-guest local budget. Source/recorded raw restoration and
the actual execution counters are retained in the separate P1208 local packet.

VALUE-ALIGNMENT-RISK: a shared private control contract and one extra existing
RETURN snapshot on explicit-final paths add trusted review/evidence upkeep.
Tail-only source refactoring remains an alternative. Capability/native success
does not establish general soundness or smaller human UB audit dependency
closure. Audit locality UNMEASURED; GO624 PAUSED; H1 HOLD; P1036/PR1050 STOP.
