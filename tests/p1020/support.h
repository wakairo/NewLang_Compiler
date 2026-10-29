#ifndef P1020_SUPPORT_H
#define P1020_SUPPORT_H
/* Compile the unchanged CLI consumer in this P-owned test translation unit.
 * No test entry/issuer is added to the Production executable or public API. */
#define main p1020_original_cli_main
#include "../../src/main.c"
#undef main
#include "../p974/support.h"

static inline NLNodeCStatus p1020_prepare(NLCheckedFragment *entry,
                                          V0Program *program)
{
    const NLSemanticContext *c = nl_checked_context(entry);
    const NLCheckedNodeId root = nl_checked_root(entry);
    const NLCheckedNodeView *node = nl_checked_node_view(entry, root);
    *program = (V0Program){.unit = nl_semantic_unit_type(c),
                           .u8 = nl_semantic_core_type(c, NL_TYPE_U8)};
    if (!node || node->type != program->unit || node->result_count ||
        !v0_validate_call(entry, root, program)) {
        if (program->copy_body_status == NL_CHECK_OUT_OF_MEMORY)
            return NL_NODE_C_OUT_OF_MEMORY;
        if (program->copy_body_status == NL_CHECK_RESOURCE_LIMIT)
            return NL_NODE_C_RESOURCE_LIMIT;
        return NL_NODE_C_UNSUPPORTED;
    }
    program->entry_function = node->function;
    return NL_NODE_C_OK;
}

static inline NLNodeCStatus p1020_text(NLCheckedFragment *entry, char **out,
                                       size_t *length)
{
    V0Program program;
    NLNodeCStatus status = p1020_prepare(entry, &program);
    if (status != NL_NODE_C_OK)
        return status;
    FILE *file = tmpfile();
    if (!file)
        return NL_NODE_C_RESOURCE_LIMIT;
    if (!v0_emit_c(file, &program) || fflush(file) ||
        fseek(file, 0, SEEK_END)) {
        fclose(file);
        return NL_NODE_C_RESOURCE_LIMIT;
    }
    const long size = ftell(file);
    if (size <= 0 || fseek(file, 0, SEEK_SET)) {
        fclose(file);
        return NL_NODE_C_RESOURCE_LIMIT;
    }
    char *text = malloc((size_t)size + 1);
    if (!text) {
        fclose(file);
        return NL_NODE_C_OUT_OF_MEMORY;
    }
    const bool copied = fread(text, 1, (size_t)size, file) == (size_t)size;
    const int closed = fclose(file);
    if (!copied || closed) {
        free(text);
        return NL_NODE_C_RESOURCE_LIMIT;
    }
    text[size] = 0;
    *out = text;
    *length = (size_t)size;
    return NL_NODE_C_OK;
}

static inline bool p1020_exact(TestNode *node, const char *expected,
                               size_t bytes)
{
    char *text = NULL;
    size_t length = 777;
    CHECK(p1020_text(node->entry, &text, &length) == NL_NODE_C_OK && text &&
          length == bytes && !memcmp(text, expected, bytes));
    free(text);
    return true;
}
#endif
