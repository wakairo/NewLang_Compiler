#include "observer.h"
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdalign.h>

int stb264_check(const char *label, const struct Rec *items, const void *header,
                 size_t header_bytes, size_t count, size_t capacity,
                 const uint32_t *expected_ids, size_t expected_len,
                 size_t expected_capacity)
{
    if (count != expected_len || capacity != expected_capacity ||
        count > capacity || (items == NULL && count != 0)) {
        fprintf(stderr,"OBS_REJECT|%s|BAD_LENGTH_OR_CAP|len=%zu|cap=%zu|expected=%zu/%zu\n",
                label,count,capacity,expected_len,expected_capacity);
        return 0;
    }
    if (items != NULL) {
        uintptr_t base = (uintptr_t)header, item = (uintptr_t)items;
        if (base + header_bytes != item || item % _Alignof(struct Rec) != 0) {
            fprintf(stderr,"OBS_REJECT|%s|BAD_HEADER_OFFSET_OR_ALIGNMENT\n",label);
            return 0;
        }
    }
    for (size_t i=0; i<count; ++i) {
        uint32_t id=expected_ids[i], checksum=17u*id+3u;
        if (items[i].id != id || items[i].checksum != checksum) {
            fprintf(stderr,"OBS_REJECT|%s|BAD_RECORD|index=%zu|got=%" PRIu32 ":%" PRIu32
                    "|expected=%" PRIu32 ":%" PRIu32 "\n",
                    label,i,items[i].id,items[i].checksum,id,checksum);
            return 0;
        }
    }
    printf("OBS_PASS|%s|len=%zu|cap=%zu|checked=%zu\n",label,count,capacity,count);
    return 1;
}
