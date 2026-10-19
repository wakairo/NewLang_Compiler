# R638 fresh BLIND oracle freeze (2026-10-19 JST)
Target fixed main: `032cf60aabf4997e672bc3607cf53775d91ececb`.
Target candidate parent: `5e37869d1c4847b6fddbd205a369462be495b697`.
Canonical: Draft17.32. Independent source/oracle written before first candidate compiler run.
This file is immutable across test repairs. Never rewrite expected output or source to fit candidate.

| Source | SHA256 (exact UTF-8 LF bytes) | Source CHECK | C17 expectation | Run-time oracle |
|---|---|---|---|---|
| lit_true.nl | 116a6bb20e23de4369bffe110b636a93606556dbfaf9244a7f0bd9170e21878d | OK | finite intended YES | condition true, exactly one arm, chosen Copy 29; not 203 |
| lit_false.nl | c4daae65a267e6f5bf88dc4da5cf90e4304204fd75c49c3ce23c8b7d2b785e8d | OK | finite intended YES | condition false, exactly one arm, chosen Copy 211; not 17 |
| copy_equal.nl | c9be52298fd1f2628b1f42ebeffb199280177cf996bb670ba65afe43e673b69e | OK | finite intended YES | true, chosen 74; separate arm private Copy issuers despite same 74 |
| field_true.nl | d395e9fb925e27e95c6284038d365ce992599cf8fa37893e0fd9ec6c41496a70 | OK (if selected root/field syntax supported) | MAY BE finite-backend UNSUPPORTED (precision only) | original slab remains rooted, field first initial 31→62, sibling 200 unchanged, old readback 31, same physical root |
| wrong_type.nl | 5e886e57de91fcd034df765e552677056399dea208b1c8357e2fb9575cc7d6b8 | SEMANTIC_ERROR | no bytes | u8 vs usize result mismatch |
| out_of_range.nl | 767fc0a16d9112f36ce0d224f1e8184e1ae2703c48cceaef26456c2b77184cf6 | semantic or typed-literal range rejection | no bytes | u8(256) disallowed |
| nested_legal.nl | afd7c03b2d3a51709a5889267f9de89fc0f322817995bc87f1147bae4dfbac2e | OK | finite backend UNSUPPORTED acceptable | true outer/inner selects 7 if emitted; no forged authority |
| second_local.nl | e9bd4fb7788541571bb76efcb0aa3deed6da3d5cadecb741fc47a869ddf7b662 | OK | finite backend UNSUPPORTED acceptable | true selects Copy 13 if emitted |

Host and checked evidence oracle: genuine registered main() body, distinct owned IF-arm contexts; no arm-private ID may be reinterpreted in parent context. Corrupt test-owned child semantic/world/parent/arm index (including same-valued arms) and invalid zero/out-of-range scalar, place/field IDs must fail closed; original genuine artifact remains unchanged and validates. Unsupported validator hooks must be reported NOT TESTED; do not cast-away-const on genuine issuer-owned memory.
Native oracle: strict C17 -std=c17 -pedantic-errors -Wall -Wextra -Werror, GCC and Clang O0/O2, guest value read by observer only (never staged/minted). No ASan/UBSan complaint, no unexpected runtime arm.
Classification: language-legal but outside selected backend = UNSUPPORTED/precision; substantive semantic/authority/native discrepancy = STOP. No source/oracle repairs.
