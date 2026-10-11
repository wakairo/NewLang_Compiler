/*
 * R322 independently authored owned-artifact corruption observer.
 * NON-BLIND test-only negative fixture; no compiler or production changes.
 * Positives originate exclusively in the real parser/register/entry checker.
 * Private layout is used ONLY to perturb existing checked-owned memory safely;
 * all pass/fail oracle decisions come from seven PUBLIC validator calls.
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "newlang/source.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/checked.h"
#include "newlang/captured_closure.h"
#include "semantic_internal.h"

#define REQUIRE(e) do { if (!(e)) { fprintf(stderr,"R322_BARRIER line=%d expr=%s\n",__LINE__,#e); goto done; } } while(0)
typedef struct {
    const NLCheckedFragment *f;
    NLCheckedNodeId id;
} PublicCall;
typedef struct {
    NLCheckedFragment *seventh;
    PublicCall closures[5], scoped[2];
    size_t nc, ns, seven_count;
} Walk;
typedef struct {
    void *at;
    unsigned char *original;
    size_t bytes;
} Saved;
typedef struct {
    NLCheckedFragment *f;
    NLCapturedChange *seventh;
    NLCheckedNodeView *write;
    NLSemanticContext *before,*after;
    NLCheckedNodeId first_call,second_call;
    NLPlaceId root_a,root_b,root_c,old_payload;
    NLPlaceId field_a,field_b,field_other;
    NLOccurrenceId old_occ,new_occ;
    NLScopeId a_scope,b_scope;
    NLValueId old_payload_value,new_payload_value;
} Site;

static int walk(NLCheckedFragment *f, Walk *w, unsigned depth) {
    if (!f || depth>32) return 0;
    if (nl_checked_captured_change_count(f)==7) {
        if (w->seven_count++ || w->seventh) return 0;
        w->seventh=f;
    }
    for (NLCheckedNodeId id=1; id<=nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n=nl_checked_node_view(f,id);
        if (!n) return 0;
        if (n->kind==NL_CHECKED_MATCH) {
            NLCapturedClosureView v={0};
            if (nl_checked_captured_closure_view(f,id,&v)) {
                if(w->nc>=5) return 0;
                w->closures[w->nc++]=(PublicCall){f,id};
            }
            for (size_t arm=0;arm<2;arm++) {
                const NLCheckedFragment *a=nl_checked_match_arm(f,id,arm);
                if (a && !walk((NLCheckedFragment *)a,w,depth+1)) return 0;
            }
        }
        if (n->kind==NL_CHECKED_IF) {
            for (size_t arm=0;arm<2;arm++) {
                const NLCheckedFragment *a=nl_checked_if_arm(f,id,arm);
                if (a && !walk((NLCheckedFragment *)a,w,depth+1)) return 0;
            }
        }
        if (n->kind==NL_CHECKED_REGISTERED_CALL) {
            NLScopedFieldCallView v={0};
            if (nl_checked_scoped_field_call_view(f,id,&v)) {
                if (w->ns>=2) return 0;
                w->scoped[w->ns++]=(PublicCall){f,id};
            }
            const NLCheckedFragment *body=nl_checked_call_body(f,id);
            if (body && !walk((NLCheckedFragment *)body,w,depth+1)) return 0;
        }
    }
    return 1;
}
static int public_statuses(const Walk *w, NLCheckStatus status[7],bool emit) {
    if(w->nc!=5 || w->ns!=2) return 0;
    for(size_t i=0;i<5;i++)
        status[i]=nl_checked_captured_closure_validate(w->closures[i].f,w->closures[i].id);
    for(size_t i=0;i<2;i++)
        status[5+i]=nl_checked_scoped_field_call_validate(w->scoped[i].f,w->scoped[i].id);
    if(emit) {
        printf("PUBLIC_VALIDATOR_STATUS closure=[");
        for(size_t i=0;i<5;i++)printf("%s%d",i?",":"",(int)status[i]);
        printf("] named=[%d,%d]\n",(int)status[5],(int)status[6]);
    }
    return 1;
}
static int all_clean(const Walk *w) {
    NLCheckStatus s[7];
    if(!public_statuses(w,s,true))return 0;
    for(size_t i=0;i<7;i++)if(s[i]!=NL_CHECK_OK)return 0;
    return 1;
}
static int lookup_root(const NLSemanticContext *ctx,size_t region,NLPlaceId *out) {
    NLSemanticSnapshot snap={0};
    if(!nl_semantic_snapshot(ctx,&snap))return 0;
    *out=0;
    for(NLPlaceId id=1;id<=snap.places;id++) {
        NLSemanticPlaceView p={0};
        if(!nl_semantic_place_view(ctx,id,&p))return 0;
        if(p.live && p.fixed_field_count==4 && p.placement.region==region) {
            if(*out)return 0;
            *out=id;
        }
    }
    return *out!=0;
}
static int prepare(const Walk *w, Site *s) {
    memset(s,0,sizeof(*s));
    s->f=w->seventh;
    if(!s->f || s->f->field_change_count!=7)return 0;
    s->seventh=s->f->field_changes[6];
    s->first_call=s->f->scoped_field_call;
    s->second_call=s->f->second_scoped_field_call;
    if(!s->seventh || !s->first_call || !s->second_call ||
       s->first_call>=s->second_call || s->second_call>=s->seventh->node ||
       !s->seventh->before || !s->seventh->after ||
       s->seventh->node>s->f->count)return 0;
    s->write=&s->f->nodes[s->seventh->node-1];
    s->before=s->seventh->before;
    s->after=s->seventh->after;
    if(s->write->kind!=NL_CHECKED_REPLACE || s->write->field.index!=0 ||
       !s->write->field.old_value || !s->write->field.new_value ||
       s->write->field.old_value>s->before->value_count ||
       s->write->field.new_value>s->before->value_count)return 0;
    NLScopedFieldCallView named[2]={{0}};
    if(!nl_checked_scoped_field_call_view(s->f,s->first_call,&named[0]) ||
       !nl_checked_scoped_field_call_view(s->f,s->second_call,&named[1]))return 0;
    const NLCheckedNodeView *bwrite=nl_checked_node_view(named[1].body,named[1].change.operation);
    const NLCheckedNodeView *bproj=nl_checked_node_view(s->f,named[0].projection);
    const NLCheckedNodeView *aproj=nl_checked_node_view(s->f,s->write->first_argument);
    if(!bwrite || !bproj || !aproj)return 0;
    s->b_scope=bproj->reference_result.scope;
    s->a_scope=aproj->reference_result.scope;
    s->root_a=s->write->field.parent;
    s->root_b=bwrite->field.parent;
    s->field_a=s->write->field.child;
    if(!s->root_a || !s->root_b || !s->field_a || 
       s->root_a>s->before->place_count || s->root_b>s->before->place_count ||
       s->field_a>s->before->place_count ||
       !s->a_scope || s->a_scope>s->f->context->scope_count ||
       !s->b_scope || s->b_scope>s->before->scope_count)return 0;
    NLSemanticPlaceView a0={0},a1={0},field0={0},field1={0};
    if(!nl_semantic_place_view(s->before,s->root_a,&a0) ||
       !nl_semantic_place_view(s->after,s->root_a,&a1) ||
       !nl_semantic_place_view(s->before,s->field_a,&field0) ||
       !nl_semantic_place_view(s->after,s->field_a,&field1) ||
       !a0.live || a0.placement.region!=2 || a0.governing_domain!=2 ||
       !a1.live || a1.governing_domain!=2 ||
       a0.fixed_fields[0]!=s->field_a || !field0.payload_occurrence ||
       !field1.payload_occurrence || field0.payload_occurrence==field1.payload_occurrence)
       return 0;
    s->old_occ=field0.payload_occurrence;
    s->new_occ=field1.payload_occurrence;
    if(s->old_occ>s->after->occurrence_count || s->new_occ>s->after->occurrence_count)return 0;
    NLSemanticOccurrenceView old={0},dead={0},fresh={0};
    if(!nl_semantic_occurrence_view(s->before,s->old_occ,&old) ||
       !nl_semantic_occurrence_view(s->after,s->old_occ,&dead) ||
       !nl_semantic_occurrence_view(s->after,s->new_occ,&fresh) ||
       !old.live || dead.live || !fresh.live || !old.payload_place ||
       old.payload_place>s->after->place_count)return 0;
    s->old_payload=old.payload_place;
    NLSemanticValueView v0={0},v1={0};
    if(!nl_semantic_value_view(s->before,s->write->field.old_value,&v0) ||
       !nl_semantic_value_view(s->before,s->write->field.new_value,&v1) ||
       v0.variant!=2 || v1.variant!=2 ||
       !v0.sum_payload || !v1.sum_payload ||
       v0.sum_payload>s->before->value_count ||
       v1.sum_payload>s->before->value_count)return 0;
    s->old_payload_value=v0.sum_payload;
    s->new_payload_value=v1.sum_payload;
    NLSemanticValueView old_ptr={0},new_ptr={0};
    if(!nl_semantic_value_view(s->before,s->old_payload_value,&old_ptr) ||
       !nl_semantic_value_view(s->before,s->new_payload_value,&new_ptr) ||
       old_ptr.reference.place!=s->root_b)return 0;
    s->root_c=new_ptr.reference.place;
    NLPlaceId c=0,src=0,dst=0;
    if(!lookup_root(s->before,1,&src) || !lookup_root(s->before,4,&c) ||
       !lookup_root(s->before,5,&dst) || c!=s->root_c ||
       !lookup_root(s->before,3,&s->root_b) ||
       !lookup_root(s->before,2,&s->root_a))return 0;
    s->field_b=s->before->places[s->root_b-1].fixed_fields[2];
    s->field_other=s->before->places[src-1].fixed_fields[2];
    if(!s->field_b || !s->field_other)return 0;
    const NLSemanticContext *prior=named[1].change.after;
    if(!prior)return 0;
    for(size_t r=1;r<=5;r++) {
        NLPlaceId root=0;
        if(!lookup_root(prior,r,&root))return 0;
        NLSemanticPlaceView p={0},q={0};
        if(!nl_semantic_place_view(prior,root,&p) ||
           !nl_semantic_place_view(s->before,root,&q) ||
           memcmp(&p,&q,sizeof(p)))return 0;
        for(size_t j=0;j<4;j++){
            NLSemanticPlaceView x={0},y={0};
            if(!nl_semantic_place_view(prior,p.fixed_fields[j],&x) ||
               !nl_semantic_place_view(s->before,p.fixed_fields[j],&y) ||
               memcmp(&x,&y,sizeof(x)))return 0;
        }
    }
    printf("R322_REAL_SEVENTH node=%zu calls=%zu,%zu original_A_root=%zu B_root=%zu C_root=%zu A_field=%zu old_occ=%zu new_occ=%zu five_root_20field_contiguity=1\n",
           s->seventh->node,s->first_call,s->second_call,s->root_a,s->root_b,s->root_c,s->field_a,s->old_occ,s->new_occ);
    return 1;
}
static int stash(Saved *sv,void *ptr,size_t n) {
    if(!ptr || !n)return 0;
    sv->original=malloc(n);
    if(!sv->original)return 0;
    sv->at=ptr;sv->bytes=n;
    memcpy(sv->original,ptr,n);
    return 1;
}
static void unstash(Saved *sv) {
    if(!sv->original)return;
    memcpy(sv->at,sv->original,sv->bytes);
    free(sv->original);
    sv->original=NULL;
}
/* Registered before executing CI. Every mutation is a valid in-bounds edit to
 * an actual generated object; no borrowed pointer redirection or allocator
 * poisoning. Discrete cases, whole-object bytewise restore, then revalidate.
 */
static const char *const classes[]={
 "evidence_operation_rebound_to_setup",
 "original_A_operation_kind_lost",
 "operation_retargets_original_B_root",
 "operation_targets_B_sibling_field",
 "operation_old_value_replaced_with_new",
 "operation_new_value_replaced_with_old",
 "operation_pre_post_fact_collapsed",
 "second_named_call_rebound_to_first",
 "seventh_chronology_before_second",
 "original_A_before_governing_D_changed",
 "original_A_after_backing_region_changed",
 "old_A_next_occurrence_resurrected",
 "old_A_next_payload_place_resurrected",
 "new_A_next_occurrence_aliases_old",
 "old_pointer_target_forged_to_C",
 "new_pointer_target_forged_to_B",
 "B_root_fact_changed_between_second_and_seventh",
 "other_original_sibling_fact_changed_between_calls",
 "first_B_scope_active_during_seventh",
 "A_scope_active_in_final_owned_context",
 "new_C_payload_forged_after_change"
};
#define NUM_CASES (sizeof(classes)/sizeof(classes[0]))
static int corrupt(unsigned which,Site *s,Saved *backup){
#define MUTATE(type,ptr,statement) do{type *cell=(ptr);if(!stash(backup,cell,sizeof(*cell)))return 0;statement;}while(0)
    switch(which){
    case 0: MUTATE(NLCapturedChange,s->seventh,cell->node=s->f->field_changes[1]->node);break;
    case 1: MUTATE(NLCheckedNodeView,s->write,cell->kind=NL_CHECKED_STATEMENT);break;
    case 2: MUTATE(NLCheckedNodeView,s->write,cell->field.parent=s->root_b);break;
    case 3: MUTATE(NLCheckedNodeView,s->write,cell->field.child=s->field_b);break;
    case 4: MUTATE(NLCheckedNodeView,s->write,cell->field.old_value=cell->field.new_value);break;
    case 5: MUTATE(NLCheckedNodeView,s->write,cell->field.new_value=cell->field.old_value);break;
    case 6: MUTATE(NLCheckedNodeView,s->write,cell->field.child_fact=cell->field.child_post_fact);break;
    case 7: MUTATE(NLCheckedFragment,s->f,cell->second_scoped_field_call=s->first_call);break;
    case 8: MUTATE(NLCheckedFragment,s->f,cell->second_scoped_field_call=s->seventh->node);break;
    case 9: MUTATE(NLSemanticPlaceView,&s->before->places[s->root_a-1],cell->governing_domain=3);break;
    case 10: MUTATE(NLSemanticPlaceView,&s->after->places[s->root_a-1],cell->placement.region=3);break;
    case 11: MUTATE(NLSemanticOccurrenceView,&s->after->occurrences[s->old_occ-1],cell->live=true);break;
    case 12: MUTATE(NLSemanticPlaceView,&s->after->places[s->old_payload-1],cell->live=true);break;
    case 13: MUTATE(NLSemanticPlaceView,&s->after->places[s->field_a-1],cell->payload_occurrence=s->old_occ);break;
    case 14: MUTATE(NLSemanticValueView,&s->before->values[s->old_payload_value-1],cell->reference.place=s->root_c);break;
    case 15: MUTATE(NLSemanticValueView,&s->before->values[s->new_payload_value-1],cell->reference.place=s->root_b);break;
    case 16: MUTATE(NLSemanticPlaceView,&s->before->places[s->root_b-1],cell->current_fact++);break;
    case 17: MUTATE(NLSemanticPlaceView,&s->before->places[s->field_other-1],cell->current_fact++);break;
    case 18: MUTATE(NLSemanticScopeView,&s->before->scopes[s->b_scope-1],cell->active=true);break;
    case 19: MUTATE(NLSemanticScopeView,&((NLSemanticContext*)s->f->context)->scopes[s->a_scope-1],cell->active=true);break;
    case 20: MUTATE(NLSemanticValueView,&s->after->values[s->new_payload_value-1],cell->reference.place=s->root_b);break;
    default:return 0;
    }
#undef MUTATE
    if(!memcmp(backup->at,backup->original,backup->bytes)){
        unstash(backup);return 0;
    }
    return 1;
}
int main(int argc,char **argv){
    NLSource *source=NULL,*call_source=NULL;
    NLParser *parser=NULL,*call_parser=NULL;
    NLSyntaxTree *tree=NULL,*call_tree=NULL;
    NLSemanticContext *context=NULL;
    NLCheckedFragment *owned=NULL;
    bool pass=false;
    if(argc!=2){fputs("usage: r322_observer witness.nl\n",stderr);return 2;}
    REQUIRE(nl_source_load(argv[1],&source)==NL_SOURCE_OK);
    REQUIRE(nl_parser_create(source,&parser)==NL_PARSE_OK);
    NLParseDiagnostic pd={0};
    NLParseStatus ps=nl_parser_parse_function_unit(parser,&tree,&pd);
    printf("R322_PARSER=%d\n",(int)ps);
    REQUIRE(ps==NL_PARSE_OK);
    REQUIRE(nl_semantic_create(&context)==NL_CHECK_OK);
    const NLSyntaxTree *arr[1]={tree};
    NLFunctionUnitDiagnostic fd={0};
    NLCheckStatus st=nl_semantic_register_function_unit(context,arr,1,&fd);
    printf("R322_REGISTER=%d\n",(int)st);
    REQUIRE(st==NL_CHECK_OK);
    REQUIRE(nl_source_create("main()",6,"r322_main",&call_source)==NL_SOURCE_OK);
    REQUIRE(nl_parser_create(call_source,&call_parser)==NL_PARSE_OK);
    REQUIRE(nl_parser_parse_expression_fragment(call_parser,&call_tree,&pd)==NL_PARSE_OK);
    NLCheckDiagnostic cd={0};
    st=nl_semantic_check_expression(context,call_tree,&owned,&cd);
    printf("R322_ENTRY=%d OWNED=%d\n",(int)st,owned!=NULL);
    REQUIRE(st==NL_CHECK_OK && owned!=NULL);
    Walk walk_result={0};
    REQUIRE(walk(owned,&walk_result,0));
    REQUIRE(walk_result.nc==5 && walk_result.ns==2 && walk_result.seven_count==1);
    printf("R322_PUBLIC_COUNTS closures=%zu scoped=%zu seventh=%zu\n",
           walk_result.nc,walk_result.ns,walk_result.seven_count);
    REQUIRE(all_clean(&walk_result));
    Site site={0};
    REQUIRE(prepare(&walk_result,&site));
    puts("R322_REAL_SOURCE_GENERATED_POSITIVE_OK");
    unsigned accepted=0,invalid=0,unrestored=0;
    for(unsigned case_id=0;case_id<NUM_CASES;case_id++){
        REQUIRE(all_clean(&walk_result));
        Saved backup={0};
        if(!corrupt(case_id,&site,&backup)){
            fprintf(stderr,"R322_MUTATION_BARRIER case=%u\n",case_id);
            goto done;
        }
        NLCheckStatus statuses[7]={0};
        int observed=public_statuses(&walk_result,statuses,false);
        int clean_count=0,fail_count=0;
        for(size_t j=0;j<7;j++) {
            if(statuses[j]==NL_CHECK_OK)clean_count++;
            else fail_count++;
        }
        printf("R322_CASE %02u %-54s captured=%d,%d,%d,%d,%d scoped=%d,%d rejected=%d\n",
               case_id,classes[case_id],(int)statuses[0],(int)statuses[1],
               (int)statuses[2],(int)statuses[3],(int)statuses[4],
               (int)statuses[5],(int)statuses[6],observed && fail_count>0);
        if(observed && fail_count)accepted++;
        else invalid++;
        unstash(&backup);
        if(!all_clean(&walk_result)){
            printf("R322_RESTORE_FAILED case=%u\n",case_id);unrestored++;break;
        }
        printf("R322_RESTORED case=%02u status=7/7_OK\n",case_id);
    }
    printf("R322_CORRUPTION_MATRIX proposed=%zu rejected=%u not_rejected=%u restore_failed=%u\n",
           NUM_CASES,accepted,invalid,unrestored);
    if(accepted!=NUM_CASES || invalid || unrestored)goto done;
    puts("R322_ALL_EXPECTED_CORRUPTIONS_REJECTED_AND_RESTORED");
    pass=true;
done:
    nl_checked_destroy(owned);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(call_tree);nl_parser_destroy(call_parser);nl_source_destroy(call_source);
    nl_syntax_tree_destroy(tree);nl_parser_destroy(parser);nl_source_destroy(source);
    return pass?0:1;
}
