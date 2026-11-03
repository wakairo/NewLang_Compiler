# R1129 C221c final evidence

## Result and stopping point

**FULL5 scoped finite observations completed; final PARTIAL remains incomplete after an R mechanical command timeout. There is no overall all-green R validation result, Product PASS, READY or merge decision.**

Prepared 2026-11-03 22:22:58 UTC. Authority: [C221c](https://github.com/wakairo/NewLang_Compiler/issues/1129#issuecomment-6614995406). R PR [1132](https://github.com/wakairo/NewLang_Compiler/pull/1132) remains DRAFT / NEVER MERGE. This archive adds evidence only to the final tested R HEAD. The archive commit itself is not a new tested execution HEAD.

Method: GitHub dedicated branch and natural Actions only; no local shell/executor/compiler/native/build/test execution. P implementation, fixtures, oracle/reports, F/M/B concrete examples and old R sources/oracles/raw remained unseen. Public interfaces and this R's own emitted C/AST/raw were the permitted mechanical inputs.

V1 is the original canonical-only BLIND source freeze. V2 and all execution are **PROFILE-CALIBRATED / NON-BLIND to declared structural scope; implementation/fixtures unseen**. No source or semantic oracle changed after its freeze.

## Fixed identities

| Item | Identity |
|---|---|
| Canonical main | `73ef10a97f2fab7b70d094b2114359e961acefb5` |
| Canonical draft | CURRENT_SPEC → Draft17.34 |
| Target P1130 HEAD | `dc21cf33105a7dba832e197f24e153ad87862a0f` |
| Target P tree | `34c85ebdebb0fe601d20856ad3f5f3a5446362e0` |
| Final tested R HEAD | `bc3386f1b710f262e51ebea9a3d05db6064f398b` |
| Final tested R tree | `df05451c46ebe95e696ca61157ba50100a30f066` |
| Actual final PR checkout | `c04eb5f1487056f928db597f49d2c776d5455dd9` |
| Actual checkout parents | `dc21cf33105a7dba832e197f24e153ad87862a0f`, `bc3386f1b710f262e51ebea9a3d05db6064f398b` |
| Final harness manifest SHA-256 | `68ef2e5ecb6469fed5a8d4812eef05c1a07f0dfc2b18d29bf08f721f9ac75807` |
| V1 manifest SHA-256 | `70134add33049ac8f3f09e442b55a89d3aecadc46f79a8ac7ce9f0c60299fe31` |
| V2 manifest SHA-256 | `3555c90c0fac9604ffe70a05450b23204e021222317292015fb9a0b6f6491a95` |
| V2 oracle SHA-256 | `4679977c9b0afc941d58f1e54a03bec4b8e996d1050ccdc1268c1ded17863644` |

V1 contains 9 files / 34,255 bytes; V2 contains 10 files / 24,553 bytes. Logical `r1129/v1|v2/...` paths map byte-for-byte to `evidence/r1129/v1|v2/...`. Runtime harness verification and the tested Git tree retain all frozen payload hashes. The final tested R tree adds exactly 28 R-owned leaves; all 5,817 existing fixed-P leaves are unchanged. See [tree receipt](tested-R-vs-fixed-P-tree-receipt.json).

## Complete natural Actions accounting

The whole-branch, all-page repository run collection contains exactly three push runs and four pull_request workflow runs. All attempts are 1. All jobs are completed. [Full ledger](actions-ledger.json) includes job metadata, step conclusions, actual checkout relationships and archive branch filter evidence.

| Phase | Run | Actual jobs | Outcome |
|---|---:|---:|---|
| Initial FAST | 41722844617 | 1 | Formatter failure; build/inventory skipped |
| Formatter repair FAST | 41724212879 | 1 | Success; actual unique R CTest registration, no R execution |
| Initial FULL | 41724783042 | 5 | Each 662/663 CTests passed; only R failed at passive mapper |
| Initial PARTIAL | 41724783037 | 1 | 661/662 passed; only R failed at same passive mapper |
| Final mechanical repair FAST | 41727594880 | 1 | Success; actual unique R CTest registration, no R execution |
| Final FULL | 41727605147 | 5 | Every job 663/663 passed, R finite observations complete |
| Final PARTIAL | 41727605211 | 1 | 661/662 passed; only R failed at Clang version timeout |

FAST budget is 3/6. Natural PR sets are 2/2, exactly 12 actual PR jobs. Manual runs, workflow dispatches and local executor/compiler/native runs are 0. No further triggering push, rerun or close/reopen is authorized. The archive ref is used only for non-triggering evidence preservation.

## Final FULL observations

Final FULL jobs: GCC 126236388413, GCC Release 126236388052, Clang 126236387772, ASan 126236388113, UBSan 126236387340.

Both normal and immediate-BREAK sources completed CHECK acceptance, ordinary complete-unit registration and genuine source-issued owned/public observation, C17 emission, unmodified native compilation/execution at O0/O2, and separate passive observed native compilation/execution at O0/O2.

For normal, the observed phases are [4,19], with one Continue/backedge and one BREAK. For the immediate control, the sole observed phase is [19], with zero Continue events and one BREAK. This is a finite trace; the syntactic/source-issued checked loop still reports one Continue edge and one BREAK edge in both sources.

The field order below is (original.signal, original.checkpoint, bystander.signal, bystander.checkpoint).

| Live observation | Values |
|---|---|
| Before loop, first header, normal Continue and next header | (146,37,89,211) |
| After B helper, BREAK, after loop | (146,58,89,211) |
| After caller write 1 | (37,58,89,211) |
| After caller write 2 | (37,58,89,58) |

The B helper's actual destination is the original checkpoint field. It records old=37, new=58, current=58. The next helper writes the original signal field with 37 and returns old=146. The last helper writes the bystander checkpoint field with 58 and returns old=211. The bystander signal remains 89. All four field addresses are distinct. The six root/field addresses stay stable throughout each passive process's live snapshots; helper destinations equal the corresponding mapped field addresses. Addresses are never compared across processes.

Public owned observations separately expose the twelve named scalar facts, including prior=37, returned_old=37, after_target=58, stable_sibling=146, other_target=211, other_sibling=89, caller_old=146, transferred_old=211 and final values 58/37/58/89. The LOOP issuer reports header_inductive=true, Continue edges=1, BREAK edges=1. The profile-specific direct-BREAK validator reports false and is not treated as a universal validator for this mixed loop. Public post-main root views are historical and live=false, with current_fact/current_value=0; they are not final live-place assertions.

There are exactly 40 guest executions in final FULL: 20 unmodified and 20 passive (two sources × two optimization levels × five jobs × two variants). All returned exit 0 without runtime stderr. ASan and UBSan each cover eight of those guest runs; their flags apply to generated guest compilation, not merely to the compiler host.

- GCC and GCC Release use Ubuntu GCC 13.3.0-6ubuntu2~24.04.1, with `-std=c17 -Wall -Wextra -Wpedantic -Werror` and actual O0/O2.
- Clang uses actual curated 23.1.3. Its existing public workflow warning policy adds only `-Wno-error=gcc-install-dir-libstdcxx`.
- ASan/UBSan add `-fsanitize=address` / `-fsanitize=undefined`, `-fno-omit-frame-pointer`, and `-fno-sanitize-recover=all` to the same strict guest command. Exact argv, compiler versions, executable hashes and environment are retained per job.

[Six-job receipt](final/six-job-receipt.json) records every command result. [Read-only raw review](final/independent-raw-review.json) records all 20 passive traces and 10 public observations without modifying raw driver grades. Representative readable [normal trace](final/job-126236388413/normal/observed-O0.stdout), [immediate trace](final/job-126236388413/immediate_break/observed-O0.stdout), [actual C map](final/job-126236388413/normal/storage-map.json), [passive C](final/job-126236388413/normal/observed.c), and [public assessment](final/job-126236388413/normal/owned-assessment.json) are included.

## Focused refusals and no-clobber

All five final FULL jobs record the same first public refusal during REGISTER. CLI CHECK, public observer and named C17 emission each exit 3 for every focused probe. Every named-output sentinel preserves its exact frozen bytes.

| Frozen probe | Actual diagnostic | Reached layer and bounded interpretation |
|---|---|---|
| equal_bits | P14-LOOP-PRECISION, status 3, bytes 316..695 | Legal but non-stuttering C-path Change receives precision/profile refusal; no positive Change-C transfer claim |
| loan_escape | P8-EXIT-DEPENDENCY, status 1, bytes 401..470 | Diagnostic directly covers the ending-loan reference escape |
| continue_type | P14-CONTINUE-TYPE, status 1, bytes 387..404 | Diagnostic directly covers exact carry type mismatch |
| continue_arity | P14-CONTINUE-ARITY, status 1, bytes 387..417 | Diagnostic directly covers exact carry count mismatch |
| continue_target | P14-CONTROL-CONTEXT, status 1, bytes 441..461 | Diagnostic directly covers absence of an active lexical loop target across the loan boundary |

The diagnostic source slices were read from the unchanged frozen files and matched their hashes. Thus four focused illegal boundary diagnostics are actually reached during registration. This does not establish arbitrary loan/owner safety. The driver raw conservatively retains `targeted_negative_pass=false` and `UNCLASSIFIED_PENDING_RAW_REVIEW`; this separate post-run classification does not edit it.

The wider V1 stutter question receives CHECK exit 3, P14-LOOP-PRECISION at 46:19–46:34 in every final FULL job. It remains outside the declared one-C/one-B structural profile. Its owned/C17/native stages were NOT_RUN. See [first diagnostic receipts](final/wider-V1-first-diagnostic.json).

## Final PARTIAL limitation

Final PARTIAL job 126236387574 completed normal CHECK, genuine public observations, C17 emission, and unmodified O0/O2 compile/run. Its next command `/usr/bin/clang --version` exceeded the frozen 30-second timeout after 30.029573534999997 seconds: exit=null, timed_out=true. It printed 110 stdout bytes describing Ubuntu Clang 18.1.3 and no stderr. The underlying timeout cause is not inferred.

The normal emitted C is byte-identical across all final FULL jobs and PARTIAL: 3,924 bytes, SHA-256 `fa8f57fd2abaf4939a200aaba1445dd55e0b0da3d70737858053fdfdf1caa04d`. PARTIAL used /usr/bin/gcc, Ubuntu 13.3.0-6ubuntu2~24.04.1 and the same strict C17 flags plus O0/O2. Its two compiles and two unmodified runs exit 0 with no stderr. [Exact evidence binding](final/normal-C-identity-binding.json) preserves each job's complete argv.

PARTIAL AST extraction, passive observations, immediate control, five probes and wider V1 are NOT_RUN. The separate later workflow Production CLI step is SKIPPED. Identical normal C and successful FULL observations do not replace these missing PARTIAL stages. Overall PARTIAL status remains MECHANICAL_OR_OBSERVATION_LIMIT, FIRST type MechanicalFailure. The [original timeout receipt](final/job-126236387574/normal-ast-version.result.json), [stdout](final/job-126236387574/normal-ast-version.stdout) and [complete raw](final/job-126236387574/complete-raw-bundle.json) are retained.

## FIRST and mechanical revisions

The initial completed R commit 6c4e5a57c37aeec8bd3c5f04014a7b7699190671 was created before the first ref, with fixed P as sole parent.

1. The first FAST failed at pinned formatting. Only the exact owned.c two-line wrapping suggestion was applied; new owned SHA-256 e11c9ae4c10e4151d8565300c3b91e3f7833ae34a7e461f2512bd5e436935147. Its full received formatter artifact also contains a foreign tests/p1117/lineage.c section. That body was retained opaquely, not mined or applied; the existing ignored file remained unchanged. Commit 3908666af7cf58041732294827c60b538090b272 changed only owned.c wrapping and the manifest.
2. Every initial PR job reached normal CHECK/public/C17/unmodified O0/O2 before the passive mapper reported “no unique phase-comparison IF.” Actual own emitted C/AST established that comparison/carry ranges ended at standard UINTMAX_C macro expansions; ordering-only lookups had unnecessarily requested both range endpoints. Both lookups now read the direct begin offset, and the compared other operand is narrowly guarded as pure integer/cast syntax. All text-slice/insertion, scope, shadowing, alias and helper guards remain. No guest/source/oracle change occurred. Revised instrument SHA-256 f6f8d46b3a0d08d51161d638941851ff911ddd0737cf28504e6b07c7c154c809. Final commit bc3386f1b710f262e51ebea9a3d05db6064f398b changes only instrument.py and its manifest from 3908666.
3. Final PARTIAL's version-command timeout is retained without a further repair or run.

Initial six-job raw, initial emitted C/AST and FIRST summaries are under [initial/](initial/). All revisions were frozen with new bytes/hashes/reasons before their natural push. These R mechanical failures are not mislabeled as Production defects.

## Raw format and limits

[raw-index.json](raw-index.json) lists every archive payload path, exact UTF-8 size/SHA-256 and Git blob SHA. The index excludes itself to avoid circular hashing; its own hash and archive commit/ref are recorded in the Issue handback.

Each per-job complete-raw-bundle.json preserves the exact R CTest post-test transport records, wire_base64 and transport manifest. Decode base64, then gunzip only codec=gzip+base64; validate wire/plain sizes and SHA-256. Codec=hash-only-executable intentionally contains size/SHA/command evidence without executable bytes. The declared raw_bytes aggregate includes these executable sizes and must not be described as all preserved executable content. Every transported non-executable payload was size/hash checked, including gzip CRC and length. There were no extra source runs to decode or archive this data.

The complete R raw is retained, together with CTest completion metadata. Concrete non-R implementation/fixture/oracle outputs were not adopted as R expectations. Native storage mapping follows this finite emitted-C effect graph; it is not a universal public source-to-C storage certificate. Public checking and CLI emission are separate genuine checks of identical bytes, not the same owned artifact object. Finite traces prove neither all-H induction nor general termination, universal memory safety, broader profile support, or P-owned OOM campaigns.

## Separate value handback

The finite feature behavior above is observed within the declared scope. Comparative UB-prevention audit locality and adoption value were not measured. **UNMEASURED / GO624 PAUSED / H1 HOLD** remains unchanged. C owns any separate bounded P readiness assessment; R does not convert its incomplete PARTIAL matrix into overall PASS.
