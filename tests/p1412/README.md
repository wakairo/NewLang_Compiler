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
