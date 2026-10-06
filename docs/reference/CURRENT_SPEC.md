# Canonical NewLang v0 specification

The canonical NewLang v0 specification on a branch is the Draft named here.

```text
NewLang_v0_spec_Draft17_14.md
```

Repository policy:

- `main` is the canonical repository branch.
- `docs/reference/CURRENT_SPEC.md` identifies the canonical Draft on that branch.
- Design-thread reports, FormalProof results, compiler findings, and review-resolution documents are inputs to specification work; they do not override the canonical Draft by themselves.
- A newer Draft on an unmerged branch is a candidate until that branch is reviewed and merged into `main`.
- Prompts handed to M / F / P tracks should cite the `main` commit SHA and this file before relying on conversational memory.

Draft 17.14 adds the targeted M9.9 structural source-word / ordinary-name rule on top of Draft 17.13. The current `fn`, `let`, `return`, and `match` spellings are reserved from the ordinary lexical namespace as a small structural set, without requiring lexer-wide keyword tokenization or automatically reserving field/variant/member namespaces; exact `unit` remains a separate distinguished non-shadowable core spelling.
