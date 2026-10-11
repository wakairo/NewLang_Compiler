/* R333 independently authored, public-API-only source-owned observer.
 * No fixtures, private header, injected semantic world or artificial certificate. */
#include "newlang/parser.h"
#include "newlang/captured_closure.h"
#include "newlang/raw_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(x) do { if (!(x)) { fprintf(stderr,"R333_ASSERT:%d:%s\n",__LINE__,#x); return false; } } while (0)

typedef struct {
    NLSource *src, *entry_src;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *unit, *entry_syntax;
    NLSemanticContext *ctx;
    NLCheckedFragment *entry;
} Session;

static void clear(Session *s) {
    nl_checked_destroy(s->entry);
    nl_semantic_destroy(s->ctx);
    nl_syntax_tree_destroy(s->entry_syntax);
    nl_syntax_tree_destroy(s->unit);
    nl_parser_destroy(s->entry_parser);
    nl_parser_destroy(s->parser);
    nl_source_destroy(s->entry_src);
    nl_source_destroy(s->src);
}
static bool load(Session *s, const char *file) {
    const NLSyntaxTree *items[1];
    NLFunctionUnitDiagnostic d = {0};
    NLParseDiagnostic pd = {0};
    REQUIRE(nl_source_load(file, &s->src) == NL_SOURCE_OK);
    REQUIRE(nl_parser_create(s->src, &s->parser) == NL_PARSE_OK);
    NLParseStatus ps=nl_parser_parse_function_unit(s->parser,&s->unit,&pd);
    if (ps != NL_PARSE_OK) nl_parse_diagnostic_render(stderr,s->src,&pd);
    REQUIRE(ps == NL_PARSE_OK);
    REQUIRE(nl_semantic_create(&s->ctx) == NL_CHECK_OK);
    items[0]=s->unit;
    NLCheckStatus st=nl_semantic_register_function_unit(s->ctx,items,1,&d);
    if(st!=NL_CHECK_OK) nl_check_diagnostic_render(stderr,s->src,&d.diagnostic);
    REQUIRE(st == NL_CHECK_OK);
    REQUIRE(nl_source_create("main()",6,"<R333-main>",&s->entry_src)==NL_SOURCE_OK);
    REQUIRE(nl_parser_create(s->entry_src,&s->entry_parser)==NL_PARSE_OK);
    REQUIRE(nl_parser_parse_expression_fragment(s->entry_parser,&s->entry_syntax,NULL)==NL_PARSE_OK);
    NLCheckDiagnostic cd={0};
    st=nl_semantic_check_expression(s->ctx,s->entry_syntax,&s->entry,&cd);
    if(st!=NL_CHECK_OK) nl_check_diagnostic_render(stderr,s->entry_src,&cd);
    REQUIRE(st==NL_CHECK_OK && s->entry && nl_checked_node_count(s->entry)>0);
    return true;
}
static NLCheckedNodeId nested_allocation(const NLCheckedFragment *f) {
    for(NLCheckedNodeId n=1;n<=nl_checked_node_count(f);++n){
        const NLCheckedNodeView *v=nl_checked_node_view(f,n);
        if(v && v->kind==NL_CHECKED_MATCH) {
            const NLCheckedNodeView *init=nl_checked_node_view(f,v->initializer);
            if(init && init->kind==NL_CHECKED_TRY_ALLOCATE_ONE) return n;
        }
    }
    return 0;
}
static bool inventory(const NLSemanticContext *c, NLValueId a[5], NLValueId d[5]){
    NLSemanticSnapshot sn;
    REQUIRE(nl_semantic_snapshot(c,&sn) && sn.backing_regions==5 && sn.domains==5);
    memset(a,0,5*sizeof(*a));memset(d,0,5*sizeof(*d));
    for(NLValueId i=1;i<=sn.values;++i) {
        NLSemanticValueView v; NLSemanticTypeView t;
        REQUIRE(nl_semantic_value_view(c,i,&v) && nl_semantic_type_view(c,v.type,&t));
        if(v.carrier==NL_CARRIER_ENDED) continue;
        if(t.kind==NL_TYPE_ALLOCATION) {
            REQUIRE(v.allocation_region>=1 && v.allocation_region<=5 && !a[v.allocation_region-1]);
            a[v.allocation_region-1]=i;
        }
        if(v.type==nl_semantic_domain_type(c)) {
            REQUIRE(v.domain>=1 && v.domain<=5 && !d[v.domain-1]);
            d[v.domain-1]=i;
        }
    }
    for(size_t i=0;i<5;++i) REQUIRE(a[i] && d[i]);
    return true;
}
static bool root_for(const NLSemanticContext *c,size_t region,
                     NLPlaceId *id,NLSemanticPlaceView *out) {
    NLSemanticSnapshot sn;
    REQUIRE(nl_semantic_snapshot(c,&sn));
    *id=0;
    for(NLPlaceId k=1;k<=sn.places;++k) {
        NLSemanticPlaceView p;
        REQUIRE(nl_semantic_place_view(c,k,&p));
        if(p.live && p.independent_root && p.placement.region==region &&
           p.governing_domain==region && p.fixed_field_count==4) {
            REQUIRE(*id==0);
            *id=k;*out=p;
        }
    }
    REQUIRE(*id);
    return true;
}
static bool link_target(const NLSemanticContext *ctx,NLPlaceId field,size_t wanted) {
    NLSemanticPlaceView p;
    NLSemanticValueView sum,ptr;
    REQUIRE(nl_semantic_place_view(ctx,field,&p) && p.live &&
            nl_semantic_value_view(ctx,p.current_value,&sum));
    if(wanted==0) {REQUIRE(sum.variant==1 && !p.payload_occurrence);return true;}
    REQUIRE(sum.variant==2 && p.payload_occurrence && sum.sum_payload);
    REQUIRE(nl_semantic_value_view(ctx,sum.sum_payload,&ptr) &&
            ptr.reference.provenance==NL_PROVENANCE_VALID);
    NLSemanticPlaceView target;
    REQUIRE(nl_semantic_place_view(ctx,ptr.reference.place,&target) && target.live &&
            target.placement.region==wanted);
    return true;
}
static bool check_transition(const NLCapturedChangeView *ch, size_t root_region,
                            size_t field_index,size_t old_target,size_t new_target) {
    const NLSemanticContext *before=ch->before,*after=ch->after;
    REQUIRE(before && after);
    NLPlaceId rid0=0,rid1=0;
    NLSemanticPlaceView r0,r1,f0,f1;
    REQUIRE(root_for(before,root_region,&rid0,&r0) &&
            root_for(after,root_region,&rid1,&r1));
    REQUIRE(rid0==rid1 && r0.incarnation==r1.incarnation &&
            r0.placement.region==r1.placement.region);
    NLPlaceId field=r0.fixed_fields[field_index];
    REQUIRE(field && field==r1.fixed_fields[field_index]);
    REQUIRE(nl_semantic_place_view(before,field,&f0) &&
            nl_semantic_place_view(after,field,&f1));
    REQUIRE(f0.incarnation==f1.incarnation && f0.current_fact!=f1.current_fact &&
            r0.current_fact!=r1.current_fact);
    REQUIRE(link_target(before,field,old_target));
    REQUIRE(link_target(after,field,new_target));
    REQUIRE(f0.payload_occurrence && f0.payload_occurrence!=f1.payload_occurrence);
    NLSemanticOccurrenceView old0,old1;
    REQUIRE(nl_semantic_occurrence_view(before,f0.payload_occurrence,&old0) && old0.live);
    REQUIRE(nl_semantic_occurrence_view(after,f0.payload_occurrence,&old1) && !old1.live);
    if(new_target) {
        NLSemanticOccurrenceView fresh;
        REQUIRE(nl_semantic_occurrence_view(after,f1.payload_occurrence,&fresh) && fresh.live);
    }
    for(size_t j=0;j<4;++j) if(j!=field_index) {
        NLSemanticPlaceView b,a;
        REQUIRE(nl_semantic_place_view(before,r0.fixed_fields[j],&b) &&
                nl_semantic_place_view(after,r1.fixed_fields[j],&a));
        REQUIRE(b.incarnation==a.incarnation && b.current_value==a.current_value &&
                b.current_fact==a.current_fact &&
                b.payload_occurrence==a.payload_occurrence);
    }
    printf("R333_CHANGE operation=%zu region=%zu field=%zu old=%zu new=%zu old_occ=%zu new_occ=%zu\n",
           ch->operation,root_region,field_index,old_target,new_target,
           f0.payload_occurrence,f1.payload_occurrence);
    return true;
}
static bool exact_five_and_twenty(const NLSemanticContext *third_after,
                                 const NLSemanticContext *fourth_before,
                                 const NLSemanticContext *final){
    static const size_t topology[5][3]={{0,0,2},{4,4,0},{0,0,0},{0,2,0},{0,0,0}};
    for(size_t i=1;i<=5;++i){
        NLPlaceId l,r,z;NLSemanticPlaceView p,q,fin;
        REQUIRE(root_for(third_after,i,&l,&p) &&
                root_for(fourth_before,i,&r,&q) &&
                root_for(final,i,&z,&fin));
        REQUIRE(l==r && r==z && p.incarnation==q.incarnation &&
                p.current_fact==q.current_fact);
        for(size_t j=0;j<4;++j){
            NLSemanticPlaceView f,g;
            REQUIRE(p.fixed_fields[j]==q.fixed_fields[j] &&
                    nl_semantic_place_view(third_after,p.fixed_fields[j],&f) &&
                    nl_semantic_place_view(fourth_before,q.fixed_fields[j],&g));
            REQUIRE(f.live && g.live && f.current_value==g.current_value &&
                    f.current_fact==g.current_fact &&
                    f.payload_occurrence==g.payload_occurrence);
        }
        for(size_t j=0;j<3;++j) REQUIRE(link_target(final,fin.fixed_fields[j],topology[i-1][j]));
    }
    puts("R333_FIVE_ORIGINAL_ROOTS_AND_20_SIBLING_VIEWS_OK");
    return true;
}
/* Independent R333 follow-on; public views from the same source-generated main. */
static bool release_sequence(const NLCheckedFragment *f,size_t regions[5],size_t *count,size_t depth) {
    REQUIRE(depth<5);
    const NLSemanticContext *c=nl_checked_context(f);
    for(NLCheckedNodeId i=1;i<=nl_checked_node_count(f);++i) {
        const NLCheckedNodeView *n=nl_checked_node_view(f,i);
        REQUIRE(n);
        if(n->kind==NL_CHECKED_DEALLOCATE) {
            const NLCheckedNodeView *a=nl_checked_node_view(f,n->first_argument);
            NLSemanticValueView value;
            REQUIRE(*count<5 && a && a->result_count==1 &&
                    nl_semantic_value_view(c,a->results[0].value,&value) &&
                    value.allocation_region>=1 && value.allocation_region<=5);
            regions[(*count)++]=value.allocation_region;
        }
        if(n->kind==NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedFragment *callee=nl_checked_call_body(f,i);
            REQUIRE(callee);
            REQUIRE(release_sequence(callee,regions,count,depth+1));
        }
    }
    return true;
}
/* Count a single original nonCopy authority path to the returned aggregate.
 * This uses public carrier/field views; no manually authored positive world. */
static bool original_is_inside_return(const NLSemanticContext *ctx,
                                      NLValueId source,NLValueId result) {
    NLValueId traversed=source;
    for(size_t depth=0;depth<9;++depth) {
        if(traversed==result) return true;
        NLSemanticValueView current,parent;
        REQUIRE(nl_semantic_value_view(ctx,traversed,&current) &&
                current.carrier==NL_CARRIER_AGGREGATE && current.aggregate_owner &&
                nl_semantic_value_view(ctx,current.aggregate_owner,&parent));
        size_t appearances=0;
        for(size_t i=0;i<parent.field_count;++i)
            appearances+=parent.fields[i]==traversed;
        REQUIRE(appearances==1);
        traversed=current.aggregate_owner;
    }
    return false;
}
static bool returned_two_owner(const NLCheckedFragment *f) {
    NLWholeValueCallView call={0};
    NLTwoRootCallView late={0};
    NLCheckedNodeId call_id=0,late_id=0;
    for(NLCheckedNodeId n=1;n<=nl_checked_node_count(f);++n) {
        NLWholeValueCallView w;
        NLTwoRootCallView t;
        if(nl_checked_whole_value_call_view(f,n,&w)) {
            REQUIRE(!call_id);
            call=w;call_id=n;
        }
        if(nl_checked_two_root_call_view(f,n,&t)) {
            REQUIRE(!late_id);
            late=t;late_id=n;
        }
    }
    REQUIRE(call_id && late_id>call_id && call.count==3 &&
            late.input==call.result && late.donor==call.receiver);
    NLCheckStatus validation=nl_checked_two_root_call_validate(f,late_id);
    REQUIRE(validation==NL_CHECK_OK);
    NLValueId baseline_a[5],baseline_d[5],other_a[5],other_d[5];
    REQUIRE(inventory(call.entry,baseline_a,baseline_d));
    const NLSemanticContext *worlds[]={call.entry,call.returned,call.received};
    for(size_t i=0;i<3;++i) {
        REQUIRE(inventory(worlds[i],other_a,other_d));
        for(size_t j=0;j<5;++j) REQUIRE(other_a[j]==baseline_a[j] && other_d[j]==baseline_d[j]);
        NLSemanticSnapshot snapshot;
        REQUIRE(nl_semantic_snapshot(worlds[i],&snapshot));
        for(NLScopeId k=1;k<=snapshot.scopes;++k) {
            NLSemanticScopeView scope;
            REQUIRE(nl_semantic_scope_view(worlds[i],k,&scope) && !scope.active);
        }
    }
    NLSemanticValueView receipt,ret,copytok;
    REQUIRE(nl_semantic_value_view(call.returned,call.result,&ret) &&
            nl_semantic_value_view(call.received,call.result,&receipt) &&
            ret.carrier==NL_CARRIER_LOOSE && receipt.carrier==NL_CARRIER_PLACE &&
            receipt.field_count==2 && ret.field_count==2 &&
            receipt.fields[0]==call.inputs[0] && receipt.fields[1]==call.inputs[1]);
    NLSemanticBindingView recv,donor,formal,done;
    REQUIRE(nl_semantic_binding_view(call.received,call.receiver,&recv) &&
            recv.availability==NL_AVAILABLE && recv.value==call.result);
    NLSemanticSnapshot snap_received;
    REQUIRE(nl_semantic_snapshot(call.received,&snap_received));
    size_t current_receivers=0;
    for(size_t binding=1;binding<=snap_received.bindings;++binding) {
        NLSemanticBindingView candidate;
        REQUIRE(nl_semantic_binding_view(call.received,binding,&candidate));
        current_receivers+=candidate.availability==NL_AVAILABLE &&
                           candidate.value==call.result && candidate.place==recv.place;
    }
    REQUIRE(current_receivers==1);
    const size_t originals[2]={5,3};
    for(size_t i=0;i<2;++i) {
        NLSemanticValueView owner,allocation,domain,pointer;
        NLSemanticPlaceView root;
        REQUIRE(nl_semantic_binding_view(call.entry,call.donors[i],&donor) &&
                donor.availability==NL_AVAILABLE && donor.value==call.inputs[i] &&
                nl_semantic_binding_view(call.returned,call.donors[i],&done) &&
                done.availability==NL_CONSUMED &&
                nl_semantic_binding_view(call.returned,call.parameters[i],&formal) &&
                formal.availability==NL_CONSUMED && formal.value==call.inputs[i] &&
                nl_semantic_value_view(call.received,call.inputs[i],&owner) &&
                owner.field_count==3 &&
                nl_semantic_value_view(call.received,owner.fields[0],&pointer) &&
                nl_semantic_value_view(call.received,owner.fields[1],&allocation) &&
                nl_semantic_value_view(call.received,owner.fields[2],&domain) &&
                nl_semantic_place_view(call.received,pointer.reference.place,&root) &&
                root.live && root.independent_root &&
                pointer.reference.provenance==NL_PROVENANCE_VALID &&
                pointer.reference.incarnation==root.incarnation &&
                root.placement.region==originals[i] &&
                allocation.allocation_region==originals[i] &&
                domain.domain==originals[i] &&
                owner.fields[1]==baseline_a[originals[i]-1] &&
                owner.fields[2]==baseline_d[originals[i]-1] &&
                original_is_inside_return(call.returned,owner.fields[1],call.result) &&
                original_is_inside_return(call.returned,owner.fields[2],call.result) &&
                original_is_inside_return(call.received,owner.fields[1],call.result) &&
                original_is_inside_return(call.received,owner.fields[2],call.result));
        printf("R333_ORIGINAL_RETURN_MEMBER index=%zu region=%zu root=%zu A=%zu D=%zu\n",
               i,originals[i],pointer.reference.place,owner.fields[1],owner.fields[2]);
    }
    REQUIRE(nl_semantic_binding_view(call.entry,call.donors[2],&donor) &&
            nl_semantic_binding_view(call.received,call.donors[2],&done) &&
            donor.availability==NL_AVAILABLE && done.availability==NL_AVAILABLE &&
            donor.value==done.value &&
            nl_semantic_value_view(call.received,donor.value,&copytok) &&
            !copytok.domain && !copytok.allocation_region && !copytok.reference.scope);
    NLSemanticBindingView consumed;
    REQUIRE(nl_semantic_binding_view(late.entry,late.donor,&recv) &&
            recv.availability==NL_AVAILABLE && recv.value==call.result &&
            nl_semantic_binding_view(late.returned,late.donor,&consumed) &&
            consumed.availability==NL_CONSUMED &&
            nl_semantic_binding_view(late.returned,late.parameter,&consumed) &&
            consumed.availability==NL_CONSUMED);
    for(size_t r=1;r<=5;++r) {
        NLSemanticBackingView b0,b1;
        NLSemanticDomainView d0,d1;
        REQUIRE(nl_semantic_backing_view(late.entry,r,&b0) &&
                nl_semantic_backing_view(late.returned,r,&b1) &&
                nl_semantic_domain_view(late.entry,r,&d0) &&
                nl_semantic_domain_view(late.returned,r,&d1) &&
                b0.live==(r==3||r==5) && d0.live==b0.live &&
                !b1.live && !d1.live);
    }
    NLTwoRootDefinition summary;
    REQUIRE(nl_semantic_two_root_definition(late.entry,nl_checked_node_view(f,late_id)->function,&summary) &&
            summary.definition_checked && summary.count==2 &&
            summary.calls[0].member!=summary.calls[1].member);
    for(size_t j=0;j<2;++j) REQUIRE(summary.calls[j].definition.requirements==NL_OWNER_ALL_REQUIREMENTS);
    size_t regions[5]={0},count=0;
    REQUIRE(release_sequence(f,regions,&count,0) && count==5);
    const size_t expected[5]={2,4,1,3,5};
    for(size_t i=0;i<5;++i) REQUIRE(regions[i]==expected[i]);
    printf("R333_WHOLE_VIEW entry_return_receive=%d,%d,%d join_node=%zu returned=%zu receiver=%zu late_node=%zu late_status=%d\n",
           !!call.entry,!!call.returned,!!call.received,call_id,call.result,call.receiver,late_id,(int)validation);
    puts("R333_EIGHT_PUBLIC_VALIDATORS_AND_RETURNED_ONLY_ORDER_OK");
    return true;
}
static bool inspect(const NLCheckedFragment *entry){
    const NLCheckedFragment *f=nl_checked_call_body(entry,nl_checked_root(entry));
    REQUIRE(f);
    for(size_t i=0;i<5;++i) {
        const NLCheckedNodeId m=nested_allocation(f);
        REQUIRE(m);
        const NLCheckStatus st=nl_checked_captured_closure_validate(f,m);
        NLCapturedClosureView view;
        REQUIRE(nl_checked_captured_closure_view(f,m,&view));
        printf("R333_CAPTURE_PROBE i=%zu status=%d count=%zu releases=%zu,%zu,%zu,%zu,%zu,%zu\\n",i,(int)st,view.count,view.release_worlds[0],view.release_worlds[1],view.release_worlds[2],view.release_worlds[3],view.release_worlds[4],view.release_worlds[5]);
        REQUIRE(st==NL_CHECK_OK && view.count==i);
        for(size_t failure_world=i;failure_world<=5;++failure_world)
            REQUIRE(view.release_worlds[failure_world]==1);
        printf("R333_CAPTURED index=%zu match=%zu status=%d original_count=%zu releases=%zu\n",
                i,m,(int)st,view.count,view.release_worlds[i+1]);
        f=nl_checked_match_arm(f,m,1);
        REQUIRE(f);
    }
    REQUIRE(nl_checked_captured_change_count(f)==8);
    size_t calls=0;
    for(NLCheckedNodeId id=1;id<=nl_checked_node_count(f);++id){
        NLScopedFieldCallView w;
        if(!nl_checked_scoped_field_call_view(f,id,&w)) continue;
        REQUIRE(calls<2);
        NLCheckStatus st=nl_checked_scoped_field_call_validate(f,id);
        const NLCheckedNodeView *p=nl_checked_node_view(f,w.projection);
        REQUIRE(st==NL_CHECK_OK && w.body && w.change.before && w.change.after &&
                p && p->kind==NL_CHECKED_FIELD_REF &&
                p->field.index==1-calls);
        REQUIRE(check_transition(&w.change,3,1-calls,calls==0?2:4,0));
        NLSemanticScopeView scope;
        REQUIRE(p->reference_result.scope &&
                nl_semantic_scope_view(nl_checked_context(f),
                                       p->reference_result.scope,&scope) && !scope.active);
        printf("R333_SCOPED index=%zu call=%zu status=%d scope=%zu\n",
               calls,id,(int)st,p->reference_result.scope);
        ++calls;
    }
    REQUIRE(calls==2);
    NLCapturedChangeView third,fourth;
    REQUIRE(nl_checked_captured_change_view(f,6,&third) &&
            nl_checked_captured_change_view(f,7,&fourth));
    REQUIRE(check_transition(&third,2,0,3,4));
    REQUIRE(check_transition(&fourth,4,1,3,2));
    NLCapturedChangeView direct[2]={third,fourth};
    for(size_t i=0;i<2;++i){
        const NLCheckedNodeView *write=nl_checked_node_view(f,direct[i].operation);
        const NLCheckedNodeView *projection=write?nl_checked_node_view(f,write->first_argument):NULL;
        NLSemanticScopeView scope;
        REQUIRE(write && write->kind==NL_CHECKED_REPLACE &&
                projection && projection->kind==NL_CHECKED_FIELD_REF &&
                projection->reference_result.scope &&
                nl_semantic_scope_view(nl_checked_context(f),
                                       projection->reference_result.scope,&scope) &&
                !scope.active);
        printf("R333_DIRECT_SCOPE_CLOSED index=%zu scope=%zu\\n",
               i,projection->reference_result.scope);
    }
    NLValueId a0[5],d0[5],a1[5],d1[5];
    REQUIRE(inventory(third.before,a0,d0) && inventory(fourth.after,a1,d1));
    for(size_t i=0;i<5;++i) REQUIRE(a0[i]==a1[i] && d0[i]==d1[i]);
    printf("R333_ORIGINAL_AUTHORITY_IDS allocation=%zu,%zu,%zu,%zu,%zu domain=%zu,%zu,%zu,%zu,%zu\n",
           a1[0],a1[1],a1[2],a1[3],a1[4],d1[0],d1[1],d1[2],d1[3],d1[4]);
    REQUIRE(exact_five_and_twenty(third.after,fourth.before,fourth.after));
    puts("R333_SEVEN_PUBLIC_VALIDATORS_STATUS_ZERO");
    REQUIRE(returned_two_owner(f));
    return true;
}
int main(int argc,char **argv) {
    if(argc!=2) {fprintf(stderr,"usage: %s R_SOURCE.nl\n",argv[0]);return 2;}
    Session s={0};
    bool ok=load(&s,argv[1]);
    if(ok) ok=inspect(s.entry);
    clear(&s);
    return ok?0:1;
}
