# Draft 17.29 durable LiveTail custody native gate — Track P

対象: Compiler Issue #227。開始時mainは
`c2838d42f5bb4894ba431d48b25e00f0403b2ea7`、CURRENT_SPECはDraft 17.29。
P #217 / merged PR #226の独立Coordination ACCEPTを前提に、§18.1cの
source/semantic/owned checked evidenceをC17実行へ接続する。
仕様変更・semantic deltaは **0**。

## Authorityと範囲

開発プロセス、Compiler Testing Strategy、Design Decision Procedure、
Design-Intent Ledger DI-009–013、Product Value StrategyとNorth Starの
anti-false-progress規則を確認した。歴史的F/R成果をnative証明の代わりに使わない。

Primary witness `tests/fixtures/live_tail_custody.nl` は変更しない。
SHA-256は `a30bbc37ff817020165d44a1436cb98ac934568f557068b378bf3118227d5644`。
canonical witnessとの一致検査も保持する。source parser / checker / canonical Draft / Ledger /
frozen North Star workload / oracle adapterの変更はない。

## Checked evidenceからCへの契約

既存
`nl_checked_c_node` の閉じたprofileにだけ対応を追加する。

- 既存producerのowned entry / return証拠を検証し、実際の別C関数でheadのlinkを
  Some(original tail ptr)からNoneに更新して、同じptr / Allocation / Domainを返す。
- 完全な `nl_checked_custody_valid` を検証する。両policy armのowned worldと、
  それぞれのowned continuationを独立にlowerする。arm-localな数値IDをsiblingから
  流用せず、証明されたancestor prefixだけを既存C carrierへ対応させる。
- recipientを別C関数として、checked bodyのbinding / Some / replace / exact-None
  consuming matchを順序どおり生成する。caller localはprivate tagged
  `nl_custody { tag, original packet }`。scoped普通write-refはそのlocalへのC pointer。
- recipient内にもcallerの格納後にもEndRoot/freeを追加しない。loan終了後の新しい
  loanで取り出し、Some payloadをwhole destructureして既存terminal receiverへ渡す。
- 相関したsuffixのreachable recovery armをchecked evidenceで選ぶ。policyの片方を
  代表に選ばない。両suffixのterminalとhead解放を別々に検証して生成する。
- 2か所のsource-proven None消費にowner処理はない。観測hookは状態を見るだけであり、
  omitted Some armの安全性をruntime assertionで証明しない。

carrier/lifecycle stageはcompiler内部の対応検証だけであり、生成Cにowner table、
provenance判定、policy runtime、Drop/RAII、implicit cleanupを追加しない。
既存§18.1b backendは以前の即時destructure profileを維持する。新profile外の
language-valid sourceは引き続き明示的backend unsupportedになる。

## 独立したnative証拠

`custody_checked_probe.c` はactual sourceをproduction checkerに渡し、AST破棄後の
public checked evidenceからroot / incarnation / BackingRegion / Domain / payload /
projection / scopeの期待値を抽出する。source文字列や生成Cを安全性のauthorityにしない。

`custody_observer.h` はapplication heap/localをconstで読む。自分のtrace stateだけを
変更し、memory修復・initialization・freeは行わない。既存のNULL-only platform shimが
実malloc/freeを呼ぶ。2つの独立した実heapアドレス、物理head link、元のpacket、
recipientへの受け渡し、caller localへのSome格納、関数とloan終了後の保持、取り出し、
terminalへのoriginal owner渡し、EndRoot / raw / finalize / matching freeを観測する。

| 実行経路 | 成功したheap roots | free | recipient | terminal |
|---|---:|---:|---:|---:|
| first allocation None | 0 | 0 | 0 | 0 |
| second allocation None | 1 | 1 | 0 | 0 |
| both Some / accept | 2 | 2 | 1 | 1 |
| both Some / refuse | 2 | 2 | 0 | 1 |

両方成功の場合はhead/tailアドレスが異なること、tailのptr / Allocation handle /
Domainがproducerから最終解放まで変わらないこと、recipientが先に解放しないことを検証する。
同じrootの重複freeはobserverがplatform呼出し前に停止し、UBをnegative証拠にしない。
観測なしの生成Cも全allocation結果で実行する。NewLang source I/Oは追加しない。

5つのactual-source変種（primary、refusal、alpha/header rename、refusal rename、
main先行）×3 allocation結果×observer有無、計30回のpositive native実行を行う。
18件の生成C mutationはwrong returned/stored/extracted ptr、wrong A/D、wrong custody
local、missing Some/unlink、invalid scope、fake None、missing terminal、duplicate/missing
free、second-failure時head leak、missing final Noneを攻撃する。observerによる明示的拒否を
要求し、単なるcrashやsanitizer findingを合格にしない。

## Failure / rollback / regression

既存64件のowned custody poisonは、certificate拒否に加え、top-level emissionが
出力pointer/lengthを変更せず拒否することを検証する。sibling/cloned worldの数値ID一致、
packet/occurrence、scope/liveness、policy world欠落、sink/owner対応の破壊を含む。
recipient checked bodyの欠落・誤operand・Some/old-value/resultの追加破壊も拒否する。

codegenで実際に通る718 allocation pathを一つずつOOM injectionする。全failureで
出力はNULL / 元のlengthのまま。fault解除後の再試行はbaseline Cとbyte-for-byte一致する。
read-only certificateの内部OOMは証拠不成立としてfail-closedに拒否し、Cへfallbackしない。

既存232 CTestsを削除しない。2件（codegen OOM / native integration）を追加して計234。
source gateのpositive期待値を正確なC emissionに更新し、25件のactual-source negative、
independent definition、preflight negative、semantic OOMと従来native gateを保持する。
oracle overlap classification / fail-closed adapter / artifact integrityは変更しない。

## 再現とvalidation

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
bash scripts/check-format.sh
```

GCC / GCC Release(NDEBUG) / Clang+format / ASan / UBSanの5構成で全CTestを実行する。
ASan/UBSan構成はcompilerだけでなく生成C/native observer/mutation executableにも適用する。
strict C17 warningsは生成Cにも `-Wall -Wextra -Wpedantic -Werror`。

local validationは5構成すべて **234/234 PASS**、format PASS。
GCC Debug / ReleaseはGCC 14.2.0、Clang / ASan / UBSanはlocked Clang 23.1.2。
GitHub ActionsはUbuntu 24.04のGCC 13とlocked Clang 23.1.2を使う。

local実測例（アドレスは実行ごとに変わる）:

```text
first None:  head=0 tail=0 trials=1 free=0 recipient=0 terminal=0
second None: head=55b8e94252a0 tail=0 trials=2 free=1 recipient=0 terminal=0
accept:      head=5643bac732a0 tail=5643bac732c0 trials=2 free=2 recipient=1 terminal=1 none=1/1
refuse:      head=559deb6652a0 tail=559deb6652c0 trials=2 free=2 recipient=0 terminal=1 none=0/1
```

固定headのrequired CI URL、実測結果、PR stateはIssue #227 / 候補PRへの
`Track: P` final reportに記録する。実装reportをCIの代替にしない。

## Findingsと停止条件

新しいsemantic hole / ambiguityは見つかっていない。今回の変更は
COMPILER-IMPLEMENTATION（既存owned evidenceのbounded C lowering）。
一般Option owner backend、3-root以上、cJSON、allocator API、FFI、LLVM、他Trackへ広げない。
5構成の固定head CIと完全native evidenceがgreenになった場合だけ
**P DRAFT17.29 DURABLE CUSTODY NATIVE GATE READY FOR COORDINATION REVIEW** として、
OPEN/unmerged PRで独立Coordinationへ引き渡し、停止する。自分でmerge/Issue closeしない。
