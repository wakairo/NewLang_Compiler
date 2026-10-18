/* R601 independent NORMAL BACKEND validation gate against a damaged checked op. */
#define main r601_internal_driver_main
#include "src/main.c"
#undef main
#include "src/syntax_internal.h"
#include "tests/support/node_checked.h"
#include <stdio.h>
static bool driver(const TestNode *n) {
    V0Program p={.unit=nl_semantic_unit_type(n->context),
                 .u8=nl_semantic_core_type(n->context,NL_TYPE_U8)};
    return v0_validate_call(n->entry,nl_checked_root(n->entry),&p);
}
static bool gate(const char *path) {
    TestNode n={0};
    CHECK(node_checked_load(path,&n));
    CHECK(driver(&n));
    NLCheckedFragment *f=(NLCheckedFragment *)nl_checked_call_body(n.entry,nl_checked_root(n.entry));
    CHECK(f);
    NLCheckedNodeId id=0;
    for(size_t i=1;i<=nl_checked_node_count(f);++i)
        if(nl_checked_node_view(f,i)->kind==NL_CHECKED_USIZE_SUBTRACTION) id=i;
    CHECK(id!=0);
    NLCheckedNodeView *op=&f->nodes[id-1];
    NLCheckedNodeView previous=*op;
    op->kind=NL_CHECKED_USIZE_ADDITION;
    const bool accepted=driver(&n);
    *op=previous;
    CHECK(!accepted && driver(&n));
    puts("NORMAL BACKEND VALIDATOR: authentic source PASS, wrong-op checked node REJECT, restored PASS");
    node_checked_destroy(&n);
    return true;
}
int main(int argc,char **argv) {return argc==2 && gate(argv[1])?0:1;}
