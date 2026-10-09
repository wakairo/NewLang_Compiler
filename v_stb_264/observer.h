#ifndef STB264_OBSERVER_H
#define STB264_OBSERVER_H
#include <stdint.h>
#include <stddef.h>
struct Rec { uint32_t id; uint32_t checksum; };
int stb264_check(const char *label, const struct Rec *items, const void *header,
                 size_t header_bytes, size_t count, size_t capacity,
                 const uint32_t *expected_ids, size_t expected_len,
                 size_t expected_capacity);
#endif
