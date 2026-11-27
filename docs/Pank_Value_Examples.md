# Pank Value Examples

数値を使う条件と、元のノードを解放しても残るノードを読める条件を、採用済みの小さな例でたどります。どちらも、コンパイラが確かめる前提と、プログラマーが決める方針を持ちます。初版の参照点は [main `37e7a5f9`](https://github.com/wakairo/NewLang_Compiler/commit/37e7a5f9a4970e8e353f9d4b35a741ad5be1613f) と、そこで [CURRENT_SPEC](https://github.com/wakairo/NewLang_Compiler/blob/37e7a5f9a4970e8e353f9d4b35a741ad5be1613f/docs/reference/CURRENT_SPEC.md) が指す Draft 17.39 です。適用範囲と証拠の一覧は [Pank Value Ledger](Pank_Value_Ledger.md) を参照してください。

## 大きすぎる件数をどう扱うか

1 件 64 バイトの領域に必要な大きさを求めるとします。採用済みのテストでは、作者が 1024 件未満で積を取り、それ以外は 0 を返すと決めています。次は正例の関数部分です。 [正例の全ソース](https://github.com/wakairo/NewLang_Compiler/blob/37e7a5f9a4970e8e353f9d4b35a741ad5be1613f/tests/p1699/source/unit-product.nl)

~~~text
fn stripe_bytes(count: usize) -> usize {
    if (count < usize(1024)) {
        count * usize(64)
    } else {
        usize(0)
    }
}
~~~

`usize` は負にならない数量型です。Draft では積がその型に収まることを要求します。この例で CHECK が見るのは、実際の真側で比較と積が同じ仮引数を指し、その真側の最大値 1023 に 64 を掛けても収まることです。テストの unit `main` は、コンパイラが単一の定数として扱わない名前付き引数を渡します。生成 C17 の観測では 1023 件で 65472、1024 件で fallback の 0 でした。64 ビットの負例は `count < 3` の真側で `usize` 最大値を掛けます。2 が真側に入るため最大値を超え、CHECK で拒否されます。別 binding をガードする例も CHECK で拒否されます。scalar `main` の積の例は CHECK を通った後の C17 出力制限に当たり、事前条件の違反を示す例とは区別します。 [Draft §5.2](reference/NewLang_v0_spec_Draft17_39.md#52-ordinary-scalar--quantity-arithmetic) · [#1702 採用レビュー](https://github.com/wakairo/NewLang_Compiler/pull/1702#pullrequestreview-6019430398) · [上限が広すぎる負例](https://github.com/wakairo/NewLang_Compiler/blob/37e7a5f9a4970e8e353f9d4b35a741ad5be1613f/tests/p1699/source/too-wide.nl) · [対象入力と実測](https://github.com/wakairo/NewLang_Compiler/issues/1699#issuecomment-7096158674)

固定サイズのまとまりに分ける算術では、74 を 8 で割るテストで商 9、余り 2 が生成 C の実行から観測されました。 数値の観測には、生成 C のコピーに値の出力を加えた試験を使いました。`input` が 46 の `input / usize(0)` は `USIZE-DIVISION-ZERO` で CHECK が止まります。`%` の非ゼロ条件も Draft にあり、ここでの直接のゼロ負例は `/` です。名前付きの既知非ゼロ除数が #1577 のソース段階で拒否されたのは、リテラルに限った実装のためです。#1577 の独立試験では観測器が数値取得まで到達しておらず、数値は実装側の測定です。 [正例の全ソース](https://github.com/wakairo/NewLang_Compiler/blob/37e7a5f9a4970e8e353f9d4b35a741ad5be1613f/tests/p1572/source/turns-tail.nl) · [ゼロ除算の負例](https://github.com/wakairo/NewLang_Compiler/blob/37e7a5f9a4970e8e353f9d4b35a741ad5be1613f/tests/p1572/source/divisor-zero.nl) · [正負例と実測](https://github.com/wakairo/NewLang_Compiler/issues/1572#issuecomment-7036439214) · [#1577 採用レビュー](https://github.com/wakairo/NewLang_Compiler/pull/1577#issuecomment-7054023049)

0 はこの例で作者が選んだ応答で、0 件の結果とも重なります。呼び出し側が両者を区別する必要があれば、失敗を別の値で返す契約を選べます。Draft は、予想される失敗に `Result` や `Option` を用意しています。同じ契約の C も、除数を検査してから割り、範囲内でのみ積を計算し、同じ失敗値を返せます。ここで確かめた Pank の利益は、試した形の演算を許せるかコンパイラが実行前に判断することです。任意の動的な除数や外部入力を受ける公開 API、可用性や DoS への効果は、この例での測定範囲外です。 [Draft §8](reference/NewLang_v0_spec_Draft17_39.md#8-precondition--unchecked--assert) · [価値と比較の位置付け](https://github.com/wakairo/NewLang_Compiler/issues/1853)

## 外した末尾を解放してから、新しい末尾を読む

固定例のリストには A、B、C があり、A の `prev` が末尾キャッシュです。元の allocation はこの 3 ノードと 2 個のヘッダの計 5 個です。最後の 3 回のリンク更新は `B.next` を空にし、`A.prev` を B に更新し、`C.prev` を空にする順です。更新に使った借用を閉じ、メンバーから外した C を元のストレージ、Domain、Allocation で一度だけ解放します。その後 A 自身の Domain を使い、現在の `A.prev` を読みます。正例の全ソースと、同じ形を再確認した記録があります。 [採用例の全ソース](https://github.com/wakairo/NewLang_Compiler/blob/37e7a5f9a4970e8e353f9d4b35a741ad5be1613f/tests/p1393/sources/P1-C-free-current-tail-read-v2.nl) · [Draft §3.2b.7](reference/NewLang_v0_spec_Draft17_39.md#32b7-draft-1737-c774--c785--one-postfree-cached-tail-read-continuation) · [#1837 対照実測](https://github.com/wakairo/NewLang_Compiler/issues/1837#issuecomment-7186481382)

- 実際の `A.prev` の Copy を match し、`Some` 側ではその中のポインタを取り出します。そのポインタ値だけでは、B の生存も所有権も確かめられません。B 自身の Domain で元の B に対する借用を確認できて初めて、現在の `B.next` を読めます。`None` 側に 2 回目の読取りはありません。
- その後、残る 4 個を元の権限で解放します。成功時の 5 回の解放と、割り当て失敗箇所ごとの 0〜4 回の解放を native で観測しました。古い C ポインタでの再借用、誤った Domain、B の解放欠落は、それぞれ通常 CHECK で `P3-STALE-POINTER`、`ALLOCATED-DOMAIN-MISMATCH`、`P5-SCOPE-OBLIGATION` でした。 [採用 PR #1396](https://github.com/wakairo/NewLang_Compiler/pull/1396) · [#1837 の正負実測](https://github.com/wakairo/NewLang_Compiler/issues/1837#issuecomment-7186481382)

作者が決めるのは、どのノードがどのリストに属し、どの経路でキャッシュを更新し、いつ元のノードを解放できるかです。まだ生きている C を古い末尾として返すことも、このリストでは誤りです。リストの所属規則は寿命検査に加えて必要です。外した C を生かす必要がある用途について、#1837 は同じアドレスを保つ private C の正例も測定しています。C の公開 API なら所属を確認するガードやハンドルで内部リンクを隠す設計があり、Rust の intrusive list でも同じノードを保持する選択肢があります。元の allocator や所有契約、追加の格納領域、失敗時の処理を揃えて比較する必要があります。 [V の比較](https://github.com/wakairo/NewLang_Compiler/issues/1836#issuecomment-7187458493)

C を生かしたまま読む Pank の固定形は Draft 17.39 §3.2b.8 に入り、#1844 で直接版が採用されました。この例では、実際の `A.prev` から元 B を借り直して読む間も C を元の所有者の下で生かし、読取りと match を閉じてから C を含む元の allocation を解放します。 [Draft §3.2b.8](reference/NewLang_v0_spec_Draft17_39.md#32b8-one-cached-tail-read-while-detached-original-c-remains-live) · [実装 #1844](https://github.com/wakairo/NewLang_Compiler/issues/1844) · [Draft §20.4](reference/NewLang_v0_spec_Draft17_39.md#204-ordinary-safe-api-の責任) · [直接版の採用記録](https://github.com/wakairo/NewLang_Compiler/issues/1844#issuecomment-7193084945)

Draft 17.40 §3.2b.9 では、構築と切断で同じ writer を呼ぶ限定ソースが選定されました。実装検証や作者・呼び出し側の比較が必要な範囲は台帳を参照してください。 [Draft 17.40 §3.2b.9](https://github.com/wakairo/NewLang_Compiler/blob/0d11fcc2f62f778faaf22622f933170163dcf1f8/docs/reference/NewLang_v0_spec_Draft17_40.md#32b9-one-shared-writer-for-the-two-retained-live-cache-changes) · [現在の比較範囲](Pank_Value_Ledger.md#元の寿命と解放責任を保つ)

## 同じバイト数で由来の異なる 0 を報告する

4,096 バイトの固定例では、`tag` を 93 から一時値 39 に替え、最初の交換が返した 93 で復元します。隣の `batching` は両方で 0 のままです。前段の判定は両方で真で、この 0 を履歴として選びます。後段の比較のしきい値だけを変えると、報告は次のようになります。 [履歴の 0 のソース](https://github.com/wakairo/NewLang_Compiler/blob/6179fae07da27db6712a962b9974ca12a4f2dbd4/tests/p1738/source/report-status-sample-zero.nl#L19-L39) · [既定値の 0 のソース](https://github.com/wakairo/NewLang_Compiler/blob/6179fae07da27db6712a962b9974ca12a4f2dbd4/tests/p1738/source/report-status-default-zero.nl#L19-L39)

| 後段の比較 | 実測した報告値と履歴採用の Bool |
| --- | --- |
| `4064 < bytes` | `(0, true)` |
| `4100 < bytes` | `(0, false)` |

2 つの 0 は、それぞれ履歴を採用した値と、設定した既定値です。生成 C17 と同じ契約の C 参照実装で、この差を観測しました。 [報告の観測と C 比較](https://github.com/wakairo/NewLang_Compiler/issues/1841#issuecomment-7187750635)
