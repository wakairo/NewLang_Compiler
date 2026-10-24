# NewLang Backend 実行方針とアーキテクチャ裁定 — v0

> **Status: Coordination ADOPTED operational policy / evidence-indexed decision record (2026-10-24).**
>
> **Authority boundary:** この文書は **v0 compilerの実装経路を選ぶ運用上の正本**であり、言語意味論、公開権限、LLVM属性、C ABI、製品判定の新たな規範ではない。言語意味論は [CURRENT_SPEC](reference/CURRENT_SPEC.md) が指すDraft、backend loweringの既存契約は [Backend Contract v0.4](reference/NewLang_Backend_Contract_v0_4.md)、投資とTrack裁定は [Development Process](NewLang_Project_Development_Process.md)、製品目標は [Product Value Strategy](NewLang_Product_Value_Strategy.md) を優先する。Issue中のTrack Bの数値・試作品は**歴史的な実測証拠**であって、採用済みのPlanTrace schemaではない。
>
> 根拠：Track B0 [#782 research](https://github.com/wakairo/NewLang_Compiler/issues/782#issuecomment-6364037700)／[Coordination ACCEPT](https://github.com/wakairo/NewLang_Compiler/issues/782#issuecomment-6364070460)、B1 [#784 research](https://github.com/wakairo/NewLang_Compiler/issues/784#issuecomment-6364703955)／[Coordination ACCEPT INCONCLUSIVE](https://github.com/wakairo/NewLang_Compiler/issues/784#issuecomment-6364762985)。新しいlanguage/backend design選定がないためProcess §4.2のsemantic revisionはN/A（既存正本KEEP）。

## 1. 採用済みの判断（運用上の正本）

1. **v0完成への既定経路は既存のdirect checked-artifact → strict C17 emitter → host GCC/Clang native** とする。対象は現行Draftが選んだsource profileと、実際にCHECK・真正な公開checked-owned証拠・backend契約が成立する範囲だけ。普通の newlangc を使えるように育てることを優先し、C17の過去のfixture成功を一般source/full-v0完成とみなさない。
2. **full Checked MIR、PlanTrace、実NewLang→LLVM backend、C17からの全面移行は未採用。** Track Bの研究報告を自動的な実装RELEASEとしない。既存のP/R作業を新しいIRの完成待ちで止めない。将来LLVMが有利なら切替・共存を別裁定できるよう、現C17を永久固定の言語設計にしない。
3. **最初の拒否層を分離する。** source/parser、semantic CHECK、source-issued receiptのissuer/arm attachment、公開validator、emitterのactual operand/carrier対応、strict C17/native、target ABI/物理実行は異なる失敗層。upstreamで証拠が発行・保持されない場合、C17/LLVM/MIR/PlanTraceの置換だけでは修復できない。逆にbackendにだけ存在するsource形状の制限を言語上の禁止や意味論の欠陥と誤認しない。
4. **新backendにも同じ信頼境界を適用する。** exact original root/incarnation/world、BackingRegion/Allocation/LifetimeDomainと物理carrier、current ValueFact/child/sibling、actual call/arm/owner fragmentとsource-issued proof origin、nonCopy一回consume、正常・RETURN・OOM/rollbackを一致させる。別worldの同数値IDやhostで生成したreceiptを権限にしない。実行opとproof grant／normal replay等のghost observationを混同しない。公開viewの観察は検証完了ではなく、borrowed ownerの寿命を越すcached receiptは認可しない。unknown/mismatchはfail closed。
5. **C17とLLVMのどちらも自動的には安全でない。** C17には順序・alias/effective type・typed storage/lifetime・alignment/layout/ABIの責務があり、LLVMには加えてSSA、poison、DataLayout、call ABI、optimizer metadataの責務がある。NewLangのref/owner表記だけからLLVM noalias、nocapture、nonnull、dereferenceable、llvm.lifetimeやC restrict等を発行しない。Backend Contract v0.4のより詳細な制約を維持する。

## 2. 過去方針との関係

初期 [Production Compiler Bootstrap Charter](NewLang_Production_Compiler_Bootstrap_Charter.md)・[Handoff Manifest](NewLang_Production_Compiler_Handoff_Manifest.md)には**LLVM primary**というP0当時の構想がある。[P0 Architecture](P0_ARCHITECTURE.md) のchecked MIR→LLVM図も将来構想で、P0のLLVM C API smokeは定数42を返すmoduleのverify・文字列検査であり、実NewLang sourceのLLVM lowering/native実行ではない。

2026-10-07の [V0 #101](https://github.com/wakairo/NewLang_Compiler/issues/101) は当初限定されたC17 bootstrap/reference execution spineを認可したが、将来のLLVMよりC17出力が優れるという体系比較ではなかった。[P0 Toolchain Decisions](P0_TOOLCHAIN_DECISIONS.md)のC17/C23比較は主に**コンパイラ実装言語の標準**の判断で、**出力backend**の二方式比較ではない。

その後の本物のdirect C17への段階的投資、source→CHECK→公開owned→nativeの有限検証、LLVM不要frontend-only buildは実装資産になった。B0/B1を受けてCoordinationが採用したのは**現時点のv0 critical pathでC17を維持する運用判断**であって、初期LLVM構想の履歴削除でも将来LLVMへの投資禁止でもない。

## 3. B0 / B1から保持する技術的知見（歴史証拠、採用schemaではない）

### B0：backendで直せる拒否と、直せないissuer拒否

- **R-A（H caller-local nonCopy return）**：当初はCHECK0でもdirect emitterがcaller-local packetの保持・consume形を扱えずCLI4/C出力0。P [#779](https://github.com/wakairo/NewLang_Compiler/pull/779)は既存issuerを変更せず、既存source-issued packetとoriginalを検証する狭いbackend consumerでC17/nativeまで到達。独立R [#780](https://github.com/wakairo/NewLang_Compiler/issues/780)は **NON-BLIND calibrated finite A-only PASS**。元R [#776](https://github.com/wakairo/NewLang_Compiler/issues/776)の初回COVERAGE FAILを遡及変更しない。**「MIRなしではcaller localが実装不能」は反証された**が、source形状依存・反復scanの将来保守費用は未測定。
- **R-B（H成功arm内のT Copy）**：B0固定sourceはCHECK0にもかかわらず、実arm Copy body/parameters validator status3、実call/entry/handoffはある一方、caller graph snapshot/originがarmへ適切に添付されていなかった。従って最初に見えたemitter拒否とは別に、**issuer側の真正なarm caller snapshot依存**があった。単なる同数値ID補完やbackend交換でvalidatorを通してはならない。これはB0の**2026-10-23固定観測**であり永続的な現行不具合宣言ではない。後続 [P #785 / PR #786](https://github.com/wakairo/NewLang_Compiler/pull/786)の限定issuerと独立 [R #805](https://github.com/wakairo/NewLang_Compiler/issues/805)を経てissuer-onlyは現mainへ採用された。さらにbackend候補 [PR #799](https://github.com/wakairo/NewLang_Compiler/pull/799)は別の独立native review gate [R #815](https://github.com/wakairo/NewLang_Compiler/issues/815)の対象であり、B0の古い「issuer欠落」と後日のbackend候補の採否を混同しない。
- **IF / scoped call**：既存のp713/p718/p728型の一部の実sourceでは、異なるarm-world、return edgeとnormal replay、source-issued known callを既存direct C17で処理できた。これは一般CFG/loop完成、任意H+T interleaveやfull backend比較の証明ではない。

B0は固定main **168c1cdafe929051ef08ff58f858fab045b17763** とP #775候補・修正P #779候補を別々に調査した。候補と後日のmainを混同せず、source/実測の詳細と固定関数アンカーは [B0全文](https://github.com/wakairo/NewLang_Compiler/issues/782#issuecomment-6364037700) を参照する。

### B1：「共通trace」が「共通lowering」を意味しない

B1は固定main **77d83dfc00850b14fd7c3bc95e7e3c12e34161fb** に対し、実public validatorが通ったR-A/valveとIF/call/IF-RETURN/two-normal IFの5 sourceを使って、借用actual owner/worldに紐付く読み取り専用scratch PlanTraceを試した。初版はHのallocation arm本体を見落とし、proof grantとruntime try、ghost replayとruntime書込みを混同し得ると判明。修正版は区別したが、source → 実C carrier/物理原物 → affine single-consumeの検証済みloweringではない。

- 326行のscratch trace / 約63,568バイト固定領域で **19種類の共通診断operation/control path** を観測した。しかしこれは19個のproduction loweringやMIR契約の成功ではない。
- H+Tの公開validator呼出数を減らす点では、**59行のprivate borrowed-receipt helper** がPlanTraceと**同じ結果**。IF/callでは反復IF validationによって呼出数が増える例もあった。よって削減はPlanTrace固有の優位ではない。
- 異world同数値、field/formal誤対応、entry/return historical snapshot混同の一部は既存validatorが拒否。ただし**実plan consumerの二重consume拒否、exact physical carrier、PlanTrace→C17 serializer、OOM時のsame-world rollback、full regressionと独立Rは未検証**。修正版traceの単純な成功や5 sourceの既存direct native成功を新backendのPASSと数えない。
- B1に対するCoordination裁定は **ACCEPT INCONCLUSIVE / PlanTrace未採用**。[B1再現付録](https://github.com/wakairo/NewLang_Compiler/issues/784#issuecomment-6364699856)、[B1判定](https://github.com/wakairo/NewLang_Compiler/issues/784#issuecomment-6364703955)、[C裁定](https://github.com/wakairo/NewLang_Compiler/issues/784#issuecomment-6364762985)。

## 4. 将来の比較・別途認可する条件

| 方策 | 現時点の位置付け | 最初に反証すべき点 |
| --- | --- | --- |
| 既存direct C17を継続 | **DEFAULT / ADOPTED** | 選択済みsourceの複数familyで同じ認証・carrier追跡を毎回重複実装しているか |
| source-issued receiptを現lifetime内で借用する小private shared verifier / def-use / carrier helper | **代替候補、実装未認可** | 実operand→元carrier・nonCopy consumeを安全に共有し、現directより検証・保守コストを下げるか |
| 最小evidence-preserving plan → C17 | **条件付き実験仮説、未採用** | 複数familyのactual carrier/RETURN/affine/OOMを旧source shape guardの単なる移設なしに共有できるか；小helperで十分ではないか |
| 同plan → LLVM object／直接checked nodes → LLVM | **将来選択肢、未実装・未比較** | genuine issuerとtarget ABI/optimizer provenanceを満たしたうえで、v0実装・保守・CI費用の純増を上回る具体的利点があるか |

今後architecture投資を再検討するのは、**実際の新しい普通のsource compositionで最初の欠落層を計測し**、それがissuer/parserではなくbackend consumerの再利用・負担にあると示せた場合。C17据置き／小helper／plan pilotの対照を、同じ既存の本物のsource-issued receiptsと同じnative・negative・OOM条件で比較する。単なる共通kind数、削減LOC、validator-call回数、green fixture数やLLVM noveltyは採用基準にしない。

別のtrusted surfaceを作る候補は**その前にCoordination IssueでGoal/Evidence/Alternatives/StopとCI予算を裁定**し、必要ならM/Fで新authenticity設計を検討、Pでbounded implementation、R §7で独立検証する。特にpublic receiptをコピーして永続化、権限を再発行、semantic graphを別worldへremap、old proof validatorをスキップするpilotは禁止。実negativeやOOMで差異が出ればSTOP。B0/B1の概算人日は低信頼・実測工数ではなく、そのままロードマップ納期にしない。

**現時点で追加Track BやLLVM移行の自動RELEASEはない。** 本文は将来判断の起点を保存するものであり、固定backend内部データ構造や現行実装の全てを永久制約にするものではない。

## 5. 製品価値との境界

技術的なbackend選択は、[Product Strategy §1.1](NewLang_Product_Value_Strategy.md#11-ub-prevention-audit-locality-value-hypothesis) の C後継としての**library author＋ordinary-safe callerのUB-prevention audit/re-audit依存閉包縮小**を実証すること自体ではない。Cを積極的に選ぶ低水準実workloadとwell-factored Cの比較、API、元pointer topology、allocation/failure/teardown、合法safe変更を含む保守再監査は別途必要。**Product GO #624 PAUSED、cJSON H1 #268 HOLD/UNDECIDED**。この未測定を理由に既選択の安全なv0 compiler実装を機械的に止めず、同時にbackend実験の成功をProduct GOへ昇格しない。
