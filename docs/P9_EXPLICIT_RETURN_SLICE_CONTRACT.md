# P9 explicit-return lexical-block slice contract

Track: P。対象はIssue [#60](https://github.com/wakairo/NewLang_Compiler/issues/60)。

## Authorityと範囲

開始mainは`b3d62614cd47da661067d6d92b703336675e605e`。
最初に`NewLang_Project_Development_Process.md`を読み、`CURRENT_SPEC.md`が指す
Draft 17.11の§18.5/18.8、§13.5a、§19.1、§26.24–25/26.30、§27.8–9を確認した。
Draft > backend contract > merged evidence > Issue/prompt > conversationを維持する。
[Phase A audit](https://github.com/wakairo/NewLang_Compiler/issues/60#issuecomment-6009524967)
をproduction変更前に投稿済み。本contractはimplementation evidenceであってnormative仕様ではない。

P8のowned registered ordinary-function body planに、exact source
`return expression;`だけを追加する。source function/declaration grammarは追加しない。
P8のformal validationとactual-state body checkingを維持する。

## Syntaxとcontext

- `NL_SYNTAX_RETURN`は専用block itemで、既存expression childとkeywordからsemicolonまでのspanを保持する。
- ordinary expression item、tailとは区別する。magic call/nameでencodeしない。
- `return;`は`P9-BARE-RETURN`、semicolon無しは`P9-RETURN-SEMICOLON`。
- expression位置の`return`は`P9-RETURN-ITEM`。newline/ASIは無い。
- unitのexact spellingは`return unit;`。`NL_CHECKED_UNIT`は明示literalの観測で、bottomの代用ではない。
- registered ordinary-function内のlexical descendantsだけがcontextを継承する。
  standalone blockのreturnは`P9-RETURN-CONTEXT`。
- callable/loan bodyのsource admissionは依然未対応。source-fragment entryでrejectする。
  P2のloan **header** entryは従来どおりbodyをopaque balanced bytesとして保持し、body check/nonescapeを証明しない。
  これはloan内returnのsemantic acceptanceではない。
- unreachable textもparseするが、return以降のitem/tailはsemantic value-useやnormal state生成に使わない。
  新しいunreachable warning policyは作らない。

## 局所outcomeとfunction exit

`Check`にfunction boundary floors/result、`terminated`、`returned`を保持する。
checked nodeは`terminates`、normal `results[]`とは別の`returned`を持つ。

1. return expressionを一度checkし、通常のCopy/non-Copy value-useを適用する。
2. declared single resultとのexact compatibilityを検査する。
3. ordinary refの直接returnをrejectする。ordinary-ref result signatureもP8 fenceを維持する。
4. 既存P5 scope obligationをfunction-wideに適用する。
   unused non-Discardable valueをcleanupで消さずrejectする。
5. 既存`nl_sem_function_exit`でsurviving value/carrierのscope/place dependencyを検査する。
   P7 reference may-setの各alternativeも従来どおり検査する。
6. terminating item/block/matchはtype 0（**absence**）、normal result count 0。
   `returned`はfunction edgeのpackageでありnormal block resultではない。
7. callerのcall nodeはcalleeの`returned`を普通のcall resultとして受け、caller continuationは終了しない。

共通`function_block`をdefinition、actual call、trusted host-known fixtureが使用する。
`nl_sem_check_function_block`は内部headerのみのcontrolled semantic fixture入口。
trusted boundary floorsを明示し、clone/check/commitで実際のsource returnを検査する。
sourceからfunction contextを捏造するpublic入口ではない。
source/treeはborrowed、返すchecked artifactはownedで、sourceはartifactより長く生存させる。

runtime destructor/drop、storage release、implicit restoreは生成しない。
logical scope-endのDiscardable処理は既存P5のものを共通化しただけである。
call temporaryの終了では、結果sum等のlive ownerへtransfer済みのpayloadを終了させない。

## Bounded function match

P9 body signatureはunit/既存flat plain type/既存ptr/ordinary-ref parameterに加えて、
**flat plain payloadだけのregistered closed sum**を扱う。
core authority、ref payload、aggregate/nested sum payloadはこのextensionへ入れない。

各armをowned guarded candidateでcheckし、全variantのresult/exitを検査する。
`normal_arms`はreturnしたarmを除いた数である。
このbody profileは**zeroまたはone normal arm**を扱う。
zeroではall-return、oneではそのarmだけがcontinuationを検証する。
複数normal statesを勝手に一つへ選ばず`P9-CONTINUATION-PRECISION`でrejectする。

- definitionではsole normal continuation（またはall-return時の任意の検証済みreturn）をformal stateで適用する。
- actual callではconcrete sum variantのarm planをactual candidateに対してcheckする。
- guarded armのcontextやhypothetical IDsはそのowned evidence内だけに留める。
  private post-state/result IDsをpublicへimportしない。
- zero-normal all-return matchにはnormal unit、bottom/never、fake resultを作らない。
- normal ref resultを使う場合もactual reference facts/provenance/scopeを保持する。
- function context外のP6/P7 match/availability/ref joinは既存pathのままである。

一般のCFG/SSA、unknown variantのeffect/result joinは導入しない。

## W1/W2とprecision境界

**W1:** concrete `ResultPacketError`（Copy Packet、non-Copy/non-Discardable Error）を登録する。

```text
{
    let header = match parsed {
        Ok(h) => { h },
        Err(e) => { return ResultPacketError.Err(e); },
    };
    return ResultPacketError.Ok(header);
}
```

両actual variantをproductionで実行する。Errは同じpayload IDをtransferし、
Okだけがheader binding/continuationへ進む。別fixtureで両armがreturnする。
`match parse(input)`を含むrepresentative形もparserで受理する。
実行fixtureは**hostが準備したparsed Resultをparameterに渡すdecoder段階**である。
P8のbody→body call fenceは維持するため、nested `parse(input)` helperを含む全decoderはまだsemantic対応しない。
concrete qualifierは既存P6 surfaceを使い、generic declaration/library parserの実装は主張しない。
このsource/precision制限とcore early-failure pressureの成功を区別する。

**W2:** 現sourceはfunction-local scopeのrefをcaller-visible carrierへ設置・復元できない。
実際のsemantic scope、ref value、carrierを既存internal constructorsで構築する（private field writeなし）。
trusted function boundaryを与えて**同じsource `{return unit;}`**をproduction checkerへ渡し、
依存残存時のreject/diagnostic/rollback、元packageへの復元後のacceptを比較する。
既存P8 fixtureはP7 may-ref dependencyも回帰検証する。

その他のbounded precision:

| 制限 | disposition |
| --- | --- |
| unknown sum variantのactual function match | `P9-MATCH-PRECISION` / COMPILER-PRECISION |
| 複数normal function-match states | `P9-CONTINUATION-PRECISION` / COMPILER-PRECISION |
| pending call/constructor operand内でのtermination | `P9-OPERAND-TERMINATION` / COMPILER-PRECISION |
| nested match、body→body call | P6/P8の既存unsupported fenceを維持 |
| full `parse(input)` helper / generic Result spellingのsemantic instantiation | 既存source/completeness境界。上のconcrete decoder fixtureのみを実装 |
| sourceによるW2 dependency setup | controlled fixtureのみ、source surfaceを発明しない |
| call-site body diagnostic | public call spanへremap。definition/fixture return spanは保持。詳細body traceはDIAGNOSTIC-QUALITYのdeferred項目 |

## Ownership / failureと停止条件

public operationsはexplicit context、candidate clone、成功時だけcommit。
parse/check/OOM/resource failureでartifactをpublishせず、binding/value/place/scope/occurrenceのpublic stateを保存する。
owned body plans、branch contexts、checked treesのretain/releaseとfirst diagnostic規則を維持する。
新しいmalloc/realloc pathsをfault-injectionでsweepする。

P9だけをreviewへ引き渡す。Draftを変更せず、source fn/top-level、recursion/forward-reference policy、
callable/loan closure、general statement/CFG/bottom、F2/R/M、LLVM/relocation/FFI/modules/concurrency/次Pへ進まない。
PRをmergeせずIssueをcloseしない。
