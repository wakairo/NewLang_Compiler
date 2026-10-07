# North Star V1 — revised typed-scalar production report

Track: P

## Task / authority

- Issue: [#103](https://github.com/wakairo/NewLang_Compiler/issues/103)（継続、OPEN）
- branch: `north-star-v1-typed-scalar`
- base main: `b0ce91187396797040211824a9bc49c3073007a2`
- canonical: `CURRENT_SPEC.md` → Draft 17.17（変更なし）
- disposition: Step 15 **RESUME V1 — REVISED ACCEPTANCE**
- mandatory: **P1 / N1 / N2**、semantic delta **0**
- `u8(-1)`: **Deferred / unchanged**、新contract / test / evidenceなし

PR・review handbackのexact head SHAとCI run URLはIssue/PR上のfinal evidenceに記録する。
commit内に自身のSHAを循環的に埋め込まない。仕様文書・toolchain pin・oracle archiveは変更しない。

## Implemented spine / evidence

`u8(DIGITS)`のpositive decimal shapeを専用syntax nodeにし、checkerでcore u8と
mathematical rangeを決定する。boundを越えるdecimalはhost overflowなしでrejectする。
accepted literalはowned checked artifactへtype / known scalar valueを保存する。
existing receiving、Copy identifier、Discardable statementを再利用し、Checked-Cは
checked valueから`const uint8_t` initializerとsymbol-ID-based local useを生成する。

- E1: actual `fn main()->unit{let x=u8(7);x;unit}`をparse/registerし、registration input
  owner破棄後のchecked call bodyでcore u8 / value7 / Copy local useを確認。
- E2: integrationでC initializer / explicit local useを確認。`007`はchecked decimal7に
  lowerし、emitterへsource bytesやsyntax treeを渡さない。
- E3: emitted C17をstrict host compilerでcompileし、native成功を確認。
- N1: `u8(256)`は `V1-U8-LITERAL-RANGE`、stdout空・C/nativeなし。
- N2: bare `7`は `P5-EXPECTED-EXPRESSION`、stdout空・C/nativeなし。
- boundaries: 0 / 255 accept、巨大decimal reject、既存contextへのfailure rollbackを確認。
- language-valid u8-returning function callは今回のbackend subset外として
  `V1-BACKEND-UNSUPPORTED` / exit4 / stdout空を確認。

Cコンパイラはrange/type oracleではない。NewLang rejectionはemission前に完了する。
C representationはbackend detailであり、source ABI/layout/implicit conversionを追加しない。

## Failure / integrity

source block、source function-unit registration、real body-sensitive direct-callに対し
malloc/reallocを順番にfault injectionし、OOM時のpublic semantic snapshot不変・
checked artifact未公開・再試行成功を検査する。negative semantic checkは同じcontextで
繰り返し、diagnostic codeとstate不変を確認する。新しいmutable global production stateなし。

frozen M7.5 `tests/programs` 241 fixturesをinspectしたところ、`u8(`の入力は0件。
現在のoracle smoke inputに新しいoverlapは生じないためadapter/smokeを変更しない。
既存のexact input exclusionとunexpected outcomeのfail-closed behaviorを維持する。
V0 integrationとartifact integrityも既存testを保持する。

## Validation / reproducibility

local baseline: Debian 13 x86_64、GCC 14.2.0、Clang/LLVM/clang-format 23.1.2、
CMake 3.31.6、Python 3.12.14、C17。checksum-locked rootless bootstrapを再実行する。
CIは既存Ubuntu 24.04 matrix（GCC13 Debug / Release、Clang23 + format、ASan、UBSan）。
新testも通常のfull CTestへ登録する。CI workflow・pinを変更する必要はない。

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake -S . -B build-v1-gcc -DCMAKE_BUILD_TYPE=Debug -DNEWLANG_SANITIZER=none
cmake --build build-v1-gcc --parallel 2
ctest --test-dir build-v1-gcc --output-on-failure
bash scripts/check-format.sh
```

Releaseは`-DCMAKE_BUILD_TYPE=Release`。Clangは`CC=clang-23`。
ASan/UBSanは別build directoryで`-DNEWLANG_SANITIZER=address` / `undefined`。

local full CTestは **146/146 PASS** × GCC Debug / GCC Release / Clang / ASan /
UBSanの全5構成。clang-format check、checksum bootstrapもPASS。
このfull CTestにV1 unit/integration、V0 integration、oracle.adapter、oracle.smoke、
artifacts.integrityが含まれる。exact-head CIの確定証拠はIssue/PR review handbackで報告する。

## Scope / findings

既存§6をpositive decimal u8 profileへ接続するだけで、new language semanticsは0。
N3のR3 source-surface gapはDeferredのまま。今回新しいcanonical blockerはない。
backend capabilityは引き続きboundedで、unsupported accepted programsは明示的に停止する。
aggregate / ptr / sum / allocation / arithmetic / other numeric families / generic IR /
LLVM / F3.1 / V2+へ進まない。open/unmerged PRでreviewを待つ。
