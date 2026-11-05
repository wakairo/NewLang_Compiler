Track: C → fresh R

# C248 PREFREEZE RELEASE — ordinary usize Copy at three phase/helper boundaries

## 1. 目的と認可する段階

次のP1188候補で必要となる独立検証の待ち時間を減らすため、**canonicalに基づく独自source／数値期待値／予測する最初の到達層を事前freeze**する。現在実行中のR1185とは異なる次候補の準備であり、R1185の代行・再実行ではない。

今回はpure text／静的検討／hash／bundle／Issue記録だけを認可。**compiler/build/native/新AST生成、local意味論実行、branch/ref/push/PR/Actions/CIはすべて0。** 新Pを起動しない。Issue作成やRELEASEとactual STARTは区別し、担当がSTARTとfreeze完了を記録する。

## 2. Authorityと独立性

最初にmainの[Project Compass](https://github.com/wakairo/NewLang_Compiler/blob/main/docs/NewLang_Project_Compass.md)から関連正本を探し、Value Strategy §1.1、Development Process §§4.1.2/4.3.1/7、Review Guidelines、CURRENT_SPEC指定のcanonical **Draft17.34**を確認する。現採用mainは **63658cd546be8b99599baf19476486a475adb320 / tree a376e3c6e111f7283daa8be82d766dc0282c16b2**。これは採用済みauthorityの参照であり、未公開次候補を検証した結果ではない。関連するordinary binding/Copy、call/formal/normal result、IF、loop/Continue/Break、current/Changeの仕様を参照する。

初期入力はこのIssue、canonicalと必要最小限の関連正本文書だけ。**P1188/P1184のコメント・local報告・実装・tests/fixtures/source/oracle、Fの入力/結論、R1185や旧Rのsource/oracle/runner/観測器/報告は読まない。** 広いIssue検索、agent全一覧、他agentディレクトリの走査をしない。露出があれば対象・内容・freezeとの時系列を正直にCへ返す。

数値、宣言順、名前、ケース、期待値はR自身がcanonicalから選ぶ。Cがここで伝える有限profileはcoverage境界であり、数値oracleではない。将来の独立性表現は実際の露出履歴に合わせる。

## 3. 有限source能力

基礎profileはunit main内のordinary two-u8 original T、fixed selected field＋disjoint sibling、one usize phase carry、direct outer guard、表現可能なpositive constant ADDによるcarry、one loop/Continueとactual phase Break、one known-direct pure helper（usize formal1→dependency-free normal u8）。helperはformal側usizeとliteralのLT又はEQ、one IF、normal armsはliteral u8。実helper normal resultをcaller ordinary binding/Copyへ流し、original selected childのone scoped replaceとold-result Copyを観測する。

今回の差分は、このprofileに次の**短いordinary usize identifier Copy経路**を認める有限increment：

1. literal origin／seed → **最大2 Copy links** → loop initializer。
2. 実iteration phase → **最大2 Copy links** → helper actual argument。
3. 当該callのfresh usize formal → **最大2 Copy links** → helperのLT/EQ条件の左operand。

一つのsourceで全境界をむやみに合成せず、各境界の到達と少数の必要な組合せを区別する。source上のordinary Copyと、本物のiteration/formalの由来・時点の取り違えを独立に壊す問いを設ける。選択されたTのold/current/sibling、entry Copy、Continue outgoing phase、Break normal resultと外側Copyを数値で予測する。zero-trip、one／複数backedge、LT/EQ又は両branch、field選択などは有限予算内の重複の少ない組合せにする。一般全cross-productは不要。

第3 link、outer guard／ADD carry／RHS boundのalias、arbitrary arithmetic/dynamic initializer、複数helper/carry/writer/loop、helper explicit RETURN composition、effect/ref/recursive helper、heap/nonCopy/dependency-bearing Hは今回の支持を宣言しない。canonical上合法なscope外構成は**coverage probe**として扱い、precision拒否を安全性不具合とは呼ばない。type/condition/dependency等の真に不正なsourceはcanonical上の不正根拠と予測拒否層を分ける。早い拒否を、後段のalias/provenance/loan検査へ到達した証拠にしない。

## 4. 有限予算・freeze

独自positive **最大6**＋ordinary direct-path control **最大1**、negative/合法coverage probe **合計最大8**。この上限を使い切る必要はない。caseごとにsource、canonical根拠、合法性、予測最初到達層、phase/helper argument/formal/normal result/Copy、old/current/sibling、Continue/Break/postの期待traceと、その役割を記録する。

実行して値を後付けせず、最初のversionを復元可能なbundleとmanifestでIssueへ固定する。修正が必要なら旧版・理由・新hashを保持し、実行前に別freezeとする。元source/oracle/FIRST/旧refsを消さない。役割選択に期待値を入力する観測器は作らない。

prefreeze返却は、full SHA256／bytes／復元方法／sourceと期待値／coverage map／未解決のcanonical質問／将来の有限観測案と限界。ここでの技術結果は **NOT_RUN**。観測器の実装や旧Rの複製は今回の段階に含めない。共有executor上でcompiler/nativeを用いて構文や数値を試さない。

## 5. 将来の実行環境と次のrelease

**実測環境はGitHub専用ブランチ＋Actions。Pと同等のlocal実行環境は使わない。** 今回はprefreezeなので実行0。P1188の新候補が実際に公開・固定された後、Cが完全HEAD/tree・最小repository allowlist・branch/PR・FAST/通常actual-job予算をこのIssueへ別途記録し、実行releaseする。旧候補のF/R PASSを新差分の証拠として流用しない。

将来の証拠PRはDRAFT/NEVERMERGE。現時点でbranch／CI枠を予約・消費したとは扱わない。公開owned APIが全loopをvalidateできると仮定せず、適用不可ならNO_APPLICABLE_PUBLIC_API/NOT_RUN。有限native、表示された値、C-level観測を一般soundness/private authority/freshnessやaudit-locality実証に格上げしない。

scope内のpure-text作業は追加GO待ちなく自律継続。canonical曖昧さ、scope/oracleの実質変更、独立性破れ、原証拠欠損、実行段階への移行はaffected STOP→C。目標逸脱・保守費用への懸念は技術結果と区別してこのIssueに記録しCoordination1044へ集約する。audit locality UNMEASURED / GO624 PAUSED / H1#268 HOLD / P1036–PR1050 STOPを維持。

## 簡潔な起動prompt

> 本Issueを読み、Compass→Value/Process/CURRENT_SPEC→Draft17.34から、ordinary usize Copyの3境界（literal seed→loop initializer、実phase→helper actual、fresh formal→LT/EQ左operand、各最大2links）を独立に検証するsource/oracleをpure textで事前freezeしてください。positive最大6＋direct control1／negative・合法coverage最大8、元版・canonical根拠・到達層・有限traceを保全。P/F/旧Rの実装・source/oracle/runner/報告を先に読まない。**実測環境はGitHub専用ブランチ＋Actions、P同等local環境は禁止。今回はcompiler/build/native/新AST生成/branch/ref/push/PR/Actions/CI0。** actual STARTとfreeze返却を本Issueへ。新公開P固定後の実行releaseを別に待ってください。
