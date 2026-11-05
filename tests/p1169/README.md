# C238: a finite changing-current loop on the same original T

Fixed source authority: main `425e7f6c8c54643765e861b8235620348a3c91a3`,
Draft17.34 §§13.5a,17.4,27.5–27.7. Inputs are independently P-owned; no new
independent R input/oracle/runner/report was consulted.

The supported family is one pre-loop ordinary two-u8 T, one selected field,
one usize carry with literal initial value, `i < L` (L>0), constant ADD step
`1 <= k <= MAX-(L-1)`, one scoped literal-u8 replace per Continue iteration,
ordinary local old-Copy aliases/field reads/discards, and one `break i`.
The iteration has no surviving loan or dependency. Post-loop Copy/read uses
the caller's structural current summary. Both field indices and equal-bits
writes are covered. No SUB, additional writer, nested/multiple loop, heap,
affine owner, extra call effect, escaping reference, public API or issuer is
added. Unsupported legal source remains a precision/profile refusal.

`oracle.json` is the new implementation oracle. The original P1164 sources
are copied byte-for-byte; its original oracle/FIRST/three archives and fixed
main449 ref are retained separately. Its precision prediction is historical,
not changed to PASS. The inherited oracle key `public_loop_validate` is only
a historical label: `nl_linear_break_validate` is compiler-private. Public
owned-body observation and private current validation are separate checks.

The private relation denotes arbitrary dependency-free selected u8 and usize
values with the exact original structural frame. It checks all surviving
packages, the complete source transfer, fresh current facts, canonical root
and sibling owner remapping, both leaf and union exits, the caller summary,
and current graph/world/registry seals. It does not unroll concrete iterations
or use first-old as later-old. The abstract structural phi is never emitted
as a runtime write; zero backedges perform zero stores. Existing stutter
retains its exact persistent-current invariant.

`safety.c` obtains a real registered main() CHECK, inspects owned contexts,
tests ghost freshness/incarnation/sibling and current-consumer rejection,
then destroys an unrestored challenged A and verifies independent B remains
usable and destructible. Its OOM mode first measures allocation ordinals and
PCs, then checks registration/call rollback, named-output no-clobber, cleanup
and retry for each reached allocation. `run.py` tests ordinary and named CLI,
two unchanged C17 controls, strict native O0/O2, and passive observations of
the actual original addresses, phase/old/alias/current/sibling/result.
This is a bounded capability test, not a general soundness or Product PASS.

FIRST and repair records distinguish the receiver/unit-binding precision
failure, a native C phase/result name collision, and mechanical observer
warnings. A failed v4 probe compile left an old executable; those six outputs
are explicitly marked INVALID for expanded-v4 coverage. Rebuilt v5/v6 tests
provide the additional matrix. Source/oracles and all original RED remain.

Runtime witness count is independent of iteration count: exact entry and
abstract header (2), actual graph/arm worlds (3), leaf/union states (4), their
sealing copies (7), and enclosing entry/post/final copies (3): 19 retained
semantic worlds for this finite actual-call consumer. This includes the
existing linear/stutter sealing machinery; only one additional abstract
header is needed relative to the analogous retained stutter relation.
The full package/node sizes and allocator census are candidate-dependent;
the measured direct allocation PCs include shared clone/seal helpers.

Audit locality remains UNMEASURED; GO624 PAUSED, H1 HOLD and P1036/PR1050 STOP
remain. Snapshot/source-walk/validator/emitter maintenance is a real cost;
independent fixed-candidate F/R and Coordination adoption remain separate.
