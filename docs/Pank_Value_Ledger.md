# Pank Value Ledger

現時点で Pank の利用者が得られる利益を、確かめた入力と適用範囲から引ける台帳です。元のデータ配置や allocation・寿命、失敗時の方針を選ぶ自由を保ち、機械的に確かめられる前提をコンパイラに任せるという製品方針に基づきます。利用者に伝えられる利益、適用範囲、反例が変わったときに更新します。実際のコードと処理の流れは [Pank Value Examples](Pank_Value_Examples.md) を参照してください。 [Value Strategy §§1.1–1.4](NewLang_Product_Value_Strategy.md#11-ub-prevention-audit-locality-value-hypothesis)

初版の参照点は [main `37e7a5f9`](https://github.com/wakairo/NewLang_Compiler/commit/37e7a5f9a4970e8e353f9d4b35a741ad5be1613f) です。この時点の正本は [CURRENT_SPEC](https://github.com/wakairo/NewLang_Compiler/blob/37e7a5f9a4970e8e353f9d4b35a741ad5be1613f/docs/reference/CURRENT_SPEC.md) が指す Draft 17.39 です。各実測の対象リビジョンと試験ログはリンク先にあります。C を置き換えられる範囲は [Coverage Map](NewLang_C_Replacement_Coverage_Map.md)、設計判断の履歴は [Design Intent Ledger](NewLang_Design_Intent_Ledger.md) を参照してください。

## 数値演算の事前条件をコンパイル時に確認する

Draft では、通常の `usize` の `+`、`-`、`*` は数学的結果が表現範囲内であること、`/` と `%` は除数が非ゼロであることを要求します。通常の演算では、事前条件をコンパイラが確認できなければコンパイルエラーです。 [Draft §§5.2–5.6](reference/NewLang_v0_spec_Draft17_39.md?plain=1#L3819) · [§8](reference/NewLang_v0_spec_Draft17_39.md?plain=1#L4045)

| 利用者が得られる利益 | 採用済みの根拠と適用範囲 |
| --- | --- |
| **固定の除数の書き間違いに実行前に気づける。** | 通常の unit `main` で `usize` の 74 を型付きリテラルの 8 で割る例は CHECK、C17 出力、native を通り、商 9 と余り 2 を観測しました。 数値の観測には、生成 C のコピーに値の出力を加えた試験を使いました。`input` が 46 の `input / usize(0)` は CHECK で `USIZE-DIVISION-ZERO`。この公開記録で直接確認したゼロ負例は `/` です。`%` の確認範囲は非ゼロ正例と Draft の規則までです。 [#1577 の採用レビュー](https://github.com/wakairo/NewLang_Compiler/pull/1577#issuecomment-7054023049) · [正負例と実測](https://github.com/wakairo/NewLang_Compiler/issues/1572#issuecomment-7036439214) |
| **積を使う分岐の上限が広すぎると気づける。** | unit `main` から呼ぶ限定形で、同じ `usize` 仮引数について `count < 1024` の真側で `count * 64` を計算します。CHECK は真側の最大値 1023 と倍率の積が収まることを検査し、C17/native では 1023 → 65472、1024 → fallback の 0 を観測しました。64 ビットで `count < 3`、倍率 `usize` 最大値の例と、別 binding を比較する例は CHECK の `USIZE-MULTIPLICATION-PRECISION` で拒否されました。 [#1702 の採用レビュー](https://github.com/wakairo/NewLang_Compiler/pull/1702#pullrequestreview-6019430398) · [正負例と実測](https://github.com/wakairo/NewLang_Compiler/issues/1699#issuecomment-7096158674) |

同じ #1577 の加減算は正例で観測しています。この台帳で対応する拒否例を提示できるのは、上の除算と限定形の積です。変換などの他の数値前提の検証は別途必要です。名前付きの既知非ゼロ除数は #1577 の選択形から外れ、ソース段階で拒否されました。#1702 の scalar `main` の積は CHECK を通って C17 出力で拒否されました。いずれも事前条件の違反を示す負例とは区別します。#1577 の数値観測は実装側の試験に基づき、独立試験では観測器が数値取得まで到達していません。 [除数の分類](https://github.com/wakairo/NewLang_Compiler/issues/1572#issuecomment-7036251442) · [#1577 の採用レビュー](https://github.com/wakairo/NewLang_Compiler/pull/1577#issuecomment-7054023049) · [#1702 の実測](https://github.com/wakairo/NewLang_Compiler/issues/1699#issuecomment-7096158674)

利用者が決めるのは、許す範囲と範囲外への応答です。同じ契約の C でも非ゼロや表現範囲をガードで確かめ、同じ応答を返せます。Pank の確認済みの範囲では、演算に必要な事前条件の確認をコンパイラに任せられます。任意の動的な除数や外部入力を取る公開 API への一般化、同じ契約での作者・呼び出し側の負担は未確認です。サービスの可用性や DoS 耐性には入力サイズ、計算量、資源制限などの別の条件があります。 [Draft §8.3](reference/NewLang_v0_spec_Draft17_39.md?plain=1#L4127) · [価値、比較、適用範囲](https://github.com/wakairo/NewLang_Compiler/issues/1853#issuecomment-7190352469)

## 元の寿命と解放責任を保つ

**元のアドレスと解放方針を保ったまま、終了済みのノードの再利用や解放漏れに実行前に気づける。** 採用済みの固定例は 5 個の独立した元 allocation を使い、末尾 C を外して元の権限で解放し、実際の現在の末尾キャッシュ `A.prev` から B を読みます。CHECK とソース由来の所有・借用検証は元の root・Domain、現在値、借用、解放責任を確認します。strict C17/native の観測では成功時に元アドレス 5 個、リンク更新 9 回、C 解放後に A・B の 2 読取り、元の解放 5 回を確認しました。割り当て失敗 5 箇所では作れた 0〜4 個だけを解放しました。古い C ポインタの再利用、B に対する誤った Domain、B の解放欠落はそれぞれ通常 CHECK で拒否されました。 [Draft §3.2b.7](reference/NewLang_v0_spec_Draft17_39.md?plain=1#L2928) · [採用 PR #1396](https://github.com/wakairo/NewLang_Compiler/pull/1396) · [#1837 の対照実測](https://github.com/wakairo/NewLang_Compiler/issues/1837#issuecomment-7186481382)

作者は所属関係、キャッシュを更新できる経路、失敗・解放・再入の順序を設計します。同じ早期解放契約の C も、外した C を一度解放してから A・B を読めます。C では private な更新関数に規則を集めたり、ガード付きの API で内部リンクを隠したりできます。#1837 は、外した C を観測時も生かす同じ寿命契約で、private C の直接版と cache 書込み共通化版が、固定の成功・割り当て失敗の各条件で同じ観測結果を満たすことを確認しました。Draft 17.39 §3.2b.8 の Pank の固定直接版は #1844 で採用されました。通常 CHECK とソース由来の所有・借用検証は、構築と切断での 2 回の実 `A.prev` 書込みの権限と実際の新旧値を確かめ、strict C17/native では現在の cache を経由した元 B の読取りと各元 allocation の解放を観測しました。比較の対象は、引数なしの Pank の `main` と、更新・読取りを内部に閉じた private C です。任意のリスト所属判定や表現を隠す公開 safe API、同じ契約での作者と呼び出し側の再監査負担、利用実績は未確認です。 [V の C・Rust 比較](https://github.com/wakairo/NewLang_Compiler/issues/1836#issuecomment-7187458493) · [Draft §3.2b.8](reference/NewLang_v0_spec_Draft17_39.md?plain=1#L2994) · [実装 #1844](https://github.com/wakairo/NewLang_Compiler/issues/1844) · [直接版の採用記録](https://github.com/wakairo/NewLang_Compiler/issues/1844#issuecomment-7193084945) · [直接版と同契約の C 実測・監査範囲](https://github.com/wakairo/NewLang_Compiler/issues/1837#issuecomment-7193552601)

#1865 は、構築と切断にある 2 つの `A.prev` 書込みで、元の借用と書込み権限の取得を各呼び出し箇所に残し、1 つの writer を直接呼ぶ限定ソースを選定しました。返る実旧値は構築時の `None` と切断時の `Some(ptr_C)` です。この限定ソースのコンパイラ実装は #1867 で採用され、実 `main` `d953b861c132d1ebb79eaa2950e926e891daf94a` に入りました。 [Draft 17.40 §3.2b.9](https://github.com/wakairo/NewLang_Compiler/blob/0d11fcc2f62f778faaf22622f933170163dcf1f8/docs/reference/NewLang_v0_spec_Draft17_40.md?plain=1#L3025) · [#1865 選定記録](https://github.com/wakairo/NewLang_Compiler/issues/1860#issuecomment-7196342761) · [共通 writer の採用記録](https://github.com/wakairo/NewLang_Compiler/issues/1863#issuecomment-7207269087)

#1837 の private C 実旧値対照では、代入直前に実 `head->prev` を読む直接版と、同じ旧値を返す共通 writer 版を、GCC の strict C17（O0/O2）で成功と割り当て失敗 1〜5 の全 6 条件に照合しました。両版は構築時に `NULL`、切断時に生存中の元 C のアドレスを確認し、現在の末尾 B、元のリンクと解放順を保ちました。共通化そのものによる追加の再監査削減は未測定です。リストの所属規則は作者が定めます。表現を隠す公開 ordinary-safe API と呼び出し側の効果は未検証です。 [C の実旧値対照](https://github.com/wakairo/NewLang_Compiler/issues/1837#issuecomment-7206856674)

採用済みの直接版と共通版で、構築・切断時の `A.prev` 書込みと、現在の `A.prev` の実値からたどる `B.next` の読取りを確認しました。それぞれ元 A・B 自身の Domain による借用が必要で、生存中の別の元 C の Domain を渡す負例は参照取得時の CHECK で拒否されました。同じ契約の private C では、実際の書込み先と読取り先が生存中の元 A・B であり、アクセス可能であることを作者が両経路で確かめます。 [書込みの実測](https://github.com/wakairo/NewLang_Compiler/issues/1869#issuecomment-7207448417) · [V 書込み比較](https://github.com/wakairo/NewLang_Compiler/issues/1837#issuecomment-7207547846) · [V 読取り比較](https://github.com/wakairo/NewLang_Compiler/issues/1837#issuecomment-7207669003)

## 一時変更を復元し、報告値の由来を残す

**一時変更から元の値に戻し、報告した値が履歴と既定値のどちらから来たかを残せる。** #1848 で採用された固定 unit `main` では、同じ writer で同じ元フィールドを 2 回更新し、それぞれの書込み借用を閉じます。検証では、1 回目の交換が実際に返した旧値で元フィールドを復元し、隣のフィールドとの対応を保つことを確かめます。報告の Bool は実際に計算したバイト数を使う後段の比較からコピーします。前段の判定に差し替えた対照は CHECK を通り、C17 出力で `V1-BACKEND-UNSUPPORTED` になります。この拒否が採用済みの出力形の境界を示します。 [採用 #1848](https://github.com/wakairo/NewLang_Compiler/pull/1848) · [実 main の監査](https://github.com/wakairo/NewLang_Compiler/issues/1841#issuecomment-7192179857) · [レビュー済みの V 記録](https://github.com/wakairo/NewLang_Compiler/issues/1853#issuecomment-7192270555)

作者は変更するフィールドと履歴にする値、更新や失敗時の方針を決めます。呼び出し側は報告条件と既定値、Bool を受けた後の処理を決めます。同じ契約の C も単一の元レコードを扱う固定例で復元と報告を実現しました。公開 safe API の強制と、作者・呼び出し側の再監査範囲や工数の差は別途検証が必要です。 [同契約の C 実測](https://github.com/wakairo/NewLang_Compiler/issues/1841#issuecomment-7187750635) · [同じバイト数で由来の異なる 0 を報告する例](Pank_Value_Examples.md#同じバイト数で由来の異なる-0-を報告する)

[Product GO #624](https://github.com/wakairo/NewLang_Compiler/issues/624) は PAUSED、[H1 #268](https://github.com/wakairo/NewLang_Compiler/issues/268) は HOLD です。
