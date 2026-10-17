# P572 — finite three-original lexical link candidate (DO NOT MERGE)

Track P. Fixed main a310c9b9dae5b507b2388fcc6cb39597267d1448, CURRENT_SPEC
Draft17.32. M571 was NOT read or used. No normative/source-rule choice, heap
owner, general CFG/effects, Array activation or H1 resumption. This is one
bounded implementation of existing checked source, not an ordinary-safe library.

## Operation and preregistration

Original tests/p471/asymmetric.nl SHA256
c578ebdce71ffd46e30758903c28f69227876c7904264b3eec1914c7f6bdd050 is byte-identical.
Its current-main CHECK/public join/C17/native GCC O0/O2 plain+passive success is
baseline calibration, NEVER a new capability. Its historical old-main failure
is not a claim about current main.

New P source frozen BEFORE candidate execution/Compiler edits, 13:19:43UTC:
three_originals.nl SHA256
b3154e8ad1766dfa36fc948a3945f69058d1c4e023178cb6b49e4dfbcf6f2eae.
Three genuine lexical originals middle/left/right, payload22/11/33, one selected
next:Option<ptr<Node>> field per Node. Initial middle.next=None,
left.next=Some(original middle),right.next=None. TWO scoped changes:
left.next=Some(original right),right.next=Some(original left). Return old Copy
Some(original middle) and None into source locals; final left/right are reciprocal
next links, middle.next and all payloads unchanged. This uses two neighbors'
NEXT slots, **not a newly admitted next/prev Node or a complete doubly linked
list detach**. Three-field next/prev/payload is not currently structurally
admissible; four-field H is tied to five fallible allocations. Neither was
invented/adopted to make this experiment work.

Frozen oracle includes original root/field/projection, old Copy, physical
addresses, final link identities and negative expectations. Original negative
predictions are retained even where the first refusal is only a profile limit.
Separately frozen ptr-as-ref calibration supplies an actual semantic authority
type rejection; source variants/oracles were not silently rewritten after use.
Four-root/third-write source-negative boundary probes are separately frozen
before their executions and do not replace the positive.

Per-operation cost contract: two direct scoped field replacements, no search,
extra guest heap allocation, membership validation, owner return or fail/refund.
No claim of a complete scalable O(1) remove-known-node operation. The three fixed
lexical values, ordinary static Copy Node/Option/ptr and zero explicit A/D/R
values are NOT original heap ownership. Native C has normal temporary value
carriers plus the three actual binding carriers; zero-cost lowering/ABI is not
promised.

## Measured first boundary and gate

UNCHANGED fixed compiler: SOURCE/CHECK0, actual registered source function body
owns metadata for3 distinct original roots and2 Change operations, C17 CLI4
V1-BACKEND-UNSUPPORTED, empty output (backend diagnostic has no source span).
First physical guard checked_c_node.c:829 rejects the third original binding;
final gate2812 also demands exactly2. The third source binding is right on line7.
Initial Some remains the existing second-Node initializer, not a new third-Some
case. Checked source already selects valid underlying operations.

Before edits the public captured-change collection is0 for this straight-line
profile. Existing public node_join is deliberately exactly2 originals/two match
arms, hence N/A, not a broken validator. Final lexical root current facts and
fixed-field arrays are cleared by normal EndRoot. Observing numeric checked
fields alone cannot substitute for live point-in-time replay evidence.

The first25–30min gate (actual elapsed25.2min, see PREFLIGHT) found one finite
vertical implementation path using existing source capture/clone/destruction,
ordinary nl_fixed_change and full-world nl_packet_same_entry. No new authority,
source grammar, general CFG or general state engine needed. Conditional GO was
recorded before publishing PR/CI. Prototype positive/native and negative/OOM
were established in that window; do not call it a root-counter-only patch.

## Limited implementation / proof boundary

- Reuse existing captured Change storage (capacity6 UNCHANGED), only first2
  actual replacements when exactly3 live two-field lexical originals exist,
  no allocated slice and no enclosing Node join certificate. Additional writes
  remain semantically checked; full-coverage validation refuses this finite C
  profile. Existing two-root normal-join capture/validator behavior is unchanged.
- New public observational NLNodeLinksView/validator requires a real retained
  registered zero-argument unit body; no provisional source-fragment certificate.
  Authenticate owned origin/world/checkpoint pointers, source-order operations
  and complete2-write/3-receiver coverage. A copied view grants no authority;
  candidate pointers are compared, not dereferenced. Source input AST/source
  are destroyed by the existing registered-route loader before checking main().
- Independently validate original live implicit-root/child incarnation relations,
  exact original scoped write ref, current field AND root ValueFacts, old Copy
  and incoming source-issued Option contents/provenance. Replay EACH core Change
  on a private clone and compare its FULL semantic world to the issued post;
  refresh the actual parent binding just as the source issuer does. This checks
  old occurrence retirement and new fresh occurrence plus independent frame.
- Connect the original object state between first post and second pre across
  legitimate scope/Copy-local bookkeeping: compare root, child, occurrence and
  recursive current package contents, including pointer provenance/dependencies,
  not equal pointer/ID bits alone. Reuse exact existing package comparison through
  one private wrapper; old callers are unmodified.
- Only successful public certification enables3 lexical C carriers. Existing
  exact2 normal-join/heap gates, normal_frame_unchanged, pointer/local root
  resolution, original initial-Some validation and allocated lifecycles remain.
  The emitter is a proof consumer, not an issuer reconstructing new roots.

## Actual identities and outcomes

| original | source symbol | root/incarnation | next child/incarnation |
|---|---:|---:|---:|
|middle|1|1/1|2/2|
|left|4|6/6|7/7|
|right|7|12/12|13/13|

Left Change: rootRF6->19, fieldF7->18, Some occurrence1->2; old target original
middle1, new target original right12. Right Change: rootRF12->25,
fieldF13->24, None occurrence0->Some4; oldNone, new target original left6.
Both root incarnations and field incarnations persist. Middle and payload
siblings retain currentness; old Some1 ends. All original lexical bindings end
normally in final state. Explicit regions/domains/Allocation responsibilities0;
no Copy locator is treated as a safe-ref or owner issuer.

SOURCE/CHECK/public-certified-owned/C17/NATIVE candidate path all reached for
unchanged frozen positive. Actual generated C2807bytes SHA256
 d98b72d7c6d5d3578ab5c14ee4adb2261bb41c345f77a03259461831b2ef36bf.
Strict GCC14.2/Clang23.1.2 O0/O2 plain and passive-observed native executions0;
observer receives only addresses of actual source binding carriers and checks
three distinct/nonoverlapping original addresses, same physical next slots,
old Copy targets, both actual writes, final reciprocal targets, unchanged
payloads. It writes only its own counters/I/O; no guest value/rights injection.
Local Clang ASan/LSan tests also pass. Full locked CI23.1.3 is separate evidence.

Source negatives, all CLI3/no C:
- read_authority: parser LOCAL-LOAN-PROFILE,9:32–9:33; semantic NOT ENTERED.
- wrong_field_type: semantic source-profile NODE-LINK-FIELD-PROFILE,9:29–9:41;
  NOT a demonstrated runtime wrong-field authority rejection.
- scope_escape: semantic P8-EXIT-DEPENDENCY,9:17–9:46.
- ptr_is_not_ref: semantic P3-WRITE-REF-REQUIRED,10:17–10:23; Copy ptr does not
  supply ref<write,T>. Its separate freeze preserves original negatives.
All registration failures rollback; parser failure leaves state unchanged.
Four roots and a third write stay CHECK0 but C17 CLI4; no new success categories.

Copied public-view corruptions17 refused. Genuine source-owned internal fault
controls17 refused by PUBLIC validator AND PUBLIC C consumer, output slots
unchanged: wrong root/incarnation/field, stale RF/F, wrong old Copy, old
occurrence, missing/duplicate/reordered evidence, stale post fact, sibling scalar
mutation, occurrence resurrection, and scoped-ref scope/write/provenance changes.
A COUPLED change of old left link content in BOTH second snapshots remains
well-typed and passes actual isolated core Change replay, but the sequence bridge
rejects it against first post. Negative cloned/replayed worlds are derived ONLY
from actual source checkpoints and are never positive source authority.

Validator OOM74 allocation positions and full C consumer OOM75 (staging+replay)
refuse with unchanged fingerprints/outputNULL/length77. Source OOM registration435
and main289 positions rollback (unchanged baseline297/151; +138 per pass from
actual snapshots). No resource/capacity limit was raised. Retains4 source world
clones per certificate, plus sequential temporary replay clones. Actual peak
memory, compile/native timing distributions and scalability were NOT benchmarked.
The Node C staging buffer remains existing131072bytes and local budget128.

Local full frontend regression319/319 PASS73.07s; latest targeted4/4 and
Clang ASan/LSan4/4 PASS. Baseline/new logs and source-issued C/physical output
are retained under evidence/. Baseline helper initially asserted an ended root
still retained its fixed-field array; corrected the observer after inspecting
actual EndRoot (root.incarnation retained, currentRF/fixed fields cleared), not
source or compiler semantics. One baseline harness invocation had an extra
argument to existing measure(); corrected before baseline native execution.
No CI repair or source/oracle change was required by these local harness issues.

## Next reusable step / product and independence

Shortest next library-facing upgrade is ONE ordinary named link-field function
with genuine caller ref/Copy arguments and body-sensitive caller-visible poststate,
not a complete detach. Static current body_signature_type still excludes nested
ref<write,Option<ptr<Node>>> / Option<ptr<Node>> result; P475/experimental PR479
are distinct, and no such helper is claimed merged or tested here. Reassess that
single boundary separately; do not hide it with type-name allowlists. This task
stops after the three-original primitive, without adding that call profile.

M371/372 independently known remaining barriers for complete safe operations:
original heap nonCopy detached receipt + failure refund/poststate SOURCE, and
protected membership/adjacency invariants for ALL safe callers. A Copy ptr and
local reciprocal links solve neither. Real heap ownership/lifecycle, failure
worlds0–5, H1 detach/adoption and product contract remain HOLD/UNDECIDED.

Goal/Evidence/Alternatives/Stop: make one adopted low-level operation reusable
through honest source/owned/native proof / bounded lexical capability and static
refusals only, no author+ordinary-caller UB-audit delta / well-factored C remains
baseline, V567 mature intrusive Rust safe opaque handle validates+unlinks+returns
same Box in bounded worst-case O(1), with private registry and retained historical
slot storage, but exact C raw ABI and actual consumer maintenance constraints
remain unproved / Coordination chooses independent R and adoption/next step.
Rust feasible native execution alone is not practical sufficiency, nor is Rust's
unsafe count a NewLang advantage. No named consumer's need for C was established
by this compiler task. Product GO stays HOLD.

Historical audit N/A faithful selected Node/Option/ref/Change implementation;
KEEP DI001/007/010 core authority/currentness, DEFER general owner/receipt,
module privacy and additional source families. Read Compass, actual Strategy1.1–2,
Process4.2/4.3.1, V526/V567 and M371/372; M571 not consulted. No specification,
Product Strategy, existing source/expectation, LLVM/bootstrap, existing PR, Array
profile, merge, Issue close or other Track change. New proof API/candidate backend
trust boundary still needs Coordination's independent R decision. DO NOT MERGE.

CI: pending the candidate's one natural FULL5 + NO LLVM/PARTIAL pair; the final
Issue report and PR description will record exact HEAD, runs and outcomes.
