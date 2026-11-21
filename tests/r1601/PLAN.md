# R1601 C1307 finite PLAN V2

## Status and authority

TEXT PREP ONLY. Exact execution target: NOT_RELEASED. Current compiler/native,
Lean/build/SDK/formatter/syntax/mock/test/apparatus execution, Git object/ref/
branch/PR/repository mutation, and Actions/CI/FAST/FULL consumption are all 0.
Every C, Python, CMake and YAML file in this packet is unexecuted draft text.

The same independent R1601 actor prepared V1 under own Issue comment
7058995046, ACTUAL RESUME 7059002546, CLI range release 7059034403 and CMake
metadata-only disposition 7059175133. V1 remains immutable in packets
7059277655/7059279455/7059281306 and PLAN FINAL 7059294717.

This separately named V2 text repair follows own-Issue review 7059358075 and
grading clarification 7059407369. ACTUAL RESUME 7059363343 was individually
GET-verified before repair. Both review comments were individually GET/read.
There is no new worker, descendant or producer/other-R exposure. V2_DELTA.md
records the changed texts, rationale and unexecuted reasoning cases. The old
V1 texts, hashes, FIRST source units and mathematical/origin oracles stay intact.

Reading main is 6287fcb4cb94df3da1c67f57d7d717b29c25ecf9, not an execution
target. CURRENT_SPEC still selects Draft17.38; its blob remains
25951270752c9e6499a1dcefc99ab7b17115bcee, identical to FIRST's selected Draft.
Compass, Value 1.1/1.3, Process 7/7.6 and the source laws were reconfirmed.
The inherited reads.json records V1's exact public/canonical blobs, returned-
text hashes, range bytes and exposure. V2_DELTA.md records this repair's own
Issue/public rereads. No extra project implementation or linked fixture was read.
The README range hashes are not whole-file SHA256 hashes.

FIRST V1 (7058681755) remains the sole source/mathematical/origin oracle.
The original manifest remains 547 bytes with SHA256:
e74cb47ae78d19b1ea30849293d1c79c78326e711228e5d9518a44050644b97d.
It is copied unchanged as FIRST.sha256; its six entries address sources/.
None of the six source bytes, including multiplication and identity, changed.

## Purpose and limits

The question is whether ordinary quantity expressions can be reused across
ordinary unary usize helper calls and a finite acyclic composition. Named Copy
is the pre-frozen ordinary alternative. It does not require reassignment.

Per-case source profiles, new issuers/worlds/schemas or additional human audit
dependencies are separate cost/goal concerns. A lawful unsupported case is
recorded at its real boundary, not relabeled invalid or changed to fit a target.
Value 1.1's author + ordinary-safe caller audit/re-audit comparison against
well-factored C remains UNMEASURED. Product GO #624 PAUSED / H1 #268 HOLD.
This packet supplies no product promotion and no extra existing PR gate.

All lawful source-law expectations require released usize maximum U >= 56.
A later release must confirm a target satisfying that profile. No 32/64-bit
width, emitted name, physical identity, diagnostic spelling for source-law
errors, or general scalar whole-call certificate is assumed.

## Frozen cases

Each row is a separate complete original unit. main returns unit. No original
assertion, I/O, new ABI, mutation or extra helper call has been added.

| Key | Source law | Mathematical oracle |
| --- | --- | --- |
| p1 | lawful positive | quotient(31+6): 37/4 = 9 |
| p2 | lawful positive | reduce(8*7): 56/3 = 18, then 18%6 = 0 |
| c1 | true invalid | quotient(9/0): false nonzero precondition before call |
| c2 | true invalid | quotient(3-5): -2 outside usize before call |
| c3 | lawful coverage | quantity=37 remains Copy; first=9, second=9 |
| c4 | lawful coverage | relay via identity(n/1): zero_result=0, nonzero_result=7 |

Definition-time n remains arbitrary in [0,U]. The helpers' constant nonzero
divisors and bounded quotient/remainder preserve representability independently
of a convenient main. A concrete zero is not an Unknown placeholder.
FIRST already fixes source order, actual/formal/body/result obligations and the
distinction between value equality and source/call/result/world authority.

## Proposed integration, not a current write release

All own files are intended under tests/r1601/. Insert exactly the statement in
root-CMake.include.txt inside the existing root if(BUILD_TESTING), after its
Python3 discovery. No hook body or unrelated test is read or modified.

CMakeLists.txt adds r1601_observe using the already declared public-library
closure, and exactly one CTest: r1601_arithmetic_actuals.integration.
include_guard(GLOBAL) avoids duplicate inclusion of this same file.
The root C17/warning/sanitizer policy remains inherited.

The ALL target r1601_registration_metadata depends on the observer and CLI
binaries, then runs only:
ctest --test-dir CONFIGURED_BUILD -N -R ^r1601_arithmetic_actuals[.]integration$
This is one host metadata enumeration, zero executed tests, zero observer/CLI/
guest program runs. Its exact own-name output and Total Tests: 1 must be
checked together with unchanged R-FAST whole-inventory uniqueness/count checks.
A build or the existence of this target alone is not registration evidence.

R-FAST remains format + host build + registration only. It must never invoke
run.py or any source CHECK/public/native test. The existing early workflow
selects its own fixed list; it is not expanded and is not a substitute FULL run.

The two YAML snippets are proposed insertions in the existing FULL/PARTIAL
jobs immediately after their primary CTest step. They only run the fixed-path
data publisher with always(). They do not replace/change that step, its exit,
required job names, matrices, toolchain, privileges, warnings or sanitizers.
No R-FAST or early workflow change is proposed. Actual workflow writes still
require a separately reviewed C release.

## Finite future execution model

Mode: GitHub dedicated branch + Actions ONLY. A P-equivalent local execution
environment is prohibited. run.py also refuses outside GitHub Actions and the
named repository. This runtime check does not create authorization.

The conceptual later CI unit is one natural FULL5 matrix plus one NO-LLVM
PARTIAL job, only if separately released. FAST attempts, FULL/PARTIAL budgets,
branch name, exact target, parent/ref creation and reviewed write scope are
NOT_RELEASED here. Process defaults do not self-release them.

Per actual invocation of the sole R CTest:

- Six cases in FIRST order, no retries.
- One recorded git rev-parse HEAD metadata child, checked against event SHA.
- At most six original --check children and six public-observer children.
- At most four emit, four C17 compilation and four native children, only for
  lawful units whose ordinary CLI CHECK and authentic public entry both succeed.
- Maximum 25 child commands total, with actual argv/status/stdout/stderr.
- Timeouts: metadata 5s; CHECK/observer/emit 10s each; native C compilation 30s;
  generated native process 5s. Runner wall budget 330s; CTest timeout 420s.
- Each normal stream cap is 32 KiB; emitter stdout is 64 KiB; metadata streams
  4 KiB. Aggregate captured-stream cap 512 KiB. A cap is an R resource stop,
  never a language limit or ordinary refusal.
- Observer: at most 16 call-body frames, 512 node visits, depth 32 and 4096
  source bytes per source-record observation. Actual diagnostic strings/spans
  are bounded to 4096 bytes and diagnostic notes to 16. Capped/incomplete fields
  or stream caps prevent a refusal grade. Missing/larger public relations remain
  unobserved; budget exhaustion stops the case.
- No OOM injection, mocks, test wrappers, replayed source state, private hooks,
  fresh helper probes, schema/issuer setters or second execution pass.

Existing raw/work directories cause refusal; earlier results are not overwritten.
A safety/resource/internal/unclassified outcome stops further cases. Remaining
cases/layers stay NOT_RUN. A layer is marked ATTEMPTING_OR_INCOMPLETE before
its child call; a launch/capture/health stop must not mislabel an attempted child
as NOT_RUN. Its start/result/raw records determine what actually ran.
Mechanical correction, if later permitted, preserves
all original input hashes/outcomes and requires separately recorded new text.

## Authentic source and public observation path

observe.c only loads the original unit, parses the function unit, records
source/declaration spans, creates the normal semantic context, and calls
nl_semantic_register_function_unit on that original tree.

Original unit parse and definition registration statuses are separate records.
The observer also receives the frozen host classification invalid/lawful; that
flag is used only by its stop guard and is never a semantic input or seeded fact.
For c1/c2, original registration OK immediately emits admission_stop and own
observer exit23, before constructing any adapter. The runner independently
checks original registration OK before interpreting observer exits or any later
adapter/refusal record, so a later exit12 cannot rescue admission.

Only for a lawful original with successful registration does the observer create
the distinct minimal apparatus source "main()\n", parse it as an expression, and
check that entry. The adapter never replaces original source or native input.
Its refusal cannot satisfy the original-source rejection grade.

From the checked entry's actual call-body accessor, the observer follows the
original main body and only the helper call occurrences already present there.
It does not call quotient/reduce/residue/relay/identity independently.
All spans are observed through the corresponding retained fragment source.
Each body frame has an R-local ordinal; semantic IDs remain qualified by their
public context/frame. No expected numeric ID or C symbol is frozen.

Node results are read through the existing public context/value/type views.
Unknown values serialize as known=false and number=null. The original-main
binding initializer path is used for the frozen numeric comparisons if that
unique relation and a Known usize are observable. An absent/Unknown result is
listed as missing; it is not fabricated or accepted as Known zero.

For actually encountered division/remainder nodes, the observer calls their
public view and read-only validators on both the actual event and its copied
observation. Their raw numbers, Known flags, operands, source spans, result and
target maximum are logged. The runner separately checks the mathematical
relation when concrete operands/results are available. Raw placeholder fields
are ignored as values when their flags are false.

Public getters/validators are used while all owners are live. Cleanup uses the
documented ownership order. No retained dangling view is used as evidence.
All error handling uses explicit conditions/return codes, not C/Python assert.

## Classification

The runner saves both byte streams and process metadata before classification,
including ordinary successful children. Normal completion, complete EOF/capture,
no signal/timeout/cap/launch failure and no sanitizer marker in either CHECK/
observer stream are required. Emitted C stdout remains source data; it is not
searched as though its literal text were a process diagnostic. Public OUT_OF_MEMORY, RESOURCE_LIMIT, INTERNAL_ERROR, unclassified
status, renderer failure or incomplete observer protocol is INCONCLUSIVE/STOP,
never a successful negative. CLI I/O exit1 and usage exit2 are separate stops.

--check exit0 must have zero stdout/stderr. CLI exit3 is initially only
CHECK_CODE3_PENDING_PUBLIC_DIAGNOSTIC_BINDING. A nonzero code, arbitrary stderr,
nonempty output or later adapter refusal cannot itself earn a source grade.
The observer records exact original source bytes, ordered public phases and the
actual structured diagnostic: source name, source role, input index, severity,
span and span bytes, optional category/code, message, optional primary range and
all bounded notes. It renders that same borrowed diagnostic with the public
parse/check renderer while the original source owner is live, retaining both the
render receipt and the entire stderr byte stream.

A CLI code3 is bound only to original parse or original registration refusal,
with the complete healthy public phase protocol and exactly one genuine
original-source ERROR diagnostic/render receipt. The source name, input index,
span bytes and optional primary range must refer to the same frozen original.
Category/code are actual nullable fields, not producer-derived expected values.
A provided primary range must agree with the actual span's LF byte coordinates.
The entire renderer stderr must equal exactly one CLI stream and the other CLI
stream must be empty. No whitespace, message, code, category, path or timestamp
normalization, output concatenation, substring match or spelling oracle is used.
Thus no extra/generated CLI bytes are accepted as part of the bound diagnostic.
A split/missing/extra/mismatched stream is INCONCLUSIVE/STOP.

For frozen true-invalid c1/c2, grading is deliberately narrow:

- CLI acceptance is an immediate STOP; subsequent public/native layers remain
  NOT_RUN. Original public registration OK is independently unconditional STOP.
- Normal original parse OK followed by original registration SEMANTIC_ERROR,
  healthy processes and exact original public-renderer/CLI diagnostic binding
  permits ORIGINAL_INVALID_SOURCE_REJECTED.
- That status establishes rejection of this exact frozen invalid input. The
  arithmetic algorithm or law-specific diagnostic path remains UNESTABLISHED.
  The public interface supplies no typed zero-divisor/underflow reason enum;
  lack of such an enum is not a new acceptance prerequisite.
- Original parser-only refusal, SEMANTIC_UNSUPPORTED or ANALYSIS_PRECISION_LIMIT
  can be retained with actual matched diagnostics, but is INCONCLUSIVE/STOP for
  the true-invalid semantic-error grade. It is never promoted to law detection.
- An adapter-only refusal, missing original evidence, mismatch, unhealthy or
  unclassified result is INCONCLUSIVE/STOP, never a negative PASS.

For lawful p1/p2/c3/c4, a genuinely bound original parse/register refusal is
recorded as LAWFUL_SOURCE_REFUSED_AT_ORIGINAL_BOUNDARY with its actual public
status. SEMANTIC_ERROR is not renamed unsupported, and an unsupported boundary
does not change source legality. Such a recording carries no numeric/backend
capability PASS. CLI acceptance paired with public refusal is INCONCLUSIVE/STOP;
no native bypass is attempted after observer failure. A green recording CTest
therefore still does not establish capability, general soundness or custody.

Confirmed Known arithmetic/result mismatches, failed authentic arithmetic
validation, violated public graph contracts or source hash changes are STOP C.
Missing/Unknown values remain missing/Unknown, including a mathematical zero.
No numeric exit is interpreted as a diagnostic reason without this binding.

## Pristine C17/native path

The chosen documented route is newlangc SOURCE. Successful nonempty stdout is
the sole C source; its exact captured file is passed with -x c to the existing
configured GCC/Clang. It is never rewritten, concatenated, instrumented or
replaced by hand-written C. Its SHA is checked again after compilation.

Compilation uses strict C17, pedantic errors, Wall/Wextra/Werror, the selected
Debug/Release mode and existing sanitizer mode. The existing Clang-specific
gcc-install-dir-libstdcxx warning exception is preserved narrowly.
Generated main is executed as supplied, without hidden symbol calls or a new
ABI. No arithmetic user assertion/I/O is added.

The README names V1-BACKEND-UNSUPPORTED, but does not promise a numeric emission
refusal code. The draft records that diagnostic separately from an UNSPECIFIED
numeric exit. Other nonzero emissions, signals, resource/sanitizer failures,
partial stdout or success with unconfirmed output health stop unclassified.
Future exact-target release must settle any additional exit-contract need.

main()->unit exit0 at most shows normal native process completion. It does not
observe the internal numbers, execution order or source/call/result/world
custody. Public value equality and an isolated valid arithmetic event also do
not supply a generic whole-call certificate.

## Raw transport and retrieval

run.py records a start file before each child; writes captured stream chunks
to dedicated binary files; writes exact status/hash/completeness metadata before
classification; and writes summary plus SHA256SUMS for the reached prefix.
The streams of successful children are saved just like failures. stdout and
stderr are separate byte sequences; no cross-stream interleaving is claimed.

Timeout/cap/aborted-drain records explicitly expose incomplete/truncated capture.
Raw prefixes are retained, but missing tails/status are not claimed as complete.
The fixed allowlisted GitHub metadata contains no environment dump or secrets.

publish.py reads only build/r1601-raw or build-frontend/r1601-raw. It opens each
allowlisted basename with O_RDONLY | O_NOFOLLOW | O_NONBLOCK, then fstat checks
regular type, link count exactly 1 and bounded size before any read. An allowed
FIFO cannot block waiting for a writer at open; nonregular and hardlinked files
are refused before reading. Descriptor device/inode/mode/link count/size/mtime/
ctime and byte count are checked again after the bounded read. The final
nofollow named-file stat must still identify that same regular single-link file.
The total cap is enforced incrementally, at most 104 files and 768 KiB. Files
that change, are incomplete or fail these checks produce explicit transport
failure. No actual FIFO/hardlink or runner behavior is asserted in this prep.

The data-only publisher emits ASCII base64 frames with byte counts and SHA256;
it executes no captured command and does not change primary suite results.
The R1601v1 frame label is unchanged transport versioning, not a claim to run
V1 apparatus. context.json identifies V2 and its actual PLAN_V2.sha256 hash;
remote retrieval must bind those and all apparatus hashes to the released V2.
Missing data uses an explicit STATUS, not a fabricated test failure to reveal
output. Publisher exit0 alone is not transport success.

RETRIEVAL.md fixes exact run/attempt/job selection, remote individual job-log
GET, framing/count/hash checks and the separate remotely-retrieved evidence
ledger. Locally retained or printed bytes are not called remotely retrieved.
Only an actual later GET and rehash can establish successful transport.

## Honest unresolved limits

- Target, branch, writes, FAST/FULL budgets and execution remain unreleased.
- This apparatus has not been formatted, syntax-checked, compiled, linked,
  mocked or run. Text/hash verification is not executable validation.
- Actual public node shape, precision, acceptance and diagnostic correspondence
  are unmeasured. Exact renderer correspondence is bounded original-input
  rejection evidence, not proof of an arithmetic checking algorithm.
- The released public surface does not expose a generic scalar whole-call
  issuer/result/world certificate. That relation remains NOT_OBSERVABLE.
- Definition-time arbitrary/Unknown formal quantities are not separately
  inspectable through this entered-main observation path.
- Source-order laws are fixed, but these pure units have no independently
  observable runtime ordering effects. Native numbers/order remain unobserved.
- Emission refusal numeric contracts beyond the stated README are unspecified.
- Real remote transport size/retention/tool truncation behavior remains untested.
  Missing footer/chunk/hash or wrong job/attempt makes evidence NOT_RETRIEVED.
- NONE of these gaps authorizes a new source form, source rewrite, API, issuer,
  world, authority, ABI, instrumentation or producer-calibrated oracle.

## V2 packet identity

PLAN_V2.sha256 enumerates the complete proposed V2 payload without self-hashing.
Twelve inherited V1 payloads remain byte-identical: six FIRST sources,
FIRST.sha256, reads.json, CMakeLists.txt, root-CMake.include.txt and the two
publisher YAML snippets. Five payloads have separately frozen V2 replacement
texts: PLAN.md, RETRIEVAL.md, observe.c, run.py and publish.py. V2_DELTA.md is new.
These are proposed path contents, not repository writes. The V1 PLAN.sha256 and
all V1 packet comments remain preserved as the independent baseline archive.

After complete V2 changed-text/delta/manifest POST -> individual GET ->
byte/SHA re-extraction and PLAN V2 FINAL POSTGET, STOP C.
