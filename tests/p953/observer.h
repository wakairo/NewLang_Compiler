/* Reuse the lifetime-safe observer, with an additional pre-producer check. */
#include "../p945/observer.h"
static void p953_bind_copy(size_t frame, size_t symbol, uint8_t *a, uint8_t *b)
{
    VERIFY(initialized == 2 && trials == 2 && !sent && !returned && !received &&
           !changes && !active_scope);
    bind_copy(frame, symbol, a, b);
}
#undef NL_MIXED_COPY_BIND
#define NL_MIXED_COPY_BIND(f, s, a, b) p953_bind_copy(f, s, a, b)
