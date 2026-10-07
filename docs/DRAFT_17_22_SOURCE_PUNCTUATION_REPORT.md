# Draft 17.22 source punctuation production report — Issue #152

Track: P

Base main: `aa12a794791135f656f547c9fa2ae901690a809e`。
CURRENT_SPEC: Draft 17.22。project processとlive §17.1 / §26.3 / §16.3を確認し、
Issue #152のsource syntax/diagnostic/test catch-upのみ実施する。core semantic delta = 0。

## Implementation / invariants

| Task | Evidence |
|---|---|
| P-A lexical/parser | @ punctuation、adjacent :: separator、dedicated field-designator ASTと既存sum constructor AST。legacy dot alias / shared dotted resolverを削除 |
| P-B field | current direct Pair localだけのstatic selection、opaque ProjectionId、fixed subobject/incarnation、Copy read、ordinary non-exclusive write loan、Changeとsibling preservationを既存checkerで維持 |
| P-B sum | resolved concrete type/variant/arity、Copy/non-Copy value-use/ownership、payload occurrence、match/Reset、destructuringを既存sum machineryで維持 |
| P-B recursive | exact Option<ptr<Node>>::None/Some witnessを移行。registration/completion/None constructionはchecker accept、Node field profileとrecursive backendはunsupported |
| P-C destructive | legacy dot、spaced colon、wrong qualifier/variant/arity/type、unknown/wrong/nested/arbitrary/ptr/ref base、module/receiver、同名value/typeの別routeとno fallback、no partial artifactを検証 |
| P-D regression | active source fixturesとnative Pair/recursive integrationを移行。local full GCC Debug/Release、Clang+format、ASan、UBSan各176/176 pass、oracle/integrity/bootstrap pass |

Pair semantic/native witnessはbefore / old / after / sibling / destructured left / rightを
7 / 7 / 11 / 9 / 11 / 9として維持する。generated Cのactual member read、ordinary pointer、
old capture→write、post read/destructureを引き続きassertし、strict C17 compile/native runで
検証する。host seed/helperやliteral-result置換を追加しない。

同名sum `p` / Pair local `p`のfixtureでは`p@left`がu8 field evidence、`p::left`が
sum result/static typeを返す。unknown `p@nope` / `p::nope`はそれぞれfield/variant error。
どちらも相互fallbackやvalue/type precedenceを持たない。

## Changed files / ownership

- `src/lexer.c`：@のtokenizationのみ。token spans/lifecycleは不変。
- `include/newlang/syntax.h` / `src/parser.c`：borrowed field base/label spansの専用node、
  adjacent ::、dot reservation、closed field loan operand。general member/generic grammarなし。
- `src/semantic_check.c`：source categoryから既存selection/read/constructorへ直接dispatch。
  dotted ambiguity/category resolverを削除。semantic modelやbackendは変更しない。
- active unit/support/integration fixtures：sum constructors、Pair fields、recursive witnesses、
  function/control/name testsのsourceを移行。existing OOM sweepsとassertionsを保つ。
- README / production contracts / migration contract/report：current grammarを更新。
  old reports、canonical Draft、frozen oracle evidenceはhistorical authorityのまま保存。

Parser、unit registration、body checking、field read/Change、sum constructors、recursive
completionの既存malloc/realloc fault injectionがnew sourceで走る。全failureはowned tree/
artifact absenceとpublic context snapshot equalityを検証する。新mutable global stateなし。

## Limits / findings / scope audit

現在のconcrete sum sourceはregistered nominal namesとexact Option<ptr<H>>のみ。
Draft例のOption<u32>/Result<T,E>一般generic frontendやu32 familyは未実装として保つ。
このmigrationで新しいgeneric grammarを必要としない。source qualifier/variant/arity/type
errorとbackend unsupportedを混同しない。oracle comparison contractは変更せず、
explicit/input-based/fail-closed。新たなCORE-SEMANTIC-GAP / SOURCE-SURFACE-GAPは未発見。

Node head@next/payload、PR #149 / COORD-149-01、recursive runtime topology、allocation/
lifecycle/raw Storage、cJSON、module/private visibility、FFI、LLVM、次sliceへ進まない。
current one-visibility-domainの既存construction/destructuringをvisibility errorにしない。

## Validation / handoff

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
cmake -S . -B build -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
bash scripts/check-format.sh
```

GCC Releaseは`-DCMAKE_BUILD_TYPE=Release`。Clangは`clang-23`。sanitizersは独立buildに
`-DNEWLANG_SANITIZER=address` / `undefined`。local baselineはDebian 13 x86_64、
GCC 14.2.0、Clang/LLVM/clang-format 23.1.2、CMake 3.31.6、Python 3.12.14。
locked bootstrapはexact URL/version/hash/TLS/provenanceを検証しpinを更新しない。
required PR CIはfresh Ubuntu 24.04、GCC 13 Debug/Release、Clang+format、ASan、UBSan。
Local full CTestは5構成すべて176/176 pass。pinned format checkとlocked bootstrapもpass。
結果、exact head、PR/run URLはIssue #152 / PRのhandoff recordを正とする。
current-head required checks greenでのみreview-readyを宣言する。

PRはOPEN / unmerged、IssueはOPENのまま停止する。
marker: `DRAFT 17.22 SOURCE PUNCTUATION PRODUCTION CATCH-UP READY FOR REVIEW`。
