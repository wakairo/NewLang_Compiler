# P9 explicit-return lexical-block production report

Track: P。Implementation complete。review handoffとexact-head CI証拠は
[Compiler Issue #60](https://github.com/wakairo/NewLang_Compiler/issues/60)の最終`Track: P` reportに記録する。
PRはopen/unmergedで停止する。

## Authority / Phase A

base main: `b3d62614cd47da661067d6d92b703336675e605e`。
`CURRENT_SPEC.md` → **NewLang_v0_spec_Draft17_11.md**。
運用方針を最初に読み、production変更前に
[Phase A](https://github.com/wakairo/NewLang_Compiler/issues/60#issuecomment-6009524967)を投稿した。
専用item、局所termination outcome、zero-normal-arm、既存exit predicate、owned branch evidenceで
canonical ruleを表現可能と判断した。canonical snapshot/backend/toolchain/LLVM/workflowは変更しない。

## 実装

- `syntax.h` / `parser.c`: dedicated RETURN block item、expression child、exact semicolon/span、malformed/context boundary。
- `checked.h`: explicit `terminates`、function `returned`とnormal `results`の分離、`normal_arms`、explicit unit literal。
- `semantic_check.c`: return expression/transfer/exit、nested propagation、unreachable item/tailのsemantic skip、
  flat-sum registered signatureとzero/one-normal-arm body match、actual-state plan check。
  hypothetical arm state/IDはowned evidence内に留める。
- `semantic_internal.h`: trusted controlled body boundary。sourceによるfunction context admissionを追加しない。
- call終了時、result sumのpayloadへtransfer済みのnon-Copy argumentを誤って終了させない。
- CMake/new P9 tests、旧P8の「return syntax自体が未対応」というscope expectationだけを更新。
  source declaration/recursion/body-call等の既存rejectionは保持。

契約・source/precision限界は[P9 contract](P9_EXPLICIT_RETURN_SLICE_CONTRACT.md)を参照。

## Required pressure / break matrix

| pressure | production evidence / result |
| --- | --- |
| direct return、nested lexical return、binding initializer経由 | transfer groupでaccept。後続non-Copy use/unknown tailをcheckしない |
| match arm return | decoder/availability groupsでaccept |
| one returns / one continues | normal_arms=1。return側のconsumeがcontinuation側のAvailable stateを汚染しない |
| all branches return | normal_arms=0、terminates=true、type=0/result_count=0、tail/normal unitなし |
| non-Copy result | original caller package IDをtransfer、caller bindingはConsumed |
| non-Copy payload result | wrapper/decoder Errでoriginal Error ID保存、result ownerの下で生存 |
| Copy result | callerはAvailable、fresh copied packageを返す |
| wrong result type | `P9-RETURN-TYPE`、registration/context不変 |
| ordinary-ref return | `P9-REF-RETURN`、ref result signatureは既存P8 precision fence |
| return-edge exit dependency | W2のreal local scope/ref/carrier + source returnで`P9-RETURN-DEPENDENCY` |
| restore-before-return | 同じW2 stateをoriginal dependencyへ復元後、同じsource returnがaccept |
| no implicit cleanup | unused non-Discardable parameter/Err payloadは`P5-SCOPE-OBLIGATION` |
| standalone return | `P9-RETURN-CONTEXT`、repeat diagnostics/state不変 |
| callable / loan boundary | source-fragment parserがreject。P2 opaque loan-headerはbody意味を判定しない |
| missing semicolon / bare return / tail return | syntax groupでreject、span/code保持。unreachable malformed returnもparse reject |
| exactly-once evaluation | return `replace(dst,v)`のold package IDとchecked operation数=1、後続store数=0 |
| normal reference facts | sole normal armのactual place/scope/provenance保存、caller scope終了後はdead-scope reject |
| semantic failure after caller write | `store`後のreturn operandがjoined-ref precision reject、public state全体rollback |
| OOM | parser、body registration、guarded match/context/evidence、actual call/result binding、visible-write/return attachment、trusted W2 entryをmalloc/realloc sweep |
| resource limit | 64 owned-arm evidence capacity超過をstructured reject、registration state不変 |

### W1 — decoder early failure

concrete Result-like decoderを両actual variantで実行。
Copy PacketのOkだけがnormal header bindingへ進み、non-Copy/non-Discardable ErrorはErr returnへtransfer。
guarded Err evidenceはnormal joinへ入らない。all-return版も両variantを実行する。
public returned Errorは元payload IDであり、guarded-context IDではない。

代表`match parse(input)` source形はparser検査済み。
**実行fixtureはhost-prepared Resultを受けるdecoder段階**である。
P8 body→body callは依然rejectするため、nested parser helperまでsemantic実装したとは主張しない。
generic declarationやparser libraryの追加は行わない。この境界はCOMPILER-PRECISION / completenessとして明記する。

### W2 — restore before return

sourceからlocal-dependent stateを構築するsurfaceは未実装。
controlled fixtureでcaller-visible carrierへ本物のlocal-scope refを設置し、trusted boundaryで
`{return unit;}`を**registered bodyと共通のproduction block/return/exit flow**に通した。
依存残存時はdeterministic return diagnostic、artifactなし、全public state不変。
scopeをinactiveにするだけでも既存exit predicateはrejectする。
元packageを復元すると同じsourceがacceptする。success pathの全OOM点もrollback sweepした。
既存P8 exit testsのmay-reference/global+local/dead-scope casesも保持した。

## Validation

local configurationごとに**72/72 CTests**（既存65 + P9 7）。
新group: `p9_syntax`、`p9_transfer`、`p9_exit`、`p9_failure`、`p9_effects`、`p9_availability`、`p9_decoder.integration`。
assertionに依存しないCHECKでRelease/NDEBUGも同じtestsを実行する。

| local host configuration | result |
| --- | --- |
| GCC 14.2 Debug | 72/72 |
| GCC 14.2 Release/NDEBUG | 72/72 |
| Clang 23.1.2 Debug + clang-format-23 | 72/72 + format |
| Clang 23.1.2 AddressSanitizer / leak detection | 72/72 |
| Clang 23.1.2 UndefinedBehaviorSanitizer | 72/72 |

P3/raw/P5/P6/P7/P8/R4-01、source/parser/OOM、LLVM C API smoke、oracle、artifact integrityを全て含む。
LLVMに変更はなく、既存smokeをregressionとして走らせるだけである。
既存PR-triggered workflowのUbuntu 24.04/GCC13/Clang23/ASan/UBSan/formatを使用する。
**exact final head SHA / run URL / 5-job successはIssue #60最終reportとPR checksを正とする**。
過去headのrunをfinal head証拠として使わない。

再現:

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake -S . -B build-p9-gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build-p9-gcc --parallel 2
ctest --test-dir build-p9-gcc --output-on-failure
# targeted:
ctest --test-dir build-p9-gcc -R p9 --output-on-failure
# Repeat with GCC Release, clang-23 Debug, and clang-23
# -DNEWLANG_SANITIZER=address / undefined, as documented in README.
bash scripts/check-format.sh
```

## Findings / handoff

- COMPILER-SPEC-HOLE / COMPILER-SPEC-AMBIGUITY / canonical contradiction: **発見なし**。
- COMPILER-IMPLEMENTATION: result-ownerへtransferしたpayloadのcall temporary終了を修正、破壊テスト追加。
- COMPILER-PRECISION: zero/one-normal-arm、concrete variant、flat plain payload、pending-operand termination、
  nested/body helper呼び出しの既存限界。無証明successやprivate ID importで補わない。
- DIAGNOSTIC-QUALITY: 詳細body trace/mandatory unreachable lintは未追加。first errorと有効source spanを保持。
- source fn/top-level、recursion/forward-reference policy、generic declarations、function pointers、
  callable/loan closure、general CFG/SSA/bottom、F2、新R、M、LLVM/relocation、FFI/modules/concurrency、別Pは未着手。

review-readyの停止条件はfull exact-head CI成功、open/unmerged PR、Issue #60 final `Track: P` report。
そこで**P9 READY FOR REVIEW**とし、merge/Issue close/次track開始を行わない。
