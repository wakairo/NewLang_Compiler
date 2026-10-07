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

Draft 17.22 adds Issue #150 bounded source punctuation separation on top of Draft 17.21: `local@field` and `loan_write(local@field)` replace the Pair-only `local.field` source forms; `SumType::Variant` and `SumType::Variant(expr)` replace the closed-sum dot constructors. `.` is reserved for future receiver call/chaining without admitting methods, implicit borrow/deref or general members. `@` and `::` do not share the old dotted field-vs-sum ambiguity; existing field projection/ordinary loan/Option/ptr semantics are unchanged (core semantic delta 0). Module/import/visibility remain Deferred with a future defining-module-private representation no-foreclosure direction, including construction/destructuring, and a possible explicit public/transparent escape hatch. Draft 17.21 adds the Issue #142 targeted recursive nominal identity/completion rule on top of Draft 17.20. A bounded recursive aggregate declaration may receive a stable incomplete nominal header before field-type resolution; while incomplete, that identity may participate only as the direct target of `ptr<Header>` in this closed profile. `ptr` is the only selected recursion-breaking constructor. After exact `Option<ptr<Header>>` resolution and value-containment-cycle validation, the aggregate is completed exactly once and structural Copy/Discardable properties are derived. Unbroken by-value cycles are rejected, completion is transactional, and the bounded declaration category is collected independent of physical source order. The first experiment uses canonical `Option<ptr<Node>>` through one exact source form rather than adding nullable pointers, a dedicated optional-pointer feature, or a general generic frontend. Draft 17.20 Pair-only field read/write source mapping remains unchanged. Allocation/lifecycle, recursive Node production implementation, general recursive types, general generic frontend, FFI, and separate compilation remain Deferred.
