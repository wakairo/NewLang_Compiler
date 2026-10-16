# P521 bounded composition candidate (UNSELECTED)

Authority: compiler/main `a513b48484b890c0d8cee6dcc7766414075b33ee`, canonical Draft17.30. Input/oracle freeze `bcc969929be100ba860b6a5cf56235d2f7413dfc`, unchanged-main evidence `716e303`. Source SHA256: frozen.json. Baseline binary SHA256 `4105ae0a8d740d85e7e95f8651f648977871fee583f9e1f3525baa8b84d31ac9`; candidate binary/detailed CLI stages: candidate.json.

## First barrier and bounded repair

Eight positives parse/check0. Literal→relay and direct IF→original-Pair-write emit C170 on fixed main; IF→relay with/without Pair emits CLI4, V1-BACKEND-UNSUPPORTED, zero C bytes. Both pure normal arms are checked; joined Copy is unknown. Three semantic negatives parse then fail registration3 with rollback and no checked artifact: invalid ELSE u8(256), usize actual for u8 formal, unit callee body for u8 result.

Read-only temporary instrumentation identifies exact first failed predicates: original main.c571 copy_if_selection default (REGISTERED_CALL), and997 copy_if_pair_frame default (same call). Existing source-issued call metadata already proves fresh actual/formal and retained body result. This is a driver-consumer mismatch, not a semantic/P9 precision refusal. Public observer first assumed retained caller/callee NLSource pointer equality; body-source copies are legitimately distinct. This harness-only assumption was corrected, without changing any source or oracle; existing call-body ownership check authenticates the exact registered source.

The repair reuses existing v0_validate_call, follows source Copy origin through one authenticated call, accounts callee-issued Copy result IDs and its formal/local binding facts in the existing finite IF frame census. No emitter code is changed: existing C call/result rendering is reused. Production C delta75 added/14 removed =61 net lines, plus16 CMake test-registration lines; no issuer/checker/public API, source syntax, new proof issuer/validator, registry, flags, workflows, locks or canonical changes. Root declarations, two owned arm worlds, Point-in-time field facts and closed lexical loan predicates remain mandatory. A source-less additional value cannot be hidden by the body-value census; body values come only from the authenticated retained body. Nested callee calls inside this composition remain outside the admitted finite frame.

## Source-issued and native evidence

Public views show original Pair root3/inc3 and children4,5/inc4,5 unchanged in BOTH pre-IF worlds; original left37/right211, root facts3→11, left4→10, right fact5 retained. Join9 unknown; real actual10 copied into fresh formal5 retaining package10; retained relay body returns11 unknown; scope1 closes; field new package14 remains unknown. Public fields and call/body/result accessors are actual source-generated artifacts after input AST destruction. Existing PRIVATE driver validation is separately exercised; this is not a new standalone public Pair/call validator. Ended final lexical roots are not called live poststates; this is local implicit lifetime, no Allocation/Domain authority or owner transfer.

All eight positives reach C170 with unchanged source bytes. GCC14.2 and local Clang23.1.2 strictC17 -O0/-O2 execute ordinary generated C. Passive observed-C additionally checks the same original root/field addresses while live, old37, selected/actual/returned/after108 or173, sibling211, and one actual emitted callee-body execution. Only observer counters/address records are written by observation; guest data/authority is never injected. C compilers may inline machine code: evidence is authenticated C17 call plus executed callee body, not a mandatory assembly CALL opcode.

Each combined input rejects six independently corrupted C outputs: callee return, condition, arm selection, root, field, sibling. Call-only/literal controls also reject wrong callee return. Per host compiler:26 successful native executions (plain+observed),30 corrupt-output refusals. Direct no-call baselines get ordinary native here; their passive physical observation is retained in the prior P517 integration test. C17 bytes, observed-C copies, CLI and public-view logs are preserved under evidence/; JSONL distinguishes ordinary native from observed-C.

Existing P517 gate rejects78 field/arm/identity/scope mutations per combined input; new call gate rejects19 body/registration/formal/result mutations per call-choice/combined input and detects missing/foreign body plans. Gates restore original PASS and leave evidence/worlds unchanged. Existing OOM sweep at registration314 and main151 allocation sites: rollback PASS, outputNULL on failure. Read-only driver admission allocates zero. Local frontend-only regression307/307 PASS; full LLVM gate remains separate. GCC native, Clang native and targeted verbose CTest evidence are checked in; verbose CTest log has trailing display whitespace normalized.

## Reproduce

Use a clean build with an appropriate GCC/Clang environment:

```sh
cmake -S . -B /tmp/p521-build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/p521-build -j4
ctest --test-dir /tmp/p521-build -R '^p521_' -V
ctest --test-dir /tmp/p521-build --output-on-failure
```

Full5 requires the repository's unchanged locked23.1.3 SDK. Local SDK23.1.2 is used only for explicitly PARTIAL frontend/native evidence, never to bypass the full gate. Natural fixed-head full5+partial CI results are reported in PR checks and the single terminal Issue521 comment; no routine reruns or manual dispatch. No historical frozen input/oracle was modified.

## Cost, coverage map delta and goal alignment

Original prediction: possible genuine CHECK3 unknown-argument precision or one finite backend-consumer gap. Measured: CHECK/owned already suffice; existing IF consumer excludes call nodes, body values and formal facts. Bounded connection fits61 net production C lines with unchanged emitter; general scalar ABI, named-ref helper and public summary remain unselected. Coverage-map delta for Coordination: unknown pure IF u8→one ordinary registered Copy call→one original lexical Pair write is a newly measured composition; its separate rungs alone were insufficient evidence. No coverage-map edits here.

VALUE-ALIGNMENT-RISK (separate from technical candidate): future safe library refactoring motivates this crossing, but real workload/library-author+ordinary-caller UB-audit locality advantage over equivalently factored C/Zig/Rust is unmeasured. Existing direct selected→write and constant→relay remain cheaper alternatives for their respective programs. No Product GO, arbitrary effects, owner/heap/Array/CFG, H1/STB change or new human-proof obligation is claimed. KEEP Draft18/19/20/27 and DI001/006/007/011; historical normative delta N/A only because existing source-issued facts are consumed. Any new summary/semantic choice returns to Coordination. DRAFT / DO NOT MERGE; stop after one terminal Issue report.
