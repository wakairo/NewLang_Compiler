# R1213 C272 independent evidence-only tests — NEVERMERGE

- Frozen BLIND V1 (3 positives, 6 negatives, 7,723 exact source bytes): [Issue #1213 freeze](https://github.com/wakairo/NewLang_Compiler/issues/1213#issuecomment-6685125763).
- Fixed canonical/executor parent: `491f54adfa64ae7308344923ed14a9fcad08a8e9`, Draft17.34. No P fixture, oracle or validator implementation is reused by this harness.
- Original manifest SHA256 recorded in Issue: `b524a8374b1069273fae494478dc030790c95736b177dfe7a20b7084903a3097`. The original manifest and ZIP bytes are **NOT** reconstructed or claimed as uploaded blobs. The separate `runner_cohort.json` here is a newly authored runner index, not the original frozen manifest.
- `v1/*.nl` files are immutable; `original_frozen_issue_comment.md` copies the original pre-execution predicted oracles as read back from GitHub.
- Source outcomes: negative CHECK acceptance is an immediate STOP; rejection does NOT certify its intended semantic rule without diagnosing first failure layer. Positive CHECK/emit refusal is recorded as safe conservative unsupported, not native PASS.
- A successfully compiled and run V1 `main()->unit` has no value-observer. Therefore native exit 0 alone does **NOT** certify old Copy, post-current, sibling identity, source-owned issuer, public checked-owned provenance, or numeric guest values. Those remain NOT TESTED and require additional independently authorized observation; do not forge them from expected constants.
- This runner never calls a private issuer, bypasses CHECK or modifies the workflow/toolchain. It must run only in the authorized GitHub Actions R CI; no P-equivalent local compiler/native execution.
- Evidence scope: physical CI raw logs, per-case CLI diagnostic, strict C17 compile/runtime only upon actual reachability. No READY, merge, or Product GO.
