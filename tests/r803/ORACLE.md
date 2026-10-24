# R #803 First-pass independent BLIND freeze

Authority: former main 77d83dfc00850b14fd7c3bc95e7e3c12e34161fb; CURRENT_SPEC Draft17.34 sections 17.32.1-4. Immutable P candidate 50ce2b050a5273041f0f86591c3cc40534db350b, tree 6195e4897c89a130f28a5b1397e89439928dbaa5. R #809 materials are not reused. No P #791/#804 source, test, oracle, implementation or report inspected before this freeze.

Positive one.nl: lexical orchard_cells; one original live Array<Option<u8>,4> root, exactly four dependent distinct typed Option<u8> child ProjectionIds/ValueFacts, no independent child top-level original/Allocation/Domain. All four current values None. Normal parser first and retained second pass accept, CHECK0, authentic public issued single record valid, C17 exact one full Option carrier of four elements with stride sizeof(Option<u8>), GCC13 and Clang23 strict C17 O0/O2 native guest returns 0. Do not assume all-zero None bytes or a foreign stable tag ABI.

Positive two.nl: independent river_cells and mountain_cells source constructors, distinct original root/place/incarnation/world-appropriate identities, eight fresh distinct dependent Option children (4+4) current None. CHECK0 and authentic public count=2 valid_at(0,1); C17 two distinct live original carriers, no root/child/provenance share, four full Option slots each, same strict native exit 0. Physical observation requires passive live observer, not fabricated C roots or numerically equal constructor output.

Source negatives: arity_three.nl and arity_five.nl are syntactically valid Array AST vectors and must fail at semantic arity; wrong_element.nl fails semantic type; wrong_nominal.nl fails type/profile mismatch (not structural equivalence). Zero generated C and nonzero CLI expected. copy_unsupported.nl, bare_option_unsupported.nl, return_option_unsupported.nl are C/E/F respectively, UNSELECTED source eligibility, fail closed IMPLEMENTATION UNSUPPORTED rather than language-wide semantic illegal; do not let uncalled return helper bypass the fence. indirect_leaf_unsupported.nl is NOT direct qualified None leaves and must be rejected as source-ineligible; separate Option local status not automatically semantic illegal.

Public proof negatives, to construct post-freeze without altering these oracles: foreign source context/world despite same number IDs, different original root with copied child, duplicated child ProjectionId, wrong element type/count, stale ended/replaced root/child, forged issuer/source retention. Reject public valid_at; cannot accept host-seeded apparent positive evidence. Verify actual first-pass registered source, owner retain, issued contexts and exact original/child identities. No unselected Some, indexing, general Array Copy, array loans, generic, FFI.

Resource oracle: actually reached parser/second-pass/issuer allocation failure, fail closed; no partial stdout C; named --emit-c17-to sentinel contents AND inode preserved; no temp residue; subsequent unchanged source retry succeeds. If fault index not demonstrably reached, report NOT TESTED rather than claiming OOM PASS.

Native oracle: unchanged emitted C17 compiled by GCC13/Clang23 with -std=c17 -pedantic-errors -Wall -Wextra -Werror at O0/O2, main return 0, full Option carrier starts/strides measured while live by passive observer; no rewriting emitted semantic C to force success. Unsupported negative sources must not produce C. Exact None bytes/tag unspecified by language.

CTest names predeclared: r803_array_one_public.unit, r803_array_two_world.unit, r803_array_negative.unit, r803_array_oom.unit, r803_array_native.integration. K=5; FULL5 five per-matrix counts 453, PARTIAL 452, only llvm.c_api delta, same FULL5 named inventory. CI one natural pair on stable HEAD, never rerun/change negative/source/oracle after first failure without C approval.

SHA256 exact file bytes (ASCII UTF-8, LF, including final newline):
eb2423e7b9eab273fe21b5eff11c7a808b9fd383e48542a7f7400ad2cc09505d  tests/r803/one.nl  (186 bytes)
e1f95235eac4c3fb029251ed8a1cf51ad9b0fef0c0ea2aac09a5a9e987423a61  tests/r803/two.nl  (340 bytes)
cb0479b27d9e4d41c96ac0aeb5114bc3f7d9eae5bb0d17d99a87ba233ed4cca2  tests/r803/arity_three.nl  (158 bytes)
4b83dc08830a9aa281b8c42ddd61b8445194a1652100ce08f20591b0065a2539  tests/r803/arity_five.nl  (209 bytes)
f39d750b64f0514bbc9026d460d99b82f8754392e9484f5b651404420c35658c  tests/r803/wrong_element.nl  (177 bytes)
429414b9b7b71822245ea22857fa54d208106f71fed1ce0df01fcc64512075ad  tests/r803/wrong_nominal.nl  (189 bytes)
b50b3b2bf674b5679016d6ac1d63a2d6bf1ab6951eb7c46d4603b2c980c85097  tests/r803/copy_unsupported.nl  (220 bytes)
981f30410d858b80523d75de5deaea0d7ceffeb3d1c14597d625f30542c59d23  tests/r803/bare_option_unsupported.nl  (73 bytes)
6bbbdaad90c19bc8d3f8e3e4c61213ca17790a90a45f020ef3174b0154312e3e  tests/r803/return_option_unsupported.nl  (95 bytes)
d3bea3f66bff0408e63bc9cf8bab50df6cba9f89ae14c26edb1b224c3cd94dc1  tests/r803/indirect_leaf_unsupported.nl  (172 bytes)

First pass only: PASS on fully observed mandatory layers; FAIL on unsound semantic, foreign world, native, negative or OOM acceptance; INCONCLUSIVE on missing coverage, tool/harness problem, unproven reached OOM. P green runs are not independent proof.
