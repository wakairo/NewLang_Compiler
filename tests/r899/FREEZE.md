# R899 second independent Stage0 observation — freeze and scope

Authority: Issue #899, Compass, Value Strategy §1.1/§1.3, Process §4.2/§4.3/§7, CURRENT_SPEC→Draft17.34. Execution exclusively normal Chat + independent R GitHub branch + Actions, NO P-equivalent local compiler/native run.

Freeze before opening P private code/tests: [Issue #899 comment #6394546227](https://github.com/wakairo/NewLang_Compiler/issues/899#issuecomment-6394546227). Exact LF + terminal LF source:
* `meridian.nl` 670 bytes SHA256 `26cff0f10d8d9e80934e883b3611f169f4ddd144862ded1050d9261fb2b5638d`, declared target index 0, PRE-old=37, sibling=163, other original=(59,201), incoming 81/118.
* `relay_pair.nl` 704 bytes SHA256 `16936f3d33086a4c03e2b401829c9b731e34ccee349922a118fe4037e058568c`, declared target index 1, PRE-old=212, sibling=43, other original=(174,26), incoming 65/147.

All two arms zero-item block with tail direct write-loan; each loan body zero-item block with tail registered known CALL; callee source-bound prior = replace result, RETURN prior. Normal CLI CHECK3 expected only; no source/C17/native proof beyond actual tests. R898 richer frozen source remains untouched/unrun, not recycled.

Calibration after freeze: private `nl_definition_call_observer`, `nl_definition_call_validate` borrowed synchronous callback API in `src/copy_body_internal.h`; source-check in `nl_semantic_register_function_unit` triggers definition-time IF callback before P6 join, no post-return pointer retention. `probe.c` independently constructs actual source parser+function-unit check, validates both arms with fixed oracle and actual graph tamper (arm owner, CALL target, child, checked RETURN), metadata tamper (numeric-ID, field, root, sibling, formal, scope), negative legal wrong-return / literal-old, absent-CALL. Build wrapper fault injector is limited to calloc for observation capture or malloc for PRE clone *inside* actual `nl_definition_call_begin`, confirmed reachable only by output count/status. Returns/callback validity is not held beyond callback.

Limits: graph swapping across separate checked contexts; all allocator sites; source mutation after seal; full base-vs-candidate stdout/stderr bytes, native and public IF issuer not claimed until independently observed. Every unrun item = NOT TESTED, not PASS. This private diagnostic has no authority; +717 private production C lines are possible author audit/re-audit maintenance cost, not §1.1 product proof. STOP at first failed source/P6/ownership/CI gate; one FULL5+PARTIAL attempt1 only; no merge/rebase/retry.
