# R1163 independent finite runtime-to-RETURN evidence

Authority: `449ccf0dc567c05f3fd6dc4430b265759e22d571`, CURRENT_SPEC Draft17.34.
Compiler: `9030055314600b804fafde64adebb03c4d6315a1`, tree
`261a8e404a1cf1982e13d3b94e3223d5da0e0499`.

Independent sources and mathematical expectations were frozen before producer
implementation, fixtures, expected outputs, or old R/F conclusions were read.
V1 and V2 are retained verbatim as **NOT_RUN**. C clarified only the supported
progression surface before execution. V3 uses direct `continue(cursor +/- 1)`,
parameter-left/literal-right guard, and actual `break cursor`. Its numeric and
trace expectations are unchanged from V1. The Issue freeze records are the
pre-execution authority; no output-derived oracle changes are permitted.

Six positives cover ADD/SUB, 0/1/3 backedges, both RETURN-arm positions, both
fixed child indexes, early return and normal replacement. Seven negatives are
required to reject in parse/registration/check and never reach owned/C17/native.
The public probe observes only public source/checked/context interfaces. It
validates ordinary result-Copy-comparison lineage and genuine RETURN normal
replay with original root/child/sibling identities and facts.

`driver.py` is Actions-only. Every positive must pass CHECK, actual public-owned
observation, identical ordinary stdout and named C17 output, strict C17 builds,
and native execution under GCC and Clang at O0/O2. Host and guest sanitizer
labels are separate; ASan/UBSan jobs add the same sanitizer to each guest lane.
`observer.py` derives passive insertion points from each actual Clang AST. It
never replaces values, conditions, arithmetic, stores, or expected output. It
reads live initialized carriers and original addresses; after return, only
counters are read. Unsupported AST shapes fail closed as machinery failures.

Every command and its return code/stdout/stderr, the original and instrumented
C, AST, observer map, inputs, frozen oracle, and per-lane receipts enter a
compressed archive. `R1163 ARCHIVE_BEGIN/CHUNK/END` records preserve it losslessly
in the normal Actions job log. Concatenate chunks in numeric order, base64
decode, and verify the declared SHA-256 before extracting. Both successful and
failed runs archive evidence. The first failed outcome is never overwritten.

The only root CMake change includes `register.cmake`. It adds this independent
CTest and relocates exactly two historical hash-checker launch paths, p965 and
p982, to a complete byte/mode-verified fixed-P tree. Those scripts, their
assertions, expected hashes, imported history, production sources, and other
CTest commands are unchanged. The snapshot does not contain a new expectation.
It is verified against all 5,992 Git blobs from the immutable P tree.

No local compiler, CMake configure, formatter, CTest, or native execution is
part of this work. Pure text syntax/hash/mock checks do not count as R evidence.
R-FAST is format/build/registration only. Normal FULL/PARTIAL and their raw
receipts are required before any validation claim. No merge or Ready action.
