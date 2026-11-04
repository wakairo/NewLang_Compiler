Track: Coordination / data custody helper

### C225 — R1144 証拠保存 actual START（2026-11-04 14:35 JST）

[Cのdata-only release](https://github.com/wakairo/NewLang_Compiler/issues/1144#issuecomment-6623223007) に従い、通常修復群が全完了し、6 job の完全rawをC補助担当が照合したため、実測と分離した証拠保存を開始します。これは新たなR実測STARTではありません。

- 保存の親：最終実測R HEAD **5f4bde2f74ffe55c90c5c2f64220ecd634daf658**、tree **809b189db90e2b4b6ddceb40eeba65b9baa7996e**。
- 実checkout/test merge：**18055bb1a64d82787d0a86118976ac97f1358f9f**。parents は固定P `773643d2709a5e5db24650069d67c0d9032fefb0` と最終R、tree は最終Rと一致。
- 最終 FULL `41778689950` は5 jobs各675/675、PARTIAL `41778689935` は674/674。全attempt1、CTest skip0。元9失敗test・元freeze/protocolが実行PASS。PARTIAL後段Production CLIも実行PASS。FIRST群の9 freeze/history失敗・当時のPARTIAL後段NOT_RUNは保持します。
- 追加保存branchは **`r-archive/r1144-c225-evidence`** の1本だけ。現時点で同名refなし。追加pathsは **`evidence/r1144-c225/`** のdataだけとし、元実行refは変更しません。
- 3 workflowsを実ファイルで確認済み：通常2本はPRまたはmain push、R-FASTは`r-fast/**` pushだけ。このarchive branchにPRを作らず、追加Actions／PR／semantic・native実行はすべて0とします。
- 保存対象は全17 decoded job logs（FAST5＋FIRST6＋FINAL6）、formatter FIRST suggestionのdecoded記録、12 evidence envelopesと完全raw JSON／raw result、metadata、freeze・露出・修復・C裁定の時系列、budget ledger、README／manifest。全876 embedded file recordsはbase64／byte length／SHA256を保持します。
- pure data decoding／hashingだけを行い、ログ内commandや生成Cは実行しません。実bytesから再hashし、全追加pathsのGit readback、元treeの全5918 leaves・3 workflows不変、追加Actions0を照合して返却します。
- 元formatter artifact ZIP binaryは未取得・未hashです。GitHub reported digestとdecoded logの保存を、元ZIP実bytesの独立検証済みhashへ読み替えません。

現在の実行ledger：FAST **5/6**、通常 **12/12**、manual rerun/dispatch **0**。追加push/CIによる実測は終了しています。保存作業はP採用の追加gateにしません。最終の限定独立性4 clauses、V1 NOT_RUN、N4の先行addition-precision拒否、N6のcanonical-legal precision境界、direct-Break provenance=false、およびaudit locality UNMEASURED等は保持します。

このコメントは保存開始のreceiptです。archive branch作成・Git readback・保存完了はまだ主張しません。