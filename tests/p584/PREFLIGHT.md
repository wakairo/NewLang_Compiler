# P584 — target precondition, not target support

Fixed main af2086216d27acb5fc8f7f745d2d93ee1171973a; CURRENT_SPEC Draft17.32.
Start 2026-10-17 21:14 UTC. Compass → actual Product Strategy1.1/1.3,
Process4.2/4.3.1, Design Decision Procedure/Ledger, canonical5.1/6,
V1 scalar contract and P0 toolchain decisions read. V579's accepted TI C28x
CHAR_BIT16/SDK alias evidence is documentation, NOT an executed TI build.

Preflight decision before implementation: GO for one shared generated C17
precondition, no target ABI/model subsystem. All four emitter surfaces are
src/main.c v0_emit_c, src/array4.c c_text, src/checked_c_node.c and
src/checked_c_five.c. Scalar/Pair has a conditional SIZE_MAX host guard, Node
heap and five have size/alignment/member guards; none establishes 8-bit C bytes.
Six actual checked-source outputs (hashes in evidence/baseline.json,
selected raw *.before.c.txt retained) have no CHAR_BIT check. Unchanged GCC accepts each after a SYNTHETIC preincluded header changes
CHAR_BIT/UINT8_MAX/uint8_t spelling. This is not a TI compiler/data-model run.

Freeze: frozen.json and *.nl were written before baseline execution/edits;
existing Pair, Array1/2, Node and five-root fixture sources remain byte-identical.
Expected bounds runtime Copy returns [0,255], real original field/Option/Array
storage preserved, all guest code byte-identical after deleting just the new
preamble. Reuse existing public-owned and passive physical native tests.
Negative source statuses recorded in evidence/baseline.json; preserve them and
empty stdout. Synthetic CHAR_BIT16 alone, wide uint8_t alias alone, wrong
UINT8_MAX alone, a signed alias, and combined probes must fail at static assertions. Ordinary
host output must compile strict C17/native GCC and Clang O0/O2. Missing standard
exact-width typedef/macro also fails closed. No claim about malicious or
nonconforming C compilers ignoring a required diagnostic.

Historical gate: N/A — faithful exact-width u8 implementation of Draft17.32
5.1/6 and unchanged V1 0..255 contract. KEEP semantic range/source/proof rules
and existing C target/layout guards. DEFER general cross-target ABI and C28x.
No ledger update: no source or language design selected/replaced.

Goal: reusable compiler backend faithfulness across all existing C outputs.
Evidence: exact current emitter mismatch plus host native/synthetic guard tests.
Alternatives: reject unsupported target (chosen), future separately authorized
qualified target lowering, or no use on that target; never reinterpret u8 as16.
Stop: any need for new ABI/source/semantic/authority/model; two Actions head
attempts maximum, one DRAFT PR, no PR576 modification/merge/other Track/H1.
Exploration GO only; Product GO HOLD and C28x unsupported remain unchanged.
