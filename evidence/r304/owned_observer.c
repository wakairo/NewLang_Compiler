/* NewLang R304: independent public-header-only source-owned witness.
   Never seeds or mutates a completed artifact or its captured worlds. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "newlang/parser.h"
#include "newlang/captured_closure.h"
#include "newlang/raw_storage.h"

#define NEED(x) do { if (!(x)) { fprintf(stderr,"R304 owned check line %d: %s\n",__LINE__,#x); exit(71); } } while (0)

static NLCheckedNodeId trial(const NLCheckedFragment *f) {
    NLCheckedNodeId id=0;
    for(size_t i=1;i<=nl_checked_node_count(f);i++){
        const NLCheckedNodeView *n=nl_checked_node_view(f,i);
        if(!n || n->kind!=NL_CHECKED_MATCH)continue;
        const NLCheckedNodeView *v=nl_checked_node_view(f,n->initializer);
        if(v && v->kind==NL_CHECKED_TRY_ALLOCATE_ONE){NEED(!id);id=i;}
    }
    return id;
}

static void owners(const NLSemanticContext *ctx,size_t a[5],size_t d[5]){
    NLSemanticSnapshot s={0};
    NEED(nl_semantic_snapshot(ctx,&s));
    NEED(s.backing_regions==5 && s.domains==5);
    memset(a,0,sizeof(size_t)*5);memset(d,0,sizeof(size_t)*5);
    for(size_t i=1;i<=s.values;i++){
        NLSemanticValueView v={0};NLSemanticTypeView t={0};
        NEED(nl_semantic_value_view(ctx,i,&v));
        NEED(nl_semantic_type_view(ctx,v.type,&t));
        if(v.carrier==NL_CARRIER_ENDED)continue;
        if(t.kind==NL_TYPE_ALLOCATION){
            NEED(v.allocation_region>=1 && v.allocation_region<=5);
            NEED(!a[v.allocation_region-1]);a[v.allocation_region-1]=i;
        }
        if(v.type==nl_semantic_domain_type(ctx)){
            NEED(v.domain>=1 && v.domain<=5);
            NEED(!d[v.domain-1]);d[v.domain-1]=i;
        }
    }
    for(size_t i=0;i<5;i++){
        NLSemanticBackingView b={0};NLSemanticDomainView dv={0};
        NEED(a[i] && d[i]);
        NEED(nl_semantic_backing_view(ctx,i+1,&b) && b.live);
        NEED(nl_semantic_domain_view(ctx,i+1,&dv) && dv.live && dv.value==d[i]);
    }
}

static void audit(const NLCheckedFragment *entry,size_t expected){
    const NLCheckedFragment *f=nl_checked_call_body(entry,nl_checked_root(entry));
    NEED(f);
    size_t closures=0;
    while(closures<5){
        NLCheckedNodeId site=trial(f);NLCapturedClosureView cv={0};
        NEED(site);
        NEED(nl_checked_captured_closure_view(f,site,&cv));
        NEED(nl_checked_captured_closure_validate(f,site)==NL_CHECK_OK);
        f=nl_checked_match_arm(f,site,1);
        NEED(f);closures++;
    }
    NEED(nl_checked_captured_change_count(f)==6);
    NLScopedFieldCallView call={0};NLCheckedNodeId call_id=0;
    for(size_t i=1;i<=nl_checked_node_count(f);i++){
        NLScopedFieldCallView candidate={0};
        if(nl_checked_scoped_field_call_view(f,i,&candidate)){
            NEED(!call_id);call_id=i;call=candidate;
        }
    }
    NEED(call_id && call.body && call.change.before && call.change.after);
    NEED(nl_checked_scoped_field_call_validate(f,call_id)==NL_CHECK_OK);
    NEED(nl_checked_captured_change_count(call.body)==1);
    const NLCheckedNodeView *op=nl_checked_node_view(call.body,call.change.operation);
    const NLCheckedNodeView *proj=nl_checked_node_view(f,call.projection);
    NEED(op && proj && op->kind==NL_CHECKED_REPLACE && proj->kind==NL_CHECKED_FIELD_REF);
    NEED(op->field.index==0 && proj->field.index==0 &&
         op->field.nominal==proj->field.nominal && op->field.child==proj->field.child);
    const NLSemanticContext *world[2]={call.change.before,call.change.after};
    NLSemanticPlaceView root[2]={0},link[2]={0},prev[2]={0};
    NLSemanticValueView value[2]={0},payload[2]={0};
    size_t alloc[2][5],domains[2][5];
    for(size_t i=0;i<2;i++){
        owners(world[i],alloc[i],domains[i]);
        NEED(nl_semantic_place_view(world[i],op->field.parent,&root[i]) && root[i].live);
        NEED(root[i].fixed_field_count==4 && root[i].fixed_fields[0]==op->field.child);
        NEED(root[i].fixed_fields[0]!=root[i].fixed_fields[1]);
        NEED(nl_semantic_place_view(world[i],op->field.child,&link[i]) && link[i].live);
        NEED(nl_semantic_place_view(world[i],root[i].fixed_fields[1],&prev[i]) && prev[i].live);
        NEED(nl_semantic_value_view(world[i],link[i].current_value,&value[i]));
        if(value[i].variant==2)NEED(nl_semantic_value_view(world[i],value[i].sum_payload,&payload[i]));
    }
    NEED(!memcmp(alloc[0],alloc[1],sizeof alloc[0]));
    NEED(!memcmp(domains[0],domains[1],sizeof domains[0]));
    NEED(root[0].placement.region==2 && root[0].governing_domain==2);
    NEED(root[0].incarnation==root[1].incarnation);
    NEED(!memcmp(&root[0].placement,&root[1].placement,sizeof root[0].placement));
    NEED(root[0].current_fact!=root[1].current_fact);
    NEED(link[0].incarnation==link[1].incarnation);
    NEED(link[0].current_fact!=link[1].current_fact);
    NEED(link[0].current_value==op->field.old_value);
    NEED(link[1].current_value==op->field.new_value);
    NEED(link[0].current_value!=link[1].current_value);
    NEED(prev[0].current_fact && prev[0].payload_occurrence);
    NEED(!memcmp(&prev[0],&prev[1],sizeof prev[0]));
    NEED(value[0].variant==2 && link[0].payload_occurrence);
    NLSemanticPlaceView prior_target={0};
    NEED(nl_semantic_place_view(world[0],payload[0].reference.place,&prior_target));
    NEED(prior_target.placement.region==3);
    NLSemanticOccurrenceView old0={0},old1={0};
    NEED(nl_semantic_occurrence_view(world[0],link[0].payload_occurrence,&old0) && old0.live);
    NEED(nl_semantic_occurrence_view(world[1],link[0].payload_occurrence,&old1) && !old1.live);
    NLSemanticPlaceView oldplace0={0},oldplace1={0};
    NEED(nl_semantic_place_view(world[0],old0.payload_place,&oldplace0));
    NEED(nl_semantic_place_view(world[1],old0.payload_place,&oldplace1));
    NEED(oldplace0.live && oldplace0.current_fact);
    NEED(!oldplace1.live && !oldplace1.current_fact && !oldplace1.current_value);
    if(expected){
        NLSemanticOccurrenceView updated={0};NLSemanticPlaceView new_target={0};
        NEED(value[1].variant==2 && link[1].payload_occurrence!=link[0].payload_occurrence);
        NEED(link[1].payload_occurrence);
        NEED(nl_semantic_occurrence_view(world[1],link[1].payload_occurrence,&updated) && updated.live);
        NEED(nl_semantic_place_view(world[1],payload[1].reference.place,&new_target));
        NEED(new_target.placement.region==expected);
    }else{
        NEED(value[1].variant==1 && !link[1].payload_occurrence);
    }
    NLSemanticScopeView scoped={0};
    NEED(nl_semantic_scope_view(world[0],proj->reference_result.scope,&scoped) && scoped.active);
    NEED(nl_semantic_scope_view(nl_checked_context(f),proj->reference_result.scope,&scoped) && !scoped.active);
    unsigned seen=0;
    for(size_t i=1;i<=nl_checked_node_count(f);i++){
        const NLCheckedNodeView *n=nl_checked_node_view(f,i);
        if(!n || n->kind!=NL_CHECKED_DEALLOCATE)continue;
        const NLCheckedNodeView *a=nl_checked_node_view(f,n->first_argument);
        const NLCheckedNodeView *r=a?nl_checked_node_view(f,a->next_argument):NULL;
        NLSemanticValueView av={0},rv={0};
        NEED(a && r && a->result_count==1 && r->result_count==1);
        NEED(nl_semantic_value_view(nl_checked_context(f),a->results[0].value,&av));
        NEED(nl_semantic_value_view(nl_checked_context(f),r->results[0].value,&rv));
        NEED(av.allocation_region>=1 && av.allocation_region<=5);
        NEED(av.allocation_region==rv.occupancy.region && rv.occupancy.start==0 && rv.occupancy.length);
        unsigned bit=1u<<(av.allocation_region-1);
        NEED(!(seen&bit));seen|=bit;
    }
    NEED(seen==31u);
    printf("{\"source_owned\":true,\"closure_validator_count\":%zu,\"caller_changes\":6,"
           "\"callee_changes\":1,\"scoped_validator\":true,\"root\":%zu,\"incarnation\":%zu,"
           "\"projection_nominal\":%zu,\"projection_index\":%zu,\"old_occ\":%zu,"
           "\"new_occ\":%zu,\"new_ptr_region\":%zu,\"old_payload_cleared\":true,"
           "\"A_prev_preserved\":true,\"five_owners_unique\":true,"
           "\"five_matched_terminal_releases\":true,\"loan_scope_ended\":true,"
           "\"native_C17\":false}\n",
           closures,op->field.parent,root[0].incarnation,op->field.nominal,
           op->field.index,link[0].payload_occurrence,link[1].payload_occurrence,expected);
}

int main(int argc,char **argv){
    NEED(argc==3);
    size_t region=(size_t)strtoul(argv[2],NULL,10);
    NEED(region==0 || region==3 || region==4);
    NLSource *source=NULL,*entry_source=NULL;NLParser *parser=NULL,*entry_parser=NULL;
    NLSyntaxTree *tree=NULL,*entry_syntax=NULL;NLSemanticContext *ctx=NULL;
    NLCheckedFragment *artifact=NULL;NLParseDiagnostic pd={0};
    NEED(nl_source_load(argv[1],&source)==NL_SOURCE_OK);
    NEED(nl_parser_create(source,&parser)==NL_PARSE_OK);
    NEED(nl_parser_parse_function_unit(parser,&tree,&pd)==NL_PARSE_OK);
    NEED(nl_semantic_create(&ctx)==NL_CHECK_OK);
    const NLSyntaxTree *items[]={tree};
    NLFunctionUnitDiagnostic fd={0};
    NLCheckStatus registration=nl_semantic_register_function_unit(ctx,items,1,&fd);
    if(registration!=NL_CHECK_OK)nl_check_diagnostic_render(stderr,source,&fd.diagnostic);
    NEED(registration==NL_CHECK_OK);
    NEED(nl_source_create("main()",6,"<R304-call>",&entry_source)==NL_SOURCE_OK);
    NEED(nl_parser_create(entry_source,&entry_parser)==NL_PARSE_OK);
    NEED(nl_parser_parse_expression_fragment(entry_parser,&entry_syntax,&pd)==NL_PARSE_OK);
    NLCheckDiagnostic cd={0};
    NLCheckStatus status=nl_semantic_check_expression(ctx,entry_syntax,&artifact,&cd);
    if(status!=NL_CHECK_OK)nl_check_diagnostic_render(stderr,entry_source,&cd);
    NEED(status==NL_CHECK_OK && artifact);
    audit(artifact,region);
    nl_checked_destroy(artifact);nl_semantic_destroy(ctx);
    nl_syntax_tree_destroy(entry_syntax);nl_parser_destroy(entry_parser);nl_source_destroy(entry_source);
    nl_syntax_tree_destroy(tree);nl_parser_destroy(parser);nl_source_destroy(source);
    return 0;
}
