# NewLang formal-proof track pointer

The bundled `NewLang_F0_Formal_Kernel_Specification_Draft0.md` is a non-normative formalization bridge and may lag the live proof repository.

Current proof repository:

```text
https://github.com/wakairo/NewLang_FormalProof
```

For production compiler work:

- Draft 17.4 remains the normative language source.
- The formal repository is evidence and a source of theorem/counterexample ideas.
- Do not copy Lean ghost-state or proof-oriented representations into production data structures merely because they are convenient in Lean.
- If a formal theorem or bridge text appears to conflict with Draft 17.4, classify and report the discrepancy rather than changing production semantics silently.

At P0, inspecting the formal repository is optional except where useful for understanding invariants or test ideas. Do not add a build dependency from the production compiler to Lean.
