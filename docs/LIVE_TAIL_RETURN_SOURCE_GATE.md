# Draft17.28 LiveTail return source / semantic gate

This report records the accepted PR #209 pre-backend evidence. The subsequent
[Issue #210 native gate](LIVE_TAIL_RETURN_NATIVE_GATE.md) separately implements
the closed producer-return execution path; the source/semantic contract here
remains unchanged.

Track: P — Issue #208。新しいnative実装を含まないpre-backend gate。

## Authority / scope

開始時mainは `e6248e8389a8d9f86039f4d3cd5c7e18d1421e02`。
`CURRENT_SPEC.md` が指すDraft17.28 §§18.1b.1–6、およびmerged PR #207の
[独立Coordination ACCEPT](https://github.com/wakairo/NewLang_Compiler/pull/207#issuecomment-6062271098)
を正本とする。Process §4.2、Design Decision Procedure、DI-009/010/011/012、
Compiler Testing Strategyを確認した。

Historical design audit: **N/A — faithful implementation of adopted §18.1b**。
core semantic deltaは0。旧§18.1a terminal receiverの終了済みpost-stateを
live returnの証拠として流用しない。DI-012は採用状態とリンクだけを同期し、
旧案の内容やcanonical Draftは変更しない。

対象は同じHの2つのfallible allocation、1つの4-parameter producer、
1つのcompiler-known `LiveTail`、既存の別terminal receiverに限定する。
一般owner/effect/aggregate framework、追加source grammar、native producer、
第三のH、traversal、allocator、FFI、LLVM、cJSON、後続Trackは対象外。

## Source → independent definition → every call → owned return

`tests/fixtures/live_tail_return.nl` は正本§18.1b.4の完全なsourceをそのまま保存する。
`LiveTail`の登録はtype identityと固定3-field shapeだけを作り、
root、BackingRegion、LifetimeDomain、semantic value、authorityは作らない。
Copy / Discardableはともにfalse。signature-only callからaggregateをmintする経路は拒否する。

独立definition checkingは既存の有限symbolic-role checkerを拡張する。
四つのformalは `head_link / p / a / d` の別々の未対応symbolであり、
具体的なO/R/D、matched caller、Storageをseedしない。
exact H Option Noneへのhead-ref replaceが1回先行し、
complete constructorの各fieldがそれぞれ元のP/A/D roleから取得されること、
left-to-right Copy/consume、重複や不足、非Discardable normal exit、
head refのresult混入禁止を独立に検査する。
成功しても保持するのは**未証明の**有限tail-entry / head-link obligationsである。
読み取り・終了・解放を含む旧receiver traceは別の証拠として保持する。

各実callでは通常のargument evaluationに続き、既存のtyped-owner predicateと
raw occupancy / future EndRoot dependency checksを適用する。
actual current p→O/incarnation、A→full R、d→governing D、元のaffine valuesと
scope compatibilityを証明してから、headの別R/D・current field incarnation・
scoped write permission・current Some(p)のsource provenanceを照合する。
数値アドレス、型一致、好都合なsibling callは証拠にしない。
head scopeは現在のDomain ref capabilityから照合するため、有限matchのbranch内でも
外側のsource loanを保持できる。branchのnode listだけからscopeを推測しない。

actual calleeはfresh parameter bindingを作るが、独立heap O_tを移動・再生成しない。
通常のreplace checkerがhead fieldへChange/Resetを実施し、
通常のaggregate constructionが既存A/Dを1つのpackageへ移す。
return境界でtailのO/incarnation/current value/full R/Dがliveのまま、
headもlive、linkがNone、旧occurrenceが終了、元のA/Dがaggregate-owned、
donorとparameterがConsumed、resultにhead-scope dependencyがないことを検査する。
whole destructureはこのworldのchecked producer result originを要求し、
同じfield valuesをfresh caller bindingsへ受け渡す。
中間のimmutable owner binding / moveにも対応する。
後続receiverは既存§18.1a every-call predicateを改めて検証する。

## Independently owned evidence

`NLCheckedNodeView.producer` はdefinition requirements、actual input/donor/parameter、
O/incarnation/full R/D、head projection/current facts、before/after Optionとresultを保持する。
fragmentはentryとreturnの**二つの独立したowned context snapshots**を所有する。
world identityはsnapshot自身に結び付き、別cloneの数値ID一致では置換できない。
callee checked bodyの所属worldとretained immutable definitionも照合する。
`nl_checked_producer_valid` はread-onlyのconsumer contract検査であり、
新しいauthorityを作らず、source ASTを再解釈しない。

unit testsは元のsource/ASTを破棄した後でこの証拠を検査する。
return snapshotではO_t/R_t/D_tがlive、後のreceiver entryでも同じO/R/Dがliveであり、
最終post-stateでは既存receiverがtailを閉じ、callerがheadを別に閉じる。

| source world | grants | producer/result | semantic EndRoot / release |
|---|---:|---:|---:|
| first None | 0 | 0 | 0 |
| first Some / second None | 1 | 0 | 1 head |
| both Some | 2 | 1 | 1 tail receiver + 1 caller head |

outer changed-frame joinは既存#186/#187のancestor-prefix closed proofを再利用する。
Some-armのpost-fork numeric identityをNone-armやpublic callerへ持ち出さない。

## Falsification / rollback

source integrationにはprimary、H/field/function rename、declaration reorder、
optional tail read省略、caller/parameter aliases、producer/caller result placement、
逆順match arms、constructor field orderを含む10 positive variantsがある。
すべてcheckerを通過し、exit 4 / `V1-BACKEND-UNSUPPORTED` / empty Cとなる。
これはnative成功の証拠ではない。

27 destructive source controlsを用いる。型が一致するwrong root/backing/domain、
good siblingの後のwrong call、head None / different Some payload / read-only ref、
消費済みdonor、不足・重複・忘れた・discardしたLiveTail、head-ref混入、
missing detach / early return、部分destructure、tail Dのlive loan、
receiverでのwrong triple・二重消費・stale ptrなどを拒否する。
invalid producerはcallerのないsourceでも独立に拒否する。

**診断の層を混同しない:** wrong root/backingは新producer callから既存の
`P193-CALL-DOMAIN/BACKING` predicateへ到達する。
wrong D_hはhead D_h loanがまだliveなので、通常のargument consume時に
`P3-REF-CONFLICT`で先に拒否される。read-only headはparameter type compatibilityで拒否する。
これらはstatic semantic checksであり、lexer/name/profile fenceをwrong-owner proofと呼ばない。
head None / different payloadは `P208-CALL-HEAD-VALUE`へ到達する。
一般的なLiveTail-valued branch-result joinは別のprecision controlでfail-closed拒否し、
型だけからfresh affine packageを合成しない。

41 checked-evidence corruption controlsではflags、O/incarnation/R/D、projection/facts、
arguments/parameters/donors、result、world差替え、post-live state、head-scope依存、
Unknown dependency、authority field mismatch / duplication、backing permission、
aggregate carrierを攻撃する。復元後は同じ証拠が再度成立する。

malloc/realloc fault injectionをsource registrationとactual main callの全allocation
positionへ実施する。public context snapshotの不変、NULL artifact、owned snapshot / body
のfailure cleanup、最後のclean completionを検証する。
新しいreturn / result destructure / branch cloneもこのpathに含まれる。

## Validation / review boundary

単一entry pointは従来どおりCTest。新規3 testsを通常の全構成に登録する。

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
bash scripts/check-format.sh
```

GCC Releaseは `-DCMAKE_BUILD_TYPE=Release`、Clangは `CC=clang-23`、
ASan / UBSanは `-DNEWLANG_SANITIZER=address/undefined` を用いる。
exact-headの実行結果・run URL・head SHAはIssue #208 / candidate PRのfinal reportに記録する。
既存215 tests、PR #202 native handoff、oracle.adapter / oracle.smoke / artifacts.integrityを保持する。

Known precision limits: producer bodyはfinite straight-line selected operations / aliases /
complete returnに限定する。unrelated call/operationはstructured
`P208-DEFINITION-PROFILE` rejection。head actualはcurrent source FIELD_REFに限定する。
LiveTail-valued alternative joinsの一般化は行わない。
これらはCOMPILER-PRECISION / COMPILER-IMPLEMENTATION-LIMITであり、新しいlanguage errorではない。
新しいcore contradiction / Draft amendmentの必要は現時点で見つかっていない。

候補PRはOPEN / unmergedで独立Coordination reviewへ渡す。

**DRAFT17.28 LIVE-TAIL RETURN SOURCE-SEMANTIC GATE READY FOR REVIEW**
