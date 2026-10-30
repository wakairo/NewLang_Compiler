#include "support.h"

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool recording, injecting, hit;
static size_t count, fail_at;
static struct {
    size_t bytes;
    char kind;
} records[8192];
static bool failure(size_t bytes, char kind)
{
    if (!recording && !injecting)
        return false;
    if (count >= sizeof(records) / sizeof(*records))
        abort();
    if (recording) {
        records[count].bytes = bytes;
        records[count].kind = kind;
    }
    const bool fail = injecting && count == fail_at;
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
    TestNode node = {0};
    P1058Call calls[2];
    CHECK(registry(path, &base) && checked(base, &node) &&
          p1058_calls(&node, calls) && p1058_valid(calls));
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(node.context, &before));
    char *expected = NULL;
    size_t bytes = 777;
    recording = true;
    NLNodeCStatus status = nl_checked_c_node(node.entry, &expected, &bytes);
    recording = false;
    CHECK(status == NL_NODE_C_OK && expected && bytes && count);
    const size_t positions = count;
    for (size_t i = 0; i < positions; ++i) {
        count = 0;
        fail_at = i;
        hit = false;
        char *text = NULL;
        size_t length = 777;
        injecting = true;
        status = nl_checked_c_node(node.entry, &text, &length);
        injecting = false;
        CHECK(hit && status != NL_NODE_C_OK && !text && length == 777 &&
              nl_semantic_snapshot(node.context, &after) &&
              !memcmp(&before, &after, sizeof(before)) &&
              nl_h_custody_validate(node.entry) &&
              nl_h_origin_validate(node.entry) && p1058_valid(calls) &&
              p1058_exact(&node, expected, bytes));
        printf(
            "P1058 OOM index=%zu kind=%c bytes=%zu status=%d "
            "unpublished=YES snapshot=YES original_owner=YES exact_retry=YES\n",
            i, records[i].kind, records[i].bytes, (int)status);
    }
    free(expected);
    node_checked_destroy(&node);
    nl_semantic_destroy(base);
    printf("P1058 OOM SUMMARY reached=%zu\n", positions);
    return true;
}
int main(int argc, char **argv)
{
    (void)node_backend;
    return argc == 2 && run(argv[1]) ? 0 : 1;
}
