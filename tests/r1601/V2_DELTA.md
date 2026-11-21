# R1601 C1307 V1-to-V2 text delta

## Authority, identity and unchanged originals

V2 is a separately named, unexecuted apparatus-text proposal. It is not a
compiler finding, target release, repository write or execution result.

- Own review: https://github.com/wakairo/NewLang_Compiler/issues/1601#issuecomment-7059358075
- Own grading clarification: https://github.com/wakairo/NewLang_Compiler/issues/1601#issuecomment-7059407369
- ACTUAL RESUME, individually GET-verified before repair: https://github.com/wakairo/NewLang_Compiler/issues/1601#issuecomment-7059363343
- Original FIRST: https://github.com/wakairo/NewLang_Compiler/issues/1601#issuecomment-7058681755
- V1 packet 1: https://github.com/wakairo/NewLang_Compiler/issues/1601#issuecomment-7059277655
- V1 packet 2: https://github.com/wakairo/NewLang_Compiler/issues/1601#issuecomment-7059279455
- V1 packet 3: https://github.com/wakairo/NewLang_Compiler/issues/1601#issuecomment-7059281306
- V1 FINAL: https://github.com/wakairo/NewLang_Compiler/issues/1601#issuecomment-7059294717

V1's separate PLAN.sha256 is 1689 bytes, SHA256
75a2c9529292f847756248abb1d6c7bcd8db1536dad26db06193f79dded2f9ae.
It and every old comment remain immutable. V2 replacement contents at proposed
paths do not overwrite or withdraw those archived texts or their findings.

All six FIRST source units, their byte counts, LF/final-LF bytes, mathematical
answers and source-law/origin obligations remain unchanged. FIRST.sha256 is
still 547 bytes, SHA256
e74cb47ae78d19b1ea30849293d1c79c78326e711228e5d9518a44050644b97d.
There is no source rewrite, new assertion/I/O, reassignment assumption, new
ABI, helper probe, fixture, oracle or expected diagnostic spelling.

PLAN_V2.sha256 lists 18 proposed payload files, excluding itself: 12 inherited
V1 files, five changed V2 files and this new delta. The 12 inherited files are
the six sources, FIRST.sha256, reads.json, CMakeLists.txt, root-CMake.include.txt,
full-publish.snippet.yml and partial-publish.snippet.yml. No R-FAST workflow or
logic change is proposed. The metadata-only own-name ctest -N target and its
observer/CLI dependencies remain exact V1 text; it executes zero tests.

## Independent rereads and exposure

Reading base remains main 6287fcb4cb94df3da1c67f57d7d717b29c25ecf9,
CURRENT_SPEC -> Draft17.38. This is a public reading base, not an execution
target. V1 reads.json remains the unchanged baseline of authorized public and
canonical reads. During V2 repair I individually reread the two own C1307
comments above and my own V1 packets. An initial anchor URL fetch returned only
the own Issue body; both C comments were then fetched by their exact individual
GitHub API comment URLs and their returned IDs/body were read.

I also reread only already-released public source.h, parser.h, semantic.h,
diagnostic.h and the allowed README 391-402 range at that same pinned main.
Their source contracts are unchanged from V1:

| Public file | Pinned Git blob |
| --- | --- |
| include/newlang/source.h | 269b2723c07bed009455e5e3f2766b9086a33dd2 |
| include/newlang/parser.h | 20584427e155e88917a3a819ac24b9d7b0850eaf |
| include/newlang/semantic.h | a2b4e786878617d048335db51ca0f7ea0d877bd5 |
| include/newlang/diagnostic.h | 8332bf9e2ffb8235c14f17472c73677c086f5c7f |
| README.md, range 391-402 only | c0f2e0d7a91c55951a4fcc402acc7f42be70d12e |

The original unit registration contract commits only after every original
declaration succeeds, with a failure reporting input index and source span.
The diagnostic API exposes actual severity, optional category/code, message,
range/notes and public rendering; it promises no typed arithmetic-law reason.
README --check binds normal success to exit0/zero streams, I/O to exit1, usage to
exit2 and parser/semantic failure with diagnostics to exit3. The already-read
emitter documentation still provides no numeric refusal-code contract.

No candidate implementation, other R/P/M/F originals, producer fixture/result,
aggregate C, shared scratch/worktree, actor inventory or descendant was used.
All work in this repair is own text construction, own/plain public read and
text/SHA verification. There is no local executable apparatus file or run.

## Changed texts and reasons

| Proposed V2 file | Concrete change from immutable V1 | Reason and limit |
| --- | --- | --- |
| observe.c | Adds a frozen host-only invalid/lawful argument; invalid original registration OK records admission_stop and observer exit23 before adapter creation. | Original invalid-unit admission cannot be rescued by an adapter refusal. The flag seeds no compiler state or semantic fact. |
| observe.c | Records complete bounded actual diagnostic fields and source/span bytes; calls the corresponding public renderer unchanged and records its result. | Preserves actual original-phase diagnostic evidence without a producer oracle. Missing/capped/invalid/render-failed data cannot grade as healthy refusal. |
| run.py | Independently checks any invalid original register OK before downstream exit/adapter grading; retains both raw streams, actual phases and diagnostic records. | The original V1 exit12 rescue path is removed. Existing raw command files remain independent of the interpreted summary. |
| run.py | Treats code3 as provisional, validates ordered public phase/health protocol and requires exact original renderer bytes in one CLI stream with the other empty. | A number or arbitrary stderr alone is insufficient. Source/span/category/code are preserved from the same actual public diagnostic and no normalization or expected spelling is introduced. |
| run.py | Uses ORIGINAL_INVALID_SOURCE_REJECTED only for healthy original parse OK + register SEMANTIC_ERROR + exact diagnostic binding; separately retains law_specific_reason=UNESTABLISHED. | Implements the narrower observable refusal grade without demanding an unprovided typed law enum or claiming which arithmetic algorithm ran. |
| run.py | Stops invalid parser-only/unsupported/precision and all mismatch/unhealthy cases; requires lawful CLI and public success before native continuation. | Avoids both false negative success and a native bypass after public refusal. A lawful bound original refusal remains actual coverage information, not source invalidity or capability PASS. |
| run.py | Marks a layer ATTEMPTING_OR_INCOMPLETE before calling its child, then replaces that with observed status only after return. | A launch/capture/health exception cannot leave an attempted child mislabeled NOT_RUN; raw start/result files remain authoritative. |
| run.py | Checks sanitizer markers in both CHECK/observer streams while emitted C stdout remains source data; records V2 revision and the actual PLAN_V2.sha256 file hash in context. | Keeps health and externally pinned apparatus identity ahead of grading. Hash presence alone is not execution authorization. |
| publish.py | Opens with O_NOFOLLOW and O_NONBLOCK, then requires a bounded regular file with st_nlink==1 before reading. | A potential FIFO open cannot wait for a writer; nonregular/hardlinked evidence is refused before reading. No actual such file is asserted. |
| publish.py | Rechecks descriptor type/link count/dev/inode/mode/size/mtime/ctime, read byte count, and final nofollow named-file identity; enforces aggregate cap while collecting. | Retains finite bounded reads and detects observed changes without substituting another file. Data failure remains explicit, and publisher exit0 never proves retrieval. |
| PLAN.md, RETRIEVAL.md | Align classification, original/adapter separation, raw binding, V2 identity and publisher barriers with the changed text. | Documentation no longer implies V1's broader negative grade. Execution and remote retrieval remain unperformed. |

All host return codes 10/11/12/20/21/22/23 here belong to this proposed observer.
They do not claim compiler CLI exit semantics. The only CLI check numbers used
are the documented 0/1/2/3; final emitter refusal codes remain UNSPECIFIED.

The exact public-renderer match is an observable projection of the actual
structured diagnostic. It does not assert that rendering is injective, that a
particular arithmetic-law implementation was traversed, or that the CLI exposes
a second structured diagnostic API. Actual nullable category/code fields remain
available for review; no missing typed reason enum is treated as a new blocker.

## Unexecuted conditional reasoning table

These are text-level branch arguments only. No input fixture, subprocess, mock,
test harness, FIFO, hardlink, compiler, formatter or syntax check was executed.
They predict how the proposed guard/classification text is intended to behave;
they are not recorded implementation outcomes.

| Hypothetical reached evidence | Intended V2 disposition | Evidence not supplied |
| --- | --- | --- |
| c1/c2 original registration OK, with a possible later adapter refusal/exit12 | Observer stops before creating the adapter; independently, runner original-admission check is unconditional STOP even if later refusal records exist. | No negative success and no claim an actual target produced this sequence. |
| c1/c2 original parse OK, original register SEMANTIC_ERROR, healthy observer/CLI, exact actual renderer/CLI diagnostic match | ORIGINAL_INVALID_SOURCE_REJECTED for the exact original invalid unit; source/span/category/code and raw are retained. | Arithmetic algorithm/law-specific diagnostic-path proof stays UNESTABLISHED. |
| Same healthy semantic-error case with optional category/code null and no typed arithmetic reason enum | The narrower original-input rejection grade remains possible; no expected enum/spelling is invented. | No law-specific classification from absent data. |
| c1/c2 parser-only refusal or original SEMANTIC_UNSUPPORTED/ANALYSIS_PRECISION_LIMIT, even if diagnostics match | Retain the actual original boundary and diagnostic; INCONCLUSIVE/STOP for true-invalid semantic-error grading. | No assertion the intended arithmetic rejection rule was exercised. |
| CLI code3 with arbitrary/extra/split/missing bytes, unrelated source/span, failed renderer or diagnostic mismatch | INCONCLUSIVE/STOP; both captured streams and actual structured records remain available. | No negative PASS from a code number or nonempty stderr. |
| Signal, timeout, output cap, sanitizer evidence, public resource/internal/unclassified status or incomplete protocol/raw | Health/resource/protocol STOP before successful grading; reached prefix and missing layers stay explicit. | No conversion into an ordinary source rejection. |
| Lawful CLI acceptance but public adapter/original refusal | INCONCLUSIVE/STOP; no native bypass and no source invalidity relabeling. | No capability conclusion based on just one route. |
| Lawful public relation missing/Unknown, or ordinary main()->unit native exit0 | Preserve missing/Unknown and only the observed process completion; a concrete mathematical zero is not filled in for Unknown. | Runtime quantities/order, arbitrary-formal observation and whole-call custody remain NOT_OBSERVABLE. |
| Allowlisted path is a FIFO/nonregular file or regular hardlink | Nonblocking/nofollow open, then descriptor type/single-link check refuses it before reading. | No actual file creation or claim of observed malicious data. |
| Regular evidence changes size/type/link count/identity/timestamps during read or named path differs afterwards | Publisher data error, no claimed complete bundle; total bytes/files remain bounded. | No claim of remote recovery from a publisher process exit alone. |

## Present limits and final handoff

Exact compiler target remains NOT_RELEASED. Future mode is GitHub dedicated
branch + Actions ONLY; a P-equivalent local execution environment is prohibited.
Current compiler/native/Lean/build/test/mock/apparatus/SDK/formatter/syntax,
Git object/ref/branch/PR/repository and Actions/CI/FAST/FULL budgets remain 0.

Source profile U>=56 still needs an eventual exact-target release. All six
actual acceptance/refusal/public/value/C17/native outcomes remain unmeasured.
Text re-extraction/SHA verification establishes proposal integrity, not C/Python
correctness, compilation, passing tests or real log transport. The unchanged
finite command/time/capture budgets are proposed bounds, not consumed budgets.

Public number equality, borrowed views and an authentic isolated arithmetic
event do not establish generic source/call/result/world authority. Native unit
exit0 does not expose internal quantity or evaluation order. No new API/ABI,
source form, issuer/world/schema, mechanism investment or product promotion
resolves these gaps. Author plus ordinary-safe caller audit/re-audit closure
versus well-factored C remains UNMEASURED; ProductGO624 PAUSED / H1 HOLD.

Freeze complete changed texts, this delta and PLAN_V2.sha256; individually GET
each comment, re-extract exact UTF-8/LF/final-LF bytes and rehash. Then POSTGET
PLAN V2 FINAL and STOP C. No execution or additional release is self-authorized.
