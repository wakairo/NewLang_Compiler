# Restoring FIRST from Issue1190

The freeze receipt on Issue1190 names the exact deterministic `r1190_prefreeze_FIRST.tar.gz` byte length and full SHA256. The payload comment(s) contain its base64 between `R1190_FIRST_BASE64_BEGIN` and `R1190_FIRST_BASE64_END` markers; if chunked, concatenate chunk data in the numbered order.

1. Save the exact base64 text to a local file. Decode it as base64 to `r1190_prefreeze_FIRST.tar.gz`.
2. Verify the archive's SHA256 and byte length against the Issue receipt **before extraction**.
3. Extract into a fresh directory. All archive members are regular files under `FIRST/`, with no symlinks or path traversal.
4. In extracted `FIRST/`, run a file-hash verifier against `SHA256SUMS`. The sums include manifest.json; the archive hash protects SHA256SUMS itself.
5. Compare manifest entry byte lengths and SHA256 values. It enumerates every payload file except manifest.json/SHA256SUMS to avoid cyclic hashes.
6. Read README.md, records/read-log.md and RESTART_PROMPT.md. Restore sources and oracle unchanged. This does not authorize guest execution.

This restoration is only byte/text/archive processing. No compiler/build/native/new-AST/guest evaluator is needed. The original FIRST must not be overwritten by any later V2 or calibrated packet. Future execution stays GitHub専用ブランチ＋Actions、P同等local禁止 and requires C's separate release.
