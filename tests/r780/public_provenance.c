/* Track R #780. Independent read-only source-issued proof and foreign-world gate.
 * All facts originate from checked NewLang source; no host-generated authority. */
#include "../support/node_checked.h"
#include "newlang/copy_body.h"
#include "newlang/checked_c_node.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef struct {
    size_t copies, producers, receivers, initialized, scopes;
    NLCheckedField fields[2];
    unsigned old[2], current[2], sibling[2];
    NLCheckedFragment const *copy_owner, *producer_owner;
    NLCheckedNodeId copy_id, producer_id;
    NLCopyBodyView copy_view;
    NLCopyBodyParameters parameters;
    NLProducerLocalReturnView local;
} Report;
static bool inspect(const NLCheckedFragment *f, Report *report, bool expect_b)
{
    for (size_t id=1;id<=nl_checked_node_count(f);id++) {
        const NLCheckedNodeView *node=nl_checked_node_view(f,id);
        NLCopyBodyView v={0};
        NLCopyBodyParameters p={0};
        if (nl_checked_copy_body_view(f,id,&v)) {
            CHECK(nl_checked_copy_body_parameters(f,id,&p));
            CHECK(report->copies<2);
            NLCheckStatus vs=nl_checked_copy_body_validate(f,id,&v);
            NLCheckStatus ps=nl_checked_copy_body_parameters_validate(f,id,&p);
            if(expect_b) CHECK((int)vs==3 && (int)ps==3);
            else CHECK(vs==NL_CHECK_OK && ps==NL_CHECK_OK);
            NLSemanticPlaceView target={0},origin={0},other={0};
            NLSemanticValueView prior={0},next={0},sibling={0};
            CHECK(nl_semantic_place_view(p.handoff,p.field.child,&target));
            CHECK(nl_semantic_place_view(p.handoff,p.field.parent,&origin));
            CHECK(p.field.index<2);
            CHECK(nl_semantic_place_view(p.handoff,
                origin.fixed_fields[1-p.field.index],&other));
            CHECK(nl_semantic_value_view(v.live_exit,v.result.value,&prior));
            CHECK(nl_semantic_value_view(p.handoff,target.current_value,&next));
            CHECK(nl_semantic_value_view(p.handoff,other.current_value,&sibling));
            size_t i=report->copies++;
            report->fields[i]=p.field;
            report->old[i]=(unsigned)prior.scalar_value;
            report->current[i]=(unsigned)next.scalar_value;
            report->sibling[i]=(unsigned)sibling.scalar_value;
            if(!report->copy_owner){
                report->copy_owner=f;report->copy_id=id;
                report->copy_view=v;report->parameters=p;
            }
            printf("R780_PUBLIC Copy=%zu index=%zu old=%u new=%u sibling=%u "
                   "body=%d parameters=%d source_issued=1\n",
                   i,p.field.index,report->old[i],report->current[i],
                   report->sibling[i],(int)vs,(int)ps);
        }
        if(node->kind==NL_CHECKED_REF_FROM_PTR && node->has_reference_result)
            report->scopes++;
        if(node->kind==NL_CHECKED_INITIALIZE) {
            CHECK(node->lifetime_place && node->lifetime_incarnation &&
                  node->lifetime_range.region && node->lifetime_domain);
            report->initialized++;
            printf("R780_PUBLIC original=%zu place=%zu incarnation=%zu backing=%zu domain=%zu\n",
                  report->initialized,node->lifetime_place,node->lifetime_incarnation,
                  node->lifetime_range.region,node->lifetime_domain);
        }
        if(node->producer.entry_proved) {
            NLProducerLocalReturnView local={0};
            CHECK(nl_checked_producer_valid(f,id) &&
                  nl_checked_producer_local_return_view(f,id,&local) &&
                  nl_checked_producer_local_return_valid(f,id,&local));
            CHECK(local.entry!=local.returned && local.body &&
                  nl_checked_context(local.body)==nl_checked_context(f));
            NLSemanticBindingView binding={0};
            NLSemanticValueView packet={0},ptr={0},allocation={0},domain={0};
            CHECK(nl_semantic_binding_view(local.returned,local.local,&binding));
            CHECK(nl_semantic_value_view(local.returned,local.result.value,&packet));
            CHECK(packet.field_count==3);
            CHECK(nl_semantic_value_view(local.returned,packet.fields[0],&ptr));
            CHECK(nl_semantic_value_view(local.returned,packet.fields[1],&allocation));
            CHECK(nl_semantic_value_view(local.returned,packet.fields[2],&domain));
            CHECK(ptr.reference.place==node->producer.root &&
                  allocation.allocation_region==node->producer.range.region &&
                  domain.domain==node->producer.domain);
            report->producer_owner=f;report->producer_id=id;report->local=local;
            report->producers++;
            printf("R780_PUBLIC H producer place=%zu incarnation=%zu backing=%zu "
                   "domain=%zu local=%zu returned_valid=1 physical_origins_exact=1\n",
                   node->producer.root,node->producer.incarnation,
                   node->producer.range.region,node->producer.domain,local.local);
        }
        if(node->owner_call.entry_proved) {
            const NLSemanticContext *entry=nl_checked_owner_entry(f,id);
            NLSemanticPlaceView place={0};
            NLSemanticValueView a={0},d={0};
            CHECK(entry && nl_semantic_place_view(entry,node->owner_call.root,&place) &&
                  nl_semantic_value_view(entry,node->owner_call.inputs[1],&a) &&
                  nl_semantic_value_view(entry,node->owner_call.inputs[2],&d));
            CHECK(node->owner_call.post_proved &&
                  a.allocation_region==place.placement.region &&
                  d.domain==place.governing_domain);
            report->receivers++;
        }
        if(node->kind==NL_CHECKED_MATCH)
            for(size_t a=0;a<node->item_count;a++) {
                const NLCheckedFragment *arm=nl_checked_match_arm(f,id,a);
                CHECK(arm && inspect(arm,report,expect_b));
            }
        const NLCheckedFragment *callee=nl_checked_call_body(f,id);
        if(callee)CHECK(inspect(callee,report,expect_b));
    }
    return true;
}
static bool one(const char *path, int kind)
{
    TestNode n={0};
    CHECK(node_checked_load(path,&n));
    Report report={0};
    CHECK(inspect(n.entry,&report,kind==2));
    CHECK(report.producers==1 && report.receivers==1 &&
          report.initialized==2 && report.copies==1 && report.scopes>=2);
    if(kind==0)CHECK(report.fields[0].index==0 &&
       report.old[0]==41 && report.current[0]==96 &&
       report.sibling[0]==149);
    if(kind==1)CHECK(report.fields[0].index==1 &&
       report.old[0]==27 && report.current[0]==132 &&
       report.sibling[0]==185);
    if(kind==2)CHECK(report.fields[0].index==1 &&
       report.old[0]==184 && report.current[0]==27 &&
       report.sibling[0]==7);
    char *out=NULL;
    size_t size=279;
    NLNodeCStatus status=nl_checked_c_node(n.entry,&out,&size);
    if(kind==2)CHECK(status==NL_NODE_C_UNSUPPORTED && out==NULL && size==279);
    else CHECK(status==NL_NODE_C_OK && out && size>500);
    free(out);
    printf("R780_PUBLIC path=%s type=%d copies=%zu H_producers=%zu "
           "receivers=%zu original_inits=%zu scopes=%zu C_status=%d\n",
           path,kind,report.copies,report.producers,report.receivers,
           report.initialized,report.scopes,(int)status);
    if(kind==0) {
        TestNode other={0};
        CHECK(node_checked_load(path,&other));
        Report foreign={0};
        CHECK(inspect(other.entry,&foreign,false));
        CHECK(report.copy_id==foreign.copy_id &&
           report.parameters.entry!=foreign.parameters.entry &&
           report.copy_view.body!=foreign.copy_view.body);
        CHECK(nl_checked_copy_body_validate(report.copy_owner,report.copy_id,
                    &foreign.copy_view)!=NL_CHECK_OK &&
           nl_checked_copy_body_parameters_validate(report.copy_owner,
                    report.copy_id,&foreign.parameters)!=NL_CHECK_OK);
        CHECK(report.producer_id==foreign.producer_id &&
              !nl_checked_producer_local_return_valid(
                 report.producer_owner,report.producer_id,&foreign.local));
        printf("R780_PUBLIC same-numeric foreign-world Copy/H REJECTED\n");
        node_checked_destroy(&other);
    }
    node_checked_destroy(&n);
    return true;
}
int main(int argc,char **argv)
{
    if(argc!=4)return 2;
    return one(argv[1],0) && one(argv[2],1) && one(argv[3],2) ? 0 : 1;
}
