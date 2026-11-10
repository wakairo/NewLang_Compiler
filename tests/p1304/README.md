# One helper reused by distinct named callers

The independent H-free source defines `rotate_value` once. Two different
ordinary named caller functions each construct their own `Telemetry` original,
borrow a declared field and call that helper. They read their own postcurrent
and sibling before returning a genuine old u8 Copy to main. Primary writes167,
returns83, sibling211; secondary receives that83, writes83, returns109,
sibling37. Sequential callee lifetimes can reuse an address, so observations
check physical original preservation within each lifetime.

At adopted main202c, ordinary CHECK and both existing public helper
body/parameters/event current validators pass. C17 refuses with exit4. The
outer flat Copy validator excludes child bodies; its historical GET is not a
permission. A second scalar caller also falls outside that flat proof profile.
The complete original source/oracle/FIRST and raw are preserved separately.

The private driver now borrows the full caller-node snapshot already captured
by CHECK for a nested write-u8 Copy call. It authenticates allocation base,
count, root, source and body before node reads, requires the existing CURRENT
validator, then reuses ordinary V0 node validation and scalar emission. There
is no new CHECK issuance, world, public validator/API, owner, ABI or role count.
Selected composed evidence cannot fall back to a weaker pure/flat path.

The admitted family is a normal, branch-free internal u8 caller at the existing
outer scalar depth, with zero or one existing u8 formal, ordinary scalar/flat-T
bindings and source-issued field write-loans calling the existing u8 Copy
helper profile, plus a scalar tail. Existing function/type/depth budgets remain.
Explicit caller RETURN, IF/loop/general CFG, exported stateful bodies and
broader interprocedural signatures are outside this increment. Pure and flat
callee paths retain their previous admission and generated output.

Six registered tests cover genuine public proofs and cross-caller refusals,
current graph/count/short-allocation/world/function rejection, unrestored
A destruction and B survival, strict raw C then passive GCC/Clang O0/O2,
retained-context consumer OOM with identical retry, named-output failure/OOM
no-clobber, and five unchanged old C17 outputs. Consumer injection uses every
fresh measured ordinal up to1024; whole-CLI injection selects at most17 of a
fresh census capped8192. They are distinct measurements, not an exhaustive
whole-CLI failure claim. Clang passive guest programs additionally use
ASan+UBSan; host sanitizer evidence is reported separately.

This is a finite source-composition capability. It does not establish ordinary
safe library invariants, representation hiding, general soundness, Product GO
or a smaller author-plus-caller UB audit/re-audit closure than well-factored C
or a safe Rust facade. Borrowing an existing certificate adds a maintenance
dependency to its internal representation; the scope and negative tests make
that dependency explicit.
