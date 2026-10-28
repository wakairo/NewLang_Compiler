#include "../p974/support.h"
#include <stdio.h>

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool recording, injecting, hit;
static size_t count, fail_at;
static struct {
    size_t bytes;
    char kind;
} records[4096];
static bool failure(size_t bytes, char kind)
{
    if (!recording && !injecting)
        return false;
    CHECK(count < sizeof(records) / sizeof(*records));
    if (recording) {
        records[count].bytes = bytes;
        records[count].kind = kind;
    }
    bool fail = injecting && count == fail_at;
    ++count;
    hit |= fail;
    return fail;
}
void *__wrap_malloc(size_t bytes)
{
    return failure(bytes, 'm') ? NULL : __real_malloc(bytes);
}
void *__wrap_calloc(size_t n, size_t bytes)
{
    return failure(n * bytes, 'c') ? NULL : __real_calloc(n, bytes);
}
void *__wrap_realloc(void *old, size_t bytes)
{
    return failure(bytes, 'r') ? NULL : __real_realloc(old, bytes);
}
static bool run(const char *path)
{
    NLSemanticContext *base = NULL;
    TestNode a = {0};
    P974Observation o;
    CHECK(registry(path, &base) && checked(base, &a) &&
          p974_find(a.entry, false, &o));
    CHECK(p974_get(&o, false));
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(a.context, &before));
    char *expected = NULL;
    size_t length = 0;
    recording = true;
    NLNodeCStatus status = nl_checked_c_node(a.entry, &expected, &length);
    recording = false;
    CHECK(status == NL_NODE_C_OK && expected && length && count);
    const size_t positions = count;
    for (size_t i = 0; i < positions; ++i) {
        count = 0;
        fail_at = i;
        hit = false;
        char *out = NULL;
        size_t bytes = 777;
        injecting = true;
        status = nl_checked_c_node(a.entry, &out, &bytes);
        injecting = false;
        CHECK(hit && status != NL_NODE_C_OK && !out && bytes == 777);
        CHECK(nl_semantic_snapshot(a.context, &after) &&
              !memcmp(&before, &after, sizeof(before)));
        CHECK(nl_h_origin_validate(a.entry) && p974_validate(&o, false, true));
        CHECK(emit(a.entry, true, expected, length));
        printf("P982 reached_C17_OOM index=%zu kind=%c bytes=%zu status=%d "
               "unpublished=YES retry=YES\n",
               i, records[i].kind, records[i].bytes, (int)status);
    }
    node_checked_destroy(&a);
    nl_semantic_destroy(base);
    free(expected);
    printf("P982 new_emitter_reached_OOM=%zu PASS\n", positions);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && run(argv[1]) ? 0 : 1;
}
