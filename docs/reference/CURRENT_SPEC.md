# Canonical NewLang v0 specification

The canonical NewLang v0 specification on a branch is the Draft named here.

```text
NewLang_v0_spec_Draft17_18.md
```

Repository policy:

- `main` is the canonical repository branch.
- `docs/reference/CURRENT_SPEC.md` identifies the canonical Draft on that branch.
- Design-thread reports, FormalProof results, compiler findings, and review-resolution documents are inputs to specification work; they do not override the canonical Draft by themselves.
- A newer Draft on an unmerged branch is a candidate until that branch is reviewed and merged into `main`.
- Prompts handed to M / F / P tracks should cite the `main` commit SHA and this file before relying on conversational memory.

Draft 17.18 adds the Issue #108 targeted loan normal-result / bounded local-root ptr/ref source clarification on top of Draft 17.17 with semantic delta 0. Exactly-once loan normal completion checks scope-exit compatibility, ends the loan scope, then forwards the unchanged body result package into ordinary value flow; scoped refs therefore cannot escape while a `ptr<T>` produced from a ref may survive when no blocking loan-scope dependency remains. A Provisional North Star-only read profile maps `loan_read(local) { |r| ... }`, `ptr_from_ref(r)`, and `loan_read_ptr(p) { |r| ... }` to the existing §10.1/§10.2/§11.1/§13.7 semantics without programmer-visible `LifetimeDomain` plumbing. General/final loan syntax, write spellings, field projection/mutation, raw storage, and broader parser/name policy remain unresolved.
