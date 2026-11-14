# P1412: actual-call authentication for same-original ptr IF

Main is `26e21ddca3f5473e14fa954754d4aecfa706a065` / Draft17.38.
The preserved P1408 S/N sources, original oracle and first P13/C17 RED remain
unchanged. This test reuses their real registered program and actual `main()`.

The CHECK stage requires the existing actual-call result validator to accept
the authentic same-original IF subtree. The same receipt authenticates its
issued nodes, lineage and semantic storage, including the commit-pending root
candidate rather than the old public context. It creates no owner or Domain.

`seal.c` tests source/AST teardown, both ptr arms, wrong/unissued calls,
node/lineage/semantic facts and storage-base faults, and genuine A/B foreign
arm substitution followed by A's unrestored destruction and B's survival.
Selected real allocation failures are injected only inside the existing issuer
and must leave caller state/out unchanged with a healthy retry. They are run
explicitly under the Issue's finite cumulative OOM budget, not an open sweep.

Stage1 is not C17/native success. The conditional physical consumer is a
separate subsequent gate and commit. No unchecked grammar is implemented.

The prospective C17 consumer uses the actual checked condition and each arm's
parent-qualified runtime variable. Effect-free ref-to-ptr and initial None
observations retain all common original/Domain/range/scope/permission checks.
They do not execute the older committed-link mutation protocol. Its old paths
remain unchanged and selected old native C is compared byte for byte.

P1408's original `owned.c` keeps its historical C17-unsupported assertion.
Its CHECK/owned coverage continues in `p1412_result_seal.unit`, and the current
backend expectation is checked in `p1412_C17_false/true.unit`. This prospective
registration replaces that obsolete coverage assertion; original S/N, their
oracles, old source, C1 commit and RED evidence are unchanged. The semantic
negative and original ptr-copy OOM registrations remain active.

`native.py PACKET` performs exactly eight process runs (one raw, seven passive),
without modifying generated C: GCC/Clang O0/O2, true/false IF, H0/H1/H2 cleanup.
Clang observations also use ASan/UBSan/LSan. It refuses to repeat a saved ledger.
