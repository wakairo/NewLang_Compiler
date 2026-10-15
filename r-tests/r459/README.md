# R459 fresh BLIND pre-registration (written before first candidate invocation)

Fixed candidate: `b69b6f63c6eb8681ebd468e7988f961b0378f23f` (parent `42cebcc661d24fa1a4bb99d80e54b256490eb2a3`), baseline main `4506112b1dea4a5a63b4ac41ff9e8f8f8388146d`.
Dedicated R branch `r459-initial-some-blind-ci`. No P457/PR458 fixture, source, rationale or CI imported.
Source file contents are frozen by this authored commit's Git blob IDs. The dedicated run prints SHA-256 of each actual source and emitted C.
**Only one R test commit and one newly triggered Actions run**.

Independent oracle before execution:
- Lexical `anchor` is physically the **first original** Node, payload 37, next = None.
- Distinct original lexical `peer` has payload 211; its initial `next=Some(pin)` where `pin` originates only from `loan_read(anchor){|borrowed|ptr_from_ref(borrowed)}`. Physical C pointer must equal **&anchor original**, not a carrier copy or other Node. Both originals must have disjoint physical addresses. No host/C store may repair next.
- `none_control.nl` has both Node next = None, same payloads and two distinct roots.
- Postconstruction read of `peer@next` and match/reloan in the positive case tests Copy and currentness. `anchor@next` read remains None.
- `ref_escape.nl` predicts source/check refusal of returning scoped ref; `stale_root.nl` predicts stale local-ref reacquisition refusal or a conservatively syntax-unsupported spell: **never** treat parser unsupported as checked lifetime refusal. A negative unexpectedly accepted is a finding.
- A public checked/owned observer must consume the compiler's **real source-issued main call** and find its owned body and ptr_from_ref/aggregate/Some evidence; no synthesized manifest.
- Physical native observer uses **read-only GDB** over -g generated C, requiring variable-to-original symbol mapping, separate Option tags, stored ptr equal to exact original address, two payloads. It never mutates generated C or guest memory. Failure to map generated C into trusted original symbols => **observer inconclusive**, not semantic success or native-negative. O2 and sanitizer execution alone cannot replace physical observer.
- `--check` accepted but emitted C refused => backend-unsupported, not native pass. Compiler emission/native semantic divergence => finding if identified; missing observer/debugger/build evidence => inconclusive.

No claimed Product/H1 GO, general heap/multiroot, GC, callbacks, concurrency or original-owner transfer.
Stop on first unsafe acceptance, missing evidence, native mismatch, CI environment issue, or inability to observe. No retests/production edits.
