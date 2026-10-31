# P961 C78 — LOCAL STOP / NOT A MERGE CANDIDATE

Base: `2ac1c705a88a15f9b11b732744720a3655b7dd8d` (frozen PR954).
The new private CHECK-issued root/main/allocation origin and canonical custody
implementation is preserved as an incomplete local checkpoint. No push, PR or
Actions was performed. PR946/954 remain DRAFT HOLD and P953 SELF-STOP remains.

The exact original P953 harness is `evidence/original_foreign.c.txt`; SHA256
`db1b4481de8a2ce96f181e7fff96bdef18c05cecbc0f3e260af6caf5f75addf1`.
Its historical acceptance remains a safety FAIL. On this checkpoint it rejected
with status1/C0 and healthy restoration. New tests use two real CHECK invocations
of one registered plan/source, reject nine origin mutations per source, and
exercise canonical teardown while a foreign view remains substituted.

GCC critical tests: 3/3. ASan/LSan critical/P953 integrity/selected driver OOM: 7/7.
Exact historical hash normalization and mutation controls: PASS; old manifests
unchanged. Ordinary Echo/Window CHECK/C17: 0/0, 14007/14006 bytes. No new guest
native execution or full regression is claimed.

## FIRST STOP — do not silently repair or rerun

The next ASan CTest (`p961_reached_oom.unit`) failed in test setup at `oom.c:99`:
`nl_source_create` received a non-NULL output pointer after `nl_source_destroy(s)`.
Fault injection has not started (the enable is later in the file). `CHECK` exits
without disposing the registered context, and LSan reports 60879 bytes in 343
allocations. This is a harness-setup failure; it does not establish a Production
OOM or origin-custody defect. The same setup also leaves the old parser pointer
non-NULL; that subsequent call was NOT reached. No additional repair was made.

CTest `--stop-on-failure` prevented the named-output test from running. New
root-origin targeted OOM, full frontend, UBSan and healthy native checks remain
incomplete. Local safety gates are therefore NOT complete; PR/CI were not
unlocked. The initial failed invocation, full sanitizer output, successful
prior gates, freeze and implementation are retained for Coordination.

Future continuation needs a separate C decision, preserving this failure.
A small harness-only ownership/setup correction and cleanup may be considered;
this checkpoint has NOT made that correction. Do not infer general source-rich
soundness, independent R PASS or Product GO. Value Strategy audit-locality
improvement remains UNMEASURED; Product GO PAUSED, H1 HOLD.
