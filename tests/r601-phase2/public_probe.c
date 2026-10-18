/* R601-PHASE2 independent public proof: authentic function registration, not seeded fragments. */
#include "tests/support/node_checked.h"
#include "newlang/usize_addition.h"
#ifdef R601_CANDIDATE
#include "newlang/usize_subtraction.h"
#endif
#include <stdio.h>
#include <string.h>

static const NLCheckedFragment *body(TestNode *n) {
    return nl_checked_call_body(n->entry, nl_checked_root(n->entry));
}
static bool root_zero(const NLCheckedFragment *f, NLValueId result, size_t answer) {
    NLSemanticValueView v;
    CHECK(nl_semantic_value_view(nl_checked_context(f),result,&v));
    CHECK(v.scalar_known && v.scalar_value==answer && v.domain==0 &&
          v.reference.place==0 && v.reference.scope==0 &&
          v.reference.incarnation==0 && !v.reference_count &&
          !v.allocation_region && !v.slot_place && !v.value_dependency_count &&
          !v.field_count && !v.sum_payload && !v.sum_owner &&
          !v.aggregate_owner);
    return true;
}
static bool addition(const char *path, TestNode *n,
                     NLCheckedNodeId *id, NLUsizeAdditionView *view,
                     size_t l, size_t r, size_t answer) {
    CHECK(node_checked_load(path,n)); /* original source/parser/AST destroyed */
    const NLCheckedFragment *f=body(n);
    CHECK(f);
    size_t count=0;
    for(size_t i=1;i<=nl_checked_node_count(f);++i) {
        const NLCheckedNodeView *v=nl_checked_node_view(f,i);
        if(v && v->kind==NL_CHECKED_USIZE_ADDITION) {
            *id=i; ++count;
        }
    }
    CHECK(count==1 && nl_checked_usize_addition_view(f,*id,view));
    CHECK(view->origin==f && view->node==*id && view->numbers[0]==l &&
          view->numbers[1]==r && view->sum==answer);
    CHECK(nl_checked_usize_addition_valid(f,*id,view));
    CHECK(nl_checked_usize_addition_valid(f,*id,NULL));
    CHECK(view->values[0]!=view->values[1] &&
          view->result!=view->values[0] && view->result!=view->values[1]);
    CHECK(view->span.start_byte==view->operand_spans[0].start_byte &&
          view->span.end_byte==view->operand_spans[1].end_byte &&
          view->operand_spans[0].end_byte<=view->operator_span.start_byte &&
          view->operator_span.end_byte<=view->operand_spans[1].start_byte);
    CHECK(root_zero(f,view->result,answer));
    return true;
}
#ifdef R601_CANDIDATE
static bool subtraction(const char *path, TestNode *n,
                        NLCheckedNodeId *id, NLUsizeSubtractionView *view,
                        size_t l, size_t r, size_t answer) {
    CHECK(node_checked_load(path,n));
    const NLCheckedFragment *f=body(n);
    CHECK(f);
    size_t count=0;
    for(size_t i=1;i<=nl_checked_node_count(f);++i) {
        const NLCheckedNodeView *v=nl_checked_node_view(f,i);
        if(v && v->kind==NL_CHECKED_USIZE_SUBTRACTION) {
            *id=i; ++count;
        }
    }
    CHECK(count==1 && nl_checked_usize_subtraction_view(f,*id,view));
    CHECK(view->origin==f && view->node==*id && view->numbers[0]==l &&
          view->numbers[1]==r && view->difference==answer);
    CHECK(nl_checked_usize_subtraction_valid(f,*id,view));
    CHECK(nl_checked_usize_subtraction_valid(f,*id,NULL));
    CHECK(root_zero(f,view->result,answer));
    return true;
}
#endif
static bool run_all(int argc,char **argv) {
    if(argc==3 && strcmp(argv[1],"--preflight")==0) {
        TestNode n={0}; NLCheckedNodeId id=0; NLUsizeAdditionView proof;
        CHECK(addition(argv[2],&n,&id,&proof,37,59,96));
        node_checked_destroy(&n);
        puts("PREFLIGHT public registered plus view/validator=PASS");
        return true;
    }
#ifdef R601_CANDIDATE
    if(argc!=7) return false;
#else
    if(argc!=4) return false;
#endif
    TestNode a={0},b={0},z={0};
    NLCheckedNodeId ai=0,bi=0,zi=0;
    NLUsizeAdditionView av,bv,zv;
    CHECK(addition(argv[1],&a,&ai,&av,37,59,96));
    CHECK(addition(argv[2],&b,&bi,&bv,37,59,96));
    CHECK(addition(argv[3],&z,&zi,&zv,0,0,0));
    const NLCheckedFragment *af=body(&a),*bf=body(&b),*zf=body(&z);
    CHECK(af!=bf && av.origin!=bv.origin && av.sum==bv.sum);
    CHECK(!nl_checked_usize_addition_valid(af,ai,&bv));
    CHECK(!nl_checked_usize_addition_valid(bf,bi,&av));
    NLUsizeAdditionView tamper=av;
    tamper.origin=bf; CHECK(!nl_checked_usize_addition_valid(af,ai,&tamper));
    tamper=av; tamper.numbers[0]^=1;
    CHECK(!nl_checked_usize_addition_valid(af,ai,&tamper));
    tamper=av; tamper.facts[0]+=1;
    CHECK(!nl_checked_usize_addition_valid(af,ai,&tamper));
    tamper=av; tamper.operand_spans[0].start_byte+=1;
    CHECK(!nl_checked_usize_addition_valid(af,ai,&tamper));
    tamper=av; tamper.symbols[0]+=1;
    CHECK(!nl_checked_usize_addition_valid(af,ai,&tamper));
    tamper=av;
    CHECK(!nl_checked_usize_addition_view(af,0,&tamper));
    CHECK(memcmp(&tamper,&av,sizeof av)==0);
    CHECK(nl_checked_usize_addition_valid(af,ai,&av));
    CHECK(nl_checked_usize_addition_valid(bf,bi,&bv));
    CHECK(nl_checked_usize_addition_valid(zf,zi,&zv));
    printf("AUTH PLUS A/B: number=37,59 answer=96 origin-distinct=1 node-collision=%d result-id-collision=%d foreign-A2B/B2A=REJECT restored=PASS zero=PASS\n",
           (int)(ai==bi),(int)(av.result==bv.result));
#ifdef R601_CANDIDATE
    TestNode s={0},t={0},u={0};
    NLCheckedNodeId si=0,ti=0,ui=0;
    NLUsizeSubtractionView sv,tv,uv;
    CHECK(subtraction(argv[4],&s,&si,&sv,71,9,62));
    CHECK(subtraction(argv[5],&t,&ti,&tv,71,9,62));
    CHECK(subtraction(argv[6],&u,&ui,&uv,21,21,0));
    const NLCheckedFragment *sf=body(&s),*tf=body(&t),*uf=body(&u);
    CHECK(sf!=tf && sv.origin!=tv.origin);
    CHECK(!nl_checked_usize_subtraction_valid(sf,si,&tv));
    CHECK(!nl_checked_usize_subtraction_valid(tf,ti,&sv));
    NLUsizeSubtractionView changed=sv;
    changed.origin=tf; CHECK(!nl_checked_usize_subtraction_valid(sf,si,&changed));
    changed=sv; changed.numbers[1]=changed.numbers[0];
    CHECK(!nl_checked_usize_subtraction_valid(sf,si,&changed));
    changed=sv; changed.operator_span.start_byte+=1;
    CHECK(!nl_checked_usize_subtraction_valid(sf,si,&changed));
    CHECK(!nl_checked_usize_subtraction_valid(af,ai,NULL));
    CHECK(!nl_checked_usize_subtraction_valid(zf,zi,NULL));
    CHECK(!nl_checked_usize_addition_valid(sf,si,NULL));
    CHECK(!nl_checked_usize_addition_valid(uf,ui,NULL));
    NLUsizeAdditionView untouched=av;
    CHECK(!nl_checked_usize_addition_view(sf,si,&untouched));
    CHECK(memcmp(&untouched,&av,sizeof av)==0);
    NLUsizeSubtractionView sub_untouched=sv;
    CHECK(!nl_checked_usize_subtraction_view(af,ai,&sub_untouched));
    CHECK(memcmp(&sub_untouched,&sv,sizeof sv)==0);
    CHECK(nl_checked_usize_subtraction_valid(sf,si,&sv) &&
          nl_checked_usize_subtraction_valid(tf,ti,&tv) &&
          nl_checked_usize_subtraction_valid(uf,ui,&uv));
    CHECK(nl_checked_usize_addition_valid(zf,zi,&zv));
    printf("AUTH MINUS A/B: number=71,9 answer=62 origin-distinct=1 node-collision=%d result-id-collision=%d foreign-A2B/B2A=REJECT restored=PASS\n",
           (int)(si==ti),(int)(sv.result==tv.result));
    puts("CROSS OP including matching zero answer=0: wrong-op view and validator=REJECT, out sentinel=UNCHANGED");
    node_checked_destroy(&u);node_checked_destroy(&t);node_checked_destroy(&s);
#endif
    node_checked_destroy(&z);node_checked_destroy(&b);node_checked_destroy(&a);
    return true;
}
int main(int argc,char **argv) { return run_all(argc,argv) ? 0 : 1; }
