# P1 — Source & Lexical Frontend Foundation

Branch: `p1-lexical-frontend`, based on verified latest main / merged Pre-P1
`4d4a2fc2868eebc9e2b4680310aa7cee027c57c6`. P0 and Pre-P1 are historical closed
milestones; their reports/snapshots have not been rewritten. P1 review only:
Codex must not merge this PR or begin P2.

PR: [#3](https://github.com/wakairo/NewLang_Compiler/pull/3), open against main,
unmerged. Implementation and initial fresh Ubuntu PR CI are green; final
current-head evidence is tracked in the PR description/checks as below.

## Scope and audit

Implemented source bytes -> NLSource -> NLLexer -> NLToken. No parser, AST,
semantic checking, MIR, lowering, object/executable generation or public CLI
mode. No SourceId registry, multi-file compilation architecture, allocator,
arena, collection, lookahead, interning, fuzzing or thread framework.
CLI compile paths still return the original explicit unsupported diagnostic.

The [P1.0 audit](P1_LEXICAL_CONTRACT_AUDIT.md) was written before source/lexer
implementation. Required C/testing/review/location policies and P0 architecture,
toolchain/history were read; relevant Draft 17.4 source-surface/numeric/function/
control-flow/location sections were read after document-wide lexical searches.
Backend Contract v0.4 was reviewed. N language/backend > A closure > O oracle >
F formal > I implementation authority remains unchanged. LLVM never appears in
source/token/lexer interfaces; M7 downstream proof-origin constraints remain.

- **FIXED:** §6 typed integer semantic constraints and §28.1 path-as-metadata
  constraints. No complete normative lexical grammar is claimed. P1 does not
  implement typed literal semantics or range checking.
- **IMPLEMENTATION-SUBSET:** ASCII WORD `[A-Za-z_][A-Za-z0-9_]*`, decimal DIGITS
  `[0-9]+`, single-byte PUNCTUATION from `(){}[],:;.+-*/%<>=!&|?`, spacing SP,
  TAB, LF, CRLF. Composite operators are separate atoms. A punctuation atom
  does not assert a corresponding semantic operator (`?` is not propagation
  syntax; Draft §26.30 expressly excludes it).
- **DEFERRED:** keyword reservation/contextual classification, full identifier
  and numeric spelling, comments, source encoding/Unicode/BOM/NUL policy,
  literal decoding, normative whitespace/newlines and maximal munch.
- **COMPILER-SPEC-HOLE:** LEX-01 identifiers, LEX-02 keyword reservation,
  LEX-03 spacing/newlines, LEX-04 comments, LEX-05 punctuation boundaries,
  LEX-06 numeric spelling, LEX-07 encoding/exceptional bytes. Audit records
  minimal inputs, Draft sections, backend/oracle/formal relevance and candidate
  adjudication choices. These remain open language-level findings.
- **COMPILER-SPEC-AMBIGUITY:** none confirmed. Missing grammar is not portrayed
  as contradictory normative rules. Draft §0 says examples are conceptual.

Unsupported bytes include NUL, all non-ASCII bytes (valid/invalid UTF-8 alike),
BOM, lone CR, VT/FF, quotes, backslash and punctuation outside the explicit set.
`//` and `/*` openers are unsupported two-byte spans, not skipped comments.
Other unsupported spans have length one. Unsupported is a P1 capability result,
not a normative assertion that the source is invalid NewLang.

## Source, ownership, failure and locations

`NLSource` is opaque and owns its immutable unsigned-byte allocation, logical
`size_t` length and copied NUL-terminated display name. Memory construction
copies both inputs, borrowing them only for the call. File loading reads binary
blocks without seek/file-size conversion, then transfers the completed buffer
to the source (no second contents copy). File path is copied as display name.
It preserves CRLF/LF, NUL, BOM, arbitrary invalid encoding and non-ASCII exactly.
No content sentinel or decoding is used. Empty bytes may be NULL.

The caller supplies an initialized NULL owner slot. Success yields one owner;
failure leaves the output untouched and cleans partial storage. Nonempty slots
are rejected rather than overwritten. Status distinguishes invalid arguments,
too-large storage, OOM and I/O failure. Destroy consumes source/bytes/name;
NULL destruction is allowed. All sources/views must cease to be used after
destruction; C cannot enforce this lifetime statically. Name/views are borrowed
until destruction and cannot be freed or modified by callers.

Memory inputs above PTRDIFF_MAX return TOO_LARGE before access/allocation.
File growth checks addition and doubling; realloc uses a temporary pointer and
old storage is freed on failure. No zero-sized allocation, NULL-pointer
arithmetic or C-string length assumption is used for contents. fopen/read/close
failures return IO_ERROR, not source errors; files are closed on all paths.
OOM returns OUT_OF_MEMORY with cleanup and no process exit/source diagnostic.
Loading does not promise an atomic snapshot of a concurrently changed file.

`NLSourceSpan` is a small value `{size_t start_byte, end_byte}` with invariant
`0 <= start <= end <= length`, representing `[start,end)`. Empty spans including
EOF are valid. `span_valid` and `view` reject inverted/out-of-bounds spans in
Debug and Release; failed views leave output untouched. Empty views must not be
dereferenced. Spans are interpreted with their source object; equal paths do
not give equal source identity. No registry or nominal declaration identities
are introduced. Line/column remain derived presentation data. P1 needs no
adapter/line index; existing NLSourceRange/diagnostics are unchanged, and lexer
state contains only byte offset rather than line/column.

## Token and lexer contracts

`NLToken` is kind plus canonical span, owning nothing. Kinds are EOF, WORD,
DIGITS, PUNCTUATION and UNSUPPORTED; no token.c is needed. Text is obtained as a
borrowed view from source + span; no per-token text copy/allocation, number
conversion or keyword table. Token values survive lexer reinitialization, but
dereferencing corresponding text requires the source to remain live.

`NLLexer` borrows const NLSource and owns cursor state only. The source must
outlive every operation. Two small fields are public for allocation-free stack
storage, module-private by contract; callers/tests use init/next, never inspect
or mutate fields. No heap allocation, buffers, teardown resources or global
state. This avoids an opaque allocated context/wrapper for two non-owning fields;
NLSource remains opaque because its owning fields materially need protection.

| Result | Output/state |
|---|---|
| TOKEN | Valid kind/span; commits spacing plus nonempty atom, advances |
| EOF | EOF `[length,length)`; remains at end, repeated calls deterministic |
| UNSUPPORTED | Valid nonempty unsupported span; commits preceding spacing, leaves offending bytes unconsumed; repeats same span until init |
| INTERNAL_ERROR | NULL argument/inert or invalid module; state and token output unchanged; not reported as invalid source |

Init succeeds for a live source and resets cursor; a NULL source returns false
and leaves a non-NULL lexer inert. Caller may abandon the lexer without deinit.
State can be reused via init after unsupported/input reset. Checked cursor and
lookahead bounds avoid unsigned wrap and exact-end reads. Explicit unsigned
ASCII comparisons use no ctype, locale, pointer identity, environment or hash
order. Same source and initialized state produce identical results/spans.

## Test inventory and evidence

Baseline bootstrap / format / GCC and Clang each passed the existing 10 CTests
before implementation; failures were not hidden by P1 changes.

| CTest | Owning contract / coverage |
|---|---|
| source.unit | Empty/ASCII/raw NUL/LF/CRLF/BOM/non-ASCII/invalid bytes; exact length/all small valid slices/empty EOF span; inverted/OOB/SIZE_MAX span rejection and unchanged output; independent identities with equal names; byte/name copy; view lifetime; nonempty-slot rejection; repeated create/destroy; invalid args/overflow; injected malloc failures and retry |
| lexer.unit | Empty/spacing-only EOF; WORD/DIGITS; adjacent punctuation and split composite spellings; typed-literal-shaped atoms without interpretation; exact zero-based spans/lexeme views; no keyword classification; numeric-word boundary; paired lexers/reinitialization determinism; repeated EOF and unsupported; exhaustive one-byte unsupported-set boundaries including NUL/high bytes/CR; comment openers; unmodified source/borrowed views; inert/NULL failure leaves output/state unchanged |
| source_lexer.integration | Python creates isolated exact-byte files, executes the instrumented C binary; CRLF answer-shaped fixture with complete token/span sequence twice; binary source retains NUL/BOM/UTF-8/invalid bytes and scanner reports unsupported NUL; empty source EOF; multi-block 15,000-byte preservation; 8,192-byte long atom/exact full-block EOF; missing file/read-error directory; injected initial/subsequent realloc failure cleanup and successful retry |

Three new tests use meaningful module APIs, not production .c inclusion or
private field inspection. Small tests/support/test.h only supplies an always-on
boolean CHECK with file/line context. Linux test-only linker `--wrap=malloc` /
`--wrap=realloc` inject allocation failures; production APIs are unchanged and
no general allocator framework is added. Allocation counts/capacities/layout
are not assertions. Tests stop fault enumeration at the first successful call.
Read/close failures after partially returned data and impossible huge-file
limits are code-reviewed, not claimed as separately fault-injected cases.

Normal CTest includes all three new tests plus the ten unchanged P0 tests:
diagnostics, six deterministic CLI cases, LLVM C API, artifact integrity,
frozen M7.5 oracle. **Semantic/oracle: N/A for new P1 lexical modules**; existing
oracle smoke remains green. Neither the 466-test historical suite nor live Lean
build is required/claimed. No references/oracle were changed.

Local Debian 13 x86_64, C17, CMake 3.31.6, LLVM/Clang/clang-format 23.1.2,
GCC 14.2.0, Python 3.12.14: GCC, Clang, ASan and UBSan pass **13/13 each**
(52/52 final matrix executions). A GCC Release/NDEBUG build additionally runs
all three new module/integration tests: **3/3 pass**, verifying that bounds,
unsupported handling and test assertions do not depend on Debug assertions.
ASan leak detection and UBSan halt-on-error exercise source/lexer plus allocation
failure paths, not merely compilation. Strict flags remain
`-Wall -Wextra -Wpedantic -Werror -std=c17`. Checks stay active under NDEBUG.
Formatter/check use unchanged .clang-format and pinned clang-format 23.1.2.
No toolchain/bootstrap/provenance/CI workflow pin changes are needed.

Reproduction: README bootstrap/activation/format commands followed by CMake
`--fresh` configure, build and CTest for build-gcc/build-clang/build-asan/
build-ubsan, using address/undefined sanitizer settings as before.

## PR / current-head CI gate

Fresh Ubuntu 24.04 PR-triggered
[run 37204009773](https://github.com/wakairo/NewLang_Compiler/actions/runs/37204009773)
at implementation head `b13c05a5da63b0f4d3e1ad0504b5b7d415f1d56f` completed
**success**: GCC, Clang, ASan, UBSan jobs, bootstrap and configure/build/test
steps all pass, including Clang's non-modifying format check. Each job executes
the normal 13-test suite with the three new tests; nothing is skipped.

The report-only revision also requires its own successful current-head run.
[Current-head PR checks](https://github.com/wakairo/NewLang_Compiler/pull/3/checks)
and the PR description/final handoff record that final exact SHA/run URL;
the earlier implementation run above is not substituted for that evidence.
Current-head validation must remain green for reviewer acceptance. This report
is not permission to merge or begin P2.

## NewLang-aware C observations

- **C-specific friction:** owned allocations require manual partial cleanup,
  temporary realloc ownership and destroy discipline. Const views communicate
  immutability without enforcing lifetimes. Test-only linker wrapping supplies
  deterministic OOM evidence without distorting production interfaces.
- **Compiler architecture requirement:** any implementation language needs
  stable source storage, explicit source -> lexer borrow lifetime, canonical
  spans independent of display coordinates, and capability/error separation.
- **Possible NewLang design pressure:** source owner -> borrowed lexer and
  non-owning token/span values look natural; these examples may later test
  scoped borrow/non-escape ergonomics. No NewLang semantics implemented or
  proof of ergonomic difficulty claimed; no redesign proposed.

COMPILER-IMPLEMENTATION choices are opaque owned source, stack non-owning
lexer, explicit failure statuses and sticky unsupported spans. Linux-only
test linker wrapping is a COMPILER-PORTABILITY boundary within the existing
Linux x86_64 target; no compiler extension is used in production C. No
COMPILER-LOWERING/DIAGNOSTIC/PERFORMANCE defect discovered. Lexical spec holes
remain explicit adjudication work, rather than silently resolved in C.

## Completion and P2 readiness

All P1 criteria **A–AW** have implementation, documentation, local matrix and
fresh PR CI evidence, subject to rechecking the final report-only revision's
current-head CI at handoff. Branch/PR are dedicated, main is untouched and the
PR remains open/unmerged. Existing P0 regression and oracle pass. The open
language-level lexical findings are intentionally outside the P1 subset and
do not block this software foundation milestone.

P2 has an engineering foundation for a minimal syntax subset: stable bytes,
streaming atoms, spans, failure contracts and direct tests. Faithful complete
NewLang syntax is **not** ready without lexical/surface adjudication. P2 must
review keyword/identifier/spacing/comment/operator choices and explicitly scope
its subset before implementation. Parser cursor, lookahead, AST, semantic work
and lowering are intentionally absent. P1 stops for ChatGPT/human review.
