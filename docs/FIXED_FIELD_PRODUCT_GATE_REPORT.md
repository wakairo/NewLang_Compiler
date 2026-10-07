# Fixed-field product gate implementation report — Issue #135

Track: P

Base main: `12d49d049aae6831ce42a7a0b7b55934f01d9a2a`。
CURRENT_SPEC: Draft 17.20。FormalProof authority:
`8d7026970cda1cd5689d73025f40d5c8ac6cb5fd`。
Issue #129 architecture、#130 value evidence、R10 CLEAN、R9-01 CLOSED、merged formal
adjunctをauthority auditで確認した。semantic delta = 0。normative/oracle/toolchain
snapshotとdependency lockを変更していない。

## 実装とE1–E9

| Evidence | Production実装 / test evidence |
|---|---|
| E1 fixed subplaces | Pair installation時に二つのchildを一度生成。parent/incarnation/index/type relation、非所有member view、parent endでのdetachを検証 |
| E2 Copy read | current memberのCopy。before=7、after=11、sibling=9。Pair Availableを保持し、readからwrite authorityを作らない |
| E3 ordinary write | childへのnon-exclusive ordinary ref。implicit parent stability、same incarnation、nonescape/normal forwardingをchecked artifactで検証 |
| E4 atomic Change | parent/target package+factsをrefresh。root/child incarnations、right ValueId/fact/place metadataは不変。old parentだけhistorical ended、member sole ownership維持 |
| E5 old/post coherence | actual-source bodyでbefore/old/after/sibling/destructured left/right = 7/7/11/9/11/9。whole Copyの別carrierとsequential replaceも検証 |
| E6 dependencies | exact left/parent atomはsemantic reject、right-only atomはaccept/preserve。Unknown/hiddenはprecision reject。function exit/root end/control fenceも検証 |
| E7 artifact | resolved symbol、nominal/index/type、parent/child IDs/incarnations、pre/post facts、old/new packages、scope/dependency completion evidence。backendはsourceを再解釈しない |
| E8 C/native | actual member read、mutable Pair carrier、ordinary field pointer、old capture→write、post read/destructure。strict C17 host compile/native success、deterministic emission |
| E9 transaction | parser、unit registration、actual function body、attach/read/Change、receiving/exitのmalloc/realloc failure sweep。全failureでcontext snapshot/history不変、artifactなし。counter exhaustion/invalid ownership/stale child、backend unsupported/output errorも検証 |

Primary sourceはIssueのwitnessそのままのprofileを用い、test-only seed/prelude/helperで
実行可能化していない。native exitだけを意味の証拠にせず、semantic valueとchecked
identity/facts、generated C fragments、host executionの三層を組み合わせる。
runtime I/Oや特殊NewLang exit codeは追加していない。

## Files / boundaries

- `src/fixed_field.c`：non-owning child relation、package transfer、bounded exact
  overlap/dependency、ownership invariant。mutable global stateなし。
- semantic/checked/syntax headersと`semantic_internal.h`：inline owned atom、child
  metadata、point-in-time checked field evidence、neutral dotted syntax。
- parser/checker/semantic/function_body/control：source integration、category
  no-fallback、Copy use、ordinary loan、Change、end/exit/fork precision fences。
- `src/main.c`：bounded resolved-field validator、member lowering、checked carrier
  mutability。general backend frameworkを作っていない。
- tests/CMake：7 unit groups + 1 end-to-end integrationを全CI matrixに登録。
  old one-level field parser-negativeを新coverageに合わせnested-negativeへ更新。
- READMEとcontract/report：implementation coverage/limit/verificationを記録。

## Negative / precision / regression

unknown field、wrong nominal/profile base、field-vs-sum ambiguity、no fallback、wrong
new type、read-as-write authority、scoped ref escape、dead/stale parent、exclusive
conflict、nested/general member、range errorをreject。CLI negativesはno C/executable。
semantic store / valid scalar-returning call等、backend外のaccepted constructは
language-invalidにせず明示的unsupportedで止める。Non-Copy partial moveは従来の
prohibitionを保つ。V0/V1/AVS/local-root/stable-root/R9の既存testsを維持する。

COMPILER-PRECISION：exact may-atoms最大4、live exact evidenceを含むsource IF/MATCH/LOOP
は未対応としてreject。programmatic ref join / ptr conversionのexact inputもprecision
rejectし、dependency launderingを防ぐ。field swap、nested/general/member/ptr base等はprofile外。
canonical gap/ambiguity、新ownership/lifetime/effect ruleの必要性は発見していない。
dependency evidenceはtrusted internal controlsで検証し、新source annotationやdependency
producer language featureを追加していない。oracle overlap classificationは未変更、
explicit/input-based/fail-closed adapterとfrozen archive integrityを維持する。

## Validation / handoff

Local environment: Debian 13 x86_64、GCC 14.2.0、Clang/LLVM/clang-format 23.1.2、
CMake 3.31.6、Python 3.12.14、C17。`python3 scripts/bootstrap.py`は既存lockの
exact URLs/versions/SHA-256/TLS/provenanceを検証して成功し、pinを書き換えない。

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
cmake -S . -B build -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
bash scripts/check-format.sh
```

Releaseは`-DCMAKE_BUILD_TYPE=Release`。Clangは`-DCMAKE_C_COMPILER=clang-23`。
sanitizersは独立buildに`-DNEWLANG_SANITIZER=address` / `undefined`を指定する。
CIは既存Ubuntu 24.04 fresh runner、GCC 13 Debug/Release、Clang+format、ASan、UBSan
の全jobでfull CTestとlocked bootstrapを実行する。exact-headのSHA/run URL/statusは
Issue #135とPRのhandoff recordを正とし、過去headのgreenでreview-readyを宣言しない。

Local validation結果：GCC Debug、GCC Release/NDEBUG、Clang、ASan、UBSanのすべてで
169/169 CTests green。format check green。LLVM C API smoke、oracle.adapter、
oracle.smoke、artifacts.integrityも各configurationのfull suiteに含めてgreen。

PRはopen/unmergedで停止する。raw-storage、recursive Node、cJSON、LLVM、M/F/R、次slice
を開始しない。review handoff markerは`FIXED-FIELD PRODUCT GATE READY FOR REVIEW`。
