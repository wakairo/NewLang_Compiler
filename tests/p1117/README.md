# P1117 / C212 — ordinary runtime usize predicates

Exact base main65055be / tree468bcea, canonical Draft17.34. The source checker
now accepts dependency-free unknown usize equality/order and records a typed
Unknown Bool. It retains the dependency guard; unknown bits do not prove
equality, pointer/root identity, an owner, a current state or a loan.
Existing IF checking evaluates both arm transfers and structurally joins them.

The private C17 consumer admits one ordinary one-usize-formal/u8-result pure
callable, scalar Copy aliases, equality/order, and one two-normal-arm u8-literal
selection. It authenticates the current registered body/signature/source,
actual argument/formal package, every used checked scalar and IF child lineage.
Both arms are checked; runtime evaluates the actual predicate on the guest
parameter and selects one arm. It creates no permission or semantic state.
The internal static C function uses the existing native-size_t quantity target
guard. Public source/API/ABI, owner/issuer, H custody and toolchain are unchanged.

The profile excludes effects/refs, multiple formal arguments, nested calls/IF,
general Bool/effects/CFG architecture, and true LOOP induction. General source
IF in a main callable is still outside the old root-only C17 consumer (source03).
Sources10/11 are legal but outside this finite consumer and must leave output
untouched. Source07 is byte-identical P1115 input02: CHECK must still refuse its
unproved T-frame H_loop. This work does not implement iterative CONTINUE.

V1/V2 full pre-execution sources/oracles/harnesses and first raw are archived
unchanged. V1's known-IF C17-success prediction was wrong; its real exit4 and
harness RED remain. V2 split CHECK/emission expectations and calibrated the
known-Bool/no-IF path. V3 adds wide quantity, effect/arity refusal, actual current
source/world/signature/graph mutations, canonical A/B destruction, explicit
fixture-only dependency checks, exhaustive reached API/named-CLI single OOM
failures, no-clobber and disabled-fault retry. Every new version is frozen before
execution. Fixture seeds never supply ordinary CLI/physical authority.

The passive native variant asserts actual helper input/returned values for
true, false, boundary and >255 cases. Raw output runs separately with strict
GCC/Clang O0/O2. Sanitizer flags apply to both host and generated native paths.
Numbers are independent P oracles, never observed values promoted to facts.

Legacy P504/P603 rollback units originally expected unknown formal comparison
to be precision-rejected. The preserved originals retain that expectation;
P603's first new-candidate RED is archived. legacy_current.py pins both source
SHA256s and generates a prospective translation unit changing only that final
precision expectation to actual registration + typed Bool invocation checks.
All original parser/value/type/loan/write/rollback/OOM checks otherwise compile
verbatim. No checker return value/diagnostic is faked. The original C files are
not edited. This is an explicit current oracle version, not a claim that the
original precision oracle passed.

Prospective history v22 externally pins main's v21 SHA256/Git OID and inherits
all protected data. Original v1-v21 manifests and all FIRST/RED remain exact.
P1107 gets only an appended harness v7 for the legitimate CMake successor;
its old input versions/source/oracle remain intact. The forward history runner
preserves old assertions and selects the current generation. Only Git-managed
files enter the inventory. RUN_SERIAL on p990_freeze_protocol.unit is retained.

P1036/PR1050 STOP persists. No R source/oracle/fixture/raw/values or M1118
conclusion is an input. Capability evidence is not general soundness or measured
NewLang-vs-well-factored-C UB audit locality; Product GO624/H1 remain held.
