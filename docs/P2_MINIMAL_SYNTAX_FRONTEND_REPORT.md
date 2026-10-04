# P2 — Minimal Syntax Frontend

Status: implementation/local validation complete; P2 review gate. No merge or P3
work is authorized by this milestone. Branch: `p2-minimal-syntax-frontend`.
Base: main `5a948621f6ddbcf9172f76628ff96cdd59fe6eca`, the merged P1 PR #3.
PR/current-head CI evidence will be added after publication.

## Inputs, authority and audit

The user-supplied P2 handoff (`Pasted text.txt`, SHA-256
`c5b72a4057d25f82043337930a7e3af9c2e75fc5557fe49edb93aa2472327531`)
defines this task and explicitly supplies the closed M8.0a/M8.1 lexical profile,
M8.1 capability/loan forms and M8.2 call-shaped surface. Standalone M8 artifacts
were not attached or present: their independent contents/hashes are not claimed
verified. The authority order is Draft 17.4 > Backend Contract v0.4 > adjudicated
M8 surface > M7 closure > formal evidence > frozen oracle > implementation.

The required C, testing, review, Pre-P1 source/location, P1 audit/report and
source/token/lexer interfaces/implementations were reviewed against unchanged
merged P1. Relevant Draft surface/type/function/control/location sections and
Backend Contract constraints inform the scope. The
[grammar contract](P2_MINIMAL_SYNTAX_CONTRACT.md) was written before code, with
FIXED/ADJUDICATED, P2-SUBSET, DEFERRED, UNSUPPORTED, SPEC-HOLE and SPEC-AMBIGUITY
separated. Historical references, oracle, P0/P1 reports and dependency pins
are unchanged. Baseline bootstrap/format and GCC/Clang each passed 13/13 tests.

## Closed grammar and lexical boundary

Four public entry points each reset to source start and require one fragment
plus EOF. No compilation-unit grammar, separator policy, recovery or
backtracking is inferred.

```text
type    := name | ptr '<' type '>' | ref '<' mode ',' type '>'
         | exclusive ref '<' mode ',' type '>'
expr    := name | name '(' [expr (',' expr)*] ')'
binding := let name '=' expr
loan    := loan [exclusive] mode name [using name] as name body
mode    := read | write
body    := balanced braced region of supported P1 atoms
```

`name` is a WORD. P1 is unchanged: WORD `[A-Za-z_][A-Za-z0-9_]*`, DIGITS
`[0-9]+`, punctuation `(){}[],:;.+-*/%<>=!&|?`, spacing SP/TAB/LF/CRLF.
NUL, non-ASCII, BOM, lone CR, quotes and comment openers `//`/`/*` remain
lexically unsupported. No decoding, normalization, global keyword table or
composite punctuation is introduced. Future composite adjacency must compare
canonical span endpoints, not whitespace-stripped spelling.

`let`, `loan`, `exclusive`, `read`, `write`, `using`, `as`, `ptr` and `ref`
are matched only in grammar positions. Ordinary expression names/callees are
all WORDs. Bare ptr/ref are type-name syntax; `<` requests their compound form.
Exclusive type-head syntax requests the closed exclusive-ref form. No permanent
global reservation or builtin existence is decided.

LifetimeDomain(), ptr_from_ref(), finalize_domain(), initialize(), take(),
destroy(), replace(), store(), swap() and future names use the same call node.
Argument count/order is syntax only; take() and take(a,b,c) parse without builtin
arity validation. LifetimeDomain() does not introduce constructor syntax.
`loan()` in expression entry is a generic call shape, not the lexical loan
construct or a semantic loan operation.

Loan records mode/exclusivity separately, source name, optional stability name,
bound name and full/open/interior/close spans. Optional using is retained without
checking whether the source needs it. Body capture balances nested braces
iteratively and preserves bytes. Operators, control words and mismatched
non-brace delimiters inside are opaque; lexical unsupported still propagates.
Capture does not validate body syntax/semantics or define a token-tree language.

## Architecture, ownership and diagnostics

`newlang_parser` depends on syntax, lexer/source and diagnostics; syntax depends
on source. Neither library includes or links LLVM. Public CLI behavior remains
the original explicit compile-unsupported behavior.

- `NLParser`: opaque owned state, one streaming lexer, one cached token and
  localized cursor/depth/error state. No token vector, rewind or global state.
  Independent parsers can borrow one source; calls are synchronous/non-reentrant.
- `NLSyntaxTree`: opaque concrete owner for root/all nodes, borrowing source.
  Seven node kinds only: type name/ptr/ref, expression name/call, binding, loan.
  Public immutable views expose kind/spans/children and source roles; call
  arguments are linked in source order. No semantic IDs, checked permissions,
  domain state, value-flow facts, type resolution or LLVM handles.
- Ownership: checked malloc for parser/tree/nodes. A successful parse transfers
  one tree into an initialized NULL slot. Failure destroys partial nodes and
  leaves that slot unchanged. Parser reuse/destruction never invalidates earlier
  trees. Tree destruction is iterative and consumes all nodes, never source.
  Source must remain live for every parser/tree use and text-view access; nodes
  and views become invalid at tree destruction. C cannot enforce these borrows.
- Diagnostics: first structured severity/category/code/message plus canonical
  primary span. Static borrowed strings, no diagnostic allocations. The renderer
  adapter derives byte-based line/column presentation synchronously. LF advances
  lines, CRLF gives one physical newline, TAB/lone CR consume one byte column.
  EOF has a valid zero-width span; no display-width/encoding rule is invented.

Results distinguish OK, LEXICALLY_UNSUPPORTED, SYNTAX_UNSUPPORTED,
SYNTAX_ERROR, OUT_OF_MEMORY, RESOURCE_LIMIT and INTERNAL_ERROR. Lexer failures
preserve unsupported spans/cause. Unsupported extensions (operators, numeric
expressions, member/general generic syntax, tuples, annotations, extra syntax,
trailing call commas) do not assert invalid NewLang. Missing required tokens,
invalid ref modes and malformed closed constructs are syntax errors. OOM and
resource failures use category host, not source syntax. Invalid API inputs leave
output and parser state unchanged; parser remains reusable after parse failure.

Implementation budgets: 128 recursive type/expression levels, 128 body braces,
4096 syntax nodes. These are COMPILER-IMPLEMENTATION-LIMIT, not language limits.
No input-dependent assert, recursive destruction, process abort, unsigned size
growth or lexeme-as-C-string operation is used. No general allocator/arena,
parser macro DSL, universal AST, symbol table or semantic compatibility layer.

## Tests and reproducibility

Six unit groups test type, expression/binding, loan, ownership, limits and
diagnostics through public interfaces. `syntax_check.h` is test-only and checks
public node relationships, containment, spelling and deterministic equivalence;
it does not inspect private storage.

- Positive: all required nested capability types; access/exclusive distinctions;
  bare identifier and zero/multiple/nested calls; all supplied M8 call spellings;
  single bindings; all eight loan mode/exclusive/using combinations; contextual
  words in ordinary names and loan operands; SP/TAB/CRLF variations; nested and
  empty bodies. Assertions inspect kinds, exact spans, names, children/argument
  order, flags and body spans, not acceptance alone.
- Negative: required malformed ref/ptr, missing mode/as/using/binding/body,
  missing call arguments/separators, unclosed delimiters, unsupported patterns,
  numeric/operator/member/generic/function/control grammar, annotations/trailing
  syntax, comment/quote/BOM/non-ASCII/NUL/lone-CR propagation and diagnostic spans.
- Ownership: independent parsers/trees, reuse after success/error, parser destroyed
  before returned tree use, NULL/invalid owner slots and unchanged outputs.
  Test-only Linux malloc wrapping injects each allocation failure until success
  for all four entries, verifies OOM cleanup/host category and successful retry;
  production does not gain an allocator interface.
- Limits: exact 128-level/depth and 4096-node boundaries succeed; over-budget
  nesting/wide calls return RESOURCE_LIMIT with bounded spans and cleanup.
- Integration: binary temporary files -> owned source -> P1 lexer -> each
  parser entry -> public tree views; exact original bytes/spans retained,
  repeated parse equivalence and ownership order exercised. A raw NUL body file
  propagates lexical unsupported. Python only creates fixtures/invokes the
  instrumented C executable; C CHECK remains active under NDEBUG.

All seven new CTests are in normal CTest, bringing 13 existing tests to 20.
Existing source/lexer, diagnostics, CLI, LLVM C API, artifact integrity and oracle
smoke tests remain unchanged. Oracle semantic differential testing is N/A for
syntax-only shapes; frozen M7.5 smoke still runs and is evidence, not grammar.

Local host: Codex Linux x86_64, Debian 13, GCC 14.2.0, Clang/LLVM 23.1.2,
C17, CMake/CTest 3.31.6, Python 3.12.14. Strict flags remain
`-Wall -Wextra -Wpedantic -Werror`, extensions off. Sanitizers instrument all
new production/test C targets; imported LLVM is not rebuilt. ASan leak detection
and UBSan halt-on-error remain enabled.

| Configuration | Result |
|---|---|
| GCC Debug | 20/20 PASS |
| Clang Debug | 20/20 PASS |
| Clang ASan | 20/20 PASS, leak detection |
| Clang UBSan | 20/20 PASS, halt-on-error |
| GCC Release / NDEBUG | 20/20 PASS, CHECK assertions active |
| Locked bootstrap / clang-format check | PASS |

Reproduce using README bootstrap/activation, then configure/build/CTest for the
four existing build directories. Release additionally:

```sh
. .deps/activate.sh
CC=gcc cmake --fresh -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel 2
ctest --test-dir build-release --output-on-failure
```

The unchanged PR workflow automatically discovers all 20 tests for GCC, Clang,
ASan and UBSan on fresh Ubuntu 24.04; Clang additionally checks locked formatting.
It retains exact package/checksum pins and reference integrity validation.

## Findings and P3 handoff

COMPILER-SPEC-HOLE: no new blocking hole in the closed P2 subset. P1 LEX-01..07
remain the historical record of broader identifier/reservation/spacing/comment/
punctuation/numeric/encoding gaps; the task-supplied minimal profile suffices here
without claiming to settle their full-language policy. General block/program,
separator, tuple, constructor and global keyword grammar remain deferred.
COMPILER-SPEC-AMBIGUITY: none confirmed. COMPILER-IMPLEMENTATION: explicit depth/
node budgets and borrow/cleanup contracts; no unresolved defect. No new
COMPILER-LOWERING, DIAGNOSTIC, PERFORMANCE or PORTABILITY blocker. P2 changes
no backend attributes/ABI/layout/lifetime decisions or normative specification.

NewLang-aware feedback:

- C-specific friction: partial-node allocation cleanup and borrowed-source
  lifetime require discipline; one iterative owner and explicit statuses suffice.
- Compiler architecture requirement: syntax spelling/shape/source roles must be
  independent of semantic IDs, access authority, ownership flow and LLVM facts.
- Possible NewLang design pressure: future block/separator/pattern/global-word
  grammar needs adjudication before expansion. C cleanup friction is not evidence
  for adding language destructors or changing normative ownership semantics.

P3 can consume identifiers, ordinary calls, single bindings, capability types
and loan headers directly through immutable views **without reparsing them**.
Loan body contents deliberately remain opaque and require a later adjudicated
body parser before semantic traversal. Full programs outside these fragments are
not ready. Recommendation: P2 ready for review after current-head CI; P3 work
starts only after the human review/merge gate. No P3, semantic checker, typed IR,
LLVM lowering, M8.3 semantics or public dump modes were implemented.
