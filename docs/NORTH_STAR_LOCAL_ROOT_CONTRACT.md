# North Star local-root ptr/ref product gate contract

Track: P

## Authority / scope

Issue [#110](https://github.com/wakairo/NewLang_Compiler/issues/110)のみを実装する。
base main `7cd61f033f4f9c8c7b24c8147d2c6a263312b954`、CURRENT_SPEC → Draft 17.18。
canonical §10.1/§10.2/§11.1/§13.5a/§13.7/§13.8を再利用する。
**SEMANTIC DELTA = 0**。運用は`NewLang_Project_Development_Process.md`に従う。

```newlang
fn main() -> unit {
    let x = u8(7);
    let p = loan_read(x) { |r| ptr_from_ref(r) };
    loan_read_ptr(p) { |r2| ptr_from_ref(r2); unit };
    unit
}
```

このactual-source witnessだけをprimary product evidenceとする。
fixture root/ref/domain、unchecked、prelude/helperの挿入で成功させない。

## Source / semantic boundary

追加syntaxはProvisionalなread-only simple-name loan expressionとreal body。
`NL_SYNTAX_LOCAL_READ_LOAN`はoperand、binder span、BLOCK、direct/ptr modeを保持する。
P2/P3の既存opaque header fragmentはそのまま保持し、body証明と混同しない。
一般place、arbitrary ptr expression、write loanを追加しない。
ordinary name admissibilityは変更しない。same-spelling general callのpolicyは決めず、
profile外のoperand/body-less callは`LOCAL-LOAN-PROFILE` unsupportedで止める。

ordinary source `let`が作るcore u8 placeに`implicit_local`を付ける。
そのPlaceId/current IncarnationIdがnominal implicit governing identityであり、
machine address / explicit LifetimeDomain / raw BackingRegionではない。
domain=0、同じstatic type、ptrの存在だけではmarkerやhidden stabilityを得られない。

loan_readはAvailableなsource local / independent live root / implicit marker / u8 / no
explicit backing/domain / dependency proofを要求し、既存conflict checkを通す。
loan_read_ptrはこれに加えconcrete provenance、current incarnation、target identity、
read accessと、対応するまだvisible/liveなlocal carrierを確認する。
ordinary compiler localのtyped lifetime / valid u8 packageがrepresentation、alignment、
range、ordinary read accessを与える。raw addr、MMIO、conditional subplaceやlayout
不明なexternal storageをこのproof pathへ入れない。

新active scopeとread ref packageを作り、fresh loan-body-local binderへreceiveする。
bodyは既存lexical block checkerを一度だけ通る。returnはloan境界を越えず、outer loop
controlもloanへ持ち込まない。このgateのzero-normal bodyはunsupportedであり新しい
control semanticsを決めない。

normal body result Rとsurviving current-value stateに既存scope-exit checkを適用する。
ending scopeへのref dependencyは`P8-EXIT-DEPENDENCY` semantic rejection。
unknown/hidden dependencyは既存precision fenceで拒否し、dependency-free扱いしない。
exit成功後にgenerated scopeを終了し、その後に同じRのtype/value IDをforwardする。
outer letはそのpackageを受け取り、追加Copyや再mintをしない。
`ptr_from_ref`は既存operationを使用し、ptrのpersistent referent/incarnation/provenanceを
保持する。originating ref scope/occurrenceのblocking loanはptrへ持たせず、その他の
dependency knowledgeは保持する。loan forwarding自体はdependencyを一切変更しない。

## Checked / C boundary

既存`NL_CHECKED_LOAN_HEADER`を最小拡張し、source symbol、referent place/incarnation、
fresh scope、resolved ref symbol、implicit/local-ptr mode、checked BLOCK initializer、
successful nonescape/normal-forwarding evidenceと結果を保持する。
`PTR_FROM_REF`はchecked reference factsを保持する。
backendはAST/source/name textを参照せず、checked symbol/type/value/loan evidenceだけを読む。

C subsetはu8 local、read loan、ref→ptr、local ptr receiving/Copy identifier use、
same-local ptr→read-ref、ptr_from_ref use/discard、unit completion。
ptr-valued loan tailはchecked ptr_from_ref / ptr identifierに限定する。
scalar/ref-valued loan結果、richer branches等のaccepted constructはemission前に既存
`V1-BACKEND-UNSUPPORTED`で止める。language-invalidとは扱わない。

C17は`const uint8_t`とread-only C pointer/carrier、lexical block、normal-result temporaryを
使う。ghost scopeはcompile-timeで検査後にeraseできる。C temporaryへのassignmentは
backend sequencing detailでありNewLang assignment featureではない。
C pointer equality/address/host UB/runtime guardsはsafety validatorではない。
NewLang representation/ABI/layout保証も導かない。

## Failure / stop / review

既存candidate clone/commit、owned artifacts/retained source planを維持する。
semantic failure、OOM、resource limitでpublic state/output ownerを部分変更しない。
parser、unit registration、definition check、actual body-sensitive call、fragment checkの
allocation pathsをfault-injectし、resource ceilingもpublic snapshotで確認する。

N1はscoped ref escape、N2はpersistent tokenの生存後のstale reacquisition。
N3はsupporting invariant fixtureと既存production conflict checkerで示す。
u8 Copy localをendingする新source spellingは発明しない。

Issue S1-S10が発火すれば停止してminimal witnessをCoordinationへ戻す。
required exact-head CI greenとE1-E8/N1-N3/regressionsをIssue/PRへ記録した後、
**LOCAL-ROOT PTR/REF PRODUCT GATE READY FOR REVIEW**で停止する。
merge/Issue close、field mutation、raw-storage、recursive Node、cJSON、次sliceを実施しない。

C emissionの追加precision fence: ptr normal resultはloanのouter rootと同じchecked
place/incarnationへ限定する。body-local root終了後にも保持可能なpersistent ptr tokenは
NewLangでinvalidではないが、C address representationの範囲外としてemission前に
backend unsupportedへ戻す。dangling C pointerのindeterminate valueをhostで読んで
validityを決めたり、runtime guardで補うことはしない。integrationにaccepted tokenの
unsupported controlを固定した。
