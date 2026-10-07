# Draft 17.23 Node @link production contract — Issue #159

Track: P

Base main: `457bc4eea79441f12880daf808d916a7ae52e80d`。
CURRENT_SPEC: Draft 17.23。これはproductionのbounded実装契約であり、normative Draftを変更しない。

Historical design audit: N/A — faithful implementation of Draft17.23 §§16.3, 17.1, 13.7/13.8, 26.

開発プロセス§4.2、Design Decision Procedure、LedgerのDI-001/002/007を照合した。
`@` / `::` / reserved dotの分離を維持し、module/privateやptr/ref-baseの一般projectionを
追加しない。nominal/fieldのspellingやC layoutはsemantic authorityではない。

## Exact production mapping

| Canonical rule | Production boundary |
|---|---|
| §16.3 completed recursive H | committed header/completion metadata、exact `Option<ptr<H>>` link + u8 sibling、Copy/Discardable propertyを照合 |
| §17.1 `local@link` | 既存dedicated field AST。available/current/live/implicit lexical rootから、committed nominal field indexとfixed child identityへ解決。Pairの既存両fieldを維持 |
| §17.1 Copy read | current Option packageとptr payloadを独立にCopyし、provenance / reference facts / dependency atomsを保存 |
| §13.7/13.8 link write loan | childを指すordinary non-exclusive `ref<write,Option<ptr<H>>>`、enclosing rootのhidden stability、exactly-once body、scope-exit / nonescape / unchanged result forwarding |
| §17 Change + §26.7 | whole link packageをreplace/store。root/link incarnationを維持、root/link ValueFact更新、disjoint u8 siblingを保存。old conditional occurrence End、新Some occurrence fresh、Some→Someも同じ |
| §13.7/13.8 Node read | core u8に加え、exact completed recursive lexical rootのみread / safe ptr read reacquisitionを許可。visible implicit governing root/current incarnation/type/provenance/access/dependency proofを要求 |
| §26 finite match | Node Optionの全normal armがdependency-free unitを返し、incoming public frameが厳密に不変の場合だけnormal unit join。全armを検査し、arm-owned ID/stateをpublicへimportしない |

fixed childはaggregate-owned Optionのborrowed viewであり、第二のpackage ownerではない。
Option payloadのconditional memory occurrenceはこのchildの下にattachされる。
replaceのreturned old Optionはold semantic payloadを保持し、old memory occurrenceは移さない。
rootをEndするとowned field / Option / ptr packageを再帰的にEndする。新しいcleanup ruleではない。

## Invariants / ownership

- `NLCheckedField`はnominal/type、parent/child、各incarnation、pre/post ValueFacts、old/new
  packageとpre/post payload occurrenceをpoint-in-time evidenceとして保持する。
- ASTは新設しない。registrationはowned body source/treeを保持し、checked artifactはsyntax
  node pointerを保持しない。match armのIDsはそのartifact-owned contextにだけ属する。
- 正常なordinary aliases自体はwriteを禁止しない。actual mutationはexact current-value
  dependencies / live payload occurrence refs / exclusive conflictsで検査する。
- ptr tokenはaccess/stability authorityではない。visible live lexical governing rootが必要。
  same PlaceIdでもfresh incarnationならold ptrは失効したまま。
- hidden/Unknown dependencyをdependency-freeへ変換しない。足りないproofはprecision rejection。
- public checking / registrationはprivate candidate内で実行し、semantic failure/OOM/budget
  failureでcontextをcommitせず、checked outputも返さない。

## Backend / stop boundary

`tests/fixtures/node_link_semantic.nl`がprimary actual-source witnessである。
tail read → persistent ptr → head link Some → Copy observation → match/pointer reloan → None
まで既存checkerで検査する。seed/helperをprimary witnessへ追加しない。

このsourceはchecker-validだが、CLIはdeterministic `V1-BACKEND-UNSUPPORTED` / exit 4 / empty
stdoutで停止する。C/object/executableを生成しない。Pair/u8/AVSの既存native regressionは維持する。

Node payload、unknown member、ptr/ref/nested/expression base、wrong nominal/type、legacy dot、
aggregate-root writeを許可しない。general Node member、recursive backend、executable topology、
allocation/lifecycle、raw-storage、cJSON、LLVM、FFI、modules、FormalProof、spec変更はscope外。
old #148 / PR #149も変更しない。独立Coordination reviewまでPR OPEN / unmergedで停止する。
