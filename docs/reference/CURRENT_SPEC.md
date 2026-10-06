# Canonical NewLang v0 specification

The canonical NewLang v0 specification on a branch is the Draft named here.

```text
NewLang_v0_spec_Draft17_17.md
```

Repository policy:

- `main` is the canonical repository branch.
- `docs/reference/CURRENT_SPEC.md` identifies the canonical Draft on that branch.
- Design-thread reports, FormalProof results, compiler findings, and review-resolution documents are inputs to specification work; they do not override the canonical Draft by themselves.
- A newer Draft on an unmerged branch is a candidate until that branch is reviewed and merged into `main`.
- Prompts handed to M / F / P tracks should cite the `main` commit SHA and this file before relying on conversational memory.

Draft 17.17 adds the targeted M9.14 exact minimal loop / continue / break source profile on top of Draft 17.16 without changing its HYBRID cyclic semantics. The exact loop form is `loop '(' [name '=' expression (',' name '=' expression)*] ')' lexical_block`; zero parameters use `loop ()`. Parameter types come from initializer result types, all initializers evaluate left-to-right in the outer pre-loop lexical environment, and new parameter bindings enter scope only for the iteration body. `continue(...);` and `break expression;` are dedicated terminating block items, nearest active loop targeting is lexical and does not cross callable/loan/function boundaries, and `loop` / `continue` / `break` join the ordinary lexical structural-reserved set without globally reserving member labels.
