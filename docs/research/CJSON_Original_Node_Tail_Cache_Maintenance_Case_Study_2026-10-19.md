# When a cached tail outlives a removed node: a reproducible cJSON maintenance case

*Internal technical article — DRAFT for C/Rust maintainer and editorial review, 2026-10-19. Not published, not a security advisory, and not a claim about today's cJSON source or NewLang product readiness.*

A small pointer-list edit can be locally correct yet leave a second representation of the same list stale. A **real historical cJSON revision from 2020** illustrates this with a cached tail pointer: remove the last node, free it through a normal library API, and a later valid append writes through the freed node. We can now distinguish the original code path, a minimal model, and **GCC and Clang AddressSanitizer executions of both pinned upstream revisions**. The broader lesson applies to C libraries and Rust intrusive containers whether or not their maintainers are interested in a new programming language.

## 1. Exact scope: original nodes, membership, cached tail, ownership

The pinned upstream [cJSON struct](https://github.com/DaveGamble/cJSON/blob/3999b12848214a201bf56ef416b9dac1ec63f811/cJSON.h#L103-L123) has `next`, `prev`, and `child` pointers. In the 2020 implementation, `parent->child` points to the **original first child**. Its `prev` has a special meaning: for a nonempty array/object list it caches the **last child**, not the first child's normal predecessor. In a singleton `[A]`, `A.prev == A`; in `[A,B,C]`, `A.prev == C`. `C.next == NULL`. This is a fast-append cache, not a circular doubly linked list.

For every nonempty list with first original node `H` and last reachable original node `T`, the maintenance invariant is:

- `parent->child == H`, `H.prev == T`, and `T.next == NULL`; a singleton has `H == T`.
- Except for the **first node's dual-use `prev`**, adjacent members have reciprocal `next`/`prev` links.
- The cached `T` is still **a live, currently owned member of this list**. A non-NULL address does not establish membership or lifetime: a detached node may be live but outside the list, or already freed.

The [upstream README's ownership contract](https://github.com/DaveGamble/cJSON/blob/3999b12848214a201bf56ef416b9dac1ec63f811/README.md#L214-L239) matters. `cJSON_Create*` allocates original nodes. A **successful** `cJSON_AddItemToArray` transfers ownership to the array: the ordinary caller must not also delete that owned node. A detach API returns the **same original node** and transfers its ownership back to the caller. `cJSON_DeleteItemFromArray` instead detaches then destroys that node; `cJSON_Delete(parent)` tears down the parent's remaining owned children (subject to cJSON's reference/constant-string flags). The reproduced path uses successful creation/adds, simple number leaves, a public deletion, and a public append. No stale pointer or foreign member is supplied by the caller.

**O(1) needs an operation boundary.** Given an actually current, valid member pointer, [`cJSON_DetachItemViaPointer`](https://github.com/DaveGamble/cJSON/blob/a65abf2f4f1842765761a616e93c14ae25a284fd/cJSON.c#L2190-L2226) unlinks a bounded number of links and returns the original node in **O(1) in list length**. Its raw-pointer API relies on the caller/author meeting its current-member/lifetime preconditions. The public `cJSON_DeleteItemFromArray(parent, index)` used below does an **O(n) index lookup** first, then unlink and deallocation; deleting a node with owned children adds subtree work. Do **not** describe this complete indexed public operation as O(1). Cached-tail append is constant time while its cache invariant holds; the old implementation also has a scan fallback when the cache is null.

## 2. What the 2020 fix corrected — and why it was easy to miss

The exact historical change is [DaveGamble/cJSON commit `a65abf2f4f1842765761a616e93c14ae25a284fd`](https://github.com/DaveGamble/cJSON/commit/a65abf2f4f1842765761a616e93c14ae25a284fd), dated **2020-04-03**, parent [`3999b12848214a201bf56ef416b9dac1ec63f811`](https://github.com/DaveGamble/cJSON/tree/3999b12848214a201bf56ef416b9dac1ec63f811). It adds a last-but-not-first case to `cJSON_DetachItemViaPointer`. The decisive new original upstream statement is:

```c
parent->child->prev = item->prev;
```

It executes when `item != parent->child` and `item->next == NULL`: after unlinking the tail, the **current first child's cached tail** must point to the former predecessor. The actual fix includes that branch and its comment, not a new owner allocation or a new list representation ([pinned fixed source](https://github.com/DaveGamble/cJSON/blob/a65abf2f4f1842765761a616e93c14ae25a284fd/cJSON.c#L2190-L2228)). This one-line quotation is from cJSON, copyright © Dave Gamble and contributors, MIT license; see [original LICENSE](https://github.com/DaveGamble/cJSON/blob/3999b12848214a201bf56ef416b9dac1ec63f811/LICENSE).

| Original member operation | Old cached-tail outcome | Corrected outcome | Reason |
| --- | --- | --- | --- |
| Detach the only node `A` | Parent becomes empty | Same | No head/tail remains |
| Detach first `A` from `[A,B,C]` | New first `B`; `B.prev == C` | Same | The new first already carries the tail |
| Detach middle `B` from `[A,B,C]` | `A.next == C`; `A.prev == C` | Same | Existing cached tail is still a member |
| Detach last `C` from `[A,B,C]` | `B.next == NULL` but **`A.prev == C`** | **`A.prev == B`** | Missing *additional* cached-tail writer |
| Detach last `B` from `[A,B]` | `A.next == NULL` but **`A.prev == B`** | **`A.prev == A`** | Smallest broken case |

The ordinary link write `B.prev->next = NULL` does not update `A.prev`: the latter is a **separate representation of the tail**, read elsewhere. Old [`add_item_to_array`](https://github.com/DaveGamble/cJSON/blob/3999b12848214a201bf56ef416b9dac1ec63f811/cJSON.c#L1945-L1996) reads `child->prev` and passes it to [`suffix_object`](https://github.com/DaveGamble/cJSON/blob/3999b12848214a201bf56ef416b9dac1ec63f811/cJSON.c#L1922-L1926), which writes `prev->next = item`. After `B` is detached and freed, the stale `A.prev` makes that a write into freed `B` storage.

**Do not collapse membership and lifetime errors.** If the caller merely **detaches** `B` and retains the returned owner *alive*, the old cache points to a **live but no-longer-member** `B`. An append can mutate detached `B.next` instead of reachable `A.next`: topology/membership corruption, not necessarily use-after-free. The specific **delete** path then frees `B`, making the later append a proven heap-use-after-free write. A next-only count check *immediately after removal* could miss the stale cache.

## 3. A valid public API sequence and the observed failure

The actual pinned C #617 workflow builds and executes the following ordinary `cJSON.h` API sequence. This readable listing mirrors its source; for **byte-exact `repro.c` SHA256**, obtain the heredoc from the [frozen workflow](https://github.com/wakairo/NewLang_Compiler/blob/e3017a1e0bb44cf0258b4061c8bec2e1d2213ba0/.github/workflows/c617-upstream-cjson-asan.yml) rather than trusting transcription or whitespace in an article.

```c
#include "cJSON.h"
#include <assert.h>

int main(void) {
    cJSON *p = cJSON_CreateArray();
    cJSON *a = cJSON_CreateNumber(1);
    cJSON *b = cJSON_CreateNumber(2);
    cJSON *c = cJSON_CreateNumber(3); /* allocated BEFORE deletion */
    assert(p && a && b && c);

    assert(cJSON_AddItemToArray(p, a));
    assert(cJSON_AddItemToArray(p, b));
    cJSON_DeleteItemFromArray(p, 1);  /* original B is detached and freed */
    assert(cJSON_AddItemToArray(p, c));
    assert(cJSON_GetArraySize(p) == 2);
    assert(cJSON_GetArrayItem(p, 0) == a);
    assert(cJSON_GetArrayItem(p, 1) == c);
    cJSON_Delete(p);
    return 0;
}
```

Preallocating `C` before freeing `B` makes the original-node identity comparison unambiguous and avoids relying on allocator address reuse. The assertions check the expected `[A,C]` list, the **same original A/C pointers**, and parent teardown; they do not prove all other library paths correct.

The faulty causal chain is **writer → free → later reader/write**:

1. Two adds: `p.child=A`, `A.next=B`, `A.prev=B`, `B.prev=A`.
2. `DeleteItemFromArray(p,1)` locates current `B`, the old detach writes `A.next=NULL` but neglects `A.prev`, then `cJSON_Delete(B)` **frees the original B**.
3. `AddItemToArray(p,C)` legitimately consults the cached tail `A.prev`. Old `suffix_object(B,C)` writes `B.next=C` after free.
4. The fixed code resets `A.prev=A` on tail detach; the later append instead links `A.next=C`, `C.prev=A`, `A.prev=C`.

The **caller need not know** the first-child tail-cache encoding to use the documented add/delete APIs correctly. The defect is an internal library-author maintenance error, even though the library is written in C.

## 4. Reproducibility: real upstream revisions, hashes, and ASan output

**Primary source identity.** C's [Issue #617 final evidence](https://github.com/wakairo/NewLang_Compiler/issues/617#issuecomment-6260349673) used an isolated pinned [GitHub Actions run #39204409642](https://github.com/wakairo/NewLang_Compiler/actions/runs/39204409642), one successful attempt, single [job #117904005205](https://github.com/wakairo/NewLang_Compiler/actions/runs/39204409642/job/117904005205). That job fetched **the actual unmodified 2020 upstream `cJSON.c`, `cJSON.h`, and MIT `LICENSE`** at the two full commit SHAs, checked Git blob identities **before compilation**, recorded SHA256, compiled four original code/harness combinations, and inspected the real sanitizer and fixed assertions. The workflow commit `e3017a1e0bb44cf0258b4061c8bec2e1d2213ba0` is on a **separate non-main test branch** and is not a NewLang compiler change.

| Verified input | Revision / Git blob SHA-1 | SHA256 of source bytes in C617 |
| --- | --- | --- |
| Upstream **old** `cJSON.c` | [`3999b12848214a201bf56ef416b9dac1ec63f811`](https://github.com/DaveGamble/cJSON/blob/3999b12848214a201bf56ef416b9dac1ec63f811/cJSON.c) / `500fcf574fdc4a2aa79b662505d42276df11f7f6` | `caa480cced80c56c10a0ef5569939d64ca8ef4dd908ca4a5368135b18dfaad4c` |
| Upstream **fixed** `cJSON.c` | [`a65abf2f4f1842765761a616e93c14ae25a284fd`](https://github.com/DaveGamble/cJSON/blob/a65abf2f4f1842765761a616e93c14ae25a284fd/cJSON.c) / `cc1c0166773a0adc316c77f6754be897c414d24b` | `270b942f564a068f9e67267f92fd91a0156faca85cf3788c6af13cf6e4f7fecc` |
| Shared `cJSON.h` (identical at both) | `0c6c8e070efd754803e8d8b6549e2bb34315e9a4` | `a2a312b4aaa74be3ae0bef9775cbd98e26f46cfe05ff5268227e330b243582e9` |
| Shared upstream **MIT LICENSE** | `78deb0406d713ab9730e3c2447be1abdbd70b9a2` | `a36dda207c36db5818729c54e7ad4e8b0c6fba847491ba64f372c1a2037b6d5c` |
| Exact original workflow-generated `repro.c` | [C617 workflow heredoc](https://github.com/wakairo/NewLang_Compiler/blob/e3017a1e0bb44cf0258b4061c8bec2e1d2213ba0/.github/workflows/c617-upstream-cjson-asan.yml) | `bdc606cab8674767fdbbac5c145153915535b8fb319559f4adadf5510c723d05` |

The C617 runner was **Ubuntu 24.04**, GCC **13.3.0**, Clang **18.1.3**, with the same strict flags for all four builds: `-std=c17 -Wall -Wextra -Wpedantic -Werror -g -O0 -fsanitize=address -fno-omit-frame-pointer`, linking `-lm`; runtime `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`. All four compilations passed without warning suppression. These are **observed outputs of the C617 job**, not executions newly performed for this article.

| Actual original-source build/runtime | Old upstream parent | Fixed upstream child |
| --- | --- | --- |
| **GCC 13.3.0 / ASan** | **Exit 1**; `AddressSanitizer: heap-use-after-free`, **WRITE of size 8**, `suffix_object` **`parent/cJSON.c:1924`**; `add_item_to_array:1975` → public add `:1995`. Freed by `cJSON_Delete:267` through `cJSON_DeleteItemFromArray:2232` | **Exit 0**; empty ASan stderr, count `2` and original `A`/`C` address assertions pass, parent teardown completes |
| **Clang 18.1.3 / ASan** | **Exit 1**; same `heap-use-after-free` **WRITE of size 8** at `suffix_object:1924`, `add_item_to_array:1975`, with freed stack `DeleteItemFromArray:2232` → `cJSON_Delete:267` | **Exit 0**; empty ASan stderr, same count/identity assertions and teardown complete |

The [original log](https://github.com/wakairo/NewLang_Compiler/actions/runs/39204409642/job/117904005205) records allocation of original `B` via `cJSON_New_Item:239` / `cJSON_CreateNumber:2415`, the free, and subsequent write, not just a zero/nonzero script result. The final logged marker `C617_ALL_FOUR_ORIGINAL_NATIVE_RUNS_VERIFIED` followed these checks. **Observed fact:** this specific historic pinned, successful-public-caller path produces UAF before and passes cleanly after the patch on these two toolchains. It is **not** evidence of exploitability, a CVE, vulnerabilities in the current upstream branch, complete correctness of every fixed path, or general fault-injection/FFI safety.

### Reproduce against the same upstream bytes

The minimal procedure below uses the actual pinned public upstream sources (not a reimplementation). Run in a **disposable test directory** on a machine with Git, curl, C compilers, and ASan. Use the **byte-exact original `repro.c` heredoc in [C617's workflow](https://github.com/wakairo/NewLang_Compiler/blob/e3017a1e0bb44cf0258b4061c8bec2e1d2213ba0/.github/workflows/c617-upstream-cjson-asan.yml)** to match its SHA. The readable listing above is sufficient to exercise the same API path but is **not claimed to match that byte hash**.

```sh
old=3999b12848214a201bf56ef416b9dac1ec63f811
fixed=a65abf2f4f1842765761a616e93c14ae25a284fd
for pair in "parent:$old" "fixed:$fixed"; do
    phase=${pair%%:*}
    revision=${pair#*:}
    mkdir -p "$phase"
    for file in cJSON.c cJSON.h LICENSE; do
        curl -fL "https://raw.githubusercontent.com/DaveGamble/cJSON/$revision/$file" \
            -o "$phase/$file"
    done
done

# Confirm pinned file contents BEFORE compiling (git hash-object means Git blob SHA).
git hash-object parent/cJSON.c   # 500fcf574fdc4a2aa79b662505d42276df11f7f6
git hash-object fixed/cJSON.c    # cc1c0166773a0adc316c77f6754be897c414d24b
git hash-object parent/cJSON.h   # 0c6c8e070efd754803e8d8b6549e2bb34315e9a4
git hash-object parent/LICENSE   # 78deb0406d713ab9730e3c2447be1abdbd70b9a2
sha256sum parent/cJSON.c fixed/cJSON.c parent/cJSON.h parent/LICENSE
sha256sum repro.c  # exact C617 harness: bdc606cab8674767fdbbac5c145153915535b8fb319559f4adadf5510c723d05

for cc in gcc clang; do
    for phase in parent fixed; do
        "$cc" -std=c17 -Wall -Wextra -Wpedantic -Werror -g -O0 \
            -fsanitize=address -fno-omit-frame-pointer \
            -I"$phase" repro.c "$phase/cJSON.c" -lm -o "$cc-$phase"
    done
    # Old parent intentionally fails under ASan; run without shell fail-fast.
    ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 "./$cc-parent"
    # Fixed child: expected exit 0, no ASan diagnostics, assertions pass.
    ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 "./$cc-fixed"
done
```

Do not substitute `main`, an arbitrary modern release, or only an independently written model. For a rigorous new check, verify the displayed Git hashes and SHA256 **against the table**, capture stdout/stderr/exit code separately for all four original binaries, and retain the LICENSE with copied upstream source. The deliberately failing old run must not be classified as a CI failure if the test is asserting that particular diagnostic; the [C617 workflow](https://github.com/wakairo/NewLang_Compiler/blob/e3017a1e0bb44cf0258b4061c8bec2e1d2213ba0/.github/workflows/c617-upstream-cjson-asan.yml) demonstrates that explicit classification. Exact stack formatting, allocator addresses, and sanitizer exit status can vary by environment.

### Prior evidence that must stay separate

| Who and when | Evidence level | What it **did not** establish |
| --- | --- | --- |
| [V #526](https://github.com/wakairo/NewLang_Compiler/issues/526#issuecomment-6214336649) | **Static source-only** identification of the 2020 tail-cache reachability problem | No native sanitizer run |
| [V #615](https://github.com/wakairo/NewLang_Compiler/issues/615#issuecomment-6259941685) | An independent **33-line simplified C17 MODEL**, GCC **14.2.0** and Clang **17.0.0** old-model ASan UAF WRITE; fixed-model exit 0; model SHA256 `a23db324bf3627c7185ba35e7ab6850d55f2801e138f19477a31eed7e14a14f1d` | **Not** actual upstream `cJSON.c` execution |
| [V #616](https://github.com/wakairo/NewLang_Compiler/issues/616#issuecomment-6260201481) | **Environment-blocked** Chat executor (no upstream source download), upstream native runs **zero** | Neither an upstream PASS nor a compiler defect |
| [C #617](https://github.com/wakairo/NewLang_Compiler/issues/617#issuecomment-6260349673) | **Real original upstream** two pinned revisions × GCC/Clang ASan, exact frozen bytes and real public caller, results in the table above | Not a V616 rerun, not independent R, not broad current-release safety proof |
| [V #567](https://github.com/wakairo/NewLang_Compiler/issues/567#issuecomment-6233871201) | **Different, actual compiled Rust** intrusive-list/safe-handle experiment | Not a direct cJSON API/ABI or lifetime-research proof |

## 5. The real audit boundary: library author versus ordinary public caller

A useful maintenance dependency graph for this *specific* defect is:

```text
ordinary successful public caller: Create(A,B,C) -> Add(A) -> Add(B)
    -> DeleteItemFromArray(index 1) [lookup O(n)]
         -> DetachItemViaPointer(B) [valid known-member unlink O(1)]
              writer: predecessor.next <- NULL
              MISSING writer: first.prev (cached tail) <- predecessor
         -> cJSON_Delete(B) [original B lifetime ends]
    -> AddItemToArray(C)
         -> add_item_to_array reads first.prev cached tail
         -> suffix_object(tail,C) writes tail.next [stale B => UAF]
    -> cJSON_Delete(parent) [teardown of current owned list]
```

**Library author responsibility:** maintain original member/owner/lifetime, the special `head.prev` shortcut, reciprocal links, and their dependencies across every **legal writer** that can change head/tail/membership (detach, insert/replace, append and relevant object/item paths), plus the **readers** (`add_item_to_array`, `suffix_object`), the delete/free chain and its teardown/error branches. The minimal re-audit closure is the actual set of functions and operations capable of establishing, changing or relying on the same invariant—not merely the touched line, but also **not the entire repository by default**. One additional link write is easy; the hard part is noticing its dependency on the later append and freed original `B`. Keep **missing cases** explicit: allocation/OOM recovery, reference flags, reentrancy, concurrency and FFI were not exhaustively tested by C617.

**Ordinary documented C caller responsibility:** allocate/check inputs, use supported public APIs with their success/ownership-transfer rules, never free an already-transferred item, release or reattach a returned detached owner exactly once, avoid use after a delete, handle failure results, and meet current-member preconditions when choosing raw-pointer APIs. **The compliant indexed DeleteItemFromArray→AddItemToArray caller does not maintain the hidden `head.prev` cache**. A well-factored C library *already localizes that part of the burden to its author*. Comparing NewLang to a straw-man caller that manually rewires `prev` would be unfair. Internal assertions of `head.prev == reachable_tail`, an invariant-preserving helper for tail mutations, and an ASan regression for last/singleton/head/middle/delete→append can improve well-factored C without changing language.

**Change impact, not unsafe-line counting:** changing the tail-detach branch obliges reviewing legitimate tail writers, the cached-tail append reader, and freeing/teardown edges that affect the pointer's validity. In contrast, changing a **pure scalar-only formatting routine** with no reachable writes to list/owner/allocator state ordinarily requires rechecking its formatting behavior, **not** the full tail-cache invariant. Good C encapsulation already achieves that degree of audit locality. Any other language's claimed advantage needs to show a further concrete reduction in the **library author *and* ordinary-safe caller** review/re-review dependency closure on the same useful operation; neither ASan finding counts nor bare `unsafe`/unchecked line counts settle it.

## 6. Practical Rust counterexample: safe-handle O(1), real costs

A strong comparison is **not** an O(n) `Vec<Box<Node>>` scan/removal presented as the best Rust can do. The independent [V #567 final](https://github.com/wakairo/NewLang_Compiler/issues/567#issuecomment-6233871201) built an actual wrapper using **`intrusive-collections =0.10.3`**, `memoffset =0.9.1`, **rustc 1.99.0**, and boxed original heap nodes with embedded intrusive `LinkedListLink`. Its frozen [Rust source](https://github.com/wakairo/NewLang_Compiler/blob/2012e04bf3a4e17f940bbb31d879ccef6a57e990/experiments/value/dla2_o1/src/main.rs), [Cargo dependencies](https://github.com/wakairo/NewLang_Compiler/blob/2012e04bf3a4e17f940bbb31d879ccef6a57e990/experiments/value/dla2_o1/Cargo.toml), [oracle](https://github.com/wakairo/NewLang_Compiler/blob/2012e04bf3a4e17f940bbb31d879ccef6a57e990/experiments/value/dla2_o1/oracle.txt), and independently completed [native Actions #39032086097](https://github.com/wakairo/NewLang_Compiler/actions/runs/39032086097) (attempt 1, SUCCESS) make its claim inspectable.

It has a private linked list and `Vec<Slot>` registry. `append(Box<Node>)` returns an **opaque typed `Handle`** containing list/pool ID, slot index, generation and original numeric address. `detach_known(handle) -> Option<Box<Node>>` performs a bounded pool check and one indexed slot lookup, verifies generation/current pointer/address **before raw dereference**, uses a **private locally unsafe** `cursor_mut_from_ptr(...).remove()` on the actual member, invalidates the slot, and returns **the exact original Box/heap address**, not a rebuilt value. The slot's current-member invariant is an **author-maintained obligation**. The public safe caller uses a validated handle and gets either the original owner or `None`, without having to prove raw membership on each call. This is a stronger safe-call contract than passing any old `cJSON*` into a raw-pointer API.

The complete **known-current-handle detach**, including validation and owner transfer, is **worst-case O(1) in live list length** by source-path/linked-list analysis—not a timing benchmark. Its **four actual unit tests passed**: historical delete-then-append with original nodes; exact original `Box<Node>` detach and reparent; stale/foreign/wrong generation/slot/address rejection; and scale cases `n=3,16,256` with **one registry probe** per attempted known-member removal. The test oracle's list traversal is *outside* this operation. It also has a no-error standalone binary run. No human-hours or sustained product use were measured.

The costs are substantive:

- **Different caller API/ABI:** issued `Handle` is not an arbitrary externally copied raw `cJSON*`. This does not establish compatible C ABI or equal acceptability to existing downstream consumers.
- **Private metadata and proof:** pool identity, slot currentness, original address, generation/ABA and exclusive access must remain mutually consistent with actual linked `Box<Node>` storage; the crate's unsafe cursor contract still needs a sound private invariant.
- **Storage and insertion:** `Vec<Slot>` **never reuses slots** in this frozen design, retaining **O(total historical appends)** metadata rather than O(current members). Append is **amortized O(1)** but `Vec` reallocation may be O(total slots) and allocation can fail. The O(1) claim belongs to `detach_known`, not every operation.
- **Not tested/established:** sustained churn and bounded-slot recycling, fallible append/OOM/refund, unwinding under every allocation failure, reentrant callbacks, concurrency, FFI and raw-pointer ABI parity. These are engineering work, not a proof that Rust is infeasible.

Thus **stable Rust can preserve physical original heap nodes and offer a validated O(1) safe-handle removal** for this contract. A good C raw-pointer API may avoid the registry, while placing more current-member/lifetime proof on its caller. Which tradeoff a real consumer prefers requires concrete ABI, memory/churn, maintenance and safety requirements. A modern Rust safe facade has meaningful protection; it is neither costless nor impossible.

## 7. Where NewLang actually stands (not the result of this test)

NewLang's historical executed L1 capability is a **different, limited problem**: three **original lexical (local, not allocated heap) Nodes** and two scoped `Option<ptr<Node>> next` field changes in a genuinely registered source→semantic CHECK→authentic public checked-owned→strict C17/native profile ([P572 original](https://github.com/wakairo/NewLang_Compiler/blob/6654ce474d236cdd17cfdbbc1eee72fd5248f2da/tests/p572/three_originals.nl), [V607 honest packet](https://github.com/wakairo/NewLang_Compiler/issues/607#issuecomment-6257583407)). [R588's calibrated limited independent verification](https://github.com/wakairo/NewLang_Compiler/issues/588#issuecomment-6242719977) exercised a **different independent three-node source** and cannot be relabeled an independent run of P572's exact file; earlier R580/R583 inconclusive reports remain.

That finite mechanism does **not** supply a fallible heap-original nonCopy owner, returned detached owner/refund, protected list membership, enforceable library representation hiding and reusable **ordinary-safe API**, full O(1) list detach/adoption, the 2020 cJSON API/ABI, or exhaustive failure/teardown/reentrancy behavior. Nothing executed here used NewLang on cJSON or showed it would have **prevented this cached-tail policy/invariant bug**. Nor is there a measured advantage over well-factored C in human author+ordinary caller UB-prevention audit and re-audit scope. The [Product Strategy §1.1 and §1.4](../NewLang_Product_Value_Strategy.md) and frozen H1/Product GO remain unaltered. This C/Rust case is useful without NewLang.

## 8. Transferable maintenance and review checklist

Use this on any intrusive structure with a shortcut or nonowning cache, regardless of implementation language.

1. **Draw the actual original storage graph.** Identify the heap allocations/physical identities and who owns each object. Distinguish a live node from a **live current member** and a **freed node**; a pointer's non-nullness proves neither.
2. **Name the special invariant.** If `head.prev` is a cached tail, write down `head.prev == last_reachable_node`, empty/singleton cases, reciprocal neighbor links, and who is allowed to mutate them. Do not read a head-special field as ordinary adjacency.
3. **State which operation is O(1).** Valid-known-member direct unlink vs indexed lookup, entire safe-handle validation vs only pointer splicing, append worst-case vs amortized, and any recursive teardown are different costs.
4. **Enumerate legal state transitions.** First/middle/last/singleton detach, successful append, insert/replace, raw-pointer vs indexed calls, object variants, retained-detached owner, delete/free and full parent teardown. Test transitions, not only list lengths.
5. **Trace writer → later reader → free.** For every cached pointer, find all writers and readers; ask whether deleting the original item can leave a reader holding a dangling address. Also test live-but-detached membership corruption separately from UAF.
6. **Specify ownership on success and failure.** Who frees an original after an add fails? Who gets a detached owner? Which `Option`/`Result` or C return code carries FULL/EMPTY/OOM? Do not turn an ordinary failure into an unchecked lifetime premise.
7. **Preserve ordinary caller boundaries.** A properly factored C library already hides `head.prev` from an index-based caller. In Rust, inspect what a safe handle/opaque API guarantees and what the internal `unsafe` code must prove. Avoid shifting hidden invariants to ordinary users in the comparison.
8. **Test original identities and negative inputs.** Assert original A/C pointers after delete+append, membership and cached tail after last/singleton deletion, stale and foreign handles for safe APIs, and ASan/UBSan diagnostics on pinned code. Record true negative failures versus compile refusals or harness errors.
9. **Audit teardown, errors and callbacks only where relevant.** Include ownership transfer/refund, actual free sites, fallible allocation, reentrancy/FFI and concurrency if the library promises them; mark untested contracts plainly rather than assuming one ASan pass proves them.
10. **Compare maintainability under an equal contract.** Measure the author+ordinary-safe-caller review dependency closure after a representative legal link change and after an unrelated scalar edit; account for Rust handle registry storage/ABI and C's existing API encapsulation. Do not infer value from `unsafe`/unchecked counts or from missing market data.

## Source and editorial status

**Primary historical C:** [2020 upstream commit](https://github.com/DaveGamble/cJSON/commit/a65abf2f4f1842765761a616e93c14ae25a284fd), pinned [old](https://github.com/DaveGamble/cJSON/blob/3999b12848214a201bf56ef416b9dac1ec63f811/cJSON.c) and [fixed](https://github.com/DaveGamble/cJSON/blob/a65abf2f4f1842765761a616e93c14ae25a284fd/cJSON.c) `cJSON.c`, [struct/API header](https://github.com/DaveGamble/cJSON/blob/3999b12848214a201bf56ef416b9dac1ec63f811/cJSON.h), [ownership README](https://github.com/DaveGamble/cJSON/blob/3999b12848214a201bf56ef416b9dac1ec63f811/README.md), [MIT license](https://github.com/DaveGamble/cJSON/blob/3999b12848214a201bf56ef416b9dac1ec63f811/LICENSE). **Real upstream native:** [C617 report](https://github.com/wakairo/NewLang_Compiler/issues/617#issuecomment-6260349673), [frozen workflow](https://github.com/wakairo/NewLang_Compiler/blob/e3017a1e0bb44cf0258b4061c8bec2e1d2213ba0/.github/workflows/c617-upstream-cjson-asan.yml), [full Actions log](https://github.com/wakairo/NewLang_Compiler/actions/runs/39204409642/job/117904005205). **Separate evidence:** [V526 source only](https://github.com/wakairo/NewLang_Compiler/issues/526#issuecomment-6214336649), [V615 model/native](https://github.com/wakairo/NewLang_Compiler/issues/615#issuecomment-6259941685), [V616 no-upstream-run](https://github.com/wakairo/NewLang_Compiler/issues/616#issuecomment-6260201481), [V567 Rust original-heap native](https://github.com/wakairo/NewLang_Compiler/issues/567#issuecomment-6233871201), [V607 NewLang lexical L1](https://github.com/wakairo/NewLang_Compiler/issues/607#issuecomment-6257583407).

**Copyright/attribution:** cJSON is © 2009–2017 Dave Gamble and cJSON contributors under the MIT license in the pinned original. This article reproduces **one short upstream source statement**, cites the exact copyrighted original source, and supplies its own explanatory prose/API test listing. Anyone copying or redistributing substantial original upstream code (including the reproducibility downloads) must preserve the [full original copyright and permission notice](https://github.com/DaveGamble/cJSON/blob/3999b12848214a201bf56ef416b9dac1ec63f811/LICENSE). The Rust sample is linked as independent experiment evidence, not reproduced as an upstream cJSON implementation.

**Review status:** **INTERNAL DRAFT / READY FOR HUMAN C/RUST TECHNICAL EDITORIAL REVIEW, NOT APPROVED FOR PUBLICATION.** The historic upstream ASan authenticity gap is now filled, but a human should still check API documentation and failure-mode qualifications against these **pinned 2020** sources, verify language and citation clarity, run any desired additional membership-only/OOM/teardown examples under separate authorization, and review the receiving venue's copyright/AI-submission rules. **No external post, mail, questionnaire, HN submission, release date or solicitation is authorized.** This article neither presumes market demand nor changes any NewLang Product GO gate.
