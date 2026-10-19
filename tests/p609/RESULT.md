# P609 local pre-push evidence — remote CI pending at this snapshot

Authority: main2cca0c6118cdf725ee68b48e07fc566b29622230, Draft17.32. Remote original PR604bdb505d, local starting382feda and exact P608 bundle/patch verified. P603/P605/P608 terminal reports/evidence remain historical.

## Implementation
Only new production change is src/main.c: for existing validated flat usize less-than, materialize original source operands into const size_t left then right, then compute the real _Bool comparison and retain existing Copy binding. Names derive from actual checked operand node IDs. Both lexical values and typed literals retain original guest data; no checker's known bool emitted. Existing bool/source authentication, root0/no authority, target-size guard, +/- / == emitter remain unchanged. Scalar statement and binding emission sequence the temporaries before use; direct less-than IF/ABI/general expressions remain unsupported. No new syntax/public issuer/ownership/effect/CFG. Two scalar local C temporaries are a source-level lowering cost; no native performance or zero-cost claim.

## Actual evidence
Old P603 max_false C snapshot freshly reproduced GCC13 strict compile1/type-limits/nativeNULL. A C-template feasibility probe then compiled/executed9 old snapshot shapes ×2 compilers ×O0/O2 ×plain/passive=72 runs. This preliminary probe is explicitly template evidence, not the compiler result. The first version of that probe script had a Python regex group-slice error before any C trial; corrected as test tooling, not a compiler or semantic finding.

Actual integrated compiler/probes rebuilt from current production source. Frozen27 original NewLang sources/oracles unchanged. All9 original source patterns (literaltrue/false,equal,zero,MAXtrue/false/equal,namedtrue/equal) now source/CHECK0, original registered checked-owned public operand/bool views after AST teardown, private gate22 corruption refusals/snapshot unchanged, C17 emission0. Actual strict GCC13.3.0-16 and Clang23.1.2 each compile+execute all9 at O0/O2, both plain guest and passive observer; all native exits0 and both original operand carriers/result/Copy observed. Generated-C hashes in raw logs exactly match nine fresh P609 snapshots. All18 frozen source/semantic/precision negatives keep CLI3 and Cbytes0, including unknown uintptr type stopping earlier than original TYPE oracle (preserved calibration).

Per compiler:54 synthetic trials,18 SAME_TRUTH native0 and36 TRUTH_CHANGING native91; no new compile-refused trial because the genuine staged shape fixes those warnings too. Historical boundary mutant refusals stay recorded under P605/P608. Policy still reports either mathematical class's synthetic-only compile refusal with nativeNULL, never exempts actual originals, and requires runtime91 per <= / > / Copy family on nonboundary frozen cases. Phase unit verifies deliberate original compile failure, wrong compiled observer exit and missing runtime family all fail. Deliberate #error unit is not NewLang native evidence.

Supplementary P605 composed +, -, < cases pass actual GCC13+Clang O0/O2 public existing +/- revalidation and ordinary copied bool with no authority;4 mixed-operation forms remain source3/Cbytes0. Main production shared arithmetic implementation/headers and actual semantic arithmetic capture body are byte-identical to fixed main, verified separately by SHA and direct git comparison.

Local entire frontend-only/C17 suite: GCC14.2 Debug332/332 PASS, Clang23.1.2 Debug332/332 PASS. This is PARTIAL, not the locked LLVM23.1.3 FULL matrix. Tests include accepted +/- / equality, Array4,u8,Node, original P603 error/rollback/OOM and P605 composition. OOM injection observed parser16, semantic32, registration101, actual-main44 allocation-failure prefixes, semantic/public state rollback PASS. Full C formatting PASS. Compiler source and harness were not modified after these final suite runs except test snapshot validation was added before the relevant full-suite integration tests ran. No new sanitizer/FULL CI claim yet.

All old27 sources/oracles, old9 C snapshots/hashes,61 P603 records and P605/P608 history preserved. New9 C snapshots, exact old/new diff, source/old/new SHA manifest and raw logs are under evidence/. verify_evidence.py rechecks preserved history and exact adopted arithmetic region. New guest C bytes are intentionally versioned separately under C609 authorization.

## Reproduction
```sh
. /workspace/NewLang_Compiler/.deps/activate.sh
cmake --build build --parallel 2
python3 tests/p609/verify_evidence.py
python3 tests/p603/integration_test.py build/newlangc build/p603_public_probe build/p603_gate_probe /workspace/p605-tools/usr/bin/x86_64-linux-gnu-gcc-13 tests/p603 none
python3 tests/p603/integration_test.py build/newlangc build/p603_public_probe build/p603_gate_probe clang-23 tests/p603 none
ctest --test-dir build --output-on-failure
bash scripts/check-format.sh
```
Exact local GCC13 package provenance retained in P605; Debian13.3.0-16 is distinct from Ubuntu GCC13 CI, local Clang23.1.2 distinct from locked CI23.1.3. No system install/lock/bootstrap/workflow change.

One new natural FULL5+PARTIAL Actions pair will run on the final pushed PR604 HEAD, no rerun after failure; this committed record predates that pair. Actual CI IDs/outcomes belong to the one terminal Issue609 report, not an invented green entry here. DO NOT MERGE.

Goal: reusable strict-C17 quantity comparator; first-stage feasibility succeeded without new proof/semantic machinery. Historical gate N/A faithful lowering/mechanical tests; KEEP canonical total <, Copy/LTR and authority boundary; DEFER broad grammar/CFG/effect/owner. Product Strategy1.1 author+ordinary-safe-caller UB audit/re-audit locality versus factored C UNMEASURED, no Product/H1 GO and no distinct goal deviation. Scope ends at fixed-head CI/result report.
