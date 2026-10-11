/* R #314 independent PUBLIC-API owned-certificate observer.
   No private symbol, no source implementation access, no synthetic authority. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "newlang/source.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/checked.h"
#include "newlang/captured_closure.h"

typedef struct {
    const NLCheckedFragment *owner;
    NLCheckedNodeId call_id;
    NLScopedFieldCallView scoped;
    NLCheckedField field;
    NLCheckStatus status;
} CallEvidence;
static CallEvidence calls[16];
static size_t call_count, captured_count, captured_ok, depth_max;
static NLCapturedClosureView deepest_originals;
static bool deepest_present;

static const char *sc(NLCheckStatus c) {
    static const char *n[]={"OK","SEMANTIC_ERROR","SEMANTIC_UNSUPPORTED",
      "ANALYSIS_PRECISION_LIMIT","OUT_OF_MEMORY","RESOURCE_LIMIT","INTERNAL_ERROR"};
    return ((unsigned)c<7) ? n[c] : "UNRECOGNIZED";
}
static void snap(const NLSemanticContext *c, const char *tag) {
    NLSemanticSnapshot x={0};
    if(!nl_semantic_snapshot(c,&x)) {printf("%s unavailable\n",tag);return;}
    printf("%s types=%zu bindings=%zu values=%zu places=%zu domains=%zu scopes=%zu functions=%zu regions=%zu occurrences=%zu\n",
        tag,x.types,x.bindings,x.values,x.places,x.domains,x.scopes,x.functions,x.backing_regions,x.occurrences);
}
static bool eqsnap(NLSemanticSnapshot a,NLSemanticSnapshot b) {
    return a.types==b.types && a.bindings==b.bindings && a.values==b.values &&
        a.places==b.places && a.domains==b.domains && a.scopes==b.scopes &&
        a.functions==b.functions && a.last_incarnation==b.last_incarnation &&
        a.last_value_fact==b.last_value_fact && a.backing_regions==b.backing_regions &&
        a.raw_intervals==b.raw_intervals && a.occurrences==b.occurrences;
}
static void inspect(const NLCheckedFragment *f, unsigned depth) {
    if(!f || depth>35) return;
    if(depth>depth_max) depth_max=depth;
    for(NLCheckedNodeId i=1;i<=nl_checked_node_count(f);i++) {
        const NLCheckedNodeView *n=nl_checked_node_view(f,i);
        if(!n) continue;
        if(n->kind==NL_CHECKED_MATCH) {
            NLCapturedClosureView cv={0};
            if(nl_checked_captured_closure_view(f,i,&cv)) {
                captured_count++;
                if(cv.count==4) { deepest_originals=cv; deepest_present=true; }
                NLCheckStatus st=nl_checked_captured_closure_validate(f,i);
                if(st==NL_CHECK_OK) captured_ok++;
                printf("CAPTURED depth=%u match=%zu originals=%zu validator=%s releases=%zu,%zu,%zu,%zu,%zu,%zu\n",
                  depth,i,cv.count,sc(st),cv.release_worlds[0],
                  cv.release_worlds[1],cv.release_worlds[2],
                  cv.release_worlds[3],cv.release_worlds[4],cv.release_worlds[5]);
            }
            for(size_t arm=0;arm<2;arm++) inspect(nl_checked_match_arm(f,i,arm),depth+1);
        }
        if(n->kind==NL_CHECKED_IF)
            for(size_t arm=0;arm<2;arm++) inspect(nl_checked_if_arm(f,i,arm),depth+1);
        if(n->kind==NL_CHECKED_REGISTERED_CALL) {
            NLScopedFieldCallView v={0};
            if(nl_checked_scoped_field_call_view(f,i,&v)) {
                NLCheckStatus st=nl_checked_scoped_field_call_validate(f,i);
                const NLCheckedNodeView *p=nl_checked_node_view(f,v.projection);
                printf("SCOPED depth=%u call=%zu callee_nodes=%zu projection=%zu found=%d validation=%s before=%p after=%p\n",
                  depth,i,v.body?nl_checked_node_count(v.body):0,
                  v.projection,p!=NULL,sc(st),
                  (void*)v.change.before,(void*)v.change.after);
                if(p) printf(" FIELD present=%d parent=%zu child=%zu index=%zu parent-inc=%zu child-inc=%zu before=%zu after=%zu old-occ=%zu post-occ=%zu\n",
                  p->field.present,p->field.parent,p->field.child,p->field.index,
                  p->field.parent_incarnation,p->field.child_incarnation,
                  p->field.child_fact,p->field.child_post_fact,
                  p->field.payload_occurrence,p->field.post_payload_occurrence);
                if(call_count<16) {
                    CallEvidence *out=&calls[call_count++];
                    out->owner=f;out->call_id=i;out->scoped=v;out->field=p?p->field:(NLCheckedField){0};out->status=st;
                }
            }
            inspect(nl_checked_call_body(f,i),depth+1);
        }
    }
}
static bool is_live_some(const NLSemanticContext *c,NLPlaceId p,bool *some,size_t *variant) {
    NLSemanticPlaceView pl={0};NLSemanticValueView val={0};
    if(!c || !nl_semantic_place_view(c,p,&pl) || !nl_semantic_value_view(c,pl.current_value,&val))
        return false;
    *variant=val.variant;
    *some = pl.payload_occurrence!=0;
    return true;
}
static bool prove_two(void) {
    if(call_count!=2) {printf("R314_TWO_CALLS_BARRIER: observed=%zu\n",call_count);return false;}
    CallEvidence *a=&calls[0],*b=&calls[1];
    bool ok=a->status==NL_CHECK_OK && b->status==NL_CHECK_OK;
    const NLCheckedField *x=&a->field,*y=&b->field;
    if(!x->present || !y->present) {
        printf("R314_FIELD_BARRIER: projection-metadata-missing\n");return false;
    }
    NLPlaceId root=x->parent;
    NLSemanticPlaceView first_pre={0},first_post={0},second_pre={0},second_post={0};
    if(!nl_semantic_place_view(a->scoped.change.before,root,&first_pre) ||
       !nl_semantic_place_view(a->scoped.change.after,root,&first_post) ||
       !nl_semantic_place_view(b->scoped.change.before,root,&second_pre) ||
       !nl_semantic_place_view(b->scoped.change.after,root,&second_post)) {
        puts("R314_WORLD_BARRIER: four root views not available");return false;
    }
    if(deepest_present) {
        bool distinct=true;
        for(size_t i=0;i<deepest_originals.count;i++) {
            const NLCapturedOriginal *o=&deepest_originals.originals[i];
            printf("R314_ORIGINAL_ANCESTOR %zu root=%zu inc=%zu D=%zu R=%zu alloc_value=%zu domain_value=%zu\\n",
              i,o->root,o->incarnation,o->domain,o->extent.region,
              o->allocation_value,o->domain_value);
            for(size_t j=0;j<i;j++) {
                const NLCapturedOriginal *other=&deepest_originals.originals[j];
                if(o->root==other->root || o->domain==other->domain ||
                   o->extent.region==other->extent.region ||
                   o->allocation_value==other->allocation_value ||
                   o->domain_value==other->domain_value) distinct=false;
            }
        }
        bool matched=deepest_originals.originals[2].root==root &&
            deepest_originals.originals[2].domain==first_pre.governing_domain &&
            deepest_originals.originals[2].extent.region==first_pre.placement.region;
        printf("R314_ORIGINAL_DISTINCT_FOUR_ANCESTORS=%d B_MAPPED=%d\\n",distinct,matched);
        ok &= distinct && matched;
    } else { puts("R314_FIVE_ORIGINAL_CAPTURE_BARRIER");ok=false; }
    printf("R314_ROOT pre inc=%zu D=%zu R=%zu, mid inc=%zu D=%zu R=%zu, second pre inc=%zu D=%zu R=%zu, post inc=%zu D=%zu R=%zu\n",
       first_pre.incarnation,first_pre.governing_domain,first_pre.placement.region,
       first_post.incarnation,first_post.governing_domain,first_post.placement.region,
       second_pre.incarnation,second_pre.governing_domain,second_pre.placement.region,
       second_post.incarnation,second_post.governing_domain,second_post.placement.region);
    ok &= x->parent==y->parent && x->child!=y->child && x->index!=y->index &&
       x->parent_incarnation==y->parent_incarnation && first_pre.incarnation==second_post.incarnation &&
       first_pre.governing_domain!=0 && first_pre.governing_domain==first_post.governing_domain &&
       first_pre.governing_domain==second_pre.governing_domain &&
       first_pre.governing_domain==second_post.governing_domain &&
       first_pre.placement.region!=0 && first_pre.placement.region==second_post.placement.region;
    NLSemanticPlaceView ax0,ax1,ax2,ay1,ay2,ay3;
    if(!nl_semantic_place_view(a->scoped.change.before,x->child,&ax0) ||
       !nl_semantic_place_view(a->scoped.change.after,x->child,&ax1) ||
       !nl_semantic_place_view(b->scoped.change.before,x->child,&ax2) ||
       !nl_semantic_place_view(a->scoped.change.after,y->child,&ay1) ||
       !nl_semantic_place_view(b->scoped.change.before,y->child,&ay2) ||
       !nl_semantic_place_view(b->scoped.change.after,y->child,&ay3)) {
        puts("R314_FIELD_STATE_BARRIER");return false;
    }
    bool sx0=false,sx1=false,sx2=false,sy1=false,sy2=false,sy3=false;
    size_t vx0=0,vx1=0,vx2=0,vy1=0,vy2=0,vy3=0;
    if(!is_live_some(a->scoped.change.before,x->child,&sx0,&vx0) ||
       !is_live_some(a->scoped.change.after,x->child,&sx1,&vx1) ||
       !is_live_some(b->scoped.change.before,x->child,&sx2,&vx2) ||
       !is_live_some(a->scoped.change.after,y->child,&sy1,&vy1) ||
       !is_live_some(b->scoped.change.before,y->child,&sy2,&vy2) ||
       !is_live_some(b->scoped.change.after,y->child,&sy3,&vy3)) {
        puts("R314_SUM_STATE_BARRIER");return false;
    }
    printf("R314_CURRENT: prev: first-pre some=%d var=%zu first-post some=%d var=%zu second-pre some=%d var=%zu facts %zu -> %zu -> %zu\n",
        sx0,vx0,sx1,vx1,sx2,vx2,ax0.current_fact,ax1.current_fact,ax2.current_fact);
    printf("R314_CURRENT: next: first-post some=%d var=%zu second-pre some=%d var=%zu second-post some=%d var=%zu facts %zu -> %zu -> %zu\n",
        sy1,vy1,sy2,vy2,sy3,vy3,ay1.current_fact,ay2.current_fact,ay3.current_fact);
    if(first_pre.fixed_field_count>=3) {
        NLPlaceId child=first_pre.fixed_fields[2];
        NLSemanticPlaceView child_before={0},child_post={0};
        bool seen=child!=0 &&
            nl_semantic_place_view(a->scoped.change.before,child,&child_before) &&
            nl_semantic_place_view(b->scoped.change.after,child,&child_post);
        bool kept=seen && child_before.current_fact==child_post.current_fact &&
            child_before.incarnation==child_post.incarnation;
        printf("R314_B_CHILD_SIBLING_SAVED=%d place=%zu fact_before=%zu fact_after=%zu\\n",
            kept,child,child_before.current_fact,child_post.current_fact);
        ok &= kept;
    } else {puts("R314_CHILD_FIELD_BARRIER");ok=false;}
    ok &= sx0 && !sx1 && !sx2 && sy1 && sy2 && !sy3 &&
      ax1.current_fact==ax2.current_fact && ay1.current_fact==ay2.current_fact &&
      ax1.current_fact!=ax0.current_fact && ay2.current_fact!=ay3.current_fact;
    if(x->payload_occurrence) {
        NLSemanticOccurrenceView occ={0};
        bool observed=nl_semantic_occurrence_view(a->scoped.change.after,x->payload_occurrence,&occ);
        printf("R314_OLD_PREV_OCC: id=%zu observable=%d live=%d\n",x->payload_occurrence,observed,occ.live);
        ok &= observed && !occ.live;
    } else {puts("R314_OCCURRENCE_BARRIER: no original prev occurrence");ok=false;}
    if(y->payload_occurrence) {
        NLSemanticOccurrenceView occ={0};
        bool observed=nl_semantic_occurrence_view(b->scoped.change.after,y->payload_occurrence,&occ);
        printf("R314_OLD_NEXT_OCC: id=%zu observable=%d live=%d\n",y->payload_occurrence,observed,occ.live);
        ok &= observed && !occ.live;
    } else {puts("R314_OCCURRENCE_BARRIER: no original next occurrence");ok=false;}
    printf("R314_TWO_CALLS_CHRONOLOGICAL_PUBLIC_EVIDENCE=%s\n",ok?"PASS":"BARRIER");
    return ok;
}
int main(int argc,char **argv) {
    if(argc!=2) {fprintf(stderr,"usage: %s file.nl\n",argv[0]);return 2;}
    NLSource *src=NULL,*expr=NULL;NLParser *p=NULL,*ep=NULL;
    NLSyntaxTree *unit=NULL,*call=NULL;NLSemanticContext *sem=NULL;
    NLCheckedFragment *owned=NULL;
    if(nl_source_load(argv[1],&src)!=NL_SOURCE_OK || !src){puts("LOAD_BARRIER");return 3;}
    NLParseStatus ps=nl_parser_create(src,&p);
    if(ps!=NL_PARSE_OK){printf("PARSER_CREATE %d\n",ps);goto end;}
    NLParseDiagnostic pd={0};
    ps=nl_parser_parse_function_unit(p,&unit,&pd);
    printf("R314_PARSE status=%d code=%s span=[%zu,%zu]\n",ps,
      pd.diagnostic.code?pd.diagnostic.code:"NONE",pd.span.start_byte,pd.span.end_byte);
    if(ps!=NL_PARSE_OK) goto end;
    NLCheckStatus cs=nl_semantic_create(&sem);
    printf("R314_SEMANTIC_CREATE %s\n",sc(cs));
    if(cs!=NL_CHECK_OK)goto end;
    NLSemanticSnapshot before={0},after={0};
    nl_semantic_snapshot(sem,&before);
    NLFunctionUnitDiagnostic ud={0};
    const NLSyntaxTree *inputs[]={unit};
    cs=nl_semantic_register_function_unit(sem,inputs,1,&ud);
    nl_semantic_snapshot(sem,&after);
    printf("R314_REGISTER %s input_index=%zu diag=%s span=[%zu,%zu] rollback_if_reject=%d\n",
      sc(cs),ud.input_index,ud.diagnostic.diagnostic.code?ud.diagnostic.diagnostic.code:"NONE",
      ud.diagnostic.span.start_byte,ud.diagnostic.span.end_byte,
      cs==NL_CHECK_OK?-1:eqsnap(before,after));
    snap(sem,"R314_REG_CONTEXT");
    if(cs!=NL_CHECK_OK)goto end;
    if(nl_source_create("main()",6,"r314-known-entry",&expr)!=NL_SOURCE_OK || nl_parser_create(expr,&ep)!=NL_PARSE_OK){
       puts("ENTRY_PARSE_SETUP_BARRIER");goto end;
    }
    ps=nl_parser_parse_expression_fragment(ep,&call,&pd);
    printf("R314_ENTRY_PARSE status=%d diag=%s\n",ps,pd.diagnostic.code?pd.diagnostic.code:"NONE");
    if(ps!=NL_PARSE_OK)goto end;
    NLCheckDiagnostic cd={0};
    NLSemanticSnapshot cb={0},ca={0};
    nl_semantic_snapshot(sem,&cb);
    cs=nl_semantic_check_expression(sem,call,&owned,&cd);
    nl_semantic_snapshot(sem,&ca);
    printf("R314_CHECK_OWNED_MAIN status=%s diag=%s span=[%zu,%zu] owned=%d rollback_if_reject=%d\n",
      sc(cs),cd.diagnostic.code?cd.diagnostic.code:"NONE",
      cd.span.start_byte,cd.span.end_byte,owned!=NULL,
      cs==NL_CHECK_OK?-1:eqsnap(cb,ca));
    if(cs!=NL_CHECK_OK || !owned)goto end;
    inspect(owned,0);
    printf("R314_OWNED nodes=%zu observed_named_field_calls=%zu captured=%zu captured_valid=%zu max_depth=%zu\n",
      nl_checked_node_count(owned),call_count,captured_count,captured_ok,depth_max);
    bool passed=prove_two();
    printf("R314_DISPOSITION_INPUT %s\n",passed?"CHECK_PUBLIC_VALIDATOR_COMPLETENESS":"INCONCLUSIVE_PUBLIC_BARRIER");
end:
    nl_checked_destroy(owned);nl_syntax_tree_destroy(call);nl_syntax_tree_destroy(unit);
    nl_parser_destroy(ep);nl_parser_destroy(p);nl_semantic_destroy(sem);
    nl_source_destroy(expr);nl_source_destroy(src);
    return 0; /* outputs are evidence; result classifications are human-reviewed */
}
