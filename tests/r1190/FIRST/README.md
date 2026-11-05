# R1190 / C248 — FIRST pure-text prefreeze report

## Result and authorization

**Technical result: NOT_RUN.** This is a recoverable pre-execution source/oracle freeze for the *next* candidate. It is not R1185's result and does not validate a new P implementation.

Authority reference: main `63658cd546be8b99599baf19476486a475adb320`, tree `a376e3c6e111f7283daa8be82d766dc0282c16b2`; CURRENT_SPEC selects **Draft17.34**. The tree identifier is the adopted authority stated in Issue1190, not a fresh execution claim. Authority document blob IDs and pinned URLs are in `authority/SOURCES.json`.

actual START: https://github.com/wakairo/NewLang_Compiler/issues/1190#issuecomment-6658789184

**実測環境はGitHub専用ブランチ＋Actions、Pと同等のlocal実行環境は禁止。** This prefreeze consumes compiler/build/native/new-AST/branch/ref/push/PR/Actions/CI **0**. No observer or runner was implemented, no new P was started and no Productioncaptain role was assumed.

## Packet and finite budget

- `sources/*.nl`: thirteen complete physical source inputs, each one semantic unit.
- `oracle.json`: hand-authored exact numeric rows, per-case legality, canonical references and predicted first gate.
- `COVERAGE.md`: finite coverage mapping and exclusions.
- `authority/reasoning-ledger.md`, `authority/canonical-excerpts.md`, `authority/SOURCES.json`: the derivation and immutable authority provenance.
- `records/issue1190-release.md`, `records/read-log.md`: release scope and actual read/exposure history.
- `FUTURE_OBSERVATION.md`: conditional future measurement plan and limits; no implementation.
- `RESTART_PROMPT.md`: short restart instructions carrying the environment restriction.
- `manifest.json` / `SHA256SUMS`: byte-level restoration guards, generated after the text packet is complete.

Budget used: **positive 5/6; direct control 1/1; legal coverage probes 3 + true negatives 4 = 7/8.** No full cross-product is claimed.

## Independent questions and oracle

I means literal-origin usize to loop initializer, A means actual iteration phase to helper actual, F means same-call fresh usize formal to the comparison's left operand. Counts are *additional identifier Copy links*, excluding ordinary receiving at initializer/call. Every scalar link retains the value and type; its receiving binding has its own lexical identity.

| Case | I/A/F | Phase sequence (last is Break) | Helper normal u8 sequence | old / old_copy sequence | Post selected / sibling | Break / outside Copy |
|---|---|---|---|---|---|---|
| C0 | 0/0/0 | 2,5,8 | 21,46 | 33,21 | 46 / 157 | 8 / 8 |
| P1 | 1/0/0 | 42 | none | none | 59 / 181 | 42 / 42 |
| P2 | 2/0/0 | 9,12 | 73 | 41 | 73 / 193 | 12 / 12 |
| P3 | 0/2/0 | 4,7,10,13 | 61,89,89 | 25,61,89 | 89 / 163 | 13 / 13 |
| P4 | 0/0/2 | 6,8,10,12,14 | 82,82,117,82 | 39,82,82,117 | 82 / 179 | 14 / 14 |
| P5 | 0/1/1 | 1,3,5,7 | 94,94,132 | 57,94,94 | 132 / 199 | 7 / 7 |

The complete oracle additionally records every A/F Copy value, fresh formal, guard and helper branch result, entry Copy, selected before/after, Continue outgoing and terminal bindings. Helper normal also determines caller `from_call` and its ordinary u8 Copy `replacement`.

P1 executes zero active writer/helper iterations: its first header directly Breaks. P2 has one backedge and uses `orbit` as an outer seed name and a fresh loop parameter name. Its `seed_after` is **9**, while Break is **12**. P3 must use successive actual phases, not entry phase 4. P4's EQ is false, false, true, false across distinct calls; a stale formal from the first call cannot yield that sequence. P5 is the only mixed-boundary positive: one A link and one F link, with the same spelling `ticket` across the caller/callee lexical boundary.

Each original T has exactly two core-u8 fields. Selected declaration index 0 and 1 are both used. Named initializer order is reversed relative to field declaration, so field identity must follow the label and declaration index, not initializer position. Each selected write is one syntactic scoped replace site repeated only by the loop. The root, selected child and sibling incarnations are preserved by Change. The sibling and pre-loop entry Copy stay numerically unchanged. The old package leaves the loan as dependency-free u8 and its ordinary Copy retains the old value.

These are mathematical predictions written before execution. Local scripts only wrote literal text/JSON and checked file shape; no guest evaluator produced the rows.

## Legal scope-out probes

| Case | Outside increment | Conditional canonical phases | Conditional helper outputs | old sequence | Post selected/sibling; Break |
|---|---|---|---|---|---|
| X1 | I third identifier link | 3,5,7 | 62,91 | 47,62 | 91/167; 7 |
| X2 | A third identifier link | 2,5,8 | 76,124 | 52,76 | 124/173; 8 |
| X3 | F third identifier link | 1,3,5,7 | 98,98,136 | 45,98,98 | 136/187; 7 |

Canonical ordinary Copy has no language-level two-link cap. These are **legal coverage probes**, outside C248's selected increment. Predicted conservative boundary is the first bounded alias/semantic-analysis or owned-profile gate that cannot support the extra link; exact compiler stage/diagnostic is unknown because implementation was not read. A safe precision rejection here is not a safety defect. If accepted through genuine supported paths, the conditional rows remain the predicted numeric behavior. This does not authorize forcing an unsupported source into private APIs or native execution.

## True invalid sources and first-rejection hypotheses

| Case | Canonical invalidity | Predicted first rejecting layer | What is not established by that rejection |
|---|---|---|---|
| N1 | two ordinary Copies retain u8, but helper formal requires usize; no explicit conversion | semantic call argument/static-type compatibility | later phase alias, provenance, loan, owned and native checks |
| N2 | two formal Copies retain usize, used directly as IF condition; bool is required | helper definition-time condition typing | any successful call, IF branch execution or caller mutation |
| N3 | usize loop header receives u8 on Continue; exact type equality is mandatory | semantic Continue/header type compatibility | later header recurrence, owned or native correctness |
| N4 | helper normal arms are u8 and usize; normal IF types must agree exactly | helper definition-time normal IF type join | helper normal-return composition, caller write or native behavior |

There is **no legal runtime trace** for N1–N4. Do not present a hypothetical prefix as executed or as evidence that a downstream checker was reached. The oracle records these static expectations separately from numeric traces.

## First-arrival and evidence boundaries

For C0/P1–P5 the canonical source/type/flow is eligible under the selected finite profile. The first delta-sensitive hypothesis is semantic fact propagation across the chosen ordinary usize Copy boundary. A candidate that actually declares the selected support should carry these facts faithfully. This prefreeze does not know the unpublished candidate's diagnostic names or actual accepted envelope.

Canonical legality, current parser/semantic acceptance, genuine source-issued public owned applicability, C17 eligibility and native behavior are distinct. A canonical-legal source may be conservatively precision-rejected; that must be recorded at the actual first layer. An explicit promised supported case that does not reach its declared gate is a missed capability, not silently reclassified. A source may have no applicable public owned API for its entire loop; record **NO_APPLICABLE_PUBLIC_API / NOT_RUN** rather than forging evidence.

No measured SOURCE/CHECK/OWNED/C17/NATIVE/F/PASS claim is made. Static inspection is limited to authored source spelling/structure and hand reasoning.

## Static inspection performed

Each file has one T declaration, two ordinary functions (main and one helper), one loop, one loan_write/replace, one Continue and one Break syntactic site. Helper result is normal tail IF; no explicit helper RETURN. In positive/control/coverage cases the outer guard uses the actual loop parameter directly, RHS is a typed literal, and Continue is direct phase plus a positive typed constant. No guard/ADD/RHS alias was added.

Static file checks confirmed balanced delimiter counts, LF/trailing newline, those literal syntactic site counts, seventeen fields per oracle row, and at most five rows per legal case. Human review checked fixed field choice, helper argument/formal relation, selected arm, old/current order, invariant sibling and entry values, and Break/post values. This is not parser/type/native validation. No source or oracle revision was made after creation; FIRST is the original packet.

## Read history and independence

The source/oracle/runner/report bodies of P/F and other R were not read. Scope was own Issue and required authority documents. No agent listing, broad Issue search or other-agent directory scanning occurred.

Incidental exposure was reported before source creation: a too-wide Value Strategy §1 extraction displayed its §1.4 historical high-level summary, CURRENT_SPEC and relevant canonical passages included historical implementation summaries, and one docs metadata listing for Review Guidelines included unrelated filenames/blob IDs. No linked producer report/source/oracle was opened. The historical summary had counts/status, so an initial “非数値” description was corrected to “non-oracle high-level historical summary.” Full detail is in read-log. The numbers here were chosen independently from the canonical laws. Do not represent this history as absolute information isolation or make a stricter BLIND label than C adjudicates.

## Remaining questions, investment limits and STOP

There is **no unresolved canonical question for these finite authored cases**. Exact unpublished implementation reach, target-specific public API availability and future observation capability are unknown and require C's later release/allowlist. Future tool/mechanical source corrections must preserve FIRST, reason and new hash; a substantive source/oracle/scope change or unexpected exposure is affected STOP→C.

The value of this packet is a bounded independent check of ordinary scalar flow across existing language constructs. It does not justify accumulating a separate general symbolic executor or expected-value-driven observer. No extra cases or framework were added after the listed finite questions were covered.

Future evidence PR must be DRAFT/NEVERMERGE. No default FAST allowance is assumed for the later phase; only C's explicit new Issue release budget applies. Product conclusions remain **audit locality UNMEASURED / GO624 PAUSED / H1#268 HOLD / P1036–PR1050 STOP**.
