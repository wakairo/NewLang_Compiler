# P683: experimental POSIX-like u8 status transport

Base: `eabae4b5e0e352d00ea1a770abae30a25e34befb`, Draft17.33.
[M682](https://github.com/wakairo/NewLang_Compiler/issues/682#issuecomment-6306576767)
and C authorize a **nonnormative target experiment**, not new source/entry rules.
Historical gate: N/A for canonical semantics; KEEP ordinary typed return,
scoped Change/Reset and §31 target separation; DEFER general entry/FFI/I/O.

`freeze.json` predates compiler/source execution. `evidence/baseline.json`
preserves unchanged-main CHECK0 / default emit4 / zero C for both positives,
plus semantic CLI3 negatives. The genuine two-write operation is P679's
original record: saved port7 -> mode write7 returns old mode31 (or47) ->
that issued Copy binding supplies the port write -> actual final port read.
The two positive sources differ only in initial mode. No guest values or
permissions come from a host observer. These are lexical objects, not heap
Allocation/Domain or a safe library API.

## Opt-in invocation

After the ordinary frontend-only README setup:

```sh
./build-frontend/newlangc --check tests/p683/status31.nl
./build-frontend/newlangc --experimental-posix-u8-exit tests/p683/status31.nl > build-frontend/status.c
gcc-13 -std=c17 -Wall -Wextra -Wpedantic -Werror build-frontend/status.c -o build-frontend/status
python3 -c 'import subprocess; r=subprocess.run(["./build-frontend/status"]); print("guest termination status:",r.returncode); assert r.returncode==31'
```

The **guest** terminates normally with nonzero31, which conventionally means
**failure**, not application success. Python is an external status consumer;
it does not inject a guest value. The guest prints nothing. Mode47 yields47.
`set -e` can abort a shell script on either guest status. A negative Python
returncode means signal termination, not normal exit data. Compiler CLI error
codes1/2/3/4 belong to a separate process; compiler stdout contains C source.
This is diagnostic transport only, NOT stdout, a general result API, a stable
exit ABI, full USER-CLI-1 or Product GO.

The build and emitted target are restricted to Linux x86_64, tested by the
existing Ubuntu24.04 CI; no other C target/OS status equivalence is promised.
The default CLI and `--check` retain their old meaning. Without the option,
the same u8 entry remains backend-unsupported4 with zero C. Unit entry under
the option is also outside this experimental profile; its default exit0 is
unchanged. Literal-only entry, calls/branches/general bodies, escaped refs,
nonCopy results and broader record/ABI mechanisms are not admitted here.

The driver reuses the existing record, Copy, field and scoped-loan gates.
Its private finite entry gate additionally authenticates the registered
retained body/context, normal typed tail/call result, one original record,
two closed loans, linked root/child facts, the first old Copy as second input,
and the direct final changed-field Copy. No numeric result is extracted from
checker-known scalar data to emit a constant return. The generated internal
uint8_t callee returns the real local; C main calls it once and converts to int.
All semantic/profile validation precedes output. Existing FILE I/O failure
behavior is unchanged: a write error can leave partial output; callers must
check compiler status before using a C file. Zero-output claims apply to
semantic/profile refusal, not arbitrary broken stdout streams.

## Tests and limits

`p683_posix_u8.integration` reads public owned evidence after AST destruction,
checks normal/default/opt-in diagnostics, compiles **unchanged generated C**
strictly at O0/O2, and consumes the real guest status. No passive C observer
is used. Owned observations cover both final fields; this task's new native
channel exposes only port. Full numerical/physical sibling observation from
P679 remains separate prior evidence. `comparison.c17` is an independent
factored C baseline with the same actual computation, not a NewLang seed.
Its author/caller still audit live writable pointer/access/alias/lifetime,
writer order and host termination semantics. This local C example has a small
audit closure; a NewLang audit-locality improvement is **unmeasured**.

`p683_private_gate_oom.unit` mutates/restores only test-owned genuine artifacts:
call result, literal tail, wrong Copy symbol/use, root/fact and loan evidence.
It checks fail-closed admission, restored validity, allocation-free read-only
gating, real registration/entry fail-at-N OOM rollback and NULL failed artifact.
It is a private driver test, not a new public proof API or independent R.

The legacy --help/no-argument exact output is preserved to avoid changing
existing CLI oracles; the experimental option is documented in the main
README and here. An initial help addition caused exactly those two existing
local tests to fail and was removed without changing their tests.

A supplemental richer-body control initially placed a function before its
required leading struct (AVS-DECL-PROFILE3); its original bytes and predicted
oracle are retained. Only declaration order was calibrated in a separately
frozen version; this is not a language change or a source acceptance claim.

During probe development a missing stdlib include was fixed; an initial
observer assumption that every Copy use retains the field-read ValueId was
also corrected. The real read/binding value13 and subsequent Copy tail20
are distinct issued values linked by the checked identifier symbol7; body
and entry call both return20. Source/oracle bytes were not changed. An initial
implementation binding scan was corrected to follow the actual initializer
node relation instead of assuming bindings precede their initializers.
These were local test/implementation mistakes, not a canonical source finding.

Product GO scoring remains PAUSED; H1 HOLD, one visibility domain, R655 G3
incomplete and R675's finite independent evidence remain separate.
