# P315 — one cross-root A.next Change after two original-B named helpers

**UNSELECTED EXPERIMENT / DRAFT DO NOT MERGE / SOURCE→OWNED ONLY.**

## Fixed authorities and historical gates A–C

Compiler/main a5343bf70004b30a8b026e52aa2fde93b5920be0, canonical Draft17.30 selected in docs/reference/CURRENT_SPEC.md, FormalProof/main a8de4ebb4099f4742fc161d26867740dcac48414 (F59 proof-only). Frozen P experimental parent PR313 f6034297edababdf37b90ddfd1dd083e31804b80; PR309 and unrelated PR298 remain unmodified. Proposed Draft17.31/DI015 NOT selected.

Read Development Process §§4.1.1/4.2/6/7/9/10, Design Decision Procedure Gates A–C, Design-Intent Ledger DI001/006/007/009/010/011–014, proposed DI015, prior M266/M290/M305, P306/P308/P312, R307/R310/R311/R314 and prior product H1 issue268.

| Canonical, historical alternative / classification | Decision / no-foreclosure |
|---|---|
| Draft17.30 §3.2b and DI014 (normative ADOPTED/BOUNDED) | KEEP five original separate fallible roots and exactly six initial links, actual per-field ProjectionIds and original 0–5 release obligations |
| DI006/007/010 source loan/ref/pointer grammar (bounded plus Deferred) | KEEP explicit A's original D_A loan, mode-preserving ref_from_ptr(write,ptr_A,stable_A) field; DEFER general ref/ptr field grammar, final unified loan modes, alias registry; Copy ptr is locator, not ownership |
| DI011 known-call / P312-R314 experimental two B helper calls | KEEP actual named two-formal body-sensitive caller→callee Change for B.prev and B.next; this A.next change is a DIRECT replace, not a new named call. DEFER arbitrary N-call effects |
| DI009 original allocator/DI012–013 nonCopy custody (normative bounded) | KEEP original B Allocation/Domain through ordinary complete Holder whole value, manually matched unique release; DEFER true returned TreeBOwner(K_dst,K_B), partial nonCopy convenience, RAII and native backend |
| Draft17.31/DI015 proposed, F59 proof-only | DEFER: no F0/F1/F2 registry, new normative source rules, or silent adoption |

INTENTIONALLY REPLACE no selected canonical rule. This candidate only extends an unselected DEFAULT-OFF compiler finite certificate, not general NewLang semantics. Some old SurfaceDraft chats are not exhaustively available; absence is not proof of no historical alternatives. Ledger/CURRENT_SPEC unmodified.

## Actual fixed-PR313 first refusal

Source SHA256 7d160c7f6beb6a932fd8026257c7092f0e322c7c9d65314e41f8bd1f9de6dc83, 16456 bytes, adds five lines and one D_A-scoped ref direct A.next Some(B)→Some(C) after two B helper calls to original P312 source SHA256 40b5dad768a0c265e85bd111e9aca7a9160c325e04bf96f249767896b616f73e. On immutable PR313 and all previous eight flags ON, [Actions 38138576073](https://github.com/wakairo/NewLang_Compiler/actions/runs/38138576073): source parse0/register3, CLI exit3, CAPTURED-CLOSURE-PRECISION at 211:49–212:84 / bytes 13369–13482, public snapshot unchanged and no owned artifact. Source existing two named calls accepted, CLI4/backend unsupported. Mismatched B-Domain loan for A rejects genuine semantic ALLOCATED-DOMAIN-MISMATCH at 210:71–210:114, rollback. The exact first barrier was source captured-change array/guard limit6 in captured_closure.c/semantic_internal.h, not a canonical semantic contradiction.

## Ninth flag minimal candidate

- NEWLANG_EXPERIMENTAL_CROSS_ROOT_A_NEXT_AFTER_TWO_CALLS defaults OFF and requires prior eight opt-ins.
- Capture seventh actual directly source-backed original A.next Change only under ninth flag. Existing public captured_change_view index6 exposes its real before/after semantic worlds, while independent B.prev and B.next named call public views/validators remain unchanged.
- Exact seventh certificate guard checks A's original D_A/governing root, old Some(original B)→fresh Some(original C), all original B/C live/current, preceding two B named call certificates valid, their source order and post→seventh-pre root/field facts for A/B/C consistent, B.prev/B.next None, A.prev C and C.prev B preserved, dst.child None, no Copy authority. Ordinary change_validate rederives scope/dependency/Change/Reset/Option old-occurrence ending. An eighth direct change/third named call still not admitted.
- No grammar, general effect/borrow registry, extra physical H, new source-visible owner rule, native backend or cleanup revision.

## Empirical protocol / reporting boundaries

Evidence-only immutable baseline branch p315-fixed-baseline-evidence. Source generation and fixed/new CLI/public registration: evidence/p315/probe_candidate.py. Public C observer evidence/p315/public_seventh_observer.c reuses the existing public API and checks actual nonNULL source-produced owned main, five captured closure validators, both named B scoped validators, original A/B/C root/Domain/Allocation/currentness, old A.next Some(B) payload ended/new Some(C) live, B.prev/B.next/child and A.prev/A.child/C.prev unchanged, five matched original static release paths. Fourteen invalid inputs plus same-context successful retry in evidence/p315/probe_negatives.py distinguish semantic rejection from finite precision/unsupported.

CLI exit4 V1-BACKEND-UNSUPPORTED means zero NewLang-generated C bytes. No native C heap/failure-world execution, full H1 4 detach+2 adopt, TreeBOwner join, donor/receiver ordering, ≥5/7 semantic bugclass PASS, topology≥5/6 or human-value product PASS. H1 remains PASS/FAIL UNDECIDED. This experimental candidate awaits independent Coordination/R and explicit gates; no merge authorized.
