# Fixed-field product gate contract — Issue #135

Track: P

Base main: `12d49d049aae6831ce42a7a0b7b55934f01d9a2a`。
Canonical: `docs/reference/CURRENT_SPEC.md` → Draft 17.20。
本書はbounded production contractであり、canonical Draftを変更しない。
Semantic delta = 0。

## Sourceと責務

Draft §17.1のclosed profileを接続する。current direct lexical localの
registered nominal `Pair { left: u8, right: u8 }`だけに、one-level Copy read
`p.left` / `p.right`とordinary `loan_write(p.left){|w|...}`を許す。
declaration orderをfield indexとして保持し、initializer orderには依存しない。
他のAVS construction/destructuringは従来のcoverageを維持する。

parserはneutral dotted nodeを作り、checkerがfield / closed-sum categoryを
独立に判定する。両方が候補なら`FIELD-DOTTED-AMBIGUOUS`。選択後のunknown
field/variantを他categoryへfallbackしない。write designatorはfield category
だけを選択する。nested / arbitrary-expression / ptr base、method syntaxはこの
slice外。新keyword、general member lookup、Non-Copy partial moveを導入しない。

## Ownershipとidentity

`src/fixed_field.c`がfixed child relation、overlap、exact dependencyとその
invariantを扱う。context以外にmutable semantic stateを置かない。

- installed parent packageがmember packageの唯一のowner。
- fixed childは非所有viewで、parent PlaceId/incarnation、field index/type、
  own PlaceId/incarnation/current factを保持する。independent rootではない。
- parent installation時にchildをeagerに生成し、read/loanで再生成しない。
- live child current ValueIdはparent.current.fields[index]と一致する。
- parent end時にviewをdetachし、memberをparent経由で一度だけendする。
- machine address、offset、C struct layoutをsemantic identityに用いない。

公開viewはcontextをborrowする値のcopy。internal helperはtransaction candidate
だけを変更する。`nl_fixed_attach` / `nl_fixed_change`単体はpublic transaction
APIではなく、失敗したcandidateを破棄する既存checkerがatomicityを担う。

## Read / loan / Change

Copy readはcurrent child packageをcopyし、PairのAvailable状態を保つ。
exclusive conflictをparent/child overlapで検査し、write authorityを作らない。
field loanはimplicit parent-local stabilityからordinary、aliasable、non-exclusive
`ref<write,u8>`を作る。既存scope/nonescape/normal-result forwardingを使う。

`replace` / `store`のfield semantic operationでは、targetをChangeし、fresh
parent packageへmember ownershipをtransferする。parent/target current factは
fresh、root/両child incarnationとsibling ValueId/current factは不変。old parent
packageはhistorical ended状態になるが、preserved siblingをrecursive endしない。
replaceのold targetはordinary loose resultとなる。storeではDiscardable oldをend
する。field swapはprecision/profile fenceで停止する。

## Bounded exact dependency

valueに最大4個の`(PlaceId, current ValueFactId)` may-atomsをinline保持する。
これはcompiler resource boundであってlanguage limitではない。Unknown / hidden
coarse evidenceはprecision reject。無効/stale exact atomはrejectし、capacity超過は
resource rejectする。public seed APIはexactを捏造する経路を提供しない。

Change(left)はsurviving Value(left)またはValue(parent) atomをrejectする。
known-disjoint Value(right) atomは保存する。numeric addressでdisjointnessを証明
しない。Copy/transfer/cloneはatomを保存し、root endとfunction exitはending local
へのdependencyを検査する。source IF/MATCH/LOOPのlive exact evidenceはstructured
precision fence、既存raw/control header等の未対応pathもnon-free fenceを維持する。
programmatic ref joinとptr conversionもexact inputをprecision rejectし、atomを落として
dependency-free resultを生成しない。source-visible annotation、general solver、新effect
systemはない。

## Checked artifact / Checked-C

`NLCheckedField`はbase symbol、nominal/field type/index、parent/child IDとincarnation、
point-in-time pre/post facts、access、old/new ValueId、dependency proofを保持する。
loanにはscope completion、nonescape、result forwarding evidenceがある。最終contextで
localがendedでもpoint-in-time evidenceは歴史として残る。backendはsource AST/textを
再parseしない。

bounded validatorはresolved metadataを検査してから全C emissionを開始する。
synthetic `f0` / `f1`を使い、checked write evidenceのあるcarrierだけmutableにする。
readは実際の`.fN`、writeはordinary field pointer経由でoldをcapture後、新operandを
一度評価して代入する。const-cast、ABI/layout guarantee、hidden helperはない。
field store等のaccepted outside-backend constructは`V1-BACKEND-UNSUPPORTED`。
semantic rejectionではCを一度も生成せず、host compileも実行しない。

## Verification / 限界

`fixed_field_test`のparser/state/evidence/dependencies/negatives/failures/resource
groupsと`fixed_field_checked_c.integration`をnormal CTestへ登録する。
actual-source primary witnessはhost seed/preludeなし。dependency controlsだけがtrusted
internal negative/control fixtureを使い、source-positive evidenceへ混ぜない。
fault injectionは既存malloc/realloc harnessを拡張し、parser、registration、actual
body、field attach/read/Change、receiving/scope exitをsweepする。inline atom/fact counter
自体はheap allocationしないため、table clone OOMとcounter exhaustionを別に検査する。
Checked-C validation/emission setupもallocation-free。unsupported validationとoutput
failureを検査し、存在しないallocation point用のfault mechanismは作らない。

V0/V1/AVS/local-root/stable-root/R9、oracle adapter/smoke、artifact integrityを含む
full CTest、GCC Debug/Release、Clang+format、ASan、UBSanとlocked bootstrapを要求する。
general aggregate projection/mutation、raw Storage、allocation、recursive Node、cJSON、
LLVM、MIR、FFI等へ進まず、open/unmerged PRでreviewに渡す。
