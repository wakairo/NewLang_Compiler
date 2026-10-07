# Draft 17.21 production catch-up contract — Issue #146

Track: P

Base main: `46e07d7f627fc037dc794c3edb28d720815ca196`。
Initial authorityはDraft 17.21。Current `CURRENT_SPEC.md` → Draft 17.22の
§16.3とexisting aggregate/sum semanticsを正とする。constructor punctuationは`::`。
本taskはdeclaration/type-graph constructionのbounded catch-upであり、recursive topology
execution、Node field source、allocation/lifecycleへ進まない。

## Exact source

```newlang
struct Node { next: Option<ptr<Node>>, payload: u8, }
fn main()->unit {
    let n=Node{next:Option<ptr<Node>>::None,payload:u8(7)};
    let Node{next,payload}=n;
    next;payload;unit
}
```

Node / next / payloadはbuiltinではなく、admissible nominal nameとdistinct labelsを使う。
one semantic unitにつきat most one bounded recursive declaration。first fieldはexact
`Option<ptr<same nominal>>`、secondはcore u8、optional trailing commaのみ。recursive
categoryだけがdeclaration-only file／function前後／physical input permutationを許す。
旧AVS one-leading two-u8制限や他categoryのorderingを一般化しない。

parserは専用recursive declaration / exact Option type nodeを生成する。arbitrary Foo<T>、
signed/general type grammar、mutual declarations、methods/layoutは導入しない。
§16.3.3のexact `Option<ptr<H>>::None` / `::Some(p)`はexisting constructor checkerへ接続する。
Node field lookup/writeは既存Pair-only gateの外であり、unsupportedのまま。

## Header / completion boundary

`src/recursive_type.c`はprivate transaction candidateのdeclaration stateを扱う。
name admission/collision後、stable nominal TypeIdを生成し、incomplete stateを保持する。
cloneはincomplete/recursive-header/exact-Option argument metadataを保存する。
public `nl_semantic_type_completion`はcompletion stateだけを観測する。
incomplete nominalのfull type/property/field queryは失敗し、ordinary value/root/layout/
signature useも拒否する。source-visible incomplete/forward declarationはない。

selected `ptr<H>` type formationだけはHのfield/property/layoutを要求しない。
ptr Copy/Discardableはptr自身の規則から決まり、value/provenance/incarnation/place/backing
を生成しない。header未完成のptr/Option valueも生成しない。ref/slot等を代替indirectionに
せず、未完成targetを含むordinary useをfenceする。

exact Option concrete identityはpredefined Option + same ptr TypeIdでinternし、source
spellingによる別type生成をしない。None/Some(ptr<H>)のordinary closed sumを使う。
public general sum registryのptr-payload admissionを広げず、private exact-instantiation
helperだけがこのshapeを作る。nullable pointerや別Maybe typeはない。

## Containment / atomicity

cycle checkはaggregate fieldとsum payloadを追い、ptr targetで止まる。temporary color
arrayを既存4096-entry budget内で所有し、all pathsで解放する。candidate fieldsをheaderへ
仮commitせずに検査する。C sizeof/layout、backend acceptanceは使用しない。
direct self/sum-mediated by-value cycleはsemantic error。source mutual recursionは未対応。

resolution/cycle成功後、field labelsをowned copyし、全allocation成功後にsame TypeIdへ
field identities/typesをcommit、Copy/Discardableをderive、completeを最後に設定する。
completionはidempotentではなく、identical/inconsistent repeatともrejectする。
semantic unit endにincomplete headerが残ればreject。既存clone/check/commit transactionが
header、ptr、Option、completion、signature/bodyをまとめてrollbackする。

## Value witness / ownership

completion後のNone-link actual-source constructionとwhole destructuringは既存aggregate
value semanticsを使う。Option memberを含むCopyでは既存package copyをmemberにも適用し、
Some payloadを二つのaggregateで共有所有しない。standalone Someのconditional occurrence
とptr factsは既存sum machineryを維持する。Some ownership controlだけがtrusted pointer
fixtureを使用し、primary actual-source positiveにはseed/prelude/helperを追加しない。

Checked artifactのnominal/field/sum TypeIdsとvaluesが意味の証拠。このshapeはChecked-C
subset外なので、CLIはsemantic accept後に`V1-BACKEND-UNSUPPORTED`、C bytesなしで停止する。
Node C struct／recursive executable／runtime topologyを生成しない。

## Verification / stop

6 unit groups: parser、identity、source/order、cycle、negative、malloc/realloc fault sweep。
CLI integrationはactual-source acceptanceとbackend boundary、reject-before-emissionを検証する。
OOM testsはheader/type/Option/field strings/colors/completion/body stagesを既存harnessでsweepし、
public context snapshot/counters/ownershipとartifact absenceを確認する。
existing V0/V1/AVS/field/local-root/stable-root/R9、oracle/integrityとfull compiler matrixを保つ。
spec変更、新generic frontend、Node field widening、allocation、Checked-C recursive backend、
cJSON、LLVM、次sliceを必要としたら停止する。open/unmerged PRでreviewへ渡す。
