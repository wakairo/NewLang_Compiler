# R875 FROZEN GATE A — FIRST-PASS BLIND oracle (before producer exposure)

Coordination: Issue #875 RELEASE. Compiler a8057d1f82e1c6ca498d1ff43860fdf9822b97a0; tree 4b0f7f1b4b40b7594c403f9af751f2ead33b166f. Canonical CURRENT_SPEC -> Draft17.34. Public include/newlang/copy_body.h, if_field.h, checked.h; no producer PR/diff/implementation/tests/oracle read. All sources independently authored.

A: TRUE `usize(29)==usize(29)`, THEN NORMAL with two-formal `adjust(ref<write,u8>,u8)->unit`; ELSE `return unit;`. Original Gauge.temperature 31 -> 72, sibling checksum 211 -> 211, receiver.checksum 8 -> 72, receiver.temperature remains 147. Source-original old 31 must be Copy from `replace` and copied twice within callee. No original root/incarnation/loan escape.
B: FALSE `usize(61)==usize(74)`, THEN `return unit;`, ELSE NORMAL with two-formal `reroute(ref<write,u8>,u8)->unit`. Original Axis.yaw 57 -> 94, sibling pitch 193 -> 193, other.yaw 153 -> 94, other.pitch remains 22. Source-original old 57 Copy twice in callee. No original root/incarnation/loan escape.

Validation required from actual source CHECK and issued owner: genuine call node and registered `NLCopyUnitView`, exact two actuals/formals, `nl_checked_copy_body_parameters_validate`, `nl_checked_copy_unit_validate`, `nl_checked_if_return_validate`, normal-arm event view/validator original old Copy/Change, sibling/root and return node. Generic `nl_checked_if_field_validate` is NOT applicable. After caller return/normal join, public postcall is conservatively Unknown/may-set, NEVER a newly minted concretely known `ValueFact` merely because native scalar happens to be predictable. Distinguish symbolic public fact vs emitted runtime scalar.

Unmodified strict C17 compiled with GCC and Clang at O0/O2; separately SHA-hashed passive same-live-scope C observer MAY ONLY inspect original physical values, never mirror or mutate guest. Sanitizers through natural CI. OOM only PASS if injected allocation failure genuinely reached and counted; otherwise NOT TESTED.

Negatives must CHECK reject with no named-output changes: `wrong_argument_type.nl` (usize for u8), `third_argument.nl` (extra call argument), `wrong_unit_return.nl` (u8 for unit), `unknown_original_field.nl` (not declared). Public owned-view tamper matrix: foreign owner, call, loan, argument, changed root/child/sibling, Change/event count and return arm/world; all fail closed. Do not dereference foreign world.

## LF UTF-8, terminal newline, SHA256
- tests/r875/sources/true_then_normal.nl: 5b135246445caa4fa33c38ffa470396ebe15e6d25a29db08bc9abbba6cd1e57e
- tests/r875/sources/false_else_normal.nl: 072bdffbf07e00a493c5a13f72261b5e1a5d13e378766e18acf062ae69c4db29
- tests/r875/negative/wrong_argument_type.nl: ea1db69fd3bf4a9e45005e27dc410f8041f0d97d14b146669a86b6afa77b64b1
- tests/r875/negative/third_argument.nl: 214c726e138c28a12a356314ec9084d4c57de49586d49c9108f9f1450629de11
- tests/r875/negative/wrong_unit_return.nl: 22db198cca96bd6748063545e3a45ec05c6f5ba768be34f11b4d5ea0c2060c17
- tests/r875/negative/unknown_original_field.nl: 55f7e36cba04fb4a9602f9b165ff2d1027640728b1574b78ff1af153b2116f2f

Immutable source/oracle commit before ANY producer metadata; one R branch from P872 fixed head, no old R changes, no local Work/Codex runtime; at most one natural FULL5 + PARTIAL pair after draft PR; no retries/dispatch/push after first CI. STOP on source eligibility doubt, format/build/check/validator/native/sanitizer fail or target/CI drift. No PR merge, no Product GO claim.
