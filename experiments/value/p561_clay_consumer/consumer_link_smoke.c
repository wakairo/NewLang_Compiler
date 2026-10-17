/* V561 unchanged C17-facing consumer smoke of a VENDOR-built Rust staticlib.
 * No Rust compiler, Cargo, or Rust source needed during the final C-only link. */
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
/* abi: C unsigned 32-bit inputs, uint64_t output; no allocations/ownership */
extern uint64_t p561_rectangle_area(uint32_t width, uint32_t height);
int main(void) {
    uint64_t got = p561_rectangle_area(64u, 32u);
    if (got != UINT64_C(2048)) return 2;
    printf("P561_RUST_STATICLIB_C_ABI_OK area=%" PRIu64 "\n", got);
    return 0;
}
