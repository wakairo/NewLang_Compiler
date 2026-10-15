# P488 final — PUBLIC PROOF GAP

Track: P. Frozen compiler PR479 03b1751b513e7aef2d7ba193eda671a556f54b47;
main d6e603e12591c2ff39e3e1b77c784681fec66033; canonical Draft17.30.
Pre-run freeze 5978805cb8134e4b1a7dc0a7e8161199248a5671.
Candidate SHA256 06c96f6df6688167f2f9d8e2768bfc5bc2ea9f93e09838502400039d2de5f8aa.
Frozen source/oracle unchanged. Exactly one positive; no substitute or compiler edit.

## Actual ladder

- Lexer/parser/registration/CHECK: CLI --check exit0, stdout0/stderr0, with a
  fresh Debug NEWLANG_FRONTEND_ONLY=ON build. Binary SHA in evidence/check.json.
- Source-generated owned: public parser -> registered whole actual source ->
  check actual main() call -> owned main body -> owned set_link body. Public-only
  probe exits0; stdout retains qualified graph observations and assertions.
- Main body node35 is the real registered helper call (actual arg ref symbol8);
  child body root1, replace2, formal operands symbols9/10. Child nodes use their
  own artifact-local IDs/source spans, NOT main-local operation IDs.
- Actual write loan3 is original tail.next place2/inc2, original tail place1/inc1,
  implicit local Domain (domain0), not a minted explicit Domain/Allocation.
  Authentic ptr_from_ref points to same original1/inc1. Original head is6/inc6.
- Child replace: field F2->20, parent RF1->21, occurrence0->4, old value1 None,
  new value22 Some(original tail1/inc1); real call returns old value1. Public
  assertions check actual write-ref scope/place/incarnation and one child replace,
  old Copy return, exact incoming original ptr, nonescape/result forwarding.
  Head setup F7->10/RF6->11/occ0->1, reset F10->25/RF11->26/occ1->0.
- **CERT: PUBLIC PROOF NOT ESTABLISHED.** Child accessor and point-in-time field
  records are observations, not an independently validating ordinary-call
  argument/formal/entry/exit/effect certificate. Public owner/producer/custody
  validators are specialized other profiles; their entry flags are false here.
  No ordinary call proof validator in include/newlang. Parent final context is
  after lexical locals end, not a borrowed source-point entry/post snapshot.
  No assertion of independently validated caller sibling/current-state closure.
- C17 and NATIVE **NOT ATTEMPTED** after the pre-registered independent-proof stop.
  Read-only src/checked_c_node.c loan requires a direct REPLACE tail; matching
  ref/Option known-call lowering is a potential next gap, NOT a measured CLI4.

## Distinct references and safety

P471 inline control is existing source/public join/C17/native PASS, distinct from
this nonmatch source. Exact original P475/P478 named match-arm source remains
CLI3 P9-CONTINUATION-PRECISION on PR479, as previously measured; not rerun or
renamed to claim a PASS. P478 ptr_as_ref frozen negative previously CLI3 semantic
P3-TYPE-MISMATCH; referenced only, no fresh independent safety result. R480's
call-IN-arm join-risk profile is NOT exercised by this straight-line probe.
No proof bypass or claim that CHECK0 proves a safe ABI/general library.

Prediction was CHECK0 followed by ordinary-call public-proof gap; that is the
actual frontier. Removing the two-arm join does not remove the need for
source-issued ordinary-call argument/body/return/effect closure evidence.
A future finite P9 certificate must handle nested child source identity and
actual/formal/old-result relation before join composition; this experiment does
not implement or price a general effect system. Cost implication: do not budget
P9 as a guard relaxation or signature-only/emitter-only fix. A finite transparent
body certificate may suffice, but design/revalidation/ABI costs remain separate.
No quantified product-author/caller audit benefit measured. H1/STB/HOLD unchanged.

## Reproduce (from this evidence branch)

```
cmake -S . -B /tmp/p488-build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/p488-build --target newlangc -j4
/tmp/p488-build/newlangc --check tests/p488/nonmatch_link.nl
gcc -std=c17 -Wall -Wextra -Wpedantic -Werror -Iinclude tests/p488/public_probe.c \
  -o /tmp/p488-probe -Wl,--start-group /tmp/p488-build/libnewlang_*.a -Wl,--end-group
/tmp/p488-probe tests/p488/nonmatch_link.nl
```

Changes only new tests/p488/**. No Production/include/CMake/adopted tests/docs/
workflow/toolchain/PR479 changes. Evidence branch only, no PR/CI warranted for
this exact proof stop; no merge/Issue close/other Track. One terminal report to
Issue488, then STOP this task. Track C owns future investment.
