# Draft 17.9 review resolution

Track: M / Coordination

この文書は P5 Phase A（Issue #23）で確認された source-surface gap に対する targeted adjudication を記録する。
normative authority は、この文書ではなく `docs/reference/CURRENT_SPEC.md` が選択する canonical Draft である。

## Scope

Draft 17.9 は Draft 17.8 の ownership / lifetime / dependency / raw-storage / aggregate-value semantics を変更しない。

閉じるのは次の三点だけ:

1. multi-result operation の dedicated receiving source form;
2. lexical block の non-tail item separator と tail-result 判定;
3. registered fixed-shape nominal aggregate の最小 construction / whole-value destructuring source form。

## Adjudication

### Multi-result receiving

```text
let (a, b, ...) = expression;
```

を dedicated result receiver list とする。

これは tuple value / tuple type / general tuple pattern ではない。
receiver countとchecked result countはexactly一致し、RHS成功後に全receiver bindingをatomicに成立させる。

この形により `take(p, ending)` の `T` と `slot<T>` を別々のaffine responsibilityとして受け取れる。

### Lexical block sequencing

closed subsetでは:

```text
{
    item;
    item;
    tail_expr
}
```

を用いる。

newlineはwhitespaceのまま。
non-tail itemは `;` で終了し、最後のsemicolon無しexpressionだけがtail result。
tail無しnormal completionは `unit`。

`expr;` はresult discardを意味するが、ordinary `Discardable` ruleを迂回しない。

### Aggregate route for P5

P5がsum/match exact surfaceを新たに決める必要をなくすため、registered fixed-shape aggregateについて:

```text
Pair { a: expr_a, b: expr_b }
let Pair { a, b } = expr;
```

をclosed source profileとして固定する。

construction / destructuringはいずれもwhole-value semanticsだけを使い、
partial move / partially-live target / aggregate declaration grammarは追加しない。

## Non-goals

Draft 17.9 は以下を固定しない:

- sum declaration / constructor / match exact source syntax;
- aggregate declaration grammar;
- nested/rest/renaming patterns;
- tuple types/values;
- general statement grammar / newline termination;
- function declaration grammar;
- M9 semantics;
- F1.5 relocation;
- LLVM lowering;
- FFI / concurrency / modules.

## P5 disposition

Draft 17.9 がcanonicalになった後、P5 Phase A blocker A/B/C はこのclosed source profileの範囲で解消したものとしてよい。

P5は:
- multi-result receiverをtupleとして実装しない;
- block newlineをterminatorとして扱わない;
- representative M8.4 routeにはregistered aggregate profileを使う;
- sum/match exact source surfaceをP側で発明しない。

P4/R1/F1.4およびSemantic Sync #22はreopenしない。
