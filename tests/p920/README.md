# P920 — canonical parent IF arm ownership repair

Fixed producer P918 `58f54c155d2b9515ad4f277c96186bf73e599290`,
tree `38ac1fdb52df514b91d1957487bcb7533ea284c8`, Draft17.34.
P-owned NON-BLIND calibrated tests, not independent R/F. Original P918,
F67 static STOP, R919 preparation, old P/R sources/oracles/CI are preserved.

Before branch/production code/CI the read-only Gate A constructed two NEW
CounterTune source programs (TRUE/FALSE) on the fixed P918 library. Source,
oracle, standalone harness, exact mutation order and expected legacy/fixed
statuses were frozen first in freeze.json. Raw old-producer observations are
retained: both genuine arms private0; WHOLE parent arm-entry swap changes the
canonical getter, match0=0 loses its original arm mapping, context_origin0
becomes the other valid arm world. Old P918 nevertheless accepts both arms
for every mutation (12 false positives), and takes all six corrupted roots.
Restoration is exact and all artifacts stay alive/owned exactly once.

New production repair: one adjacent private `owned_arm_edges` predicate,
used BEFORE original-tree transfer and before EVERY owned replay. It checks
both source-ordered parent entries against the official nl_checked_if_arm,
same IF/match/backpointers/ordinal, original entry proof owner/source and
artifact/world mapping, context_origin, private link world, and two distinct
arm graphs/worlds. A pending caller is not dereferenced until its exact parent
edge is established. Same CALL IDs are not owner identity; caller/callee may
legitimately share a semantic context. Existing replay/owner/destructor and
Stage0 callback contract remain unchanged. No new allocation or schema.

New tests:

- `p920_parent_arm_edges.unit`: both positives after input/parser/registration
  context destruction; independently mutate WHOLE entries / match /
  context_origin; confirm canonical mutation reached and both private
  validates return precision3; restore and revalidate both0 each time.
- `p920_take_edge_rollback.unit`: linker wrapper injects the same real parent
  entry mutations immediately BEFORE the real take function. Assert root and
  context pointers remain the original locals (no transfer), then restore
  before ordinary error cleanup. Private count0/status3, SAME registration
  context retry, release original input, and replay healthy both arms0.
- `p920_cli_unchanged.integration`: frozen source SHA, ordinary CHECK3/P6,
  stdout C17 empty, named output/sentinel retained and no temporary file.

Existing P917 RETURN-old, equal-literal/wrong RETURN, real equal-number foreign
graph, actual new pending/PRE/POST OOM and rollback tests are unchanged and run
as regressions. New own evidence also compares P905/P918/new CLI byte triplets.
Host C17 tests/sanitizers are NOT emitted M8b guest C17/native evidence.

The external pre-code harness was initially formatted using clang-format's
fallback style outside the repository. Repository dry-run rejected that style
before production build/test. Only pinned repository formatting was applied to
the derived executable test. Original frozen harness bytes/SHA remain in
evidence/edges-precode.c.txt; formatter-preparation.json records both hashes and
the exact formatting-only relationship. No frozen source/oracle was changed.
Raw verbose CTest output retains its original trailing spaces; code/doc diff
checks exclude only evidence .log files instead of rewriting them.

Profile stays one definition / exactly one IF / two NORMAL arms / one direct
ref-write,u8-to-old-u8 call under each loan, two lexical originals. A predecessor
unrelated IF still exceeds root.arm_count==2 and remains unsupported; R919's
original source is not changed, admitted or regraded here. Optional additional
IF/duplicate-match permutations are NOT claimed tested. No H/Array/RETURN arm,
general CFG/ownership, arbitrary hostile pointer validity or callback reentry.

Only copy_body.c, additive CMake and new P920 tests/evidence change. Public IF
validator/proof/Change issuance, normal CHECK0 and backend/native stay HOLD.
Product GO PAUSED/H1 HOLD, well-factored-C author+ordinary-safe caller audit
locality UNMEASURED. New F/R or adoption remains Coordination's decision.
