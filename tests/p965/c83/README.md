# Track P / C83 continuation — STOP at existing P817 regression

Resume checkpoint: `b8729f46c61d891a76db7ecbd2484ffdeb6a65cc` in the original
`/workspace/p965` worktree. Original pre-run P965 candidate
`cf0b6dc66e50ba2d59c80e43871a8fa47e78aa10`, original freeze and first P633 RED
logs remain byte-for-byte unchanged. PR962/954/946 were checked at their pinned
HEADs and OPEN/DRAFT. No environment reconstruction or branch replacement.

Tested C83 candidate: `4ca365c9107e875dac20051fdbf1afb3f104e3ff`, tree
`598ddc0a9e7ebc13e68bcee3d01dc8de56a2a794`. Implementation changes relative to
the stopped checkpoint are confined to `src/checked.c` and
`src/h_origin_internal.c/.h`. Intermediate seam candidate `c852db9` and its
successful first GCC P633 log are also preserved; no RED was rewritten.

## Seam and ordered completed tests

Borrowed getter validation permits only a currently installed non-owning ARM
observation with the same original source/world/lineage and child tables in a
zero-H-site root. A true member of either CHECK owner cannot qualify as a new
shallow observation. An unrelated shallow parent cannot use the root's counts.
Body/continuation entries, original counts and buffer bases stay strict. Nodes
and world facts still pass through the existing scalar semantic verifier.

`nl_h_custody_validate` retains exact original membership for C17 entry/source
authority, including zero-site node entries. H-site entries cannot use the
observation allowance, and `nl_h_origin_validate/edge` use strict custody.
CHECK publishers, canonical inventory/teardown, H issuance and original scalar
checker/Copy validators were not modified. No owner/shadow registration, new
source right, public/header change, or test-specific source/value whitelist.

1. Original unmodified `p633_local_same_value.unit` FIRST passed GCC14.2,
   Clang23.1.3, Clang ASan+LSan and UBSan. Each logged35 original controls
   rejecting, wrong-context/paired-context=false, healthy shallow replay=true,
   and protected worlds unchanged.
2. Frozen P96526 gates plus P953/P961 H-origin7 gates passed33/33 under ASan+LSan
   and33/33 under UBSan. Each configuration reached the same502 P965 selected
   source/mode/index OOM observations as C82, with rollback/healthy retry and494
   ownership trials, including genuine B arm into A, unrestored A teardown/B
   lives, count/base changes and continuation. Old P96125 OOM positions passed.
3. The complete GCC frontend build succeeded. Serial full frontend CTest then
   stopped at the first new failure:263 PASS,1 FAIL,251 NOT REACHED of515.
   Existing P633 different-values/CLI, P634 historical hash and P605 arithmetic
   integration gates passed before STOP.

## First new RED: preserve query/reject distinction

```
264: p817_arm_integrity.unit
/workspace/p965/tests/p817/integrity.c:62: rejected(a)
CTest exit8
```

The unmodified input is `tests/p813/sources/charge_false.nl` (SHA256
`5ad114e6f978c2b3cd4923a1bccff5c82ce7c51413b1ef01070d9697645acc92`). Harness
`tests/p817/integrity.c` SHA256 is
`fe5516fbb289eca2b4878da8d611677c7525334aa35a6a4061f38d1129b7557e`.
The failing mutation substitutes `a.caller->context = f.caller->context`.

`rejected(a)` L2–10 requires both proof-view/parameter retrieval calls to succeed
AND both validators to reject. Its assertion failing is not itself evidence that
a validator accepted a wrong-context proof, emitted C, or caused memory damage.
Static code inspection finds: the canonical-world comparison in
`h_origin_internal.c:213` rejects the caller's changed world; borrowed getter
preflight returns NULL from `checked.c::nl_checked_call_body`; unchanged
`copy_body.c:418` proof lookup returns NULL; proof-view retrieval L425–430 returns
false. That early rejection also makes the composite test predicate false.
No runtime instrumentation/rerun was performed to measure individual statuses.

This is an existing proof-observation availability contract regression. It is
still a RED local gate and was not waived. The candidate restored P633 precision
but did not satisfy all existing getter consumers. P has not changed P817,
P785 helpers, Copy proof issuers/validators, source expectations, or getter policy
again after failure. Coordination must adjudicate the next minimal boundary
between proof observation under a challenged world and owner/emission admission.

## STOP and preservation

Old P633 first RED and new P817 first RED both remain. The complete new logs,
patch, fixed tested HEAD/tree and hashes are under `evidence/`. Original sources,
tests/oracles/SHA/freezes, `main.c`, `semantic_internal.h`, public headers and H
source/custody issuance/destruction are unchanged. Original P953 FAIL/P961 RED
and F76 STATIC Gate B are not upgraded. R963 source/oracle inputs were not read.

No additional repair or tests after P817 failed. The required original
Echo/Window strict GCC/Clang O0/O2 native0/1/2 heap observer under the C83 candidate,
extra guest sanitizer/native runs, final full formatter gate and remaining251
frontend cases are NOT RUN. Earlier existing-family native passes are not their
substitutes. Push0/PR0/Actions0/FULL5=0/PARTIAL=0; no READY/merge or R/F launch.

**VALUE-ALIGNMENT-RISK:** separating owned identity from borrowed observation
restores one old precision contract but eager getter validation also changes
proof retrieval under a deliberately wrong world. The extra maintainer contract
and audit dependencies remain unresolved. Comparative library-author plus
ordinary-safe-caller UB audit/re-audit improvement versus well-factored C is
UNMEASURED. KEEP Draft17.34 / REPLACE NONE; bounded technical work only. Product
GO PAUSED/H1 HOLD. STOP / return to Coordination, no automatic continuation.
