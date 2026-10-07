# Draft 17.22 source punctuation production contract — Issue #152

Track: P

Base main: `aa12a794791135f656f547c9fa2ae901690a809e`。
Canonical: `CURRENT_SPEC.md` → Draft 17.22、live §17.1 / §26.3 / §16.3。
Issue #152 P-A〜P-Dのsource-only migration。core semantic delta = 0。
PR #149 / COORD-149-01を解決するtaskではない。

## Exact source categories

```newlang
struct Pair { left: u8, right: u8 }
fn main()->unit {
    let p=Pair{left:u8(7),right:u8(9)};
    let before=p@left;
    let old=loan_write(p@left){|w|replace(w,u8(11))};
    let after=p@left;
    let sibling=p@right;
    before;old;after;sibling;unit
}
```

lexerは`@`をone-character punctuationとして扱う。`::`は二つのcolon tokenの
source byte spanがadjacentな場合だけconstructor separatorとなる。
`: :` / `:\n:`は同じseparatorではない。qualifier/variant周辺の通常のwhitespaceは
colon同士のadjacencyとは別に扱う。comment syntax等の既存lexer制限は不変。

`local@field`はdedicated `NL_SYNTAX_FIELD_DESIGNATOR`を生成し、borrowed source spans
`base` / `field`を保持する。checkerはcurrent lexical local→exact Pair→declared fieldの
順でだけ解決する。simple localだけを認め、nested/arbitrary-expression/call/ptr/ref base、
method invocationは認めない。`loan_write(local@field)`もこのdesignatorだけを使う。
ordinary localの`loan_write(local)`は既存profileのまま。

`SumType::Variant` / `SumType::Variant(expression)`は既存
`NL_SYNTAX_SUM_CONSTRUCTOR`を生成し、type qualifier→closed sum→own variant→arity/type
だけを解決する。named registered concrete sumsとexact recursive
`Option<ptr<H>>::None` / `::Some(p)`が既存production coverage。
一般`Option<u32>` / `Result<T,E>` generic grammar、u32 literals、general module-qualified
callsは追加しない。historical host-known concrete Option/Result testsの意味論coverageを
new punctuationへ移す。frontend外のgeneric formsを受理したとは主張しない。

fieldとconstructorはparser/AST/checkerで別category。旧DOTTED nodeと
`FIELD-DOTTED-AMBIGUOUS`判定を削除し、相互fallbackしない。同spellingのPair localと
registered sum typeが存在しても`p@left`はfield、`p::left`はconstructorになる。
unknown field/variant/typeでも他category / ordinary functionへfallbackしない。

`.`はlexer punctuationとして残るがsource field/constructor/receiver routeを持たない。
legacy dotは`SOURCE-DOT-RESERVED`等のsyntax/unsupported diagnosticで拒否する。
旧spellingのcompatibility aliasはない。

## Semantic / checked boundary

既存fixed selection/readとordinary field write loanへ接続する。ProjectionId、field type、
parent/child identity/incarnation、pre/post ValueFact、hidden/exact dependencies、result
forwarding/nonescapeを保持する。ordinary writeは非exclusiveで、noalias等を追加しない。
replaceは既存Changeを使い、root/sibling incarnationとdisjoint sibling factを保存する。
checked field metadataから既存C emitterがmember read/pointer writeをlowerする。
source punctuationやC layoutをsemantic authorityにせず、backend/source再解析はない。

constructor payload evaluation/value-use、sum package identity/Copy/discardability、conditional
occurrence、match/Reset、whole destructuringは既存sum checkerへそのまま接続する。
Node declaration/completionとNone-link constructionはchecker-valid。Node field read/write、
recursive Checked-C、runtime topologyはscope外で、explicit backend unsupportedを維持する。
現在のone visibility domainは不変。module/private-field policyのruntime enforcementはない。

## Failure / validation / handoff

既存clone/check/commit transactionとowned syntax/checked artifact boundaryを使う。
parser error/OOMでsyntax treeを公開せず、semantic error/OOMでchecked artifactとpartial
contextを公開しない。fixed-field / sum / recursive登録・bodyのmalloc/realloc sweepsを保つ。
CLI reject-before-emissionでno C / no executableを検証する。

既存176 CTestsのsource fixturesを移行し、assertionを維持する。同じgroupsにlegacy dot、
adjacency、category separation/no fallback、ptr/ref/nested/arbitrary base controlsを追加する。
GCC Debug/Release、Clang+format、ASan、UBSanのfull suite、native Pair、oracle.adapter/smoke、
artifacts.integrityとlocked bootstrapを要求する。frozen oracle/archive、normative Draft、
archival report/evidence、toolchain pinsは改変しない。

OPEN / unmerged PRとexact-head CI evidenceで停止する。
marker: `DRAFT 17.22 SOURCE PUNCTUATION PRODUCTION CATCH-UP READY FOR REVIEW`。
Node link-field、PR #149修正、allocation/lifecycle、cJSON、次sliceは開始しない。
