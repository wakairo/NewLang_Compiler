# R518 — pre-execution second-pass freeze (Track R)

## Authority and isolation
- Fixed main: `9e6389388a44950d17e19bb134cbfde37ac75f97`, CURRENT_SPEC -> Draft 17.30.
- Candidate **exactly** `431ff6413f6e24e4de307c25f140ca8161d95bbc`, never moving PR ref; PR #515 remains DRAFT.
- Informational FormalProof/main `5437f92d099e228773d31e2323e8ee6228ca21bf`; no new formal proof.
- Reuse immutable **original** R516 sources at freeze commit `19e9ed06a57ee822a569b777c1e2d9a8d51c21a8`, found in tests/r516/ on inherited R branch. Do NOT modify R516 source, oracles, logs or first-pass finding. Initial R516 BLIND FINDING (coverage gap; native inconclusive) survives untouched.
- `true.nl` SHA256: `c921596f72e64116630676b98d6ce43c529420a4bec26c437d01981cc39cc864`.
- `false.nl` SHA256: `0dfc413cfc63fa379b6b4c1d20a6beea1421188add4e05b9d1bc660fc0ca1664`.
- `reject_arm.nl` SHA256: `d9f6c09fdb63ba50e5beaeb26b6ada585cfe67c28883a53c53c7504ce6c0c272`.

## Predicted outcomes; independent oracle frozen before corrected compiler execution
- TRUE: two distinct source operand bindings 29 and 29; original Pair initially f0=37/f1=211; selected=108; old=37; same ORIGINAL Pair f0=108 after exactly one scoped write; f1=211.
- FALSE: source bindings 29 and 30; select=173; old=37; same original Pair f0=173; f1=211.
- In each world original is created before condition, and remains physically one live original root until function return, with field addresses &original.f0==&original, &original.f1==&original+1 (byte layout, only for emitted 2 x uint8_t C struct).
- `reject_arm`: u8(256) in never-selected ELSE still SEMANTIC-CHECK rejects with exit=3/range code; no backend-pass interpretation.
- CHECK0 alone does not establish all the public owned facts; report precise API-visible facts and unavailable layers separately.
- Strict unchanged emitted stdout C17: GCC and Clang, O0/O2, -std=c17 -Wall -Wextra -Wpedantic -Werror; native exit0 is necessary but insufficient.
- Passive GDB **inside actual nl_fn_X before its return**: break at original field write, inspect root address/f0/f1, referenced pointer, advance one C statement, observe same address and f0 selected; break again on the end of that same static source function, where old, post-read left and right scalar carriers must be live, not wrapper `int main()`. Require the same original addresses and 37 -> 108/173, sibling 211; never inspect expired stack. If symbols/live address unavailable => NATIVE IDENTITY INCONCLUSIVE, STOP.
- Static source-to-owned and backend review focuses on exact source binding, child/projection/incarnation/fact/currentness, distinct pure arm frames, direct source equality, scoped ref, one left Change, right stability, no allocator/domain authority amplification. No claim for Array/heap/general IF/owners.
- Passive observer is read-only: guest local oracles never written, no injected Pair. Corrupted-C checks may **only mutate a COPY** of baseline guest C to confirm observer rejects wrong values/field or selected arm. They never contribute to a passing guest claim; baseline C SHA is rechecked.
- Stop at first unexpected CHECK/owned/emission/native/identity discrepancy, unavailable live original, evidence ambiguity, budget exhaustion. One natural CI plus <=1 extra CI for demonstrable R-owned repair, <=2 narrowly scoped R-only repair commits; preserve initial failure and offline preflight; no tuning source or semantic oracle.
- Product Strategy §1.1: C topology retained and compiler-checkable bookkeeping moved to compiler, not zero unchecked. Author+ordinary-safe caller UB-audit dependency-closure reduction remains UNMEASURED. Alternative: factor pure choice then one write in C, Zig, same-topology Rust. Report VALUE-ALIGNMENT-RISK separately, never Product GO.

## Evidence format
Preserve candidate SHA, binary SHA256, source hashes, unchanged C SHA256, strict compile/native outputs, GDB text+parsed values+addresses, generated-C structure review, malformed-C controls and their negative observations, public checked-owned facts and limits, all CI run IDs. A prior P success is corroboration only, never the independent R oracle.
