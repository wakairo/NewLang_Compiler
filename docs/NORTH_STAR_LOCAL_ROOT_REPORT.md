# North Star local-root ptr/ref product gate evidence

Track: P

## Authority / task

- Issue: [#110](https://github.com/wakairo/NewLang_Compiler/issues/110)
- base main: `7cd61f033f4f9c8c7b24c8147d2c6a263312b954`
- CURRENT_SPEC: Draft 17.18、normative snapshot変更なし。
- branch: `local-root-ptr-ref-gate`
- semantic delta: **0**。`u8(-1)`はDeferred / unchanged。
- exact PR/head/required CI runはIssue/PR handbackへ記録する。

先にmainの運用方針、最新Issue/comments、canonical §§10/11/13を確認し、初期auditを
[Issueへ記録](https://github.com/wakairo/NewLang_Compiler/issues/110#issuecomment-6035822817)した。
既存place/incarnation/ref/ptr/conflict/transaction machineryを再利用し、header-only loanは
成功証拠に数えない。canonical変更、新lifetime modelは不要である。

## E1-E8 / P1

`local_root_test evidence`はprimary P1をactual function unitとしてregisterし、入力tree/source
を破棄してからreal `main()` body-sensitive callを検査する。host-created root/ref/domainはない。

| Obligation | Direct evidence |
|---|---|
| E1 | checked u8 value7、source binding/place、implicit governing marker、same incarnation、explicit domain/backing allocationゼロ |
| E2 | real checked BLOCK、resolved local source/binder、fresh active ref scope、prevent-lifetime-end obligation |
| E3 | existing PTR_FROM_REF node、checked ptr facts、scope=0、body/loan/bindingの同じresult ValueId、nonescape-before-forwarding evidence |
| E4 | second loanはfrom_ptr mode、same root/incarnation、different fresh ref scope/binder、current local proofを必要とする |
| E5 | second checked bodyのptr_from_ref argumentがそのfresh ref binderのCopy use |
| E6 | unchanged body R forwarding；nested inner loanからouter-dependent refを保持して渡せるcontrol、outer escapeはreject；hidden/unknown依存を精度不足としてreject |
| E7 | integrationがchecked-value initializer、address carrier、result receiving、fresh second ref/useを確認しstrict C17 compile/native execution、determinismを確認 |
| E8 | actual witnessにunchecked/prelude/helperなし。backendはsource loan/capability name textを再lookupしない |

第一scope終了後、ordinary outer receivingが同じptr packageを受け取る。
function completion後のcontextはhistorical place/incarnation/scope IDsを保持するが、localは
ended、双方のscopeもinactiveである。backendはそのhistorical evidenceを再解釈して
新authorityを作らない。source witnessそのものをnative executionまで使用する。

追加bounded positivesはptr Copy receiving、discarded ptr-producing loan、unit loan result、
nested read-only unit loans。強いobservable I/O、dereference syntaxは追加しない。

## N1 / N2 / N3 / destructive controls

- N1 actual function source: loan-bound ref resultがsurviveし、`P8-EXIT-DEPENDENCY`。
- N2 actual function source: ptr tokenはinner loan/lexical-local scopeを越えられるが、後の
  reacquisitionは`P3-STALE-POINTER`。token survivalだけのfragmentも別にACCEPTし、
  referent placeがdeadなのにtokenをreceiveできることとaccess rejectionを区別する。
- N1/N2のcompiler stdoutは空でC/native未生成。C生成後の削除で成立させない。
- N3 supporting fixture: live read refに対するnon-Copy local transferを既存production
  `conflicts(...ending=true)`経路で`P3-REF-CONFLICT` reject。same checkerはsource local
  exit / transferにも使用される。既存take/destroy conflict regressionsも維持する。
  fixtureはprimary actual-source product evidenceではない。
- unknown/invalid provenance、wrong incarnation、read access不足のptrを、actual sourceで
  作ったlive u8 localに向けたsupporting controlsでもrejectする。
- domain-zero host valueだけではlocal implicit authorityにならずprofile unsupported。
- unknown body call、loan boundaryからのreturn、body-local binderの外側useもreject。
- general operands/body-less same-spelling callsはprofile unsupported。ordinary local name
  `loan_read`/`loan_read_ptr`はadmissibleのままで、blanket keyword policyは導入しない。
- checker-accepted scalar-result loanはexplicit backend unsupported、C stdoutは空。

## Ownership / limits / integrity

malloc/reallocの各allocation位置をparser reuse、durable registration/definition checking、
real body call、source fragmentの四pathで失敗させる。public snapshots不変、artifact未公開、
cleanupと再試行成功を確認する。4096 scope host budgetを満たしたcontextの追加loanは
resource rejection、既存root marker/incarnationを含むobservable state不変。

local-root proofはsource u8 typed objectに限定。explicit domain/backing/conditional-subplace、
unknown dependency、non-normal loan bodyはclosed gate外でunsupported/precisionとする。
ref-resultが互換なouter scopeに依存する場合のchecker forwardingは保持するが、C emissionは
ptr/unit normal-result profileだけ。一般loan grammar/control/capability graphへ拡張しない。

frozen oracleの241 `.nl` inputsに`loan_read(`/`loan_read_ptr(`は0件。新overlapなし。
oracle adapter/exclusions/archive/lock/provenanceは変更せず、explicit/input-based/fail-closedを維持。
V0/V1/AVS positives/negatives、artifact integrity、P3以降の全testsを保持する。

## Validation / handoff

developer entryは既存CMake/CTestと`bash scripts/check-format.sh`。
locked bootstrapは`python3 scripts/bootstrap.py`、各shellで`. .deps/activate.sh`。
C17、LLVM/Clang/clang-format 23.1.2、local GCC14.2.0、CMake3.31.6、Python3.12.14。
CIのUbuntu24.04ではGCC13を使用する。

required matrixはGCC Debug、GCC Release/NDEBUG、Clang+format、ASan、UBSanのfull CTest。
新7 testsを含め159 testsで、oracle.adapter/oracle.smoke/artifacts.integrityも実際に実行する。
local GCC Debug / GCC Release / Clang / ASan / UBSanは全て159/159、formatとlocked
bootstrapもgreen。exact current head / pull_request event / five-job resultsはlive Issue/PR handbackのrun URLで
確定する。過去headのgreenをcurrent head evidenceへ代用しない。

S1-S10によるcanonical blockerは見つかっていない。profile外のcompile能力は明示的な
implementation/precision limitsであり新language errorを定義しない。
field mutation、write loan、raw-storage、recursive Node、cJSON、LLVM/relocation/MIR、
F3.1、次sliceは実装していない。required validation後もreview gateで停止し、merge/closeしない。

C emissionの追加precision fence: ptr normal resultはloanのouter rootと同じchecked
place/incarnationへ限定する。body-local root終了後にも保持可能なpersistent ptr tokenは
NewLangでinvalidではないが、C address representationの範囲外としてemission前に
backend unsupportedへ戻す。dangling C pointerのindeterminate valueをhostで読んで
validityを決めたり、runtime guardで補うことはしない。integrationにaccepted tokenの
unsupported controlを固定した。

## Findings classification

canonical spec hole / ambiguity / semantic deltaは発見していない。
`COMPILER-IMPLEMENTATION-LIMIT`: u8 simple-local read-only profile、normal ptr/unit C結果、
body-local ended-root tokenのC表現をunsupportedとする限界。
`COMPILER-PRECISION`: hidden/unknown dependency / richer provenance proofは保守的拒否。
これらの限界はIssue認可のbounded gate内のP1/N1/N2/N3を阻害せず、canonical semanticsを
変更しない。新M/F/Rや次sliceを必要とする回避策は実装していない。
