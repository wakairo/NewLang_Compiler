# P471 — matched Some(q) cross-root write frontier

Test-only bounded measurement against fixed Compiler/main
`9ff3759df34aa711108e344baa6d7cfa0ce597fd`, CURRENT_SPEC -> Draft17.30.
Source/expectations were frozen in commit
`ab82b95` before any compiler execution. See PREFLIGHT.md and frozen.json.
A is byte-identical to old PR396's asymmetric source (SHA c578...050);
S differs only by a None-arm tail.next None replace. The adopted control
fixture is unmodified and its Git blob matches old/current main.
No third candidate or optional post-match tail read was created.

## Reproduce

Use the standard locked repository build, or a frontend-only build without
LLVM download (PARTIAL evidence):

```sh
cmake -S . -B build-frontend -DNEWLANG_FRONTEND_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-frontend --parallel 2
ctest --test-dir build-frontend -R '^p471_crossroot_frontier.integration$' -V
```

The runner freezes hashes before each source/check invocation. It runs the
unchanged control first; S/A are independent first-barrier cases. Public-only
source probes precede ordinary C emission, strict O0/O2 compilation and native.
The final success marker is printed **only if control and BOTH frozen cases
actually reach successful physical observation**. CTest requires this marker;
runner exit0 alone means only that a first-barrier map was recorded. No output
suppression can silently relabel a new source/backend barrier as native PASS.
The existing configured sanitizer is passed unchanged to the actual guest C.

Public probe verifies source-issued join via existing independent validator;
Some operand identifier equals the real arm binder q, is Copy, and still targets
the original tail incarnation. Entry/post root identities and sibling facts
remain sound; wrong copied projection evidence is refused. The old unchanged
control uses its existing normal_frame_unchanged evidence and no changed-join
certificate. No private checker layout, semantic seed or emitter call in probes.

Passive observer values were fixed before execution: tail payload2, head
payload1; original addresses distinct; original head points to original tail;
actual Some q equals original tail; old_tail None; updated tail.next points to
that same q/original tail; head is later reset None and old_some retains the
prior Some(original tail) Copy. No guest writes or branch-local old carrier
address retained beyond scope. Role IDs come from existing public probes, never
generated C text. Plain and observed binaries use identical unmodified guest C.

## Measured locally

All three sources: CHECK0 / public0 / C17 CLI0, no diagnostics. S and A each
have a genuine source-issued Node join certificate accepted by the public
validator. Current A moves past old PR396's CLI3 P9-CONTINUATION-PRECISION;
the old finding remains correct for its frozen old compiler.

| source | SHA256 | C bytes | C SHA256 |
| --- | --- | ---: | --- |
| control | afe989f4839f1fabf1112bf1f27a7498531db30a7e407784079d401690e15ada | 2209 | d040b5abee56df765707e83d6f0db1952bab9094e7ee9931ae3fe6833c500e61 |
| S | dfc07c373f4b72be95114149cf0c503f4601d58b99ebb494a49400cc76bd0531 | 2922 | 0c6a710d3e29c7d3be2e9564614a5e79303e81893066ea9c0ace824aecdb7b95 |
| A | c578ebdce71ffd46e30758903c28f69227876c7904264b3eec1914c7f6bdd050 | 2545 | 92359ec3c59d72c7286cd45fd28dd8f68302410d935d9417934609feeb123acc |

For S/A common entry: original tail place/inc1/1, head6/6; changed field2,
sibling3; input Some points tail1; head RF11 unchanged through this join.
Tail RF1->RF17 in the conservative joined post. S None arm F2->17/RF1->18
(occurrence0->0); Some arm F2->18/RF1->19 (occurrence0->3). A None arm has
operation0/unchanged; its Some arm has that same qualified transition. Numeric
IDs are world-qualified; arm-local facts are not imported into the common post.
Public Some binder8 equals operand symbol8, Copy target1/inc1. Public validator
also rejects wrong-projection copies and still accepts the original issued view.

GCC14.2 and Clang23.1.2 local strict C17 O0/O2 plain/observed each pass12 native
runs (control plus S/A); raw source/public/physical observations in evidence/.
The single local NO LLVM/PARTIAL GCC preflight passed all283/283 CTests in
55.94 seconds (282 existing + this new test).
These are local unsanitized evidence, separate from hosted locked23.1.3 CI.
Hosted FULL/PARTIAL attempt1 outcomes are recorded in PR/Issue, no extra push
for CI-result documentation and no reruns. No source/oracle correction needed.

P466 is a separate already-adopted PASS: it never writes q and both arms mutate
peer.next. Current S/A test actual q transport and, for A, unchanged None arm.
No product/heap/nonCopy authority is inferred. These are original lexical Nodes,
implicit local identities, public Domain ID0 / BackingRegion0, not heap owners.

## Scope / disposition

Only new tests/evidence and CMake test registration. No production, canonical,
current fixtures, adopted tests, toolchain, docs or workflow changes. No archived
PR396 checkout/merge. §4.2 N/A unchanged selected semantics; §4.3.1 Exploration
only. H1/STB/V Product holds remain. First substantive barrier in this exact set:
none; this checkpoint ends here. No post-match third probe, general mechanism
or automatic implementation issue/other Track. C decides the next investment.
