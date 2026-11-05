# T1166 fixed Git context prototype

C238 release: https://github.com/wakairo/NewLang_Compiler/issues/1166#issuecomment-6648363017

This standalone Git/Python prototype prepares an independent temporary snapshot
index for a caller's already fixed file copy. It is not installed in a compiler,
root CMake, old history test, R runner, workflow, or current release gate.

## Authority and use

`FixedGitContext.create(source_repo, copy_root, parent, commit, tree)` requires
an already available, legitimate local Git repository, a full commit SHA and
its fixed tree SHA, a canonical existing copy root, and a canonical existing
parent directory. It accepts regular Git modes 100644 and 100755 only; it
rejects symlinks, gitlinks, non-UTF-8 tree names, empty inventories and unsafe
paths. The actual fixed P inventory was previously observed as all 100644.

The commit/tree, recursive Git tree path/mode/blob records and transferred
object bytes establish authority. The helper copies an explicit finite object
closure into its own object store, verifies each object's Git hash, marks the
fixed commit as a shallow boundary if it has parents, checks every copied byte
and mode, uses `read-tree`, then reads back every stage-0 index entry. No `git
add`, filesystem discovery as authority, alternates, linked worktree, shared
index, source ref/index mutation or network operation is used. Ambient Git
variables are removed for both preparation and consumer execution; Git config
injection, global/system configs and lazy fetch are disabled. Non-Git variables
(including compiler/sanitizer and network proxy/trust variables) are preserved.

Use the context manager and synchronous `context.run(argv)` only after the
caller is authorized to execute that exact consumer. The argv list is passed
unchanged without a shell, the consumer return code is retained, and inventory
is verified before and after it. `require_tracked(candidate_paths)` rejects any
untracked candidate even when its bytes equal a tracked input. Other incidental
untracked outputs do not become authority. Existing history assertions remain
responsible for their original candidate selection and expectations.

Each call exclusively allocates a fresh private directory. Existing destination
paths are never reused. All consumers must finish before context exit; detached
children and concurrent modification are unsupported. Cleanup requires the same
owned directory inode and ownership marker and uses symlink-safe removal. An
ownership/cleanup failure raises and retains the path for operator review; it
must not be converted to success or repaired by deleting an unverified path.
Inputs and parent directories must remain stable while the synchronous call is
active. Filesystem race resistance against a concurrent hostile actor is not
claimed. The helper is a tested prototype, not a general repository sandbox.

## Finite test and actual-P boundary

`fixture.json` is a predeclared six-leaf synthetic commit/tree and exact object
payloads. `run_suite.py --output <new-evidence-directory>` runs the frozen cases
once, records raw argv/stdin/stdout/stderr as reversible base64 in JSONL, and
retains the first unexpected failure. A real fault stops the remaining cases;
there is no automatic retry. Source repository file/mode/hash/mtime inventories
are evidence of nonmutation, not input authority. `rglob` measures storage only.
The one source-unavailable case deliberately hides a separate disposable
provider's Git directory and restores it; the original source remains intact.
Expected rejection cases are fault injections, not successful compiler tests.

Actual P is `9030055314600b804fafde64adebb03c4d6315a1`, tree
`261a8e404a1cf1982e13d3b94e3223d5da0e0499`, 5992 leaves. Its authorized local
objects/transport are unavailable here: actual-P integration and the unchanged
P1062 five attacks/render/untracked regression are **NOT RUN / ACCESS STOP**.
Fetch attempts remain 0/1; no substitute download or Git-object reconstruction
is used. Synthetic PASS cannot establish actual-P integration PASS or repair
R's overall RED CI. Compiler/native/CMake/CTest/SDK/Actions/CI are not run.

## Future adapter, not implemented

A separately authorized future R adapter can supply its existing fixed P copy
and already acquired fixed objects, keep this context alive for all consumers,
and point its existing script registration at `context.work`. Existing copy,
hash, mode, old-history assertions, runner arguments and FIRST/RED records stay
in force. This release does not change `snapshot.py` or `register.cmake`.
The alternative of avoiding root CMake coupling still requires a distinct
acceptance design: the current T993 hook's at-most-six CHECK accept/reject
entries do not replace the owned/native/probe R obligations. No framework
extension is proposed here.

Costs recorded by the suite are only synthetic preparation time, copied bytes,
pack bytes, command count and filesystem bytes. Full-P scale, production
consumer behavior, CI portability, interruption recovery and adversarial
concurrency remain unmeasured. The additional maintenance contract is this
regular-file-only object-transfer/index/environment/ownership API and its finite
standalone tests; adoption needs a separate decision using actual-P evidence.
