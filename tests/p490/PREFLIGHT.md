# P490 immutable pre-execution continuation freeze

Track: P. Exact main d6e603e12591c2ff39e3e1b77c784681fec66033,
CURRENT_SPEC -> Draft17.30. Compass -> Product Strategy1.1 (audit dependency
closure), Process4.1.1/4.2/4.3.1, canonical loan/Copy/currentness/Change/Reset/
Option and normal joins. No PR479 inheritance or other task output as evidence.

Both complete independent candidates frozen in this commit BEFORE any compiler
execution; bytes and semantic/physical expectations are in frozen.json. C1 is
byte-for-byte P471 A except post-first-match let joined_tail=tail@next;
joined_tail; before final head reset. C2 is preselected C1 plus exhaustive
match joined_tail with None unit and Some(p) ordinary nonescaping scoped
loan_read_ptr(p){|tail_ref|unit}. Existing source syntax, no forged q/p or safe
ref. No candidate rewrite, third case, post-hoc oracle or compiler fix allowed.

Expected canonical legality: after Some(q) original-tail update, roots/incarnations
remain original and live, old Copy None, physical tail.next self-link, read should
observe Some(original tail), head final None, immutable payload2/1. C2 should loan
same original if its exact current ptr facts can be proved. However production
normal join deliberately makes changed Option Unknown (and parent RF new), not
an arbitrary concrete branch result. Prediction: C1 may CHECK and copy Unknown
while preserving genuine public join proof; C2 likely first refuses subsequent
Unknown Option match/extraction before reloan. C1 backend read is unmeasured.

Stages: exact-main LLVM-free --check both independently. On first CHECK error,
record full span/CLI/diagnostic and stop that case. For CHECK0 inspect authentic
source-owned public entry/arms/post + validator, original field/root identity,
Change/Reset old Copy and sibling/payload; verify subsequent FIELD_READ matches
actual post proof, without pretending Unknown equals Some. If proof cannot be
validated, stop. Only then unmodified C17 emission; strict GCC and Clang O0/O2,
passive address/tag observer from public source mappings if native reached.
No exit0-only provenance claim, C patch, host authority/proof overwrite.

Safety negatives stale EndRoot, old Copy as live capability, treating None as
Some are UNTESTED if not meaningfully expressible in these fixed two inputs;
never call a precision refusal semantic-safety PASS. Wrong copied proof views
may test existing validator, not new source legality. Unknown is not a Domain.

Only new tests/p490/**; optional one minimal CMake registration only for an
actually validated positive regression. No Production/public issuer/validator/
backend/adopted tests/docs/toolchain/workflow edits. If useful DRAFT/DO NOT MERGE
PR: one naturally triggered FULL+PARTIAL pair, no retries/success-driven rewrite.
One final Issue490 report after evidence collection, stop task; no merge/close/
other Track. §4.2 N/A unchanged normative/source semantics; finite Exploration
only, no Ring/heap/nonCopy/library/Product GO. H1/STB/470 unchanged. C owns next.
