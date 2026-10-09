# Draft 17.30 five-root / three-field source admission

Track: P — Compiler #237, original task plus Coordination RESUME
[6079188142](https://github.com/wakairo/NewLang_Compiler/issues/237#issuecomment-6079188142).
This is complete actual-source **semantic admission**, not native execution or
cJSON PASS. Candidate remains OPEN/unmerged for independent Coordination review.

## Authority and scope

開始時Compiler/main: `4943057907f406d1fdc347d6c2147bc9749165c6`。
`CURRENT_SPEC.md` → canonical Draft 17.30 §3.2b、特に§3.2b.3。
FormalProof/main: `608d99505737e011006f902768478430f32c83b8`。
P #244 / merged PR #248のACCEPT済みcertificate基盤を再利用した。
Formalの有限モデルはproduction correctnessやnative実行の証明としない。

Process §§4.2/6、Design Decision Procedure、Testing Strategy、DI-009–014を確認。
Historical design-intent audit: **N/A** — 採用済み§3.2bの忠実な実装であり、
仕様/API/owner lawの裁定やLedger変更はない。semantic delta = 0。
backend、B detach/adoption、一般allocator/owner/field syntax、他Trackは対象外。

## Frozen source and ordinary compiler path

`tests/fixtures/five_root_three_field.nl`を変更していない。
canonical §3.2b.3の15,055文字（終端LFを除く）の全ソースから、comment-only行
だけを除いたfixtureである。5つのinline allocation-siteコメントも残っている。
root、3 link、A.prev=C、明示的0–5 cleanupを削減していない。

| Input | SHA-256 |
| --- | --- |
| Canonical full source including terminal LF | `5145c955343a18d497c2f0568bc78cc0f31c54bdda4e0ff3a0734a6ea5b4fac4` |
| Frozen fixture, comment-only lines removed | `812042833072145f761ddc21cbf774d81b64da3a34f6d466ba7db01a6d704d7d` |

通常の `newlangc tests/fixtures/five_root_three_field.nl` は、実ソースのparse、
独立したdefinition-time checking、既知の `main()` direct-call検査を通過し、
**exit 4 / V1-BACKEND-UNSUPPORTED**になる。stdoutは空、C・実行ファイルの生成なし。
private probeをCLIへ接続したりhost authorityをseedしたりしていない。

Source-unit/body scannerでは既存canonical witnessに含まれる`//`をtriviaとして
処理する。source位置と本文を保持し、scanner外でinlineコメントを消していない。
従来atom/type/expression scannerの制限とblock-comment拒否は維持する。
全canonicalコメントを残したソースも独立したpositive controlとして通る。

## Checked identities, projections and conservation

- 一つのcompleted Hにexact `next/prev/child:Option<ptr<H>>, payload:u8`を登録する。
  既存one-link/two-field HとAVSの経路も維持する。
- 各rootに一つのBackingRegion/full rangeと元のincarnation、LifetimeDomain、
  unique非Copy Allocation/domain packageがある。5組の同時生存を検査する。
- Static opaque ProjectionIdは既存checked fieldの `(nominal,index)`。
  各instanceのchild PlaceId/incarnationは別で、3 siblingは一つのroot/Dを共有し、
  独立したBackingRegionを偽造しない。source/C offsetから権限を推定しない。
- `ref_from_ptr(write,ptr,stable)`のchecked operands、元のptr provenanceとD scoped
  loanを検査した後、root refのread/writeモードを維持してfield refを導出する。
  ordinary writeはnonexclusive、Copy ptrはowner authorityを生まない。
- 各実 `replace` はowned before/after snapshotsとworld anchorを持つ。public
  read-only revalidatorがactual operands、D loan、mode、旧/current ValueFact、
  occurrenceを確認し、private cloneで既存Changeを再検査して全post-stateを比較する。
  両armのancestor-derived common postも独立に検証し、一方のarmをcommitしない。

Source/ASTを破棄してからpublic APIsで、最後のChange後・cleanup前の5 live rootsと
7 current factsを確認する:

```text
src.child=A  A.prev=C  A.next=B  B.prev=A
B.next=C    C.prev=B  dst.child=None
```

6つの本物のscoped replaceがあり、各Changeは選択field/ancestor factだけを更新し、
sibling fields/current occurrenceと元のR/O/D/Aを保存する。Readonly revalidatorは
source textや元のASTを参照しない。Callerのpublic contextにはarm-local region/domain
suffixを持ち出さない。

5つのfallible source matchは各None/Some armを独立検査する。terminal worldごとの
explicit EndRoot、same-range slot/Storage回復、domain finalization、matching deallocate
を追跡し、0/1/2/3/4/5 releasesが各1 worldで成立する。これはsemantic evidenceの
histogramでありruntime counterではない。暗黙cleanup・Allocation remintはない。

## Source positives and adversarial classification

[Machine-readable observations](evidence/FIVE_ROOT_THREE_FIELD_SOURCE_RESUME_OBSERVATIONS.json)
にはinput hash、実CLI exit、stdout/stderr全体、生成fileなしを記録する。
各inputを2回実行してbyte単位のdeterminismも検査する。

| Control | Outcome |
| --- | --- |
| Frozen exact source; H/ptr/domain alpha rename; outer+third arm order permutation; canonical with all comments | Full semantic acceptance + owned 7-fact evidence; backend unsupported |
| Same-address repeated Some replacement | Semantic acceptance; fresh payload package/occurrence; stale old occurrence poison rejected |
| A.prev=B instead of frozen A.prev=C | Memory-safe semantic acceptance; **library-policy mismatch**, not static language error or full positive topology |
| Wrong field, ptr base, @payload, sixth allocation, fourth link | Early bounded profile rejection |
| Second recursive nominal, read→write escalation, wrong domain, cross-Allocation/full Storage, wrong EndRoot domain, duplicate/nonCopy reuse | Semantic refusal |
| Captured None cleanup omitted (sites 2–5); wrong matching Allocation in None (sites 3–5) | Precision refusal for incomplete common post; semantic refusal for wrong allocation |
| Active loan at EndRoot; stale ptr; scoped ref escape | Semantic refusal; escape retains legacy P3-INTERNAL diagnostic code |
| Borrowed Some mutation; unproved root-ref alias | Explicit precision rejection; no fabricated proof or success |

Post-teardown poison tests retain all #244 tuple/world/original/common-post/None/Some trace
attacks and add cloned sibling projections, child/root incarnation, old/new occurrence,
current ValueFact, checked operand, write permission, dependency flag, erased Change,
foreign snapshot epoch, D loan/source/ref operand and missing lifetime-end protection.
Restoring each artifact restores successful validation. Wrong certificate/OOM publishes
no authority. Identical numeric IDs do not prove world identity.

## OOM, rollback and measurement limits

- Actual full source certificate constructor: exhaustive **141** allocation positions.
- Actual full source public revalidator: valid proofの全 **844** allocation positions
  をexhaustiveに検査。Invalid proofでは最初のallocation failureも注入し、OOMと
  通常のprecision rejectionを区別して、証拠を復元したclean retryを確認する。
- Whole actual public registration/checker/direct-call path: **248** deterministic
  distributed injected failures among **15,468** allocation opportunities, covering
  first/last 64 and every 128th. This is explicitly **sampled**, not exhaustive.
  Declaration names/types, arm worlds, Change snapshots, certificate/post-state and
  artifact commit are covered. Public snapshot counts remain unchanged after failure;
  no artifact is published; clean retries and public revalidation succeed.
- Existing exhaustive two-root tests, semantic/raw invariants and ASan/UBSan cover
  partially initialized ownership destruction. No runtime bitmap/owner table is added.

## Reproduction and validation

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
cmake -S . -B build -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
bash scripts/check-format.sh
build/newlangc tests/fixtures/five_root_three_field.nl  # expected exit 4
build/captured_closure_test full-evidence tests/fixtures/five_root_three_field.nl 5
build/captured_closure_test full-poison tests/fixtures/five_root_three_field.nl
build/captured_closure_test full-oom tests/fixtures/five_root_three_field.nl
build/captured_closure_test full-checker-oom tests/fixtures/five_root_three_field.nl
```

Local validation: **246/246 passed in each** of GCC Debug, GCC Release/NDEBUG,
Clang, ASan and UBSan (241 retained + 5 new). Format and `git diff --check` passed.
Local host GCC 14.2.0 / Clang 23.1.2; CI additionally checks GCC 13 on Ubuntu 24.04.
Final local results,
fixed candidate head and five-job pull_request CI are recorded in the PR and the
single final #237 report; no self-referential commit hash is embedded here.
`oracle.adapter`, `oracle.smoke`, `artifacts.integrity` and existing two-root native
custody tests remain in every full suite. Oracle classification remains input-based,
explicit and fail-closed; oracle adapter is unchanged.

Findings: no new spec hole/ambiguity or normative amendment. Bounded unproved alias,
borrowed match and operations beyond the six-Change profile remain
`COMPILER-PRECISION` / `COMPILER-IMPLEMENTATION-LIMIT`; scope-escape legacy code is
`COMPILER-DIAGNOSTIC`. Three-link backend remains unsupported. Historical HOLD/#244
reports and observation files are retained rather than overwritten.

This gate stops for independent Coordination review. No native five-root claim,
cJSON PASS, B detach/adoption, subsequent Issue or Track is authorized by this result.
