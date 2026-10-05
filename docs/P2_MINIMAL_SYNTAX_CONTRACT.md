# P2 minimal syntax contract (pre-implementation audit)

Baseline: merged P1 main `5a948621f6ddbcf9172f76628ff96cdd59fe6eca`.
Audit date: 2026-10-05 (client timezone). This Class I implementation contract
was written before parser/syntax code. It describes closed fragments, not a
complete NewLang grammar or semantic acceptance.

## Authority and evidence

Draft 17.4 semantics > Backend Contract v0.4 > adjudicated M8 surface decisions
> M7/M7.5 closure evidence > FormalProof evidence > frozen Python oracle >
production implementation. Draft §0 treats examples as conceptual syntax;
§§10–13, 18–19, 27 and 28 constrain meanings/source roles without supplying a
complete grammar. No normative/historical snapshot is amended.

The user-provided P2 handoff explicitly supplies the closed M8.0a/M8.1 minimal
lexical profile, M8.1 capability/loan surface and M8.2 ordinary-call forms.
Those supplied decisions are used here. Standalone M8 adjudication artifacts
were not attached or present in this repository; independent M8 artifact/hash
verification is not claimed. The handoff's exact spellings are recorded below,
without treating them as authority to change Draft semantics.

## Classification

| Class | P2 disposition |
|---|---|
| FIXED / ADJUDICATED | Supplied M8 capability forms, separate read/write and exclusive axes, loan/optional using header and braced lexical body; listed operations remain ordinary calls. Draft §6 does not make bare integers general expressions |
| P2-SUBSET | Four standalone fragment entries, identifiers/free calls/single-name binding, recursive ptr/ref targets, identifier-only loan operands, opaque balanced body, EOF boundary, no trailing call comma |
| DEFERRED | Permanent/global keyword reservation, full Unicode/encoding/identifier/numeric policies, comments, general block items/results/separators, tuple patterns and generic/constructor syntax |
| UNSUPPORTED-BY-P2 | Operators/literals, member/method/postfix calls, generic applications other than ptr/ref, function/control-flow/declaration/file grammar, destructuring, semantic/builtin/IR work |
| SPEC-HOLE | Existing P1 gaps outside the supplied minimal profile remain open. No separator/global reservation/tuple/constructor rule is needed by the fragment boundary |
| SPEC-AMBIGUITY | No conflicting normative rule identified for the supplied subset. Do not interpret missing general grammar as a contradiction |

## Lexical profile and words

Use P1 unchanged: WORD `[A-Za-z_][A-Za-z0-9_]*`, DIGITS `[0-9]+`, single-byte
punctuation `(){}[],:;.+-*/%<>=!&|?`, and skipped SP/TAB/LF/CRLF. NUL,
non-ASCII/BOM/lone CR and `//`/`/*` remain lexically unsupported, including in
an opaque body. No comments, decoding or normalization are introduced.
Words/punctuation are compared through length-based source views, never as
NUL-terminated lexemes. Composite punctuation is not needed. If introduced
later, adjacency must mean `a.end_byte == b.start_byte`; `->` differs from
`- >`. No operator/maximal-munch infrastructure is created now.

No lexer/global reserved-word table: `let` is structural only in binding entry,
`loan` only in loan entry; `exclusive/read/write/using/as` are contextual in
the relevant type/loan production. Ordinary expression names/callees remain
WORD-shaped, even when their spelling has a grammar role elsewhere. Parsing
`loan()` as a generic call-shaped expression is not implementing a loan
operation or declaring that a callable of that name exists. `ptr`, `ref`,
`LifetimeDomain`, `finalize_domain`, `ptr_from_ref` are not global keyword kinds.

## Closed productions

`name` is one WORD atom; literal terminals are compared contextually.
Whitespace/newlines are insignificant between atoms, not statement terminators.

```text
type       := name
            | 'ptr' '<' type '>'
            | 'ref' '<' mode ',' type '>'
            | 'exclusive' 'ref' '<' mode ',' type '>'
mode       := 'read' | 'write'
expr       := name | name '(' [expr (',' expr)*] ')'
binding    := 'let' name '=' expr
loan       := 'loan' ['exclusive'] mode name
              ['using' name] 'as' name body
body       := balanced braced region of supported P1 atoms
fragment   := exactly one requested production followed by EOF
```

In type entry `ptr`/`ref` acquire their compound role when followed by `<`;
bare words are name syntax without builtin resolution. `exclusive` at type
head requests only the closed exclusive-ref form. A target recursively uses
this same type subset. Access mode and exclusivity stay separate syntax axes.
`LifetimeDomain` is name syntax, not a resolved type identity.

Call heads are names, not arbitrary expressions; arguments preserve order.
`LifetimeDomain()`, `ptr_from_ref`, `finalize_domain`, `initialize`, `take`,
`destroy`, `replace`, `store`, `swap`, and future names like `split` all use the
same call node. No builtin arity, constructor meaning or type inference.
Single bindings do not imply Copy/consume or scope semantics.

Loan source/stability are name expressions only. Syntax records optional using,
mode, exclusivity, bound-name span, full span, open/interior/close body spans.
It does not determine source type, whether using is required/forbidden, domain
identity, stability or authority. Body capture balances braces iteratively;
other parentheses/operators/control words inside are not parsed. Capture
success is only exact source-region capture, not validation of body syntax or
semantics and not a macro/token-tree language commitment.

## Failure classification (first failure only)

- OK: one closed fragment plus EOF; no language-semantic acceptance claim.
- LEXICALLY_UNSUPPORTED: the next required P1 atom is unsupported. Preserve
  the lexer span/capability cause, never relabel it as invalid NewLang syntax.
- SYNTAX_UNSUPPORTED: outside P2 coverage, including operators/member/generic
  extensions, DIGITS as expressions, tuple binding, trailing call comma or
  extra tokens after a completed fragment. No permanent language rejection.
- SYNTAX_ERROR: malformed closed shape, missing delimiters/required words,
  invalid ref mode/extra ref arguments, missing call argument/separator, or
  unclosed loan body. This diagnoses the supplied closed grammar, not arbitrary
  future NewLang productions. `exclusive ptr<T>` is an invalid closed exclusive
  ref spelling, not an accepted alternate.
- OUT_OF_MEMORY: checked host allocation failure, partial syntax destroyed,
  owner slot unchanged; not an invalid-source diagnostic.
- RESOURCE_LIMIT: implementation budgets, not NewLang language limits.
- INTERNAL_ERROR: invalid module/API arguments or impossible lexer state.

Examples: `ptr<>`, `ptr<T`, `ref<T>`, `ref<read>`, `ref<read,T`,
`ref<exclusive,T>`, `ref<read,write,T>`, `exclusive ptr<T>` are syntax errors
in their attempted closed type shapes. `Foo<T>` is an unsupported generic
extension. `loan read x r {}` is missing required `as`; `let (x,y)=take(p,d)`
is unsupported pattern grammar. Recovery/backtracking are out of scope.

## Representation, resources and diagnostics

Every node/full construct and significant name has canonical half-open byte
spans; children are contained in parents. Source storage is unchanged and
borrowed throughout. Names are spans, never interned/copied strings.
One concrete syntax owner frees all allocated nodes iteratively (including
partial trees), avoiding recursive destruction and any general arena/container
framework. Parser uses a streaming lexer and one lookahead token, resetting
for each standalone fragment; independently returned trees are immutable.

Resource budgets: at most 128 recursive type/expression levels, 128 body brace
levels and 4096 syntax nodes per fragment. Report RESOURCE_LIMIT with a bounded
span and COMPILER-IMPLEMENTATION-LIMIT classification, not a language limit.
Zero-width EOF spans remain valid. Syntax errors/unsupported are never asserts.

Diagnostics retain a structured NLDiagnostic code/category/message and a
canonical primary span. A small renderer adapter derives one-based byte
line/column presentation: LF starts a line, CRLF completes one newline, tabs
and lone CR consume one byte column. No Unicode/display-width or lexical
semantics is inferred from that presentation policy. First-error-only records
own no strings/source; static diagnostic strings and explicit source lifetime.

The public CLI remains compile-unsupported. No parser/file/AST dump mode,
resolver, semantic facts, typed MIR, LLVM frontend coupling or P3 implementation.
