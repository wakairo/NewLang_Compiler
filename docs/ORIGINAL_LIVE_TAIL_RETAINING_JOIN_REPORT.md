# Original LiveTail owner-retaining normal join

Track: P — [Issue #222](https://github.com/wakairo/NewLang_Compiler/issues/222)。

元の非Copy LiveTailを、通常のexhaustive policy matchの**両normal armで保持**し、
合流後にwhole destructureして既存terminal receiverへ一度だけ移転するbounded sliceです。
実ソースの定義登録とactual `main()`のsemantic checking、owned証拠の保存を検証します。
Checked-Cは引き続き`V1-BACKEND-UNSUPPORTED`です。native成功ではありません。

## Authority / scope

- 開始時main: `12245ea4f6c4409aa76a7450030571c006b19bf6`。
- CURRENT_SPEC → canonical Draft17.29。§§3.1–3.4、10–14、17.4、18.1a–c、
  18.2/5–8、26.9–13、27.3/4のidentity、非Copy、dependency、normal join規則を維持。
- 開発プロセス、Compiler Testing Strategy、Design Decision Procedure、Ledger DI-009–013を確認。
  Historical design audit: **N/A — existing canonical ownership/normal-join rulesの忠実な実装**。
  新しいnormative source/API/semantic decision、Ledger変更はありません。
- #219 / merged #220は両armでterminal releaseする証拠。
  #217 / merged #218、#221はpartial definition/read-only preflight。
  #221の独立ACCEPTはcustody受理を意味しません。#217はOPEN/HOLDのままです。
  F #39を実装dependencyやproduction証明として使いません。

## Primary witness

[tests/fixtures/live_tail_packet_retained.nl](../tests/fixtures/live_tail_packet_retained.nl)
は、既存`custody_source_hold_test.py`のno-custody continuation probeと完全に一致します。
sourceから元の2つのfallible allocation、head-link detach、producer return、packetを作り、
policyのNone/Some両armでunitを返します。その後元のpacketをwhole receivingし、tailを
既存receiverで終了／解放、headも明示的に終了／解放します。host seedはありません。

SHA-256:
`357a0684d96aa6f15e8474ed9799e6764ebe0347f62f98e2f350a8fc2f4d0e5c`。
Some(ptr_h) variant:
`1c4f5dbef3b74231600ff2a6a1eb0e396ced4dd6d4641f82248a892fa87704c8`。

既存HOLD reportは当時の証拠として変更しません。本reportが更新するのはその中の
**2つのno-custody continuation probeだけ**です。元のcustody primaryは依然として
`CUSTODY-TRANSFER-PRECISION`です。

## Bounded invariant / owned contract

1. 親のproducer entry/return証拠に由来する、唯一のAvailable packetを要求する。
   ptr provenance、元のO/incarnation、full R、D、Allocation、aggregate carrierと
   dependencyを照合し、active scope/Unknown blockerを消去しない。
2. arm検査前の親worldだけから2つの別のproof targetを構築する。
   既存closed-tail/live-head targetと、新しいretained-live-tail/live-head target。
   新しいtargetは元のprefixをcloneし、pattern用Copy scrutinee temporaryだけを終了する。
   source binding、owner、fresh rootやbranch-local IDを作らない。
3. distinct hypothetical worldでNoneとSomeの**両arm body**を独立に検査する。
   各armのprefixは同じtargetと正確に一致し、suffixの責任は完了している必要がある。
   retained判定では元のpacket Available、元のlive O/R/D/A、head factsを再照合する。
   control exitsや未解決dependencyを正常継続と見なさない。
4. 2つのparent-defined targetへの証明のintersectionを取る。
   両closedまたは両retainedだけを認め、片方消費・片方保持はrejectする。
   known initial variantによる省略、arm代表値のimport、`normal_frame_unchanged`の偽装はない。
   継続worldは親targetから構築し、childの数値IDやfactを持ち出さない。
5. ordinary identifier consume直後／field receiving前に、**現在のjoined world**の元の
   packet関係を再検査する。whole receivingにはparent/match/packet/current world/
   retained-post worldを修飾した由来を残す。
6. 既存のindependent terminal definition、actual callのroot/backing/domain関係、
   donor/formal consume、EndRoot/finalize/deallocateを通常通り検査する。
   `nl_checked_packet_retaining_join_valid`は両armとparent-derived postを検証し、
   `nl_checked_packet_retention_release_valid`はさらに後段whole receivingとterminal証拠を照合する。
   元のtailが終了／解放、packetがConsumed/Endedであることを確認する。

entry/post snapshots、両arm、producer/receiver bodyはartifact treeが所有します。
childとreceivingはそのtree内のancestor/worldをborrowし、treeの寿命を越えません。
source/AST teardown後もreadonly certificateを検査できます。失敗途中のsnapshotも
既存のfragment destructorで解放します。public stateはtransaction成功時だけ更新します。

## 実測 / destructive evidence

- 6 actual-source positives: policy None、Some(ptr_h)、swapped arm order、
  packet/arm binder/terminal rename、source-derived別Copy policy carrier、独立したnested-block形。
  最後のcaseは別々のarm localにu8(7)/u8(255)を持ち、同じ数値IDを親authorityに採用しません。
  全てdefinition registrationとactual callが受理され、owned join/late-release証拠を検査できます。
- 23 source negatives: 各armのrelease-vs-retain、move、whole receivingだけ、wrong owner、
  head責任喪失／各armでのsource Change/Resetによるhead fact更新、後段のwrong root/backing/domain、missing release、double receiving/release、
  stale reloan、live dependent ref、scope escape、active fork scope、missing head cleanup。
  登録失敗時、元のsource-created u8 binding/valueとpublic snapshotは不変です。
- 深いowner拒否: `P193-CALL-DOMAIN` / `P193-CALL-BACKING`。
  消費／責任: `P3-USE-AFTER-CONSUME` / `P5-SCOPE-OBLIGATION`。
  stale/ref conflict: `P3-STALE-POINTER` / `P3-REF-CONFLICT`。
  mixed target: `P219-POSTSTATE-PRECISION`、active entry scope:
  `P219-FORK-ENTRY-PRECISION`。これらprecision拒否をlanguage-invalidと再分類しません。
- scope escapeは既存の`P3-INTERNAL`で保守的に失敗します。精密なescape diagnosticと
  誤報せず、**DIAGNOSTIC-QUALITY limitation**として維持します。
- 41 source-backed certificate corruptions: sibling/copy/foreign world、ancestor/entry world、
  variant/packet/match、cloned context、stale incarnation/current fact、wrong A/D、Unknown
  dependency、consumed packet、false unchanged flag、foreign/changed parent post、
  receiving lineage、terminal entry/post/root/incarnation/R/D/Aの改変を拒否。
  改変を戻すと証拠は再び有効。元の#219の42改変と#221の6拒否も維持。
- primary OOM: registration **2168**、actual source call **1764** fault points。
  各failureでpublic state rollback、partial artifactなし、同じcontextでclean completionし、
  actual callの成功後に再度owned retaining/late-release証拠を検査。
  mixed-arm failureの登録にもfault injectionを行い、精度拒否までrollbackを確認。
- no C/no executable: semantic negativesはexit3、accepted unsupportedはexit4、stdoutに
  Cなし。emitter APIもUNSUPPORTEDでoutput=NULL、length未変更。後からCを削除しません。

## #217への有限feasibility境界（実装しない）

本taskは、両armが**同じlive original owner state**を証明するconservationです。
§18.1cのadoption/refusalではC.Some/live tailとC.None/dead tailが異なるため、
このtarget intersectionをそのまま使えません。必要となる有限証拠は、同じO/R/D/Aの
2つのworld-qualified normal alternativesとCのpayload occurrenceとの相関を所有し、
後段extraction/terminal callを各alternativeで検査するものです。Unknownをdependency-freeに
したり、Some payloadをtypeだけから作ったり、None worldの解放済みownerを再生してはなりません。
この前提の見通しを示すだけで、recipient、storage、differential joinは実装／受理しません。

## 再現 / validation

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
ctest --test-dir build -R 'packet_retention|packet_fork|custody' --output-on-failure
build/newlangc tests/fixtures/live_tail_packet_retained.nl
```

最後はexit4 / `V1-BACKEND-UNSUPPORTED`。GCC、GCC Release/NDEBUG、Clang+format、
ASan、UBSanでfull CTestを実行します。既存226 namesを維持し、3件を追加し229件。
localはGCC14.2.0、Clang/LLVM23.1.2、CMake3.31.6、Python3.12.14。

| local構成 | full CTest |
| --- | --- |
| GCC Debug | 229/229、29.25秒 |
| GCC Release / NDEBUG | 229/229、20.94秒 |
| Clang + format | 229/229、31.15秒 |
| ASan | 229/229、94.57秒 |
| UBSan | 229/229、43.20秒 |

最後に追加した2つのsource head-fact変更negativeも5構成のtargeted integrationで検証します。
CIのUbuntu24.04/GCC13とchecksum-locked bootstrap、5-job workflowは変更しません。
exact headとPR-triggered CI URL/結果はIssue #222とPRのhandoffに固定します。

対象外: custody/recipient actual transfer、differential ownership join、新しいnative backend、
一般owner system、canonical変更、他Track、後続task。
PRはOPEN/unmergedで独立Coordination reviewへ渡し、#217はOPEN/HOLDのままです。
