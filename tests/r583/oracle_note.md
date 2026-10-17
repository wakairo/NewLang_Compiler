# R583 calibrated oracle — frozen before run (NON-BLIND)

Original first-pass BLIND **INCONCLUSIVE** is not changed: R580 commit
38e1dbd2874c12983635ae4e7ff320f94566b613,
source SHA256 a748ba8cd9f2819d8a473543bd79158c590bb42aefc89ee2bbefdfbf2146386f,
tests/r580/frozen_oracle.json, inspector public_probe.c:50 and runs
39042824603 / 39042910004. This file is a **new** calibrated test, not
a replacement or reinterpretation of its positive oracle.

Semantics derived from canonical Draft 17.32 §§10–13,17,26 and candidate
src/semantic_check.c local loan builder:
- From a direct lexical local fixed field, from_ptr = false.
- Completed scoped body sets body_nonescape_proved = true.
- It sets implicit_local = true and prevent_lifetime_end = true.
- Ordinary ref<write,T> is *non-exclusive* (§11.2), so the
  prevent_conflicting_access field is zero for this local loan; no
  alias exclusivity or lifetime-ending right may be inferred from write.
  Therefore R580 compound assertion's conflicting_access conjunct was
  an overconstraint; it is NOT a compiler unsoundness finding.

Profile discrepancy to test without rewriting original R580:
- R580 source creates lexical Node roots c,b,a and mutates a.next, b.next.
- Candidate node_links replay admits roots in source order [r0,r1,r2],
  with first mutation r1.next and second mutation r2.next
  (src/node_links.c replay p.parent == roots[index+1]).
- New, separate tests/r583/calibrated.nl creates c,a,b and mutates
  a.next := Some(b), b.next := Some(c), both original None.
  Three original lexical carriers and two sequential Changes are retained.
- The original source is observed independently with NO retroactive
  reclassification of its R580 oracle; its candidate validator status
  is reported, not forced to PASS. This new source is only calibration
  of the candidate's explicitly smaller finite issuance/profile.

Pre-run expectations for new calibrated source:
1. actual registered fn main() from source; SOURCE/CHECK=0, no messages;
2. two ordinary scoped write loans: !from_ptr, nonescape, implicit_local,
   and no claim of exclusive access;
3. public node_links_view and validator OK with 3 distinct source-owned
   lexical roots [c,a,b], exact parent/field/current-fact transitions,
   correct original inc, two old None Copy carriers, Some(b)/Some(c)
   pointer targets, and complete first-after/second-before bridge;
4. copied-view root/op/step/foreign edits and actual-owned snapshot
   fact/permission/occurrence mutations rejected, with later original
   validator still valid;
5. only if public validator passes: emitted strict C17 and
   gcc/clang -O0/-O2 plain+passive original-memory observers;
6. semantic negatives, unsupported profile + no C output. STOP
   on first genuine failure. No claim of general heap owner, safe API,
   general O(1) detach, coverage beyond executed checks or Product GO.

No host-created original semantic root or forged positive authority.
Host mutations are *negative tests only*. Preserve all stdout/logs.
