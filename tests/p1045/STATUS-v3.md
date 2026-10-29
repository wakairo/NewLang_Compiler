# C137 forward history and bounded test composition

Production and the P1045 native/negative/OOM program remain byte-identical to
tested b63d228. All four-source GCC13/Clang/ASan+LSan/UBSan targeted gates passed.
GCC13 full was576/577 exit8, preserved as a real regression RED; Clang full
was not launched after that failure. The failed wrapper reached old P1030's
history scratch case after its original current/native/negative/OOM checks.

The old scratch case rewrites v7 and CURRENT's immediate predecessor digest.
That is coherent when CURRENT is v8, but v9's immediate predecessor is v8,
so the malformed attack now refuses at previous-freeze-digest before reaching
the intended original-v7 external pin. The original P1030 script/runner,
oracle, source and first RED remain unchanged. New P-owned forward copies
retain all five old rejection oracles and every original P1030 check while
propagating a simulated v7 rewrite through all intervening mutable manifests
before testing the unchanged trusted original-v7 validator. This strengthens
the adversarial fixture rather than relaxing rejection or masking a failure.

The new P1045 suite took53.434s under ASan+LSan locally; the predecessor's
existing umbrella already took57.54s in its actual ASan CI. Serializing both
under one unchanged90s CTest would needlessly couple budgets. One ordinary
P-owned CTest registration now runs the exact same P1045 suite independently.
All old577 registered names and assertions remain, giving578 frontend tests
and579 with the LLVM smoke. No timeout/environment/policy/workflow/T/R hook
changes or extra CI jobs are introduced. The P990 umbrella merely selects
the new P-owned forward P1030 runner, and the new P1045 test gets its own90s.

Required verification retains the already green identical Production results,
checks affected registered tests, completes the previously unrun Clang cohort
and tests the actual ASan CTest budget. All old failure records remain distinct.
No all-green original576/577 claim, R original acceptance, new F approval,
Product GO, READY, merge or independent track launch is implied.
