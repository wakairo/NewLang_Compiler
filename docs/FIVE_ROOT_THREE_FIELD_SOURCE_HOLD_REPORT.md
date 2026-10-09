# Draft17.30 five-root / three-field source gate — HOLD

Track: P. Issue [#237](https://github.com/wakairo/NewLang_Compiler/issues/237).

## Authority and disposition

Start/base main: `d6f1a7b92c351cd23661ad7714ecb51d29e9468f`.
`docs/reference/CURRENT_SPEC.md` selects canonical Draft17.30. The complete
§3.2b witness is normative; current production profile restrictions are not
language restrictions. Process §§4.2/6, Design Decision Procedure, Compiler
Testing Strategy, and DI-009–014 were checked. Historical-decision gate: N/A,
because no language/design selection is made or replaced here. No Draft or
Ledger changes, no additional Track or prerequisite task is launched.

**HOLD, not READY.** Full source semantic acceptance is not established.
This candidate preserves reproducible actual-source failure evidence. It adds
no production admission, forged artifact, or replacement of a checker guard.
The finding is **COMPILER-IMPLEMENTATION-LIMIT / COMPILER-PRECISION**, not a
specification hole or evidence that the canonical program is unsound.

## Full primary source and actual observations

`tests/fixtures/five_root_three_field.nl` contains the entire canonical fenced
§3.2b.3 program, retaining all five syntactic allocation sites, all three link
fields, six writes, the A.prev=C tail shortcut and explicit 0–5 cleanup.
Only lines whose first non-whitespace characters are `//` were removed.
Inline comments, whitespace, spelling and terminal LF are retained.
The canonical payload has 15,055 characters excluding its terminal LF;
15,056 bytes including that LF. SHA-256:

- Canonical payload including LF:
  `5145c955343a18d497c2f0568bc78cc0f31c54bdda4e0ff3a0734a6ea5b4fac4`.
- Comment-only-stripped fixture:
  `812042833072145f761ddc21cbf774d81b64da3a34f6d466ba7db01a6d704d7d`.

The actual production CLI rejects the full source at its second field,
`prev: Option<ptr<Node>>`: exit **3**, `AVS-DECL-PROFILE`, empty stdout and no
created C/object/executable. Two unchanged-topology variants (nominal/local
alpha-renaming and in-range payload changes) have the same refusal.
These are early implementation-profile observations, **not** semantic
negative controls, successful source admission, or backend-unsupported exit 4.

Exact diagnostic/output bytes, hashes and repeated-run identity are recorded
in `docs/evidence/FIVE_ROOT_THREE_FIELD_SOURCE_OBSERVATIONS.json`.
Reproduce all observations, including canonical extraction verification:

```sh
python3 tests/integration/five_root_source_audit.py build-v1-gcc/newlangc
```

The tool reports observations; exit 0 means the probe ran, **not** that the
source gate passed. It is deliberately not registered as a passing admission
CTest. Future implementation acceptance must replace this HOLD evidence,
not preserve the current rejection as normative behavior.

## Minimum proof blocker beyond surface guards

At the entry to the **third** allocation, the original src and A roots are
both live. The None successor explicitly destroys/releases A and src; the
Some successor must eventually do the same plus its own fresh responsibilities.
An ancestor-derived closed post-state must therefore certify **two separate**
O/R/D/Allocation tuples and prove both successors against that post-state.
Neither source field syntax nor numeric cardinality by itself is that proof.

Current implementation:

- `src/allocated_join.c:nl_allocated_closed_prefix` requires exactly one
  BackingRegion and one LifetimeDomain. It finds one live typed root and one
  Allocation/domain binding pair, constructs their closed ancestor post-state,
  and writes one tuple into the certificate.
- `include/newlang/checked.h:NLCheckedNodeView` has one `captured_backing`,
  `captured_root`, `captured_incarnation`, `captured_domain`,
  `captured_allocation` and `captured_domain_binding` tuple.
- `src/semantic_check.c:allocated_match` constructs that post-state before
  checking **both** arms, requires `nl_allocated_post_matches` independently,
  and publishes the ancestor-derived state without selecting an arm. Its
  current two-site profile is consistent with that one-captured-root proof.
- The later canonical matches require three and four captured tuples.
  Reusing one tuple, pretending the frame is unchanged, taking one arm's
  state, or comparing fresh numeric IDs across worlds cannot discharge the
  other original obligations. The existing code has no bounded multi-owner
  certificate/public read-only revalidator for these matches.

The isolated actual-source fixture
`tests/fixtures/three_allocation_no_links.nl` removes all field writes and uses
only the existing one-link Node shape. It still requires the above two-owner
captured join and is refused at site 3: exit 3,
`ALLOCATED-CARDINALITY-PROFILE`, no output. The corresponding
`tests/fixtures/two_allocation_no_links.nl` control **passes semantic checking**
and stops at exit 4 / `V1-BACKEND-UNSUPPORTED`, with no C. Both fixtures explicitly
release every original successful grant on every path. They isolate the
implementation proof boundary; neither replaces the complete primary source.

This is a concrete missing production proof representation, not a claim that
such a representation is mathematically impossible. A sound implementation
would need a bounded ancestor-qualified set of up to four captured tuples,
construction and independent validation of every arm's complete post-state,
owned lifetime/occurrence evidence and poison/OOM validation. No such complete
implementation is supplied in this candidate. Raising the allocation budget
and declaration field count alone would conceal this outstanding obligation.
Coordination receives this decisive HOLD instead of a partial READY or a
self-launched chain of micro-prerequisite tasks.

## Additional implementation work not claimed complete

The existing parser/recursive completion/fixed-field attach and validation
paths admit two fields only. Ref projection selects declaration index zero.
Three distinct next/prev/child projections inside each original root,
per-field occurrence/Change/Reset preservation and all five original owners
have **not** been validated by an accepted full-source artifact here.
No five-root owned artifact exists, so post-AST-teardown validation, checked
poison attacks and five-site OOM rollback are **not claimed**. Wrong list
wiring has not been reclassified as a memory-safety error. No native source,
backend, observer, allocator, B detach/adoption or cJSON implementation changed.

## Regression / handoff

Validation results and fixed-head CI are reported in the Issue/PR handoff;
existing CTests include the original two-root native custody, oracle.adapter,
oracle.smoke and artifacts.integrity. Their success is regression evidence,
not five-root acceptance. Candidate PR remains OPEN/unmerged, Issue remains
OPEN. No next task or Track is started.

P DRAFT17.30 FIVE-ROOT THREE-FIELD SOURCE GATE HOLD — COORDINATION INPUT REQUIRED
