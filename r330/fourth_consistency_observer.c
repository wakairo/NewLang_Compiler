/*
 * Track R / Issue 330: independent NON-BLIND test-only consistency probe.
 * Genuine source-generated positive; controlled in-bounds owned mutations only.
 * PRIVATE layout solely for negative perturbations. Seven PUBLIC read-only
 * validators alone decide success/refusal/recovery; no private validator oracle.
 * Manifest r330/PREREGISTERED_CASES.md was committed before any CI trial.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "newlang/parser.h"
#include "newlang/captured_closure.h"
#include "semantic_internal.h"

#define MUST(c) do { if (!(c)) { fprintf(stderr,"R330_PRECONDITION line=%d: %s\n",__LINE__,#c); goto finish; } } while (0)

typedef struct {
    NLSource *source, *call_source;
    NLParser *parser, *call_parser;
    NLSyntaxTree *unit, *call_ast;
    NLSemanticContext *ctx;
    NLCheckedFragment *owned;
} Session;
typedef struct { const NLCheckedFragment *fragment; NLCheckedNodeId node; } Validator;
typedef struct {
    NLCheckedFragment *f;
    Validator closure[5], scoped[2];
    NLCapturedChange *a, *c;
    NLCheckedNodeView *op;
    NLSemanticContext *before, *after, *previous;
    NLPlaceId root[6], c_prev, c_sibling, b_other;
    NLOccurrenceId old_occ, new_occ;
    NLValueId old_ptr, new_ptr;
    NLScopeId c_scope;
} Evidence;
typedef struct { void *address; size_t length; unsigned char *original; } Undo;

static void destroy_session(Session *s) {
    nl_checked_destroy(s->owned);
    nl_semantic_destroy(s->ctx);
    nl_syntax_tree_destroy(s->call_ast);
    nl_syntax_tree_destroy(s->unit);
    nl_parser_destroy(s->call_parser);
    nl_parser_destroy(s->parser);
    nl_source_destroy(s->call_source);
    nl_source_destroy(s->source);
}
static bool source_to_owned(Session *s, const char *path) {
    NLParseDiagnostic parse={0};
    NLFunctionUnitDiagnostic fd={0};
    NLCheckDiagnostic cd={0};
    const NLSyntaxTree *units[1];
    NLParseStatus ps;
    NLCheckStatus st;
    if(nl_source_load(path,&s->source)!=NL_SOURCE_OK ||
       nl_parser_create(s->source,&s->parser)!=NL_PARSE_OK) return false;
    ps=nl_parser_parse_function_unit(s->parser,&s->unit,&parse);
    if(ps!=NL_PARSE_OK) {
        nl_parse_diagnostic_render(stderr,s->source,&parse);
        return false;
    }
    if(nl_semantic_create(&s->ctx)!=NL_CHECK_OK) return false;
    units[0]=s->unit;
    st=nl_semantic_register_function_unit(s->ctx,units,1,&fd);
    if(st!=NL_CHECK_OK) {
        nl_check_diagnostic_render(stderr,s->source,&fd.diagnostic);
        return false;
    }
    if(nl_source_create("main()",6,"<r330-main>",&s->call_source)!=NL_SOURCE_OK ||
       nl_parser_create(s->call_source,&s->call_parser)!=NL_PARSE_OK ||
       nl_parser_parse_expression_fragment(s->call_parser,&s->call_ast,NULL)!=NL_PARSE_OK) return false;
    st=nl_semantic_check_expression(s->ctx,s->call_ast,&s->owned,&cd);
    if(st!=NL_CHECK_OK) nl_check_diagnostic_render(stderr,s->call_source,&cd);
    printf("R330_SOURCE parser=0 register=0 main=%d owned=%d\n",(int)st,s->owned!=NULL);
    return st==NL_CHECK_OK && s->owned && nl_checked_node_count(s->owned);
}
static NLCheckedNodeId allocation_match(const NLCheckedFragment *f) {
    for(NLCheckedNodeId id=1;id<=nl_checked_node_count(f);id++) {
        const NLCheckedNodeView *v=nl_checked_node_view(f,id);
        if(v && v->kind==NL_CHECKED_MATCH) {
            const NLCheckedNodeView *p=nl_checked_node_view(f,v->initializer);
            if(p && p->kind==NL_CHECKED_TRY_ALLOCATE_ONE) return id;
        }
    }
    return 0;
}
static bool find_original(const NLSemanticContext *ctx, size_t region, NLPlaceId *id) {
    NLSemanticSnapshot snap;
    if(!nl_semantic_snapshot(ctx,&snap)) return false;
    *id=0;
    for(NLPlaceId i=1;i<=snap.places;i++) {
        NLSemanticPlaceView p;
        if(!nl_semantic_place_view(ctx,i,&p)) return false;
        if(p.live && p.independent_root && p.fixed_field_count==4 &&
           p.placement.region==region && p.governing_domain==region) {
            if(*id) return false;
            *id=i;
        }
    }
    return *id!=0;
}
static bool target_region(const NLSemanticContext *ctx, NLPlaceId field, size_t region) {
    NLSemanticPlaceView p, dest;
    NLSemanticValueView v,ptr;
    if(!nl_semantic_place_view(ctx,field,&p) ||
       !nl_semantic_value_view(ctx,p.current_value,&v)) return false;
    if(!region) return v.variant==1 && !p.payload_occurrence;
    if(v.variant!=2 || !v.sum_payload || !p.payload_occurrence ||
       !nl_semantic_value_view(ctx,v.sum_payload,&ptr) ||
       !nl_semantic_place_view(ctx,ptr.reference.place,&dest)) return false;
    return dest.live && dest.placement.region==region &&
           ptr.reference.provenance==NL_PROVENANCE_VALID;
}
static bool inventory(const NLSemanticContext *ctx) {
    NLSemanticSnapshot snap;
    NLValueId av[5]={0},dv[5]={0};
    if(!nl_semantic_snapshot(ctx,&snap) ||
       snap.backing_regions!=5 || snap.domains!=5) return false;
    for(NLValueId id=1;id<=snap.values;id++) {
        NLSemanticValueView v;
        NLSemanticTypeView t;
        if(!nl_semantic_value_view(ctx,id,&v) ||
           !nl_semantic_type_view(ctx,v.type,&t)) return false;
        if(v.carrier==NL_CARRIER_ENDED) continue;
        if(t.kind==NL_TYPE_ALLOCATION) {
            if(v.allocation_region<1 || v.allocation_region>5 ||
               av[v.allocation_region-1]) return false;
            av[v.allocation_region-1]=id;
        }
        if(v.type==nl_semantic_domain_type(ctx)) {
            if(v.domain<1 || v.domain>5 || dv[v.domain-1]) return false;
            dv[v.domain-1]=id;
        }
    }
    for(size_t r=0;r<5;r++) if(!av[r] || !dv[r]) return false;
    printf("R330_ORIGINAL_AUTHORITY allocation=%zu,%zu,%zu,%zu,%zu domains=%zu,%zu,%zu,%zu,%zu\n",
           av[0],av[1],av[2],av[3],av[4],dv[0],dv[1],dv[2],dv[3],dv[4]);
    return true;
}
static bool contiguous_worlds(const NLSemanticContext *left,
                              const NLSemanticContext *right) {
    for(size_t r=1;r<=5;r++) {
        NLPlaceId i,j;
        NLSemanticPlaceView a,b;
        if(!find_original(left,r,&i) || !find_original(right,r,&j) ||
           i!=j || !nl_semantic_place_view(left,i,&a) ||
           !nl_semantic_place_view(right,j,&b) ||
           a.incarnation!=b.incarnation || a.current_fact!=b.current_fact) return false;
        for(size_t k=0;k<4;k++) {
            NLSemanticPlaceView fa,fb;
            if(a.fixed_fields[k]!=b.fixed_fields[k] ||
               !nl_semantic_place_view(left,a.fixed_fields[k],&fa) ||
               !nl_semantic_place_view(right,b.fixed_fields[k],&fb) ||
               fa.incarnation!=fb.incarnation || fa.current_fact!=fb.current_fact ||
               fa.current_value!=fb.current_value ||
               fa.payload_occurrence!=fb.payload_occurrence) return false;
        }
    }
    return true;
}
static bool validators(const Evidence *e, NLCheckStatus s[7]) {
    for(size_t i=0;i<5;i++)
        s[i]=nl_checked_captured_closure_validate(e->closure[i].fragment,e->closure[i].node);
    for(size_t i=0;i<2;i++)
        s[5+i]=nl_checked_scoped_field_call_validate(e->scoped[i].fragment,e->scoped[i].node);
    return true;
}
static bool statuses_all_zero(const NLCheckStatus s[7]) {
    for(size_t i=0;i<7;i++) if(s[i]!=NL_CHECK_OK) return false;
    return true;
}
static bool statuses_refuse(const NLCheckStatus s[7]) {
    for(size_t i=0;i<7;i++) if(s[i]!=NL_CHECK_OK) return true;
    return false;
}
static void log_status(const char *label,unsigned id,const NLCheckStatus s[7]) {
    printf("R330_%s id=%02u closure=[%d,%d,%d,%d,%d] scoped=[%d,%d]\n",
           label,id,(int)s[0],(int)s[1],(int)s[2],(int)s[3],
           (int)s[4],(int)s[5],(int)s[6]);
}
static bool bind_evidence(Session *s, Evidence *e) {
    memset(e,0,sizeof(*e));
    NLCheckedFragment *f=(NLCheckedFragment*)nl_checked_call_body(s->owned,nl_checked_root(s->owned));
    if(!f) return false;
    for(size_t i=0;i<5;i++) {
        NLCheckedNodeId match=allocation_match(f);
        NLCapturedClosureView view;
        if(!match || !nl_checked_captured_closure_view(f,match,&view) ||
           view.count!=i) return false;
        for(size_t world=i;world<=5;world++) if(view.release_worlds[world]!=1) return false;
        e->closure[i]=(Validator){f,match};
        f=(NLCheckedFragment*)nl_checked_match_arm(f,match,1);
        if(!f) return false;
    }
    e->f=f;
    if(nl_checked_captured_change_count(f)!=8 || f->field_change_count!=8 ||
       !f->field_changes[6] || !f->field_changes[7]) return false;
    size_t n=0;
    for(NLCheckedNodeId id=1;id<=nl_checked_node_count(f);id++) {
        NLScopedFieldCallView v={0};
        if(!nl_checked_scoped_field_call_view(f,id,&v)) continue;
        if(n==2 || !v.body || !v.change.before || !v.change.after) return false;
        e->scoped[n++]=(Validator){f,id};
    }
    if(n!=2) return false;
    e->a=f->field_changes[6];
    e->c=f->field_changes[7];
    if(!e->a->after || !e->a->before || !e->c->before || !e->c->after ||
       !e->c->node || e->c->node>f->count ||
       e->a->node>=e->c->node) return false;
    e->op=&f->nodes[e->c->node-1];
    e->before=e->c->before; e->after=e->c->after; e->previous=e->a->after;
    if(e->op->kind!=NL_CHECKED_REPLACE ||
       e->op->field.index!=1 ||
       e->op->field.parent==0 || e->op->field.child==0 ||
       e->op->field.old_value==0 || e->op->field.new_value==0 ||
       e->op->field.old_value>e->before->value_count ||
       e->op->field.new_value>e->before->value_count ||
       !contiguous_worlds(e->previous,e->before)) return false;
    for(size_t r=1;r<=5;r++)
        if(!find_original(e->before,r,&e->root[r])) return false;
    if(e->op->field.parent!=e->root[4]) return false;
    NLSemanticPlaceView c0,c1,field0,field1;
    if(!nl_semantic_place_view(e->before,e->root[4],&c0) ||
       !nl_semantic_place_view(e->after,e->root[4],&c1) ||
       c1.fixed_fields[1]!=c0.fixed_fields[1] ||
       e->op->field.child!=c0.fixed_fields[1] ||
       !nl_semantic_place_view(e->before,c0.fixed_fields[1],&field0) ||
       !nl_semantic_place_view(e->after,c0.fixed_fields[1],&field1) ||
       field0.current_fact==field1.current_fact ||
       !field0.payload_occurrence || !field1.payload_occurrence ||
       field0.payload_occurrence==field1.payload_occurrence) return false;
    e->c_prev=c0.fixed_fields[1];
    e->c_sibling=c0.fixed_fields[0];
    NLSemanticPlaceView b;
    if(!nl_semantic_place_view(e->before,e->root[3],&b)) return false;
    e->b_other=b.fixed_fields[2];
    e->old_occ=field0.payload_occurrence;
    e->new_occ=field1.payload_occurrence;
    if(e->old_occ>e->after->occurrence_count ||
       e->new_occ>e->after->occurrence_count) return false;
    NLSemanticOccurrenceView o0,ended,fresh;
    if(!nl_semantic_occurrence_view(e->before,e->old_occ,&o0) ||
       !nl_semantic_occurrence_view(e->after,e->old_occ,&ended) ||
       !nl_semantic_occurrence_view(e->after,e->new_occ,&fresh) ||
       !o0.live || ended.live || !fresh.live) return false;
    NLSemanticValueView old,newv,oldptr,newptr;
    if(!nl_semantic_value_view(e->before,e->op->field.old_value,&old) ||
       !nl_semantic_value_view(e->before,e->op->field.new_value,&newv) ||
       old.variant!=2 || newv.variant!=2 || !old.sum_payload || !newv.sum_payload ||
       old.sum_payload>e->before->value_count ||
       newv.sum_payload>e->before->value_count ||
       !nl_semantic_value_view(e->before,old.sum_payload,&oldptr) ||
       !nl_semantic_value_view(e->before,newv.sum_payload,&newptr) ||
       oldptr.reference.place!=e->root[3] ||
       newptr.reference.place!=e->root[2]) return false;
    e->old_ptr=old.sum_payload; e->new_ptr=newv.sum_payload;
    if(!target_region(e->before,e->c_prev,3) ||
       !target_region(e->after,e->c_prev,2) ||
       !inventory(e->before) || !inventory(e->after)) return false;
    const NLCheckedNodeView *projection=nl_checked_node_view(f,e->op->first_argument);
    NLSemanticScopeView sc;
    if(!projection || projection->kind!=NL_CHECKED_FIELD_REF ||
       !projection->reference_result.scope ||
       !nl_semantic_scope_view(f->context,projection->reference_result.scope,&sc) ||
       sc.active) return false;
    e->c_scope=projection->reference_result.scope;
    if(e->c_scope>f->context->scope_count ||
       e->root[2]>e->before->place_count ||
       e->root[3]>e->before->place_count ||
       e->root[4]>e->after->place_count ||
       e->c_prev>e->after->place_count ||
       e->c_sibling>e->after->place_count ||
       e->old_ptr>e->before->value_count ||
       e->new_ptr>e->before->value_count) return false;
    printf("R330_REAL_SOURCE fourth_operation=%zu third_operation=%zu scope=%zu rootC=%zu rootA=%zu rootB=%zu Cprev=%zu old_occ=%zu new_occ=%zu originalC_region=4 originalC_domain=4 third_after_fourth_before_5roots_20fields=OK\n",
           e->c->node,e->a->node,e->c_scope,e->root[4],e->root[2],
           e->root[3],e->c_prev,e->old_occ,e->new_occ);
    return true;
}
static bool save(Undo *u,void *ptr,size_t n) {
    if(!ptr || !n || u->original) return false;
    u->original=malloc(n);
    if(!u->original) return false;
    u->address=ptr;u->length=n;
    memcpy(u->original,ptr,n);
    return true;
}
static void revert(Undo *u) {
    if(u->original) {
        memcpy(u->address,u->original,u->length);
        free(u->original);
        memset(u,0,sizeof(*u));
    }
}
static bool changed(const Undo *u) {
    return u->original && memcmp(u->address,u->original,u->length)!=0;
}
static bool perturb(unsigned k,Evidence *e,Undo *one,Undo *two) {
#define EDIT(T,location,expr) do { T *v=(location); if(!save(one,v,sizeof(*v))) return false; expr; } while(0)
    switch(k) {
    case 0: EDIT(NLCheckedFragment,e->f,v->field_change_count=7); break;
    case 1: EDIT(NLCapturedChange,e->c,v->node=e->a->node); break;
    case 2: {
        NLCheckedNodeId a=e->a->node,c=e->c->node;
        EDIT(NLCapturedChange,e->c,v->node=a);
        if(!save(two,e->a,sizeof(*e->a))) return false;
        e->a->node=c;
        break;
    }
    case 3: EDIT(NLCheckedNodeView,e->op,v->kind=NL_CHECKED_STATEMENT); break;
    case 4: EDIT(NLCheckedNodeView,e->op,v->field.parent=e->root[2]); break;
    case 5: EDIT(NLCheckedNodeView,e->op,v->field.child=e->c_sibling); break;
    case 6: EDIT(NLCheckedNodeView,e->op,v->field.index=0); break;
    case 7: EDIT(NLCheckedNodeView,e->op,v->field.old_value=v->field.new_value); break;
    case 8: EDIT(NLCheckedNodeView,e->op,v->field.new_value=v->field.old_value); break;
    case 9: EDIT(NLSemanticPlaceView,&e->before->places[e->root[4]-1],v->placement.region=2); break;
    case 10: EDIT(NLSemanticPlaceView,&e->before->places[e->root[4]-1],v->governing_domain=2); break;
    case 11: EDIT(NLSemanticPlaceView,&e->after->places[e->root[4]-1],v->governing_domain=2); break;
    case 12: EDIT(NLSemanticPlaceView,&e->before->places[e->c_prev-1],v->current_fact++); break;
    case 13: EDIT(NLSemanticPlaceView,&e->after->places[e->c_prev-1],
                  v->current_fact=e->before->places[e->c_prev-1].current_fact); break;
    case 14: EDIT(NLSemanticValueView,&e->before->values[e->old_ptr-1],v->reference.place=e->root[4]); break;
    case 15: EDIT(NLSemanticValueView,&e->before->values[e->new_ptr-1],v->reference.place=e->root[3]); break;
    case 16: EDIT(NLSemanticOccurrenceView,&e->after->occurrences[e->old_occ-1],v->live=true); break;
    case 17: EDIT(NLSemanticOccurrenceView,&e->after->occurrences[e->new_occ-1],v->live=false); break;
    case 18: EDIT(NLSemanticPlaceView,&e->after->places[e->c_prev-1],v->payload_occurrence=e->old_occ); break;
    case 19: {
        if(e->previous==e->before || e->root[3]>e->previous->place_count) return false;
        EDIT(NLSemanticPlaceView,&e->previous->places[e->root[3]-1],v->current_fact++);
        if(!save(two,&e->before->places[e->root[3]-1],
                 sizeof(e->before->places[e->root[3]-1]))) return false;
        e->before->places[e->root[3]-1].current_fact=
            e->previous->places[e->root[3]-1].current_fact;
        break;
    }
    case 20: EDIT(NLSemanticScopeView,
                  &((NLSemanticContext*)e->f->context)->scopes[e->c_scope-1],
                  v->active=true); break;
    case 21: EDIT(NLSemanticPlaceView,&e->after->places[e->c_sibling-1],v->current_fact++); break;
    case 22: EDIT(NLCapturedChange,e->c,
                  v->before=e->a->before;v->before_origin=e->a->before); break;
    default: return false;
    }
#undef EDIT
    return changed(one) && (!two->original || changed(two));
}
int main(int argc,char **argv) {
    Session session={0};
    Evidence evidence={0};
    bool ok=false;
    unsigned refused=0, accepted=0, rest_fail=0;
    if(argc!=2) {fprintf(stderr,"usage: r330-observer r330/independent_calibrated.nl\n");return 2;}
    MUST(source_to_owned(&session,argv[1]));
    MUST(bind_evidence(&session,&evidence));
    NLCheckStatus baseline[7];
    validators(&evidence,baseline);
    log_status("BASELINE",99,baseline);
    MUST(statuses_all_zero(baseline));
    puts("R330_PUBLIC_BASELINE_5_CAPTURED_2_SCOPED_OK");
    for(unsigned k=0;k<23;k++) {
        Undo one={0},two={0};
        NLCheckStatus mutant[7],restored[7];
        bool did_mutate=perturb(k,&evidence,&one,&two);
        if(!did_mutate) {
            printf("R330_UNSAFE_OR_NO_CHANGE id=%02u\n",k);
            revert(&two);revert(&one);
            goto finish;
        }
        validators(&evidence,mutant);
        log_status("MUTATED",k,mutant);
        bool rejected=statuses_refuse(mutant);
        if(rejected) refused++; else accepted++;
        revert(&two);revert(&one);
        validators(&evidence,restored);
        log_status("RESTORED",k,restored);
        if(!statuses_all_zero(restored)) rest_fail++;
        printf("R330_CASE id=%02u rejected=%u restored7=%u\n",
               k,rejected?1u:0u,statuses_all_zero(restored)?1u:0u);
        if(!rejected || !statuses_all_zero(restored)) {
            /* Fail closed while retaining precise case statuses in remote logs. */
            printf("R330_FIRST_FAIL id=%02u reason=%s\n",k,!rejected?"publicly-accepted":"restoration");
            goto finish;
        }
    }
    printf("R330_MATRIX proposed=23 rejected=%u publicly_accepted=%u restore_failed=%u\n",
           refused,accepted,rest_fail);
    MUST(refused==23 && accepted==0 && rest_fail==0);
    puts("R330_REAL_ARTIFACT_PUBLIC_VALIDATOR_ORACLE_PASS");
    ok=true;
finish:
    if(!ok)
        fprintf(stderr,"R330_RESULT_FAIL_CLOSED refused=%u accepted=%u restoration_fail=%u\n",
                refused,accepted,rest_fail);
    destroy_session(&session);
    return ok?0:1;
}
