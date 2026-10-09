# Draft17.29 durable custody continuation — partial preflight / HOLD

Track: P — [Issue #217](https://github.com/wakairo/NewLang_Compiler/issues/217),
[Coordination再開裁定](https://github.com/wakairo/NewLang_Compiler/issues/217#issuecomment-6072879961)。

**P DRAFT17.29 DURABLE CUSTODY SOURCE GATE HOLD — COORDINATION INPUT REQUIRED**

この候補はread-onlyなcall-entry preflightを実装した部分成果です。
実際のrecipient call、Someへの移転、durable custody、取り出しを受理していません。
primaryの結果はprecision rejectionであり、semantic acceptanceや
`V1-BACKEND-UNSUPPORTED`として報告しません。

## Authority / delta audit

- 開始時main: `f6ebaa10add43d001d0574f24aea5aa52ae24823`。
- `CURRENT_SPEC.md` → canonical Draft17.29、§18.1c。
- #219 / merged #220は独立Coordination ACCEPT。parent/child worldで修飾された
  元のpacketの由来と、**両分岐ともtailを解放する**共通post-stateを再利用します。
- #218は定義時の条件付きdescriptorとfail-closedなprefixの承認です。
  #217全体の受理を意味しません。F #39の有限モデルはcheckerの証拠に使いません。
- 開発プロセス、Compiler Testing Strategy、Design Decision Procedure、DI-009–013を確認。
  Historical design audit: **N/A — §18.1cの忠実なpreflight／precision境界の検証**。
  仕様・source/API・Ledger・CI・toolchain・backendの変更はありません。

## 実装した境界

`custody_preflight`は引数のevaluation、非Copy bindingのconsume、callee replayに
入る**前**に、現在のactual sourceの引数名を解決し、次をread-onlyに確認します。

1. independently checked recipient descriptorの未証明条件を変更せず、直接のsink／packet
   引数と、実際のcaller-local source loanを要求する。
2. sinkはordinary write refであり、scope・incarnation・provenanceが現在も有効。
   loanのchecked source operand、local root、ref symbol、accessを照合する。
3. Cは独立したimplicit-local sum rootで、currentはexact None、payload occurrenceなし。
   Cはraw／heap root／parameter ownerから権限を得たものではない。
4. 未解決のdependency、追加のactive scope、別の生存refは保守的にprecision rejectする。
   ordinary write refをnoaliasと見なさない。これは完全なalias解析ではない。
5. 元のAvailable packetを、#219のparent-owned producer証拠とchild entryの正確な
   prefix対応に照合する。ptr／A／D、元のroot incarnation、full BackingRegion、
   governing Domain、aggregate carrierとdependencyを保存する。

`nl_packet_available_inherited`は内部のread-only関係predicateです。
packetの由来をsinkのscope検証から分けますが、既存のfork／whole receiving／closureの
unloaned条件を弱めません。新しいAllocation、Domain、typed root、ownerを作りません。
このpredicate単独ではsink、alias、transfer、post-stateを証明できません。

primary／refusal／renamingの4 sourceはpreflightを通り、直後の
`CUSTODY-TRANSFER-PRECISION`で停止します。
**引数をconsumeする前の停止であり、actual-call certificateはまだありません。**
不適切なdirect profileは引き続き`CUSTODY-ENTRY-PRECISION`です。

## 正確な残件 / HOLD理由

§18.1cのpolicyの正常終了状態は、同一のoriginal identityについて次の二択です。

| world | packet binding | caller C | original tail O/R/D/A |
| --- | --- | --- | --- |
| adoption | Consumed | Some(original packet)、fresh occurrence | live / custody payloadに保持 |
| refusal | Consumed | None | terminal receiverで終了／解放 |

#219の`nl_packet_closed`はparentだけから**両方dead**の一つのconcrete stateを作ります。
`nl_packet_arm_closed`は各armがその共通targetに一致することを要求します。
この証拠をadoptionに適用するとtailを早期に終了させる誤りになります。
`normal_frame_unchanged`を立てることも、どちらかのarmをimportすることもできません。

現在の`NLSemanticContext`は一つの`current_value`／sum variant／payload occurrenceを持つ
concrete stateです。`function_match`の複数normal-arm continuationに、このCとtailの
相関を保持する証拠はありません。`nl_sum_attach`／`nl_sum_validate`は未知variantを
受理しません。後のexhaustive extractionを既存のhypothetical-arm経路へ流すと、
Someのpayloadを型から作る経路があり、元のLiveTail ownerの証明には使えません。

残る最小の実装単位は、**この一つのC／元のtailに限定した、world-qualifiedな
二つのnormal post-stateと継続の検査・合流**です。すべてのbranchで後のreplace、
payload extraction、terminal entry、二番目のexact-None消費まで証明してから
共通終了状態を認める必要があります。一般owner systemや仕様変更は必要だと
結論していませんが、今回そのowned conditional certificateを安全に完成できていません。

関連する最小の既存source-only probeもproduction checkerで実行します。
元のtwo-allocation producer sourceで、custody／recipientを一切使わず、次を挿入します。

```newlang
let policy = Option<ptr<Node>>::None;
match policy { None => { unit }, Some(q) => { unit } };
let LiveTail { owned_ptr, owned_allocation, owned_domain } = packet;
receive_and_release_tail(owned_ptr, owned_allocation, owned_domain);
```

両armではpacketをそのまま保ち、後で一度だけ明示的に解放します。
None／Some(ptr_h)の両policyで、既存terminal-only fork処理の
`P219-POSTSTATE-PRECISION`に停止します。これは新しいlanguage errorではなく、
owned packetを持つnormal continuationがまだterminal-onlyである証拠です。
このprobeを完全なcustody sourceの代替acceptance witnessにはしません。
実ソースの生成とCLI実行は`custody_source_hold_test.py`内で再現可能です。

元のprimary fixtureは§18.1c.4の全実行文を保持しています（コメント行だけ除去）。
SHA-256: `a30bbc37ff817020165d44a1436cb98ac934568f557068b378bf3118227d5644`。

## 実測証拠と限界

- 5 independent-definition positives、16 definition-shape negativesを維持。
- 4 primary／refusal／rename preflight controls：`CUSTODY-TRANSFER-PRECISION`、C／executableなし。
- 7 source preflight拒否：read mode、wrong sink、unknown／consumed／wrong packet、
  live copied sink alias、追加domain scope。alias／domain scopeは保守的なprecision拒否。
- 3 wrong producer ptr／A／D semantic negativesと、#219の2 backend-unsupported positivesを維持。
- 2 retained-packet continuation precision probes。いずれもsemantic acceptanceではない。
- 元のowned entry snapshotによるread-only packet lookupと6 identity／dependency corruptionを追加。
  AST/source teardown後の証拠を使い、lookupがpacket／snapshotを変更しないことを確認。
  これはcustody certificateのcorruption検査ではない。
- GCCのfault injection：independent definitionまで296 OOM、primary preflight停止まで1634 OOM。
  失敗時のpublic snapshotと既存source-created u8 binding/valueが不変。
  同じcontextで正常なsource checkを再実行可能。

**未検証・未実装:** actual recipient consume/return、Some payload occurrenceの移転、
両actual exact-None exception、conditional Current(C)のjoin、遅延extractionと元のterminal call、
occupied／may-Some／Unknown sinkの完全なcall applicability、stale custody occurrence、
actual custody certificateの保存／corruption、これらのpost-consume OOM。
早いguardで止まるnegativeをこれらの安全性規則の成功証拠には数えません。

分類：**COMPILER-PRECISION**。新たなnormative contradiction／spec holeは発見していません。
Coordinationへは、この有限conditional evidence continuationの実装境界を戻します。
recipient guardを除去する承認やDraft改訂を求めるものではありません。

## 再現 / review

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
cmake --build build-v1-gcc -j 4
ctest --test-dir build-v1-gcc -R 'custody|packet_fork' --output-on-failure
ctest --test-dir build-v1-gcc --output-on-failure
build-v1-gcc/newlangc tests/fixtures/live_tail_custody.nl
```

最後のコマンドはexit 3、`CUSTODY-TRANSFER-PRECISION`です。
元の226 CTestを維持し、localでは5構成とも226/226成功しました。

| local構成 | full CTest |
| --- | --- |
| GCC Debug | 226/226、25.82秒 |
| GCC Release / NDEBUG | 226/226、20.81秒 |
| Clang + format | 226/226、28.58秒、format成功 |
| ASan / leak detection | 226/226、80.26秒 |
| UBSan | 226/226、38.31秒 |

checksum-locked bootstrapも成功。localのGCC14.2、Clang/LLVM23.1.2、
Python3.12.14、CMake3.31.6を使用しました。
exact head・CI run・各構成の実測結果はIssue／OPEN PRに記録します。
既存のnative 0/1/2 allocation経路、oracle fail-closed、artifact integrityは維持します。
native custody、一般owner system、third root／5-root、cJSON、LLVM、他Trackは実装しません。

候補は部分成果として独立Coordination reviewへ渡し、#217をOPEN／HOLDに保ちます。
merge、Issue close、後続task開始は行いません。
