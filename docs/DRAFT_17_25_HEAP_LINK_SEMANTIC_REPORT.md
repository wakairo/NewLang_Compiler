# Draft 17.25 heap-owned link source-semantic report

Track: P — [Issue #178](https://github.com/wakairo/NewLang_Compiler/issues/178)。
Contract: [source-semantic contract](DRAFT_17_25_HEAP_LINK_SEMANTIC_CONTRACT.md)。

## Authority / implementation

Base main: `060731a121cb29ba6cfe2559d135a4736c3dc429`。
Canonical: CURRENT_SPEC → Draft 17.25 §§10.1 / 17.1。
Issue #176 / accepted merged PR #177のsource/APIだけをproductionへ接続した。
Historical auditはProcess §4.2に対してN/A — faithful implementation。
**Semantic delta = 0**。DI-010はADOPTED / BOUNDEDへ同期した。

Parserはrequested read/write modeを保持する。
Checkerはwrite-capable allocated root取得とmode-preserving fixed link projectionを
別gateとして検査する。checked root取得はselected ptr/stability operands、
root incarnation、D、backing range、accessを所有する。
`NL_CHECKED_FIELD_REF`はparent ref operandとsemantic projection key / child siteを保持し、
`NL_CHECKED_LINK_READ`はcurrent Copy Option packageとprovenanceを保持する。
`replace`は既存Change/Resetへ接続し、old package・ancestor/link factの変更、
fixed incarnationとdisjoint siblingの保持を記録する。
source ASTを破棄した後にもowned function body / arm artifactを検査できる。

## Actual-source evidence

`tests/fixtures/heap_link_semantic.nl`はDraft §17.1のfull witnessを
ordinary `fn main()->unit`で包んだもの。test-only authorityやpreludeを入れていない。

`heap_link_source.integration`の31 controls:

- canonical / renamed H・link・payload / write→read weakening / scoped field-ref alias /
  ordinary write-root aliasをsemantic ACCEPT。
- read-only destination、ptr/wrong root、wrong ptr/stability type、wrong D、payload/unknown field、
  root/field ref escape、active ref対EndRoot、stale acquisitionを拒否。
- omitted/double release、omitted finalize/slot reclaim、invalid None body、Some wildcard、
  nonexhaustive allocation matchを拒否。
- general read、dotted/nested/arbitrary-base accessを既存source/semantic unsupportedで停止。
  borrowed matchは既存function-body precision rejectionとして分類する。

全positiveはsemantic ACCEPT後に **exit 4 / V1-BACKEND-UNSUPPORTED / empty stdout**。
negativeもC/object/executableを生成しない。
このgateは新heap fieldのnative execution実績を主張しない。

`heap_link_evidence.unit`はactual sourceをparse/register/checkし、両owned armsを検査する。
Noneはregion/Dを持たず、Someは1つのregion/Dを所有する。
Someの3 root refs / 3 field projections / 1 Copy read / 2 ChangesとEndRootを検査し、
heapとlexical rootの区別、requested mode、同じprojection site/incarnation、
scope/provenance/dependency継承、CopyされたSome ptr、None→Some→None、
Some occurrence終了、payload sibling package保持、全非Copy責任消費を確認する。
common callerへarm-local authority IDは輸入されない。
checked artifactに対するbackend呼出しもUNSUPPORTED / output NULL / length未変更。

## Supporting claims / rollback

`heap_link_supporting_claims.unit`は**programmatic SUPPORTING evidence**。
sourceで表せないpermissionをlogical semantic fixtureで検査し、
同じproduction write predicateがnon-writable ptr/backing、unknown/invalid provenance、
stale incarnation、dead backing、不適合alignmentを拒否することを確認する。
既存mutation pathにsurviving Value(link)とconditional Some payload refを置き、
Change/Reset blocker rejectionを確認する。Value(payload sibling)は保存する。
host addressやruntime Nodeを使用せず、actual-source success evidenceとは区別する。

`heap_link_failures.unit`は既存fault-injection harnessへfull actual-source witnessを渡す。
registrationとbody/owned artifact検査のmalloc/realloc各failure pointを順に注入し、
OOM後のcaller state / registry / claims / NULL artifact、clean retryを確認する。
新evidence unitは同じsemantic failureを2回再現し、diagnostic code/span、unchanged state、
修正済みsourceでのretryを確認する。

## Validation / limitations

Pinned bootstrap: LLVM / Clang / clang-format **23.1.2**、artifact checksum検証成功。
local host: Debian 13 x86_64、GCC 14.2.0、CMake 3.31.6、Python 3.12.14。
buildとtestは既存CMake / CTest、全196 tests。
local GCC Debug / GCC Release(NDEBUG) / Clang / ASan / UBSanは各**196/196 PASS**。
clang-format-23 dry-runと`git diff --check`もPASS。

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
bash scripts/check-format.sh
```

Clangは`-DCMAKE_C_COMPILER=clang-23`、Releaseは`-DCMAKE_BUILD_TYPE=Release`、
ASan / UBSanはClangで`-DNEWLANG_SANITIZER=address` / `undefined`。
別build directoryで各構成を検証する。
既存PR workflowのGCC / GCC Release / Clang+format / ASan / UBSanを変更せず使用する。
**レビューhandoffのexact head SHA / PR-triggered CI run / 5-job結果はIssue #178と候補PRへ記録する**。
CI greenを独立Coordination ACCEPTやmergeと同一視しない。

全CTestには既存allocated-H source→native (#174)、lexical Node topology、fixed-field/raw/lifetime/
Option、oracle.adapter、oracle.smoke、artifacts.integrityを含む。
oracle・backend・toolchain pins・CI・canonical Draftは変更していない。

Canonical SPEC-HOLE / SPEC-AMBIGUITYやnew semantic blockerは発見していない。
既存のbounded limitationsを明示する:

- **COMPILER-PRECISION**: function body内borrowed matchは`P9-MATCH-PRECISION`。
  occurrence blockerはprogrammatic supporting controlsで検査する。
- **COMPILER-DIAGNOSTIC**: scoped ref escapeは拒否するが既存`P3-INTERNAL`を返す。
  新しいscope escapeを許可したりdependencyを落として成功させたりしない。
- **COMPILER-IMPLEMENTATION-LIMIT**: current allocated slice内のH link ref-baseだけ。
  general/lexical ref-base projectionと新heap field backendはunsupported。

1 allocated H自身のlinkと1 lexical H tailのsemantic gateで停止する。
新heap field native、複数heap roots、allocation/lifecycle redesign、detach、recursive delete、
cJSON、一般member/ref API、FFI、LLVM、次sliceへ進まない。
候補PRはOPEN / unmergedで独立Coordination reviewへ引き渡す。
