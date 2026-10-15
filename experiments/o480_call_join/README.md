# Track O — O482 finite call-aware two-normal Option<ptr<Node>> join

**Verdict: `O HYPOTHESIS / COUNTEREXAMPLE (LIMITED)`; provisional O/H research, not source/proof authority.** Bounded scope of NewLang_Compiler Issue #482. No Production/Canonical/FormalProof changes, no follow-up tracks.

## Authority and orientation

Fixed Compiler/main `d6e603e12591c2ff39e3e1b77c784681fec66033`, `docs/reference/CURRENT_SPEC.md` → Draft17.30, spec blob `db8fe08f811acc8286f2d37e9d3229b5d7a67067`. Read `docs/NewLang_Track_O_Research_Charter.md` and the Project Compass; product goal is preserving low-level C-class topology while reducing human UB-prevention audit/re-audit closure (Product Strategy §1.1). This **small reversible analysis experiment** is not Product GO. P478 PR479 frozen head `03b1751b513e7aef2d7ba193eda671a556f54b47`: actual named-source CHECK stops at P9 two-normal join; no owned/C17/native named target result exists. Old O5 PR357 was read as historical evidence, **not copied as checker code or truth**.

Read first the unmodified `PREFLIGHT.md` (SHA-256 in `manifest.json`). The design and positive/negative expectations were frozen **before code/experiments**. Synthetic `Original`, `D`, `Allocation`, ref, checked world, mapping, returned-old tokens are hypotheses, not C-issued root rights, certified proof or safe API.

## Reproduce (Python >=3.12, standard library only)

```sh
cd experiments/o480_call_join
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -p test_o482.py -v
PYTHONDONTWRITEBYTECODE=1 python3 run_study.py > rerun.json
python3 -c 'import json; r=json.load(open("rerun.json")); print(r["unit_test_methods"],r["matrix_size"],r["matrix_statuses"],r["unsafe_concrete_reject_abstract_accept"])'
```

Run from this directory if unpacked separately (the `cd` line then is unnecessary). `results.json` records actual full deterministic case/seed hashes and 51 repetition timings (timings/host-dependent bytes vary when rerun). No local or GitHub Production C compiler was run and no LLVM/toolchain downloads or CI retries were performed. The untouched original input `PREFLIGHT.md` and `manifest.json` allow verification of all files.

## Neutral IR and independently implemented semantics

Exactly two **distinct lexical original** Node roots `head` and `tail`, each with separate `region/root/incarnation/Allocation/Domain/owner`, stable four fixed fields `next/prev/child: Option<ptr<Node>>`, `payload:u8`. `head.next` produces an abstract guard Copy Option: on the None normal arm it is None; on the Some normal arm it is Some(ptr to the original tail) and binds `q`. Thus the reference enumerates TWO **different real branch hypotheses**; it does not treat a concrete known Some as having a physically executable None branch. The same original tail always starts with None or an explicitly chosen old Some and the Some arm performs one scoped whole `replace` of `tail.next`.

Known direct named `set_link(field: ref<write,Option<ptr<Node>>>, next: Option<ptr<Node>>) -> Option<ptr<Node>>` conceptually performs body-visible `replace(field,next)`. The single writer's formal `field` → actual `tail.next`, Copy `next` → matched `q`, stability original `D.tail` are mapped explicitly; the bounded loan is a correct-domain READ stability proof plus separately derived WRITE field ref, nonexclusive and nonescaping. No Copy pointer can supply Allocation/Domain/write grant. Whole replace changes **field ValueFact AND containing root ValueFact**, ends old field occurrence, creates fresh Some occurrence EVEN if pointer locator remains equal; the original root/place/incarnation/A/D/owner and 3 siblings remain unchanged. Old Copy Option returned as a separate return-carrier value; it is not asserted to keep an invalid old fixed-field occurrence live. The None arm does not write. The two outcomes are maintained as normal alternatives, not a claimed single runtime state.

The Python files have NO dependency/import calls between the three verdict functions (unit tests verify imports and monkeypatch opposite checker to raise):

- `concrete.py`: finite two-world **reference interpreter**, separate procedural body semantics; ignores budget/claimed effect-summary optimizations, so it can accept cases where an abstract checker correctly reports `PRECISION`.
- `snapshot.py` (**A**): copies whole typed 8-field+2-parent-fact semantic state on a named/inline replace; caller actuals build a callee formal environment before transfer, checks scope, returned-old, binding/field/identity, surviving dependencies, exact old/new occurrence, and every normal alternative before safe continuation.
- `delta.py` (**B**): independently checks mapped relative-place effect atoms, creates one immutable local `Change` record containing `('Change','Reset')` with old/new Option and field/parent facts, old/new occurrence, concrete scoped D, source operation ID, explicit Copy/owner/field dependencies and returned-old value. It changes **two** semantic locations in the abstract frontier (fixed field + parent root), while keeping original identity and disjoint siblings in the base. For side-by-side corpus comparison only, the driver materializes full successor worlds at the end; this **has real Python time/memory costs** and is not counted as persistent delta's saved mutations. B never imports A's acceptance.

**Important alias distinction:** two *distinct formals* mapping to the same actual place is not intrinsically illegal under Draft §13.5c; this experiment allows an additional inert alias formal when the only known effect uses one write-ref and the rest do not invalidate live dependency. The **same formal** appearing twice with inconsistent actual mapping is rejected, as is a false root-to-formal binding. No general alias/borrow permission theorem is claimed.

## Minimal source-like counterexample and precision result

```text
Original head, tail live with distinct R/O/inc/A/D; old tail.next = Some(ptr(tail))
match observed_copy_of_head_next {
    None => { unit }                           // old fixed-field facts F0/RF0 unchanged
    Some(q) => {
        old = set_link(tail@next, Some(q))     // formals: wref→tail.next, next→Copy q
        unit                                    // returned old Copy Option observed locally
    }
}
// p = a live dependent ref/package tied to the old tail.next ValueFact F0,
// or to its containing root ValueFact RF0, surviving across match/call
```

Even when `tail.next` has *exactly the same Copy pointer bits* on both sides, Some-arm whole replace makes fresh `F1`, `Occurrence1`, and `RF1`; None-arm retains `F0`, `Occurrence0`, `RF0`. **A naive locator-equality join would falsely ACCEPT the old dependent package.** Our reference and BOTH independent analyses `SEMANTIC_REJECT` it. Conversely, without surviving old deps, a later `write` on the same live original can be accepted in this bounded model after checking **both** alternatives with correct D/read stability; `read_some` over a None-vs-Some mixed joined state is `PRECISION`, not guessed Some.

Missing `Reset` in a purported summary is a **precision gap**, not proof that the underlying concrete source body is invalid: the independently interpreted body may still be legal, but A/B cannot accept the incomplete summary. An unresolved effect likewise stays `PRECISION`. Duplicate root/allocation/domain, stale ref/scope, wrong actual/formal, fabricated returned-old, unrecorded sibling and surviving old fact are refused. Unmodeled third allocated root or runtime/FFI is `UNSUPPORTED`.

## Measured finite matrix

From Python 3.13.5, deterministic seed `48220261015`:

| Cases | Result |
| --- | --- |
| 40 named finite matrix cases | `ACCEPT 10` / `SEMANTIC_REJECT 23` / `PRECISION 4` / `UNSUPPORTED 3` |
| 320 deterministic seeded repeated selections | all expected; 0 observed reference safety reject + abstract ACCEPT |
| 96 deterministic formal-map order permutations | all accepted with exactly same successor worlds |
| Independent unittest methods | 7/7 pass; AST + mock non-delegation |
| Correct named call / direct inline | both models ACCEPT and agree on the TWO successor worlds |
| Cap=1 vs two states | abstract PRECISION / reference concrete ACCEPT |
| Claimed summary Change without Reset | abstract PRECISION / concrete body reference ACCEPT |

**State size (modeled structural counts, not native bytes):** input branch snapshots: `2 × (8 typed fields + 2 root facts) = 20` semantic entries; A clones **10** entries for one Some-arm Change and retains **20** successor semantic entries, with a separate returned-old package. B retains two input bases (20 entries) plus one immutable Change/Reset delta **2 updated semantic entries** plus effect/dependency/old-return metadata. `delta.py` *diagnostically* reconstructs two full output states as well, so peak Python allocations and 51-run wall time are not reliably lower. There is no evidence that B is faster in Production C. For the exact host-dependent medians and peaks see `results.json`, recorded with `tracemalloc` active: on the named case both are roughly 0.23ms in Python 3.13.5; observed peaks are roughly 8.6–8.9KiB, often higher for B in this debug-mode implementation. Complexity for fixed 2 branches and 1 field mutation: A O(2F) field copying; B O(F + E) base + sparse E=1 recorded change (with the full output materialization still O(F) per alternative). Broader CFG/callee/alias/owner logic is deliberately not modeled.

## Narrow transferable proposal to Track C / P decision maker

**Potential C representation, not approved implementation**: a typed call effect application record `(source-owned arm/node, formal-place→actual-original scoped ref, original R/O/inc/A/D, stable scope, operand Copy q with provenance, old returned Option package, field Change ValueFact, Reset old/new occurrence, containing root ValueFact, affected sibling mask, dependency closure)` applied strictly in source-order against the actual caller world. At the two-normal join keep 2 correlated alternatives or a conservative MAY/Unknown state; do not import branch-private fact IDs as if public origins, map copy ptr to D/Allocation authority, drop Reset from Change, or clear surviving dependency. Retain exact original root, scope and nonCopy owner signatures. On nonrepresentable formal alias/unknown summary/extra root, **PRECISION/UNSUPPORTED**; on proven conflicting surviving deps, **SEMANTIC_REJECT**. Atomic rollback on semantic/resource/OOM must be tested separately by P.

**First independent real C falsifier for any future approved P slice**: keep P471 actual original head/tail and frozen named P475-style body; in two NORMAL arms only Some branch calls real body-visible `set_link(tail@next, Some(q))` with correctly bound q; require source-owned checked artifact + public call/body/arm validator (not only `--check`), track returned old None/Some, fresh field/root facts and occurrence, unchanged `tail.prev/child/payload`, correct D-ref/scope, wrong-root and stale ref/Domain negatives, and actual native only if reached. The reported P478 P9 stop remains a **real precision boundary**, not an observed named-call success. Track C chooses whether to launch P; no other Track is started by O.

## Out of scope / stop

No independent heap issuance, Node layout/ABI, runtime native, FFI, general caller-named owner transfer, recursive/unknown body, more than two normal arms, full CFG/SSA solver, branch-local actual certificate, allocator failure worlds, original cJSON H1, Ring audit-locality Product GO or formal proof. The shared fixture IR may hide common-mode modeling error despite independence of acceptance algorithms. **Track O stops on Issue #482 after one final report; DRAFT evidence only, DO NOT MERGE.**
