# North Star Aggregate Value Slice — AVS contract

Track: P

## Authority / bounded D2 surface

Issue [#105](https://github.com/wakairo/NewLang_Compiler/issues/105)だけを実装する。
base mainは `4428dbef0acf4c66d3fcdb013df8ce33d7eb2902`。
`CURRENT_SPEC.md` → Draft 17.17が正本。運用方針に従いIssue/PRへevidenceを記録する。
**SEMANTIC DELTA = 0**。

許可されたD2 source plumbingのexact profile:

```text
one optional leading declaration:
  struct NAME { LABEL: u8, LABEL: u8 [,] }
followed by one or more existing fn declarations
```

NAME / LABELは既存WORDで、NAMEには既存ordinary-name admissionを適用する。
LABELはfield labelでありordinary binding nameではない。`struct`はこのtop-level
profileで認識するが、新しいlexer-wide keyword reservationは導入しない。
二つのcore-u8 fields、一つのleading declaration、一つのsource input unitに限定する。
trailing commaはoptional、declarationの後にsemicolonは置かない。
これを一般/最終aggregate declaration grammarとは扱わない。
late/multiple declarations、他field型、generic/recursive/nested declarationsや複数input
間のtype orderingはprofile外としてunsupported。language validityを新しく決定しない。
既存fn-only複数input、order-independent fn signaturesは維持する。

## Semantic registration / checking

source declarationをfunction-unit registrationのprivate candidate内で既存
`nl_semantic_register_aggregate`へ渡す。全bodyが成功したときだけnominal identity、
field names/types、signatures、owned body plansを一括commitする。host-only登録を
positive witnessに使わない。名前/fieldsはregistryがcopy/ownし、input source/tree破棄後も
維持される。Copy/Discardableは既存structural trait ruleでfieldsから導出する。

function bodyでは二つのu8 fieldを持つCopy/Discardable aggregateだけを新たにadmitする。
§16.2 constructionと§16.1 whole destructuringの既存checker/carrier経路を使う:

- named fieldsをdeclared identityへ対応付け、全field exactly onceを検査する。
- initializerはsource順にleft-to-rightでcheckする。field indexとsource順は別である。
- complete aggregate ValuePackageを作り、partially-live field-placeを作らない。
- Copy aggregate RHSは一度value-useされ、source bindingはAvailableのまま。
- copied whole packageをdestructureし、u8 member packagesをfresh bindingsへ一括receiveする。
- unknown/duplicate/missing fieldsや途中の失敗はcandidate全体をrollbackする。

aggregate parameter/result signature、他aggregate shapes、member expressionやmutationを
function/backend profileへ広げない。既存programmatic P5 aggregate能力は維持する。

## Checked authority / ownership

既存checked representationに必要な情報があるためgeneral IRや新しいaggregate node
hierarchyを作らない:

- AGGREGATE: nominal `type`、complete result package、checked fieldsのlinked source順。
- AGGREGATE_FIELD: declaration-order `field_index`、checked initializer、u8 type。
- AGGREGATE_BINDING: checked RHSのsame nominal typeとCopy use、ordered receivers。
- RECEIVER: resolved `field_index`、u8 type、nominal symbol identity。
- u8 literal/use: V1のchecked scalar value / identifier evidence。

`nl_semantic_aggregate_field_view`はassociated semantic contextのdeclaration-order
name/typeをread-onlyで返す。nameは次のsuccessful mutation/destroyまでborrowed。
backendはtypeだけを読み、source field nameをlookup/reparseしない。invalid selectionはoutを
変更しない。physical offset、padding、layoutは返さない。

registered checked bodyは既存owned plan sourceをretainする。元のsource/tree破棄後の
actual checked callからE1-E3を検査できる。artifactから公開semantic IDの意味を得る際は
関連contextのlifetimeを維持する。

## Bounded Checked-C

既存V0/V1 emitterにだけ追加する。supported subset:

- 一つのnominal two-u8 aggregate type。
- immutable aggregate localのcomplete literal initializer。
- checked u8 literal/local readだけのeffect-free field initializers。
- Copy aggregate local identifierをRHSとするwhole destructuring。
- u8 receiver locals / scalar identifier use。

synthetic `nl_type_ID` / `f0,f1` / `nl_local_SYMBOL`を生成する。
whole RHSを一度C temporaryへcopyし、そのvalueからreceiver localsを生成する。
checked field source順を保持してdesignated initializerを出力する。supported C initializerは
effect-freeなのでCのinitializer evaluation-order差でNewLangの効果順序を変更しない。
body-sensitive call等のricher initializerはchecker ACCEPTでもbackend unsupportedとする。
これをhidden helperで近似しない。

whole backend subset validationの後だけC stdout emissionに入る。
semantic rejectionはexit3、backend unsupportedは既存`V1-BACKEND-UNSUPPORTED` / exit4。
どちらもC bytesを出さず、host compileへ進めない。reject後に生成物を削除する方式ではない。

C structはexecution representationのみ。NewLang-visible offset/padding、native == C layout、
ABI identity、repr(C)、packed保証やfield addressは一切導入しない。

## Acceptance / regression / stop

positiveはIssueのactual Pair declaration + construction(7,9) + whole destructuring + u8 use。

- E1: source登録からone nominal Pair、fields 0=left/u8、1=right/u8、Copy/Discardable。
- E2: same type / checked scalar7,9 / resolved field indices。reordered source controlも検査。
- E3: same nominal RHS、Copy use、receiversのu8/value7,9とsymbol/field index。
- E4: checked evidenceのみからsynthetic struct/init/destructuring/use。source名はCへ転用しない。
- E5: deterministic generated C17をstrict host compileしnative成功。

mandatory N1 unknown field、N2 missing field、N3 duplicate field、N5 unknown destructuring fieldは
既存`P5-AGGREGATE-FIELD` / `P5-AGGREGATE-FIELD-COUNT`でreject、no C/native。
第二scalar familyをwrong-type negativeのために追加しない。

parser / registration / real construction-destructuring callの全malloc/realloc fault位置で
partial owner破棄・artifact非公開・public state不変・再試行成功を確認する。
V0/V1、oracle.adapter、oracle.smoke、artifacts.integrity、全compiler/sanitizer/Release/formatを維持。
`u8(-1)`はDeferred / unchanged、contract/testを追加しない。

S1-S7が必要ならCoordinationへhand backして停止する。
review-readyでも自動merge/Issue close/next slice開始はしない。
field mutation / ptr/ref / recursive Node / cJSON / LLVM / F3.1等は今回実装しない。
