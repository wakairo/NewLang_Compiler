# R1421 independent canonical oracle v1

## Authority, provenance and scope

Canonical: Compiler main 521a7b10e763e2ef75cd9d8d10b11e569dc2310d; its CURRENT_SPEC pointer selects docs/reference/NewLang_v0_spec_Draft17_38.md. Authority was checked by actual GET before ACTUAL START and again before ACTUAL RESUME. Normative basis: Draft §§3.2,8.1–8.2,10.1–10.3,13.8,14,16.1–16.3,17.1,17.4,18.1,19.1,20.4,26.3,26.9–26.13,27.1 and Process §7. Compass is navigation; Product Strategy §§1–1.1 supplies the value objective, not implementation evidence.

C891 authorized this same successor to resume after exposure to one predecessor's operational STOP metadata: that predecessor had no FIRST/source/oracle or technical prediction/results. The original STOP is preserved. This history is not claimed to be unexposed/pristine. No producer implementation/design/fixture/oracle/results, prior technical R, candidate API, CI or commit message/patch was used to construct these programs. Issue #1421 BODY and C891's individual comment are the only other-actor Issue text read.

This is one canonical-only PREP pass. Three independent complete compilation units below, one oracle, no harness. All compiler/build/CLI/native/Lean/Actions/CI/branch/commit/PR/workflow/Production changes remain 0; FAST 0, FULL/PARTIAL 0. Local work is text formatting and UTF-8/SHA256 calculation only. Future execution mode is GitHub専用ブランチ＋Actions, not the P-equivalent environment. Candidate/API access and execution require a separate C RELEASE. All future API/CLI names, signatures, artifact schemas and validator interfaces are UNSPECIFIED here.

## Frozen inputs and exact target operations

Each source is UTF-8 without BOM, LF only, four-space indentation, with exactly one terminal LF. Each filename denotes a separate semantic unit; do not concatenate the units. Hashes cover the source bytes only, excluding Markdown fences and labels. Byte offsets are zero-based, half-open; line and column positions are one-based, end column exclusive.

| Source | Bytes | SHA256 | Target span |
| --- | ---: | --- | --- |
| r1421_author_route_v1.nl | 1829 | 8dfee230ee73d0d751755d65016799e340e5d91aa20e8fee357fe361473eddba | bytes [1198,1244), line 34 columns [32,78) |
| r1421_wrong_domain_v1.nl | 1443 | dccd19e56f371970e4be9a51e391b4b11c778d0aa798b62c9d5aa48e8fb04f0f | bytes [762,814), line 23 columns [33,85) |
| r1421_ended_original_v1.nl | 1330 | 33633b2261204d8492946667213280e598ea61015ce5dee458c800a0c9b6bf70 | bytes [818,862), line 25 columns [33,77) |

The exact target expressions are respectively:
- unchecked ref_from_ptr(read, selected, stable)
- unchecked ref_from_ptr(read, origin, foreign_stable)
- unchecked ref_from_ptr(read, origin, stable)

These labels and classifications belong to this oracle; they are not invented compiler API or artifact field names.

## Independent question and non-assumptions

A true original-origin/current-liveness/governing-D relation can remain true even if a particular checker loses precision across a Copy link read and match join. Can a source-local unchecked acquisition preserve that distinction in public evidence while retaining the real source-issued owner and stability? Conversely, will a genuine live stability ref to a different D, or a stability ref to D after O has ended, be rejected without manufacturing a Known fact or a new original?

No program is asserted in advance to defeat every possible machine proof. The positive has a source-level proof; an implementation may retain enough facts to prove it completely. A machine-proved acceptance is useful control evidence, but does not exercise the author-assumed seam. An author-qualified acceptance, a machine-proved acceptance, a precision refusal, source-profile refusal, earlier rejection and absence of public observation are distinct outcomes. The modifier's spelling alone is not proof that the compiler delegated a condition to the author.

## P: true premise, local author, ordinary control

r1421_author_route_v1.nl contains one fallible allocation, one real Allocation/Backing/Storage chain, one fresh Domain D and one initialized original O. The mark byte 173 is not an identity oracle and is not read as proof. Allocation failure returns 0 before any caller-owned backing, Domain or root exists.

On allocation success, into_slot transfers the real raw claim. initialize consumes that slot, creates the single original O under D, and returns origin. The initialization loan ends without ending O. No host-seeded owner, forged ptr, tuple surrogate, future-incarnation token or synthetic Domain is used.

Under a fresh live ordinary loan of the same D, the unmodified write acquisition and replace set O.relay to Some(origin). This is an explicit post-initialization self-link fixup; it does not mint a pointer before lifetime start. replace returns an old Discardable Option<ptr<LedgerRoot>>. O and its fixed relay incarnation remain live; no EndRoot occurs.

The unmodified baseline read acquisition at line 24 and the actual read at line 25 are ordinary controls. They remain machine-check obligations. The Copy route and exhaustive match return origin in the None arm or the copied link pointer in the Some arm. In the actual success trace the stored link is Some(origin), with no intervening mutation. Both the source-established active path and the fallback expression identify the same live O. The target unchecked acquisition is the sole author-qualified operation; it does not qualify the earlier write, baseline read, field reads, cleanup or enclosing function.

At the target, the backing is live/readable, O's initialized representation/range/alignment/access derive from its real allocation and initialization, current provenance points to O, O is governed by D, and stable is a live ordinary ref to that same D. The target's generated ref and its relay projection are confined to the loan. The pointer copy by itself is not ownership or stability evidence. No ref or claim escapes.

The actual relay read through promised must observe Some, so the success branch result is 87. The source includes a total None arm returning 11, but 11 is not an allowed successful execution result for this exact frozen source. Scope exit must validate the actual resulting package; scalar type alone is not a waiver of hidden dependency rules. The returned branch literals themselves introduce no ref dependency.

After the read loan ends, destroy consumes O's lifetime under the original D's exclusive ending authority, returning the original empty slot. erase_slot recovers its full raw range; finalize_domain consumes D; deallocate consumes the matching original Allocation and raw claim exactly once. The self pointer and remaining Copy ptr tokens are not blocking owner claims. Failure and success require no ordinary caller's hidden lifetime proof. author_route has no parameters, exports no root/ref/owner and invokes no callback/FFI; the local author controls the sole relay writer and the complete allocation/normal/teardown sequence.

Predicted values, if a separately authorized execution is reached: genuine allocation failure = 0, genuine success = 87. No native execution or forced-failure experiment is authorized or claimed here.

## N1: genuine stability, wrong Domain

r1421_wrong_domain_v1.nl creates O under fresh D, plus a distinct fresh other_domain E. E is real and live, and foreign_stable is a real live ordinary ref to E. Therefore this is not a missing-token case. At the target, origin identifies the current live O but O is governed by D, not E. Allocation freshness/domain creation and initialize establish the mismatch directly.

The exact target must not yield a valid ref or executable owned artifact by treating wrong-D as an author assumption. The unchecked prefix neither changes the governing identity nor makes E a stability witness for O. Neither Domain is a numeric address or interchangeable merely because the same nominal type is used.

The complete source contains the normal failure arm and textual cleanup for E, O/D and the original backing. That trailing cleanup is source completeness, not a claim that semantic checking or execution can lawfully pass the target. The constants 19 and 23 are not valid native oracles: the intended false-premise path must never be sent to native, even if a compiler accepts it.

## N2: live backing and genuine D stability, ended original

r1421_ended_original_v1.nl first issues real O under D and then ends O with destroy using D's exclusive authority. At the target, Allocation and the same BackingRegion still exist, the empty slot is retained, D is live, and a fresh ordinary stable loan of D exists. Only the original incarnation's current liveness has deliberately been made false. No deallocation, same-address reuse, new initialization, missing owner or missing stability is needed to explain the failure.

The persistent origin token may remain after destroy, but cannot revive O. An empty slot and still-live backing are not a live LedgerRoot. The exact unchecked acquisition must not issue a ref, convert EndRoot into a fresh Known current original, or erase the source obligation. The later erase_slot/finalize_domain/deallocate text closes the explicit responsibilities if viewed as source structure; it is not executable evidence past the rejected acquisition. Constants 29 and 31 are not native targets. This false-premise case is permanently excluded from native execution.

## Preregistered stage ledger

| Stage | P: author route | N1: wrong D | N2: ended original |
| --- | --- | --- | --- |
| SOURCE | Canonical shape expected eligible: one exact recursive H, one allocation site, existing link ref read/write, ordinary function, Copy match join and exact contextual read prefix. Candidate acceptance is not assumed. | Same source categories; correct token mode/arity and actual typed operands. Wrong D is a semantic premise failure, not a missing-stable syntax case. | Same source categories; retained ptr after EndRoot is itself legal, with a genuine fresh stability loan. |
| Definition registration | Nominal completion and function signature collection are source-defined; body/whole-unit semantic success remains conditional on actual checking. A parse/header registration is not CHECK. | Declaration shape/signature may be collectible; the bad function body must not become an accepted safe definition. Transactional registration may reject/rollback the whole unit. | Same distinction; no partial header/definition residue is a valid accepted artifact. |
| actual CHECK | NOT RUN. Conditional permissible outcomes: demonstrable full machine proof; honest author-qualified checking; or fail-closed source/precision/representation refusal. No acceptance requirement is invented. | NOT RUN. At the intended operation, proven D != E must reject. Earlier source/precision rejection records the actual layer and target NOT_REACHED. | NOT RUN. At the intended operation, known ended O must reject. Earlier source/precision rejection records the actual layer and target NOT_REACHED. |
| Public owned / classification | NOT OBSERVED; interface UNSPECIFIED. If accepted, require genuine source-issued original/owner/Domain/loan evidence. Separate machine-proved from delegated author conditions. CHECK without observable validated public evidence is incomplete evidence. | No valid accepted ref/owned artifact from the target. A parse or definition-registration receipt is not semantic acceptance. | No valid accepted ref/owned artifact from the target, no resurrection/new issuer. |
| Refusal location | Record exact first rejected construct, source span, phase and stated reason. Refusal before line 34 does not test the unchecked seam. Refusal at line 34 may be sound lack of precision or representability, not a false-premise finding. | Distinguish the target D mismatch from an earlier grammar/profile/body/loan failure. Any earlier failure leaves intended wrong-D rejection NOT_REACHED. | Distinguish target ended-incarnation failure from an earlier grammar/profile/owner/scope failure. Any earlier failure leaves intended EndRoot rejection NOT_REACHED. |
| Native / later layers | NOT_RUN by PREP; future permission and true public-owned validation required. Planned value oracle is 0/87 only. | NEVER_NATIVE for the intentionally false premise, even after an erroneous CHECK accept. | NEVER_NATIVE for the intentionally false premise, even after an erroneous CHECK accept. |

A conservative rejection is not proof of a language defect, nor evidence that the intended negative condition was checked. Conversely an accepted complete semantic CHECK at a known-false target is not repaired by calling it a harness issue, executing only an OOM branch or suppressing the input.

## Public evidence honesty and the author/machine boundary

For any future accepted P artifact, the consumer must be able to distinguish:
1. real source-issued Allocation/Backing/Domain/original/loan and their lifecycle;
2. the exact frozen source operation/span;
3. each operation precondition actually machine-established;
4. each precondition, if any, entrusted to the author;
5. current scope/hidden dependency obligations and the unchanged normal effects/cleanup.

These are semantic observations, not proposed API fields. If a precondition is delegated, §8.2 requires its exact source operation/span and author-assumed condition to remain identifiable in accepted evidence. Calling a delegated fact ordinary machine-proved Known, minting an original/issuer absent from the source, transferring hidden proof to the ordinary caller, or losing the condition through public-owned conversion is not an acceptable author-qualified result. If the checker/owned/validator/backend cannot represent that distinction soundly, fail closed.

If the implementation genuinely proves every precondition for P, record MACHINE_PROVED and AUTHOR_ASSUMPTION_SEAM_NOT_REACHED. An empty assumption list alone does not establish that a machine proof exists. If public evidence cannot expose enough to decide, record INCONCLUSIVE / NOT_OBSERVED and the missing observation; do not infer proof from a scalar result, CHECK0, field count, CI green, a bare accepted boolean, source parsing, or host oracle construction.

No observer may construct guest provenance/owners/stability, choose a replacement original based on equal bytes, hide a source obligation or repair the compiler. Any future mechanical source spelling repair must preserve V1 and its original predictions/results and use a separately identified V2 with new bytes/SHA and pre-result rationale under the applicable C release. This PREP authorizes no such later trial now.

## Explicit finite coverage and omitted edges

Directly prepared: one source-owned true-premise route with an ordinary control, one machine-known wrong-D foil, one machine-known ended-original foil, complete allocation failure and textual owner/teardown paths.

Missing stable, wrong arity/read mode, expired stable, escaped ref, same-address reincarnation, foreign backing, extra roots, general traversal, reentrancy, FFI and broad ordinary-safe wrapper APIs are not additional prepared programs and are NOT TESTED. Their normative categories remain distinct: omitted/ill-shaped stable is a source-form error; an expired/out-of-scope stable cannot be supplied as an existing live lexical ref and may reject at its escape/binding before any target; a live wrong-D ref is N1; a live same-D ref without live O is N2. None can be made valid by the prefix. No coverage or universal soundness is inferred beyond the three frozen units.

## Stop, interpretation and value

FIRST is an immutable prediction record, not a PASS. If authority changes, technical producer information enters, material normative ambiguity appears, or future semantic/owned/provenance/native evidence contradicts this oracle, stop and return to C with original bytes/logs intact. Do not silently replace the source, broaden the profile, change the owner identity, relax a validator, relabel a premature refusal as the target negative PASS, or add trials.

The useful capability under examination is honest localization of a true author obligation while preserving real ownership and protecting the ordinary safe caller. The audit locus for P includes the allocation/initialization/fixup/link writer/control join/read/teardown dependency closure and the trusted builtin contracts, not only the unchecked line. This small synthetic function does not establish an important real-world C-rational workload or measure audit reduction against factored C. There is no new goal-deviation evidence; implementation evidence and product value remain separate. Author+ordinary-safe-caller audit/re-audit improvement is UNMEASURED. Product GO PAUSED / H1 HOLD.
