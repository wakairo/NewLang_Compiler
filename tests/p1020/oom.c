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
static bool observation(TestNode *node, P974Observation *out)
{
    CHECK(nl_h_custody_validate(node->entry) &&
          p974_find(node->entry, true, out) && p974_get(out, true) &&
          p974_validate(out, true, true));
    return true;
}
static bool phase(const char *path, bool admission)
{
    NLSemanticContext *base = NULL;
    TestNode node = {0};
    P974Observation o;
    CHECK(registry(path, &base) && checked(base, &node) &&
          observation(&node, &o));
    char *expected = NULL;
    size_t bytes = 0;
    CHECK(p1020_text(node.entry, &expected, &bytes) == NL_NODE_C_OK);
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(node.context, &before));
    V0Program program;
    count = 0;
    recording = true;
    int status =
        admission
            ? (int)p1020_prepare(node.entry, &program)
            : (int)nl_checked_copy_unit_validate(o.caller, o.call, &o.unit);
    recording = false;
    CHECK(status == 0 && count);
    const size_t positions = count;
    for (size_t i = 0; i < positions; ++i) {
        count = 0;
        fail_at = i;
        hit = false;
        injecting = true;
        status =
            admission
                ? (int)p1020_prepare(node.entry, &program)
                : (int)nl_checked_copy_unit_validate(o.caller, o.call, &o.unit);
        injecting = false;
        CHECK(hit && status != 0);
        CHECK(nl_semantic_snapshot(node.context, &after) &&
              !memcmp(&before, &after, sizeof(before)) &&
              observation(&node, &o) && p1020_exact(&node, expected, bytes));
        printf("P1020 OOM phase=%s index=%zu kind=%c bytes=%zu status=%d "
               "unpublished=YES snapshot=YES owner=YES exact_retry=YES\n",
               admission ? "C17-admission" : "UNIT-current", i, records[i].kind,
               records[i].bytes, status);
    }
    free(expected);
    node_checked_destroy(&node);
    nl_semantic_destroy(base);
    printf("P1020 OOM SUMMARY phase=%s reached=%zu\n",
           admission ? "C17-admission" : "UNIT-current", positions);
    return true;
}
static bool check_phase(const char *path)
{
    NLSemanticContext *base = NULL;
    TestNode healthy = {0};
    P974Observation o;
    CHECK(registry(path, &base) && checked(base, &healthy) &&
          observation(&healthy, &o));
    char *expected = NULL;
    size_t bytes = 0;
    CHECK(p1020_text(healthy.entry, &expected, &bytes) == NL_NODE_C_OK);
    node_checked_destroy(&healthy);
    TestNode node = {0};
    NLSyntaxTree *tree = NULL;
    CHECK(nl_sem_clone(base, &node.context) == NL_CHECK_OK &&
          entry_tree(&node, &tree));
    count = 0;
    recording = true;
    NLCheckStatus status =
        nl_semantic_check_expression(node.context, tree, &node.entry, NULL);
    recording = false;
    CHECK(status == NL_CHECK_OK && node.entry && count &&
          observation(&node, &o) && p1020_exact(&node, expected, bytes));
    const size_t positions = count;
    nl_syntax_tree_destroy(tree);
    node_checked_destroy(&node);
    for (size_t i = 0; i < positions; ++i) {
        tree = NULL;
        CHECK(nl_sem_clone(base, &node.context) == NL_CHECK_OK &&
              entry_tree(&node, &tree));
        NLSemanticSnapshot before, after;
        CHECK(nl_semantic_snapshot(node.context, &before));
        count = 0;
        fail_at = i;
        hit = false;
        injecting = true;
        status =
            nl_semantic_check_expression(node.context, tree, &node.entry, NULL);
        injecting = false;
        CHECK(hit && status != NL_CHECK_OK && !node.entry &&
              nl_semantic_snapshot(node.context, &after) &&
              !memcmp(&before, &after, sizeof(before)));
        CHECK(nl_semantic_check_expression(node.context, tree, &node.entry,
                                           NULL) == NL_CHECK_OK &&
              observation(&node, &o) && p1020_exact(&node, expected, bytes));
        printf("P1020 OOM phase=CHECK index=%zu kind=%c bytes=%zu status=%d "
               "unpublished=YES snapshot=YES owner=YES exact_retry=YES\n",
               i, records[i].kind, records[i].bytes, (int)status);
        nl_syntax_tree_destroy(tree);
        node_checked_destroy(&node);
    }
    free(expected);
    nl_semantic_destroy(base);
    printf("P1020 OOM SUMMARY phase=CHECK reached=%zu\n", positions);
    return true;
}
int main(int argc, char **argv)
{
    (void)node_backend;
    return argc == 2 && phase(argv[1], false) && phase(argv[1], true) &&
                   check_phase(argv[1])
               ? 0
               : 1;
}
