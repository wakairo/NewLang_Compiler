# Nested allocated-match captured-prefix join

Track: P / [Issue #186](https://github.com/wakairo/NewLang_Compiler/issues/186)

Authority: Compiler main `1c089ba933a31d73fc16013fdaee638741f13f3f`、
CURRENT_SPEC → Draft17.26 §3.2、§26.25、§27.3/27.4。
[Issue #185のfinding](https://github.com/wakairo/NewLang_Compiler/issues/185#issuecomment-6054777641)
および[Coordination裁定](https://github.com/wakairo/NewLang_Compiler/issues/185#issuecomment-6054819246)
に対するimplementation prerequisiteのみ。
Historical audit: N/A — canonicalへの忠実な実装、semantic delta **0**。

## Contract

`tests/fixtures/nested_allocation_join.nl` は#185のlinkなし縮小source。
実parser・registration・checkerを通し、次の三経路をowned artifactで保持する。

| Owned path | Semantic grants / EndRoot / matching deallocate |
|---|---|
| outer None | 0 / 0 / 0、inner trialなし |
| outer Some → inner None | headだけ1 / 1 / 1 |
| outer Some → inner Some | headとtail、別R/D/root incarnation、2 / 2 / 2 |

join対象はinner matchの**unit normal exits**だけ。
first Someのpre-fork prefixには、1つのlive BackingRegion・domain・completed H root、
exact matching Allocation/domain bindingが存在する。
利用可能なその他non-Copy owner、active scope、hidden/Unknown dependencyは
`ALLOCATED-CAPTURE-PRECISION`で保守的に拒否する。

`nl_allocated_closed_prefix` はincoming contextだけをcloneし、headのroot/package、
Allocation/domain binding、BackingRegion/domainが閉じた比較対象を構成する。
これはsource operation実行やimplicit cleanupではない。
None/Someを**それぞれ通常checkerで検査した後**、両armが比較対象と一致して初めてcommitする。
cleanup欠落を比較対象で補って成功させる経路はない。

比較はcaptured binding availability/value/place、root incarnation・fixed child/facts、
region属性・validity partition、domain value/liveness、scope、occurrence、
prefix package identity・carrier・dependency/provenanceまでexact。
新しいarm-local region/domain/placeは終了済み、non-Discardable packageは消費済みを要求する。
一致しないpost-stateは`ALLOCATED-CAPTURED-JOIN-PRECISION`。
第三nested trial等は`ALLOCATED-CARDINALITY-PROFILE`（unsupported）。
一般result phi・field mutation join・loop/terminating edge解析へ拡張しない。

## Ownership / identity / caller projection

両armの一致後、incoming prefixから構成した比較対象のcloneだけをcallerへcommitする。
どちらかのarmを選ぶことやarm-local IDのimportは行わない。
同じ数値IDでも、異なるowned arm path内のsuffixは異なるidentityである。
既存prefix IDだけがancestor由来として比較可能。
headのptr tokenはCopyのまま残り得るが、rootはdeadでsafe reacquisitionは拒否する。
caller continuationのAllocation/domain useもConsumedとして拒否する。

changed matchの`normal_frame_unchanged`は**false**。
独立の`captured_frame_closed`とR/root/incarnation/D/owner-symbolを記録する。
`nl_checked_captured_post(fragment, match)`はfragment所有のimmutable比較対象を借用する。
このsnapshotは後続lexical scope cleanupとは独立して保持され、AST破棄後も検査できる。
既存one-H unchanged certificateの意味は維持する。

比較対象・arm・continuation clone・artifact allocationの失敗は外側transactionをrollbackし、
caller registry/context/outputを変更しない。比較対象の所有権は成功時だけfragmentへ渡す。
checked moduleはsemantic context destructor callbackを保持し、下位moduleからcheckerを呼ばない。

## Evidence / limits

- `allocated_join_evidence.unit`: AST teardown後の3経路ledger、prefix-only snapshot、
  distinct R/D/root/inc、同数値suffixの異なる意味、10種のstate corruptionを各armに適用、
  malformed result/certificate拒否、output未変更。
- `allocated_join_source.integration`: actual source、renaming/scalar/owner alias、caller continuation、
  cleanupの非対称・欠落・重複、cross-region、wrong D、消費後/stale ptr、tail claim escape、
  pattern/profile/result拒否。入力ごとにexpected outcomeを固定する。
- `allocated_join_failures.unit`:既存malloc/realloc fault injection harnessに新sourceを追加。
  registrationおよびmain()で全allocation indexを順に失敗させ、NULL artifact、
  caller不変、clean retryを検証する。

new sourceのCLI outcomeはexit **4 / V1-BACKEND-UNSUPPORTED / empty stdout**。
new two-H C/native emissionは一切追加しない。既存one-H native/oracle/integrityを維持する。
full Draft17.26 heap-link witnessは既存field/Option pathの自然な合成によって
checker ACCEPT/backend unsupportedになったが、#185のrequired negative/owned evidence一式を
検証したclaimではない。このtaskではfield/Option implementationやnative observerを変更しない。
#185の再開・受理判断は本prerequisiteの独立Coordination acceptance後に別途行う。

仕様gap/ambiguityは発見していない。rich captured memory/dependencyはprecision limitation。
Canonical Draft、CI workflow、product/cJSON/oracleの規則は変更しない。

## Validation

Pinned bootstrap LLVM/Clang/formatter23.1.2、host GCC14.2.0。
CTestに3 testを追加、既存198と合わせて201。
GCC Debug、GCC Release/NDEBUG、Clang、ASan、UBSanでfull CTestを実行し、format checkも行う。
PR-triggered exact-head結果・SHA・run URLはIssue/PR handoffに記録する。

独立Coordination review前にmerge、#185再開、next-track起動を行わない。
