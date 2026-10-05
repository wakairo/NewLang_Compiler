# Draft 17.10 review resolution

Track: M

この文書は Compiler Issue #32 / M9.1 で行った、closed nominal sum の constructor / match exact source profile に対する targeted source-surface adjudication を記録する。

normative authority はこの文書ではなく、`docs/reference/CURRENT_SPEC.md` が選択する canonical Draft である。
Draft 17.10 はPR #33でreview・merge済みで、current `main` の `CURRENT_SPEC.md` が選択している。

## Scope

Draft 17.10 は Draft 17.9 の ownership / lifetime / dependency / occurrence / sum-value semantics を変更しない。

閉じるのは一つだけ:

> semantic contextで既知のclosed nominal sumへ ordinary source を一意に写像するための、最小 exact constructor + match profile。

対象は少なくとも predefined `Option<T>` / `Result<T,E>` と、将来productionがhost registration等でsemantic contextへ供給できる同等のclosed nominal sum。

general sum declaration grammarは固定しない。

## Source decisions

### Qualified constructor

valid closed form:

```text
SumType.Variant
SumType.Variant(expression)
```

- payloadless variantは前者。
- unary payload variantは後者。
- `SumType` はsurrounding type source syntaxで表現でき、semantic analysisで具体的なclosed nominal sum typeへ解決されるtype form。
- variant candidate setはそのsum自身のvariant setだけ。
- constructorはordinary function / associated-function / member lookupへfallbackしない。
- unqualified variant inference、ADL、overload ranking、specializationを追加しない。
- payload expressionはordinary expression/value-use。
- successful expression typeはexactly resolved sum type。

unknown variant、wrong-sum variant、payload arity mismatchはconstructor semantic errorとして扱う。
source spellingの失敗を別lookupで救済しない。

### Match expression

exact closed form:

```text
match expression {
    Pattern => { ... },
    Pattern => { ... },
}
```

grammar:

```text
match_expression :=
    'match' expression '{'
        [ match_arm (',' match_arm)* [','] ]
    '}'

match_arm :=
    sum_pattern '=>' lexical_block
```

- arm bodyは§19.1 lexical block。
- arm separatorはcomma。
- trailing commaは可。
- newline / CRLFはwhitespaceでありseparatorではない。
- `=>` は隣接した `=` + `>` のcontextual two-character punctuator。`= >` は不受理。
- empty arm listはsyntaxとしてparse可能。variantを持つsumではexhaustivenessによりsemantic reject。
- duplicate / missing armはsyntax errorへ落とさずsemantic exhaustiveness error。

### Exact v0 patterns

本profileは次だけ:

```text
Variant
Variant(binding)
Variant(_)
```

- payloadless variantはbare `Variant`。
- payload variantは `Variant(binding)` または `Variant(_)`。
- variantはscrutinee static nominal sum typeのvariant setだけから解決。
- bindingはselected arm bodyだけのfresh binding。
- `_` はpayload-discard markerだけ。standalone wildcard armではない。
- general wildcard、qualified pattern、nested / OR / literal / range / aggregate pattern、guard、implicit ref/mut modeは導入しない。

### Consuming / borrowed mapping

新しいmode keywordは導入しない。

- ordinary `Sum` value -> consuming match。
- `ref<read,Sum>` / `ref<write,Sum>` -> borrowed match。
- direct `ptr<Sum>` -> match対象ではない。
- implicit borrow / derefは行わない。

borrowed `Variant(binding)` のbindingはparent ref modeを継承したpayload refで、既存§26どおりcurrent payload occurrence dependencyを持つ。

## Representative witnesses

### W1 — Result consuming match / terminating error arm

function declaration grammarはM9.1の対象外なので、以下は既存function body内にあるsource fragmentとして読む。

```text
let header =
    match parse(input) {
        Ok(h) => {
            h
        },
        Err(e) => {
            return Result<Packet, Error>.Err(e)
        },
    };
```

解釈:

- `parse(input)` は一度だけ評価。
- ordinary non-Copy `Result<Header,Error>` ならconsuming match。
- `Ok(h)` はactive success payloadをfresh `h` へtransfer。
- `Err(e)` はpayloadをfresh `e` へtransferし、qualified constructorでerror resultを作ってreturn。
- terminating Err armはnormal match-result type / availability joinに参加しない。
- `?` sugarは不要。

### W2 — borrowed payload occurrence dependency

`state_ref : ref<write,Option<T>>` を仮定する。

```text
match state_ref {
    Some(payload) => {
        replace(payload, next_value);
        use(payload)
    },
    None => {
        unit
    },
}
```

`payload : ref<write,T>` はcurrent `Some` payload occurrenceに依存する。

- payload-only `replace(payload, ...)` はそのoccurrenceをpreserveするので、既存ruleで許容可能。
- 同じarm内で `replace(state_ref, Option<T>.None)` のようなwhole-sum transitionを行い、`payload` dependencyがsurviveするならreject。
- aliasing broad write callがoccurrence preservationを証明できない場合もconservative reject。
- source profileはdependencyをptr provenanceへ変換せず、hidden lifetime/effect annotationも追加しない。

### W3 — non-Discardable payload

`Option<Storage>` を例にする。

consuming transfer:

```text
match storage_option {
    Some(s) => {
        consume_storage(s);
        unit
    },
    None => {
        unit
    },
}
```

は `Some(s)` がpayloadをfresh bindingへtransferするため、payloadがnon-Discardableでも成立し得る。

一方:

```text
match storage_option {
    Some(_) => {
        unit
    },
    None => {
        unit
    },
}
```

はconsuming matchではactive `Storage` payloadをdiscardするためreject。

`ref<read,Option<Storage>>` をscrutineeとするborrowed `Some(_)` はpayload valueをdiscardしないため、`Discardable(Storage)` を要求しない。

whole-sum `store` のapplicabilityはこのpattern choiceと独立し、引き続きsum type全体のstatic `Discardable` を使う。

## Required ambiguity attacks

1. **constructor -> ordinary function lookup fallback**  
   Reject. Qualified sum-constructor syntaxはsum constructorとしてのみ解決し、失敗時にfunction/associated/member lookupへfallbackしない。

2. **same-name unqualified variant across sums**  
   No ambiguity. Constructorはsum typeを明示し、patternはscrutinee static sum typeだけをcandidate sourceとする。unqualified constructor inferenceは導入しない。

3. **payload arity mismatch**  
   Reject semantically after variant resolution。payloadlessはbare form、payload variantはexactly one expression form。

4. **pattern variant resolved outside scrutinee sum**  
   Reject. Other sum / function / associated lookupをcandidateにしない。

5. **duplicate / missing / wrong-sum arm classification**  
   Duplicate/missingはsemantic exhaustiveness error、wrong-sum/unknownはsemantic pattern-resolution error。syntax errorへ潰さない。

6. **`Variant(_)` expands to general wildcard**  
   Does not. `_` はvariant payload positionだけのdiscard marker。standalone `_` armはgrammar外。

7. **borrowed match introduces implicit borrow/deref**  
   Does not. Borrowed modeはscrutineeのstatic typeが既にordinary refである場合だけ。

8. **consuming match creates partial-move ordinary binding**  
   Does not. non-Copy whole sumをconsumeし、selected payloadをfresh arm bindingへtransferする既存§26 semanticsを維持。

9. **arm separator/newline conflicts with §19.1**  
   No conflict. Outer match arm listはcomma-separated、arm body内部だけ§19.1 semicolon/tail rule。newlineは両方でwhitespace。

10. **match result / availability join altered by surface sugar**  
    Does not. normal armのexact result type join、terminating-edge exclusion、outer availability agreementをそのまま使用。

11. **payload occurrence dependency collapsed into ptr provenance**  
    Does not. borrowed bindingはref capability + occurrence dependency。persistent ptr semanticsは§26.19–20のまま別。

12. **constructor qualification adds generic inference/specialization**  
    Does not. `sum_type` は既にsourceで表現され解決されるconcrete type formを要求し、本profileはtype inference / specialization / rankingを追加しない。

## Findings

- **M9.1-SURFACE-ONLY:** constructor qualification、match punctuation/list grammar、pattern source formsをexact化した。既存§26 semanticsは変更していない。
- **M9.1-SCOPE:** general sum declaration grammar、general pattern language、qualified pattern、function/type/full-file frontendは意図的にscope外のまま。
- **M9.1-SEMANTIC-AMBIGUITY:** none identified.
- **M9.1-SEMANTIC-CONTRADICTION:** none identified.
- Deep Research / targeted precondition: not required.

## Non-goals retained

Draft 17.10 / M9.1 は以下を行わない。

- general sum declaration grammar;
- general pattern language;
- `if let` / `while let` / `?`;
- guards / OR / nested / literal / range patterns;
- general wildcard arm;
- new ownership / lifetime / dependency mechanism;
- source-visible lifetime/effect annotation;
- full function/type declaration frontend;
- full-file/module frontend;
- production parser/checker work;
- FormalProof work;
- relocation source/API;
- LLVM / FFI / concurrency / modules;
- broad generic redesign。

## Disposition

M9.1はsource-only closureとしてreview・merge済みでCLOSEDである。
このclosure自体はP/F/Rおよび次M milestoneを開始しない。
