/* R601 phase2 test-private corruption of the genuine source-issued event. */
#include "src/usize_addition.c"
#include "tests/support/node_checked.h"
#include <stdio.h>
static bool attack(const char *a,const char *b,NLUsizeOperation op) {
    TestNode n={0},foreign={0};
    CHECK(node_checked_load(a,&n) && node_checked_load(b,&foreign));
    NLCheckedFragment *f=(NLCheckedFragment *)nl_checked_call_body(n.entry,nl_checked_root(n.entry));
    const NLCheckedFragment *other=nl_checked_call_body(foreign.entry,nl_checked_root(foreign.entry));
    CHECK(f && other && f!=other);
    NLCheckedNodeId id=0,other_id=0;
    const NLCheckedKind want=op==NL_USIZE_ADD?NL_CHECKED_USIZE_ADDITION:NL_CHECKED_USIZE_SUBTRACTION;
    for(size_t i=1;i<=nl_checked_node_count(f);++i)
        if(nl_checked_node_view(f,i)->kind==want) id=i;
    for(size_t i=1;i<=nl_checked_node_count(other);++i)
        if(nl_checked_node_view(other,i)->kind==want) other_id=i;
    CHECK(id && other_id);
    NLUsizeAdditionProof *p=(NLUsizeAdditionProof *)find(f,id);
    const NLUsizeAdditionProof *q=find(other,other_id);
    CHECK(p && q && p!=q && p->issued.origin!=q->issued.origin);
    CHECK(arithmetic_valid(f,id,op,NULL));
    const NLUsizeAdditionProof old=*p;
    int refused=0;
#define CORRUPT(code) do { \
        code; \
        CHECK(!arithmetic_valid(f,id,op,NULL)); \
        CHECK(!nl_checked_usize_addition_valid(f,id,NULL)); \
        CHECK(!nl_checked_usize_subtraction_valid(f,id,NULL)); \
        *p=old; \
        CHECK(arithmetic_valid(f,id,op,NULL)); \
        ++refused; \
    } while(0)
    CORRUPT(p->issued.origin=other);
    CORRUPT(p->operation=op==NL_USIZE_ADD?NL_USIZE_SUBTRACT:NL_USIZE_ADD);
    CORRUPT(p->issued.answer^=1);
    CORRUPT(p->issued.operator_span.start_byte+=1);
    CORRUPT(p->issued.operands[0]=p->issued.operands[1]);
    CORRUPT(p->issued.values[0]=p->issued.values[1]);
    CORRUPT(p->issued.numbers[0]^=1);
    CORRUPT(p->context=other->context);
    CORRUPT(p->issued.type=1);
#undef CORRUPT
    CHECK(refused==9);
    CHECK(op==NL_USIZE_ADD?nl_checked_usize_addition_valid(f,id,NULL):
                            nl_checked_usize_subtraction_valid(f,id,NULL));
    printf("PRIVATE op=%s faults=%d rejected restored=PASS world-mismatch=REJECT node-collision=%d\n",
           op==NL_USIZE_ADD?"+":"-",refused,(int)(id==other_id));
    node_checked_destroy(&foreign);node_checked_destroy(&n);
    return true;
}
int main(int argc,char **argv) {
    return argc==5 && attack(argv[1],argv[2],NL_USIZE_ADD) &&
           attack(argv[3],argv[4],NL_USIZE_SUBTRACT)?0:1;
}
