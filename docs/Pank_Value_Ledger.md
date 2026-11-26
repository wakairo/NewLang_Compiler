# Pank Value Ledger

現時点で Pank の利用者が得られる利益を、確かめた入力と適用範囲から引ける台帳です。元のデータ配置や allocation・寿命、失敗時の方針を選ぶ自由を保ち、機械的に確かめられる前提をコンパイラに任せるという製品方針に基づきます。利用者に伝えられる利益、適用範囲、反例が変わったときに更新します。実際のコードと処理の流れは [Pank Value Examples](Pank_Value_Examples.md) を参照してください。 [Value Strategy §§1.1–1.4](NewLang_Product_Value_Strategy.md#11-ub-prevention-audit-locality-value-hypothesis)

初版の参照点は [main `37e7a5f9`](https://github.com/wakairo/NewLang_Compiler/commit/37e7a5f9a4970e8e353f9d4b35a741ad5be1613f) です。この時点の正本は [CURRENT_SPEC](https://github.com/wakairo/NewLang_Compiler/blob/37e7a5f9a4970e8e353f9d4b35a741ad5be1613f/docs/reference/CURRENT_SPEC.md) が指す Draft 17.39 です。各実測の対象リビジョンと試験ログはリンク先にあります。C を置き換えられる範囲は [Coverage Map](NewLang_C_Replacement_Coverage_Map.md)、設計判断の履歴は [Design Intent Ledger](NewLang_Design_Intent_Ledger.md) を参照してください。

## 数値演算の事前条件をコンパイル時に確認する

Draft では、通常の `usize` の `+`、`-`、`*` は数学的結果が表現範囲内であること、`/` と `%` は除数が非ゼロであることを要求します。通常の演算では、事前条件をコンパイラが確認できなければコンパイルエラーです。 [Draft §§5.2–5.6](reference/NewLang_v0_spec_Draft17_39.md#52-ordinary-scalar--quantity-arithmetic) · [§8](reference/NewLang_v0_spec_Draft17_39.md#8-precondition--unchecked--assert)

| 利用者が得られる利益 | 採用済みの根拠と適用範囲 |
| --- | --- |
| **固定の除数の書き間違いに実行前に気づける。** | 通常の unit `main` で `usize` の 74 を型付きリテラルの 8 で割る例は CHECK、C17 出力、native を通り、商 9 と余り 2 を観測しました。 数値の観測には、生成 C のコピーに値の出力を加えた試験を使いました。`input` が 46 の `input / usize(0)` は CHECK で `USIZE-DIVISION-ZERO`。この公開記録で直接確認したゼロ負例は `/` です。`%` の確認範囲は非ゼロ正例と Draft の規則までです。 [#1577 の採用レビュー](https://github.com/wakairo/NewLang_Compiler/pull/1577#issuecomment-7054023049) · [正負例と実測](https://github.com/wakairo/NewLang_Compiler/issues/1572#issuecomment-7036439214) |
| **積を使う分岐の上限が広すぎると気づける。** | unit `main` から呼ぶ限定形で、同じ `usize` 仮引数について `count < 1024` の真側で `count * 64` を計算します。CHECK は真側の最大値 1023 と倍率の積が収まることを検査し、C17/native では 1023 → 65472、1024 → fallback の 0 を観測しました。64 ビットで `count < 3`、倍率 `usize` 最大値の例と、別 binding を比較する例は CHECK の `USIZE-MULTIPLICATION-PRECISION` で拒否されました。 [#1702 の採用レビュー](https://github.com/wakairo/NewLang_Compiler/pull/1702#pullrequestreview-6019430398) · [正負例と実測](https://github.com/wakairo/NewLang_Compiler/issues/1699#issuecomment-7096158674) |

同じ #1577 の加減算は正例で観測しています。この台帳で対応する拒否例を提示できるのは、上の除算と限定形の積です。変換などの他の数値前提の検証は別途必要です。名前付きの既知非ゼロ除数は #1577 の選択形から外れ、ソース段階で拒否されました。#1702 の scalar `main` の積は CHECK を通って C17 出力で拒否されました。いずれも事前条件の違反を示す負例とは区別します。#1577 の数値観測は実装側の試験に基づき、独立試験では観測器が数値取得まで到達していません。 [除数の分類](https://github.com/wakairo/NewLang_Compiler/issues/1572#issuecomment-7036251442) · [#1577 の採用レビュー](https://github.com/wakairo/NewLang_Compiler/pull/1577#issuecomment-7054023049) · [#1702 の実測](https://github.com/wakairo/NewLang_Compiler/issues/1699#issuecomment-7096158674)

利用者が決めるのは、許す範囲と範囲外への応答です。同じ契約の C でも非ゼロや表現範囲をガードで確かめ、同じ応答を返せます。Pank の確認済みの範囲では、演算に必要な事前条件の確認をコンパイラに任せられます。任意の動的な除数や外部入力を取る公開 API への一般化、同じ契約での作者・呼び出し側の負担は未確認です。サービスの可用性や DoS 耐性には入力サイズ、計算量、資源制限などの別の条件があります。 [Draft §8.3](reference/NewLang_v0_spec_Draft17_39.md#83-total-checked-api) · [価値、比較、適用範囲](https://github.com/wakairo/NewLang_Compiler/issues/1853#issuecomment-7190352469)

## 元の寿命と解放責任を保つ

**元のアドレスと解放方針を保ったまま、終了済みのノードの再利用や解放漏れに実行前に気づける。** 採用済みの固定例は 5 個の独立した元 allocation を使い、末尾 C を外して元の権限で解放し、実際の現在の末尾キャッシュ `A.prev` から B を読みます。CHECK とソース由来の所有・借用検証は元の root・Domain、現在値、借用、解放責任を確認します。strict C17/native の観測では成功時に元アドレス 5 個、リンク更新 9 回、C 解放後に A・B の 2 読取り、元の解放 5 回を確認しました。割り当て失敗 5 箇所では作れた 0〜4 個だけを解放しました。古い C ポインタの再利用、B に対する誤った Domain、B の解放欠落はそれぞれ通常 CHECK で拒否されました。 [Draft §3.2b.7](reference/NewLang_v0_spec_Draft17_39.md#32b7-draft-1737-c774--c785--one-postfree-cached-tail-read-continuation) · [採用 PR #1396](https://github.com/wakairo/NewLang_Compiler/pull/1396) · [#1837 の対照実測](https://github.com/wakairo/NewLang_Compiler/issues/1837#issuecomment-7186481382)

作者は所属関係、キャッシュを更新できる経路、失敗・解放・再入の順序を設計します。同じ早期解放契約の C も、外した C を一度解放してから A・B を読めます。C では private な更新関数に規則を集めたり、ガード付きの API で内部リンクを隠したりできます。#1837 で実測した private C は、外した C を読取り時に生かす契約です。Draft 17.39 §3.2b.8 は生かしたまま読む固定形を選定済みで、初版時点の通常コンパイラの実装・native 検証は #1844 の対象です。任意のリスト所属判定や表現を隠す公開 safe API、同じ契約での作者と呼び出し側の再監査負担、利用実績は未確認です。 [V の C・Rust 比較](https://github.com/wakairo/NewLang_Compiler/issues/1836#issuecomment-7187458493) · [Draft §3.2b.8](reference/NewLang_v0_spec_Draft17_39.md#32b8-one-cached-tail-read-while-detached-original-c-remains-live) · [実装 #1844](https://github.com/wakairo/NewLang_Compiler/issues/1844)

[Product GO #624](https://github.com/wakairo/NewLang_Compiler/issues/624) は PAUSED、[H1 #268](https://github.com/wakairo/NewLang_Compiler/issues/268) は HOLD です。
