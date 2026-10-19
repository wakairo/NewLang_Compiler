# P633 / USER-CLI-3B — Stage0 correlation PASS, integration HOLD

Evidence only. **Not a mergeable compiler candidate.** Production sources,
CMake and both historical Python scripts at this branch HEAD are byte-for-byte
fixed main `032cf60aabf4997e672bc3607cf53775d91ececb` / Draft17.32.
The finite attempted implementation is retained as an **unapplied** patch,
not silently shipped. No candidate PR, FULL5/PARTIAL Actions or rerun was made.

## Authority, historical audit, and pre-edit freeze

Issue633's user launch, Compass -> Value Strategy §§1.1/1.3, Development
Process §§4.2/4.3/4.3.1/7, current Roadmap, Decision Procedure and Ledger were
read. M632/C631 and the genuine P630/P631 frozen findings were reviewed. KEEP
canonical §§19.1/27.1–27.4 and historical Draft17.15 IF; N/A for language
semantics replacement. The added private issuance invariant is explicitly a
**new non-normative internal proof contract**, not an already-proven public
issuer. No source grammar, semantic join or Domain/Allocation authority changes.
DI-009–014 owners and DI-016 Array4 remain outside scope.

`stage0-freeze.json` was written before edits and records original source,
oracle, raw failures and old script hashes plus equal83/83 paired-swap MUST
REJECT. `stage1-freeze.json` predates integration execution. The archived
`tests/p630/**` and `tests/p631/**` are unchanged from evidence commit
`147bead4848699ed388f2e6df6d35f89330571c6`, not current Production capabilities.
`fixed-main-ci.json` rechecks exact-main FULL39224748355/PARTIAL39224748336:
SUCCESS, attempt1. Live main remained fixed at launch and preservation.

## Stage0 — narrowly passed, no arbitrary-parent authenticity claim

The real checker issues two different arm contexts in `source_if`. The patch
adds one borrowed `context_origin` pointer to each private parent-held
`NLCheckedArm`, captured at `save_arm`. The private consumer compares the
inspected arm world with that issuance-time pointer and parent/IF/index/source
and current slot. This authentic parent record, not the mutable child slot,
is the independent anchor.

**Actual trust boundary:** `newlangc` owns its freshly source-checked entry and
parent body throughout validation/emission. It has no input API for external,
serialized or caller-supplied parent artifacts. The protected parent issuance
record remains authentic; a test-owned shallow child view may be substituted
in the child slot. The test does not overwrite the trusted origin record except
for explicit single-field corruption followed by restoration. Equal scalar
values/numeric IDs/source spelling are never used as issuer identity.

This is **not** cryptographic immutability or resistance to arbitrary writes to
all parent+capture+child host memory. A self-consistent entirely forged parent
could lie to this private check; a future consumer admitting untrusted parents
needs an independent trusted boundary. This change is not such a public proof
API and cannot justify that future use. Capture corruption alone and a copied
parent borrowing genuine original children are rejected. No new issuer/global
registry/authority is constructed.

GCC14 and Clang23 local frontend-only builds accepted genuine source-issued
arms after original AST/parser/input teardown, and rejected 15 controls per
source: single and paired equal83/83 swap, unequal83/24 swap, parent/IF/index,
source/borrowed child, captured-origin corruption and copied-parent mismatch.
Source bytes, nodes, world values/places/bindings/scopes and parent arm records
were fingerprinted before/after and unchanged. `stage0-disposition.json` and
`stage0-*` logs contain actual observations. Stage0 itself is the private origin
gate, not a claim of local CLI/native admission before Stage1.

The pointer is nonowning; the parent-owned child already owns that context and
outlives the consumer. Realloc moves records, not their referenced contexts;
there is no new allocation or destructor. Observed 64-bit record size16->24,
one pointer per arm, at most512 extra bytes per 64-arm parent. Existing failure
paths destroy the uncommitted subtree. Pointer is neither owner nor Domain.

## Stage1 — genuine finite successes, followed by STOP

The exact historical P630 +211/-26 prototype was first replayed on an isolated
copy of fixed `main.c`; its SHA256 matched the old recorded35d7f190... baseline.
Its finite consumer/emitter was then combined with the origin guard (final
main.c +236/-28; total three Production files +242/-29). Names and punctuation
checks authenticate actual declared source identities; no filename or fixture
whitelist was added. Arm-local IDs/facts do not become caller IDs or facts.
Generated real IF/ELSE evaluates the selected branch, then forwards its Copy
result; branch-local C symbols are scoped to avoid collision with caller IDs.

All seven frozen P630 positives, both unchanged old complex-arm sources and
both P631 probes reached actual CLI CHECK0, real checked-owned public readback,
C17 generation and strict GCC/Clang O0/O2 native. Plain guest and passive
assertions are separate runs; no host guest initializer or result injection.
P626 complex selects42; P628 complex updates original Pair27->91 with old27,
right201 and unchanged live root/field addresses. The other Pair cases preserve
old18/right209 and select83 or24, including equal83 arms and distinct equal83
outer sources. Public readback verifies separate arm worlds, ended local
bindings/places, no Domain/region/occurrence authority, and unknown joined Copy
rather than a forged known scalar. Existing scoped write metadata preserves
original root/children/incarnations, old Copy, Change and ended loan.

Only the two authorized `complex_arm.nl` Python cases were temporarily migrated;
the patch records that exact diff. Original sources/manifests never changed.
No new public validator was added: public accessors/readback, existing checks,
private consumer mutation tests and native observations are distinct layers.

32 new local controls per two-local-arm source passed GCC/Clang: paired context,
receiver/binding/name/span, forged ValueID, declaration links, scalar metadata,
revived ended arm place/current fact/value, consumed/hidden binding. Separate
same83 different outer-symbol and source-span derivations were rejected (2).
Existing 71 identity controls were rerun on five new paired sources, reporting
unchanged worlds/zero driver allocations; historical controls are not a
substitute for the new gate. Actual Change/effects *inside* an admitted pure arm
and owner/Domain payload controls are NOT REPRESENTABLE by this selected
one-u8-binding profile, not declared PASS.

Type and range errors remain CLI3 semantic USIZE-LESS-THAN-TYPE and
V1-U8-LITERAL-RANGE with no C, rollback PASS/artifact NULL. Two-local/nested
inputs remain CHECK0/backend4/no C. Existing branch_effect.nl remains CLI3
precision P6-JOIN-PRECISION. The new test initially wrongly assumed this last
case was CHECK0/backend4; the unchanged-main and candidate diagnostics were
identical. Both failed initial local logs and the honest harness classification
correction are retained in effectful-test-calibration.json; no old source or
oracle was adjusted for that calibration.

OOM every-prefix registration/main passed on both-local259/148,
outer261/150, pure152/96, with unchanged state and NULL checked output on failure.
Existing semantic refusal rollback passed. These are not new full-driver I/O
failure injection tests. No new ASan/UBSan or remote CI was run.

## Exact blockers — no unbounded repairs

1. **Integration regression / authorization boundary:** both complete local
   frontend-only suites produced346/347, stopping only at
   `p605_arithmetic_comparison.integration` line21. Its adopted-proof manifest
   fixes the *entire* `src/semantic_internal.h` hash to15e89aef...; the authorized
   one-pointer private record change makes it647ae68f.... Public arithmetic
   issuer/validator files remain byte-identical. `tests/p605/**` is outside
   Issue633's allowed edits. No manifest relaxation, skipping or out-of-scope
   migration was performed. C must authorize an exact, historically preserved
   integrity-check migration or choose fallback CLI4. This is a real test
   contract conflict, not an arithmetic/native failure. See raw full-local
   logs and out-of-scope-hash-conflict.json.
2. **Additional private integrity question:** after the32 passing controls,
   replacing Change child_post_fact or parent_post_fact with an arm-local fact
   is rejected. But setting the caller's **already-ended** original root's
   current_fact0 to arm-local numeric7 is accepted by the private consumer AND
   existing internal nl_sem_validate (OK). Both equal83/83 and unequal83/24,
   GCC/Clang, reproduce it. The root is live=false, not a live authority;
   source/nodes/write evidence stay unchanged and the real value is restored
   before original revalidation. This does **not** demonstrate live-root
   currentness loss, bad native output, language unsoundness, public proof
   forgery or authority creation. Existing fixed-field validation explicitly
   skips ended places. Whether canonical irrelevant ended-place residue is
   outside the consumer evidence contract, or should be explicitly rejected,
   needs a precise C/M disposition. P did not weaken the pre-frozen rejection
   class or add a new parent-state invariant on its own. The added diagnostic
   gate intentionally exits1; parent-fact-stop.json preserves actual results.

The full suites ran before this last diagnostic was added; their347 counts
include the earlier32-control gate, not a PASS of the later failing control.
GCC13 targeted9/9 was also before that control. The later GCC/Clang diagnostic
failures are separate. Full5/PARTIAL candidate Actions, sanitizers and integrated
candidate PR are NOT RUN/NONE. No test suppression or repair after remote failure.

## Reproduce the preserved attempted implementation

On a clean worktree of this evidence HEAD (Production remains fixed main):

```
git apply --check --unidiff-zero tests/p633/evidence/unapplied-candidate.diff.txt
git apply --unidiff-zero tests/p633/evidence/unapplied-candidate.diff.txt
CC=gcc cmake -S . -B build-replay -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-replay --parallel 2
ctest --test-dir build-replay -R p633 --output-on-failure
```

Expected: Stage0 origins and genuine native tests pass; the last ended-parent
fact assertion fails. The full suite also fails unchanged P605 header hash.
This is **not** a green candidate. Use Clang as CC for the second local build.
Toolchain activation is required for CMake/Clang; normal FULL LLVM23.1.3 policy
is unchanged. Local retained Clang is23.1.2 and local tests are PARTIAL only.

`unapplied-candidate.diff.txt` and candidate-metadata.json preserve the exact
attempt and hashes. source-owned-c17.json and eleven C17 text files are actual
emitter output. Native stdout is in raw GCC JSON / Clang test log; synthetic C
mutations only test the passive observer. Empty local-equal stdout files are
preserved first failed diagnostic attempts, not PASS records.

## Final state, cost, and goal alignment

Production, CMake and both old Python scripts were restored to fixed main.
All historical freeze hashes agree, old integrations2/2 pass, four representative
sources again CHECK0/backend4/Cbytes0. No main merge, Canonical change, external
publication, other Track or independent R launch. No candidate PR or Actions.

Stage0 finished within five minutes of task start; STOP was reached early,
well below90–150 minutes. The initial bounded private-correlation hypothesis
worked, but whole-header historical integrity tests and exact ended-parent
metadata contract prevent claiming an integrated candidate. Next authorization
should address those specific points before one natural CI pair/fresh BLIND R;
no public issuer or general CFG is implied necessary by these results.

Technical capability is not Product GO. No separate VALUE-ALIGNMENT-RISK is
asserted for the small reusable selected-IF experiment; further broad proof
machinery would require renewed cost/alternative review. Strategy§1.1 remains
unmeasured, active Product GO scoring PAUSED under624, H1 HOLD unchanged.
