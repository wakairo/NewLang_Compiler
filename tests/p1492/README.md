# Source-owned guarded usize quantity

Base: main f7c277af59a4c4281c07fd6d3cc1d98f8bf88f48, Draft17.38.
Authorization and preserved FIRST: Compiler Issue #1492, C1103.

One readonly usize formal is compared directly with a positive typed literal
L. In the true arm, that same unchanged formal is added to typed literal k,
where k <= SIZE_MAX-(L-1). A normal local Copy binding returns that sum; the
false arm returns the formal without evaluating the addition. The caller main
binds the actual usize call result, makes ordinary Copies and returns it.
Bounds and steps are source values, with no special capacity or function name.

CHECK uses a private mode of the existing arithmetic issuer, canonical arm
custody and the existing whole-root result/world seal. Guarded sums remain
Unknown even during concrete replay: the checker visits both hypothetical
arms, including an unreachable true arm for an actual MAX argument. No cached
zero or bound is a numerical answer. The public Known addition receipt API and
its meanings are unchanged. No memory authority is derived from this quantity.

The C17 consumer admits that authenticated finite plan only. It emits a real
size_t formal, one runtime condition and only the selected arm's addition,
then the actual normal return/call/binding/Copy chain. Process main still
returns int zero; the source main's usize value is an internal normal result,
not a new process exit-status or foreign export ABI.

`oracle.json` and complete sources were frozen before observations. Q is
byte-identical to P1489's quantity source. B changes L/k; M checks MAX/k1 and a
false-arm actual MAX. Negatives reject an unrelated guard and MAX/k2. The
owned probe checks actual public MACHINE, unknown-result evidence, unchanged
Known receipt semantics, current-source/call/world tampering, foreign-child
destruction, reached OOM rollback/retry and named-output no-clobber. The native
runner compiles unchanged CLI output at O0/O2, then includes that same output in
a passive observer. Inputs come from argv; expected answers are test data only.
Boundary fixtures require the native 64-bit usize target used in the CI matrix.

Dynamic bounds, arithmetic on a changed formal, nested result composition,
multiple calls, explicit RETURN, H/T state composition and general range/CFG
analysis are outside the original P1492 C17 consumer. P1499 adds sequential
result-to-next-call composition under the same existing issuer; see
[its supported boundary](../p1499/README.md). Other such legal sources may remain safely
unsupported. Earlier P/A/Q/N source/oracle/FIRST/RED are not rewritten.

GCC Release, Clang ASan/LSan and Clang UBSan are the local host configurations.
Original raw bytes, observed values, forward failures, selected injection
receipts and CI status belong to the Issue's evidence report. Git is the source
history authority; this adds no custom freeze-generation protocol.

This capability is not general soundness or a Product PASS. Product GO stays
PAUSED; H1 HOLD and audit-locality comparison with factored C remain separate.
