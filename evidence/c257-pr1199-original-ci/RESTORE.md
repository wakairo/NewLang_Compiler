# Restore original PR1199 CI evidence

This is evidence-only custody of the original24339 candidate, NEVERMERGE. F-1198-01 remains unresolved for that candidate; CI green is not adoption. The original packet is byte-preserved.

Read `archive-index.json` for ordered chunks and hashes. Concatenate `chunks/packet-*.b64` in lexical order, remove whitespace, Base64-decode to `ci-audit-packet.zip`, and verify SHA256 `fef63e29018229f1765a74a8527ca5a52608222dc87dd4b3b4017ee1f79d6da6` and575920 bytes. Check ZIP CRC, exact36-member set and every one of35 payload lengths/SHA256 against `manifest.json`. `FINAL-RECEIPT.json` is outside the ZIP to avoid a circular hash.

C's `C-RESTORE-CHECK.json` records independent data-only restoration checks. No archived compiler, guest, observer, semantic decoder or audit script needs execution to restore or inspect these bytes. The original six decoded whole job logs, exact checkout/job metadata and finite audit are preserved. GitHub's original binary job-log ZIP bytes are not claimed.

This branch changes only this evidence directory relative to fixed24339. It opens no PR, changes no workflow and requests no Actions run. Original CI budgets remain6/12, manual0. Repaired-head evidence must be recorded separately.
