# Pre-P1 Source Input and Location Contract

**Status:** production-compiler implementation contract for P1 foundation  
**Authority:** Class I implementation policy; not normative NewLang source syntax  
**Normative baseline:** Draft 17.4 remains authoritative

## 0. Why this document exists

P1 will begin introducing source-facing compiler components. Draft 17.4 does not yet fully
fix source-file encoding, BOM handling, embedded NUL behavior, Unicode identifier policy,
or all newline rules.

Those questions must not be silently decided by the C implementation.

At the same time, source storage, spans, diagnostics, lexer APIs, AST nodes, and later
semantic diagnostics need a stable internal location model before implementation volume grows.

This document therefore fixes the **compiler representation boundary** that is safe to fix now,
while leaving unresolved source-language acceptance rules explicitly open.

## 1. Source ingestion preserves bytes

The source-ingestion layer reads an input as an exact byte sequence plus source identity/path.

It must not:

- normalize newline bytes,
- strip a UTF BOM,
- validate or transcode UTF-8,
- reinterpret embedded NUL as end-of-input,
- replace invalid byte sequences,
- silently change source bytes for lexer convenience.

All source APIs are length-based. They must not depend on C-string termination for source
contents.

A trailing sentinel byte may be allocated privately for implementation convenience only if it
is outside the logical source length and can never be confused with an input byte.

## 2. Canonical internal span

The canonical compiler source location is a half-open byte span:

```text
source identity
start_byte
end_byte

0 <= start_byte <= end_byte <= source_length
```

The represented source region is:

```text
[start_byte, end_byte)
```

Byte offsets refer to the **original preserved input bytes**.

This representation should be used by lexer/parser/AST/semantic structures unless a component
has a concrete reason to use another local representation.

Line/column coordinates are a derived presentation view, not the canonical semantic location.

## 3. Empty and point spans

An empty span where:

```text
start_byte == end_byte
```

is valid and may represent an insertion point, end-of-input diagnostic, or zero-width parser
location.

A missing/unknown source location remains distinct from an empty known span.

## 4. Source identity

A span must be meaningful only together with the source buffer to which its offsets belong.

P1 need not create a general global SourceId allocator before multiple-source compilation needs
it. A module-local source object/reference is sufficient.

Do not infer source identity from path-string equality. Paths are presentation/input metadata,
not semantic object identity.

## 5. Line/column derivation

Diagnostic rendering may derive one-based line/column coordinates from the canonical byte span.

The existing `NLSourceRange` is a renderer-facing representation and need not become the
canonical location stored throughout the frontend.

Until source-text Unicode/display rules are normatively settled:

- byte offsets remain authoritative internally,
- line/column conversion is presentation logic,
- no semantic rule may depend on a display column number.

For physical line indexing, the implementation may recognize LF and CRLF for diagnostic
presentation. It must preserve the underlying bytes. A lone CR must not silently acquire
language-level newline semantics merely because diagnostics can display it sensibly.

If display-column behavior for tabs or Unicode becomes user-visible, specify and test it
deliberately rather than letting terminal behavior define it accidentally.

## 6. Encoding, BOM, NUL, and non-ASCII are not silently decided here

This contract deliberately does **not** declare any of the following to be normative NewLang
source-language rules:

- UTF-8 is required,
- arbitrary bytes are valid source text,
- UTF BOM is accepted/rejected/ignored,
- embedded NUL is accepted/rejected,
- identifiers may contain Unicode,
- non-ASCII bytes are lexical errors,
- lone CR is a language newline.

Until the semantic/source-surface track fixes such a rule, a production feature that encounters
one of these cases must either:

1. implement behavior already justified by a normative rule, or
2. classify/report the missing decision as `COMPILER-SPEC-HOLE` or
   `COMPILER-SPEC-AMBIGUITY`, or
3. mark the case unsupported at the current implementation milestone.

Do not convert an implementation convenience into a language rule.

## 7. Lexer implication

The lexer should consume a source byte buffer and produce tokens with canonical half-open byte
spans.

The lexer must not own diagnostic rendering coordinates as its primary location state.

If the initial P1 lexical subset is ASCII-only, bytes outside that subset should be handled
according to the milestone's explicit supported/unsupported rule rather than being accidentally
accepted through locale-dependent C character functions.

Use locale-independent byte classification. Do not pass negative plain `char` values to
`ctype.h` functions.

## 8. Overflow and bounds

Source lengths and byte offsets use `size_t` unless a later representation decision supplies
a stronger reason.

All span construction/manipulation must maintain:

```text
start <= end <= source_length
```

Arithmetic used to advance offsets must not rely on unsigned wraparound.

Invalid internal spans are compiler implementation defects; invalid user source is not.

## 9. Test implications

The future source-buffer module should have direct unit tests covering at least:

- empty source,
- ordinary ASCII bytes,
- embedded NUL preserved within logical length,
- LF bytes preserved,
- CRLF bytes preserved,
- non-ASCII bytes preserved,
- exact length,
- exact byte slicing,
- valid empty spans,
- bounds rejection/assertion according to API contract.

These tests establish source-storage behavior, **not** that every preserved byte sequence is a
valid NewLang program.

Lexer tests should separately establish which byte sequences the current language/milestone
accepts.

## 10. Relationship to diagnostics

A future diagnostic adapter may convert:

```text
NLSource + NLSourceSpan
```

to the existing renderer-facing:

```text
NLSourceRange
```

The renderer can remain independent of the frontend's canonical storage model.

Do not eagerly replace the P0 diagnostic API before a concrete P1 source module requires an
adapter.

## 11. Deliberate deferrals

This document does not fix:

- final source encoding,
- Unicode normalization,
- Unicode identifier policy,
- grapheme/display-column semantics,
- tab display width,
- final newline lexical semantics,
- stable multi-file SourceId representation,
- include/module/import source management,
- source-map/macro-expansion locations,
- incremental source editing.

Those decisions should be made from actual language/tooling requirements.

## 12. Review rule

Any P1 implementation that changes source bytes during ingestion, makes line/column the only
location identity, depends on C-string termination, or silently chooses an unresolved encoding
rule requires explicit review.

The intended invariant is:

```text
preserved bytes
    +
canonical half-open byte spans
    +
derived presentation coordinates
```

with source-language text acceptance remaining under normative semantic control.
