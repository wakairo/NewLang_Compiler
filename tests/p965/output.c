#define main p965_driver_main
#include "../../src/main.c"
#undef main
#include "shared.h"
static TestNode foreign;
static bool attack;
static NLCheckedFragment *if_parent(NLCheckedFragment *f)
{
    for (size_t i = 0; i < f->arm_count; ++i)
        if (f->nodes[f->arms[i].match - 1].kind == NL_CHECKED_IF)
            return f;
    for (size_t i = 0; i < f->arm_count; ++i) {
        NLCheckedFragment *p = if_parent(f->arms[i].artifact);
        if (p)
            return p;
    }
    for (size_t i = 0; i < f->body_count; ++i) {
        NLCheckedFragment *p = if_parent(f->bodies[i]);
        if (p)
            return p;
    }
    return NULL;
}
NLNodeCStatus __real_nl_checked_c_node(const NLCheckedFragment *, char **,
                                       size_t *);
NLNodeCStatus __wrap_nl_checked_c_node(const NLCheckedFragment *f, char **out,
                                       size_t *bytes)
{
    if (attack) {
        NLCheckedFragment *a = if_parent((NLCheckedFragment *)f),
                          *b = if_parent(foreign.entry);
        if (!a || !b)
            return NL_NODE_C_UNSUPPORTED;
        a->arms[0] = b->arms[0];
    }
    return __real_nl_checked_c_node(f, out, bytes);
}
int main(int argc, char **argv)
{
    if (argc != 4 || !node_checked_load(argv[1], &foreign))
        return 1;
    attack = strcmp(argv[3], "attack") == 0;
    char *actual[] = {argv[0], "--emit-c17-to", argv[2], argv[1]};
    int result = p965_driver_main(4, actual);
    attack = false;
    bool live = emit(foreign.entry, true, NULL, 0);
    node_checked_destroy(&foreign);
    return live ? result : 1;
}
