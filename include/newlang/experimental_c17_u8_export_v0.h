#ifndef NEWLANG_EXPERIMENTAL_C17_U8_EXPORT_V0_H
#define NEWLANG_EXPERIMENTAL_C17_U8_EXPORT_V0_H

/* Experimental target ABI v0, not a NewLang source FFI or authority API.
 * Both separately compiled TUs must include this header and use compatible
 * C17 compiler/target ABIs. One producer per linked program; duplicate
 * producers collide at link time. No cross-target/version ABI promise.
 * A foreign TU bypassing this prototype retains C-side UB responsibility. */
#include <limits.h>
#include <stdint.h>

_Static_assert(CHAR_BIT == 8, "NewLang C17 requires 8-bit bytes");
_Static_assert(UINT8_MAX == 255 && sizeof(uint8_t) == 1 && ((uint8_t)-1) == 255,
               "NewLang C17 requires exact 8-bit uint8_t");

uint8_t newlang_c17_u8_export_v0(void);

#endif
