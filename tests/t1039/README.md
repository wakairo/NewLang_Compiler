# T1039 — synthetic artifact name / raw custody preflight (UNADOPTED)

This is a T-owned, isolated Python stdlib-only proof of concept. It is not a
GitHub workflow, an R rerun, a native test, or new compiler proof. The two Python
files contain ONLY synthetic names, ref scenarios and temporary mock raw files;
they never read any R frozen source/oracle/runner/ZIP or T1014 bridge.

## Historical incident and verified fix

R v1 CALIBRATED run 40869364041/attempt1 produced 43 runner-local raw files,
but upload-artifact@v4.6.2 rejected an artifact name containing the raw branch
r-calibrated/r1016-pr1031 (a forbidden forward slash). The v1 artifact list was
empty and the 43 raw files are LOST. Nothing here recovers or regrades them.

At approved R v2 executor 1edc429ff22513db1b47bd1cbd3fcda31f573d53,
the inspected workflow file uses a fixed suffix r1016-pr1031-v2 rather than
an unsanitized branch name; its name contains run_id, run_attempt and sha.
The distinct authentic v2 run 40874742688/attempt1 already uploaded artifact
12897102916 (43 files) successfully. We did not replay or access its raw files.

The test suite deliberately models the BAD old name using a literal raw
{ref} token and the GOOD name using the explicit {ref_slug} token or the
fixed v2 suffix. These are simulation templates, not a workflow installation.
The script restricts simulated refs to the two calibrated branch versions
(including the original slash-bearing v1 ref). It requires run_id, attempt and
SHA in the template; excludes ambiguous formatting expressions, duplicate
names, invalid platform characters, empty/overlong names and reserved names.
It additionally checks required nonempty regular files and exact count, and
records SHA256 per synthetic file. The max 160-character name length is a
conservative local policy, NOT a claim about the upstream GitHub length limit.

## Local isolated test commands (NO Actions, R, compiler, GitHub writes)

From this folder:

  python3 -m unittest discover -s . -p 'test_*.py' -v

The locally observed result was 19/19 PASS, including separate checks for
the historical bad raw-branch name, portable v1/v2 names, attempt/run/SHA
uniqueness, duplicate output names, missing fields, forbidden characters,
format interpolation, empty or missing raw files, missing mandatory receipts,
incorrect count, 43 synthetic files, symlinks and nonportable filenames.

Example: after manually creating a synthetic-raw directory with nonempty
journal.jsonl and status.txt:

  python3 artifact_custody_preflight.py \
    --template 'r1032-CALIBRATED-{run_id}-attempt{attempt}-{ref_slug}-{sha}' \
    --ref 'r-calibrated/r1016-pr1031-v2' --run-id 40874742688 \
    --attempt 1 --sha aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa \
    --raw-dir ./synthetic-raw --expected-count 2 \
    --required journal.jsonl --required status.txt

Positive synthetic invocation: exit0 and JSON with filename, run/attempt/SHA,
2 raw-file SHA256 digests. Negative v1 name using {ref}: exit2 with an
explicit unsafe artifact name containing the slash. Neither output establishes
an upload receipt or a real R observation.

## Initial RED, local repair and unresolved evidence

The earliest local unittest invocation had 16 cases and one FAILURE because
the test incorrectly expected CON-<suffix> to be an NTFS reserved filename.
That label is not reserved. The T-owned fixture was corrected to CON.<suffix>;
19/19 subsequently passed. Both the synthetic bad-name RED and the genuine
historical R v1 upload failure are retained as failures; neither is relabeled
as semantic PASS.

This preflight does not call the real GitHub action and CANNOT prove artifact
persistence or v1 recovery. Future explicit Coordination authorization could
integrate a reviewed variant before uploading, retaining if: always() and
if-no-files-found: error; the actual upload artifact ID/digest/ZIP and original
run/attempt/executor SHA, all entries/CRC/JSONL/stage receipts still require
post-upload independent verification. It is important to re-review the ref
allowlist, file inventory and extra maintenance before any trusted deployment.
There is no new workflow/PR, CI run, R branch or Production mutation here.

VALUE-ALIGNMENT-RISK: adding a new privileged pre-upload check or trusting
its summary as evidence may increase CI upkeep and obscure actual custody.
It does not reduce the library-author + ordinary-safe caller UB-prevention
audit/re-audit closure relative to well-factored C. That value remains
UNMEASURED. Product GO PAUSED, H1 HOLD. No READY/merge/GO claim.
