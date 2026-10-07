# Canonical NewLang v0 specification

The canonical NewLang v0 specification on a branch is the Draft named here.

```text
NewLang_v0_spec_Draft17_20.md
```

Repository policy:

- `main` is the canonical repository branch.
- `docs/reference/CURRENT_SPEC.md` identifies the canonical Draft on that branch.
- Design-thread reports, FormalProof results, compiler findings, and review-resolution documents are inputs to specification work; they do not override the canonical Draft by themselves.
- A newer Draft on an unmerged branch is a candidate until that branch is reviewed and merged into `main`.
- Prompts handed to M / F / P tracks should cite the `main` commit SHA and this file before relying on conversational memory.

Draft 17.20 adds the Issue #128 targeted bounded fixed-field source clarification on top of Draft 17.19 with semantic delta 0. For the registered AVS `Pair { left: u8, right: u8 }` direct lexical local only, `local.field` is a one-level Copy field read and `loan_write(local.field) { |w| ... }` yields exactly ordinary `ref<write,u8>` to that fixed subobject. The source rule explicitly separates value-local field resolution from the existing type-qualified sum-constructor route without introducing a general member-expression system. Field `replace` uses the existing `Change(field)` / structural current-state rules: root and fixed-field incarnations are preserved, the target and ancestor value facts refresh, and the known-disjoint sibling fact is preserved. Nested/general member access, aggregate-wide write loans, ptr-field source forms, recursive nominal/link mutation, raw storage, and general/final loan syntax remain unresolved.
