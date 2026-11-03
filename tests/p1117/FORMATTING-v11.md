The initial CI Clang job125860151895, run41619166518, stopped at formatting
of the pre-execution frozen lineage.c. Original83d114e, source/oracle, all
FIRST/RED and raw-v1.zip remain unchanged. Production is byte-identical.

lineage_v2.c is exactly curated clang-format23.1.3 output from lineage.c.
It has the identical foreign-result/coherent-root refusal oracle and executes
in the same target. The original is retained at its exact frozen path; the
standard formatter ignore file exempts only this historical noncompiled input,
with a comment naming the compiled format-checked successor. No live test or
Production source is exempt; no workflow/check-format algorithm changes.

The original source SHA is d4614473cb0bd5d4cfd673bea9ea994b5e22ffc08e608734604214e8b52d7c13;
formatted successor SHA is 5af0b0c5c7ca37b5a02f7e7a532f83db0f8b52b7a15ed967b657086c13137c07.
Before execution freeze v11 covers the complete successor and same oracle.
Local validation will run the unchanged all-tracked-C formatter script, build
and run the successor lineage/current/history checks. Existing native/OOM and
642 frontend success records remain applicable: Production is unchanged.
Only the final authorized natural CI pair will be used after batching repair.
