#ifndef STB264_HOOKS_H
#define STB264_HOOKS_H
#include <stddef.h>
#include <stdint.h>
void *stb264_realloc(void *p, size_t size);
void stb264_free(void *p);
int stb264_probe_bad_base(const void *p);
int stb264_probe_double_by_id(unsigned id);
int stb264_probe_missing_release(void);
int stb264_probe_bad_growth_count(size_t bogus);
void stb264_assert_final(void);
unsigned stb264_final_free_id(void);
#endif
