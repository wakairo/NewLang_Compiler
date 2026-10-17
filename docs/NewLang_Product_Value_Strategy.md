# NewLang Product / Value Strategy

> Status: Track V product/value strategy
>
> この文書はNewLangのproduct strategy、targeting、value validationの基本原則を定める。
> language semanticsのnormative authorityは `docs/reference/CURRENT_SPEC.md` が指すDraftであり、本書はそれを上書きしない。
> 変更履歴はGit commit historyを正とし、文書内に別のVersion historyを持たない。

## 1. Product thesis

NewLangはC後継のsystems programming languageを目指す。

> **Preserve the architecture, replace the bookkeeping.**

Cで合理的に採用されている低水準architecture / topology / explicit mechanismを維持しつつ、lifetime、ownership、current-state、dependency、provenance等のsemantic bookkeepingのうち、compilerが静的に検査できる部分をcompilerへ移す。

> **Policy-neutral, mechanism-strict.**

application / data-structure policyを過度にlanguage coreへ取り込まず、低水準mechanismの意味を厳密にする。

**多言語主義と存在理由（mission）：** NewLangはRustの後継でも、すべてをNewLangに統一する構想でもない。GC言語・Rust・Cは、同じ計算を実現できても設計・API・性能・失敗処理・配備・開発運用上の合理的な選択理由が異なり、**複数が同じworkloadに対して十分合理的であることもある**。NewLangは、Cで合理的に選ばれる低水準architecture／pointer topology／allocation・lifetime／外部host契約／toolchain・C-source distribution等の自由を維持しつつ、人間のUB防止のsemantic bookkeepingをcompilerへ移すC後継候補である。**Rustも実用的であること自体はCの合理性を否定せず、Rustを排除してCの排他的優位を証明することは目的ではない。** CのUB・任意の不正アクセスを保存する自由は対象外。

**反証可能性：** 重要なworkloadについて、Cを選ぶ**具体的で継続的に重要な積極的理由**が検証できなければ、その領域の優先度を下げる。Rustが実用上十分であるという事実だけではC合理性の反証にならず、Rustを含む代替がCの実際の利点を解消した場合はターゲット評価を更新する。Cの現行採用、移植惰性、実装可能性、個人的な好みだけでは積極的合理性を認定しない。NewLangがその領域でwell-factored Cに対するUB防止監査・再監査の依存閉包を実用上縮小できない場合も、製品価値仮説を支持しない。対象発見の現時点の非規範的文献証拠は[2026-10-17 C/Rust target discovery](research/C_Rust_Target_Discovery_2026-10-17.md)に保存し、歴史的調査結果を遡及的にProduct GOへ昇格させない。

## 1.1 UB-prevention audit locality (value hypothesis)

**製品価値仮説**は、C-classのoriginal pointer topology / allocation・lifetime policy /
physical architectureを維持しながら、**未定義動作（UB）を防ぐために人間が
監査・再監査しなければならない意味論的範囲を小さくすること**である。
compilerが現実的に検査できるlifetime、currentness、provenance、access、
responsibility等の条件はchecked側で拒否・保証し、証明不能でも実際に真で
ある必要がある事前条件だけを明示的・局所的な `unchecked` に残す。
`unchecked` の前提違反はUBであり、明示した人間／library authorが証明責任を持つ。

**Audit locus（監査の実際の対象）**は `unchecked` の行数ではない。
各privileged operationと**実際のauthority issuer**、元の
Storage / BackingRegion / Allocation / LifetimeDomain、
前提となる封じ込め済み不変条件、それに影響するlegal mutationと
normal / error / teardown / reentrant callback / FFI経路、
および変更時にそれらの成立性を再検討すべき**影響依存関係**を含む。
raw addressやmetadata（例：Ringの `head/len`）は責任・権限を無から
生成できない。例えば `unchecked buf[i]` では、元の `buf` の領域・
寿命・アクセス権限と `i` の範囲だけでなく、それらを生成・更新する
writer、呼出し、状態遷移まで監査対象が広がり得る。これは
`unchecked` の行以外を確認する必要があるというだけで、局所化の失敗ではない。
**実際の前提からの依存関係を特定し、その閉包だけを監査・再監査すれば
よく、無関係なsafeコードの再監査を避けられる**なら、Cより監査範囲を
絞れた価値がある。逆に公開ordinary-safe APIの合法な操作が
前提を破壊できるなら、安全な抽象化は成立していない。実装内部の
safe helperやwriterを変更するときには、その変更が影響する
不変条件の依存閉包を再監査し、compilerが保護できる部分は委ねる。
「uncheckedの行だけ見ればよい」は、この依存閉包の確認を
省略してよいという意味ではない。

**ordinary-safe library/APIの成立条件**は、APIから合法的に到達できる
**すべてのsafe-reachable state**とその状態遷移に対し、内部uncheckedの
前提が維持されることである。通常処理だけでなく、失敗時の責任の返却／
rollback／commit、途中状態での公開、破棄、借用中の変更、callbackによる
再入、FFI境界を含む。これはstaticに強制するか、健全なruntime guardと
アクセス制御を伴う内部不変条件として成立させる必要がある。
FULL / EMPTY等の予期される操作結果は `Option` / `Result` 等へtotalize／
明示的に返し、UBの「前提」に押し込まない。representation hidingや
safe mutationの制限を実際に強制できないなら、監査の局所化はまだ成立しない。

**責務の分離**：library authorは、原権限を保ったclaim transfer、
その不変条件とすべての合法な変更・失敗経路、内部uncheckedの正当性を負う。
ordinary callerは公開された型付きsafe APIの使用とapplication policyを
負うが、隠れたpointer hopごとのlifetime／owner証明を要求されない。
明示的な `unchecked`／外部契約をcaller自身が選ぶ場合は、その境界の
人間による証明責任が残る。

**比較の順序は二段階である。** 安全性評価の厳密さは変えず、対象を選ぶ比較とC後継の製品価値比較を混同しない。

1. **ターゲット適格性（Cを選ぶ積極的合理性）**：実際の利用者にとって、Cでのarchitecture／physical identity／allocation・lifetime・failure policy、操作の計算量／storage、ABI・配備、既存システム統合、開発・変更・保守の総負担などに、**具体的で重要なC選択の理由**があるかを確認する。stable Rust＋既存library、局所`unsafe`＋soundなsafe facade、`no_std`、FFI、GC言語やその他の現実的代替を公平に評価するが、**Rustも十分に実用的という理由だけで、証拠のあるCの合理性を棄却しない**。同じ用途でCとRustの両方が合理的に選ばれることも許容する。その用途でCを選ぶ利点が実際に重要・継続的か、単なる慣性・一時的な移植費用かを区別する。`unsafe`の有無だけで判定せず、機能的実装可能性を唯一の競争条件にしない。

2. **C後継としての製品価値（Cに対し何を改善するか）**：上記のC合理性が認められ、NewLangにも同じ実用要求を維持する見込みがあるworkloadで、**十分にAPI化・必要に応じstatic/runtime guardを備えた現実的なC**とNewLangを比較する。両者の本質的なpointer topology、allocation／lifetime／失敗処理／配備条件と公開APIを揃え、*library author + ordinary-safe caller*が人間の規律で維持するUB防止監査・再監査の依存閉包、合法writer、error／teardown／再入／FFI、無関係なsafe変更の影響を比較する。NewLangがCの自由を損なう、または人間の責務を実用上改善しないなら**Cから移行する製品理由は未実証**。

**第1関門はRustの実用的不十分さを立証する試験ではない。** 同じ操作がRustでcompile/native PASSした事実は**実現可能性**を示すが、その結果も、自然な設計・API・開発・変更・保守まで含めてRustが実用的という結果も、**Cを積極的に選ぶ実際の理由が残るか**という別の問いを自動的に決着させない。特にCのデータ構造／アルゴリズムについて、必要な物理identity・外部owner・membership・失敗／teardown契約を維持したときのRustのadapterやhandle、公開APIの制約、内部の証明契約、設計変更時の修正・再監査範囲は、Cの合理性を評価する**一資料**であって、Rustに対する排他的優越の必須証明ではない。初期移植可否に限らず実際の保守変更前後のauthor／ordinary callerへの影響を観察し、AIがコード記述を高速化しても残る設計制約は観察対象とするが、AI／人間の工数差を未測定で断定しない。

**公平性と実証の境界：** Rustには成熟したcrate・`Pin`・内部`unsafe`＋soundなsafe API・RAII等を使った**最善の自然な設計**を許す。Cの偶然のfield配置、式の順序、字句的APIをRustに強制しない。ただし利用者に不可欠なidentity／placement／external contractを別物に置き換えて「同等」とも数えない。Rustのborrow checkが防いだ欠陥、callerを守るwrapper、変更時に自動で検出される破綻は**実用上の利点**として数え、C側のmanual UB防止監査・レビュー・既存API化の費用も負担に含める。`unsafe`の行数、compiler errorの回数、コード量、単なる好みだけで総負担を判定しない。

**判定に必要な証拠：** 実際のconsumer／保守要求、双方の成立する操作契約、Rustの最善の構成と必要な追加設計、API・変更影響の具体的な差を記録する。**実装やAIへの生成指示より前に**、利用者に必要な各操作の計算量（最悪／期待／償却を区別）、追加storageと長期churn、original identity・owner移転・ABI、valid／invalid入力の安全性責任を、該当するものだけ固定する。たとえばCの既知ノードO(1) detachをRustの探索＋配列削除O(n)に変更したなら、値・アドレスが一致しても**元の計算量要件の同等性PASSではない**。その計算量要件が実利用者に必須かどうかは**別の製品判断**として明示し、要件を緩めたRust案も代替として公平に示す。RustにCの偶然の内部実装を強制せず、Rustの安全性向上とC側の未検査ポインタ前提・監査負担の差も記録する。実行PASS、source上の依存関係、測定した変更作業、利用者が費用を許容できないという証拠は**異なる強さの証拠**である。比較できる工数を実測していなければ「未測定」とする。実際のC利点の重要性が裏付けられない限り、**「Rustでも書けるが面倒そう」、C採用の惰性、未確認の移植障害だけでC合理性を認定しない**。反対に、Rustの動作一致や実用上の十分性だけでC合理性を棄却しない。**ターゲット適格性はNewLangのProduct GOを意味しない**。

**第1関門の対象分類（同じ要求・証拠の範囲内）：**

| 実際の選択状況 | ターゲット評価 |
| --- | --- |
| Rustでは実用上の制約が大きく、Cが明確に有利 | **有力候補** |
| **Rustも十分実用的だが、Cにも重要な合理的利点がある** | **十分に候補となる**（Rustとの排他的優位は不要） |
| Cの採用がほぼ過去の惰性で、実質的な利点が見つからない | **優先度は低い**（重要な追加証拠があれば再評価） |

分類は具体的な利用者・使用source・要求から行い、既存Cコードの存在だけで中段に自動昇格させない。**RustとのUB監査責務競争は第2段階のProduct GO条件ではない。** Rustの有力な代替が後から成立し、確認したC選択の理由を失わせた場合は第1関門を再評価する。Zig等の他の実用言語も、追加言語を導入する必要性を問う代替として考慮できるが、すべてに対する監査優位を形式的な合格条件にはしない。Rustの内部safe facadeや技術的証拠を研究・安全性検証の参照対象とすることまで禁止しない。

**成功の観測対象**は、人間が維持するL/R/P/V/C等のsemantic obligation、
author/callerの分担、unchecked前提をsafe mutationで破壊できるかという
negative evidence、変更時の相対的な再監査範囲・費用、および
topology維持である（分類は[North Star §8](NewLang_North_Star_Product_Validation.md#8-human-obligation-ledger)を再利用する）。
zero unchecked lines / zero bugsは目標指標ではない。

**保証と現状の線引き**：健全なcompiler・backend・runtime・FFI・platform
契約と、実際に正しい局所unchecked前提が成立した*将来の条件付き到達点*
として、公開ordinary-safe APIを合法的に使うcallerや監査済み依存閉包の
外側のsafeな変更が、内部uncheckedの前提を破壊しないことを目指す。
依存閉包の内側を変更する際には、真の前提の維持について
人間の再監査が残り得るが、compilerが機械的に保証する部分は
再監査対象から外せることを目指す。
logic / application policyの欠陥、C互換UB、未カバーのplatform effect、
v0範囲外のconcurrency、誤証明されたunchecked前提をなくす約束ではない。
現行Productionにgeneral source `unchecked` / `requires fn`、
module-level representation hidingの強制、general Ring safe APIが
実装済みだとは主張しない。現在の有限source → checked-owned →
C17/nativeのPASSやCHECK0／CI greenは、soundness theoremでもProduct GOでもない。

## 1.2 NewLang-native library design as the primary value-validation path

§1.1は、**任意の既存CソースをそのままNewLangへ移せば、すべての`unchecked`の監査依存範囲が小さくなる**という万能の移植保証ではない。元Cコードの偶然の式順序、helper分割、公開された内部表現まで忠実に再現することを、NewLangの製品価値の必要条件にしない。

**製品価値の主たる検証対象**は、Cを使う合理的な理由がある典型的なデータ構造・アルゴリズム（例：固定容量Ring、intrusive list、arena/allocator等）を、**NewLangで最初から自然に設計・実装した低水準library**である。library authorがそのworkloadのために合理的に選ぶpointer topology、物理配置、storage/backing、明示的なallocation・lifetime policy、実際の性能・外部契約を満たす。一方、偶然の元C実装詳細（式順序、helper分割、非契約上の内部表現）は維持する必要がなく、安全なAPI境界や失敗時の返却構成もNewLangに適した形を選んでよい。例えばintrusiveな元ポインタ同一性が要る操作を無関係なindex-only/GC表現へ置き換えるなど、**実用上不可欠な物理制約を外して別workloadにした結果**は同等の置換成功と数えない。

**責務分担の評価**では、compilerが現実的に判定できるlifetime / responsibility / provenance / currentness / access等をcheckedで保証し、なお真でなければならないが機械的に証明できない前提だけを、正当な原権限と実際の依存関係に基づく局所的な`unchecked`へ残す。`unchecked`の個数・字句的な近さ・全操作のchecked化を合格条件にしない。library authorは内部の論理的不変条件、影響する合法writer、失敗・rollback・破棄・再入・FFIの依存閉包を引き続き監査する。**ordinary-safe callerが許される全状態遷移**で内部の前提を破壊できないことも必要である。compilerの検査対象外となる本当の前提を`unchecked`と記しただけでは監査は局所化されない。

代表workloadは、ターゲット適格性を先に確認し、少なくとも一つの具体的な利用APIと実際の物理対象、正常経路、該当するFULL/EMPTY・allocation failure等の通常の失敗結果、破棄／責任の移転を持たせる。内部不変条件に影響する**合法な変更**と、影響しない**無関係なsafe変更**の両方を考え、それぞれでauthorとordinary callerが再監査するL/R/P/V/C依存範囲を特定する。ターゲット発見では、**利用者に不可欠な物理identity・失敗処理・実用API契約**を保った現実的なRust構成を比較する。Cの偶然の内部表現の再現は要求しないが、Rustによる動作一致だけで設計・変更・保守の自然さまでPASSとはしない。ターゲットが妥当と判定されてからの製品価値は、十分にfactorしたC実装を主要baselineとして測定する。Zig等も代替案として扱い、NewLang固有の利得が無ければ正直に未実証とする。局所化できない代表workloadや予想外に広い依存閉包も、重要な負の製品証拠として残す。

**既存Cの忠実な移植・実際の修正履歴は引き続き有効**である。original topology、移植可能性、checker精度、権限の偽造、失敗・teardown等の厳しい反例を調べる*補助的なpressure test*として使う。ただし一つの既存C実装をそのままsource/CHECK→nativeで再現できないことと、自然なNewLang-native実装の製品価値が成立しないことを同一視しない。逆に、簡単なNewLang-native toy例のCHECK/native成功もProduct GOではない。

**現行能力との区別：** Draft上の`unchecked`意味論、Array/Ring/ownerの概念的機構、またはlexical Node/Pairの有限C17/native成功は、general source `unchecked`、representation hiding、original heap ownerの安全な返却・refund、safe library APIを現Productionが実装済みである証拠ではない。代表ライブラリの設計評価と、現在のSOURCE／CHECK／公開owned／C17／nativeの到達範囲は別々に記録する。複数の独立機構不足があれば最初の不足で探索を停止し、Product GOまで無制限に実装を連鎖させない。

## 1.3 再利用可能なcompiler能力と製品価値検証への並行投資

**基本方針：** 初期ターゲットは引き続き§1.1の**Cを積極的・合理的に選ぶ領域**である。ただし個別North StarやピンポイントなworkloadのProduct GOは、**再利用可能なcompiler能力への投資の一律の前提条件ではない**。NewLangの実物のsource受理・semantic CHECK・真正な公開checked-owned証拠・C17/nativeが拡大すると、従来は推論に依存した製品仮説を実プログラムと反例で検証できる。コンパイラ開発は研究対象であると同時に**価値検証を可能にする研究基盤**でもあるため、価値実証と開発を過度に直列化しない。

**並行する三つの投資：** (A) 既選択のcanonical semanticsを実際のsourceから公開証明・C17/nativeまで接続し、異なるworkloadへ再利用できるcompiler、診断・検証基盤を**積極的に育てる**。(B) 新たな一般的ownership／effects／representation hiding／proof mechanism等の広く高コストな追加は、具体的な利用先、既存の代替、最初の障害、保守費用を比較して限定的に進める。(C) Track Vの実利用者にとって重要なC採用理由の発見と、**NewLang対十分にfactorしたC**のlibrary author＋ordinary-safe callerのUB-prevention audit/re-audit依存閉包の検証を並行して継続する。三者は独立して進め得るが、実測に応じて互いの優先順位を更新する。

**手堅い能力投資の条件：** 既選択の意味論に忠実であり、複数の具体的なsource／workloadへ再利用可能な見込みがあり、初めて不成立になるparser／CHECK／public issuer／C17／nativeの境界を反証可能な形で特定でき、実装・テスト・診断・将来互換性を含む費用が限定されていること。失敗時にも次の投資判断が改善することを含める。逆に、単一fixtureのためだけの専用profile・複雑なpublic validator・world複製の増殖を能力蓄積と数えない。新しい大きなproof/source/semantic機構を必要としたら、既存checked、正当な局所unchecked、別source構成、小さなlibrary境界を比較して停止・再裁定する。**安全性の独立検証を実装速度と引き換えに省略しない。**

**証拠と費用の境界：** SOURCE/CHECK／公開owned／C17/nativeの成功はCAPABILITY evidenceであり、一般soundnessでも§1.1のProduct GOでもない。製品化には実際のC採用理由、well-factored Cとの監査局所性改善、導入・運用・toolchain費用の検証が必要である。研究資源は**再利用可能なcompiler実装・基盤研究を重要な投資先として優先**しつつ、R/Fの独立安全性検証とVの価値探索を維持する。60/20/20等の比率は実測に基づく最適値ではなく、**固定quotaや追加の一律承認gateにしない**。具体的な許可・停止は[Development Process §4.3.1](NewLang_Project_Development_Process.md#431-exploration-go-and-product-go)に従う。

**長期の選択肢：** 初期のC後継価値を優先しながら、将来必要な型・APIだけに厳格なownership・exclusive access・不変条件保護を選べるよう発展し、Rustを合理的に選ぶ領域へも届く可能性を不必要に閉ざさない。これは現在のv0で包括的なRust型の規律や未選択機構を導入する許可ではなく、§6 no-foreclosureの長期的視野である。

## 2. 多段階ターゲティング

NewLangはcurrent North Starと最終市場を同一視しない。

> **North Star scope is an experimental beachhead, not a definition of NewLang's ultimate addressable workload.**

### Stage 0 — Evidence-led C-rational target discovery

実際の利用者が**なぜCを合理的に選び得るか**という積極的な理由から調べる。RustやGC言語でも同じ機能を実現できること、Rustも自然で実用的であることをCの自動棄却条件としない。現行stable Rust API、外部crate、適切な内部`unsafe`／safe wrapper、C FFI、対象platformやGCの代替は公平に評価し、Cの性能・構造・ABI／配備・変更費用などの具体的な便益がどこにあり、現実に重要かを記録する。実務上の状況は§1.1の**明確なC優位／両言語とも合理的／慣性のみ**の3分類で扱う。原因をfailure／recovery（F）、data structure／algorithm（D）、lifetime／resources（L）、alias／interaction（A）、execution／runtime（E）、portability／source deployment（P）等に整理するが、分類は仮説であって製品機能の採用リストではない。最初の現実的な調査候補はfallible host library、Cソース配布、小さな複数hook外部所有構造等。調査資料は[非規範研究レポート](research/C_Rust_Target_Discovery_2026-10-17.md)を参照。初期beachheadはこの調査だけで採用・変更しない。

### Stage 1 — Beachhead / North Star

最初に価値差を最も鮮明に検証する。主対象はskilled systems programmer、explicit pointer topologyを持つsingle-threadedまたはexternally synchronizedな低水準component、custom / intrusive structure、allocator/runtime/VM/GC、DB/storage内部、low-level library等。

Stage 1は最初に勝つ場所であり、NewLangの最終用途一覧ではない。

### Stage 2 — Adjacent systems workloads

Beachheadと同じsemantic foundationがOS component、embedded / firmware、storage/networking internals、game/runtime、allocator、custom container等へ自然に伸びるかを検証する。

### Stage 3 — Practical C-replacement envelope

新規にCを選ぶことが合理的な実用workloadについて、NewLangを選択不能にする大きなcapability holeを残さない状態を目指す。

ここではC ABI / FFI、function pointer / callback、external backing、mmap/shared memory/DMA、MMIO/volatile、raw storage、freestanding、atomics/concurrency、separate compilation/modules/linkage、platform ABI boundary等がcoverage問題になる。

### Stage 4 — Strategic destination

組織がNewLangを採用したとき、実用上重要なresidual C islandを十分小さくできる状態を目指す。

## 3. なぜC replacementまで目指すのか

実務上、GC言語・Rust・Cは、同じ要求を満たせる場合にも**それぞれ合理的な選択肢として共存**している。これは計算可能性による排他的な序列ではなく、物理構造、API／ABI、実行条件、開発者の設計自由、導入・保守費用などの実務上の選択である。

NewLangが一部の用途に新しい選択肢を一つ加えるだけなら、language／toolchainの導入・教育・保守という**追加の組織費用**を生む。したがって、Rustが扱えない用途だけを探して狭い隙間を占めることも、既にRustが実用的な用途をすべて除外することも、C後継としての長期目的を適切に表さない。

**長期的な狙いは、Cを選ぶ積極的な合理性がある実用領域で、NewLangがその設計・失敗・配備の自由を維持し、人間のUB防止責務を減らすことで、CからNewLangへの置換を徐々に実用的にすること**である。Rustの置換やRustへの排他的優越は目標ではない。RustもCも十分合理的なworkloadは、NewLangにとっても候補になり得るが、第2関門の**well-factored Cに対する監査局所性の改善**と導入・維持費用を別途示す必要がある。Cの合理的利点が存在しない場合、またはNewLangが追加言語費用に見合う便益を出せない場合は優先しない。

## 4. C replacementの意味

長期目標は:

> **Cが合理的に選択されている実用的なsystems-programming領域を、Cのunsafe semantic modelを引き継がずにNewLangで担えるようにする。**

英語では:

> **NewLang should eventually cover the practical capability envelope for which C is rationally chosen, without inheriting C's unsafe semantic model.**

これはC source compatibility、Cのundefined behavior互換、全compiler extension、C preprocessor programming model、C++ ABIの包括的再現を意味しない。置換対象はCのsource semanticsそのものではなく、Cが実務で担っている役割である。

## 5. Beachhead != Final target

Track Vは次を区別する。

```text
current validation scope
!= v1 product boundary
!= ultimate addressable workload
```

North Starからfeatureを外すことは、現在の価値仮説には不要という判断であって、将来もNewLangに不要という判断ではない。

逆に、将来C replacementに必要そうだからという理由だけでcurrent North Starへfeatureを追加しない。

## 6. No-foreclosure rule

Deferred capabilityについて必要に応じて次を確認する。

> **今のcore semantics / source decisionsが、将来そのC-class capabilityを自然に追加する道を不必要に閉ざしていないか。**

これを **no-foreclosure rule** とする。FFI、function pointer、external backing、in-place initialization等を今実装しなくても、後付け時に基礎意味論の破壊を要求しないかは先にpressure-testしてよい。

no-foreclosure reviewはfuture featureの実装authorizationではない。

## 7. Track V — Value / Product Validation

Track Vは **誰のどの問題を、どのevidenceで解けたと言うか** を担当する。

主な責務:

- target user / workload / workflowの定義
- **C-rational target eligibility**：実利用者にとってのCの**積極的・重要な採用理由**を確認し、Rust／GCその他の十分実用的な代替も公平に記録する。**RustとCの両方が合理的なケースを候補から棄却しない**。Cを選ぶ実質的に重要な利点、慣性や一度限りの移行費用、実際の利用者の受容・拒否、撤退条件を証拠とともに区別する
- value hypothesisの明文化
- North Star / representative workloadの選択
- PASS / FAIL criteriaとanti-gaming ruleのpre-registration
- human obligation、ceremony、topology preservationの比較（§1.1のauthor/caller別監査・再監査範囲とsafe mutationによる前提破壊リスクを含む）
- intermediate product gateの設計とfalse progressの検出
- implementation後のevidenceがproduct claimを本当に支持するかの評価
- practical C-replacement envelopeのcoverage risk管理
- Deferred capabilityへのno-foreclosure確認

Track Vはcanonical language semantics、Draft revision、implementation architecture、formal proof strategy、merge / milestone sequencingを単独では決定しない。

## 8. 他trackとの問いの分離

```text
V: 誰の何が改善されれば価値があるか？
M: その価値を成立させる最小で整合したlanguage ruleは何か？
F: そのruleは期待する性質を形式化できるか？
P: production compilerで忠実にcheck / diagnose / lowerできるか？
R: preferred interpretationを外して読むと何が壊れるか？
Coordination: どのfindingを、いつ、どのtrackへ渡すか？
```

## 9. Slice選択原則

新しいfeature / sliceでは少なくとも次を見る。

- **User value**: 誰のどのreal workloadが改善するか。
- **Beachhead necessity**: current gateに本当に必要か。
- **Failure attribution**: 複数の未確定mechanismを一つのgateへ混ぜていないか。
- **Anti-false-progress**: host fixture、toy rewrite、unchecked、backend trickでuser-visible gapを隠していないか。safe mutationによるunchecked前提破壊の反例・未検証範囲を隠していないか。
- **Topology preservation**: NewLangを通すために元のC architectureを本質的に別物へ変えていないか。
- **NewLang-native validation**: 既存Cソースの逐語的な再現を必須にしない。まず**Cを選ぶ重要な積極的理由**がある要求を特定する（Rustも実用的であることだけで棄却しない）。その要求をNewLang-native低水準libraryで保持した上で、同等topology・実用契約の**Cとの**author＋ordinary-safe callerの監査依存閉包を検証できるか。Rust/Zigはターゲット適格性・他の代替可能性を再評価する参照先であり、NewLangとの一般的なUB監査優劣レースを作らない。
- **No foreclosure**: 小さな現在判断が将来のC replacementを不必要に閉ざしていないか。
- **Lifecycle cost**: implementation costだけでなくsemantic/test/diagnostic/compatibility/documentation/future-interaction costを数える。
- **Reusable compiler capability**: Product GOや特定North Starの完遂前でも、既選択のsemanticsを真正なsource→公開証拠→C17/nativeに再利用可能な形でつなぐ研究・実装の価値（§1.3）を評価する。

AIによりimplementation costが下がっても、semantic complexityやcompatibility costは自動的には下がらない。

## 10. Deferredのproduct分類

- **Not needed for current gate** — 現在のvalue hypothesisに不要。将来必要性は未判定。
- **Deferred but addable** — 将来必要になり得て、現在設計から自然に追加できるevidenceがある。
- **Strategic coverage gap** — practical C-replacement envelopeに重要だが、追加可能性のevidenceが不足。
- **Outside strategic envelope** — C互換のためだけで、NewLangの実用C replacementには不要と判断したもの。

これはproduct/strategy分類であり、language validity分類ではない。

## 11. Product success

成功はfeature countではなく、次の順で積み上げる。

```text
one sharp value claim
  -> actual-source vertical evidence
  -> adjacent workload reuse
  -> capability coverage expansion
  -> residual-C reduction
```

最終的には、**Rustを含む他の選択肢も存在するなかで、Cを選ぶ重要かつ積極的な合理性があるworkload**が実在し続けるか（Rustも十分実用的であり得る）、そこでNewLangがCの物理・失敗・配備条件を維持して**十分にfactorしたC**に比べsemantic bookkeepingとauthor／ordinary-safe caller双方のUB予防監査・再監査範囲を実務上どれだけ減らすか（safe mutationの負の証拠と相対的な再監査半径を含む）、追加language/toolchainコストに見合うかで評価する。

## 12. 関連文書

- `NewLang_North_Star_Product_Validation.md` — current beachhead / North Starのproduct validation contract
- `NewLang_C_Replacement_Coverage_Map.md` — practical C-replacement envelopeのcoverage ledger
- `NewLang_Project_Development_Process.md` — track運用・handoff・authority
- `reference/CURRENT_SPEC.md` — canonical language semanticsを指すauthority
