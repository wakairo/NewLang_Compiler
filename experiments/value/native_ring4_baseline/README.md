# Track V #536 — V-only same-topology Ring4 comparator

**DRAFT experiment; not NewLang compiler/source evidence. Never merge into main without Coordination.**

Four fixed physical tagged slot envelopes (`Option<OwnedFrame>` or C equivalent); actual heap payload pointer identity preserved while moving non-Copy owning handles. No VecDeque/index-only/GC rewrite, no external slot pointer or in-place pinning contract, no allocator/FFI/concurrency.

## Rust — safe source, compilation NOT MEASURED

```sh
rustc --version
rustc --edition=2021 -D warnings --test ring4.rs -o ring4_tests
./ring4_tests
rustc --edition=2021 -D warnings ring4.rs -o ring4
./ring4
```

`ring4.rs` uses a separate `tagged_ring` module with **private** `slots`, `head`, `len`; `std::array::from_fn(|_| None)`, `Option::take()`, `Box<[u8;16]>`, `Drop`, no `unsafe`, and no Clone/Copy on OwnedFrame. Tests cover A/B/C/D; FULL returns unchanged E owner; pop A/B; push E/F into wrapped slots; pop C/D/E/F; EMPTY; explicitly drop six originals; nonempty finish refunds the Ring; early Rust ring drop RAII releases retained frame; internal test checks physical Option cell spacing. A returned frame's numeric address is observed while its Box remains live; no freed pointer is dereferenced.

## C17 — compiled and executed in restricted host

```sh
gcc -std=c17 -O0 -g -Wall -Wextra -Werror -pedantic ring4_c.c ring4_c_test.c -o ring4_c_test
./ring4_c_test
gcc -std=c17 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=undefined -fno-sanitize-recover=all ring4_c.c ring4_c_test.c -o ring4_c_ubsan
./ring4_c_ubsan
gcc -std=c17 -O0 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all ring4_c.c ring4_c_test.c -o ring4_c_sanitized
ASAN_OPTIONS=detect_leaks=1 ./ring4_c_sanitized
```

Local record 2026-10-16 JST, Linux x86_64, GCC Debian **14.2.0-19**: all three invocations exited 0 with `C17_RING4_SCENARIO_OK released=6 original_heap_addresses_unchanged`, no ASan/UBSan diagnostics observed. Test assertions use default enabled asserts (no NDEBUG). This is a fixed scenario result, not whole-program UB freedom.

**Toolchain limitation:** Local environment had no `rustc`, `cargo`, or `zig`, and DNS to Rust distribution host failed; Rust/Zig actual compilation and runtime are explicitly **UNMEASURED**. Do not report compiler PASS from source inspection or from the repo's generic CI. Rust code can be run in any independent environment with rustc.

Source SHA256 for the exact local comparator (confirm remote text before claiming identical byte hash):

- `ring4.rs`: `dfd5f9f1c479c900fe713e1b83435cb007e90c392e5969aceb817210b53bc86a`
- `ring4_c.h`: `fe1e9260dd0a79de91184859842da25f110f2c8fbaa64ca726e57b76fef6dfd2`
- `ring4_c.c`: `cc74be101bdeea794e61de4f57f9fab72de6b5926b8c7bf79102cdf6ec13d96d`
- `ring4_c_test.c`: `a27c7396f7b691e64a5a008920d3168919f09478565be0517ad9f46c05a4dd2c`

## Zig same physical comparator (source/contract only, Zig uncompiled)

Use `slots: [4]?OwnedFrame`, `head`/`len`, `?Owner` read and explicit move-out/null transitions, one original owned heap payload for each A..F, result carrier returning the **original** input E when FULL. A Zig `?Owner`/error union plus runtime safety checks do not by themselves enforce exclusive/affine alias lifetime; the original owner must be settled once by application convention. A private API can already hide all indexing from ordinary callers, as in C and Rust. Zig compiler/build-mode-dependent bounds diagnostics were not measured.

## Human audit / re-audit edits

- RELATED: change `(head+len)%4` to a separately maintained tail cursor. This is legal safe Rust and must be assessed for FIFO, metadata/Option correspondence, occupied-slot loss, ownership refunds, scope of `front`, and teardown. Rust guards against memory UB and invalid moves but not logical FIFO; C/Zig have additional manual owner-alias/double-release obligations. A deliberate wrong index can yield an EMPTY instead of the correct frame; this does NOT necessarily mean UB in safe Rust.
- UNRELATED: edit only `format_id(u8)`, a pure scalar formatter with no access to Ring/root/resources/callbacks. Its behavior oracle may change but it does not enter the Ring UB-precondition dependency closure; no author/caller memory-safety re-audit is warranted.

NewLang's accepted compiler main at fixed Issue #536 is `c6ae58512683090b2d98b09f51765143b1517345` / canonical Draft17.30. PR533 Draft17.32 is a separate **UNMERGED** source proposal; no `Array<Option<OwnedFrame>,4>` NewLang SOURCE/CHECK/owned/C17/native claim. No CI workflow, compiler test, canonical or other Track code is modified by this V-only experiment.
