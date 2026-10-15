# P489 pre-execution freeze — None path coverage

Track: P. Exact main d6e603e12591c2ff39e3e1b77c784681fec66033,
CURRENT_SPEC -> Draft17.30. No PR479 inheritance or parallel-task evidence.
Exactly two candidate complete sources and expected observations in frozen.json,
both preselected BEFORE any compiler execution. Each is byte-for-byte adopted
P471 A or S, except first head.next replace Some(tail_ptr) -> None. Original
stored authentic tail_ptr, dormant Some(q) body, both normal arms, tail/head
locals and payload2/1, observed read and final head reset remain unchanged.
No candidate run yet. No third source, oracle relaxation or success-driven rewrite.

Predicted semantic legality: actual runtime input None; A leaves original tail
untouched, S replaces tail.next None->None and yields old None. Both head stores
also yield old None, head final None, original tail/head distinct and payloads
stable. Neither Copy pointer nor observer creates safe-ref/Domain/owner authority.
Static analysis may rightly reject imprecise dormant Some q/currentness or
withhold a two-arm certificate for known None: classify exact source/check/proof
barrier, not unsafe semantic acceptance or fake native success.

Compass -> Product Strategy §1.1 audit dependency closure; Process4.1.1/4.2/4.3.1;
canonical fixed fields/loan/Copy/Change/Reset/Option/normal joins (§§10,13,17.4,26,27.3).
KEEP existing semantics, proof issuer and validator. Historical P471 native Some
path is a control, not evidence of executing its None arm. P466 None workload
has a different mutation target and cannot fill this hole. §4.2 N/A test-only,
no source/API/meaning selection. Exploration reliability only, Product HOLD.

Run fresh exact-main LLVM-free --check on both independently. If accepted,
inspect real source-generated owned entry/arm/post evidence with public validator
before C17/native. No fictitious dormant Some certificate or host semantic seeds.
A first source/check/proof/backend/runtime stop ends each case; safety mismatch
stops whole task. No unmodified C17/native process exit claimed as physical proof.
Only if complete public proof exists: unchanged emitted C and passive physical
observer, GCC/Clang strict C17 O0/O2, correct None selection and original pointers/
field/payload state; no guest writes or C patches. Freeze source SHA again after.

Only new tests/p489/** and optional one minimal CMake test for real supported
positive; no compiler/include/adopted tests/docs/toolchain/workflow/PR edits.
At most one natural FULL+PARTIAL if supported positive warrants DRAFT PR; no
manual reruns. If CHECK/proof stops, preserve evidence branch without PR/CI.
One final Issue489 report then STOP task. No merge, close or other Track launch.
H1/STB/Product HOLD unchanged; C owns investment/adoption.
