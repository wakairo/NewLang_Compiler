# DLA-1 frozen provenance / semantic oracle (BEFORE any new Rust compiler run)

Fixed NewLang Compiler/main 9fec31cb60ad7191861e402b3c9bd64e2b5a22e3; Draft17.32; Gate 1 ONLY.
Real upstream DaveGamble/cJSON parent 3999b12848214a201bf56ef416b9dac1ec63f811
to historical fix a65abf2f4f1842765761a616e93c14ae25a284fd on 2020-04-03.
One existing upstream 6-line C edit in cJSON_DetachItemViaPointer:
  else if (item->next == NULL) { parent->child->prev = item->prev; }
Historic issue: last nonhead detach leaves first child's cached tail prev
stale, and existing add_item_to_array/suffix_object follows it; deleting
the removed B before append C can make a freed-original-node writer.
This is source-level V526 evidence, not a new ASan/native run of old C.

Required public semantic oracle: original A,B,C heap objects; preserve
pointee addresses while live; maintain original array relative order;
public detach-at-index, detach-by-identity (not arbitrary raw stale deref),
delete-at-index; detach returns EXACT original heap Node owner, so it can be
re-adopted without allocation/reconstruction; delete frees original once;
last-delete B then append C yields A,C; head/middle/last/singleton detach;
invalid locator/out-of-range returns normal None/false without mutations;
no raw Node ptr dereference is required of normal callers; release once.
C public ABI (cJSON*) and O(1) detach-by-live-pointer are NOT matched by the
Rust safe Handle (u64 nonce + numeric original pointer identity), whose
lookup over Vec<Box<Node>> is O(n); Array index deletion is O(n) already
under C cJSON get_array_item lookup, but C direct-pointer detach can be O(1).
Vec growth moves Box handles, never original Box<Node> pointees. Additional
Vec storage/reallocation differs from C's pointers-only intrusive chain.

CHOSEN fair initial Rust alternative: stable std-only Vec<Box<Node>>, not
a fake general raw-linked graph; exact original allocation identity and
detached/reparent semantics do NOT require C's accidental head.prev cached
tail representation. BEFORE source exposes append/read/identity handle.
AFTER source adds detach_at, delete_at, detach_by_handle without altering
old caller signatures or existing methods. This tests one real incremental
public behavior maintenance episode, NOT full cJSON JSON parsing, object key
lookup, serialization, arbitrary cJSON* ABI or O(1) pointer detach parity.
Mature alternative if O(1) pointer removal is truly essential:
intrusive-collections crate LinkedList<Adapter/Box<Node>>, supports owned Box
and cursor_mut.remove, but cursor_mut_from_ptr is unsafe without genuine live
membership proof; complex full parity is not asserted or run.
V552 separate limited Rust Pin/raw same-original intrusive positive is prior
feasibility evidence only, not itself new incremental edit.

Expected pre-change Rust:
  rustc --edition=2021 -D warnings --test rust_before.rs
  1 test PASS, normal binary one-line:
  DLA1_RUST_BEFORE_OK stable_original_boxes; append; safe_get; freed2
Expected post-change Rust:
  rustc --edition=2021 -D warnings --test rust_after.rs
  4 tests PASS; normal binary one-line:
  DLA1_RUST_AFTER_OK old_contract; last_delete_append; same_box_detach_reparent; head_middle_last_singleton; frees_exact
A true PASS requires actual recorded rustc version and source hash.
No performance, global safety/fuzz proof, person-hours or controlled AI
effect measured. No repeated V526 old C bug/crash. Fail-closed on absent
pinned Rust compiler: mark runtime unmeasured; no installing nightly.
