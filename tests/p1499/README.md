# Sequential source-owned usize result composition (C1122 / P1499)

Authority: main 7ea4dc1a2d72e4274433572b7bff3c1dacfd7434,
CURRENT_SPEC → Draft17.38 §§5.2,18.1–2,18.5,18.7–8,19.1.
Authorization, predictions, FIRST and forward receipts: Compiler Issue #1499.

A guarded occupancy result feeds a guarded offset/end consumer through an actual
caller Copy. B calls the same declaration twice, with fresh formal/body/value
identities. Both complete functions use the adopted unchanged-formal true-edge
quantity proof (x<L, x+k, representable under the literal bound); both return the
input on the false edge. All guarded results stay Unknown to CHECK. Each actual
call has its own owned body and existing result seal; the outer seal authenticates
the final caller world and ordered Copy chain. No new public issuer, range lattice,
memory authority, source law, ABI, or semantic_check/h_origin change is introduced.

The zero-parameter caller starts with one typed usize literal; each later binding
is either a Copy of its immediate predecessor or a guarded helper call with that
Copy as its actual. The final predecessor is returned normally. The existing
64-body resource bound remains. Multiple declarations and repeated invocations
are supported; the direct C17 emitter borrows the authenticated plans, defines
each helper once, and emits the real caller operands/results in source order.
Other value-flow graphs, nested call expressions, dynamic bounds, explicit RETURN,
H/T composition, general ranges/CFG and arbitrary scalar function bodies remain
unsupported by this consumer. Direct-H teardown is a distinct future dependency,
not repaired by this patch. No new safe library facade is claimed.

The owned probe verifies two distinct actual/formal/result flows, Unknown values,
unchanged public Known arithmetic views, eleven second-call/current/world/source/
arm/operand/result mutations and foreign-child destruction with zero C bytes.
It reuses P1492's selected seven OOM sites plus named-output failure: genuine hits,
NULL result, unchanged context, valid rollback and same-context retry. Overflow and
wrong-formal sources reject at arithmetic precision. The canonical scoped usize
write-loan negative is rejected earlier at LOCAL-LOAN-PROFILE (unsupported);
it does not prove changed-formal arithmetic precision. Its first invalid
non-scoped syntax is preserved in history, never relabeled semantic evidence.

The native runner compiles unmodified CLI C with strict C17 at O0/O2, runs it,
then includes those exact bytes in a passive observer. Runtime argv is passed to
the unchanged first helper and its computed result to the unchanged second helper;
the source zero-argument caller is also observed (A=16, B=13). This is not a new
runtime entry ABI. Boundary oracle values require native 64-bit usize. Expected
answers exist only in the tests; production emission never uses them. The caller
process main still returns zero. Existing P1492 inputs/oracle/FIRST remain intact.

FIRST used the inherited compiler in the clean pinned checkout (not a separately
rebuilt/attested baseline binary): A/B CHECK0 and emitter4, no C bytes/native.
Static pinned-base source comparison independently explains its single-call
recognition limit. Forward GCC and Clang ASan/LSan/UBSan results and original
harness failures are reported separately in the Issue and Release evidence.
This is bounded capability evidence, not general soundness, Product GO or measured
UB audit/re-audit locality improvement against well-factored C. Product GO remains
PAUSED and H1 HOLD. Independent review and substantive C disposition are pending.
