# R1129 C221c frozen execution contract

Authority: Issue #1129 comment 6614995406. GitHub dedicated branch plus natural
Actions only; no compiler/native/build/test execution in Codex or shared executor.
Execution is PROFILE-CALIBRATED / NON-BLIND to declared structural scope;
implementation and fixtures remain unseen.

## Fixed inputs and identity

Canonical main: 73ef10a97f2fab7b70d094b2114359e961acefb5, Draft17.34.
P target: dc21cf33105a7dba832e197f24e153ad87862a0f.
P tree: 34c85ebdebb0fe601d20856ad3f5f3a5446362e0.
Base branch: p1123-c219-stuttering-loop-continue.
R branch: r-fast/r1129-c221-stutter, initially absent.
Initial R commit has only the fixed P commit as parent. Complete blobs/tree/commit
are prepared before the first ref. No existing production/include/canonical,
workflow, toolchain, root CMakeLists, guard, test, raw, or ref is changed.

All nineteen files from V1 comment 6614215197 and V2 comment 6614900934 retain
exact bytes. The only mapping is logical r1129/... -> repository evidence/r1129/....
V1 manifest SHA256: 70134add33049ac8f3f09e442b55a89d3aecadc46f79a8ac7ce9f0c60299fe31.
V2 manifest SHA256: 3555c90c0fac9604ffe70a05450b23204e021222317292015fb9a0b6f6491a95.
The execution harness manifest lists each complete source, oracle, plan and new
harness file with exact byte count and SHA256. It excludes itself; its own
identity is recorded in the enclosing Issue freeze and runtime receipt.

## Registration and commands

New root CTestConfig.cmake defers one public-library observer target and one test:
r1129_c221_stutter.integration. The observer links only newlang_semantic and its
public transitive dependencies; existing strict flags and sanitizer options apply.
An ALL target runs only ctest -N -R ^r1129_c221_stutter[.]integration$ so FAST logs
contain the actual unique registration. It never runs source or native code.

The natural PR job invokes:
python3 evidence/r1129/harness/run.py
  --root <checkout> --output <build>/r1129-raw
  --cli <build>/newlangc --owned <build>/r1129_owned
  --compiler <CMAKE_C_COMPILER> --compiler-id <CMAKE_C_COMPILER_ID>
  --sanitizer <NEWLANG_SANITIZER> --build-type <CMAKE_BUILD_TYPE>
  --frontend-only <NEWLANG_FRONTEND_ONLY> --host-cflags=<CMAKE_C_FLAGS>

No shell expansion is used by the Python command runner. Every exact argv,
working directory, timeout, return status, stdout and stderr is saved before
interpretation. Runtime source/harness hashes, actual checkout commit/tree/parents,
Actions run/attempt/ref/event, versions, host binary hashes, flags and sanitizer
environment are recorded. Runtime checks require the authorized natural PR branch.

Each primary is a fresh complete unit, normal first then immediate_break:
1. newlangc --check <source>
2. r1129_owned <same-source>
3. newlangc --emit-c17-to <raw>/<case>/unmodified.c <same-source>
4. <C compiler> -std=c17 -Wall -Wextra -Wpedantic -Werror <sanitizer flags>
   -O0/-O2 unmodified.c -o unmodified.O0/O2.exe; execute each.
5. Existing clang-23 or clang -std=c17 -Xclang -ast-dump=json -fsyntax-only
   unmodified.c supplies a public C AST. No SDK is installed by this test.
   Missing Clang or unsupported mapping is an explicit observation limit.
6. Create a separate passive observed.c and diff/map; compile and execute O0/O2
   with the same compiler and applicable sanitizer flags.

Each native process has a hard 2-second timeout. Other command timeout is at most
30 seconds, driver total budget 240 seconds, CTest timeout 300 seconds. No retries.
The actual matrix compiler supplies guest GCC/Clang evidence; ASan/UBSan flags are
also applied to both generated variants, not inferred from host instrumentation.
For Clang23.1.3 only, the existing public compiler-ci workflow policy permits
-Wno-error=gcc-install-dir-libstdcxx; no other warning is relaxed.

## Genuine observations and explicit limits

The public observer loads/parses/registers the actual complete unit, parses an
ordinary main() call, and checks it in that registered context. It reads only
public source-issued owned nodes, bodies, type/value/binding/place/field views and
the explicitly limited direct-Break provenance accessor. No seed/root/proof is
fabricated. Joined reference alternatives are reported unobserved unless a public
singular-reference view is valid. Borrowed bodies/worlds are never separately freed.

There is no universal public current-valid or source-to-C storage-map API.
Historical ended local views are not live current assertions. Semantic IDs remain
world-qualified. The source-issued graph and CLI C emission are independent
genuine checks of identical bytes, not the same artifact object across processes.
Unmodified C execution supplies exit/sanitizer evidence, not injected field reads.

The passive variant maps only its own emitted C using actual write destinations,
two live unshadowed local storage objects and declared two-u8 fields. It inserts
read-only observations only at direct compound-statement sites. Unsafe or unknown
aliases, helper effects, phase expressions, scopes, layouts, macro locations and
byte offsets are refused. It never writes guest expected values or borrows P macro
names. Addresses are compared only within each passive execution while storage is
live. That finite mapping is not a universal issuer certificate or a claim about
addresses in the unmodified process.

Frozen V2 values remain: phases normal [4,19], immediate [19]; Continue counts 1/0;
one normal Break. Initial original(signal,checkpoint)=(146,37), bystander=(89,211).
Break helper replaces original.checkpoint with 58 and returns old 37. Caller writes
original.signal=37 (old146), then bystander.checkpoint=58 (old211). Final original
is (37,58), bystander (89,58). Main returns unit. Source-issued known facts, when
exposed, must agree; unknown facts are limits, never invented assertions.
No finite trace establishes all-H induction or general termination.

## Focused probes and first layer

After primary observations, equal_bits, loan_escape, continue_type, continue_arity
and continue_target each receive fresh CLI CHECK and public observer attempts.
Record the first actual parse/register/check status/code/span/message. Named
emission after source refusal must preserve exact sentinel:
R1129-V2:r1129/v2/<case>.nl:KEEP followed by one LF.

Arbitrary nonzero status is not targeted-negative PASS. Earlier profile/parser/
registration rejection leaves the intended semantic boundary NOT REACHED or
unclassified pending raw review. The canonical-legal equal-bits Change is outside
pure-C scope; unexpected wider acceptance returns for C review and is not by
itself an unsafe finding. Illegal-source acceptance is a material STOP.
One optional wider V1 stutter CHECK is separate; out-of-profile refusal is not
failure of the primary V2 scope, and acceptance is not owned/C17/native PASS.

## Raw, repair and stop

Append-only command records and FIRST.json preserve the first failure. The
R-only CTestCustom post-test command transports all R raw files to job logs with
size/SHA256 and base64; larger files use gzip+base64 with separate wire identity.
Executable content is hash-only, with exact compilation/launch records.
No other tests are filtered, ignored, weakened or marked successful by transport.
Reconfigure retains an identical R-owned CTestCustom file and refuses foreign bytes.

Stop the affected execution at the first unexpected semantic/value/storage/issuer/
result/layer, sanitizer, identity or authority discrepancy. Observer resource,
output, mapping, parser and registration defects are reviewed as mechanical or
coverage limits, not automatically Production defects. Safe R-only mechanical
repairs preserve old bytes/raw and freeze new bytes before any rerun.

FAST maximum6 natural push runs, counting initial branch creation and every state/
attempt/page. Check ledger after initial ref, before additional pushes and before
the single draft/never-merge PR; checkpoint after run4 before run5. Natural PR CI is
initial FULL5 plus PARTIAL, with at most one mechanical repair set (max12 jobs).
Manual rerun/dispatch0. First trustworthy all-green ends optional execution.

Optional non-triggering r-archive/r1129-c221-final is at most one ref, with a commit
parented by final tested R HEAD and only raw/manifest/receipt additions. Original
sources and tested HEAD remain unchanged. Verify archive Actions0.
Value hypothesis remains UNMEASURED / GO624 PAUSED / H1 HOLD.
