Track: R

# R1179 / C244a FINAL — finite runtime evidence; first reliable all-green

同じ R1179、[C244](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6651737917)／[C244a](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6651940495) の有限 release を完了した。**PASS_FINITE_RUNTIME_AND_PUBLIC_OBSERVATIONS**。公開 whole-loop validator の項目は **NOT_RUN / NO_APPLICABLE_PUBLIC_API** のままで、この結果から validator 成功や ghost freshness／private authority の一般証明を導かない。

最終修復組は **FULL 5 jobs 各 720/720、NO-LLVM PARTIAL 1 job 719/719**。各 job の独立 R CTest は 7 positive/control、8 negative classification、84 actual guest executions を完了した。6 jobs 合計 **504 native executions**（未改変252＋passive252、うち guest ASan84／UBSan84）。全 native rc0・stderr0。有限7ケースを複数host laneで再観測した件数であり、504種類の独立sourceではない。

全6 job の原decoded log・lossless exportを実bytesで復元し、archive SHA256・730 files/job の全index/hash・実argv/receipt・traceを照合した。**2026-11-05 08:30:34 UTC（17:30:34 JST） 時点で first reliable all-green を確認し、追加push/CI停止。FAST3/6、通常12/12、manual rerun/dispatch0。** 恒久archiveはCの別release対象であり、本返却で新archive ref/commit/pushを作らない。

[実START](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6651762904)／[公開validator境界の原STOP](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6651901956)／[C244a実RESUMEと露出記録](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6651960335)を保全。

## 1. 固定対象・Git identity・freeze

- 固定compiler HEAD **bb8b33cefc030ed99941c9c93fef9c7ef89b0ac0** / tree **d8fd0b0a95184d3449d7e1525ba56574429dbf8f**。
- compiler parents **212bd3a8789a51b1afaf11d6088ed346f983c354 + 217dda0c0c786f64ef4376aec1cb7e72b2a4ecc2**。
- 最終R HEAD **9b58f0f4bf7d09619c904adc61b4e40db6294bdb**、sole parent **e9d5941e1ef770d73ad1866fd87909974ca39555**、tree **11f66a5e660d4d59cf3bb599aecad24fb895fa2a**。
- 実PR checkout **2193444841acdac9308cf697e45b0985900f881c**、parents **bb8b33cefc030ed99941c9c93fef9c7ef89b0ac0 + 9b58f0f4bf7d09619c904adc61b4e40db6294bdb**、treeは最終Rと完全同一 **11f66a5e660d4d59cf3bb599aecad24fb895fa2a**。
- branch **r-fast/r1179-phase-selected-current** 1本、[PR1183](https://github.com/wakairo/NewLang_Compiler/pull/1183) 1件、**DRAFT / NEVERMERGE**。base **p-c243-phase-selected-current = bb8b33cefc030ed99941c9c93fef9c7ef89b0ac0**。READY0 / merge0。
- authority main **11d4d078c2a9deb68b77c428bba079431dee72fc**、CURRENT_SPEC→Draft17.34。実行対象はこのmainへ差し替えていない。
- 全通常jobの実fixed-context receipt：commit bb8 / tree d8、6086 leaf blobs / 345 trees、real index SHA256 **519d2e34304ca13da364ef49cdc593c32ce69e3546a400c2562480b53811c2e3**、clean fixed worktree。historical scriptのcwdとrepo argvを同じ固定contextへ対応させた。
- 6086 inherited pathsの6085はbyte/mode/blob不変。変更した既存pathはroot CMakeLists.txtのR registration include 1行だけ。R32追加files以外のProduction／既存tests/assertions/timeout／3 workflows／toolchain変更0。

原freezeは別の結果recordで追記する方式とし、bytesを書き換えていない。

| 凍結物 | bytes | SHA256 | 原記録 |
|---|---:|---|---|
| V1 bundle | 49260 | b1a671dad926d2ad3e57eb33f6c7fb80bb12eab3a7d18b7faf84280c862aa893 | [V1原freeze](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6651324251) |
| V2 bundle | 46854 | 2dff96cd7271a2b5637c46e7a45ab798fbbf84249b447fdf82657b484a69d680 | [V2原freeze](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6651810301)／[readback](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6651851886) |
| V2 oracle | 27762 | 4fe82f7bec6315d85a1c4eb4b41634d0135e2f363c1e38640ae1fa7f9c511cff | V2の同一17payload内 |

V1は全件**未実測**のまま保全。V2はC244の事前scope較正（unit main / one usize phase carry / Continue(next_phase) / Break phase）後、機材参照・compiler実行前に凍結した同一15source＋oracle/calibrationであり、実行後のsource/oracle変更0。

[初版機材freeze](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6652375474) → [R-only format freeze](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6652454488) → [最終機材freeze/readback対象](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6653020239) → [最終ref update](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6653021444)。各refの前に完全commit＋機材freeze＋server readbackを実施した。

最終機材 SHA256：
- driver.py **86cbf6cdf56dbd1eb986ea69d4c6d319e8dbbc92d70fcc5114ef47fc00a3d401**
- observer.py **67b2a4b431cd76133fc8fd92635f06e280dd12223a4ba3b327d0f368054b23ec**
- probe.c **805fb0f93ea2c533ede8c5a8e7d5f76173d32e76e51e5db1587fbf3580aacf37**
- equipment-manifest.json **d82ee62660816afcf238202b27d4c83caf1a006b685a48eb87103246b731de54**

## 2. 全run/job/attempt台帳

branch全体をhead/event横断で列挙し、**7 runs / 15 actual jobs**を照合。全run_attempt=1。HEADだけの一覧ではない。

| 段階 | event / head | run | actual job | conclusion / 実到達 |
|---|---|---:|---:|---|
| FAST1 | push / 333ace592984e4de55f30b4e80a7101b0b7e7d72 | [41987779956](https://github.com/wakairo/NewLang_Compiler/actions/runs/41987779956) | 127112527398 | failure：pinned formatのみ、build/inventory未到達 |
| FAST2 | push / e9d5941e1ef770d73ad1866fd87909974ca39555 | [41988343142](https://github.com/wakairo/NewLang_Compiler/actions/runs/41988343142) | 127114331484 | success：format/build、717 CTests登録、tests未実行 |
| 初回FULL GCC | pull_request / e9d5941 | [41988634241](https://github.com/wakairo/NewLang_Compiler/actions/runs/41988634241) | 127115286119 | failure：719/720、R recognizerで停止、native0 |
| 初回FULL Clang | 同上 | 41988634241 | 127115286373 | 同上 |
| 初回FULL Release | 同上 | 41988634241 | 127115286486 | 同上 |
| 初回FULL ASan | 同上 | 41988634241 | 127115286512 | 同上 |
| 初回FULL UBSan | 同上 | 41988634241 | 127115286361 | 同上 |
| 初回PARTIAL | pull_request / e9d5941 | [41988634273](https://github.com/wakairo/NewLang_Compiler/actions/runs/41988634273) | 127115285777 | failure：718/719、R recognizerで停止、native0 |
| FAST3 | push / 9b58f0f | [41992265581](https://github.com/wakairo/NewLang_Compiler/actions/runs/41992265581) | 127127028279 | success：format/build、717 CTests登録、tests未実行 |
| 修復FULL GCC | pull_request / 9b58f0f | [41992273219](https://github.com/wakairo/NewLang_Compiler/actions/runs/41992273219) | 127127053063 | success：720/720、R 84 native |
| 修復FULL Clang | 同上 | 41992273219 | 127127053249 | success：720/720、R 84 native |
| 修復FULL Release | 同上 | 41992273219 | 127127053303 | success：720/720、R 84 native |
| 修復FULL ASan | 同上 | 41992273219 | 127127053418 | success：720/720、R 84 native |
| 修復FULL UBSan | 同上 | 41992273219 | 127127053423 | success：720/720、R 84 native |
| 修復PARTIAL | pull_request / 9b58f0f | [41992273473](https://github.com/wakairo/NewLang_Compiler/actions/runs/41992273473) | 127127054015 | success：719/719、R 84 native |

FAST2/3のrequired registration 12/12は**未実行 inventory**。通常成功件数に加えない。FULL5の720件は名前集合も同一、PARTIAL719との差は **llvm.c_api** 1件のみ。R CTestは各jobちょうど1件で、既存test除外・元assertion緩和なし。

## 3. FIRST/REDと修復の境界

[初回通常6の原結果](https://github.com/wakairo/NewLang_Compiler/issues/1179#issuecomment-6652929154)を保持。初回6 jobでは各7 CHECK／7公開owned read-only／7 C17 emission／8 negative classificationまで到達し、P01 AST対応の **MECHANISM_RECOGNIZER: one actual C loop required** で停止。guest native0であり、初回をcompiler defectやnative PASSへ読み替えていない。

初回format REDの原log／原suggestion／実ZIP bytesを保存。ZIP 1784bytes / SHA256 **ec1d8dbcfe028639d03b22af6075ebc4b712737bce928275d022a950f54d0fac** は実取得bytesでGitHub reported digestと一致。無関係な旧P blockは表示・適用せず、R probeの3hunk／4箇所のwhitespace折返しだけを修復した。

通常の修復はR機材だけ：actual mainから到達するfunction内loop、anonymous typedefのRecordDecl対応、Continue IF後のfallthrough Break、実current-phase binding、same Break receiving carrierのloop後再読取り、macro expansion終端のbalanced-parenthesis認識を実C/ASTへ対応させた。driverは実phase／incoming binding・Copy列／finishedの照合を追加し、元の失敗判定を弱めていない。

Cの機材だけの静的feedback（top-level qualifier正規化、store RHSのpassive純粋形制限、FIRST macro終端など）は元機材hashを残して採用。V2 source/oracle不変。Cが認可した保存済みFIRST C＋ASTだけのlocal pure-text replayは6回。新AST生成／compiler／build／SDK／guest／nativeのlocal実行0。この静的再生自体をCI/native PASSと数えていない。

修復後の7元Cは全6jobで初回とbyte同一。passiveは元bytesを同順序に保持し、stdio include＋実carrierを読むprintfだけを挿入。P01–P05は各24printf／23挿入位置、P06は25／24、L01は22／21。元emitterのobserver hooksは有効化していない。observerの入力は元C＋そのCのActions-generated Clang ASTだけであり、source/oracleは渡していない。

## 4. 通常経路・公開観測・nativeで実測した範囲

全7 positive/controlで通常source CLI CHECK → source-issued public parser/register/main-call owned observation → 既存通常C17 emissionの順に成功。その後、各caseについて下記12実行を実施した。

| guest mode | 未改変 | passive | 全6jobの合計 |
|---|---:|---:|---:|
| GCC -O0 | 1/case/job | 1/case/job | 84 |
| GCC -O2 | 1/case/job | 1/case/job | 84 |
| Clang -O0 | 1/case/job | 1/case/job | 84 |
| Clang -O2 | 1/case/job | 1/case/job | 84 |
| guest ASan | 1/case/job | 1/case/job | 84 |
| guest UBSan | 1/case/job | 1/case/job | 84 |

実version receiptは全jobのguest GCC **13.3.0**、FULL5のguest Clang **23.1.3**、PARTIALの既存Ubuntu Clang **18.1.3**。NO-LLVM PARTIALはLLVM C API/SDKを導入せず、既存runnerのC toolを使った。

共通strict flagsは **-std=c17 -Wall -Wextra -Wpedantic -Werror**。guest sanitizerは実compile argvに **-O1 -g -fsanitize=address|undefined -fno-omit-frame-pointer -fno-sanitize-recover=all** を記録。実execute receiptには ASAN_OPTIONS=detect_leaks=1:halt_on_error=1、UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1。host laneのASan/UBSan表示からguest coverageを推定していない。

各nativeのbinary byte count/SHA、compile receipt、execute argv/cwd/rc/stdout/stderr bytes/SHAはlossless evidenceに保存。未改変native stdout0、passiveは全traceを解析。binary自体はexport boundのためarchive外で、binary hashはActions内の実bytes計算receiptであり、localにbinaryを再取得／再hash／再実行した主張ではない。

公開read-onlyでは各caseに1 LOOP/header_inductiveと1観測可能REPLACE field receipt、正しいselected declaration index、world-qualified body/node/result/field/historyを記録。P01–P05は83records/6worlds、P06は86/6、L01は66/4。全usize_bits=64。worldを跨いでIDを同一authorityとして扱わず、history/current context/physical C addressを混同しない。whole-loop validationおよびその改変試験は **NOT_RUN / NO_APPLICABLE_PUBLIC_API**。新private accessor/authority/validator作成0、既存false→true変更0。

### 凍結oracleと一致した実値

下表のheader列は最後のBreak headerも含む。実Continue outgoing phaseは各列の次headerと一致した。incomingは実IF結果→binding/Copy（P06では追加Copy）からstoreへ渡る値、oldは実old load→loan result→old/old_copyの全列で照合した。L01は直接literal対照である。

| ID | 実header phase | incoming = after-current | old = old_copy | sibling | Break = finished | post-Break original (f0,f1) |
|---|---|---|---|---:|---:|---|
| P01 | 5 | [] | [] | 91 | 5 | (17,91) |
| P02 | 2,3 | 47 | 19 | 83 | 3 | (83,47) |
| P03 | 1,3,5,7,9 | 34,34,78,78 | 12,34,34,78 | 93 | 9 | (78,93) |
| P04 | 0,3,6,9 | 52,52,86 | 22,52,52 | 109 | 9 | (109,86) |
| P05 | 0,1,2,3 | 61,97,97 | 61,61,97 | 131 | 3 | (97,131) |
| P06 | 2,4,6,8,10 | 27,149,27,27 | 39,27,149,27 | 157 | 10 | (157,27) |
| L01 | 1,3,5,7,9 | 78,78,78,78 | 12,78,78,78 | 93 | 9 | (78,93) |

P01はwriter0、P02は1edge、残りは複数edge。P01/P03/P05/L01がfield0、P02/P04/P06がfield1。P04の初期化順と宣言順を区別し、P05のsame-bitsとP06のEQ true/false両分岐も元予測に一致した。

各passive実行でB0、全headers、before/after store、Continue、Break、loop exit、post-Break actual field loadsを観測し、**同じ実original root/child addresses**を保持、child0≠child1、writer destination=選択child、未選択sibling不変を照合した。rootとoffset0 childの数値アドレス一致は許容し、実行間の固定数値アドレス／ABI offset／old Copy local addressは要求していない。same carrierのfinished=[]は架空Copyを作らず、actual receiving variableをBreakとloop後に再観測した。

これらは有限caseのphysical/value/control観測である。same-bits一致やroot/childアドレス一致をghost incarnation freshness、loan/source-issued private authority一般証明へ格上げしない。

## 5. 負例の実拒否層と未到達

全jobで同じ分類。下記8本すべてCHECK rc3、通常emission rc3、C stdout0、negative native0。

| ID | 凍結分類・意図 | 実diagnostic | 判定の限界 |
|---|---|---|---|
| N01 | 非bool IF condition | P13-IF-CONDITION | 条件型で拒否 |
| N02 | normal arms u8/usize不一致 | P13-IF-RESULT | IF joinで拒否 |
| N03 | usize incoming→u8 field | P3-TYPE-MISMATCH | replace operand型で拒否 |
| N04 | writer loan escape | P8-EXIT-DEPENDENCY | scope dependencyで拒否 |
| N05 | 64bit overflow-intent | USIZE-ADDITION-PRECISION | 先行precision拒否。**目的のoverflow guard直接検証は未到達** |
| N06 | 未定義field | FIELD-UNKNOWN-FIELD | field resolutionで拒否 |
| N07 | canonical legal conditional／scope外two-writer | USIZE-ADDITION-PRECISION | EARLIER_CHECK_REJECTION_SCOPE_OUTSIDE。言語不正とはしない |
| N08 | canonical legal conditional／scope外terminating arm | USIZE-ADDITION-PRECISION | EARLIER_CHECK_REJECTION_SCOPE_OUTSIDE。言語不正とはしない |

N05は実公開usize_bits=64を記録したが、precision拒否をoverflow guard PASSへ読み替えない。N07/N08は合法scope外の分類を保ち、CHECKやC17の未到達機能を成功扱いしない。不正CHECK受理・UB・owner/loanの実soundness findingはこの有限実行で観測されなかった。

## 6. 原証拠の実bytes照合

全修復jobは **1 lossless tar.gz / 730 evidence files / 225 command receipts**。各archiveの宣言bytes/SHAと実base64復元bytes/SHA、さらに全memberのevidence-index bytes/SHAを照合。原C／AST／passive C／diff／role-map／公開owned JSONL／全raw stdout/stderr/receipt／binary hash receipts／fixed-context／reportを含む。

| lane / actual job | 原decoded log bytes | log SHA256 | lossless archive bytes | archive SHA256 |
|---|---:|---|---:|---|
| GCC / 127127053063 | 858010 | 1c3cff0d1e6d8d6492c090e48eab40ad28d2ef33a22e6137d571321c02a9c454 | 463534 | 79a27ef5f4c7fa3a0ff2204490bbd5bdba3449659b4e546396cdade81455556e |
| Clang / 127127053249 | 860337 | 5ebabbe8ccea879d8259a8eeb2a35d6ff6fc5c757cc1498089370c918690230d | 464089 | f27cd802ae932ce9fa3d5c1f95147d92eaf4007cf834af184f27ea307a4be81c |
| GCC Release / 127127053303 | 858173 | ed50377f15320a0b3e9fc294b3324eb63024e620992d07e8d9f755ef8be1a992 | 464011 | 5d153cb327b66065a9ba9be77be212f43540b08a251db21af08a00eed9e66c95 |
| ASan / 127127053418 | 858440 | 34b8ba1022b0ed0ac62dc61d46c9ceee292da3aed2680d2676e9efc61b31259e | 464097 | 2c4244116b1f0c4d8f8021b17eb4820c74bac05fab0c5fadc39ac6b7c70ec5e6 |
| UBSan / 127127053423 | 858719 | 86b7c06a39e67ad6c1eee5a8e187c263705915ef4dcb95a319564237c1a5b5a3 | 463864 | f7eaa187086d925315cfdf8bbfcb540e396d63f4ff376da8a18a55447c111b32 |
| PARTIAL / 127127054015 | 905845 | ddb88d113d4e60af285a91d06e60e88a62ff216b15ae2b4c43224baac30d1b52 | 466220 | a3f436619f4a18ff308b9447b1298786c9a02f2454c0acf7f5e98830ff5d708f |

復元後の集約監査JSONは 22733bytes / SHA256 **f5ff800f77705412b06b3e084ae1838553583fea446fdfa16770d252caaaf17f**。これは保存済みActions bytesのpost-run集約であり、新たなcompiler/native実行結果ではない。

FAST3 decoded logは **87161bytes / SHA256 15e4bb5b8110806339a0b8d461290bc4aaec3513a9270d741869e9580db31b93**。FIRST format/通常6の原log、元archive、旧machine hashes、旧commit ancestry、認識失敗と未到達native0は削除せず保存している。原decoded logのSHAとGitHub artifact reported digestは混同せず、実bytes取得したものにだけ実検証済みを付している。

C引継ぎ用の作業証拠は `/workspace/scratch/809a1b02c5d4/r1179_c244_evidence/`（normal1/FIRST、normal2/全修復、fast1–3、text_replay、全ledger・derived audit）。凍結原文はIssueの原freezeにも復元可能bytesで保存済み。恒久archiveの実行・公開・new refは本release内で行っていない。

## 7. 独立性・残る境界・返却

独立性表示：**pre-execution frozen independent cohort / scope-calibrated V2; post-freeze incidental P CI-summary exposure**。

V2 source/oracle凍結後に許可されたT1166 fixed helperと、旧Rのsnapshot/register/exportだけを配管参照。新source/observer/probe/driverはこのR用に独立作成。canonical/READMEの歴史annotationと、V2 freeze後の偶発P CI metadata/集計・C freeze照合の露出は元記録を維持し、入力値・source・oracle・機材期待値へ転用していない。P/F/旧R由来のsource/fixture/oracleを入力・期待値の材料にしていない。F内容の参照0。例外の露出は上記post-freeze CI-summaryに限定して記録する。Cの新R機材だけの静的feedbackは記録済みで、全期間無限定fresh-BLINDを主張しない。

GitHub専用branch＋Actions以外でcompiler/native/build/SDK検証0。P同等環境構築0。localはpure text/hash/Git metadata/既存log decode、および明示認可された既存C/AST pure-text再生だけ。実行releaseを新scope、source変更、Production実装、新P、manual CIへ拡張していない。

**実行は完了・停止。PR1183 DRAFT/NEVERMERGEを維持してCへ返却する。** 公開whole-loop validator、N05の目的overflow guard、一般化されたprivate authority/freshnessは本結果の成功範囲外。監査局所性 **UNMEASURED**、**GO624 PAUSED / H1 HOLD / P1036–PR1050 STOP** を維持する。
