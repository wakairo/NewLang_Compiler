# North Star V1 — revised typed-scalar data spine contract

Track: P

## Authority / scope

開始時 main は `b0ce91187396797040211824a9bc49c3073007a2`。
`docs/reference/CURRENT_SPEC.md` → Draft 17.17 が正本。
Issue [#103](https://github.com/wakairo/NewLang_Compiler/issues/103) の
[Step 15裁定](https://github.com/wakairo/NewLang_Compiler/issues/103#issuecomment-6033161662)
に従い、mandatory acceptance は **P1 / N1 / N2**。
**SEMANTIC DELTA = 0**。

Draft §5.1 の既存 core `u8` と §6 の typed integer literal を接続する。
positive decimal `u8(DIGITS)` の bounded profile だけを追加し、mathematical
integer を target range `0..255` で検査する。wrap / truncate / modulo はしない。
bare integer は ordinary expression にしない。

**`u8(-1)` は R3 / Deferred / unchanged**。その source surface、rejection phase、
negative unsigned semantics は今回決定しない。専用処理・test・成功証拠を追加しない。
この実装は complete integer literal grammar を定義しない。

## Data / ownership boundary

```text
actual fn source
  → dedicated NL_SYNTAX_U8_LITERAL (unchecked decimal span)
  → existing semantic checker (core u8 / range validation)
  → NL_CHECKED_U8_LITERAL (immutable type + known value)
  → existing local receiving / Copy identifier use / Discardable statement
  → bounded Checked-C (const uint8_t local / identifier use)
  → strict host C17 compile → native success
```

parser は range を決めず、`u8(256)` も literal node として parse する。
checker は decimal accumulator を常に255以下に保ち、次の digit で範囲を越える場合
`V1-U8-LITERAL-RANGE` を返す。host整数overflowも、C conversionによる検査もない。
source tokenはDIGITSのみで、accepted valueは既存ValuePackageの
`scalar_known/scalar_value` とchecked nodeの `scalar_result` に保存する。
新しい general scalar / constant IR は導入しない。

checked fragment は既存のownership契約に従う。literal nodeはsyntax pointerを
保持せず、semantic type / valueを持つ。registered bodyは既存のowned plan sourceを
保持し、元のregistration source/treeを破棄してもchecked call body evidenceを得られる。
Copy useとscope終了は既存のbinding/checker経路を使う。

emitter はchecked nodeのkind / type / scalar value / resolved symbolを読む。
syntax tree、literal source bytes、decimal parserはemitterに渡さない。
C local名はnominal symbol IDから生成し、source binding名をC identifierに転用しない。
Cの`uint8_t` / `const`はexecution representationであり、NewLang ABI/layout/
conversion/promotion保証ではない。unused-local警告を抑える`(void)local;`は
language cleanupや追加のsemantic useではない。

backend subset全体を検証してから初めてstdoutへCを書き出す。
language-validでもsubset外ならexit 4 / `V1-BACKEND-UNSUPPORTED`。
NewLang-side rejectionならexit 3 / no C bytes。native artifactはdriverでは生成せず、
test/clientがaccept後だけhost compilerを起動する。

## Fixed acceptance

P1:

```newlang
fn main() -> unit {
    let x = u8(7);
    x;
    unit
}
```

- E1: `u8_body.unit` がactual source declarationをregister/checkし、owned checked
  bodyのliteralにcore u8 / value 7を確認する。bindingとidentifierのsymbol一致、
  `NL_VALUE_COPIED`、Copy packageのvalue 7も確認する。hidden prelude / seedなし。
- E2: integrationがchecked value 7のC initializerとsource identifier useを確認する。
  decimal `007` の追加controlはCにsemantic `7`を出し、source spellingをコピーしない。
  これはemitterがsourceを読まないarchitectureと組み合わせた証拠である。
- E3: deterministic Cを`-std=c17 -Wall -Wextra -Wpedantic -Werror`でcompileしnative
  exit 0 / stdout・stderrなしを確認する。NewLang I/O / integer exit semanticsなし。

N1: `let x = u8(256);` → NewLang checker range reject。C/native生成なし。
N2: `let x = 7;` → 既存 `P5-EXPECTED-EXPRESSION`。C/native生成なし。
N3はDeferredでacceptanceから除外。testを追加しない。
optional checker boundariesは `u8(0)` / `u8(255)`。

## Failure / regression / stop

semantic rejectionはcontext rollbackとdeterministic diagnosticを維持する。
既存malloc/realloc fault injectionでsource block check、function-unit registration、
real direct-call body checkingを全allocation位置で失敗させ、artifactなし・public state
不変・cleanupを検査する。既存のresource fence / transactional commitを再利用する。

V0 single unit function / acyclic direct callを保持し、unknown callee / incompatible
returnのchecker rejectionとno C/native invariantを既存testで継続する。
`oracle.adapter` / `oracle.smoke` / `artifacts.integrity` は変更しない。
overlap classificationはexplicit / input-based / fail-closedのまま。

aggregate、Node、sum、Allocation/Storage、ptr/ref、他integer family、signed literal、
arithmetic、conversion、mutable binding、generic IR/MIR、LLVM、FFI、modules、cJSON、
F3.1、V2+を追加しない。review-readyで止まり、自動merge・次slice開始は行わない。
