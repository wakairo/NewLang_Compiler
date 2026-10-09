# Draft17.30 five-root / three-field native C17 contract

Track: P — Issue #251 only. Authority: Compiler main
`d8765a5ae11919d3076cec0d1432973a0f18fdcd`, `CURRENT_SPEC.md` → Draft17.30
§3.2b.3. The canonical fixture remains byte-identical, SHA256
`812042833072145f761ddc21cbf774d81b64da3a34f6d466ba7db01a6d704d7d`.

Historical design audit (§4.2): **N/A — faithful lowering of adopted §3.2b.3**.
DI-009/010/014's unique full-region responsibility, explicit governing-domain
reloan, mode-preserving fixed projection, distinct originals and explicit
cleanup are KEEP. DI-011/012/013 and the old two-root native path are retained.
No source/API/semantic selection, Draft, Ledger or ABI change is made. Formal
finite models and source certificates are evidence, not native correctness
proofs. Independent R work is not a source of this implementation's evidence.

## Checked/backend boundary

The ordinary `newlangc` pipeline parses and registers the real complete unit,
checks actual `main()`, and hands the owned checked body to the Node emitter.
The four-field allocation profile dispatches to one private bounded adapter;
the old two-root emitter is not generalized or rewritten.

Before allocating output, the adapter invokes the public read-only captured
closure revalidator on the actual top match. This checks both arms, ancestor
lineage, original R/O/D/Allocation, all six Change snapshots, sibling
ProjectionIds, occurrences and every terminal world. No source spelling,
source hash, C address equality or offset establishes semantic authority.

Emission walks the actual checked block/operand edges in evaluation order.
Every executable node must be covered exactly once in its own fragment;
metadata-only receivers/field initializer wrappers/arm grants are handled by
their enclosing operations. This additionally rejects traversal poisoning
which leaves authentic operation nodes in the array but skips their execution.
Scalar/aggregate/sum payload and receiving metadata are checked against the
semantic value table. Loan source, domain, ref binder, mode, scope and
incarnation must correspond; root acquisition and field projection remain
separate checked operations.

Compiler-side local carriers are `(owned fragment world, symbol, C frame)`.
Each match arm receives an independent copy of ancestor-prefix availability.
Inherited nonCopy availability is checked against the authenticated common
closed post-state **for both arms**, then committed from that post-state.
No selected arm or branch-local numeric suffix becomes public state.
Loan scopes share same-world traversal coverage, and only ancestor local
availability is projected back. This is emission bookkeeping, not a runtime
owner/authority registry.

## Physical execution

Each source allocation site emits its own `malloc(56)` and real NULL/Some
branch. Some's physical handle and full raw span come from that result, not
host input. Source operations move Allocation/Storage carriers, create an
opaque checked Domain token, initialize the actual heap object, acquire a
checked scoped root ref, project the selected private C member and perform
the actual field replacement. The old Copy Option is returned by value.

The private C17 representation has three `nl_option` siblings and a `uint8_t`
payload. On supported Linux x86_64, static assertions check 56-byte size,
8-byte alignment, 16-byte Option carriers and offsets 0/16/32/48. These are
**execution-plan checks**, not NewLang layout/ABI guarantees or safety proofs.
ProjectionId selects the member only after semantic revalidation. Read/write
ref mode determines the C pointer qualification; no read→write amplification.

Only source `destroy`, `erase_slot`, `finalize_domain`, `deallocate` ends and
releases an original. There is no cleanup stack, hidden free or RAII. The six
runtime worlds have respectively 0/1/2/3/4/5 successful original allocations
and matching frees, in explicit source reverse-original order. Persistent
nonowning ptr tokens do not create release responsibility.

## Independent measurement boundary

`five_checked_probe` uses public production source/checker APIs, disposes the
original source/AST and independently revalidates owned certificates. It
provides five checked original identities/payloads and six scoped root,
projection/mode and occurrence records to the test observer. It does not
invoke the emitter or create a positive authority prelude.

`five_platform.c` interposes only generated application's guest malloc/free.
Nth-call NULL injection (1..5) is separate from compiler OOM. Each success
uses real platform malloc; no heap bytes are initialized or repaired there.

`five_observer.h` maintains test bookkeeping and reads live application
objects. It checks actual distinct addresses, scalar payloads, old/new Option
values, root/domain/projection/mode correspondence and every explicit terminal
sequence. Immediately after Change six, before any EndRoot, it inspects all
15 sibling fields, including the exact seven required structural facts.
**After this snapshot it never reads Node fields again.** Remaining EndRoot
and free-order observations use captured integer addresses and checked IDs.
It never writes, links, unlinks or frees application storage.

The observer's seven-fact library policy is independent of emission. Changing
`A.prev=C` to the memory-safe `A.prev=B` remains source-admitted and produces
different real C, but deliberately fails this policy oracle. It is not a
language-invalid result or a checker finding. Alpha/arm/constructor-argument
permutations and canonical comments preserve the real topology; constructor
permutations visibly alter generated C rather than a hardcoded body.

The same production output is also compiled without observer hooks for all
six worlds, and with neither observer nor allocation shim for real-platform
all-Some execution. Instrumentation is not required for correct behavior.

## Failure/ownership contract and limits

The borrowed artifact is immutable. Success returns owned, NUL-terminated C
text and byte length; caller frees text. Failure preserves the initial NULL
output pointer and arbitrary length. The emitter stages all bytes privately,
publishes only after full validation/coverage, and distinguishes validation
or subset failure (`V1-BACKEND-UNSUPPORTED`), OOM and resource exhaustion.
Compiler OOM cannot become a guest None outcome.

The finite adapter permits five sites/six source Changes only, at most 2047
nodes per fragment, 2048 emission steps, 256 local carriers per scope, depth
12 and a 131072-byte staged buffer. These are implementation limits, not
language rules. Other checked constructs remain explicitly unsupported.

Tests exhaust all 845 allocator failure points in revalidation plus emission,
check unchanged snapshot/view/output/length, and compare clean retry bytes.
57 certificate poisons and 110 additional traversal/scalar/payload/entry/
domain/loan poisons must fail before any C publication. Source negatives keep
their established diagnostics. Native C mutants fail via active observer
checks, exit 77 before invalid real free/dereference; post-free requests use
pre-captured integer identity, not a load of an indeterminate freed pointer.
Sanitizer crashes after UB are not accepted evidence.

This contract stops at **five-root pre-detach native execution**. It does not
authorize B detach/adoption, a general allocator/owner system, other language
rules, native cJSON PASS, LLVM, FFI, concurrency or another Track/task.
