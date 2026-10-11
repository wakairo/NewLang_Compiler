/* R307 independent public-header-only source→owned observer.
 * This host C program is NOT NewLang generated code. */
#include "newlang/captured_closure.h"
#include "newlang/parser.h"
#include "newlang/raw_storage.h"
#include "newlang/source.h"
#include "newlang/semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQ(c) do { if (!(c)) { fprintf(stderr,"R307 OBSERVER FAIL %s:%d: %s\n",__FILE__,__LINE__,#c); goto done; } } while (0)

static NLCheckedNodeId first_allocation_match(const NLCheckedFragment *f) {
    for (size_t i=1;i<=nl_checked_node_count(f);++i) {
        const NLCheckedNodeView *n=nl_checked_node_view(f,i);
        const NLCheckedNodeView *a=n ? nl_checked_node_view(f,n->initializer):NULL;
        if (n && n->kind==NL_CHECKED_MATCH && a && a->kind==NL_CHECKED_TRY_ALLOCATE_ONE) return i;
    }
    return 0;
}
static NLSymbolId symbol_in(const NLCheckedFragment *f,const NLSource *src,const NLSemanticContext *c,const char *name) {
    NLSymbolId id=nl_semantic_find_binding(c,name);
    if(id) return id;
    for (size_t i=1;i<=nl_checked_node_count(f);++i) {
        const NLCheckedNodeView *n=nl_checked_node_view(f,i);
        NLSourceView v={0};
        if (n && n->symbol && nl_source_view(src,n->name,&v) &&
            v.length==strlen(name) && memcmp(v.bytes,name,v.length)==0) return n->symbol;
    }
    return 0;
}
static int present(const NLSemanticContext *c,NLValueId allocations[5],NLValueId domains[5]) {
    NLSemanticSnapshot s;
    if(!nl_semantic_snapshot(c,&s) || s.backing_regions!=5 || s.domains!=5) return 0;
    memset(allocations,0,5*sizeof(*allocations));
    memset(domains,0,5*sizeof(*domains));
    for(size_t i=1;i<=s.values;++i) {
        NLSemanticValueView v; NLSemanticTypeView t;
        if(!nl_semantic_value_view(c,i,&v) || !nl_semantic_type_view(c,v.type,&t)) return 0;
        if(v.carrier==NL_CARRIER_ENDED) continue;
        if(t.kind==NL_TYPE_ALLOCATION) {
            if(v.allocation_region==0 || v.allocation_region>5 || allocations[v.allocation_region-1]) return 0;
            allocations[v.allocation_region-1]=i;
        }
        if(v.type==nl_semantic_domain_type(c)) {
            if(v.domain==0 || v.domain>5 || domains[v.domain-1]) return 0;
            domains[v.domain-1]=i;
        }
    }
    for(size_t i=0;i<5;++i) {
        NLSemanticBackingView b; NLSemanticDomainView d;
        if(!allocations[i] || !domains[i] || !nl_semantic_backing_view(c,i+1,&b) ||
           !nl_semantic_domain_view(c,i+1,&d) || !b.live || !d.live || d.value!=domains[i]) return 0;
    }
    return 1;
}
static int validate_owned(const NLCheckedFragment *entry,const NLSource *src,int want_none) {
    const NLCheckedFragment *f=nl_checked_call_body(entry,nl_checked_root(entry));
    if(!f) return 0;
    NLCapturedOriginal original={0};
    for (size_t i=0;i<5;++i) {
        NLCheckedNodeId m=first_allocation_match(f);
        if(!m || nl_checked_captured_closure_validate(f,m)!=NL_CHECK_OK) return 0;
        NLCapturedClosureView c;
        if(!nl_checked_captured_closure_view(f,m,&c)) return 0;
        for(size_t k=0;k<c.count;++k) if(c.originals[k].domain==3) original=c.originals[k];
        f=nl_checked_match_arm(f,m,1);
        if(!f) return 0;
    }
    if(!original.root || nl_checked_captured_change_count(f)!=6) return 0;
    NLScopedFieldCallView callview={0};
    NLCheckedNodeId call_id=0;
    for(size_t i=1;i<=nl_checked_node_count(f);++i) {
        NLScopedFieldCallView candidate={0};
        if(nl_checked_scoped_field_call_view(f,i,&candidate)) {
            if(call_id) return 0;
            call_id=i;callview=candidate;
        }
    }
    if(!call_id || nl_checked_scoped_field_call_validate(f,call_id)!=NL_CHECK_OK ||
        nl_checked_captured_change_count(callview.body)!=1) return 0;
    const NLCheckedNodeView *r=nl_checked_node_view(callview.body,callview.change.operation);
    const NLCheckedNodeView *project=nl_checked_node_view(f,callview.projection);
    if(!r || !project || r->kind!=NL_CHECKED_REPLACE || project->kind!=NL_CHECKED_FIELD_REF ||
       r->field.index!=0 || project->field.index!=0 ||
       r->field.parent!=original.root || r->field.child!=project->field.child ||
       r->field.nominal!=project->field.nominal) return 0;

    const NLSemanticContext *before=callview.change.before, *after=callview.change.after;
    NLValueId as_before[5],ds_before[5],as_after[5],ds_after[5];
    if(!present(before,as_before,ds_before) || !present(after,as_after,ds_after) ||
       memcmp(as_before,as_after,sizeof(as_before)) || memcmp(ds_before,ds_after,sizeof(ds_before)) ||
       as_before[2]!=original.allocation_value || ds_before[2]!=original.domain_value) return 0;
    NLSemanticPlaceView b0,b1,n0,n1,prev0,prev1;
    if(!nl_semantic_place_view(before,original.root,&b0) ||
       !nl_semantic_place_view(after,original.root,&b1) ||
       !nl_semantic_place_view(before,r->field.child,&n0) ||
       !nl_semantic_place_view(after,r->field.child,&n1) ||
       b0.governing_domain!=3 || b1.governing_domain!=3 ||
       b0.placement.region!=3 || b0.incarnation!=original.incarnation ||
       b0.incarnation!=b1.incarnation || n0.incarnation!=n1.incarnation ||
       n0.current_fact==n1.current_fact || b0.current_fact==b1.current_fact ||
       b0.fixed_fields[0]!=r->field.child || !b0.fixed_fields[1] ||
       !nl_semantic_place_view(before,b0.fixed_fields[1],&prev0) ||
       !nl_semantic_place_view(after,b0.fixed_fields[1],&prev1) ||
       memcmp(&prev0,&prev1,sizeof(prev0))) return 0;
    NLSemanticValueView v0,v1,payload0;
    if(!nl_semantic_value_view(before,n0.current_value,&v0) ||
       !nl_semantic_value_view(after,n1.current_value,&v1) ||
       v0.variant!=2 || v1.variant!=(want_none?1:2) ||
       n0.current_value!=r->field.old_value || n1.current_value!=r->field.new_value ||
       !nl_semantic_value_view(before,v0.sum_payload,&payload0)) return 0;
    NLSemanticPlaceView original_c={0};
    if(!nl_semantic_place_view(before,payload0.reference.place,&original_c) ||
       original_c.placement.region!=4) return 0;
    NLSemanticOccurrenceView o_before,o_after;
    NLSemanticPlaceView stale;
    if(!n0.payload_occurrence ||
       !nl_semantic_occurrence_view(before,n0.payload_occurrence,&o_before) || !o_before.live ||
       !nl_semantic_occurrence_view(after,n0.payload_occurrence,&o_after) || o_after.live ||
       !nl_semantic_place_view(after,o_before.payload_place,&stale) || stale.live ||
       stale.current_value || stale.current_fact) return 0;
    if(want_none) {
        if(n1.payload_occurrence) return 0;
    } else {
        NLSemanticOccurrenceView fresh;
        NLSemanticValueView payload1;
        NLSemanticPlaceView referent;
        if(!n1.payload_occurrence || n0.payload_occurrence==n1.payload_occurrence ||
           !nl_semantic_occurrence_view(after,n1.payload_occurrence,&fresh) || !fresh.live ||
           !nl_semantic_value_view(after,v1.sum_payload,&payload1) ||
           !nl_semantic_place_view(after,payload1.reference.place,&referent) ||
           referent.placement.region!=3) return 0;
    }
    NLSymbolId oldsym=symbol_in(f,src,before,"life_B");
    NLSymbolId newsym=symbol_in(f,src,before,"moved_d_B");
    NLSymbolId allocsym=symbol_in(f,src,before,"moved_a_B");
    NLSemanticBindingView old_binding,new_binding,alloc_binding;
    NLSemanticPlaceView old_place,new_place;
    if(!oldsym || !newsym || !allocsym ||
       !nl_semantic_binding_view(before,oldsym,&old_binding) ||
       !nl_semantic_binding_view(before,newsym,&new_binding) ||
       !nl_semantic_binding_view(before,allocsym,&alloc_binding) ||
       old_binding.availability!=NL_CONSUMED ||
       new_binding.availability!=NL_AVAILABLE || alloc_binding.availability!=NL_AVAILABLE ||
       new_binding.value!=original.domain_value || alloc_binding.value!=original.allocation_value ||
       new_binding.place==old_binding.place ||
       !nl_semantic_place_view(before,old_binding.place,&old_place) ||
       !nl_semantic_place_view(before,new_binding.place,&new_place) ||
       old_place.live || new_place.current_value!=original.domain_value) return 0;

    size_t matching_loan=0, older_loans=0;
    for(size_t i=1;i<=nl_checked_node_count(f);++i) {
        const NLCheckedNodeView *n=nl_checked_node_view(f,i);
        if(!n || n->kind!=NL_CHECKED_LOAN_HEADER || n->loan.domain!=3) continue;
        if(n->loan.scope==project->reference_result.scope) {
            if(n->loan.place!=new_binding.place || n->loan.incarnation!=new_place.incarnation ||
               n->loan.is_exclusive) return 0;
            ++matching_loan;
        } else if(n->loan.scope<project->reference_result.scope) {
            NLSemanticScopeView s;
            if(!nl_semantic_scope_view(before,n->loan.scope,&s) || s.active) return 0;
            ++older_loans;
        }
    }
    NLSemanticScopeView s_before,s_final;
    if(matching_loan!=1 || older_loans!=2 ||
       !nl_semantic_scope_view(before,project->reference_result.scope,&s_before) || !s_before.active ||
       !nl_semantic_scope_view(nl_checked_context(f),project->reference_result.scope,&s_final) || s_final.active) return 0;

    const NLSemanticContext *last=nl_checked_context(f);
    NLSemanticDomainView ended_d;
    NLSemanticBackingView ended_b;
    if(!nl_semantic_domain_view(last,3,&ended_d) || ended_d.live ||
       !nl_semantic_backing_view(last,3,&ended_b) || ended_b.live) return 0;
    size_t releases=0;int seen[5]={0};
    for(size_t i=1;i<=nl_checked_node_count(f);++i) {
        const NLCheckedNodeView *n=nl_checked_node_view(f,i);
        if(!n || n->kind!=NL_CHECKED_DEALLOCATE) continue;
        const NLCheckedNodeView *a=nl_checked_node_view(f,n->first_argument);
        const NLCheckedNodeView *s=a?nl_checked_node_view(f,a->next_argument):NULL;
        NLSemanticValueView av,sv;
        if(!a || !s || a->result_count!=1 || s->result_count!=1 ||
           !nl_semantic_value_view(last,a->results[0].value,&av) ||
           !nl_semantic_value_view(last,s->results[0].value,&sv) ||
           av.allocation_region<1 || av.allocation_region>5 ||
           av.allocation_region!=sv.occupancy.region ||
           sv.occupancy.start || !sv.occupancy.length || seen[av.allocation_region-1]++) return 0;
        releases++;
    }
    if(releases!=5) return 0;
    printf("{\"R\":\"307\",\"original_B_root\":%zu,\"incarnation\":%zu,\"domain_id\":3,"
           "\"allocation_value\":%zu,\"domain_value\":%zu,\"old_D_place\":%zu,"
           "\"reloan_D_place\":%zu,\"projection_nominal\":%zu,\"projection_index\":0,"
           "\"field_fact_before\":%zu,\"field_fact_after\":%zu,"
           "\"old_occurrence\":%zu,\"new_occurrence\":%zu,"
           "\"old_payload_ended\":true,\"prev_unchanged\":true,"
           "\"source_owned_validators\":true,\"five_matched_frees\":true,"
           "\"new_some\":%s,\"native_execution\":false}\n",
           original.root, original.incarnation, original.allocation_value,
           original.domain_value,old_binding.place,new_binding.place,
           r->field.nominal,n0.current_fact,n1.current_fact,
           n0.payload_occurrence,n1.payload_occurrence,want_none?"false":"true");
    return 1;
}
int main(int argc,char **argv) {
    if(argc!=3 || (strcmp(argv[2],"none") && strcmp(argv[2],"some_b"))) return 2;
    const int want_none=!strcmp(argv[2],"none");
    NLSource *src=NULL,*entrysrc=NULL;
    NLParser *parser=NULL,*entryparser=NULL;
    NLSyntaxTree *unit=NULL,*expr=NULL;
    NLSemanticContext *ctx=NULL;
    NLCheckedFragment *entry=NULL;
    int ok=0;
    REQ(nl_source_load(argv[1],&src)==NL_SOURCE_OK);
    REQ(nl_parser_create(src,&parser)==NL_PARSE_OK);
    REQ(nl_parser_parse_function_unit(parser,&unit,NULL)==NL_PARSE_OK);
    REQ(nl_semantic_create(&ctx)==NL_CHECK_OK);
    const NLSyntaxTree *units[]={unit};
    NLFunctionUnitDiagnostic diag={0};NLCheckDiagnostic entrydiag={0};
    REQ(nl_semantic_register_function_unit(ctx,units,1,&diag)==NL_CHECK_OK);
    REQ(nl_source_create("main()",6,"<R307-entry>",&entrysrc)==NL_SOURCE_OK);
    REQ(nl_parser_create(entrysrc,&entryparser)==NL_PARSE_OK);
    REQ(nl_parser_parse_expression_fragment(entryparser,&expr,NULL)==NL_PARSE_OK);
    REQ(nl_semantic_check_expression(ctx,expr,&entry,&entrydiag)==NL_CHECK_OK && entry);
    REQ(validate_owned(entry,src,want_none));
    ok=1;
done:
    nl_checked_destroy(entry);
    nl_semantic_destroy(ctx);
    nl_syntax_tree_destroy(expr);
    nl_parser_destroy(entryparser);
    nl_source_destroy(entrysrc);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(src);
    return ok?0:1;
}
