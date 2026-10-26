# P945 — same-source mixed H/T terminal owner continuation

Base `fbbf8a0261669ce0e8920e5b019134268a6a58c1`, canonical Draft17.34.
Gate0 and M944 confirm that an additional caller tail read is optional, while
original root/backing/Allocation/Domain, currentness/provenance, scope closure
and single-consume remain mandatory. P943's unpushed IF consumer is recovered
from its verified local commit; its first terminal STOP remains historical.

The backend retains attach/detach and producer proof fences. After a certified
local LiveTail return it classifies at most one actual caller tail read and the
separate checked receiver-body read by issuer, original, source order and scope.
Every existing terminal owner predicate still runs. Final accounting matches
actual roles instead of assuming a gratuitous third caller operation. This is
only the selected mixed/no-pre-read lane, not general owner or CFG lowering.

The complete P942 r1 inputs and oracles are unchanged. A selects 166 by `6<14`,
named write returns old39 into a different original; B selects52 by `29==11`
FALSE, returns old184 into another original in the two-heap-success arm. Each
executes the real owner local RETURN, whole caller receive and tail/head cleanup.
Optional-read controls are different frozen inputs and never replace primaries.

`run.py` checks normal CLI modes, source SHA, strict raw C, passive live original
observation, actual malloc NULL worlds with 0/1/2 successes and 0/1/2 frees,
semantic/unsupported negatives and a reached output write/flush fault plus retry.
The observer never initializes or repairs guest data. All pointer dereferences
happen before the original C object's lifetime ends; finish uses saved numbers.
`integrity.c` loads only real source, validates existing public Copy receipts,
then checks owned arm/field/world/terminal tamper rejection and restoration.
Two existing driver OOM sweeps verify reached allocation failures, zero partial
C and healthy retry. Five compact CTest registrations cover these paths.

Raw C in evidence/*.c.txt is compiler output, not formatter-normalized C. It is
not a portable ABI. No new issuer/public proof/parser/source semantics or tool
pin is introduced. General IF/effect/ownership and richer mixed inputs remain
unsupported. P-only finite evidence is not independent R or Product GO.

Reproduce: frontend CMake build, `ctest -R '^p945_' -V`; `run.py` also accepts an
explicit GCC13 or pinned Clang23.1.3 and `none/address/undefined` guest sanitizer.
