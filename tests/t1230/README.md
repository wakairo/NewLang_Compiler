# T1230 — C285 DRAFT/HOLD only

Exact source base `aa8dce8935f44d7b0732d24695f1c0f7c305efb1` (Draft17.34).
The ONLY intended CTest runtime policy change is post-global TIMEOUT180 for
`p1055_old_origin_0.integration` and `p990_new_native.integration`.
Old P990 v59, P1107 v40/ZIP, P1217 v6/capsule, FIRST/RED, validators,
ordinary source, Production and workflows are untouched.

This branch has its OWN prospective P990 v60/P1217 v7; **NOT COMPATIBLE**
with unrelated P #1227 / DRAFT/BLOCKED PR #1231's different v60/v7.
Merge/adoption/integration order is Coordinator C only, no T READY/merge.

`escrow/root-CMake-aa8dce.txt` and `escrow/p1217-CMake-v6.txt` are direct
Git blob aliases of EXACT adopted-main historical files, not reconstructed
text. The prospective v7 adapter authenticates their Git blob identities and
the old P1217 capsule baseline tree. It executes the unchanged v6 checker
against recovered old inputs and the unchanged ten P1217 tamper controls.
It also verifies changed CURRENT root/registration against exactly derived
expected byte strings. No original checker/capsule/oracle/negative is modified.

Local authentic NewLang compiler/CTest/native/history validation: NOT_RUN
(unavailable original complete checkout). Natural PR CI is the first full
candidate test; first RED must be preserved. Product GO/H1 remain HOLD.
