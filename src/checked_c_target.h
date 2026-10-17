#ifndef NEWLANG_CHECKED_C_TARGET_H
#define NEWLANG_CHECKED_C_TARGET_H

/* Generated C17 precondition, after <stdint.h>. sizeof alone counts C bytes,
 * not bits: an SDK's uint8_t spelling does not establish NewLang u8 width.
 * This rejects unsupported targets; it does not define a portable ABI. */
#define NL_C17_U8_TARGET_GUARD                                                 \
    "#include <limits.h>\n"                                                    \
    "_Static_assert(CHAR_BIT == 8, \"NewLang C17 requires 8-bit bytes\");\n"   \
    "_Static_assert(UINT8_MAX == 255 && sizeof(uint8_t) == 1 && "              \
    "((uint8_t)-1) == 255, "                                                   \
    "\"NewLang C17 requires exact 8-bit uint8_t\");\n"

#endif
