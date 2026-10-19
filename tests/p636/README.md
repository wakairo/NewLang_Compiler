# P636 — reject malformed fixed-field IDs before indexing

Fixed main `032cf60aabf4997e672bc3607cf53775d91ececb`, CURRENT_SPEC Draft17.32.
This new branch starts from main and carries the exact PR635 patch; PR635
HEAD `b5ecc584c9fcda2fcca74dd468443661bd8107ea` stays frozen/unmerged. Relative
to that tree only `src/fixed_field.c`, test registration and `tests/p636/**`
change. No parser, source semantics, public issuer/API, owner/Domain or CI change.

## Before repair: two real failures, not flakiness

`evidence/pr635-ci.json` and complete original ASan/UBSan job text preserve
FULL39255150486 (FAILED), PARTIAL39255150693 (SUCCESS), attempt1. Both sanitizer
jobs FAILED in existing `p517_gate_test tests/p630/then_true.nl`, run from
`p633_local_cli.integration`: ASan heap-buffer-overflow READ1024; UBSan unsigned
subscript underflow at old `fixed_field.c:319`. P634's earlier ASan-pending
observation remains historical, not rewritten; C later confirmed final failure.

Before modifying Production we repeated that exact command in separate local
ASan and UBSan frontend builds. Both exited1 with the same respective findings;
`local-before.json` records commands/statuses/hashes, and full local logs remain.
There is no retry of old Actions or modification of PR635.
Original/local failure logs use lossless `.log.json` wrappers: decode `text`
as UTF-8 and verify `sha256` to recover the exact original bytes, including
trailing spaces. No failure text has been trimmed or rewritten.

P517 changes the genuine arm world's original root `current_value` to0. The new
one-local consumer can call `nl_sem_validate` before later finite frame checks.
`nl_fixed_validate` then reads `values[r.current_value-1]`, underflowing for0.
The ASan1024-byte read matches a whole NLSemanticValueView load. This is a real
compiler host memory-safety implementation defect on hostile proof mutation,
not a demonstrated legal-source guest-native failure, semantic contradiction,
or evidence that human library UB audit reduction is impossible.

## Bounded index audit and fix

The genuine context owns valid allocated tables and their counts. Only record
IDs/counts listed below are adversarial; this is not arbitrary host-pointer or
owning-count memory-corruption resistance. A small fail-closed check returns
existing `NL_CHECK_INTERNAL_ERROR`, never invents state or authority.

| Existing access | Required established bound |
|---|---|
| `types[a.type-1]` for a non-ended aggregate value | `0 < a.type <= type_count`; aggregate field count within fixed array and equal declared count |
| `a.fields[f]`, `values[a.fields[f]-1]` | existing bounded field loop and nonzero/in-range member ID retained |
| `types[p.type-1]` for a live place | nonzero/in-range place type; live fixed-field count within array |
| `places[p.parent_aggregate-1]` | nonzero parent case and parent ID <=place_count before dereference |
| `nl_fixed_live` internal `r.fixed_fields[p.parent_field_index]` | parent count <=MAX, index <parent count before helper call |
| `values[r.current_value-1]` | nonzero original parent value ID <=value_count before dereference |
| `types[r.type-1].field_types[p.parent_field_index]` | valid parent type, index <declared field count and physical fixed bound |
| `a.fields[p.parent_field_index]` | PLACE carrier, aggregate count=bounded parent count before access |
| `values[p.current_value-1]` | nonzero child value ID <=value_count before dereference |
| dependency place access after this loop | existing live/place checks; all live aggregate-child records already passed these bounds |

No helper or unrelated validator rewrite. Complexity is unchanged, no new heap
allocation/registry/snapshot/proof or semantic-world mutation. Ended aggregate
values/places retain their existing skip behavior; no global zero-fact rule.
The previously selected PR635 private ended-original-root check is unchanged.

## Actual-source regression tests

`freeze.json` predates Production changes and records every original PR635 file
hash and expected rejection. `index-contract.json` specifies17 direct controls:
original/child value0 and past-end, parent past-end, field index/count overflow,
aggregate/root/child type0 and past-end, aggregate field count overflow and
member value0/past-end. Tests require `NL_CHECK_INTERNAL_ERROR`, no crash,
read-only mutated world, and successful original revalidation after restore.

The primary worlds come from actual registered `main()` after original
AST/parser/input teardown, using both arms of unchanged P630 then_true and
P631 same_value_local. Each arm independently executes17 direct controls, then
the original private CLI consumer revalidates. Original nodes/source/parent
issuance records are not replaced, and no fake value/permission is created.

An additional non-IF example independently registers actual Pair source and
checks `let p=Pair{left:u8(31),right:u8(117)};` through the Production parser and
checker. It creates one real live lexical Pair and two dependent fields. The
same17 controls run directly against this actual source-issued world without
requiring an IF or backend admission. This is semantic-fragment evidence, not
a separately claimed whole-program native execution. It demonstrates that the
shared defensive fix is useful independently of one-local IF.

Zero `parent_aggregate` is a legitimate no-parent sentinel, so it is never
subscripted as -1 and is not a globally invalid ID. In the actual original-child
relationship corruption it is separately rejected by the unchanged private
source consumer. We do not turn a bounds fix into a new universal ownership
rule or falsely claim the shared function is a complete graph validator.

After the fix local ASan and UBSan both run the original P517 command successfully
(71 rejects, original PASS, worlds UNCHANGED, driver allocations0). Both run the
new3 tests without sanitizer findings. The actual eleven CLI sources, paired
same-value contexts, dead root facts, binding/span/ValueID/Change, outer derivation,
P605 public +/- proofs and OOM/rollback are rerun in broader targeted18 tests.
Complete final local GCC/Clang suites run after this final code, not inferred
from old unsanitized results. Exact logs and final counts accompany this file.

## Preservation and normal CLI evidence

All prior Production IF/capture/closed-root code, header, P605 exact reconstructed
full-original-header hash loop, and old P626/P628 complex positive migrations
are byte-identical to PR635. Original `.nl`, manifests, proof issuer/validator
and P517 hostile controls are unchanged. P630/P631/P633/P634 failed results stay
historical, including their old READMEs' statements about those frozen HEADs.

Eleven preserved source CHECK/owned/C17/native cases use ordinary `newlangc`,
strict GCC/Clang O0/O2 plain+passive, actual selected branch/Copy, old value,
sibling and live original Pair addresses. No host guest initializer/owner seed.
New validator checks must leave emitted C17 SHA identical to frozen PR635.
Semantic errors remain CLI3, legal unsupported forms remain CHECK0/backend4,
and no-C/error cleanup and known OOM prefixes stay tested. No general source,
heap/nonCopy ownership, public proof or CFG support is added.

Reproduce targeted:

```
CC=clang-23 cmake -S . -B build-san -DNEWLANG_FRONTEND_ONLY=ON -DNEWLANG_SANITIZER=address
cmake --build build-san --parallel 2
ctest --test-dir build-san -R 'p636|p633|p634|p605_arithmetic|p62[68]_cli' --output-on-failure
```

Use `undefined` for UBSan. Local retained Clang23.1.2 is PARTIAL; normal remote
FULL5 keeps pinned LLVM/Clang23.1.3. Only one new natural FULL5+PARTIAL pair,
no manual retries, no HEAD repair after CI. Exact frozen candidate/CI outcomes
are reported once in Issue636, never backfilled into old evidence.

## Classification, history and product boundary

Compass, Strategy §§1.1/1.3, Process §§4.2/4.3/4.3.1/7 and canonical IF/Copy
reviewed. KEEP selected semantics; Historical design audit N/A for the new
bounds repair because it preserves valid source rules and fixes unsafe host
indexing. New public issuer/CFG/lifetime/owner remains DEFER.

(S) safety-critical compiler-host indexing bug: directly reproduced and fixed
within existing invariants. (I) prior P605 historical-hash migration is preserved.
No new (D) semantic/proof architecture gap or (V) actual library-author+ordinary-
safe-caller audit/re-audit burden result is established. The number of old HOLDs
or private consumer lines is maintenance data, not proof of a language/product
dead end. The shared non-IF test and small no-allocation guard are concrete reuse.
No separate new goal-deviation finding is asserted. Long-term value still needs
well-factored C comparisons under Strategy§1.1; Product GO active scoring PAUSED,
H1 HOLD. Fresh BLIND R is a separate C/user decision before adoption, not launched
here. Candidate DRAFT / DO NOT MERGE, no main merge or PR635 change.
