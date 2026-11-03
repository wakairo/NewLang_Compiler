# P1117 / C212: runtime usize predicates in pure ordinary calls

On main `65055be55934416c911453463b5f26fa461e8d6a`, an ordinary function's
unknown `usize` parameter made `==` and `<` fail CHECK with a precision limit.
The checker now produces a dependency-free **Unknown Bool**, retaining both
operand types and rejecting hidden/unknown dependencies. Even `x == x` stays
Unknown here; this change proves no owner, loan, current, or root identity.
Canonical authority is CURRENT_SPEC → Draft17.34; semantics are KEEP.

The driver-private C17 consumer supports one `usize` formal, a `u8` result,
pure scalar Copy bindings/aliases, and one two-normal-arm IF whose arms return
u8 literals. Bound results, direct `return if(...)`, and direct tail IF work.
The generated native function evaluates the guest predicate on its real
`size_t` argument. The existing native quantity-width guard remains.

It authenticates the current callable body/plan, signature, source, actual
argument/formal package, actual RETURN/tail producer, and canonical IF child
lineage. Historical observation supplies no new current permission. Both arms
are checked by the existing IF machinery; the backend issues no proof.

The source-first inputs, complete oracles and SHA256s were posted before each
execution in Issue #1117. Sources and original expectations are immutable in
`tests/p1117/evidence/inputs-v*.zip`. The b6 self-review RED accepted another
genuine call result and emitted partial C for direct RETURN. The graph probe
was read-only and restored; that partial output was never executed. Candidate
`7b7881a` ties the result to the actual callee producer and completes the same
pure IF emission. Final current/lineage negatives reject these substitutions.

Active harnesses are `run_v2.py`, `safety_v3.c`, `safety_oom_v2.c`, `lineage.c`,
and `history_latest.py`. The latter verifies all immutable input archives and
current history v29. Original P504/P603 C/Python/source/oracle/baseline files
are unchanged: SHA-pinned prospective generators/wrappers explicitly version
only the superseded unknown-comparison precision expectations. Other original
type, reference, rollback, mutation, native, and C-byte checks still execute.
Original v1–v21 history is unchanged; v22–v29 append external predecessor
SHA256/Git blob anchors and inherit protected inputs. P1107 appends harness
freezes for CMake changes without altering its old source/oracles/FIRST.

Local evidence at integration commit `6f392d7` is in
[`summary-v1.json`](../evidence/p1117-c212-local/summary-v1.json) and the
SHA-indexed [`raw-v1.zip`](../evidence/p1117-c212-local/raw-v1.zip):

- GCC frontend: all 642 configured tests have actual success records. Execution
  continued only with missing tests after explicit legacy-oracle successors and
  a disk-capacity failure; all failed attempts and exclusions remain recorded.
- Clang: 16/16 focused checks; UBSan: 16/16 including native; ASan/LSan host:
  15/15 plus 44 successful strict GCC/Clang O0/O2 raw/passive native executions.
  The local GCC13 static ASan runtime needed `-lm` link closure, recorded as a
  transparent local adapter. No repository toolchain change was made.
- Fresh detached checkout/build: 8/8 P1117 tests, including all reached OOM.
- Equality/less OOM counts: registration 745/490, invocation 523/325, named
  CLI 1339/882. Each reached fault records allocator, PC, API phase/status;
  rollback/no owner publication, bytes/inode/mtime/no temporary clobber, and
  fault-disabled retry are asserted. Canonical A destruction preserves B.

True LOOP remains blocked: the unchanged P1115 source02 control reaches
`P14-LOOP-PRECISION`, after the earlier comparison blocker is removed. No
cyclic header, actual CONTINUE, backedge, or iterative native execution is
claimed. Effects/refs, multiple formals, nested calls/IF, general root IF and
dependent/abstract Bool remain outside this finite consumer. Their rejection
is not evidence that the canonical source is illegal.

Maintenance cost includes the new 327-line private walker, emitter adapters,
SHA-pinned legacy-oracle adapters, and append-only evidence/history storage.
This is bounded capability evidence, not general compiler soundness or
arbitrary host-memory-corruption resistance. Human UB-prevention audit and
re-audit closure versus matched well-factored C is **UNMEASURED**. Product
GO #624 stays PAUSED, H1 #268 stays HOLD; P1036/PR1050 STOP persists.
Independent R sources/oracles/raw/values and M1118 conclusions were not inputs.
Coordination and proportionate independent review remain pending. DRAFT/HOLD;
no READY, merge, or Product GO decision is made.
