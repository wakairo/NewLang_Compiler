# P #274 original-grant source/checker 実験

Track: P

**Disposition: EMPIRICAL CANDIDATE（単一ordinary recordとprimitive terminalの範囲）。**
実際のsourceから元のBのptr/typed root/region/Allocation/domainを保存し、
whole分解 → 既存のローカルdomain借用 → B.prev書き込み → 借用終了 → whole再構築
を検査できた。元のCのAllocationを混ぜてもrecord構築・B.prev書き込みは可能だが、
Bのfull原Storageを解放する操作では意味論的に拒否された。
**五-root named call/TreeTwo結果の証拠は未実装。Draft17.31採用、native B、H1/North Star PASSではない。**

## Authorityと実験の隔離

- 固定Compiler main/base: `b75baea96a644e68634baee383299b66981b3c62`。
  `CURRENT_SPEC.md`はDraft17.30。固定mainの別detached checkoutも実際にbuildした。
- 固定候補: Draft PR #273 head `e6b4e98d4f19ed03bdba9a63ff0e86d6b40dc0a2`、
  proposed §3.2c、DI-015 PROPOSED。
- [M #272報告](https://github.com/wakairo/NewLang_Compiler/issues/272#issuecomment-6093146941)、
  [Coordination独立Gate E review](https://github.com/wakairo/NewLang_Compiler/pull/273#issuecomment-6093180548)、
  [Issue #274](https://github.com/wakairo/NewLang_Compiler/issues/274)を確認した。
- [F #47 wrongAllocation/source-call gap](https://github.com/wakairo/NewLang_FormalProof/issues/47#issuecomment-6092828952)、
  [F #43の三つのadapter不足](https://github.com/wakairo/NewLang_FormalProof/issues/43#issuecomment-6084539116)、
  M #270/#271と独立裁定も確認した。FormalProof/mainの指定authorityは
  `08c8b8da4b9dbe5e125e4be0643bffb28294bfad`。Leanは変更・再実行していない。
- 実験branch: `p274-original-grant-experiment`、固定mainを親とする。
  `NEWLANG_EXPERIMENTAL_ORIGINAL_GRANT=ON`という**明示的な実験build**だけで新recordをparseする。
  defaultはOFF。PRはEXPERIMENTAL / DO NOT MERGE、Draft、未mergeとして残す。
- canonical Draft、CURRENT_SPEC、ledger、user-facing cJSON workloadは変更しない。
  full cJSON B STOP、H1/North Star UNDECIDED、H2 actor stress別扱いを維持する。

## 最小の実装と実際のsource導出

新しいruntime owner class、Matched bit、user annotation、host seedは導入しない。
parserに一つのbounded record categoryをopt-in追加し、既存のaggregate registryと
whole constructor/destructureへ渡す。許可する宣言のfield type順は
`ptr<completed H>, Allocation, LifetimeDomain`だけ。nominal名・field名・local名はgrantではない。
新しい型のCopy/Discardableは成分から導出し、物理layout/ABIを設定しない。
constructor/destructureの既存field照合、value-use、transaction、dependency検査を再利用する。
宣言は一単位につき一つで、nested recordや任意のfield-type順はまだ許可しない。

`tests/fixtures/experimental_original_grant.nl`は、canonical PRE-detach fixtureと同じ
五つの独立したfallible H siteと六つのinitial field writeを持つ完全なsourceである。
全success時だけBの既存local A/Dをordinary recordへmoveする。各None armは元のprimitive cleanupを行う。

導出する情報は既存の実運用semantic dataflowである。

| Source操作 | 独立に得るchecker情報 |
|---|---|
| `Some(OneBacking)` / whole分解 | original `allocation_region=R_i`とfull raw `occupancy=(R_i,0,size(H))` |
| `into_slot<H>(raw)` | 同じRのunique slot。rawはConsumed |
| `lifetime_domain()` | 新しいD identity |
| `initialize(slot,H,stable_D)` | 同じR上のoriginal typed root O/incarnation I、そのgoverning D、ptrのO/I provenance |
| ordinary record構築 | ptrだけCopy、元のAllocation/D valueを一度consume。成分の相関を仮定しない |
| whole分解・local借用・whole再構築 | A/D identityとptr O/Iを保存。終了するのは旧local owner-value incarnationであり、heap Oではない |
| `destroy → erase_slot → finalize_domain → deallocate` | current ptr/DとEndRootを検査し、同じoriginal full-R rawを回復し、Allocation/R一致を検査 |

実験sourceの中心部分（declared field labelsをp/a/dとして既存OneBacking localの名前と区別）:

```newlang
struct LiveRoot { p: ptr<Node>, a: Allocation, d: LifetimeDomain, }
// 実際の五つのSome/OneBacking/slot/initialize後:
let packed = LiveRoot { p: ptr_B, a: allocation_B, d: life_B };
let repacked = {
    let LiveRoot { p, a, d } = packed;
    let old = loan_read(d) { |stable|
        let w = ref_from_ptr(write, p, stable);
        replace(w@prev, Option<ptr<Node>>::Some(ptr_A))
    };
    LiveRoot { p: p, a: a, d: d }
};
let LiveRoot { p, a, d } = repacked;
// 既存のB.next/C.prev操作と独立cleanup後:
let empty_B = loan_exclusive_read(d) { |ending| destroy(p, ending) };
let full_B = erase_slot<Node>(empty_B);
finalize_domain(d);
deallocate(a, full_B);
```

上記は説明用抜粋。実行した完全なsourceと全mutationはfixtureと
`tests/integration/original_grant_test.py`を参照する。
五つのsource-positiveはすべて**public** parser → unit registration → actual `main()` callを通った。
observer `original_grant_evidence.c`はpublic gettersとowned captured-closure validatorだけを読み、
private header、`nl_captured_closure_probe`、seed/Matched injectionを使用しない。
既存`captured_closure_test full-evidence`も別途public routeでoriginal closure、六Changes、七field facts、0..5 release worldsを再検証する。

### 観測したoriginal identityとcurrent carrier

成功arm path `main/Some^5`のartifact-owned worldsと、そのprefix mappingをvalidated closureで資格付けした。
BはR=3、root place=31、heap incarnation=34、D=3。
correct recordのAllocationはR=3、mixed recordは**original CのR=4**。
sourceの二つのrecord value IDは99/110、local record incarnationは78/86で異なる。
一方、両recordのptrは同じoriginal heap O/Iを指し、元のA/D value IDは再構築を通じて同じ。

B.prev Changeのbefore/afterでheap incarnationとgoverning Dは維持される。
その時点の全current source bindingを数えると、対象AllocationとDのcarrierは各一つで、
元のAllocation/domain bindingはConsumed。旧packedとrepackedも最終的にはConsumed。
これは今回のcomplete straight-line sourceの観測であり、任意のnested/sum/call inventory theoremではない。

## 決定的なwrongAllocation対照

1. **correct-original:** recordを`p_B,A_B,D_B`で作り、分解・借用・再構築後にBを完全解放。
   public unit/check OK、owned closure validator OK。
2. **wrong-original-allocation:** recordのinitializerを一箇所だけ
   `a: allocation_B`から、別の成功site由来の**元のCの`allocation_C`**へ変更。
   同じB.prev writeは先にチェックされる。後続の`deallocate(a, full_B)`で
   `AOrigin(a)=R_C=4 != R_B=3=FullRaw(full_B).R`として拒否。
3. **mixed-harmless-repaired:** 2と同じmixed record・同じB.prev write・同じwhole再構築を使用。
   terminalだけBには別localの元のA_Bを、Cにはrecordから取り出したA_Cを使用する。
   public checkとowned validatorが成功し、B.prev checkpointでも`ptr_region=3,allocation_region=4,domain=3`を観測。
   **任意のmixed tripleを作ったこと自体は拒否していない。**

実際のwrongAllocation diagnostic:

```text
wrong-original-allocation.nl:204:56-204:57: error(semantic)[P4-ALLOCATION-MISMATCH]: Allocation and Storage have different BackingRegion identities
```

source byte range `[12978,12979)`はその`deallocate`のAllocation operand **`a`**。
registrationは`NL_CHECK_SEMANTIC_ERROR`、caller context snapshotは変更されず、owned artifactは発行されない。
ここで拒否するconsumerは**primitive deallocation**であり、未実装の`TreeTwo` admissionや
`finish_root`のsymbolic call requirementを実装済みと主張しない。

固定mainは既に、recordを使わないdirect wrong-original-Allocationのprimitive terminalを
`P4-ALLOCATION-MISMATCH`で拒否した。ただしsource spanがbody先頭を指していた。
この実験branchでは`allocated_raw`からraw checkerへoperand spanを渡し、上記の具体的operand診断を得るようにした。
分類は既存rejectionの**DIAGNOSTIC-QUALITY**修正であり、新しいownership法則ではない。

## CLI / artifact / 未対応の分離

全22 inputのsource SHA-256、CLI exit、diagnostic、source-owned evidenceは
`P274_SOURCE_OBSERVATIONS.json`に保存した。各CLIを二回実行して一致を検査し、stdout/file publicationも検査した。

| Input群 | CLI / checker結果 | credit |
|---|---|---|
| correct、mixed repaired、after-A、alpha-renamed、constructor field order | CLI **4** `V1-BACKEND-UNSUPPORTED`、public semantics/actual main/owned validator **OK**、stdout 0 | source-checker positive、native未対応 |
| wrong A_C/full B、A_B/full C | CLI **3** `P4-ALLOCATION-MISMATCH`、semantic error | 原BackingRegion不一致の意味論拒否 |
| wrong D_C | CLI 3 `ALLOCATED-DOMAIN-MISMATCH` | governing domainの意味論拒否 |
| A/D二度consume、別bindingへwhole move後の旧binding再使用、original raw再使用 | CLI 3 `P3-USE-AFTER-CONSUME` | 元のnonCopy bindingの意味論拒否 |
| live local D loan、derived H ref loan内で再構築 | CLI 3 `P3-REF-CONFLICT` | 借用中のdomain consumeの意味論拒否 |
| EndRoot後のpから再loan | CLI 3 `P3-STALE-POINTER` | stale original rootの意味論拒否 |
| forgotten whole owner、partial pattern、unknown field | CLI 3 `P5-DISCARDABLE-REQUIRED` / `P5-AGGREGATE-FIELD-COUNT` / `P5-AGGREGATE-FIELD` | value-use/patternの意味論拒否 |
| duplicate record declaration field | CLI 3 `P274-RECORD-DECLARATION` | 宣言の意味論拒否、ownership safety creditではない |
| nested `TreeTwo{root:LiveRoot,child:LiveRoot}` | CLI 3 `AVS-DECL-PROFILE` | **parser unsupported、safety credit 0** |
| five-H unitへの`finish_root(LiveRoot)`追加 | CLI 3 `FIVE-ROOT-SOURCE-PROFILE`、check status UNSUPPORTED | **source profile unsupported、safety credit 0** |
| H宣言なしのrecord | CLI 3 `P274-RECORD-PROFILE`、check status UNSUPPORTED | **未対応、safety credit 0** |

集計: source-semantic positive/backend unsupported **5**、semantic-reject **13**、
semantic declaration error **1**、parser/profile unsupported **3**。
この数をNorth Star B1–B7のscoreへ流用しない。

after-A positiveはoriginal A,C,srcを先にEndRoot/full raw/finalize/freeし、次にoriginal B,dstをfreeする。
source-owned deallocation順を**[2,4,1,3,5]**として検査した。
旧two-Hの`headLive(A)` terminalは再使用せず、各original rootのprimitive手順を使う。
これはnew named `finish_two` call/receiver ownership resultの証拠ではない。

## LiveTailとの比較と最小ブロッカー

固定mainのcanonical five-root、two-H `live_tail_return.nl`、`live_tail_custody.nl`は
実際のCLIでexit 0、C17 stdoutを出力した。既存248件回帰も成功した。
一方、ordinary recordのcorrect/wrong/mixed sourceはすべて固定mainのparserで
`AVS-DECL-PROFILE`となる。これを安全性の拒否と数えない。
固定mainの正しいtwo-H unit内でも、mainから直接LiveTailを作る試行は
`P208-CONSTRUCTOR-CONTEXT` unsupported。

LiveTailの既存member packageとprimitive原region照合は再利用可能な小さいmechanismである。
しかし、既存source constructor/destructureはtwo-H producer originとhead-link contractに閉じている。
そのsource gateを暗黙に広げるだけでは、ordinary五-rootのgrant/known-call証拠にはならない。
今回の一つのordinary nominal categoryで最初のlocal correlationは検査できた。

**次の最小ブロッカーはbounded acyclic aggregate member/current-carrier flowとknown-call relational summaryの組である。**
Priority 2を試したところnested record grammarで停止し、nested recordを使わない
単一`finish_root(LiveRoot)`定義でもthree-link H unitのmain-only gateで停止した。
そのguardを外して有利なactualから定義の相関を仮定する変更は行わない。

未実装:

- TreeTwoの二つのoriginalを含むcomplete value、nested owner inventory、nested/sum内のglobal current-carrier/ghost排除。
- `finish_root` / two-original producer-adopterの**独立symbolic** p/O/R/A/D requirement推論、
  every actual callのdischarge、parameter/result/caller-current original identity保存。
- `TreeFour → DetachResult → TreeTwo`の四detach/two-adopt Changes、実際のcaller-returned one TreeTwo、
  post-A named finish_two、全refusal/refund/normal-exitのowner accounting。
- recordを新しいtrialを越えてcaptureするclosure、general grammar、generalized LiveTail alias/constructor。
- new recordのbackend/layout/native lowering、full native cJSON B、F rich/call source refinement、product/H1評価。

Coordinationが次を裁定するための最小単位は、上記二-original aggregateとknown-direct callの
**source-generated member provenanceとcurrent owner result**を検査する後続sliceである。
ここではそのIssue/他Trackを起動しない。広いcore法則や新runtime owner tableが必要だという証拠は得ていない。

## 履歴照合・実装境界

Process §4.2、Design Decision Procedure、Testing Strategy、Review/NewLang-aware C Guidelines、
canonical §§3.2b/14.5/16/18.1a–c/26/27と候補§3.2cを照合した。

- **KEEP:** DI-001/009/010/014のsource punctuation、同じOneBacking R、explicit local-D loan、mode-preserving H ref、0..5 original cleanup。
- **KEEP:** DI-011/012/013の独立known-call proof/LiveTail source gateとnonDiscardable Option。
  今回の実装はこれらをordinary-owner callへ一般化しない。
- **候補の小部分だけを実験:** DI-015 PROPOSEDのordinary whole-value triad、local destructure/loan/repack。
  semantic lawsは元のcanonical primitive/value-use規則を使い、adoptionはしない。
- **DEFER:** DI-002/004/005/006/007/008の一般private/generic/receiver/field-loan/source grammar、
  Owner/Drop/GC/RAII/FFI/actor、広いcontainer API。
- Ledgerは変更不要: 新しい設計の採用・置換を行わず、DI-015は別の未merge候補にPROPOSEDのままある。
  この実験reportがauthorityを変更しない。
- 旧Surface Draft1/1.1、M0、module-private receiver primaryは固定Compiler checkoutと
  `/workspace/attachments`, library-files, sharedのfilename検索では見つからなかった。
  候補DI-015とDI-011–014に記録された比較を参照した。完全な旧M9会話の監査を主張しない。

## 検証と再現

Local toolchain: GCC **14.2.0**、Clang/LLVM/clang-format **23.1.2**、CMake **3.31.6**、Python **3.12.14**。
通常のC17、Wall/Wextra/Wpedantic/Werror、ASan/UBSanはproject設定どおり。

| 構成 | 実測 |
|---|---|
| 固定main GCC Debug | 248/248 CTest PASS |
| branch default/OFF GCC Debug | 249/249 PASS、追加parserとregistryのOFF拒否を別途再確認 |
| opt-in GCC Debug | 250/250 PASS、最終observer変更後の対象3件もPASS |
| opt-in GCC Release | 250/250 PASS、最終observer対象3件PASS |
| opt-in Clang | 250/250 PASS、最終observer対象3件PASS |
| opt-in ASan + LeakSanitizer | 250/250 PASS |
| opt-in UBSan | 250/250 PASS、最終observer対象3件PASS |

ローカルASanの最初のsandbox実行はLeakSanitizerのptrace制約で失敗した。
同じcheckを許可されたsandbox外実行で再検証し、`detect_leaks=1:halt_on_error=1`のまま250件成功した。
kernel policy変更やleak検査の無効化はしていない。

parserの直接契約テストはON/OFF、reparse、shape/extra-field/truncation/unsupported nested形を検査する。
semantic registry unitは、sourceで完成したHの型だけを使い、shape/property、重複、unsupported、
failure rollback、全allocation failure injectionまで成功を検査し、root/Allocation/domainをseedしない。
source integrationはpublic full pathとimmutable evidenceに責務があるため、独立した新stateful production moduleは作っていない。
既存unit/integration/oracle/e2eを削除・弱化せず、formatter/check-formatとgit diff --checkも実行した。
新しいPython runnerは**observation/regression harness**であり、未採用Draftを保証する独立semantic oracleではない。

```sh
# 通常のproject toolchainをactivateした後:
cmake -S . -B build-p274 -DCMAKE_BUILD_TYPE=Debug \
  -DNEWLANG_EXPERIMENTAL_ORIGINAL_GRANT=ON
cmake --build build-p274 --parallel 2
ctest --test-dir build-p274 --output-on-failure
python3 tests/integration/original_grant_test.py \
  build-p274/newlangc build-p274/original_grant_evidence \
  build-p274/captured_closure_test /tmp/p274-reproduce
# 固定mainは別checkoutでbuildする。old spanの実測も含めて再生成:
python3 docs/experiments/P274_BASELINE_AUDIT.py \
  /absolute/fixed-main/build/newlangc /tmp/p274-baseline /tmp/p274-reproduce
```

Branch CIは既存五構成それぞれでdefault/OFF全テスト後、explicit opt-in全テストも実行する。
exact-head remote CIのrun/job状態はIssue #274の最終報告へ記録する。
これは実験branchだけのworkflow変更であり、canonical CI/mainに適用しない。

4–6h bounded budgetの最小sliceとして実施し、Priority 2の明確なsource/checker blockerを得たため拡張を止める。
実測開始は2026-10-10 **12:19:52 JST**。実測終了・実験HEAD/PR/CIはIssue報告に固定する。
self-merge、canonical adoption、他Track起動はせず、Issue #274は独立Coordination review用にOPENで残す。

P CJSON-B-STATE-1 PRE-ADOPTION ORIGINAL-GRANT: EMPIRICAL CANDIDATE — SOURCE CORRELATION AND WRONG-ALLOCATION REJECTION OBSERVED, DRAFT UNSELECTED
