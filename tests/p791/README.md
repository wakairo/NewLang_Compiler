# P791 — ordinary CLI direct Array4 constructors

Fixed base main `77d83dfc00850b14fd7c3bc95e7e3c12e34161fb`, Draft17.34.
New source/oracle/SHA freeze `c15a3fd` predates compiler edits.
`one.nl` models four initially vacant receipt slots; `two.nl` models independent
receive/transmit slot arrays. Both bodies contain only direct construction and
unit. No element access, Copy destination, option-returning helper or Ring is
implemented. This is a compiler candidate, not Product GO.

Gate0: ordinary parser refused all frozen cases with REC-TYPE-PROFILE, whereas
historical opt-in provides genuine source-issued construction proof. M743 and
DI016/008 keep direct construction selected and C/E/F eligibility undecided.
An unsupported implementation form diagnostic is already a distinct public
parser status/category; it need not invent a new language prohibition.

The normal parser now admits Option<u8> only while reading a direct Array4
initializer. A no-allocation syntax support fence requires one or two direct
bindings plus unit, in main's unit or retained-body reparsing. It verifies the
whole physical source, including uncalled definitions; additional declarations,
Array Copy, bare Option and Option-returning forms remain explicitly unsupported.
This is deliberately the same closed full-body graph the existing public
validator can authenticate. 3/5 element arity and wrong element types proceed to
existing semantic checks. Macro default remains OFF; historical opt-in parser
and its broader tests are unchanged. No source issuer, public validator, C17
emitter, type-layout/ABI, toolchain or workflow modifications.

Build frontend-only then `ctest -R '^p791_' -V`. K5 test names frozen in manifest;
FULL5 expected448 each, PARTIAL447. Public probes are separately linked to the
NORMAL parser, then register/retain/reparse actual source and discard original
AST/source. One original root has4 dependent initialized Option places. Two
originals have distinct root/incarnation1/6 and8 distinct children; checkpoint
worlds are distinct, cumulative5/10 places+values, no domain/backing/scopes.
100 two-root view/foreign-world mutants and40 singleton mutants are rejected;
no synthetic positive world. Existing validators and their checkpoint-memory
cost are unchanged (two cumulative snapshots contain15 place/value entries;
this retains the existing growing-prefix design, not a memory-cost theorem).

`native.py` checks normal CLI and named-output parity, semantic versus unsupported
negatives, strict GCC/Clang O0/O2 unmodified C17, then a separately labelled
passive observation derivative. The derivative only reads original addresses,
sizeof Option stride, all None tags and disjoint carriers; it does not initialize
guest values/authority. Generated C is archived as `.c17.txt` data, preserving
bytes rather than formatting generated output as maintained source.

OOM: actual parser each allocation position with reusable-parser recovery;
registration and entry issuer each reached position with unchanged semantic
snapshot/artifact NULL; existing atomic-output test injects allocation failures,
fflush/fclose/rename faults and verifies target preservation and private-temp
removal. No global filesystem atomicity or exhaustive arbitrary fault guarantee.
Historical test source/oracles retained; only two live default-OFF assertions
migrated to ordinary-versus-opt-in CHECK parity, old bytes archived as data.

Historical Gate: KEEP DI001/005/008/016, Draft17.34 selected direct constructors;
N/A new source/semantic selection. DEFER generic Array/Some, C/E/F eligibility,
index/ref/Copy owners/Ring. Goal Alignment: normal CLI availability is useful
compiler groundwork; safe library API audit/re-audit closure versus well-factored
C remains unmeasured. Product GO PAUSED/H1 HOLD.
