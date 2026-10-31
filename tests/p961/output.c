/* Test-only interposition of the genuine driver-owned artifact immediately
 * before the actual backend. No source or authority is created by the test. */
#define main p961_actual_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"

static TestNode foreign;
static bool attack;
NLNodeCStatus __real_nl_checked_c_node(const NLCheckedFragment *, char **,
                                       size_t *);
NLNodeCStatus __wrap_nl_checked_c_node(const NLCheckedFragment *entry,
                                       char **out, size_t *bytes)
{
    if (!attack)
        return __real_nl_checked_c_node(entry, out, bytes);
    NLCheckedFragment *main_body = (NLCheckedFragment *)nl_checked_call_body(
        entry, nl_checked_root(entry));
    NLCheckedFragment *other_main = foreign.entry->bodies[0];
    NLCheckedFragment *parent = main_body->arms[1].artifact;
    NLCheckedFragment *other_parent = other_main->arms[1].artifact;
    NLCheckedArm saved = parent->arms[1];
    parent->arms[1] = other_parent->arms[1];
    NLNodeCStatus status = __real_nl_checked_c_node(entry, out, bytes);
    parent->arms[1] = saved;
    return status;
}
int main(int argc, char **argv)
{
    if (argc != 4 || !node_checked_load(argv[1], &foreign))
        return 1;
    attack = strcmp(argv[3], "attack") == 0;
    char *actual[] = {argv[0], "--emit-c17-to", argv[2], argv[1]};
    int status = p961_actual_driver_main(4, actual);
    node_checked_destroy(&foreign);
    return status;
}
