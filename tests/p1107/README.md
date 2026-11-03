# P1107 — a source-owned Copy helper through normal BREAK

C199 authorizes this finite implementation from main
`c6fe0b3493fb4012b07bfd8c2c17f79566b67ad4`. CURRENT_SPEC remains Draft17.34.
The source family uses an ordinary two-u8 T, one nonrecursive two-formal
`(ref<write,u8>,u8)->u8` helper, one direct `replace` of those formals, a
scoped field loan and one normal BREAK. The old result may flow through pure
old-result aliases, an explicit RETURN, a direct RETURN of replace, or the
tested binding-tail form. The native consumer requires a literal incoming
argument. Both field positions, declaration order, no later caller write,
later other-field write, equal bits and a second original are exercised.

C202 permits continuity after docs-only main
`26163bfa12a1adc56dd66f051749c823a0fb71bf` (direct parent is the fixed main).
Its sole change is Roadmap +16/-0. The original frozen base and evidence are
retained without rebase/refreeze; the PR targets the now-current main and the
compatibility claim is limited to that verified documentation-only difference.

## Responsibility and trust

The definition checker inspects its own just-checked ephemeral callee body,
fresh formal bindings and exact old-result def-use. It does not enable an
actual Copy issuer or retain a native receipt for a symbolic definition.
Actual invocation completes the owned LOOP caller world and graph inside the
outer CHECK transaction, then uses the existing public Copy body validator.
The ordinary call/parameters/entry/handoff/event proof and the shared adopted
old-result normalizer identify the real Change and returned old package.
The existing source-qualified BREAK frame still checks the entry, actual
poststate, scope cleanup, dependency-free old result, sibling and other state.

The private retained consumer independently checks graph/custody/world seals,
the enclosing **current** registry, original callable plan/syntax/parameter
names and the actual inner receipt. A historical GET is not authority in a
different current caller. The definition-only route is never used by this
consumer. No public issuer, source rule, API, ABI or H-origin mechanism changes.

The C17 consumer calls the authenticated helper exactly once with the address
of the parent original's field and forwards its real returned value. It does
not create a second T or also run the primitive store. Existing scalar/pointer
calling conventions and the ordinary-ref alias contract are retained.
Revalidation OOM propagates through root preflight and storage qualification:
failure cannot select const storage or publish an output as success.

New callee-plan references are borrowed from canonical owned call children.
Destruction still follows the adopted canonical child ledger. The A/B tests
leave foreign whole LOOP/callee display entries and changed cardinalities
unrestored, destroy A, then use and destroy genuine B. Other metadata mutations
are restored before teardown; no known-unsafe corrupted graph is executed.

## Evidence and bounded coverage

`freeze-v1.json` and its complete archived inputs precede first execution.
The original P1105 positive and equal-bits/wrong-return source SHA256 remain
`cccdc1b51e7fddb230f2a1ee268f69750559cdf7310074b0a63b49ea7f637fa1`
and `1099d5cd5ae589f52d571f683390bd83b32c27a1723c0225555b429231e3208c`.
The original oracle is unchanged. Prospective freezes preserve every previous
harness and RED, and version 6 adds current-parent/callee-plan/loan negatives,
an independent read-formal/write source and passive live address/value traces.
These are P-owned examples, not R1101's original source/oracle/runner/raw.

`evidence/fixed-main-FIRST.json` and ZIP preserve the real first registration
precision refusal of all five intended positives, plus successful outside
helper and primitive LOOP controls. The API observer records actual
registration status=3 before invocation. Later success does not rewrite FIRST.
Also retained are the initial test const-pointer compile error, passive-store
counter scope error, prospective-history import error, strict production build
warning, first named-OOM status propagation RED, and overstrict prelude metadata
comparison RED. They are different failures and candidates.

The probe checks public GET/current, source BREAK provenance, private relation,
driver consumption, copied-view negatives and 46 logged mutations per positive
family. Additional readonly observations remain historical when current
callable validation refuses. Native tests separately compile raw C17 and
passively observed C17 with GCC/Clang O0/O2, asserting live original/field
addresses, actual helper/Change counts, old/current/sibling/other-root/caller
values. Assertions and traces belong to this finite family, not general proof.

The source-owned OOM probe exhausts every reached single allocation failure
in actual CHECK and in the named CLI, checks API/CLI status, transaction state,
no publication, bytes/inode/no-temp preservation and disabled-fault retry.
Sanitizer/full regression logs and the final evidence index are reported in
Issue #1107. CI IDs are reported there without changing a green candidate HEAD.

`history_v21.py` pins adopted v20 by external SHA256 and Git blob OID, inherits
all earlier coverage and retains original source/oracle/FIRST data. The current
v21 inventory uses Git-managed files only. The small P1098 forward runner leaves
its original script/assertions intact and selects the legitimate current
generation; it does not disable historical checks or rewrite v1-v20.

## Limits and cost

Multiple helper Changes, recursive/nested calls, incoming/ref aliases inside
the helper, nonliteral native incoming arguments, general Continue/backedges,
multiple exits/nested loops, H-history plus LOOP, arbitrary aggregates and
foreign ABI are not promised. Legal unsupported programs remain precision or
backend coverage debt. Equal runtime bits do not establish old-result origin.
Source09 stops at the existing field-read-loan profile before write capability
checking; source11 separately reaches the read-formal/write type mismatch.

The adopted old-result function moves byte-for-byte to a shared private header
(function SHA256 `5c981fc9ae6ae683f429d677b618a51a3e1ca2df0ac2e68fea3694df9f412d90`).
New TCB is the finite effect adapter, completed-caller ordering, current registry
and immutable-plan checks, and original-field call lowering. No general IR is
introduced. A ten-node caller seal is 17,280 bytes on the measured host
(`sizeof(NLCheckedNodeView)=1728`); the private result view is 96 bytes, up 16.
The retained plan anchor adds five borrowed pointers (40 bytes on this host),
with no new ownership cycle. Existing Copy call entry/exit/handoff/event worlds
and allocating replay are reused. Total peak heap, speed and human maintenance
cost are not measured; extra snapshots/revalidation and compiler audit burden
remain material costs.

VALUE-ALIGNMENT-RISK: reusable selected-v0 composition is the technical goal.
No reduction of library-author plus ordinary-safe-caller UB-prevention audit
and re-audit dependency closure versus matched well-factored C has been shown.
Capability/native/CI success is not general soundness or Product PASS.
Product GO remains PAUSED, H1 HOLD; PR1050/P1036 are not reactivated.
Return the bounded candidate for Coordination's proportionate independent
review in DRAFT/HOLD; no READY, merge or Product GO.
