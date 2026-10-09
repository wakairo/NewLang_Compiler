# Draft17.29 durable LiveTail custody — partial production candidate / HOLD

Track: P — [Issue #217](https://github.com/wakairo/NewLang_Compiler/issues/217).

**P DRAFT17.29 DURABLE CUSTODY SOURCE GATE HOLD — COORDINATION INPUT REQUIRED**

This candidate does **not** establish durable custody, actual recipient-call
applicability, extraction, or native execution. Passing tests below demonstrate
the implemented prefix and conservative rejection, not completion of #217.

## Authority and scope

- Startup/current base main: `5e13a5009605389be1086b94896fc88994ca5d00`.
- `CURRENT_SPEC.md` → canonical Draft17.29, §18.1c; independently accepted
  [PR #215](https://github.com/wakairo/NewLang_Compiler/pull/215#issuecomment-6071900405),
  Track M [#214 report](https://github.com/wakairo/NewLang_Compiler/issues/214#issuecomment-6071786393).
- Process, Compiler Testing Strategy, Design Decision Procedure and DI-009–013
  were reviewed. DI-013 is ADOPTED / BOUNDED after metadata-only #216.
- Historical design-selection audit: **N/A** for faithful implementation of
  adopted source/type and conditional-definition rules. No normative decision
  was selected or changed. Draft, CURRENT_SPEC and Ledger are unchanged.

`tests/fixtures/live_tail_custody.nl` reproduces **all executable text** of
§18.1c.4 in its original order. Only comment-only `//` lines were removed:
the existing lexer rejects comments. An integration assertion compares the
fixture with the fenced canonical source under exactly that transformation.
No arms, statements, functions, ownership operations or safety preconditions
were removed. Fixture SHA-256:
`a30bbc37ff817020165d44a1436cb98ac934568f557068b378bf3118227d5644`.

## Implemented prefix

- Closed `Option<LiveTail>` type/constructor spelling parses; its H-specific
  registry contains only **type declarations**, never roots, grants or owners.
  The sum remains statically nonCopy and nonDiscardable at both variants.
- One independently checked recipient signature/body is recognized by its
  typed formal roles and bounded body, **not its function name** or a good
  caller. The body must replace the original sink with Some(original packet),
  bind the old value, consume it through exactly the specified None-only
  match, and exit unit without extra effects or packet reuse.
- `NLCustodyDefinition` in the immutable, retained function body records
  **undischarged** original-packet/current-None/local-write/alias-dependency/
  scope-nonescape obligations. Inspection survives original AST/source teardown.
  This descriptor is **not an actual call or ownership certificate**.
- The caller's fresh None local can acquire the existing implicit local
  stability and ordinary scoped ref model inside the allocated H source world.
- LiveTail and its Option are classified as authority-bearing to prevent
  signature-only calls from treating their contents as an effect-free transfer.
- Every recipient call currently fails with structured
  `CUSTODY-ENTRY-PRECISION` **before argument evaluation/consumption**. A Some
  construction lacking transfer evidence fails with
  `CUSTODY-CONSTRUCTION-PRECISION`. No general one-arm match is relaxed.

## Exact missing production evidence

The old producer certificate is owned by its entry/return fragment worlds.
`function_match` creates new owned arm fragments. Whole LiveTail receiving
currently requires the **same fragment's** `producer_call` and exact result ID.
The fork does not supply a qualified derivation carrying that original-packet
relation into the new fragment. Copying its numbers or treating the static
LiveTail type as proof would not fix this boundary.

An existing-source-only reduced reproduction is generated and executed by
`custody_source_hold_test.py`: keep the two allocation sites and original
producer/terminal receiver, bind its returned `packet`, then replace the
immediate receiving/release with:

```newlang
let policy = Option<ptr<Node>>::None;
match policy {
    None => {
        let LiveTail { owned_ptr, owned_allocation, owned_domain } = packet;
        receive_and_release_tail(owned_ptr, owned_allocation, owned_domain);
        unit
    },
    Some(q) => {
        let LiveTail { owned_ptr, owned_allocation, owned_domain } = packet;
        receive_and_release_tail(owned_ptr, owned_allocation, owned_domain);
        unit
    },
};
```

Both None and `Some(ptr_h)` policy versions fail at whole receiving with
`P208-DESTRUCTURE-ORIGIN`. This witness contains **no custody Option,
recipient, or special None-only match**. This is `COMPILER-PRECISION`, not a
language error or evidence of a normative contradiction.

There is also a subsequent state/evidence obligation: the adopted admission
and refusal arms have different intermediate C/O/R/D states; recovering a None
must not synthesize a hypothetical Some owner. Existing multiple-normal-arm
handling requires the captured frame to be unchanged and otherwise precision
rejects. It cannot be labeled `normal_frame_unchanged` for this route. Required
continuation is bounded, qualified original-packet transfer plus exact caller
current-state/occurrence evidence and sound finite conditional-arm handling.
This candidate does **not** implement that continuation or request a Draft
amendment/general owner system. Coordination should decide the bounded
prerequisite/continuation sequencing before this is treated as an admitted gate.

## Evidence and limitations

| Evidence | Result |
| --- | --- |
| Independent definitions, parameter/function/H renaming, main first | 5 source positives; owned conditional descriptor retained |
| Wrong symbolic sink/packet, store/drop, match shape, reuse/escape/extra effect, read-only/exclusive mode, shadowing | 16 source definition-shape rejections; not actual owner-call validation |
| Primary, pre-consume-refusal alternative, renamed recipient | 3 **HOLD** precision controls; no C/executable |
| Reduced producer-packet fork, None/Some policy | 2 precision witnesses; no fabricated original-owner relation |
| Wrong original ptr / Allocation / Domain passed to producer | 3 preserved existing owner-rule semantic rejections |
| Owned descriptor copy mutation | Original conditional descriptor unchanged; **not** actual custody certificate corruption testing |
| Registration OOM | 296 allocation failures to independent-definition success; 1399 to primary precision rejection; atomic rollback |
| Public state after failure | Source-created existing u8 binding/value and context snapshot preserved; no public partial artifact |

Actual admission/extraction evidence, fresh custody Some occurrence, original
O/R/D/A preservation across recipient return, alias/Unknown/occupied-sink
call proofs, both **actual** exact-None exceptions, custody-certificate identity
substitution/corruption, late/stale owner reuse, and corresponding transfer OOM
paths remain **unverified/unimplemented**. Reaching a precision guard on a
mutated source is not counted as rejection of its owner-rule violation.

The primary remains a **semantic precision rejection**, not semantic success
followed by `V1-BACKEND-UNSUPPORTED`. No new backend path was added. Existing
one/two-Node native gates, live-tail RETURN, oracle fail-closed adapter/smoke,
artifact integrity and all old tests remain required.

## Reproduction and validation

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
cmake --build build-v1-gcc -j 4
ctest --test-dir build-v1-gcc -R custody --output-on-failure
ctest --test-dir build-v1-gcc --output-on-failure -j 4
build-v1-gcc/newlangc tests/fixtures/live_tail_custody.nl
```

The last command exits 3, emits no C and reports `CUSTODY-ENTRY-PRECISION`.
The 223-test suite includes all previous 220 tests plus 3 new custody-prefix
tests. Local validation uses GCC14.2, Clang/LLVM23.1.2, Python3.12.14 and
CMake3.31.6; fixed PR CI checks GCC, GCC Release/NDEBUG, Clang+format, ASan
and UBSan using the existing workflow/toolchain pins. Exact commit, CI run and
per-configuration results are recorded on the Issue/PR handoff, avoiding a
self-referential commit hash in this document.

Stop at this **HOLD** candidate. No native custody, general owner/sum system,
third root, cJSON, FFI, LLVM or other Track/task is implemented or started.
