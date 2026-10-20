# P670 — Stage0 HOLD before experimental Production edits

**Evidence only. NON-NORMATIVE, no B implementation/native success.**

Fixed adopted compiler: `a0de7a86d04d00d75a2484dfed4184ccb177e06c`.
Canonical remains CURRENT_SPEC → Draft17.32. P Work/Codex local checkout,
branch `p670-nominal-first-rung`. No Production, CMake, canonical, Ledger,
README, public header, workflow, toolchain or existing test/oracle changes.

## Authority and scope decision

Read Issue670 and binding C comment6291025505, M669 final6290925110 and C
adjudication6291015260. Compass → Strategy1.1/1.3, Process4.2/4.3/4.3.1/7,
Roadmap USER-CLI-1, Design Decision Procedure and Ledger DI001/006/007 apply.
KEEP Draft17.32 fixed dependent-child/current-value/scope/@ semantics. A future
candidate-only change to name/label eligibility is authorized for exploration;
it has **not** become normative. General fields/loans/owners are DEFERRED.
Main FULL5 39421834952 and PARTIAL39421835050 were independently re-read as
attempt1 SUCCESS at this exact main SHA; see `evidence/main-ci.json`.

The generic issuer path is plausible: `nl_fixed_attach` creates declaration-
indexed nonindependent children and facts, `fixed_selection` resolves declared
metadata, `nl_fixed_change` updates child and ancestor facts without replacing
incarnations, and ordinary private V0 uses nominal/index/source evidence.
The first one-root no-IF profile may reuse this with little code. This is a
static feasibility hypothesis, **not a successful trial** or new authenticator.

**Stage1 gate failed before Production edits:**

- `tests/unit/fixed_field_test.c:388–392` explicitly requires `Other` two-u8
  direct field read to return unsupported FIELD-PROFILE.
- `tests/unit/node_link_test.c:398–402` has the same historical oracle.
- Both register `Other` from source, then construct/read it through source
  fragments. These are different entry routes from the new whole-function-unit
  programs. That difference must be kept explicit, not treated as a native
  failure or as proof the new programs are unsound.
- A global name-independent `nl_fixed_type` predicate would include this
  complete, nonrecursive, Copy/Discardable two-u8 type as well. It would enable
  attachment and implicit-local stability at the legacy constructor before the
  old field read. The current old rejection would no longer be justified by
  that predicate. This prospective effect is static analysis; no patch was
  applied and no deliberately red candidate test was run.

Issue670 D says to preserve the historical rejection and STOP before changing
an existing oracle. Therefore no speculative guard deletion, per-name exception,
old-test migration, alternate library to hide failures or new source-mode /
provenance mechanism was added. C must decide an explicit experimental oracle
transition, or a separately authorized distinction between registered-unit and
fragment admission. Neither technical impossibility nor a need for general
CFG/ownership is established here.

## Frozen original sources and independent expected results

`freeze.json` was written at21:46:46.750 JST, before first CLI execution at
21:46:47.058 JST on2026-10-20. All six source hashes remain unchanged.
These are fresh first-rung programs, not copies of P658/P651/R fixtures.

| Source | Declared order | Constructor order | Independent intended result |
| --- | --- | --- | --- |
| packet_stamp.nl | PacketStamp(marker,channel) | channel,marker | (115,43) → (126,43), actual old115 |
| route_stamp.nl | RouteStamp(channel,marker) | marker,channel | (54,133) → (97,133), actual old54 |
| pair_control.nl | Pair(left,right) | right,left | (212,89) → (6,89), actual old212 |

Both new programs target declaration index0, using different labels for the
actual written field and reversed declaration order. Constructors also use
label order opposite to declaration, so the frozen oracle cannot follow first
initializer spelling. The first two rows are **unexecuted guest expectations**;
only the Pair control reached C17/native.

## Actual baseline phase results

| Source | Parser / registration / normal CLI | First diagnostic |
| --- | --- | --- |
| packet_stamp.nl | parse0 / unsupported2 / CHECK3 and C17 CLI3 / C0bytes | FIELD-PROFILE, 4:31–4:43, bytes[152,164) |
| route_stamp.nl | parse0 / unsupported2 / CHECK3 and C17 CLI3 / C0bytes | FIELD-PROFILE, 4:32–4:45, bytes[151,164) |
| unknown_field.nl | same earlier nominal-profile refusal | FIELD-PROFILE; unknown-field semantics masked |
| wrong_replacement.nl | same earlier nominal-profile refusal | FIELD-PROFILE; usize-to-u8 replacement error masked |
| wrong_initializer.nl | parse0 / semantic1 / CHECK3 and C17 CLI3 / C0bytes | P5-FIELD-TYPE, 3:35–3:44 |
| pair_control.nl | parse0 / registered owned / CHECK0 / C17 CLI0 | 1091 actual C bytes, no diagnostic |

The new nominal main-call owned artifacts, dependent root/field proof, V0
admission, C carrier, guest values and reversed-declaration native falsifier
are **NOT REACHED**. No CHECK0/backend4 observation for these sources is
claimed. Unknown-field and replacement negative expectations for a future
trial stay in the freeze ledger; they were not silently relabelled as tested.
Two distinct nominals cannot be declared in the current single-file one-leading-
nominal CLI profile; the cross-nominal negative is NOT TESTED, not replaced by an
unbound type-name test. No field1 write, second root, IF/arithmetic or P658 full
program was attempted.

## Public observations, cleanup and actual Pair native control

`registration_probe.c` reuses the immutable P658 public frontend observer as
test tooling (not P658's input programs). It only parses/registers and compares
all twelve public snapshot fields on actual failures, then destroys resources.
All five registration refusals leave the snapshot unchanged. It accesses no
private context layout and alters no checked evidence.

`legacy_fragment_probe.c` is a source-only public reproduction of the relevant
Other shape and refusal; it does not run the rest of the old negative group,
which includes private corruption controls outside this task. Actual Other
metadata: nominal/copy/discardable, two core-u8 fields, live available
independent root, no fixed children/implicit-local flag under current main;
the read refuses with NULL artifact and unchanged public snapshot. No host value,
root, identity or permission is seeded. IDs belong to this probe context only.

`pair_probe.c` loads the actual frozen Pair program, destroys the input AST and
parser, and observes the retained source-generated owned main-call body through
public accessors. Actual root/incarnation1, children2/3, parent fact1→6,
written-child fact2→5, closed scope1, old/new/sibling212/6/89. The root is ended
on normal function exit. These are Pair observations, not cross-context IDs
for PacketStamp/RouteStamp or an independent generalized validator.

`observe_pair.py` invokes ordinary CLI, compiles the actual emitted C under
`-std=c17 -pedantic-errors -Wall -Wextra -Werror`, O0/O2. Plain and passive
read-only variants run on GCC13 and Clang23.1.2. The passive observer captures
addresses of the actual original root/f0/f1, reads fields through volatile const
pointers, and verifies initial212/89, actual old212, new6, stable addresses and
untouched89. It never inserts guest state or an expected return value. Both
host and Pair guests also pass local ASan/LSan and UBSan. There are four Pair
guest executions per configuration, sixteen total; none is a new-type guest.

ASan/LSan and UBSan also repeat all actual registration refusals and public
probes, with no sanitizer diagnostics. This is observable failure cleanup, not
exhaustive OOM/I/O injection or proof of all private state. OOM injection is
NOT RUN because no issuer/registration implementation was changed. Node and
Array4/local full-suite regressions are NOT RE-RUN for this Stage0 stop; the
verified existing main CI is distinct evidence. No R655 integrity run was
started or simulated.

## Reproduction and retained evidence

Use the repository frontend-only reference route; it is NO LLVM / PARTIAL,
not the locked23.1.3 FULL5 infrastructure route:

```sh
CC=gcc-13 cmake -S . -B build -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target newlangc --parallel 2
./build/newlangc --check tests/p670/packet_stamp.nl
./build/newlangc tests/p670/packet_stamp.nl > /tmp/p670.c
# Both exit3, FIELD-PROFILE; /tmp/p670.c is empty.
for probe in registration_probe legacy_fragment_probe pair_probe; do
  gcc-13 -std=c17 -pedantic-errors -Wall -Wextra -Werror -O0 \
    -Iinclude -Itests/support tests/p670/$probe.c \
    -Wl,--start-group build/libnewlang*.a -Wl,--end-group -o build/$probe
done
./build/registration_probe tests/p670/packet_stamp.nl
./build/legacy_fragment_probe
python3 tests/p670/observe_pair.py build/newlangc build/pair_probe gcc-13 none
python3 tests/p670/observe_pair.py build/newlangc build/pair_probe clang-23 none
```

For local sanitized host/guest, configure with Clang and
`-DNEWLANG_SANITIZER=address` or `undefined`, link the probes with the same
`-fsanitize` and `-fno-sanitize-recover=all`, and pass that sanitizer name to
the Python observer. ASAN_OPTIONS includes detect_leaks=1:halt_on_error=1;
UBSAN_OPTIONS includes halt_on_error=1:print_stacktrace=1.

`evidence/main-baseline.json` contains original CLI outputs;
`public-and-sanitizer-baseline.json` contains phase/rollback observations;
the four `*-pair-native.jsonl` files contain actual control results;
`historical-oracles.json` preserves unchanged old test excerpts and hashes.
Main's actual control C is retained as `main-pair_control.nl-c17.stdout`.

## Cost and authority returned to Coordination

Actual compiler change0, old test/oracle change0, new issuer/API0, Stage1 trial0,
candidate PR0, new Actions0. The source/observer evidence is retained in one
P-only commit; no decorative PR. ~276 lines of new/reused read-only observation
tooling are distinct from compiler implementation cost.

The likely smallest common predicate replacement is small, and native code
already uses actual declaration indices. But full reuse cost is **not measured**
because the existing-oracle transition is blocked by the explicit gate. Do not
report an unimplemented diff as a <=100-line successful feature. A future
source-category distinction must be explicitly selected and audited rather than
hardcoding PacketStamp/RouteStamp or silently changing old Other expectations.

Goal is natural nominal/field names with existing fixed-child safety, not new
ownership or a per-name verifier. Technical Stage0 findings do not establish
ordinary-safe library UB-audit locality, USER-CLI-1 PASS, universal soundness or
Product GO. No new goal departure is identified; expanding mechanism or oracle
policy merely to manufacture a green demo was avoided. Active Product GO
scoring remains PAUSED and H1 HOLD. C owns next scope/normative/adoption/R/F
decisions; no other Track is started here.
