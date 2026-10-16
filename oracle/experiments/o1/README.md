# O1 — independent Python FieldChange event-tape experiment

**Class O/H research, non-normative. DRAFT / DO NOT MERGE.** Evidence for [Compiler Issue #341](https://github.com/wakairo/NewLang_Compiler/issues/341). Reference Compiler/main `a5343bf70004b30a8b026e52aa2fde93b5920be0`, Canonical Draft 17.30; C PR326 `de9c4b175784f484675b670a109f62edeaf35de1` **is not used or changed**. The P338/PR340 short non-cJSON evidence is contextual, **not oracle input**.

Python >=3.12, Python standard library suffices. Pytest is optional (tests also use `unittest`). From this folder:

```sh
PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -p 'test_*.py' -v
PYTHONDONTWRITEBYTECODE=1 python run_experiment.py > o1-results.json
# Optional: PYTHONDONTWRITEBYTECODE=1 python -m pytest -q -p no:cacheprovider
```

## Shape and independence

* `neutral.py`: immutable `Original` / `Pointer` / `Option` / `Claim`, typed synthetic source operations `OpenLoan`, `Change`, `CloseLoan`, payload dependents and `Unmodeled`; independently asserted event `Evidence` tape and stable SHA-256.
* `exact.py`: concrete dict state, checks origin uniqueness, actual original governing Domain for lexical write refs, Option old/new fact and occurrence, sibling conservation, scope and payload-dependency survival; fresh `occ:<node>` every Some replacement, including same pointer.
* `abstract.py`: **separate** finite state (`LatticeCell` frozenset alternatives) and separate checking/transition code; path/alternatives (currently always singleton) and *field event budget* enforce precision rather than semantic refusal on resource exhaust. Imports only neutral IR, never `exact.py` or historical `semantics.py`. `test_o1.py` checks import AST and dynamically monkeypatches `exact.run` to throw while abstract acceptance still succeeds.
* `generate.py`: deterministic seed `20261012`, non-cJSON device directory (child/next/prev repeated updates across live directories and slots), intrusive scheduler (two distinct roots, next/prev and same-ptr Some→Some), lengths 2/8/9/10/12. Creates separate untrusted witness claims for *synthetic* source operations.
* `test_o1.py`: paired mutation falsifiers; `run_experiment.py` performs external comparisons, emits machine-readable case outcomes with source sequence SHA, first event and rule; **hard fails on exact semantic reject vs abstract accept**, and never counts unsupported as agreement.

The input `Original` records are **synthetic assumptions** made by a test case, *not* source-derived authority. Neither tape claims nor Copy ptr mint Allocation/Domain, root, or ref rights. The experiment does not verify input extraction from actual NewLang source. The only counted `FieldChange` events are caller changes; no callee fragment is modeled. Each event has an explicit loan open/close. No cJSON layout/order assumptions are in either interpreter.

## Boundary / stop rules

* Wrong original Domain, read→write, stale payload occurrence, changed sibling masquerading as current, missed/duplicated/reordered ninth certificate, invalid ptr provenance, use of a ptr as Option/Domain authority: fail closed. Alias/Unknown is `exact UNSUPPORTED` / `abstract PRECISION` (no false agreement). A live payload-dependent ref blocks whole-Option Reset.
* `Unmodeled` covers owner move / EndRoot / live loan crossing owner terminal and must return **UNSUPPORTED**, never silently accepted. Full returned nonCopy B+dst owner, callee helper substitution, 0–5 fallible allocator cleanup worlds, actual `ref_from_ptr` source admission, P source-generated checked-owned validator, and native C/LLVM emission are **out of scope**.
* `event_budget=8` on a modeled valid 12-Change program yields **PRECISION**; clean retry with 12 accepts. Budget is analysis resource, **not** a language-law cap.
* The finite abstract *alternatives* are implemented as frozensets but this straight-line O1 corpus does **not exercise joins or non-singleton alternatives**; do not extrapolate to CFG/loops.
* Stop rather than claim reuse if new work needs an unproved input authority bridge or oracle-assisted abstract acceptance. Expansion to C replay, named calls, owner terminal or production source rules requires separate Coordination approval and adversarial review.

Historical M7.5 and Draft17 Python assets informed architectural separation; **none are imported at runtime**, no historical source or generated result is a current Draft17.30 acceptance guard. This is synthetic non-native proof-of-concept only.
