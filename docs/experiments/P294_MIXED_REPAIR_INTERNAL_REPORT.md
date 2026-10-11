# P294: 終了済みwhole packetの二重終了に限定した修正

**UNSELECTED / DRAFT / DO NOT MERGE。**
`P R291 REPAIRED MIXED PACKET INTERNAL: FIXED ISOLATED DIAGNOSTIC SEAM — UNSELECTED`
5-Hのcallee再構築を含むwhole-call証明は引き続きHOLD。

## 固定authorityと再現

- Compiler/main: `b75baea96a644e68634baee383299b66981b3c62`。
- Canonical: `docs/reference/CURRENT_SPEC.md`のDraft 17.30。
- 調査対象・このbranchのbase: PR286
  `5303fcb481f7f15e9007920c106534b17ccacd69`。
- 独立R291 generator: `696d7f4f31b77a504e47db760af2ee1f2cb4446a`。
- ORIGINAL_GRANT / NESTED_CALLER / TRANSITIVE_TERMINAL /
  OWNER_AGGREGATESを全てON。通常buildでは全てdefault OFF。
- Draft17.31 / DI-015は未採用。PR286、P289、canonical、仕様、R証拠は変更しない。

元generatorが生成した次の2ファイルをbyte-identicalなfixtureとして保存した。

| source | SHA-256 |
| --- | --- |
| `mixed_alloc_repaired_5.nl` | `48f19bbd6ddd3b8ae1057a4f35a0fff95963c4af2930a9f44d9d5087ed5f12db` |
| `mixed_domain_repaired_5.nl` | `870516e459cfe85eb59d9ecf05af2f8481cdd7e2fe254e4e92d8935c34f6c4e5` |

固定compilerの両ケース: CLI exit **3**、stdout/Cなし、
`190:1–190:20 error(internal)[P3-INTERNAL]: invalid semantic module state`。
2回ずつのexit/stdout/stderrは完全一致。
公開parse APIはOK、function-unit登録はstatus **6 / INTERNAL_ERROR**。
公開snapshotは失敗前後で同一。entry check/backendは未実行、owned entry artifactなし。
固定binary SHA-256は
`0e9594c032083eed67954e7d9ec751f46d59eb804873f8189c7d967044c9a106`。
生の観測は[p294_evidence/frozen_originals.json](p294_evidence/frozen_originals.json)。

## 最初の状態破壊と後から検出する場所

parserや原始的な対応関係検査による拒否ではなく、**call後処理の状態管理バグ**。
固定`src/semantic_check.c`の`call()`で、`run_body()`完了後の引数後処理が、
既にwhole destructureでENDEDになった元packetにも`end_temporary()`を呼ぶ。
`nl_sem_end_value()`は歴史的fieldsを再帰的に終了させるため、
別の戻りpacketへ移転済みのmemberまでENDEDにしてしまう。

隔離した観測用objectで固定コードの前後にread-only validatorを置いた結果:

```text
whole input=133 result=138 entry_values=133
before-call-cleanup: nl_sem_validate=0, nl_raw_validate=0
after-call-cleanup:  nl_sem_validate=6, nl_raw_validate=6
```

Allocation IDs `[1,15,29,43]` / R `[1,2,3,4]`、
Domain value IDs `[6,20,34,48]` / D `[1,2,3,4]`が、
この後処理だけでAGGREGATEからENDEDになる。第5 rootのA57/D62はPLACEのまま。
Allocation混合時、callee戻り直前のA1/A15のownerは137/135、D6/D20は135/137。
Domain混合時にはA1/A15のownerは135/137、D6/D20は137/135。
移転先が実際に異なることと、IDを作り直していないことを観測した。
観測用instrumentationはproduction patchに含めない。

最初に破れる不変条件は`src/fixed_field.c:nl_fixed_validate()`の
**live aggregateの各fieldが、そのaggregateをownerとするAGGREGATE carrierであること**。
同時に`src/raw_storage.c:nl_raw_validate()`のlive Rごとの唯一のAllocation条件も破れる。
callerのwhole unpackで一部が再びLOOSEになるため、元diagnosticは後続の最初の
`retire(corrected_0)`内部で残った欠損を検出する。
検出経路は`call → run_body → function_block → source_block → source_binding →
nl_raw_validate → host(P3-INTERNAL)`。固定コードの`source_binding`では、
original関係検査に成功した後のraw検査で失敗している。
通常の意味エラーをinternalに翻訳しただけの問題ではない。

## 最小化と限定修正

交換もcaller修復も不要だった。測定した最小再現は、1-link Hを実際に2個allocateし、
2-member ordinary packetをcalleeでwhole unpack、片方をwhole unpack/reconstructし、
もう片方をそのまま戻すsource。
`tests/fixtures/p294/rebuild_first_2.nl`は固定compilerで58行の最初のretireにINTERNAL。
同じ構成の1-H controlは固定でも受理。両memberをreconstructする2-H例もINTERNAL。
3-link Hを1/2/3/4サイトに減らす構成は既存FIVE-ROOT-SOURCE-PROFILEの未対応であり、
意味論反例として数えない。1-link比較は元の5-H sourceとは別の縮小profileである。

production変更は`call()`の1箇所だけ。OWNER_AGGREGATES opt-inで、認識済みwhole callの
**既にENDEDであるowner aggregate shell**の再帰的な終了を省く。
live/LOOSEな引数の後処理、reborrow終了、Release前提、nonCopy consume、validatorは変更しない。
Allocation/Domain/root/Matchedを生成・書き換えない。新しいsource形式・意味規則・flagは追加しない。
default-OFFでは追加分はcompileされない。

## 修正後の分類とHOLD

| 実測source | 結果 |
| --- | --- |
| 元R291 Allocation修復 / Domain修復 (5-H) | exit3、`error(precision)[CAPTURED-CLOSURE-PRECISION]`、125:1–198:2。登録status3、rollback、entry artifactなし |
| 5-H matched whole identity、callerが先に混ぜたpacketをidentity return後に修復 | source受理・actual ownedあり、CLI exit4/backend unsupported |
| 1-H rebuild / 2-H rebuild / 2-H mixed A修復 / mixed D修復 / alpha rename | source受理・actual ownedあり、CLI exit4/backend unsupported |
| 5-H/2-H wrong Allocationでのretire | exit3、semantic `P193-CALL-BACKING` |
| 5-H/2-H wrong Domainでのretire | exit3、semantic `P193-CALL-DOMAIN` |
| duplicate receiver / consumed donor再使用 | exit3、semantic `P3-USE-AFTER-CONSUME` |
| active D loan中のpacket構築move | exit3、semantic `P3-REF-CONFLICT` |

元2件の残る最小ブロッカーは5-H captured closureの**whole-call certificateが
ordinary identity returnに限定され、calleeが新しいpacket ID138を組み直す戻り値を
entry ID133から証明できないこと**。
`src/captured_closure.c:nl_whole_call_validate()`の`identity_components()`は
result IDがentry worldに存在しないためfalseになる。
後処理修正だけでこの証明を緩めることはしない。
callee reassemblyに対する有限な元member保存証明の選定・実装は別の許可が必要。
この5-H precision拒否を意味論的な不正、parser拒否、成功と数えない。

2-Hはこの5-H captured-certificate経路を持たない既存profileであり、
そこでのsource受理を5-H証明の代替としない。
公開observerで元A/Dの一意性、実際のentry/return/receiveでの同一ID、
donor/formal CONSUMED・receiver AVAILABLE、actual deallocateのR一致を確認した。
2-H mixed修復: whole input29→result34、A IDs1/13、R1/2、D value IDs6/18、D1/2は同一。
matched freesは成功armでR1/R2、第2 allocation failure armでR1。
5-H identity修復: input/result133、A IDs1/15/29/43/57とD IDs6/20/34/48/62を保持し、
5件の公開captured certificateを再検証した。
これらはchecked semantic pathsであり、native malloc/free実行ではない。

## 検証と再実行

- `mixed_repair.integration`: 16 source variants = 7 source positives、7 semantic negatives、
  2 precision HOLD。各CLIは2回実行、C出力なし、拒否は公開snapshot rollbackを確認。
- 実際のparse→register→main()→owned fragmentを使う公開API observer。
  各9対象で7地点のmalloc/calloc/realloc失敗、計63 injected OOM。
  失敗時snapshot保持、同じcontextへの無故障再試行を確認。
  受理後のworldでoriginal A/Dを比較し、同じR/Dのlive値重複を禁止。
- local GCC14 Debug: all-four-ON **265/265**、default-OFF **249/249**。
- exact-head GCC / GCC Release / Clang / ASan+LSan / UBSanは既存PR CIの5-job×5-profileで実施。
  最終remote結果はPRとIssue294の報告を参照。

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DNEWLANG_EXPERIMENTAL_ORIGINAL_GRANT=ON \
  -DNEWLANG_EXPERIMENTAL_NESTED_CALLER=ON \
  -DNEWLANG_EXPERIMENTAL_TRANSITIVE_TERMINAL=ON \
  -DNEWLANG_EXPERIMENTAL_OWNER_AGGREGATES=ON
cmake --build build --parallel 2
ctest --test-dir build -R '^mixed_repair.integration$' --output-on-failure
build/newlangc tests/fixtures/p294/mixed_alloc_repaired_5.nl
build/newlangc tests/fixtures/p294/mixed_domain_repaired_5.nl
```

4+2 detach/adopt、full cJSON、native backend、言語機能、Draft採用、self-merge、他Track起動は対象外。
Issue294に報告後STOP。
