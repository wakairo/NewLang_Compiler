# P634 / USER-CLI-3C — final bounded ordinary CLI local Copy integration

Authority: Compiler/main `032cf60aabf4997e672bc3607cf53775d91ececb`, CURRENT_SPEC
Draft17.32. This candidate starts at that main, not evidence HEAD. Source
semantics, public proof/issuer/validator, toolchain, CI, owner rules unchanged.

## The two authorized fixes

1. P605's original `frozen.json` remains byte-identical. The single hash loop
   in `integration_test.py` permits precisely the archived four-line private
   `NLCheckedArm.context_origin` comment/field block, once in that exact record.
   It strips only that block from an in-memory header copy and requires the
   **original entire header SHA256**15e89aef...; every other adopted proof-file
   hash remains active. Full candidate header hash647ae68f... is also printed.
   No arithmetic source/readback/native assertion is changed. New hash tests
   execute the actual loop against isolated copies: missing/duplicated/misplaced
   block, changed pointer, unrelated header edit, and all3 other proof-file
   edits reject. This is a narrow historical test migration, not an exemption
   from public arithmetic proof verification.
2. The existing bounded original-Pair/IF consumer now also authenticates its
   genuinely ended original lexical root's `live=false`, `current_fact=0`,
   `current_value=0`, using the already-checked original root/Change identity.
   No canonical global ended-place invariant, new `nl_sem_validate` behavior,
   public proof or new authority is introduced. The general internal validator
   remains permissive for irrelevant ended metadata; this finite private C17
   profile refuses it. Fact0->arm7 and value0->source-issued value, or revival,
   reject while the genuine artifact remains accepted.

`freeze.json` predates either fix. `evidence/before-two-fixes.json` independently
reproduces both old failures after exact hash-matched replay of P633's unapplied
patch. All old P630/P631/P633 records/negative oracles/failed logs are copied
unchanged and remain historical. P633 README's evidence-only/current-HEAD
language describes its old frozen HEAD, not this new candidate; that text and
its preservation manifests are not retroactively edited. P605 original
full-header failure likewise stays true in history.

## Ordinary user path and finite evidence

```
newlangc --check tests/p630/both_false.nl
newlangc tests/p630/both_false.nl > program.c
cc -std=c17 -Wall -Wextra -Wpedantic -Werror -O2 program.c -o program
./program
```

The CLI now emits real IF/ELSE blocks with at most one ended local `u8` Copy
binding per arm. Its initializer is the existing u8 literal or earlier ordinary
Copy binding; its tail returns that local. Original arm-local IDs/facts are
never imported into caller state. The existing zero-local profile stays on
its original ternary path. No filename/fixture allow-list is used in Production.

All eleven original sources are unchanged: seven P630, P631 equal83/83 and
same-valued distinct outer derivation, old P626/P628 `complex_arm.nl`. Real
registered `main()` is checked after original AST/parser/input teardown; public
readback and private consumer tests are distinct layers. `source-owned-c17.json`
records source hashes, CHECK0, issued public observations and actual emitted
C17 SHA from GCC13/Clang builds. Eleven exact C outputs are saved separately.
The new checks do not change any of those emitted bytes from the P633 prototype.

Strict GCC/Clang O0/O2 execute both plain generated guest and passive observers:
TRUE/FALSE, actual comparison and selected branch, old18/new83 or24/right209,
old P626 value42, old P628 original Pair27->91/old27/right201 and unchanged live
root/field addresses. No host-initialized guest result, fake semantic owner or
permission. Synthetic C mutations exit91 and only validate the observer;
they are not static NewLang rejection evidence.

Historical P605 composition remains active: source-issued +/- public proofs,
0+0=0 with9-2=7 and TRUE, and7-7=0 with FALSE, followed by real strict C17/native.
Only its header-hash loop changed; no original source/oracle/native assertion.

Private controls: paired equal-value and unequal contexts, parent/IF/index,
source/span/name/binding/receiver, ValueID, ended local state, cross-arm Change
facts, outer same-value different derivation, final ended original root.
The old P633 gate now passes **35 rejects** per equal/unequal source (including
its previously failing ended fact). New P634 checks3 fact/value/live corruptions
per source. Stage0 gate still15 rejects per source and original+replay worlds
unchanged; two distinct outer derivations reject. Existing71-control identity
checks run on admitted paired sources, with zero driver allocations. These are
finite private checks, not a new public validator or soundness theorem.

The trust model is the actual CLI's fresh owned parent issuance record, kept
independent from untrusted copied child views. It does not authenticate arbitrary
self-consistent rewritten parents or deserialized external proofs; there is no
such CLI input. The borrowed captured context pointer has no lifetime beyond
its parent-owned subtree and creates no Domain/Allocation authority or new heap
allocation. Public scope/provenance/currentness/owner mechanisms stay unchanged.

## Rejections, rollback and CI boundary

- wrong type/range: CLI3 semantic USIZE-LESS-THAN-TYPE /
  V1-U8-LITERAL-RANGE, zero C, rollback and NULL artifact;
- two-local/nested: CHECK0, backend4 V1-BACKEND-UNSUPPORTED, zero C;
- existing effectful branch: CLI3 precision P6-JOIN-PRECISION, not backend4;
- existing unsupported read-loan grammar remains parser profile rejection,
  not a claimed semantic safety proof;
- OOM every registration/main prefix rerun on both-local259/148,
  outer261/150, pure152/96, rollback unchanged/NULL checked output. Public
  consumer stays read-only/zero allocations. This is not a new whole-driver
  I/O fail-injection test; normal semantic/backend failures emit no partial C.

`after-two-fixes-{gcc,clang}.log` are targeted15/15. Final whole local suites are
run **after** the complete ended-root controls and hash migration, not the
historical P633 pre-final346/347. Their raw logs/results are saved; local builds
are NO LLVM/PARTIAL (GCC13.3.0 and retained Clang23.1.2). Normal remote FULL5
still pins LLVM/Clang23.1.3; one natural FULL5+PARTIAL pair is permitted after
all local checks. Exact candidate CI result/HEAD is reported in Issue634, not
backfilled into old records. No manual rerun or later HEAD repair is authorized.

Reproduce local:

```
CC=gcc-13 cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

## Historical / goal / cost boundary

Compass and linked Strategy §§1.1/1.3, Process §§4.2/4.3/4.3.1/7, Roadmap,
Decision Procedure/Ledger and canonical §§19.1/27.1–27.4 reviewed. KEEP selected
IF/Copy rules and historical Draft17.15. Language replacement N/A; capture and
closed-root checks are explicitly non-normative private integrity contracts,
not new public issuers. Owner/heap/Domain/Array/general CFG/effects remain DEFER.

Technical success is separate from Product GO. **VALUE-ALIGNMENT-RISK (cost):**
multiple rounds and ~248 new Production lines for one-local IF require scope
control; no stable safe library API or measured author+ordinary-safe-caller
UB audit/re-audit closure improvement follows from eleven finite native cases.
This final continuation adds only6 Production lines and the exact hash-loop
migration to the archived P633 candidate; no general proof machinery. If a new
problem appears, C should consider CLI4 fallback/pivot rather than another ad
hoc repair round. Product GO active scoring PAUSED under624; H1 HOLD unchanged.

Candidate is DRAFT / DO NOT MERGE. Fresh BLIND R on the frozen final HEAD is
for C and a separate user launch; P does not launch it or self-merge. Canonical,
main, existing PRs and old failures remain unchanged.
