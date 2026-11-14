/* P-owned source-first C17 admission/atomicity probe; no seeded pointers. */
#define main p1408_preserved_main
#include "../p1408/owned.c"
#undef main

static bool emit_fault, hit;
static size_t emit_allocations;
void *__real_malloc(size_t);
void *__wrap_malloc(size_t n)
{
    if (emit_fault) {
        ++emit_allocations;
        hit = true;
        return NULL;
    }
    return __real_malloc(n);
}
static bool checked(const char *path, Program *p)
{
    CHECK(load(path, p) && define(p, NULL) == NL_CHECK_OK);
    CHECK(nl_semantic_check_expression(p->context, p->call, &p->artifact,
                                        NULL) == NL_CHECK_OK);
    CHECK(nl_h_custody_result_valid(p->artifact, nl_checked_root(p->artifact)));
    return true;
}
static bool emitted(Program *p, char **out, size_t *length)
{
    NLNodeCStatus s = nl_checked_c_node(p->artifact, out, length);
    if (s != NL_NODE_C_OK)
        fprintf(stderr, "emit status=%d out=%s length=%zu\n", (int)s,
                *out ? "nonNULL" : "NULL", *length);
    CHECK(s == NL_NODE_C_OK && *out && *length &&
          strlen(*out) == *length);
    return true;
}
static bool run(const char *path, const char *output, bool legacy)
{
    Program p = {0};
    if (legacy) {
        /* Old native profiles require their own canonical source authority;
         * they do not claim the new same-original ptr IF result seal. */
        CHECK(load(path, &p) && define(&p, NULL) == NL_CHECK_OK);
        CHECK(nl_semantic_check_expression(p.context, p.call, &p.artifact,
                                            NULL) == NL_CHECK_OK);
        CHECK(nl_h_custody_validate(p.artifact) && nl_h_origin_validate(p.artifact));
    } else {
        CHECK(checked(path, &p));
    }
    char *text = NULL;
    size_t length = 98765;
    CHECK(emitted(&p, &text, &length));
    if (output) {
        FILE *f = fopen(output, "wx");
        CHECK(f);
        const bool complete = fwrite(text, 1, length, f) == length;
        CHECK(fclose(f) == 0 && complete);
    }
    printf("genuine source/call-seal/C17 bytes=%zu PASS\n", length);
    free(text);
    destroy(&p);
    return true;
}
static bool rejected(Program *p)
{
    char *text = NULL;
    size_t length = 98765;
    CHECK(nl_checked_c_node(p->artifact, &text, &length) ==
              NL_NODE_C_UNSUPPORTED &&
          !text && length == 98765);
    return true;
}
static bool faults(const char *path)
{
    Program p = {0};
    CHECK(checked(path, &p));
    const NLCheckedNodeId call = nl_checked_root(p.artifact);
    NLCheckedNodeView saved = p.artifact->nodes[call - 1];
    p.artifact->nodes[call - 1].type = 0;
    CHECK(rejected(&p));
    p.artifact->nodes[call - 1] = saved;
    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(p.artifact, call);
    const NLSemanticContext *world = body->context;
    body->context = NULL;
    CHECK(rejected(&p));
    body->context = world;
    NLCheckedNodeView small = {0};
    NLCheckedNodeView *nodes = p.artifact->nodes;
    p.artifact->nodes = &small;
    CHECK(rejected(&p));
    p.artifact->nodes = nodes;
    char sentinel = 'x', *out = &sentinel;
    size_t length = 98765;
    CHECK(nl_checked_c_node(p.artifact, &out, &length) ==
              NL_NODE_C_UNSUPPORTED && out == &sentinel && sentinel == 'x' &&
          length == 98765);
    char *text = NULL;
    CHECK(emitted(&p, &text, &length));
    free(text);
    destroy(&p);
    puts("damaged-call/world/short-array/nonempty-out rejected, no-clobber/restore PASS");
    return true;
}
static bool emit_oom(const char *path)
{
    Program p = {0};
    CHECK(checked(path, &p));
    NLSemanticSnapshot before, after;
    NLCopyCallAnchor anchor = {0};
    CHECK(nl_semantic_snapshot(p.context, &before) &&
          nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK);
    const NLCheckedNodeId root = nl_checked_root(p.artifact);
    char *out = NULL;
    size_t length = 98765;
    emit_fault = true;
    NLNodeCStatus s = nl_checked_c_node(p.artifact, &out, &length);
    emit_fault = false;
    CHECK(hit && emit_allocations == 1 && s == NL_NODE_C_OUT_OF_MEMORY &&
          !out && length == 98765 && nl_checked_root(p.artifact) == root &&
          nl_semantic_snapshot(p.context, &after) &&
          !memcmp(&before, &after, sizeof(before)) &&
          nl_copy_state_matches(p.context, &anchor) &&
          nl_h_custody_result_valid(p.artifact, root));
    CHECK(emitted(&p, &out, &length));
    nl_copy_state_destroy(&anchor);
    free(out);
    destroy(&p);
    puts("emitter output-buffer malloc OOM/no-clobber/rollback/retry PASS");
    return true;
}
int main(int argc, char **argv)
{
    if (argc < 3)
        return 2;
    const bool ok = !strcmp(argv[1], "emit") ? run(argv[2], argc == 4 ? argv[3] : NULL, false)
                    : !strcmp(argv[1], "compat") ? run(argv[2], argc == 4 ? argv[3] : NULL, true)
                    : !strcmp(argv[1], "faults") ? faults(argv[2])
                    : !strcmp(argv[1], "oom") ? emit_oom(argv[2])
                    : false;
    return ok ? 0 : 1;
}
