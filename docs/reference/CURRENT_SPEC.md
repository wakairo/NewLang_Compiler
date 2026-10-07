# Canonical NewLang v0 specification

The canonical NewLang v0 specification on a branch is the Draft named here.

```text
NewLang_v0_spec_Draft17_19.md
```

Repository policy:

- `main` is the canonical repository branch.
- `docs/reference/CURRENT_SPEC.md` identifies the canonical Draft on that branch.
- Design-thread reports, FormalProof results, compiler findings, and review-resolution documents are inputs to specification work; they do not override the canonical Draft by themselves.
- A newer Draft on an unmerged branch is a candidate until that branch is reviewed and merged into `main`.
- Prompts handed to M / F / P tracks should cite the `main` commit SHA and this file before relying on conversational memory.

Draft 17.19 adds the Issue #113 targeted bounded ordinary write-loan source clarification on top of Draft 17.18 with semantic delta 0. A Provisional North-Star-only `loan_write(local) { |w| ... }` form is admitted only for a direct current lexical local root of core `u8`; `w` is exactly ordinary `ref<write,u8>`, not exclusive authority. It reuses the local implicit governing identity, exactly-once loan scope, and Draft 17.18 normal-result forwarding unchanged. Existing dependency/effect conflict rules remain authoritative, and canonical `replace` changes the current value/fact while preserving root incarnation, so a preexisting ptr to that same live incarnation remains potentially reacquirable. Other write-loan types/places, final loan syntax, field projection/mutation, raw storage, and broader parser/name policy remain unresolved.
