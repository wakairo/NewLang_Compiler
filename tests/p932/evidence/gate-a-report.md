Track P / C54 P932 **Gate A POSITIVE for a bounded source-derived AST/checked/entry diagnostic adapter**, 2026-10-26 JST. No new branch, repo source/test edit, PR or CI at this report. P930 main fixed3d6e45bc8d89c0524ba0538113433f66f474dee1/treeeab559b1744c0a8ffe5da3474d6c0432b616ba85 rechecked; CURRENT_SPEC→Draft17.34. Compass/Strategy1.1/1.3/Process4.1/4.2/4.3/4.3.2/DDP/DI006/007/011/017 unchanged/read; F74 terminal+C54 read, F72 B only. P927/R928 and P929/P930 frozen evidence untouched.

### New read-only operational evidence (not another Expected-array observation)

Built unchanged pinned main in scratch with existing GCC13/CMake frontend-only tools, no compiler changes. A new read-only GDB probe reaches the ordinary CLI's ORIGINAL parsed unit (`compile_v0_to.unit`), original source and actual nl_body_create_span return, actual checked BLOCK creation and prepare snapshot. It computes a source prefix from the original AST BEFORE inspecting snapshot values. No P930 Expected values or preselected condition is input. Only existing parser nodes/spans and raw decimal token bytes are interpreted; no second grammar parser or source reparse engine.

All four immutable P929/P930 sources ran once on this new probe; ordinary --check exits0, eight qualified snapshots match independently computed source information (definition+actual main each), actual block owners end, zero remaining block records. This is actual runtime evidence, not code inspection alone.

| Original AST / lexer spans derive | Observed result |
|---|---|
| nominal / two declared field labels, two binding names/order | obtained from original function-unit/struct/parameter/binding nodes and SourceView text, not c->entry scalars |
| declaration index vs initializer position | TRUE10/01 with source literals[9,186]/[72,224]; FALSE01/10,[17,232]/[105,64]; order-only01/01,[186,9]/[72,224]; root-swap10/01,[224,72]/[186,9] |
| raw typed literal / condition | decimal from AST-selected token spans, bounded by255 or actual target maximum; TRUE12<40, FALSE21==22; condition computed from these source operands |
| target | actual native size_t unsigned maximum / SIZE_MAX contract used by fixed typed_literal; logical value-bit count64 here, not TypeId7 or a fixed language64 claim; C adapter will compute bits from SIZE_MAX and check actual core usize kind |
| original source→body→checked | actual created plan byte slice equals original function body span; source-binding spans translate by actual body offset, checked aggregate TypeId matches that source nominal's registered declaration/fields, checked literal/result/order agree with independently read AST |
| real owner normalization | child.parent_aggregate=original PlaceId; member.aggregate_owner=original current aggregate ValueId; that package is PLACE carrier with owner_place=original root; fields[index]=child.current_value. Observed member owner VALUE3/6 normalizes to root PLACE1/4, not a numeric cast |
| qualified current block/cut | debugger sees real checked BLOCK creation under function_block's actual function_target, BEFORE source-if prepare/arms. Keys include actual unit AST/source, created plan, definition-vs-actual invocation, block graph/node/function-target and capture event, current Check world + separately borrowed graph world, entry owner. Original owners are observed ending; no post-free access |

Existing semantic views expose all numeric/type/parent/carrier components needed for this LIMITED normalization. No general owner tracker is needed. Binding/field/value categories remain separate. Definition and caller may have the SAME context address/numeric IDs but different original graph/function-target/entry and creation events. A previous scalar record is never a live pointer.

### Tiny conditional implementation path and genuinely new falsifier

Reuse current P930 real-plan/prepare wrappers and four CTests, preserve its Expected table as an unchanged regression oracle. Add a separate bounded adapter that NEVER reads Expected: borrow the already parsed original unit tree (client-owned until teardown), derive source names/literals/permutations/operator, authenticate body-span/registered native TypeId and typed checked graph, normalize actual entry via the package-to-place chain. No extra parser, context/owned-copy engine, Production hook, public API/CLI/backend or allocator. Source/root/condition values flow SOURCE AST -> source packet; actual snapshot values are comparison output only.

Plan size: **two implementation paths**: a small new `tests/p932/adapter.h` (~250–300 lines) and ~10–20 integration lines in existing `tests/p930/observe.c`; no CMake/Production changes anticipated. New data/note/evidence paths only. If substantially larger, STOP before expanding scope. Reuse existing links/real OOM and CLI tests; new adapter itself stack-only, so no invented new OOM claim.

Incremental assertions old P930 does NOT establish:
1. A diagnostic source packet carrying the WRONG checked native TypeId/field relation (e.g. genuine core u8 TypeId in place of original nominal) must reject while the unchanged C snapshot still passes all old Expected scalar rows. New source-declaration→checked aggregate→actual root type equality is checked.
2. Directly using raw member aggregate_owner VALUE as F72 root owner PLACE must reject; only normalization through the actual root-owned package is accepted. Old tests check the raw carrier but do not certify this bridge equation.
3. A real copied definition packet must not be usable as current caller packet, even with equal source/scalars/numeric IDs/address. A genuine earlier source-derived packet from budget_true must not be used for order_only with identical declared values: initializer order and qualified source/block origin differ. Compare scalar packets only; never dereference expired AST/context data.

This makes A/C prerequisites falsifiable beyond replaying a fixture match. New independent property/negative oracle will be frozen BEFORE adapter tests; original source/negative/oracle hashes and first histories remain unchanged. No old oracle repair.

### Exact schema limit / first remaining contract

Packet is `SourcePrefixCore + qualified source/block/capture key + normalized typed entry rows`. Every F72 Prefix formation field's data is available; blockInstance is an abstract alpha-label for that REAL source creation key, not an explicit guest lifetime number. Any numerical key label is diagnostic only; global injective F72 Encoding is NOT proved. C can correlate original block span, actual function target and checked BLOCK at this exact live cut. **It does NOT expose or prove a complete source event history / F72.activeBlocks derivation.** The new adapter will not synthesize activeBlocks from scopes0/implicit_local, or infer stability/access/ending rights. Full history/reachability, general parser/registry refinement, total A/C simulation and D rich lifetime/carrier remain UNPROVED/HOLD.

Preferred existing checked BLOCK/function-target identity needs NO new C hook; debugger creation-event evidence is recorded separately. A new production callback or trace system is not justified. If the original tree/plan/checked-type mapping cannot be maintained while live, or any later semantic/owner mismatch appears, STOP rather than alter source semantics or authority. Stage B at most one natural FULL5+PARTIAL pair, all attempt1, no retries/rebase/merge.

VALUE-ALIGNMENT-RISK separately: source-derived correspondence adds an actual missing relation, but still does not demonstrate reduced author+ordinary-safe-caller UB audit closure relative to well-factored C. Keep code small and reuse actual owners rather than add a private parser/engine. KEEP Draft17.34 / REPLACE NONE; broad CFG/effects/public issuer/output ABI DEFER; Product GO PAUSED/H1 HOLD, R928 INCONCLUSIVE unchanged. Conditional Gate B only; no other Track starts.
