# Rustが利用可能な現在でも、Cを選ぶ合理的理由は残るか

**NewLang向け非規範的研究レポート**  
**調査基準日：2026年10月17日**

## A. Executive Summary

### 結論

**RustではなくCを選ぶ合理的理由は、現在も存在する。ただし、確認できた理由の中心は「Rustでは実現できない」ことではなく、要求する配備範囲・失敗処理契約・組込み方法を維持するための費用と制約である。**

今回確認した範囲では、intrusive list、手動解放、arena、独自allocator、freestanding、共有アクセスといった能力について、**Rust言語全体の原理的な不可能性を示す証拠は得られなかった**。一方、Rustの一般的な標準ライブラリ構成をそのまま採用すると満たせず、外部ライブラリ、専用抽象、FFI境界、独自実装の保守が必要になる要求は確認できた。

最も証拠が強いのは、次の三つである。

1. **必要なtarget・toolchain・組込み先で利用できること。**  
   LinuxのRust対応は、C側の対応範囲全体と一致しない。ZephyrもRust対応targetの限定を明記している。必要な全環境で共通コードを動かす要求は、新規モジュールにも適用される。[Linux対応表](https://docs.kernel.org/rust/arch-support.html)、[Zephyr Rust対応文書](https://docs.zephyrproject.org/latest/develop/languages/rust/index.html)

2. **ライブラリが失敗時に呼出し元のprocessを終了させず、判断を返すこと。**  
   SQLiteとcurlは、この要求を実際の設計契約として持つ。Rustでも実装可能だが、stable標準APIと任意の依存ライブラリを組み合わせるだけで、全経路のfallibilityが揃うわけではない。[SQLite allocation設計](https://www.sqlite.org/malloc.html)、[curl開発方針](https://daniel.haxx.se/blog/2025/04/07/writing-c-for-curl/)

3. **利用者の既存Cビルドへ、ソースを直接組み込めること。**  
   Sokolは言語間・プロジェクト間の統合容易性をC採用理由として明示する。ClayやNuklearも、単一ヘッダや依存の少ない配布方式を製品特性としている。これはC ABIを提供することより強い要求である。[Sokol](https://github.com/floooh/sokol)、[Clay](https://github.com/nicbarker/clay)、[Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)

ただし、これらはそのままNewLangの優位性にはならない。**Cに蓄積されたtarget対応、ビルド互換性、成熟性は、新言語にも通常は欠ける。** NewLangが同じ要求を維持できなければ、C継続の根拠はNewLangにも不利に働く。

### 六分類の評価

| 分類 | 調査結果 | NewLangの対象発見への意味 |
|---|---|---|
| **P：Portability / Deployment** | 最も強い。実際の対応範囲・組込み方式の差を確認 | 需要は実在。ただしNewLangが解決できるかは別問題 |
| **F：Failure / Recovery** | 強い要求証拠。Rustの全体的な追加負担の大きさは未測定 | 最優先の比較実験候補 |
| **D：Data Structure** | 必要な構造はRustでも概ね実現可能 | 一般論では弱い。外部所有・複数hook・動的操作の組合せに限定して検証 |
| **L：Lifetime / Resource** | PostgreSQL等の既存契約との摩擦は具体的 | 独立分類よりF・D・FFIとの組合せが有用 |
| **A：Aliasing / Interaction** | 追加契約はあるが、広いC優位の採用証拠は弱い | callback再入や外部pointer保持を具体化する必要 |
| **E：Execution / Runtime** | 「Rustには大きなruntimeが必要」は否定 | 単独ターゲット仮説としては弱い |

### 戦略判断

NewLangが追加調査すべきなのは、「Cで書けるもの全般」ではない。以下の要求を持つ小さなworkloadである。

- 全allocation失敗を通常の失敗として返す、組込み用の状態付きライブラリ。
- 外部所有・非移動ノードを、複数のlistやindexで扱う部品。
- Cコンパイラだけで利用者が再ビルドする、ソース配布型ライブラリ。

**一般的なembedded開発、TLS、kernel driver、graph処理を広くNewLangの主要ターゲットにする根拠は不足している。** Embassy、Rustls、Rust版Binder、Androidの新規コードは、Rustが有力な解決策である反証となる。[Embassy](https://embassy.dev/)、[Rustls FFI](https://github.com/rustls/rustls-ffi)、[Binder](https://rust-for-linux.com/android-binder-driver)、[Android経験報告](https://blog.google/security/rust-in-android-move-fast-fix-things/)

### 調査方法と限界

本調査は、公式文書、maintainerの説明、公開API、公開source・testを用いた文献・実装調査である。**独自のbenchmark、fault injection、比較実装、工数測定は実施していない。**

13件のプロジェクト事例を比較したが、13件すべてが「Rustを検討してCを選んだ」事例ではない。採用理由の明示、要求だけの確認、Rustによる反例を区別した。

近年のC開発例としてClay、およびSokolの新機能開発を確認した。ただし、近年の新規開発について、**Rustとの比較検討と棄却理由を公開した強い一次資料は、十分な件数を確保できなかった**。新規C開発の存在を、Rust不適格の証明には用いない。

日付不明の継続更新文書は、公開日を推定せず「確認日時点の文書」とした。`latest`・`main`リンクは将来変化するため、後続実験ではversionとcommitを固定する必要がある。

NewLangのProject Compass／Product Value Strategyは、本調査の根拠として使用していない。指定repositoryは今回の取得経路では内容を確認できなかった。NewLangに関する評価は、実装済み能力の認定ではなく、要求への予備的な適合条件である。

---

## B. 分類ごとの詳細分析

### B.1 F — Failure & Recovery Policy

#### 要求は実在する

SQLiteはallocation失敗時にcacheの解放と再試行を行い、それでも失敗すれば`SQLITE_NOMEM`を返すか、要求したmemoryなしで処理する設計を説明している。curlも、system libraryがエラー時にexit／abortせず、applicationへ判断を返す必要をmaintainer自身が明示する。[SQLite](https://www.sqlite.org/malloc.html)、[curl](https://daniel.haxx.se/blog/2025/04/07/writing-c-for-curl/)

これは「OOMでも何としても続行する」という要求ではない。守るべき契約は、例えば次のようなものである。

- 今回の操作は失敗する。
- 既存の有効状態や永続化契約を維持する。
- 入力資源を返す、または定義済みの方法で解放する。
- 次の操作を受け付けられる状態へ戻す。
- 復旧不能なら、定義した境界で停止する。

**allocation失敗、内部不変条件の破壊、OSによる強制終了は別の失敗である。** PostgreSQL文書も、OSのovercommitによりprocessがOOM killerで終了し得ると説明する。Cの`malloc`失敗処理だけでprocess survivalが保証されるわけではない。[PostgreSQL kernel resource文書](https://www.postgresql.org/docs/18/kernel-resources.html)

#### Rust stableで既にできること

| 要求 | 現実的なRust構成 | 評価 |
|---|---|---|
| 範囲外入力を通常の失敗にする | sliceの`get`／`get_mut` | stableで十分 |
| 連続bufferの拡張失敗を返す | `Vec::try_reserve`／`try_reserve_exact` | stableで可能 |
| 事前確保後にallocationなしで処理する | capacityを確保し、追加allocationのない操作を選ぶ | 可能。ただし要素生成・callback等は別途確認 |
| 固定容量collectionの満杯を返す | `heapless`等 | 外部libraryで可能 |
| 独自allocationと解放 | `alloc`／`dealloc`、raw pointer、専用wrapper | stableの低水準機能で可能 |
| 部分初期化 | `MaybeUninit`と初期化済み部分の管理 | 可能。cleanup契約が必要 |
| 通常の失敗を伝播する | `Result`、`?`、明示的rollback | 言語機能で十分 |

根拠：[slice](https://doc.rust-lang.org/std/primitive.slice.html#method.get)、[Vec](https://doc.rust-lang.org/std/vec/struct.Vec.html)、[heapless](https://docs.rs/heapless/latest/heapless/)、[allocation](https://doc.rust-lang.org/std/alloc/fn.alloc.html)、[MaybeUninit](https://doc.rust-lang.org/std/mem/union.MaybeUninit.html)

したがって、`a[i]`がpanicすることは、Cを選ぶ一般的な理由にならない。適切な代替操作がある。

#### 現在の標準APIの穴

確認したstable標準文書は**Rust 1.99.1、2026-10-13付**である。この文書では`Box::try_new`等は引き続きunstableとして表示される。`BTreeMap`にも、allocation失敗を通常のエラーとして返す挿入APIは確認できない。`try_insert`という名前だけでOOM回復APIと判断してはならない。[stable標準文書](https://doc.rust-lang.org/stable/std/index.html)、[Box](https://doc.rust-lang.org/stable/std/boxed/struct.Box.html#method.try_new)、[BTreeMap](https://doc.rust-lang.org/std/collections/struct.BTreeMap.html)

ただし、重要な直近の改善がある。

**`Allocator`の一部を安定化するPR #156882は、2026-09-23にmerge済みである。** tracking issueでは、問題がなければ1.100に入る予定と説明されている。しかし安定化範囲は部分的であり、「標準collectionの全操作がfallibleになる」という変更ではない。[安定化PR](https://github.com/rust-lang/rust/pull/156882)、[tracking issue](https://github.com/rust-lang/rust/issues/32838)

従って、次の両方が誤りである。

- 「allocator APIは将来もstableにならない」
- 「allocator APIが安定化するのでfallible-by-design問題は解決した」

#### 外部libraryは有力な代替である

`allocator-api2`にはfallibleな`Box`構築があり、`fallible_collections`も提供されている。`hashbrown`には`try_reserve`がある。これらを使った構成を比較対象から外すべきではない。[allocator-api2 Box](https://docs.rs/allocator-api2/latest/allocator_api2/boxed/struct.Box.html#method.try_new)、[fallible_collections](https://docs.rs/fallible_collections/latest/fallible_collections/)、[hashbrown](https://docs.rs/hashbrown/latest/hashbrown/struct.HashMap.html#method.try_reserve)

ただし、ある型がfallibleでも、その型を受け渡す依存先が別のcollection型やinfallible操作を要求すれば、契約はそこで途切れる。この統合負担はAPI構成から導かれる分析であり、今回工数を実測した結果ではない。

同様に、`Result`を返す関数でも、その内部で行う`clone`、文字列生成、logging、callback、cleanupがすべてfallibleとは限らない。**返り値の型だけでは、処理全体の非abort契約を証明できない。**

#### panic捕捉は代替にならない

`catch_unwind`はunwindするpanicを捕捉する。abortするpanicは捕捉しない。`handle_alloc_error`の通常の`std`構成での既定動作はprocess abortである。[catch_unwind](https://doc.rust-lang.org/std/panic/fn.catch_unwind.html)、[handle_alloc_error](https://doc.rust-lang.org/std/alloc/fn.handle_alloc_error.html)

従って、application全体を`catch_unwind`で囲むだけでは、通常のallocation失敗を回復可能なエラーへ統一できない。

逆に、panicを利用しなくても、`Result`と適切な状態設計で復旧できる。RAIIはその際のcleanupを助け得る。ただし**資源を解放することと、操作をrollbackすることは異なる**。状態変更や外部I/Oの取消しは、CでもRustでも別途設計が必要である。

#### 実例と判定

LinuxのRust側には、allocation flagsを受け取り、失敗を`Result`で返す`KBox`等がある。これはkernel要求をRustで満たす反証である。同時に、汎用標準型だけでは足りず、専用APIを維持する実例でもある。[Linux KBox](https://rust.docs.kernel.org/kernel/alloc/kbox/type.KBox.html)

**判定：F仮説は限定付きで支持。**

- 復旧契約の実在：強い。
- Rust標準ecosystemの均一性不足：確認。
- Rustで実用的に実現できない：支持しない。
- Cより大幅に高コスト：具体的な比較実験が必要。

---

### B.2 D — Data Structure & Algorithm Freedom

#### 比較対象を分ける

以下は異なるものである。

1. Rust標準の`std::collections::LinkedList`
2. 外部crateの`intrusive_collections::LinkedList`
3. raw pointer等による専用実装

標準`LinkedList`は、要素を内部nodeへ格納する所有containerである。利用者struct内のhookを結ぶgeneral intrusive listではない。cursor APIも確認時点の文書ではunstableである。[標準LinkedList](https://doc.rust-lang.org/stable/std/collections/struct.LinkedList.html)

この制約をRust言語全体へ一般化してはならない。

#### C側の基準

Linuxのlist APIは、利用者structに埋め込む`list_head`を操作する。allocationと解放はlist操作自体から分離され、move、replace、splice、走査中削除等を提供する。[Linux list API](https://docs.kernel.org/core-api/list.html)

BSD queueも参照になるが、今回確認できたmanualには歴史的versionが含まれるため、2026年の現行BSD全体の状況を代表させない。[FreeBSD queue manual](https://man.freebsd.org/cgi/man.cgi?manpath=FreeBSD+12.1-RELEASE&query=queue&sektion=3)

Boost.Intrusiveは設計比較には有用だが、**C++ libraryである**。Cプロジェクトの採用理由を示す証拠としては数えない。[Boost.Intrusive](https://www.boost.org/doc/libs/latest/doc/html/intrusive/usage.html)

#### Rust外部libraryとの機能比較

確認した`intrusive-collections`の文書versionは**0.10.3**である。

| 必要な自由 | Linux型C API | Rust `intrusive-collections` | 残る差・注意 |
|---|---|---|---|
| node allocationと所属の分離 | 可能 | 借用pointer、各種所有pointer等で可能 | pointer方式を選ぶ |
| lifetimeとlist所属の分離 | 呼出し側の契約 | 借用型または`UnsafeRef`等で可能 | lifetime／手動保証が必要 |
| 複数hook | 可能 | adapterで別hookを選択可能 | 同じhookの二重所属とは区別 |
| 同一objectの複数list所属 | 別hookで可能 | 別hookと適切なpointer方式で可能 | 独占所有`Box`を単純に重複できない |
| 元addressの保持 | 呼出し側が保証 | 借用、安定したallocation、`Pin`等で可能 | `Pin`は唯一の方法ではない |
| 既知nodeのO(1) detach | 可能 | pointerからcursorを作り削除可能 | 所属の保証を伴う`unsafe`入口 |
| 既知位置へのinsert／move | 可能 | cursorによるremove／insert等で構成可能 | cursorの借用規則に従う |
| 走査中の削除 | safe iteration用APIあり | `CursorMut::remove`が次へ進む | 通常iteratorとの混同に注意 |
| splice | 可能 | `splice_before`／`splice_after`あり | 同じadapter型等の条件 |
| replace | 可能 | `replace_with`あり | 二重link等ではpanic契約あり |
| 明示的teardown | 呼出し側が決定 | pointer方式、remove、clear等で構成可能 | `clear`と`fast_clear`は意味が違う |
| 所属の検証 | 一般に呼出し側、debug機構等 | link状態検査とunsafe所属契約 | 「linked」と「このlistに所属」は別 |

根拠：[crate全体](https://docs.rs/intrusive-collections/latest/intrusive_collections/)、[LinkedList](https://docs.rs/intrusive-collections/latest/intrusive_collections/linked_list/struct.LinkedList.html)、[CursorMut](https://docs.rs/intrusive-collections/latest/intrusive_collections/linked_list/struct.CursorMut.html)、[UnsafeRef](https://docs.rs/intrusive-collections/latest/intrusive_collections/struct.UnsafeRef.html)

O(1)という評価は、通常のlink・adapterで近傍pointerを定数個更新するsource構造に基づく。独自adapter、destructor、allocatorまで含む処理時間の保証や実測値ではない。[公開source](https://docs.rs/intrusive-collections/latest/src/intrusive_collections/linked_list.rs.html)

#### 本当に残る制約

**第一に、所属状態の保証が必要になる。**  
`cursor_mut_from_ptr`は、そのpointerが当該listの要素であることを呼出し側に要求する。`is_linked`だけで所属listの同一性まで証明できるわけではない。

**第二に、safe APIが公開する借用と、動的な操作規律を整合させる必要がある。**  
mutable cursorがlistを借用している間に、callbackが同じlistを別経路から変更する設計は、そのままでは使えない。callbackを後回しにする、借用を閉じる、操作commandを蓄積する、専用wrapperへ変更する等の選択肢がある。これはAPIから導かれる設計上の分析であり、すべての用途で重大な費用になるという実測証拠ではない。

**第三に、手動所有方式でもRust参照の契約は残る。**  
`UnsafeRef`はreference countなしで利用できるが、利用中のobjectをmove・dropしたり、不適合なmutable referenceでaccessしたりしない保証が必要である。[UnsafeRef契約](https://docs.rs/intrusive-collections/latest/intrusive_collections/struct.UnsafeRef.html)

C側にも不正な所属、解放後参照、iterator無効化の責任がある。Linux文書は、`list_splice`後にdonor headを再利用すると不正な走査になる例を示し、`list_splice_init`を区別する。**Cの操作自由は、安全性の証拠ではない。**[Linux splice説明](https://docs.kernel.org/core-api/list.html)

#### 判定

**「Rustはgeneral intrusive doubly linked listを実現できない」という仮説は棄却する。**

残る仮説は、より狭い。

> 外部所有、複数hook、保持済みnode pointer、callbackによる再入、任意の操作順を同時に要求するとき、既存Rust libraryへの適合または専用safe wrapperの保守が大きな負担になるか。

この負担の存在条件は明確化できるが、大きさは未測定である。一般的なintrusive listの存在だけではNewLang投資の根拠にならない。

---

### B.3 L — Lifetime, Ownership & Resource Policy

#### Rustは手動管理を禁止していない

`ManuallyDrop`、`MaybeUninit`、raw pointer、`Box`のraw変換等によって、初期化、破棄、allocation解放を分ける構成が可能である。`Pin`もallocation方式や所有方式そのものを指定する機構ではない。[ManuallyDrop](https://doc.rust-lang.org/std/mem/struct.ManuallyDrop.html)、[MaybeUninit](https://doc.rust-lang.org/std/mem/union.MaybeUninit.html)、[Pin](https://doc.rust-lang.org/std/pin/index.html)

ただし、`ManuallyDrop`で自動破棄を抑制しても、二重破棄、破棄済み値へのaccess、公開generic型の不適切な操作が安全になるわけではない。`Pin`にも保持期間と破棄に関する契約がある。

#### 摩擦が具体的なのは既存hostの資源契約

PostgreSQLはmemory context単位でallocationをまとめて回収する。pgrxの文書は、contextの寿命にallocationの寿命が依存し、Rust側のlifetime表現と正しく結び付ける難しさを説明する。[pgrx memory context文書](https://github.com/pgcentralfoundation/pgrx/blob/develop/docs/src/pg-internal/memory-context.md)

これは次の要求が重なる事例である。

- allocation元はhostが決定する。
- 個別objectの所有者と、memory回収の主体が一致しない。
- 通常return以外の失敗経路がある。
- FFI越しに資源責任が移る。

Rustでもwrapperとlifetime設計で扱える。しかし、既存C idiomをraw pointerへ機械的に移しただけでは、safe referenceを公開できるとは限らない。

一方、arenaとlist所属の分離自体は`intrusive-collections`の借用pointer構成でも提供される。したがって「arenaが必要だからC」は成立しない。[scoped collections](https://docs.rs/intrusive-collections/latest/intrusive_collections/#scoped-collections)

#### 判定

**Lは、独立した広いC優位領域としては証拠不足。**

F・D・FFIと重なる既存契約への適合問題として扱う方が、調査対象を具体化できる。新規設計で通常のRust所有型へ整理できるなら、Cを選ぶ積極的理由は弱くなる。

---

### B.4 A — Aliasing, Access & Interaction Policy

#### `unsafe`でも参照契約は消えない

Rust Referenceは、`unsafe`内でもUBが許されないこと、参照のaliasing契約に従う必要があることを明示する。raw pointerを使えることと、任意の場所で`&mut T`を作れることは異なる。[Rust Reference](https://doc.rust-lang.org/reference/behavior-considered-undefined.html)

`UnsafeCell`は共有参照下の内部可変性を可能にするが、mutable referenceの排他性やdata raceの禁止を解除しない。[UnsafeCell](https://doc.rust-lang.org/std/cell/struct.UnsafeCell.html)

従って、例えば以下では追加の設計が必要になる。

- FFI先がpointerを保持し、後で変更する。
- callbackが同一objectへ再入する。
- DMAや割込みが、CPU側の参照と重なるmemoryへaccessする。
- RCU等で解放時点を遅延する。

#### 代替策は存在する

raw pointer、`UnsafeCell`、atomic、critical section、短い参照、専用guardを組み合わせられる。epoch reclamationにも既存libraryがある。[crossbeam-epoch](https://docs.rs/crossbeam-epoch/latest/crossbeam_epoch/)

MMIOのvolatile操作も提供される。ただしvolatileは一般的なthread同期機構ではなく、DMAの所有権移譲、cache整合性、device固有の順序を自動的に解決しない。[read_volatile](https://doc.rust-lang.org/std/ptr/fn.read_volatile.html)

Cにもdata race、object lifetime、aliasingに関する規則がある。比較すべきなのは、**正当なaccess規律を実装・維持するための契約差**であり、「Cでは自由に共有してよい」という前提ではない。

#### 反証と判定

Rust版Binderのmainline導入や、EmbassyのHALは、複雑な共有・device interactionがRustで扱える反証である。[Binder](https://rust-for-linux.com/android-binder-driver)、[Embassy](https://embassy.dev/)

**A仮説は条件付きの追加負担として残るが、Cを選ぶ広い独立理由としては証拠が弱い。** RCU、hazard pointer、QSBRを同一視せず、実際のreader／writer／reclamation契約を固定した比較が必要である。

---

### B.5 E — Execution & Runtime Policy

#### 広いruntime仮説は否定される

Rustは`no_std`でOS依存の標準ライブラリを使わずに構成できる。GCや大型runtimeが必須ではない。[Embedded Rust Book](https://docs.rust-embedded.org/book/intro/no-std.html)

Embassyはdynamic allocation不要のtask実行方式を提供する。確認した`embassy-executor`文書は0.10.0で、nightly機能は独立したfeatureとして示されている。[Embassy](https://embassy.dev/)、[executor文書](https://docs.rs/embassy-executor/latest/embassy_executor/)

したがって、以下だけではC選択を正当化できない。

- heapを使わない。
- OSがない。
- GCを許容しない。
- small MCUである。
- stack budgetが厳しい。

#### 残る論点

実用上は、次を選択した具体的構成で評価する必要がある。

- panic handlerとunwind／abort方針。
- hidden allocationの有無。
- destructorが行う処理と、その時間上限。
- stack使用量、再帰、割込み時の挙動。
- linker、startup、vendor SDKとの統合。

これらについて、C一般に対するRust一般の性能差・size差は今回実測していない。Cの実装にもlibraryや再帰、cleanupに起因する予測不能性はあり得る。

**判定：E単独のターゲット仮説は弱い。** 実行環境の問題の多くは、Pの対応範囲、Fの失敗契約、Lの資源契約へ分解した方がよい。

---

### B.6 P — Portability, ABI & Deployment

#### 最も明確な現実の差

Linuxの現行対応表ではRust対応architectureと制限が列挙される。例えばARMはARMv7 little-endian、x86はx86_64等の条件がある。**Rust compilerのtarget一覧、Linux kernel内のRust対応、個別driverの対応範囲は別の集合である。**[Linux対応表](https://docs.kernel.org/rust/arch-support.html)

RustのTier 2は、公式buildがある一方、常時の実行testまで保証する分類ではない。Tier 3も一律に「利用不能」と判定できない。必要な実機・機能・保守契約で判断する必要がある。[rustc platform support](https://doc.rust-lang.org/rustc/platform-support.html)

GCC側にもRust frontendの`gccrs`、rustcのGCC backendがある。したがって「RustはLLVM以外では動かない」という一般化は不適切である。ただし、これらの存在だけで、必要なRust版・crate・targetが既存GCC C構成と同等に利用できるとはいえない。[gccrs 2026年8月報告](https://rust-gcc.github.io/2026/09/08/2026-08-monthly-report.html)、[rustc_codegen_gcc](https://github.com/rust-lang/rustc_codegen_gcc)

#### ABIとソース配布を分ける

Rustは`extern "C"`、C表現、static／shared library等でCとの相互運用ができる。Rustls FFIも実例である。[Rust FFI](https://doc.rust-lang.org/nomicon/ffi.html)、[C表現](https://doc.rust-lang.org/reference/type-layout.html#the-c-representation)、[Rustls FFI](https://github.com/rustls/rustls-ffi)

従って、**「Cから呼べるlibraryが必要」だけでは、実装言語をCにする理由にならない。**

一方、以下は追加の要求である。

> 利用者が、手元のCコンパイラと既存ビルドだけで、提供ソースを再コンパイル・設定・監査できること。

Rust実装＋C ABIは、この要求を自動的には満たさない。prebuilt binaryを提供すれば利用時のcompiler依存は減らせるが、対象組合せごとの配布・再構築・更新責任が発生する。

#### 依存、固定、認証に関する反証

Cargoにはvendoringとoffline構成がある。「RustはbuildのたびにInternetへ接続しなければならない」は誤りである。[cargo vendor](https://doc.rust-lang.org/cargo/commands/cargo-vendor.html)、[offline設定](https://doc.rust-lang.org/cargo/reference/config.html#netoffline)

安全認証向けにもFerroceneが存在する。従って「認証が必要ならRustは不可能」も誤りである。ただし、認証は対象compiler版、target、library範囲、利用条件ごとに確認する必要がある。[Ferrocene対象範囲](https://public-docs.ferrocene.dev/main/user-manual/targets/index.html)、[認証済みcore subset](https://public-docs.ferrocene.dev/main/safety-manual/core/subset.html)

#### 判定

**P仮説は強く支持。**

ただし、二つに分けるべきである。

- **P1：必要環境への対応** — 新規コードでも成立する技術・製品制約。
- **P2：配布・統合・保守方式** — Cソース組込み、ABI、依存、更新、認証等の運用契約。

NewLangはどちらについても、自らの対応能力を示す必要がある。

---

## C. 実プロジェクトのケーススタディ

### C1. SQLite — 復旧可能な組込みdatabase library

公式C採用理由文書は2025-05-09更新。互換性、低依存、安定性、品質保証方式、OOM回復を挙げる。OOM injectionも公開test方針に含まれる。[採用理由](https://www.sqlite.org/whyc.html)、[test方針](https://sqlite.org/testing.html)

Rustではfallible collection、専用allocator、C ABI、`no_std`構成が候補になる。公式文書中のRustに関する評価はmaintainerの判断であり、現在の全Rust能力を実証した比較ではない。

**現在も成立する理由：** 巨大な検証資産を維持しながら、hostへ失敗を返すlibrary契約を保つこと。  
**新規への一般化：** OOM回復と配布要求は一般化できる。SQLite全体の書換え回避は既存資産の理由である。

### C2. curl／libcurl — 呼出し元の判断権と広い配布範囲

2025-04-07のmaintainer説明は、C89による移植性、API／ABI維持、全エラーの処理、libraryがabortしない方針を示す。一方、C由来の脆弱性も認めている。[Writing C for curl](https://daniel.haxx.se/blog/2025/04/07/writing-c-for-curl/)

hyper backendは2024-12-21に撤去が説明された。主要因は需要・両言語を扱う開発者・統合保守の不足であり、RustでHTTPを実装できないためではない。同記事は他のRust backendとの併用も説明する。[dropping hyper](https://daniel.haxx.se/blog/2024/12/21/dropping-hyper/)

**現在も成立する理由：** F・Pに加え、既存資産と保守体制。  
**新規への一般化：** libraryの非abort契約と必要targetは成立。hyper撤去を新規Rust採用一般へ拡張することはできない。

### C3. Linux kernel — 共通コードの対応範囲と専用抽象

architecture対応表は、共通コードのRust化が新しいtarget条件を持ち込み得ることを示す。他方、Rust側にはfallible allocation、intrusive list等の専用実装がある。[architecture](https://docs.kernel.org/rust/arch-support.html)、[allocation](https://rust.docs.kernel.org/kernel/alloc/kbox/type.KBox.html)、[list source](https://github.com/torvalds/linux/blob/master/rust/kernel/list.rs)

Rust for Linuxは、unstable機能の一部利用と、その削減方針を説明する。通常のstable applicationと、kernel専用構成を同一視できない。[unstable機能](https://rust-for-linux.com/unstable-features)

**現在も成立する理由：** 対応範囲、既存API、抽象の整備・保守費用。  
**新規への一般化：** 多architectureで共用する新規codeには強い。対応済み環境専用driverには弱くなる。

### C4. PostgreSQL／pgrx — hostの失敗・memory契約への適合

2025年FOSDEM／PGDay開発者会議記録には、`setjmp`／`longjmp`、memory context、allocator API、packagingへの懸念がある。これは開発者の懸念を示す一次資料であり、2026年の包括的なRust不採用決議ではない。[会議記録](https://wiki.postgresql.org/wiki/FOSDEM/PGDay_2025_Developer_Meeting)

反証として、pgrxは実際にRust extensionを支援する。v0.18.0のrelease説明は、FFI境界でPostgreSQLの`longjmp`を捕捉しRust panicへ変換する仕組みと、build工程の改善を説明する。[pgrx](https://github.com/pgcentralfoundation/pgrx)、[v0.18.0](https://github.com/pgcentralfoundation/pgrx/releases/tag/v0.18.0)

**現在も成立する理由：** 既存host契約への直接適合。  
**新規への一般化：** 新しいextensionにも適用されるが、pgrxで十分なextensionを除外すべき。新規database engine全般には一般化できない。

### C5. Zephyr — board／toolchain対応の非対称

Zephyr 4.1の2025年告知はRust application対応をexperimentalとして導入した。現在の文書も対応targetの限定を記す。ただし、2025年のexperimental表示を2026年全体の状態として固定してはならない。[4.1告知](https://zephyrproject.org/zephyr-rtos-4-1-is-available/)、[現在の対応文書](https://docs.zephyrproject.org/latest/develop/languages/rust/index.html)

Rustはoptional moduleとして利用でき、対応する環境ではCと併用できる。

**現在も成立する理由：** 必要board・SDK・APIがRust構成で揃うか。  
**新規への一般化：** 成立する。ただし対象boardごとの確認が必要であり、組込み全般のC優位ではない。

### C6. littlefs — bounded memoryと永続状態の復旧

READMEは、power-loss resilience、bounded RAM、無制限再帰を使わないこと、bufferを静的提供できることを設計特性として示す。[littlefs](https://github.com/littlefs-project/littlefs/blob/master/README.md)

Rust側には`littlefs2`のwrapperがあり、`no_std`にも対応する。ただしC実装を利用する構成であり、純Rust再実装の同等性を示す証拠ではない。[littlefs2](https://github.com/trussed-dev/littlefs2)

**現在も成立する理由：** 成熟したC部品の再利用と組込み容易性。  
**新規への一般化：** bounded memoryや電源断復旧自体はRustでも設計可能。C固有の優位は未確認。

### C7. Sokol — 他言語・他projectへ配るC部品

READMEはC採用理由として、他言語との統合、他projectへの統合、実行fileへの小さい追加負担を明示する。公式Rust bindingも自動更新される。[Sokol](https://github.com/floooh/sokol)

2025-05-19の記事は、新しいcompute機能とresource設計の具体的な更新を説明する。単なる休眠C資産ではない。ただし、この更新時にRustを比較して棄却した証拠ではない。[compute更新](https://floooh.github.io/2025/05/19/sokol-gfx-compute-ms2.html)

**現在も成立する理由：** Cを共通配布基盤にする製品設計。  
**新規への一般化：** 強い。ただしRust実装＋C ABIの配布と比較する必要がある。

### C8. Nuklear — 単一ヘッダと利用者主導のmemory管理

C89、単一ヘッダ、低依存、利用者によるmemory制御が明示される。[Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)

Rustの`no_std`、固定buffer、arenaでも同様の実行時特性は実現可能と考えられる。Rust bindingも案内されるが、その品質はREADME自身が保証していない。

**現在も成立する理由：** 配布方式と既存利用環境。  
**新規への一般化：** single-header C組込みが要求なら成立。UIアルゴリズムやmemory制御がRustに不可能という証拠はない。

### C9. Clay — 近年のC library開発

単一`clay.h`、標準libraryへのlinkを含む低依存、static arena、`malloc`／`free`不要の設計を示す。Rust bindingも公式READMEから案内される。[Clay](https://github.com/nicbarker/clay)、[Rust binding](https://github.com/clay-ui-rs/clay)

**現在も成立する理由：** C／C++への直接組込みとarena契約。  
**新規への一般化：** 配布要求の実例。ただし「Rustを検討して棄却した」という採用判断は未確認。arena利用だけをRust不適格の根拠にしない。

### C10. Lua — 非局所的な制御移動を持つ埋込みruntime

Lua 5.4 manualは、C構成でのerror処理に`longjmp`を用い、protected callで回復点を設けることを説明する。Rustのdestructorを飛び越す非局所移動は、そのまま許容できるものではない。[Lua manual](https://www.lua.org/manual/5.4/manual.html#4.4)、[Rust Reference](https://doc.rust-lang.org/reference/behavior-considered-undefined.html)

しかし`mlua`はprotected callとpanic変換を備える。文書は、安全化のoverheadと内部unsafeの複雑性も明示する。[mlua安全性説明](https://docs.rs/crate/mlua/latest)

**現在も成立する理由：** 既存C callback／error契約との直接適合。  
**新規への一般化：** Lua extensionにも適用されるが、`mlua`で十分な場合を除外すべき。GC言語を実装するruntimeの事例であり、一般applicationにCを勧める証拠ではない。

### C11. Embassy — E仮説への反証

allocation不要のtask実行と、対応hardware向けHALを提供する。[Embassy](https://embassy.dev/)

**判定：** 対応hardwareと実行モデルが要求に合う新規firmwareでは、Rustで十分な有力候補。  
**限界：** cooperative taskの適合性、deadline、DMA、stack、全driver対応まで自動保証するわけではない。

### C12. Rustls／Rustls FFI — C ABI要求への反証

Rust実装TLSをC等から呼び出すlibraryと、C exampleが存在する。[Rustls FFI](https://github.com/rustls/rustls-ffi)

**判定：** 対応platformとTLS機能が合えばRustで十分。C ABIだけを理由にC実装を維持する必要はない。  
**限界：** crypto providerの対応範囲、build依存、ABI更新方針を確認する必要がある。全allocation失敗から回復するlibraryであることは、本調査では認定していない。

### C13. Android／Rust版Binder — 新規systems codeへの反証

Rust for Linuxは、Rust版BinderがLinux v6.18-rc1にmergeされたと説明する。Googleの2025-11-13経験報告も、AndroidでRust採用の効果を報告する。[Binder](https://rust-for-linux.com/android-binder-driver)、[Android報告](https://blog.google/security/rust-in-android-move-fast-fix-things/)

**判定：** 複雑なIPC、security、低水準処理を理由にCを必須とする仮説への強い反証。  
**限界：** Googleの数値は同社codebaseの観測であり、任意の小規模teamや全targetへの一般的な工数・性能保証ではない。mainline mergeと全製品でのC置換完了も区別する。

---

## D. 証拠マトリクス

### 判定記号

依頼の六区分を次のように用いる。

- **①** 原理的・技術的に不可能。
- **②** 標準APIでは不足するが、library／unsafe等で可能。
- **③** 実現可能だが、要求維持に追加制約・費用がある。
- **④** 主因が既存資産の移行費用。
- **⑤** team・歴史・検討未実施等。
- **⑥** 当該要求ではRustで十分。

一つのprojectに複数の理由があるため、判定は排他的ではない。**①にproject全体を分類できる事例は確認していない。** 必要targetが未対応なら現行構成で採用できないが、言語の原理的不可能性とは異なる。

証拠種別は、**一次要求**、**実装／API**、**経験報告**、**本調査の推論**を区別する。

### D.1 要求・実現性・追加負担

| Case | 誰のどの要求か | 証拠の強さ・種類 | Rustの実現性 | 追加負担／判定 |
|---|---|---|---|---|
| C1 SQLite | 組込み利用者へのOOM回復・低依存library | 強：一次要求＋test方針 | 可能。全体同等性は未確認 | fallible経路統一、品質保証の再構築。②③④ |
| C2 curl | hostが失敗時の終了方針を決め、既存ABIで使う | 強：maintainerの明示 | 可能。全target同等性は未確認 | 非abort契約、対応範囲、混在保守。③④⑤ |
| C3 Linux | 共通codeを必要architecture全体で使う | 強：公式対応表＋source | 対象により可能／現行構成で困難 | 専用抽象、binding、compiler機能。②③④ |
| C4 PostgreSQL | extensionがcontext・ERROR契約へ適合 | 強：会議記録＋pgrx実装 | pgrxで可能な範囲あり | FFI guard、lifetime、package保守。②③④ |
| C5 Zephyr | 必要board・SDK・APIで動作 | 強：公式対応文書 | 対象依存 | Rust module、toolchain、API対応。③ |
| C6 littlefs | bounded memory・電源断復旧 | 強：設計文書。C選択理由は弱 | C FFIで可能。純Rust同等性未確認 | bindingまたは再実装・再検証。②④ |
| C7 Sokol | 多言語／多projectへ容易に配る | 強：明示的C採用理由 | ABI提供・bindingで可能 | producer／consumer双方のbuildと配布。③ |
| C8 Nuklear | single-header、低依存、memory制御 | 強：製品契約。Rust棄却理由なし | 技術的に可能 | Cソース配布の再現条件。③⑤ |
| C9 Clay | C／C++直接組込み、static arena | 強：実装・製品契約。比較判断は未確認 | bindingで可能。純Rust版評価なし | 同じ配布方式を要求する場合の統合。③⑤ |
| C10 Lua | C callbackから既存error契約を利用 | 強：manual＋mlua実装説明 | wrapperで可能 | protected call、panic、資源境界。②③ |
| C11 Embassy | 対応MCUでheap不要の並行処理 | 強：実装／API | 十分な構成あり | HAL・実行モデル適合の確認。⑥ |
| C12 Rustls | TLS libraryをC等から利用 | 強：FFI実装・example | 十分な構成あり | provider・ABI・build条件。⑥、条件により③ |
| C13 Android | securityを要する新規systems code／IPC | 強：導入実績＋経験報告 | 実際に採用 | 組織固有の統合費用。⑥ |

### D.2 歴史・新規適用・NewLang・不確実性

| Case | 歴史的要因 | 新規codeでも成立するか | NewLangとの関連 | 結論を変える確認 |
|---|---|---|---|---|
| C1 | 大：検証・実装資産 | 復旧／配布要求は成立 | F候補。ただし同等testが必要 | 全依存込みRust fault injection |
| C2 | 大：ABI・team・長期保守 | 非abortとtarget条件は成立 | F・P候補 | 必要platform集合とRust構成の差分 |
| C3 | 大：共通API・全architecture | 共通moduleでは成立 | P・D候補。NewLangも対応障壁あり | 対象subsystem・targetを固定 |
| C4 | 大：host契約 | 新規extensionでも成立 | F・L境界の調査価値 | pgrx最新構成の通常経路・失敗経路 |
| C5 | 中：SDK・driver資産 | 成立 | P候補 | 実boardのbuild／機能確認 |
| C6 | 大：実装と永続化検証 | memory要求は成立、C必須性は弱 | 主対象とする根拠不足 | Rust側の同一復旧試験 |
| C7 | 中：利用者build環境 | 強く成立し得る | Cソース出力等を要する可能性 | Rust staticlibと利用者作業の比較 |
| C8 | 中：配布慣行 | source組込み必須なら成立 | P候補 | consumer制約の確認 |
| C9 | 長期legacyだけでは説明困難 | 配布契約として成立 | P候補。Rust棄却の証拠ではない | 作者の比較判断・実利用者の要件 |
| C10 | 大：C API・制御移動 | 新規bindingにも適用 | F・A調査。ただしNewLangも難しい | mluaで不足する具体的操作 |
| C11 | C資産を前提としない反例 | 成立 | 原則主要対象から除外 | 未対応device等の具体的欠落 |
| C12 | C実装を前提としない反例 | 成立 | 通常TLS用途は優先度低 | 全OOM回復等の追加要求 |
| C13 | 大規模組織の既存基盤あり | 新規Rust採用の反例 | kernel／IPC一般を対象化しない | 小team・別targetへの移転可能性 |

各行の出典はC1〜C13およびGの台帳で追跡できる。**追加負担が重大であるという定量判定は、いずれも今回実測していない。**

---

## E. 最も有望な未解決の問い

### E1. 全経路がfallibleな状態付きlibraryは、現在のRustでどこまで自然に構成できるか

**比較対象：** C、stable Rust＋既存library、必要ならnightlyを別枠で用意する。

小さなindex更新処理を選び、buffer拡張、node確保、文字列生成、失敗報告、rollback、teardownを含める。n回目のallocationから一度だけ失敗／以後継続失敗の両方を注入する。

**支持条件：** Rustで重要な操作を諦める、依存を広く置換する、専用collection群の保守が必要になる。  
**棄却条件：** 既存libraryと局所wrapperで、要求とfailure testを無理なく満たせる。

### E2. 外部所有・複数hookのlistは、既存Rust crateで十分か

固定poolのnodeが、LRU listと実行待ちlistへ別hookで所属するworkloadを使う。既知pointerからのdetach／move、走査削除、splice、replaceを含める。

**支持条件：** 要求するaddress、計算量、memory上限、操作順のいずれかが失われる。  
**棄却条件：** 借用pointer、`UnsafeRef`または局所wrapperで同一契約を維持できる。

### E3. callback再入の制約は、本質的要求か整理可能なAPIか

callbackが同じobjectを読む場合、別fieldを変える場合、同じlistを変える場合を分ける。

**支持条件：** callback延期やcommand化が許されず、既存Rust safe APIの契約と衝突する。  
**棄却条件：** 借用範囲の短縮等で、要求を損なわず整理できる。

### E4. Cソース配布を必要とする利用者は、何を拒否しているのか

追加compilerの導入、prebuilt binary、link依存、認証対象の変更、社内再build手順の変更を個別に確認する。

**支持条件：** 実利用者の受入条件としてCソースが必要で、Rust製artifactでは満たせない。  
**棄却条件：** 再現可能なprebuilt／staticlibの提供で受け入れられる。

### E5. allocator API改善後にも、F・Lの差は残るか

1.100予定の安定化範囲を、実際のrelease後に再確認する。fallible構築、allocation flags、context寿命、非移動allocationを個別に検証する。

**支持条件：** allocator指定が可能になっても、必要操作のfallibilityや資源返却契約が揃わない。  
**棄却条件：** 標準APIと既存libraryの改善で差が実用上消える。

---

## F. NewLangプロジェクトへの提言

以下は**追加研究候補**であり、Production変更、言語仕様追加、他Trackの起動を認可するものではない。

### 候補1：失敗をhostへ返す、組込み用index／cache library

| 項目 | 内容 |
|---|---|
| 誰の要求か | SQLite／libcurl型の組込みlibrary利用者。通常の資源不足でhost processを終了させたくない |
| Rustの実際の問題 | API単体より、依存・logging・clone・cleanupを含む全経路の契約統一 |
| 維持すべきCの自由 | allocator選択、通常の失敗返却、部分rollback、資源責任の返却、明示的teardown |
| NewLangの課題 | compilerが検査する範囲と、外部code・allocator・I/Oへの契約境界。言語機能だけで完結しない |
| 最小検証 | 挿入・更新・削除を持つ小さなindexへallocation fault injectionを行い、状態と資源を照合 |
| 撤退条件 | Rust既存構成が同等契約を局所的な対応で満たし、NewLang固有の解決価値が見つからない |

**優先度：最も高い。** 顧客要求が強く、比較実験も小さくできる。

### 候補2：固定pool上の外部所有nodeを使う複数list／index

| 項目 | 内容 |
|---|---|
| 誰の要求か | Linux／BSD型のqueue、cache、scheduler部品を扱う開発者 |
| Rustの実際の問題 | 既知pointerからの操作、所属証明、複数hook、callback、外部所有を同時に維持するwrapper契約 |
| 維持すべきCの自由 | 元address、allocationなしのlink操作、既知nodeのO(1)操作、独立した解放時点 |
| NewLangの課題 | 解放・再所属・alias・走査の整合性を、要求するmemory量と計算量のまま扱うこと |
| 最小検証 | LRU＋実行待ちの2 hook構成をCと`intrusive-collections`で同じ操作列に通す |
| 撤退条件 | Rust crate＋小さなwrapperで要求を満たす、またはNewLangも同等以上の人手契約を必要とする |

**優先度：中。** 「listを実装できる」だけでは成果にならない。差が出る組合せを特定する。

### 候補3：利用者のCビルドへ直接渡す、小型ソース配布library

| 項目 | 内容 |
|---|---|
| 誰の要求か | Sokol／Clay／Nuklear型の部品提供者と、その組込み先 |
| Rustの実際の問題 | C ABI提供は可能だが、利用者側のC-only再build契約とは一致しない |
| 維持すべきCの自由 | 利用者のcompiler・linker・設定方式で再buildできること、低依存、公開C interface |
| NewLangの課題 | Cソース出力を検討する場合も、意味保存、UB、診断、debug情報、toolchain差を検証する必要 |
| 最小検証 | 一つの小型部品についてRust staticlib配布とCソース配布のconsumer作業を比較 |
| 撤退条件 | consumerがRust製artifactを受け入れる、またはNewLangも別compiler必須で同じ障壁を残す |

**優先度：需要確認を先行。** C backendの開発を、この研究だけで正当化してはならない。

---

## G. 出典・証拠台帳

**全資料の確認日：2026-10-17。**  
「強」は、その資料が直接示す要求・機能・出来事についての評価であり、CとRustの総合的優劣を意味しない。「継続更新」は公開日・最終更新日を確認できないか、versionに依存する資料である。

### G.1 実プロジェクトの要求と採用判断

| ID | 出典・日付／対象 | 支える結論 | 強さと限界 |
|---|---|---|---|
| S01 | [SQLite Why C](https://www.sqlite.org/whyc.html)／2025-05-09更新 | C継続理由、互換性、OOM、品質方針 | 強：公式判断。現在のRust代替の網羅的検証ではない |
| S02 | [SQLite allocation](https://www.sqlite.org/malloc.html)／継続更新 | allocation失敗の回復契約、allocator選択 | 強：設計契約。Rustとの費用比較なし |
| S03 | [SQLite testing](https://sqlite.org/testing.html)／継続更新、一部統計は3.42.0時点 | 一回／継続OOM、複合失敗のtest方式 | 強：公開検証方針。本調査では未再実行 |
| S04 | [Writing C for curl](https://daniel.haxx.se/blog/2025/04/07/writing-c-for-curl/)／2025-04-07 | C89、非abort、API／ABI、Cの安全上の負担 | 強：maintainer説明。全主張を独立測定したものではない |
| S05 | [dropping hyper](https://daniel.haxx.se/blog/2024/12/21/dropping-hyper/)／2024-12-21、curl 8.12.0に向けた変更 | Rust併用の保守・需要問題 | 強：撤去理由。Rust技術的不可能性を支持しない |
| S06 | [Linux Rust architecture](https://docs.kernel.org/rust/arch-support.html)／確認時点 | C側との実用対応範囲差 | 強：公式対応表。将来変動する |
| S07 | [Rust kernel policy](https://rust-for-linux.com/rust-kernel-policy)／継続更新 | subsystemごとの導入・保守負担 | 中〜強：当事者の整理。kernel全体の不変の規則ではない |
| S08 | [PostgreSQL開発者会議](https://wiki.postgresql.org/wiki/FOSDEM/PGDay_2025_Developer_Meeting)／2025年 | longjmp、allocator、packagingへの具体的懸念 | 強：発言記録。現行の包括的不採用決議ではない |
| S09 | [pgrx memory context](https://github.com/pgcentralfoundation/pgrx/blob/develop/docs/src/pg-internal/memory-context.md)／develop | host管理memoryとRust lifetimeの摩擦 | 強：実装者説明。対象commit未固定 |
| S10 | [pgrx README](https://github.com/pgcentralfoundation/pgrx)／確認時点 | Rust extension、guard、resource管理の代替 | 強：公開構成。完全なsoundness証明ではない |
| S11 | [pgrx v0.18.0](https://github.com/pgcentralfoundation/pgrx/releases/tag/v0.18.0)／取得表示は4月17日、年は未確定 | longjmp境界、backtrace、build改善 | 強：release説明。2025年懸念の全解消を意味しない |
| S12 | [Zephyr 4.1](https://zephyrproject.org/zephyr-rtos-4-1-is-available/)／2025年 | Rust application支援導入 | 強：当時の公式状態。現在へ固定しない |
| S13 | [Zephyr Rust文書](https://docs.zephyrproject.org/latest/develop/languages/rust/index.html)・[platform一覧](https://github.com/zephyrproject-rtos/zephyr-lang-rust/blob/main/etc/platforms.txt)／確認時点 | Rust対応targetの限定 | 強：文書・設定。未掲載boardすべてが不可能とは断定しない |
| S14 | [littlefs README](https://github.com/littlefs-project/littlefs/blob/master/README.md)／master | bounded memory、power-loss契約 | 強：設計要求。C採用の比較理由ではない |
| S15 | [littlefs2](https://github.com/trussed-dev/littlefs2)／確認時点 | Rustからの利用、`no_std`構成 | 強：FFI代替。純Rust実装の証拠ではない |
| S16 | [Sokol README](https://github.com/floooh/sokol)／確認時点 | 明示的なC採用理由、Rust binding | 強：製品判断。Rust実装との工数比較なし |
| S17 | [Sokol compute更新](https://floooh.github.io/2025/05/19/sokol-gfx-compute-ms2.html)／2025-05-19、追記で5月24日merge | 近年の新機能開発、resource設計 | 強：具体的開発。Rust棄却の記録ではない |
| S18 | [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)／確認時点、C89 | single-header、低依存、memory制御 | 強：製品契約。Rust binding品質は保証されない |
| S19 | [Clay](https://github.com/nicbarker/clay)・[Rust binding](https://github.com/clay-ui-rs/clay)／確認時点 | arena、ソース組込み、言語併用 | 強：実装特性。比較採用理由は未確認 |
| S20 | [Lua manual](https://www.lua.org/manual/5.4/manual.html#4.4)／Lua 5.4 | error／yieldの非局所制御移動 | 強：公式契約。Rust利用不可を意味しない |
| S21 | [mlua安全性説明](https://docs.rs/crate/mlua/latest)／確認時点0.12.2 | protected call、panic変換、安全化負担 | 強：実装者の説明。内部bug不在の証明ではない |
| S22 | [Embassy](https://embassy.dev/)・[executor](https://docs.rs/embassy-executor/latest/embassy_executor/)／executor 0.10.0 | heap不要embedded構成の反証 | 強：API・実装。全hardware・deadlineへ一般化しない |
| S23 | [Rustls](https://docs.rs/rustls/latest/rustls/)・[Rustls FFI](https://github.com/rustls/rustls-ffi)／Rustls 0.23.46 | Rust libraryからC ABIを提供できる | 強：実装・example。全OOM回復は未確認 |
| S24 | [Rust版Binder](https://rust-for-linux.com/android-binder-driver)／v6.18-rc1導入を記載 | kernel IPCの実採用反証 | 強：導入事実。全製品の置換完了とは別 |
| S25 | [Android経験報告](https://blog.google/security/rust-in-android-move-fast-fix-things/)／2025-11-13 | 新規systems codeでのRust効果 | 強：当事者観測。CとC++を含み、任意projectへの因果一般化は不可 |

### G.2 Rust能力・契約の検証

| ID | 出典・version／日付 | 支える結論 | 強さと限界 |
|---|---|---|---|
| R01 | [stable std](https://doc.rust-lang.org/stable/std/index.html)／1.99.1、2026-10-13 | 調査時のstable基準 | 強：公式文書。以後の更新に注意 |
| R02 | [Vec](https://doc.rust-lang.org/std/vec/struct.Vec.html)・[slice](https://doc.rust-lang.org/std/primitive.slice.html#method.get) | `try_reserve`、panicしない入力処理 | 強：API契約。要素側のallocationまで保証しない |
| R03 | [Box](https://doc.rust-lang.org/stable/std/boxed/struct.Box.html#method.try_new)・[BTreeMap](https://doc.rust-lang.org/std/collections/struct.BTreeMap.html) | fallible標準APIの不足範囲 | 強：確認時点のAPI。言語全体の限界ではない |
| R04 | [PR #156882](https://github.com/rust-lang/rust/pull/156882)・[issue #32838](https://github.com/rust-lang/rust/issues/32838)／2026-09-23 merge | allocator安定化の進展と部分性 | 強：採択変更。全fallible APIの完成ではない |
| R05 | [allocator-api2 Box](https://docs.rs/allocator-api2/latest/allocator_api2/boxed/struct.Box.html#method.try_new)／0.4.0 | 外部fallible Box | 強：API。依存先の型互換性は別問題 |
| R06 | [fallible_collections](https://docs.rs/fallible_collections/latest/fallible_collections/)／0.5.2 | 外部fallible操作の存在 | 強：提供API。成熟度・全経路は未監査 |
| R07 | [heapless](https://docs.rs/heapless/latest/heapless/)／0.9.3 | 固定容量による代替 | 強：API。無制限増加要求には適合しない |
| R08 | [hashbrown](https://docs.rs/hashbrown/latest/hashbrown/struct.HashMap.html#method.try_reserve)／確認時点 | fallible reserve | 強：API。`try_insert`の失敗意味と区別 |
| R09 | [alloc](https://doc.rust-lang.org/std/alloc/fn.alloc.html)・[handle_alloc_error](https://doc.rust-lang.org/std/alloc/fn.handle_alloc_error.html) | 低水準代替と既定OOM処理 | 強：公式契約。allocator／OSの実挙動は別 |
| R10 | [catch_unwind](https://doc.rust-lang.org/std/panic/fn.catch_unwind.html) | abortを捕捉できない | 強：公式契約。Result設計の代替ではない |
| R11 | [Linux KBox](https://rust.docs.kernel.org/kernel/alloc/kbox/type.KBox.html) | flags付きfallible allocation | 強：専用実装。通常のstable stdとは区別 |
| R12 | [標準LinkedList](https://doc.rust-lang.org/stable/std/collections/struct.LinkedList.html) | 非intrusive、cursorの状態 | 強：API。外部crateに一般化しない |
| R13 | [intrusive-collections](https://docs.rs/intrusive-collections/latest/intrusive_collections/)／0.10.3 | 複数hook、借用pointer、cursor | 強：機能。production全体の適合を保証しない |
| R14 | [LinkedList](https://docs.rs/intrusive-collections/latest/intrusive_collections/linked_list/struct.LinkedList.html)・[CursorMut](https://docs.rs/intrusive-collections/latest/intrusive_collections/linked_list/struct.CursorMut.html) | remove、splice、replace、teardown | 強：APIとpanic契約。custom adapterは別評価 |
| R15 | [UnsafeRef](https://docs.rs/intrusive-collections/latest/intrusive_collections/struct.UnsafeRef.html) | reference countなしの外部責任 | 強：明示契約。人間のlifetime責任が残る |
| R16 | [intrusive source](https://docs.rs/intrusive-collections/latest/src/intrusive_collections/linked_list.rs.html) | 近傍pointer操作、内部unsafe、計算量判断 | 中〜強：source読解。benchmarkではない |
| R17 | [Linux list API](https://docs.kernel.org/core-api/list.html) | C側の自由と誤用条件 | 強：公式API。C全体の安全性を示さない |
| R18 | [ManuallyDrop](https://doc.rust-lang.org/std/mem/struct.ManuallyDrop.html)・[MaybeUninit](https://doc.rust-lang.org/std/mem/union.MaybeUninit.html)・[Pin](https://doc.rust-lang.org/std/pin/index.html) | 手動管理・部分初期化・非移動の代替 | 強：公式契約。任意の組合せがsafeとは限らない |
| R19 | [Rust UB Reference](https://doc.rust-lang.org/reference/behavior-considered-undefined.html)・[UnsafeCell](https://doc.rust-lang.org/std/cell/struct.UnsafeCell.html) | unsafe内にも残る契約 | 強：公式記述。ただし厳密なaliasing modelには未確定部分あり |
| R20 | [crossbeam-epoch](https://docs.rs/crossbeam-epoch/latest/crossbeam_epoch/)・[volatile](https://doc.rust-lang.org/std/ptr/fn.read_volatile.html) | 独自access規律の代替と限界 | 強：API。RCU／QSBR／DMA全般との同等性は示さない |
| R21 | [no_std](https://docs.rust-embedded.org/book/intro/no-std.html) | 大型runtime必須説への反証 | 強：基本能力。個別依存の適合性は別 |
| R22 | [platform support](https://doc.rust-lang.org/rustc/platform-support.html) | Tierの意味とtarget条件 | 強：公式分類。個別製品の採用可否は別 |
| R23 | [gccrs報告](https://rust-gcc.github.io/2026/09/08/2026-08-monthly-report.html)・[GCC backend](https://github.com/rust-lang/rustc_codegen_gcc)／2026年 | LLVM以外の経路と進展 | 強：開発状況。全target同等性を認定しない |
| R24 | [FFI](https://doc.rust-lang.org/nomicon/ffi.html)・[C表現](https://doc.rust-lang.org/reference/type-layout.html#the-c-representation) | C ABIと実装言語の分離 | 強：公式契約。既存headerを無条件にそのまま使えるとは限らない |
| R25 | [vendor](https://doc.rust-lang.org/cargo/commands/cargo-vendor.html)・[offline](https://doc.rust-lang.org/cargo/reference/config.html#netoffline) | offline／依存固定の代替 | 強：機能。保守・監査費用ゼロとはいえない |
| R26 | [Ferrocene target](https://public-docs.ferrocene.dev/main/user-manual/targets/index.html)・[core subset](https://public-docs.ferrocene.dev/main/safety-manual/core/subset.html) | 認証分野でもRustの選択肢が存在 | 強：提供範囲。任意構成の認証ではない |
| R27 | [Rust for Linux unstable](https://rust-for-linux.com/unstable-features)・[version policy](https://rust-for-linux.com/rust-version-policy) | kernel構成と通常stableの区別 | 強：当事者方針。将来変動する |
| R28 | [PostgreSQL OS資源](https://www.postgresql.org/docs/18/kernel-resources.html)／PostgreSQL 18 | allocation回復とOS強制終了の区別 | 強：運用条件。全環境の故障モデルではない |

### 研究として残すべき否定的結果

本調査は、以下をNewLangの主要価値として採用する根拠を得なかった。

- Rustではintrusive listを作れない。
- Rustでは手動解放や非RAII方式を使えない。
- Rustには大きなruntimeが必須である。
- C ABIを提供するにはC実装が必要である。
- `unsafe`が必要ならRustは実用的代替にならない。
- Cで継続開発されているprojectは、Rustが不適格だと判断したprojectである。

追加投資の前に必要なのは、**実利用者が維持を求める契約を一つ固定し、最善の既存Rust構成でも何が残るかを比較実装で確かめること**である。