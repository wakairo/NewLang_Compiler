# P534: GO FURTHER CANDIDATE — SPECULATIVE / NOT ADOPTED

Fixed compiler/main c6ae58512683090b2d98b09f51765143b1517345; canonical CURRENT_SPEC remains Draft17.30. Source design reference ONLY: unmerged PR533 head817a5534b131b9b70b5d37f78a4bad1e39ee4ad4, complete proposed Draft17.32/DI016. Neither candidate docs nor current specs/ledger are edited or imported. FormalProof5437f92d099e228773d31e2323e8ee6228ca21bf is informational, no formal claim. Freeze commit584ee76 records source bytes, SHA256 and oracles before any execution or Production edit. Original source SHA256:0e032e49f78028fb69549b1120a060dd4fe84ae1073d25925ba679d3d8513801. All six frozen sources remain byte-identical.

## Measured frontier and actual implementation

Baseline clean fixed-main frontend-only Debug: every frozen input exitsCLI3 at parser P5-EXPRESSION-UNSUPPORTED, `<` at2:22–23; no semantic checking, owned or C17/native success. This confirms the forecast: nested concrete builtin/Array source is the first barrier, not semantic ownership rejection. The implementation is one bounded constructor attempt, not a chain of workload variants.

- StageA: real positional AST vector using existing owned-node/argument-next machinery; exact experimental Option<u8>/Array<Option<u8>,4> parsing. Vectors of3/5 and wrong element types reach actual semantic registration. Canonical type cache uses name-less builtin Option with Some payload u8 and structural **NL_TYPE_ARRAY**, element_type/count4, not a fake Array4 nominal. Existing member packages/fixed-child lifetime kernel is reused; numeric internal index labels are not named source fields. Source `@field` selection explicitly excludes Array; no element projection/source access is added. Static Copy/Discardable derives from all variants/element type, never current None. N/T are not generalized and future N=0/nonCopy are not semantically outlawed.
- StageB: immutable source-issued event record plus owned live-placement checkpoint, public read-only view/world and independent source/type/value/parent validator. The validator checks retained body owner, actual source token vector and each source node, structural type keys, one original root/four distinct dependent typed places, facts/incarnations/current None/member ownership and complete closed frame before the existing invariant kernel. It does not accept a host world, guessed raw IDs or a forged nominal. The final lexical world is ended; validity of the captured event is NOT a new current ref/owner/Domain authority.
- StageC only after StageB: public C17 consumer authenticates the unit entry/body, public placement proof and single-binding body. Emits one C Array of complete Option envelopes in the actual source function, with compiler-private symbolic None tag37; no memset or zero-bit assumption, four typed initializers, no temporary live Array root. Normal C is emitted uninstrumented and runs without guest I/O. The explicit C representation choice is opaque, not a language discriminant/foreign ABI guarantee.

Default source admission stays OFF in `newlangc`. `newlangc_p534` is a separately named TEST-ONLY executable linking a macro-enabled build of the SAME Production parser and shared actual semantic/owned/C17 code. The only CMake changes are these dedicated test targets/source wiring; no workflow, toolchain or dependency download changes. BUILD_TESTING=OFF Release also builds and retains parser refusal. This is a SPECULATIVE implementation of PR533's proposed source rule, not new source adoption on the P branch or main.

## Genuine source-issued observations

After original input source/AST teardown, public APIs report:

- structural Array type11, Option element type10, static Copy/Discardable;
- one source binding1 / original root1 / incarnation1 / fact1 / package5;
- exactly five live places in the captured world: root1 plus four NON-independent fixed children2–5, incarnations/facts2–5, source nodes3–6, distinct typed None values1–4 under parent package5;
- no Some payload or occurrence, no explicit Allocation/Storage/BackingRegion/Domain grant; local backing/governing relation uses the existing compiler-managed implicit place/incarnation identity (explicit region/domain IDs remain0). This is not a newly minted external region/owner.

Actual public view copies reject40 separately corrupted root/child/fact/incarnation/type/source-origin candidates, including foreign actual-source artifact despite equal numeric IDs. Twelve separately cloned, test-only captured-world corruptions (mis-parent, cloned sibling, ended incarnation, altered fact/value, wrong static Some type, nominal impostor, out-of-range type/value) reject through the PUBLIC validator and before C17 output. Missing issued record and foreign retained source plan reject; restoring genuine evidence passes. Immutable event facts prevent a jointly altered copied fact from becoming original issuance.

## Semantic negatives, OOM and physical native

Experimental parser0 for all five negative sources; registration/semantic stage rejects and public snapshot rolls back with no artifact:

| Source | Actual semantic-stage diagnostic |
| --- | --- |
| arity3 / arity5 | ARRAY4-INITIALIZER-ARITY |
| wrong_type u8(1) | ARRAY4-ELEMENT-TYPE |
| payloadless Some | P6-CONSTRUCTOR-SHAPE |
| Some(u8(1)) | ARRAY4-OPTION-VARIANT — unsupported closed candidate source profile, NOT a claim that canonical Some is intrinsically ill-typed |

Native GCC14.2 and local Clang23.1.2, strictC17 O0/O2: ordinary unmodified generated C exits0. Separately labelled passive observed-C reads the SAME original local Array while live, one original object address, four distinct contiguous element starts, extent4*sizeof(Option), observed stride2/opaque tag37 on these hosts. Observer writes only its own I/O/address records, never guest values or authority. It does not read inactive payload or assume a public tag encoding. O2 may eliminate the unused Array in unobserved code; physical observations are from observed-C, not an assertion of mandatory unoptimized machine storage.

Per host:4 successful native executions (plain+observed O0/O2),6 bad-C refusals (wrong None tag, missing typed initializer, wrong Array extent). ASLR addresses are expected to differ; stride and semantic envelope/original symbol mapping are checked. No NewLang element access, index4 test, Ring API or nonCopy Frame was run. `generated.c17.txt` and `observed.c17.txt` are distinct replayable artifacts, not host-created substitutes.

Source/definition registration OOM sweep114 and actual main() OOM sweep65 allocation sites: rollback PASS and failed checked artifactNULL. Public validator allocates0; C17 staging malloc failure1 leaves bufferNULL/length unchanged and semantic public world unchanged. Final targeted tests4/4 PASS, local partial regression311/311 PASS. Local LLVM23.1.2 is only the available frontend/native host, not a substitute for the unchanged locked23.1.3 FULL5 gate. Final-head natural FULL5+NO LLVM/PARTIAL initial results live in PR checks and the one terminal Issue534 report; no manual CI or flake shopping.

Initial work-in-progress parser probes (`stage-a-first.json`) exposed local stale-token/DIGITS handling mistakes, corrected before successful StageA without changing frozen sources/oracles. These are not additional accepted-language limitations. Later Some-with-payload diagnostic was classified as semantic-stage source-profile UNSUPPORTED, preserving constructor/type errors separately. Logs named local-p534 are whitespace-normalized CTest display; actual source/C/JSON payloads remain unchanged.

## Reproduce / authority boundary

```sh
cmake -S . -B /tmp/p534 -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/p534 -j4
/tmp/p534/newlangc --check tests/p534/original.nl       # default-OFF parser refusal
/tmp/p534/newlangc_p534 --check tests/p534/original.nl  # SPECULATIVE CHECK0
/tmp/p534/newlangc_p534 tests/p534/original.nl          # actual ordinary C17
ctest --test-dir /tmp/p534 -R '^p534_' -V
ctest --test-dir /tmp/p534 --output-on-failure
```

Frozen original versus input injection: EVERY positive/negative comes from the original .nl bytes, production parser, ordinary unit registration and real main() check. No seed_root/value/ref, fabricated Array context, host constant guest update or C fixture substitution. Test-only corrupt worlds are CLONES of genuine checkpoints and count solely as refusal evidence, never success.

## Cost / goal alignment / STOP

Observed scope:707 net Production C/header lines, plus24 net CMake target wiring and test-only snapshot comparison extension; no generic trait/mono engine, unbounded AST rewrite, raw/heap owner issuer or indexed projection. Source/type production was insufficient alone: original child placement proof/source validation and a real Array C carrier were necessary distinct planned stages. These fit one closed source attempt with shared existing kernels; broader types/operations remain first independent future frontiers, not automatic next steps.

Historical Gate: KEEP Draft4/18.6/25/26, DI001/008; DEFER DI002/005/006/007; DI016 stays PROPOSED because CURRENT_SPEC is17.30. This is NOT normative N/A: new candidate source eligibility is tested only under explicit user/Issue534 speculative authorization. Original H1, STB, old owner grants and PR533/531/273 remain unchanged.

VALUE-ALIGNMENT-RISK: genuine original typed Array construction is a reusable technical rung for the newly designed tagged Ring4, not a full Ring or ordinary-safe API/product PASS. No human L/R/P/V/C audit/re-audit obligation reduction, nonCopy owner identity/refund, performance advantage or caller burden improvement is demonstrated. Same-topology safe Rust Option Ring and factored C/Zig are real alternatives; all-None intrinsic remains deferred, and a four-field noncontiguous queue has a different physical contract. Product GO stays HOLD. Coordination decides separate canonical selection, independent R and any future source indexing/owner costs.

DRAFT / DO NOT MERGE. No main/canonical/PR533 changes, self-close, merge or Track launch. One terminal Issue534 report, then STOP.
