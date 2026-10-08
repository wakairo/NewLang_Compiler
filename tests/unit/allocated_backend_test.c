#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdlib.h>
void *__real_malloc(size_t);
static bool fail_malloc;
void *__wrap_malloc(size_t n)
{
    return fail_malloc ? NULL : __real_malloc(n);
}
static NLCheckedNodeView *ref(const NLCheckedFragment *f)
{
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_REF_FROM_PTR)
            return (NLCheckedNodeView *)v;
        const NLCheckedFragment *b = nl_checked_call_body(f, i);
        NLCheckedNodeView *r = NULL;
        if (b != NULL && (r = ref(b)) != NULL)
            return r;
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < v->item_count; ++a) {
                b = nl_checked_match_arm(f, i, a);
                if (b != NULL && (r = ref(b)) != NULL)
                    return r;
            }
    }
    return NULL;
}
static bool run(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    char *c = NULL;
    size_t size = 0;
    CHECK(nl_checked_c_node(n.entry, &c, &size) == NL_NODE_C_OK && c != NULL &&
          size == strlen(c));
    CHECK(strstr(c, "malloc(24)") != NULL && strstr(c, "free(nl_v_") != NULL &&
          strstr(c, "_Static_assert(sizeof(nl_node)==24") != NULL);
    free(c);
    c = NULL;
    size = 777;
    fail_malloc = true;
    NLNodeCStatus status = nl_checked_c_node(n.entry, &c, &size);
    fail_malloc = false;
    CHECK(status == NL_NODE_C_OUT_OF_MEMORY && c == NULL && size == 777);
    NLCheckedNodeView *r = ref(n.entry);
    CHECK(r != NULL);
    size_t count = r->argument_count;
    r->argument_count = 0;
    CHECK(nl_checked_c_node(n.entry, &c, &size) == NL_NODE_C_UNSUPPORTED &&
          c == NULL && size == 777);
    r->argument_count = count;
    NLCheckedNodeId first = r->first_argument;
    r->first_argument = 0;
    CHECK(nl_checked_c_node(n.entry, &c, &size) == NL_NODE_C_UNSUPPORTED &&
          c == NULL && size == 777);
    r->first_argument = first;
    NLDomainId domain = r->lifetime_domain;
    r->lifetime_domain = 0;
    CHECK(nl_checked_c_node(n.entry, &c, &size) == NL_NODE_C_UNSUPPORTED &&
          c == NULL && size == 777);
    r->lifetime_domain = domain;
    CHECK(nl_checked_c_node(n.entry, &c, &size) == NL_NODE_C_OK);
    free(c);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && run(argv[1]) ? 0 : 1;
}
