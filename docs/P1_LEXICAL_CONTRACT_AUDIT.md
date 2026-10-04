# P1.0 lexical contract audit

Performed before implementation against the unchanged Draft 17.4 snapshot on
Pre-P1 main `4d4a2fc2868eebc9e2b4680310aa7cee027c57c6` (2026-10-04).
This is Class I implementation evidence, not a lexical amendment. Authority
remains N language / N backend > A closure > O oracle > F formal > I production.

## Evidence and classifications

Draft §0 explicitly says code examples are conceptual notation and do not fix
final syntax. The title/introduction also permits provisional surface syntax.
The entire document was searched for lexical/identifier/keyword/comment,
encoding/Unicode/BOM/NUL, whitespace/newline and maximal-munch rules; relevant
sections were read in context. No independent complete lexical grammar was
found. Backend Contract v0.4 governs downstream facts, not a missing source
grammar. The Pre-P1 byte/location contract fixes storage only (Class I).

| Item | Classification | Evidence / P1 disposition |
|---|---|---|
| Bare integers are not expressions; typed mathematical integers require target range checks | A FIXED **semantic constraints**, not digit grammar | Draft §6; P1 only scans decimal runs, without value/range interpretation |
| Source paths are metadata, not semantic identity | A FIXED | Draft §28.1; source object plus span, no path-equality identity |
| Identifier character set | D COMPILER-SPEC-HOLE; B subset | §§0, 18.1, 27.1 show names but no character grammar; ASCII word atoms only |
| Keyword reservation/classification | D COMPILER-SPEC-HOLE; C deferred | §§18.1, 19, 27 use spellings; §0 does not establish a reserved/contextual keyword table; all word atoms remain WORD |
| Decimal numeric spelling | B IMPLEMENTATION-SUBSET | §6 shows decimal examples, but not bases/separators/boundaries; DIGITS are uninterpreted runs |
| Whitespace including LF/CRLF/lone CR | D COMPILER-SPEC-HOLE; B subset | §§0, 18.1, 27 examples have spacing; no normative whitespace grammar; skip SP, TAB, LF, CRLF; lone CR, VT, FF unsupported |
| Comment syntax/nesting | D COMPILER-SPEC-HOLE; C deferred | §27.3 contains explanatory `//` lines; not a comment production. `//` and `/*` openers stop scanning as unsupported, never skipped |
| Operators/punctuators and maximal munch | D COMPILER-SPEC-HOLE; B subset | §§5.2–5.5, 18.1, 19, 26.30, 27 illustrate symbols, not scanner boundaries; single-byte atoms only; `->`, `==`, `&&`, `||` are separate punctuation tokens |
| Encoding / UTF-8 validity / non-ASCII identifiers | D COMPILER-SPEC-HOLE; C deferred | No source encoding or identifier Unicode rule found; all bytes retained by ingestion, bytes >=0x80 unsupported by scanner (valid and invalid UTF-8 alike) |
| BOM | D COMPILER-SPEC-HOLE; C deferred | No BOM rule; preserve bytes, report unsupported at first BOM byte |
| Embedded NUL | D COMPILER-SPEC-HOLE; C deferred | No lexical rule; preserve logical length, never treat input NUL as EOF; unsupported at that byte |
| Strings, character literals, escape processing | C DEFERRED | Not necessary for this slice; quotes/backslash unsupported; no invented literal grammar |

There is no A-class **complete lexical** grammar being claimed. The fixed
semantic constraints above guide future work but are not implemented by P1.
No confirmed E COMPILER-SPEC-AMBIGUITY (conflicting normative rules) was found;
multiple possible answers where the spec says nothing are holes, not conflicts.

## Explicit scanner subset

- WORD: `[A-Za-z_][A-Za-z0-9_]*`, described only as ASCII word-shaped atoms.
  `fn`, `let`, `return`, `if`, `else`, type names and `move` receive no semantic
  or keyword status here. The absence of explicit move (§§4.2, 18.2) is not
  implemented as a lexical reserved-word rule.
- DIGITS: `[0-9]+`; sign is a separate punctuation atom. `123abc` yields DIGITS
  then WORD, without asserting that the spelling is a valid literal/program.
- PUNCTUATION: one byte from `(){}[],:;.+-*/%<>=!&|?`. There is no composite
  operator maximal-munch rule. Slash followed by `/` or `*` is instead an
  unsupported two-byte span because comment-like spellings are deferred.
- Skip only ASCII SP (0x20), TAB (0x09), LF (0x0a), and adjacent CRLF
  (0x0d 0x0a). This is scanning support, not a normative newline/display rule.
- Everything else is unsupported, with a one-byte span except the two-byte
  comment openers. EOF is `[length,length)`, solely based on logical length.

Every TOKEN result means an atom was scanned, not that NewLang accepted the
source. UNSUPPORTED stops at a stable span; repeated calls return the same
result until explicit reinitialization. No skipping/recovery policy is implied.
No lexer output changes source bytes or requires a C string terminator.

## Findings for human/ChatGPT adjudication

All findings below are **COMPILER-SPEC-HOLE**, remain open at language level,
and are bypassed only by the explicitly limited P1 subset. Backend relevance:
none (Backend Contract §§1, 11, R separates source rules from LLVM). Oracle:
not queried for a normative lexical answer; frozen M7.5 is lower-authority
semantic reference and the existing smoke remains regression evidence only.
Formal evidence: no lexical theorem/counterexample relied on. Candidate choices
are adjudication inputs, not adopted rules.

| Finding | Minimal bytes/example | Draft section / missing decision | Candidate resolutions |
|---|---|---|---|
| LEX-01 identifiers | `a`, `_a`, UTF-8 `é` | §§0, 18.1, 27.1: which characters/boundaries? | explicit ASCII grammar or reviewed Unicode grammar/normalization |
| LEX-02 keywords | `fn fn()`, `let if = unit` | §§0, 18.1, 27.1, 27.4: reserved versus contextual? | explicit reserved table or contextual parser classification |
| LEX-03 spacing/newlines | `a\tb`, `a\r\nb`, `a\rb`, VT/FF | §§0, 27: separator set and newline significance? | explicit byte separators/newlines or explicit Unicode spacing policy |
| LEX-04 comments | `a//b`, `a/*b*/` | §§0, 27.3: are these comments; terminators/nesting? | specified line/block syntax or no comments/different explicit syntax |
| LEX-05 punctuation boundaries | `a->b`, `a==b`, `a&&b` | §§0, 5.2–5.5, 18.1: token table and maximal munch? | reviewed composite operator table/munch or parser-composed single atoms |
| LEX-06 numeric spelling | `0x10`, `1_000`, `123abc` | §6: base/separators/boundaries missing | explicit decimal grammar or specified additional spelling families |
| LEX-07 encoding/exceptional bytes | `61 00 62`, `ef bb bf 61`, `c3 a9`, `ff` (hex) | §0 / no encoding section: NUL, BOM, UTF-8 and non-ASCII acceptance? | required encoding with explicit exceptions or specified byte-oriented policy |

Source ingestion and span/ownership foundations are independent of these holes.
P1 can complete; faithful full lexical/parser support needs adjudication before
promoting any subset choice to NewLang language semantics. Draft 17.4, backend
contract, historical snapshots and oracle remain unchanged.
