# Preserve caller storage across an input wrapper

Issue #1364 / C670 composes the existing current/Copy/custody relation for one
main original, one input wrapper/local original and one inner current-loop/local
original. The main field is updated by the actual returned free Copy after both
callee frames end. Wrapper storage is retained as well as main storage.

The original P1362 B source is unchanged in `A-caller-original.nl`. A read-only
baseline trace confirmed definition CHECK rejected its second caller original in
`current_preserve_caller`; the earlier static inference is kept separate. B uses
the opposite declared field order and different values. Source/oracle/FIRST from
P1362 is not rewritten by this implementation's success.

The private current record identifies two caller members inside its existing
worlds and retains the nearest-wrapper alias. Each member is resolved against
its actual lexical initializer/binding. Existing current-frame comparisons keep
all non-target places and values exact. The owner consumes the canonical
root/main/wrapper/inner path, existing Copy entry/LIVE exit and final graph seals,
checks actual EndLocals and follows checked Copy bindings from the exact return
to main's subsequent scoped Change and current/sibling reads. No unit Copy
certificate, host world, independent proof, issuer or public API/ABI is added.

The private C17 consumer already emitted correct storage and dataflow. Candidate1
stopped before native execution because its optional observations found only
the wrapper and inner, not the independent-input caller. Candidate2 selects
existing passive hooks from the admitted call-result-to-REPLACE path. It changes
no call admission or value computation. All candidate1 code, oracle, FIRST/raw
and exact host ELF copies are retained.

GCC Release, ASan+LSan and UBSan each pass nine new unit registrations, 85 impact
unit tests and 13 failure/compatibility integrations. Fresh registration censuses
are 2680/2679 allocations (61 clones and three plans); 12 distinct selected points
per source verify existing seeded context/body-owner rollback and retry. Actual
CHECK has 1721 allocations per source, with 16 selected points. These are not
exhaustive sweeps. Every reached outer Copy allocation (45) and inner allocation
(3) is injected with unchanged output/context/owners and retry. Seventeen old
generated C hashes remain identical, including the P1362 returned-Copy control.

Twenty local guests passed: unedited raw/passive GCC/Clang O0/O2 for both sources
and passive ASan+LSan/UBSan. Live main storage is observed before, during and after
the call; all three simultaneous originals are distinct. A observes main old29,
new89, sibling199; wrapper old59 and inner old89/current197/sibling211. B observes
main old47, new111, sibling167; wrapper old61 and inner old111/current233/sibling173.
Pointers are cleared before their objects end; atexit inspects counters/scalars.

`evidence/LOCAL-FINAL.json` binds these results to functional candidate
`11aa0d49145a11ff5bbce4f8458f9cc49b58a00c` and `local-raw.zip`. The archive includes
the baseline trace/original B, frozen candidates and oracles, original FIRST,
commands, configurations, generated C, raw, actual main postmerge logs and exact
baseline/GCC ELF bytes. Other exact host ELF copies remain in the persistent
workspace and are rebuildable from Git and the recorded configurations/hashes.

Scope excludes more main originals, H/Array, multiple formals, cross-call ref
loans, deeper wrappers, multiple inner calls and multiple wrapper calls with the
new main-original shape. These are coverage limits, not new source prohibitions.
The private mutable-host driver contract and independent F/R inputs are unchanged.
The implementation adds maintenance cost to an existing finite relation; broader
library representation protection, general soundness and the author plus ordinary
safe caller C-comparison audit-locality benefit remain unproved. Product GO is
PAUSED and H1 HOLD. Independent review and Coordination adoption are separate.
