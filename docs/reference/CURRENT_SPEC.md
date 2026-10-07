# Canonical NewLang v0 specification

The canonical NewLang v0 specification on a branch is the Draft named here.

```text
NewLang_v0_spec_Draft17_22.md
```

Repository policy:

- `main` is the canonical repository branch.
- `docs/reference/CURRENT_SPEC.md` identifies the canonical Draft on that branch.
- Design-thread reports, FormalProof results, compiler findings, and review-resolution documents are inputs to specification work; they do not override the canonical Draft by themselves.
- A newer Draft on an unmerged branch is a candidate until that branch is reviewed and merged into `main`.
- Prompts handed to M / F / P tracks should cite the `main` commit SHA and this file before relying on conversational memory.

Draft 17.22 adds the Issue #148 targeted bounded recursive-link field source clarification on top of Draft 17.21 with core semantic delta 0. For a current live direct lexical local whose static type is the exactly completed Draft 17.21 bounded recursive nominal, only that declaration's recursive link field of exact type `Option<ptr<Self>>` is added to the one-level field source profile. `local.link` is an ordinary Copy read and `loan_write(local.link) { |w| ... }` yields exactly ordinary `ref<write,Option<ptr<Self>>>` to the fixed link subobject. Mutation reuses existing field `Change`, ancestor refresh, sibling preservation, Option conditional-occurrence/reset, and `replace` semantics. Draft 17.18 `loan_read(local)` / `loan_read_ptr(ptr)` already apply to the completed nominal as ordinary `T`; no Node-specific lifetime/provenance rule is added. Node payload-field source access, nested/general/ptr-base member access, recursive backend execution, allocation/lifecycle, cJSON, general recursive types/generics, FFI, and separate compilation remain Deferred.