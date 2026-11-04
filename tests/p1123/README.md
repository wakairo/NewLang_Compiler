# P1123: finite T-stuttering Continue

This is a private implementation of already selected Draft17.34 LOOP semantics.
It is not a public header issuer, an arbitrary-loop analysis, or a product claim.
The fixed parent is a1091b2672b0a4b1d37e2571ef97bf100882b60b. Its squash merge
73ef10a97f2fab7b70d094b2114359e961acefb5 has the same tree. The original parent
branch, P/R inputs, C219 first observation, and failed C220 candidates are retained.

The supported body has one tail IF comparing a fresh usize parameter against a
usize literal with `==` or `<`. One arm only transfers a usize literal through
Continue; the other performs the existing fixed-field scoped Change/ordinary
Copy-helper Break, independent of the parameter. Either arm order, either T
field, ordinary helper RETURN/tail, source names, and scalar constants are data.
The first consumer remains an actual zero-formal unit entry with ordinary T
storage. H allocation worlds, parameter-dependent Break effects, multiple C/B
sites, general Continue, richer conditions and state-changing recurrence remain
unsupported. Unsupported legal sources keep conservative rejection.

The new trusted relation has these obligations:

1. Capture the real prefix E after the source initializer. H contains that exact
   owner-qualified persistent package/place/current/ambient frame and every
   dependency-free usize in one fresh iteration binding. Both IF arms are
   actually checked with an Unknown parameter; initial bits do not select them.
2. Exhaustively constrain the complete checked predicate/C graph to scalar
   identifier, literal, comparison, block/wrapper and source Continue. This is
   the all-H argument: these operations cannot touch E, regardless of phase.
   In addition, both actual C projections must preserve every persistent
   component. The fresh binding consumes exactly one incarnation/fact ID;
   projection ends it without rewinding either allocator. New dependencies,
   affine state and nonflat temporaries are rejected.
3. Bind both canonical arms and their complete C1+B1 union to this source
   instance, parent, operand, node and nominal target. The private control seal
   is issued only by actual source checking. A supplied exit list/count or
   matching numerical IDs does not establish completeness.
4. B is not a recurrence. Recheck the same phase-independent source arm through
   the real caller transaction, with the same nominal target. The existing
   linear-Break/Copy old-result kernel derives its real caller IDs. The private
   relation checks that abstract and normal executions start at the same E and
   act on the same original field. Abstract-world value IDs are never imported.
5. Retain E, three canonical graphs, four leaf/union exit worlds and the normal
   relation until the original CHECK owner dies. Seal before publication;
   revalidate graph, source, world, callable and canonical membership before
   consumption. All allocations precede the enclosing semantic commit.
   Canonical custody determines destruction even if display arrays/counts
   are changed or replaced by genuine foreign B arrays.

The emitter uses a runtime `for (;;)` and `continue`, a fresh scalar phase per
iteration and the same original T field address at B. A permanently repeating
phase may diverge; no termination proof or static unrolling is substituted.
The old zero-C C17 output is checked byte-for-byte.

Source/oracle revisions and the first compiler and harness REDs are immutable
under evidence/. The initial compiler precision RED exposed an overstrict
fresh-ID comparison. The three subsequent harness REDs were a no-op cardinality
mutation, a sentinel-length constant, and an incorrect CLI status prediction.
Their source/oracle/raw remain present. The final OOM observer checks actual
API OOM plus the existing exact CLI mapping, not merely a nonzero exit.

The finite native oracle observes phase/backedge, original identity, old/current,
sibling/ambient state, one B helper Change, and one caller sibling store while
storage is live. Actual source negative paths preserve named bytes/inode/mtime.
Adversarial proof graphs are validated and destroyed, never emitted or executed.
OOM enumerates registration, actual invocation and the complete named CLI;
each failure must roll back, publish nothing, preserve output and retry with
genuine current evidence. Host ASan/LSan/UBSan also covers these partial owners.

Cost: one abstract body with both arms, one E checkpoint, seven retained world
snapshots/anchors and three node snapshots in addition to the existing normal
Break record. This increases memory and OOM/re-audit sites. The constructor,
complete grammar, normal kernel, custody and consumer form the trusted closure.
Audit locality relative to well-factored C is UNMEASURED; finite capability
success is not general soundness, Product PASS, or permission to merge.
