# Draft17.29 durable LiveTail custody — 完全な bounded source gate

Track: P — [Issue #217](https://github.com/wakairo/NewLang_Compiler/issues/217)。

候補のdisposition: **P DRAFT17.29 DURABLE CUSTODY SOURCE GATE READY FOR COORDINATION REVIEW**。
独立Coordination reviewとmergeは未実施。本成果はsource/semantic acceptanceとowned checked evidenceであり、native custody実行の証拠ではない。

## Authorityと範囲

base main: `75897e9b108185c530229c774c9d6c9951756922`。
`CURRENT_SPEC.md`の参照先はDraft17.29。採用済み§18.1cをnormative authorityとする。
参照ファイルの古い説明文に残るPROPOSED表記は、canonical sectionの採用済み状態やCoordinationの再開・commit更新裁定を上書きしない。
開発プロセス、Compiler Testing Strategy、Design Decision Procedure、DI-009–013、merged #218/#220/#221/#223を確認した。
採用済みbounded profileの実装なので、新規設計選択に対するhistorical gateはN/A。**Semantic delta = 0**。
canonical Draft、CURRENT_SPEC、Ledger、frozen oracle、source fixtureは変更していない。

完全な§18.1c.4のfixtureは既存の`tests/fixtures/live_tail_custody.nl`。
SHA-256は`a30bbc37ff817020165d44a1436cb98ac934568f557068b378bf3118227d5644`。
既存integrationはcanonicalのfenced sourceからコメントだけの行を除いて比較する。
現lexerで未対応のコメント以外、実行文・分岐・宣言・順序は変えていない。
既存partial/HOLD文書は、それぞれに記載された過去候補の証拠として保持する。

## 実際の所有責任移転

独立したdefinition-time `NLCustodyDefinition`は引き続き条件付き証拠である。
各actual known-direct recipient callで、引数評価・消費の**前**に既存のread-only admissionを行う。
元のworld-qualified packet、その生存中O/R/D/Allocation、別のcaller-owned exact-None root、ordinary scoped write permission、source loan operandを照合し、未解決dependencyやlive aliasのないことを要求する。
body shapeだけではcallerのauthorityを生成しない。

その後、同じproduction body checkerが保持されたsource planを実引数で検査する。
元のnonCopy packetをSomeへ移し、通常のreplaceを行い、取り出したold Noneを第1の特別なmatch siteで明示的にconsumeする。
recipientはtailを生存させたままunitを返す。EndRoot/freeを追加しない。
Cのfresh payload occurrence/current fact、元packet、消費済みdonor/formal、physical root incarnation、live region/domainをowned entry/post snapshotへ保存する。
source/callee loanは既存scope規則で終了する。
OptionはNone/Someのどちらでも静的にnonCopy / nonDiscardableのままである。

## 異なる状態の継続と共通の最終状態

| 独立に検査したpolicy world | Cと元のtail | 同じ後続sourceの検査 |
|---|---|---|
| adoption | Some(original packet)、fresh occurrence、tail生存 | fresh loan/replaceがSomeを返し、whole saved packetを取り出してterminal release |
| pre-consume refusal | None、元packetはrefusal armでrelease済み | fresh loan/replaceがNoneを返す。到達不能Someからownerを作らない |

policy matchは**両方**のarm worldを保持する。
`normal_frame_unchanged`はfalseであり、共通retained/closed packet flagを異なる状態の代用にしない。
残りの同じlexical blockを、それぞれのowned worldから既存の共有finite work/resource budgetの下で検査する。
実際のpolicy scrutineeがNoneでも、有利なarmだけを選ばない。

後段recovered matchは両継続で構文上exhaustiveである。
各相関worldのsource事実から可能なtagを証明し、そのworldで到達可能なarmを検査する。
refusal側にhypothetical Some ownerを生成しない。
2つのpolicy world全体ではNone/Some双方の回復経路を検査する。
whole receivingは元packetのfield identityを保持し、既存terminal receiverがO/R/D/Allocation対応を再検証する。
第2のone-arm consuming matchは、実際の1回のextraction後、loan終了後、元のC bindingのcurrent valueがexact Noneである場合だけに限る。
無関係なNone値や他の不完全matchへ例外を広げない。

既存parent closed-tail証拠とhead/Cの明示的closure義務から、parentだけに由来するclosed-prefix targetを構築する。
**両方の完全なsource suffixがそのtargetを証明するまでcommitしない**。
armの状態を代表値として採用しない。
adoptionの比較では、証明済みのpacket local→C payloadの所有edgeだけを射影し、それ以外の継承事実をexactに比較する。
最終closureの比較では、消費済みでpayloadを持たないCのhistorical None IDだけを一時的な比較cloneで正規化する。
live owner/root/region/domain/occurrenceをworld間で再同定しない。
新しいsource owner/effect contractは追加していない。

## Owned checked evidenceの契約

`NLCheckedFragment`はrecipient entry/post、両policy arm、各開始worldとsource suffix、共通final targetを所有する。
parent/world pointerのborrowはowning tree内だけに限定する。
破棄時は、OOMによる部分構築を含め全world/body evidenceを解放する。
caller contextへの変更は外側transaction成功時だけ反映する。

`NLCheckedNodeView`は実際のoperand・donor/formal、元packet/world、sink incarnation、old None、new Some/occurrence、後段replaceのold/new sumとoccurrence、2つのspecial match site、回復variantを記録する。
`nl_checked_custody_continuation`は各armのsuffixへのborrowed viewを返し、その数値IDは当該owned worldに属する。
`nl_checked_custody_valid`はparent-only closure targetを再構築し、両方の証拠をread-onlyで照合する。
source/ASTの再実行やauthorityの生成・消費は行わず、validation OOMはfalseとなる。

元の入力AST/sourceとentry sourceを破棄してから検証する。
**positiveごとに64個のpoison control**で次を拒否する:

- sibling/foreign cloned world、alternative欠落、偽unchanged/retained flag、誤ったancestor。
- Unknown/may-Some sink、stale sink incarnation/occurrence。
- 元packetのfield/dependency改変、生存root/domain改変、old Noneの未消費。
- 第1/第2 None証拠の欠落、extraction改変、whole receiving field改変。
- terminal donor/parameter/root/Allocation/domainの不正な組。

sibling worldの数値ID一致だけではorigin証明にならない。

## Actual-sourceと拒否段階の証拠

新しい`custody_source.integration`は6本の完全ソースを検査する。
primary、refusal、recipient rename、header/local rename、renamed refusal、宣言順変更である。
全て**exit 4 / V1-BACKEND-UNSUPPORTED / C出力なし**に到達し、actual checked `main()`のowned evidence検査も通る。
native成功は主張しない。
既存integrationの独立definition positive 5本、definition-shape negative 16本、pre-consume guard negative 7本、producer identity negative 3本、closed/retained join prerequisiteも維持した。

新しいintegrationは**25本の破壊的actual source**を独立に検査する:

- adoption後donor再利用、double release: `P3-USE-AFTER-CONSUME`。
- 後段の誤ったptr/domain/Allocation: `P193-CALL-DOMAIN/BACKING`。
- tail/head releaseやC消費の欠落: `P5-SCOPE-OBLIGATION`。
- 回復arm欠落、早すぎる/無関係なone-arm None: `P6-EXHAUSTIVENESS`。
  Some wildcard: `P6-PAYLOAD-DISCARD`。
- double extraction、alias、追加scope、非unit回復body: structured custody precision rejection。
  read-mode recoveryやlive loan中の消費は既存type/ref-conflict規則で拒否する。
- actual adoptionのないretained-vs-closed policy: `CUSTODY-CONDITIONAL-CONTINUATION`。
- preexisting Some構築は**早期bounded constructor guardによる拒否**である。
  late occupied-sink transfer拒否の証拠とは数えない。
  Unknown/may-Some current sinkとstale occurrenceはowned evidenceで別途攻撃する。

registrationのfault injectionは**3447** allocation site、actual checked `main()` replayは**2968** siteを走査した。
各失敗で既存source-created u8 carrierと全snapshotを保持し、artifactを公開しない。
後段のwrong-Allocation terminal callで失敗するsourceは、実際の`P193-CALL-BACKING`に達する前の**2696** registration allocation siteを走査し、rollbackを確認した。
正常完了・retryも通る。
raw-invariant検証のallocation failureはprecisionへ変換せずOUT_OF_MEMORYを伝播し、同じ全点fault injectionがこの区別も検査する。
独立definitionの既存OOMテストは維持し、complete source登録の成功を新たな期待値とした。

## Validation、制限、handoff

checksum-locked bootstrapでLLVM/Clang/format **23.1.2**、CMake **3.31.6**、Python **3.12.14**、local GCC **14.2.0**を検証した。
C17、warning policy、dependency pinは変更していない。
通常コマンドは`cmake --build <build>`と`ctest --test-dir <build> --output-on-failure`。
CIではformatも検査する。

既存**229 CTests**を全て保持し、owned source evidence / exhaustive OOM / complete-source integrationの3テストで合計**232**となる。
GCC、GCC Release/NDEBUG、Clang、ASan、UBSanの全suiteには、既存2-root native RETURN、oracle.adapter、oracle.smoke、artifacts.integrityを含む。
exact candidate head、PR URL、5構成のPR-triggered CI結果はCI完了後にIssue #217と候補PRへ記録する。
本ファイル内に自己参照commit SHAは置かない。

finding: 新しいspec hole/ambiguityや矛盾は発見していない。
追加alias、Unknown状態、別の条件付きownership profile、追加custody transfer、非unit/terminating recovery suffix等は保守的なCOMPILER-PRECISION fenceの対象である。
採用済みfinite two-root source gateであり、一般owner/effect systemやsymbolic memory solverではない。
backend、native custody、追加root/field、API spelling、RAII、LLVM、cJSON、他Trackの作業は追加していない。

候補は**OPEN / unmerged**で独立Coordination reviewへ渡す。
本reportからmerge、Issue close、後続taskを開始しない。
