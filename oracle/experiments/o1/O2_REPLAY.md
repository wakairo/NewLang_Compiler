# O2 independent O1 replay: provenance and constraints (Issue #343)

**Class O/H, synthetic non-normative research. DRAFT / DO NOT MERGE.**
Frozen Compiler/main parent: `a5343bf70004b30a8b026e52aa2fde93b5920be0`.
Canonical is selected by that SHA's `docs/reference/CURRENT_SPEC.md`: **Draft 17.30**. Never copy, update, or silently substitute `CURRENT_SPEC.md` in this experiment.
Experimental compiler PR326: `de9c4b175784f484675b670a109f62edeaf35de1` (NOT executed or modified).
P338 / PR340 is contextual SHORT real non-cJSON source-admission evidence, NOT oracle input; M336 remains HOLD DESIGN.

## Authentic O1 artifact provenance

Recovered ORIGINAL files from the O1 conversation; independently checked bytes before replay:

| O1 artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| Original `O1_python_primitive_repro.zip` | 13,808 | `68c04fe1d664d78b439f8f1292db525ed7ca79c6f2841d1e98bdaa8f30f0957a` |
| Original generated `o1-results.json` | 332,141 | `cee5353583b856127537ec623795d4570c995b4699433923e221a56579af5b02` |

The original ZIP had exactly eight regular files, all rooted at `oracle/experiments/o1/`; zero traversal paths or symlinks, ZIP integrity passed. The eight original file contents were copied **byte for byte, unchanged**, into this branch. Their independent SHA-256 values:

| File | Original SHA-256 |
| --- | --- |
| `README.md` | `91dc20f5659a9f32f26a490781c25156f4a540144a50e7c9cbab6ac3bb666482` |
| `abstract.py` | `49b5730e11c2ca49e622965a904ff5fb127730ef8a764786430462a33e0ad7e9` |
| `exact.py` | `f6673b8b716219d97fe244d8cec94e87e4e9e978dc91aed1c57b30c7b412647b` |
| `generate.py` | `8251d281bc223243b7eff720d9bdb9faa16534408ada9e657d8a9af9685ae575` |
| `neutral.py` | `9ca2ab900352d7eccaf91c337e914f03e362792b8a08639d8a8e51bc5b6ba9b9` |
| `run_experiment.py` | `490a968e2dbdc495d69b86a91ef23f4f82eaaaac28075090a80c9b1e2b89cb57` |
| `summary.json` | `77ad5713f41d53810293faca9a0141255d7e5e5c7e889f995736a7c7c44d1c89` |
| `test_o1.py` | `687600ad05baf5ccba5262720105fb2adbc4cbd2a2d3d25bb6b4e9f25b83c0e4` |

**Do not commit the generated 332 KB corpus:** `./replay_o1.sh` rebuilds it deterministically with the above SHA-256 check. The immutable original corpus was also directly compared byte for byte to a freshly regenerated file in a Python3.13.5 virtual environment, and they were **identical**. `summary.json` is the authentic O1 original.

## Independent actual replay

From this folder, with Python >=3.12, Bash and Python venv support:

```sh
bash replay_o1.sh
# Or, manually in a fresh venv from this folder:
python -m unittest discover -p 'test_*.py' -q
python run_experiment.py > o1-results.json
sha256sum o1-results.json  # verify the frozen SHA-256 above
```

**Already observed, not CI claim:** Python 3.13.5, fresh isolated venv without pip, with `PYTHONDONTWRITEBYTECODE=1 PYTHONHASHSEED=771`: **9/9 unittest PASS**, 550 machine-replayed cases, regenerated complete output is byte-for-byte equal to original (digest above). 266 both ACCEPT, 280 both SEMANTIC_REJECT, 2 oracle ACCEPT / abstract PRECISION (`event_budget=8` versus12), 2 oracle UNSUPPORTED / abstract PRECISION (Unknown alias), 0 corpus-observed abstract false accepts. Unsupported != agreement. Both motif families `device_directory` and `intrusive_scheduler` ACCEPT for lengths 2/8/9/10/12 under budget12, with ninth-Change budget8 precision and unchanged budget12 clean replay.

Static inspection of `abstract.py` shows imports from `dataclasses` and `neutral` only (not exact/old checker). `test_o1.py` traps `exact.run` with an exception and still gets independent abstract ACCEPT. No exact acceptance guard is imported or called by `abstract.run`. **Independent implementation is not an independent normative specification:** both checkers are coded against the same untrusted synthetic `Program.originals`, event declarations, evidence ledger, and shared interpretation of what a Change means. A forged fully consistent pair of source operations and proof cannot be ruled out by this model.

O2's separately constructed external counterexample (not produced by `paired_mutations`): changing only the third `OpenLoan.domain` to `d_other_provenance` in the 10-Change scheduler sequence led both exact and abstract to `SEMANTIC_REJECT / WRONG_ORIGINAL_DOMAIN` at `open:03_ca000ab0` with sequence hash `c762803649bdc3e548a86141ba17c969d5e24c4c12f994c47a964d825102e9ba`. Existing independent pairs additionally check stale old Some occurrence, same-pointer new occurrence, forged sibling Current, missing/duplicated/reordered ninth proof, read→write escalation, bad ptr region, loan forgery. Unmodeled owner move/EndRoot -> UNSUPPORTED. O1 source files were **not** modified to accommodate that mutation.

## What this does NOT establish

Neither `Original` synthetic identities nor `source_node` names are a source-issued root/Allocation/Domain provenance certificate or a complete mapping to checked source nodes. No real C source→checked-owned adapter, C public validators, generic ninth/tenth source acceptance, named helper actual/formal substitution, five fallible 0–5 worlds, returned nonCopy B+dst owner, effect summary, native backend or LLVM. No joins/loops/real alias alternatives are exercised: abstract `LatticeCell` possibilities remain singleton on tested inputs. Hence **NOT H1 PASS**, **NOT C cap8 permission**, and M336 HOLD DESIGN unchanged.

## Track O / dedicated `wakairo/NewLang_Oracle` split gate — recommendation: HOLD

One reproducible primitive with no independent CI execution and no versioned neutral cross-implementation protocol is insufficient to justify a new permanent Track or repo now. Stdlib-only Python is independently runnable (no private Compiler C imports) and hashable; however source authority, parser/IR contract and independent C interoperability are absent. This evidence-only PR deliberately adds **no workflow** and does not establish independently operating Python CI, cross-OS/Python-version stability, maintained neutral event schema, or sustained multi-milestone engineering need.

**Exit criteria to reconsider split:** Coordinator explicitly accepts stable, versioned source-neutral `Program/Event/Outcome` serialization schema + negative fixtures; each test run pins Compiler commit + canonical spec blob SHA independently (not floating `main` and not duplicate normative prose); an independently gated Python3.12/3.13 Actions job on immutable O commits passes without requiring Compiler CI; isolated release/ownership/review/maintenance plan; and a separate independently checked genuine source-derived non-cJSON C bridge produces comparable outcomes without granting authority through copied ptr or asserted synthetic facts. Then migrate provenance-preserving exact file hashes/commits into `wakairo/NewLang_Oracle` under an explicit Coordination issue, not by auto-copy or submodule hard dependency.

**O2 terminal:** Preserve O1 byte-identical code, replay evidence, recommend HOLD separation; leave this PR DRAFT / DO NOT MERGE and stop. No spec/compiler/main/old PR modifications.
