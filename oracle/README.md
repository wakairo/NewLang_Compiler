# Frozen M7.5 oracle (Class O)

This directory's frozen M7.5 attachment is historical evidence, distinct from the currently **provisional Track O** Python algorithm research. Track O's process-level charter is [`docs/NewLang_Track_O_Research_Charter.md`](../docs/NewLang_Track_O_Research_Charter.md); independent experiments may live under `oracle/experiments/` in unmerged evidence branches. Neither the archive nor those experiments supersede `docs/reference/CURRENT_SPEC.md` or provide C-issued source authority.

`NewLang_FrontEnd_Prototype_M7_5.zip` is the untouched attachment, identified by
`identity.json` and independently by `docs/reference/INPUT_ARTIFACTS.json`.
M7.5's metadata version is 0.7.0; historical baseline 466/466 pytest PASS is
closure evidence, not a P0 production capability claim.

P0 invokes `newlang_frontend.driver.check_source` on four original fixtures and
`python -m newlang_frontend check FILE` through `tests/oracle/harness.py` and
`smoke.py`. Python >=3.12 and bundled `vendor/newlang_checker` suffice; there are
no third-party P0 Python requirements. The complete original pytest suite lives
in the archive's `NewLang_FrontEnd_M7_5/tests/` and is not run by P0.

Use `ctest --test-dir build-gcc -R oracle.smoke --output-on-failure` after normal
bootstrap/build. Extraction is temporary, hash-checked and isolated. Future
differential cases belong in `tests/oracle/`; unsupported C results must never
be classified as oracle agreement. See `docs/P0_ARCHITECTURE.md` for the seam.

The archive also contains historical specifications and contracts. These are
Class O/H contextual evidence, not current normative language authority.
The canonical NewLang specification is the Draft selected by
`docs/reference/CURRENT_SPEC.md` (currently Draft 17.30). References to
Draft 17.4 and Backend Contract v0.4 in frozen M7.5/P0 material describe
that historical context; they do not supersede the selected specification.
