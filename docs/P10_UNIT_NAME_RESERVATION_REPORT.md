# P10 unit ordinary lexical name reservation — production report

Track: P。targeted fixは[Compiler Issue #65](https://github.com/wakairo/NewLang_Compiler/issues/65)。
review handoff / exact final headとCI証拠は同Issue最終`Track: P` reportとopen PR checksに記録する。

## Authority / implementation audit

開始main: `d3bd67cadc67ddcb35371e12e6e45beb3f8ee34b`。
最初に`NewLang_Project_Development_Process.md`を読み、`CURRENT_SPEC.md`が指すDraft 17.12の
§4.9 / §20.1 / §12.1を確認した。canonical Draftは変更しない。
[実装前audit](https://github.com/wakairo/NewLang_Compiler/issues/65#issuecomment-6012487577)を投稿済み。

現在のordinary binding introducersはP2/P3 binding、P5 single/multi/destructuring、
P6/P7 consuming/borrowed match payload、P9 function match payload、P8/P9 host body parameter。
P3 loan-headerの`as name`もordinary ref bindingのplan nameである。
programmatic seed/bind-resultは同じlow-level binderへ到達する。
registered ordinary function名はsource callで使うordinary nameとして防御する。
aggregate field / sum variant labelsはmember namespaceであり、このrestrictionを適用しない。

builtin unitはcore type ID 1、expressionの`NL_CHECKED_UNIT`として既存実装済み。
return / statement / tailは共通expression pathを使う。
NAME argument shortcutとwrite-parameter inferenceはlexical bindingを参照していたので、
exact unitを既存literal pathへ送るよう揃えた。

## Exact rule / architecture

`nl_sem_lexical_name_admissible(bytes, length)`はborrowed spellingに対し
**length=4かつbytes="unit"だけ**を不受理とする。
case/prefix/suffixを一律に予約しない。`Unit`、`unit_`、`units`はこのruleによって拒否されない。
新しいreserved-name table、keyword token、lookup priority modelは導入しない。

- source/host-body introduction preflightの共通diagnosticは`P10-RESERVED-NAME` / semantic error。
- P3 receiverはRHS前、P5全receiversはRHS/publication前、match payloadはarm publication前に検査する。
- body function/parameter metadataはowned plan作成前に検査する。
- loan-headerはnameだけを検査。bodyはopaque / nonescape未checkのまま。
- `nl_sem_bind_in_scope`にも同一predicateを置き、seed/bind-result等でlexical unit bindingを作れないようにする。
  diagnostic outputの無いprogrammatic APIは既存semantic-error statusを返す。
- member labelsやgeneric string-copyにはpredicateを適用しない。
- `unit`は既存literalとして解決し、unit型parameterへのactualもlexical lookupを通さない。
  reference authorityとしてunitを使うinvalid operationは依然rejectする。

P9 control-flow/termination/exit/ownership/既存unreachable handlingは変更しない。
clone/check/commit、first error、owned source/body/arm artifactsのfailure cleanupを維持する。

## Required evidence

| pressure | result |
| --- | --- |
| original `{let unit=x;return unit;}` registered unit-result body | `P10-RESERVED-NAME`、body plan未publish |
| P3/P5 single let / nested block / `let unit=unit` | 同じname diagnostic、RHS lookupよりreceiverを先に検査 |
| multi receiverのfirst/secondがunit | 同じdiagnostic、他receiver未publish、take/ending state不変 |
| `Record {ok, unit}` shorthand | local unitをreject、ok未publish。field label unitのconstructionはpass |
| consuming/borrowed/function match payload binding unit | 同じdiagnostic、payload/local/public state未publish |
| registered body parameter/function name unit | 同じdiagnostic、registry/owned plan不変 |
| programmatic seed value/domain/bind-result、signature-only function name | semantic error、output sentinelと全state不変 |
| return unit / unit statement / unit block tail / type/value unit | core identity / singletonを保持 |
| `let alias=unit` / `observe_unit(unit)` / registered body unit argument | pass、literalにlexical symbolなし、既存unit parameter compatibilityを使用 |
| `Flag.unit` / match variant `unit` / `Record{unit:x}` | member labelsはpass、ordinary binding unitは作らない |
| store後のforbidden name | caller-visible writeも含め全state rollback |
| OOM | rejected receiver/multi/destructure/match、valid body registration/actual unit argumentをmalloc/realloc sweep、failureはstate不変/artifactなし |

registrationに渡したsource/tree/parameter namesの所有権は既存P8 contractを保持する。
source/treeは通常のsource-check artifactに対してborrowed、owned body planは入力owner終了後も有効。

## Validation / reproduction

既存72 + new 3 = **75 CTests**。
new groups: `p10_admission.unit`, `p10_builtin.unit`, `p10_failure.unit`。
全P9 return、P3/raw/P5/P6/P7/P8/R4-01、parser/source/OOM、LLVM smoke、oracle、artifact integrityを保持する。

required local validation:

| configuration | result |
| --- | --- |
| GCC 14.2 Debug | 75/75 |
| GCC 14.2 Release/NDEBUG | 75/75 |
| Clang 23.1.2 Debug / clang-format-23 | 75/75 / format |
| ASan / leak detection | 75/75 |
| UBSan | 75/75 |

既存PR-triggered Ubuntu 24.04 workflowのGCC13 / Release / Clang+format / ASan / UBSanを使用する。
**exact final head SHA、run URL、全5-job greenはIssue #65 final report / PR checksを正とする**。
過去headのgreenをfinal evidenceに使わない。
bootstrap、LLVM pin、CI workflow、canonical snapshotを変更していない。

```sh
python3 scripts/bootstrap.py
. .deps/activate.sh
CC=gcc cmake -S . -B build-p10-gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build-p10-gcc --parallel 2
ctest --test-dir build-p10-gcc --output-on-failure
ctest --test-dir build-p10-gcc -R p10 --output-on-failure
bash scripts/check-format.sh
# READMEと同じRelease / clang-23 / sanitizer=address,undefined configurations。
```

## Findings / limits / stop

新しいcanonical ambiguity / spec holeは発見なし。
productionのname admission漏れとunit argument shortcutをtargetedに修正した。
host parameter namesはsource parameter spanを持たないためbody spanをdiagnostic locationに使う。
source fn/callable/loop等の未対応introducerを新たに実装せず、既存profile/precision fencesを保持する。

R5-01 targeted revalidation、M/F/R、general keyword/name-system redesign、source fn、
LLVM/relocation/FFI/modules/concurrency、別milestoneは未実施。
exact-head validation後、open/unmerged PRとIssueへの`Track: P` reportを残し、
**P10 READY FOR REVIEW**で停止する。merge/Issue close/次track開始は行わない。
