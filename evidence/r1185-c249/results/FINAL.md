## R1185 FINAL — finite cohort MATCH / additional CI STOP

**2026-11-05 22:28:50 JST**。C248 の追加自然 1 組は **FULL5＋PARTIAL 全6 jobs success**。全4403 tests（既存P4397＋R6）、0 failed。最終 job は ASan 127262807923、完了 **2026-11-05 22:24:43 JST**。全6原log/gzipを保存し、各1336 members / index1335の全bytes/hashを照合済みです。

### 固定対象・会計

- P: **faad092b5b0deb608a048ff60bee0056082f9a59** / tree **01388ae82b1749453546933cb4ca6b311a25df77**
- R: **a6367e6bc8e52acfee620ca9d840afdc4b700d38** / tree **43ab70ee1fd91d7d17e3054b9831345767f31c6e** / sole parent **290f852ac1acbdde6eed3dc388b1fdf8923b115f**
- Actual PR preview **8fc98c5737f43037bd4f14f7f2598d8c32f504f2**、同じR tree。server commit parents **faad + a636**。runnerのshallow Git %Pは空行の原結果を保持し、server parent根拠と区別。
- FULL run **42032683026**、PARTIAL run **42032683094**、FAST4 run **42032672362**。全同R HEAD / attempt1。PR1189 DRAFT / NEVERMERGE。
- 追加positive passive **216起動試行＝216完了＝216 rc0、timeout0**。全216 comparison **DECODED_FINITE_VALUES_MATCH / differences=[]**、equipment errors0。
- 元 **plain252＋control passive36＝288** を再利用。guest native累計 **504**。旧control36の同AST/map/raw再decodeも全有限値一致。Rの新CHECK/public/C17/AST/plain/control nativeは **全0**。
- FAST **4/6**、通常 **18/18**、manual **0**。C248のforward push1だけ。**以後の追加push/CIはSTOP**。local compiler/build/native/新AST生成0。

### 実値（全6 hosts × 各6 guest modesで一致）

| Case | Header phases | Helper actual/formal | Caller normal result | Old result | Break |
|---|---|---|---|---|---:|
| P01 | 5 | none | none | none | 5 |
| P02 | 1,3 | 1 | 67 | 42 | 3 |
| P03 | 0,2,4,6 | 0,2,4 | 109,109,231 | 14,109,109 | 6 |
| P04 | 1,3,5,7,9 | 1,3,5,7 | 187,187,76,187 | 33,187,187,76 | 9 |
| P05 | 3,4 | 3 | 198 | 198 | 4 |
| P06 | 2,5,8,11 | 2,5,8 | 18,244,18 | 77,18,244 | 11 |

Continue values、selected/sibling、entry Copy、caller result Copy、old-result Copy、outside result/starting-phase Copy、post loadsも凍結V2 oracleと一致。P01はhelper/store/Continue0＋Break1。P05は同値198→198の実C store境界とold198。P06の外側phase Copyは2、loop resultは11。C01はactual/formal5、inferred condition false、normal/Copy162、phase-after5。

guest GCCは全host13.3.0。FULL5のguest Clangは23.1.3、PARTIALはUbuntu18.1.3のまま。O0/O2とguest ASan/UBSanの元host対応・flagsを保持しました。

### 到達境界と限界

V1 NOT_RUN、V2 source/oracle完全不変（oracle SHA256 **29410e20b599aa8ddb243c1dd20895692e2635b32366ebdd6343a3bd45633801**）。**pre-execution frozen independent V1 / scope-calibrated V2; C-supplied finite-profile boundaries**。P/F実装・入力・oracleへの新露出なし。V1のinitializer Copy／caller usize alias／formal-condition aliasはV2で未検証のまま。

NORMAL2でpositive/controlのCHECK/public/C17/actual ASTに到達した原証拠を再利用。negativeは全hostでCHECK rc3/public rc21、register_unitが第一障壁：N01 IF-CONDITION、N02 IF-RESULT、N03 TYPE-MISMATCH、N04 EXIT-DEPENDENCY、N05 SIGNATURE-PRECISION、N06 LOOP-PRECISION、N07 BODY-RESULT、N08 USIZE-ADDITION-PRECISION。N06/N08はlegal coverage probes、N05の深いescape・N08の後続call等はNOT_REACHED。negative C17/native0。早いprecision拒否を深い規則の証明にはしません。

helper caller call-resultとsame-invocation formalは実VALUE観測。**ternary choiceはcaptured operands＋actual ASTからの推論**。callee内部RETURN_VALUEは観測有無を別記し、欠落はNOT_OBSERVED。原root/child/sibling住所は各passive run内だけで照合。public IDはartifact/world付きの原factsとして保持し、数値IDの一致だけでcross-world権限やnative住所を証明しません。**machine CALL/STORE survival、一般freshness/authority/soundness、whole-loop validatorは未証明 / NOT_RUN・NO_APPLICABLE_PUBLIC_API。**

### 原raw custody（各job logのR1185_RAW blockから復元可能）

| Host / job | gzip bytes | SHA256 |
|---|---:|---|
| GCC / 127262808362 | 2366975 | dc949546998c06a367810e17f4394ce266d32913b6ce6f95b2e48a5e080a8ceb |
| Clang / 127262808412 | 2373476 | e9ff3d4fc4a03749b2a481f2a171b1187425fbf92c6dc65d2cbf4904be454c7c |
| GCC Release / 127262808062 | 2368469 | 2fcc01f31659849ed292989f6ed7a8fe04953e138f81d44b056e820a7ccdf96c |
| UBSan / 127262808183 | 2374101 | 143e0a2a1e761569317820f77670e854aa2bee3c5ab295905064a9f1abcde591 |
| ASan / 127262807923 | 2374164 | 3061e7ae464b8898a9044a08ca06fb8a066943cb8695a2264b34376d167ad840 |
| PARTIAL / 127262808700 | 2373025 | d517cd6e35853e2b4ec006f4c3c8ea8631a69f4497a09cb4c6f39349f4368746 |

原FIRST/RED、旧ref、V1/V2、E0–E6、旧NORMAL2 gzip/map/raw、全comparison-INCOMPLETEを未改変で保存。成功結果で消去していません。

**Rの結論：今回固定した有限cohortについて、凍結期待値・観測同一性の不一致は見つかりませんでした。採用判断をCへ返します。** 小さい結果receipt bundleを追記しますが、追加実行・機材変更はありません。