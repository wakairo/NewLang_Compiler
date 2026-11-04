Track: Coordination

### C225 — data-only evidence custody release（実測とは別・未START）

通常CIの最終結果が揃った後、今回R1144のFIRST／修復後結果を失わないため、**追加のデータ保存専用branchを1本だけ**認可します。これは既存の実行branch1／PR1／FAST6／通常12 jobsを拡張する認可ではありません。追加semantic/native実行、push-triggered CI、workflow rerun/dispatch、新PRはすべて0です。実測のC評価と並行可能で、保存のために新たなP採用gateは追加しません。

- branch: **`r-archive/r1144-c225-evidence`**。最終実測R HEADを親に、`evidence/r1144-c225/` のdataだけを追加。元実行branch/refは変更しません。保存前に3 workflowsの実triggerを確認し、このbranch作成でActionsが起動しないことを確認する。
- 保存対象: 全FASTの完全decoded job logs／formatter FIRST suggestion、通常FIRST6＋修復6の完全decoded原job logs・metadata・実checkout/tree・attempt・実status、各R evidence envelopeの完全bytesとraw結果、V1/V2 freeze／情報露出・修復・C裁定の時系列、全期間budget ledger。
- envelopeに含まれる73 file recordsは、その実bytesを復元できるbase64／byte length／SHA256を保持すればよく、同一データを多数の重複pathsへ展開する必要はありません。raw resultとREADME／manifestを別に置き、復元方法と各観測の限界を明記してください。これはbyte保存の認可で、生成Cや旧sourceを実行する認可ではありません。
- 全job raw、envelope内全file bytes、archive manifestを実bytesから独立再hashし、Git readbackと照合。元R treeの既存全leaves、Production/canonical/旧FIRST／3 workflowsが不変であることを確認。読めない原bytesは取得済みと称さず、具体的な欠落を報告。
- GitHubが報告するartifact ZIP digestと、実際に取得・hashしたZIP binaryを区別。完全decoded logやsuggestionを保存しても、未取得の元ZIP binaryまで独立検証済みとはしません。
- FIRST RED、旧NOT_RUN、露出後の限定独立性をそのまま保存。修復後成功で過去の失敗を上書きしません。安全性findingや予算超過をarchiveで解消したとは扱いません。

**これはRELEASEであり、まだarchive branch作成・保存START・完了を意味しません。** RまたはCのdata-custody補助担当は、実際に保存を開始／完了した時点のcommit/tree・path数・readback照合結果と、追加Actions/PR/semantic実行0をIssueへ返してください。Cが独立確認して受理します。
