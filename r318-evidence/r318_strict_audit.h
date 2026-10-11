/* R318 independent public fact audit. Source and first observer originate in R317,
 * not from P315; P-only public API knowledge is calibration, not evidence. */
#ifndef R318_STRICT_AUDIT_H
#define R318_STRICT_AUDIT_H
#define MUST(q) do { if (!(q)) { printf("R318_FACT_FAIL line=%d expression=%s\n",__LINE__,#q); return false; } } while (0)
typedef struct { NLValueId alloc[5], domain[5]; NLPlaceId roots[5]; } R318Inventory;
static bool r318_inventory(const NLSemanticContext *c, R318Inventory *v) {
    NLSemanticSnapshot s={0};
    MUST(nl_semantic_snapshot(c,&s) && s.backing_regions==5 && s.domains==5);
    memset(v,0,sizeof(*v));
    for(size_t i=1;i<=s.values;i++) {
        NLSemanticValueView val={0};
        NLSemanticTypeView type={0};
        MUST(nl_semantic_value_view(c,i,&val) && nl_semantic_type_view(c,val.type,&type));
        if(val.carrier==NL_CARRIER_ENDED) continue;
        if(type.kind==NL_TYPE_ALLOCATION) {
            MUST(val.allocation_region>=1 && val.allocation_region<=5);
            MUST(v->alloc[val.allocation_region-1]==0);
            v->alloc[val.allocation_region-1]=i;
        }
        if(val.type==nl_semantic_domain_type(c)) {
            MUST(val.domain>=1 && val.domain<=5);
            MUST(v->domain[val.domain-1]==0);
            v->domain[val.domain-1]=i;
        }
    }
    for(size_t i=1;i<=s.places;i++) {
        NLSemanticPlaceView p={0};
        MUST(nl_semantic_place_view(c,i,&p));
        if(!p.live || !p.placement.region || p.fixed_field_count!=4) continue;
        MUST(p.placement.region<=5 && v->roots[p.placement.region-1]==0);
        MUST(p.governing_domain==p.placement.region);
        v->roots[p.placement.region-1]=i;
    }
    for(size_t j=0;j<5;j++) {
        NLSemanticDomainView d={0};
        NLSemanticBackingView backing={0};
        MUST(v->alloc[j] && v->domain[j] && v->roots[j]);
        MUST(nl_semantic_domain_view(c,j+1,&d) && d.live && d.value==v->domain[j]);
        MUST(nl_semantic_backing_view(c,j+1,&backing) && backing.live);
    }
    return true;
}
static bool r318_equal_inventory(R318Inventory a,R318Inventory b) {
    return !memcmp(&a,&b,sizeof(a));
}
static bool r318_sum_target(const NLSemanticContext *ctx, NLPlaceId place, size_t expected_region) {
    NLSemanticPlaceView p={0},target={0};
    NLSemanticValueView sum={0},token={0};
    MUST(nl_semantic_place_view(ctx,place,&p) && p.live);
    MUST(nl_semantic_value_view(ctx,p.current_value,&sum));
    if(!expected_region) { MUST(sum.variant==1 && !p.payload_occurrence); return true; }
    MUST(sum.variant==2 && p.payload_occurrence && sum.sum_payload);
    MUST(nl_semantic_value_view(ctx,sum.sum_payload,&token));
    MUST(token.reference.provenance==NL_PROVENANCE_VALID);
    MUST(nl_semantic_place_view(ctx,token.reference.place,&target));
    MUST(target.live && target.placement.region==expected_region);
    return true;
}
static bool r318_call_event(const NLCheckedFragment *f, NLCheckedNodeId id, NLScopedFieldCallView *out, size_t seq) {
    MUST(nl_checked_scoped_field_call_view(f,id,out));
    MUST(nl_checked_scoped_field_call_validate(f,id)==NL_CHECK_OK);
    MUST(nl_checked_captured_change_count(out->body)==1);
    const NLCheckedNodeView *caller=nl_checked_node_view(f,id);
    const NLCheckedNodeView *projection=nl_checked_node_view(f,out->projection);
    const NLCheckedNodeView *op=nl_checked_node_view(out->body,out->change.operation);
    MUST(caller && projection && op && op->kind==NL_CHECKED_REPLACE &&
         projection->kind==NL_CHECKED_FIELD_REF);
    size_t field_index=seq==0?1:0;
    MUST(projection->field.index==field_index && op->field.index==field_index &&
         op->field.child==projection->field.child);
    const NLSemanticContext *before=out->change.before,*after=out->change.after;
    R318Inventory x,y;
    MUST(r318_inventory(before,&x) && r318_inventory(after,&y));
    MUST(r318_equal_inventory(x,y));
    NLSemanticPlaceView root[2],field[2];
    MUST(nl_semantic_place_view(before,op->field.parent,&root[0]) &&
         nl_semantic_place_view(after,op->field.parent,&root[1]) &&
         nl_semantic_place_view(before,op->field.child,&field[0]) &&
         nl_semantic_place_view(after,op->field.child,&field[1]));
    MUST(op->field.parent==x.roots[2] && root[0].governing_domain==3 &&
         root[1].governing_domain==3 && root[0].incarnation==root[1].incarnation);
    MUST(root[0].fixed_fields[field_index]==op->field.child &&
         root[0].current_fact!=root[1].current_fact &&
         field[0].current_fact==op->field.child_fact &&
         field[1].current_fact==op->field.child_post_fact &&
         field[0].current_fact!=field[1].current_fact);
    MUST(r318_sum_target(before,op->field.child,seq==0?2:4));
    MUST(r318_sum_target(after,op->field.child,0));
    NLSemanticOccurrenceView old={0},ended={0};
    MUST(field[0].payload_occurrence && !field[1].payload_occurrence);
    MUST(nl_semantic_occurrence_view(before,field[0].payload_occurrence,&old) && old.live &&
         nl_semantic_occurrence_view(after,field[0].payload_occurrence,&ended) && !ended.live);
    NLSemanticPlaceView old_payload={0},lost_payload={0};
    MUST(nl_semantic_place_view(before,old.payload_place,&old_payload) && old_payload.live &&
         nl_semantic_place_view(after,old.payload_place,&lost_payload) && !lost_payload.live);
    for(size_t j=0;j<4;j++){
        if(j==field_index)continue;
        NLSemanticPlaceView a,b;
        MUST(nl_semantic_place_view(before,root[0].fixed_fields[j],&a) &&
             nl_semantic_place_view(after,root[0].fixed_fields[j],&b) &&
             !memcmp(&a,&b,sizeof(a)));
    }
    const NLScopeId scope=projection->reference_result.scope;
    NLSemanticScopeView active={0},ended_scope={0};
    MUST(scope && nl_semantic_scope_view(before,scope,&active) && active.active &&
         nl_semantic_scope_view(nl_checked_context(f),scope,&ended_scope) && !ended_scope.active);
    printf("R318_B_HELPER_%zu_OK call=%zu field_index=%zu old_target_region=%zu old_occ=%zu old_ended=1 new_None=1 original_B_root=%zu domain=3\n",
           seq+1,id,field_index,seq==0?2u:4u,field[0].payload_occurrence,op->field.parent);
    return true;
}
static bool r318_direct(const NLCheckedFragment *f,const NLScopedFieldCallView *second,
                        NLCheckedNodeId second_id){
    MUST(nl_checked_captured_change_count(f)==7);
    NLCapturedChangeView step={0},setup={0};
    MUST(nl_checked_captured_change_view(f,6,&step) &&
         nl_checked_captured_change_view(f,1,&setup));
    const NLCheckedNodeView *write=nl_checked_node_view(f,step.operation);
    const NLCheckedNodeView *first=nl_checked_node_view(f,setup.operation);
    const NLCheckedNodeView *projection=write?nl_checked_node_view(f,write->first_argument):NULL;
    MUST(write && first && projection &&
         write->kind==NL_CHECKED_REPLACE &&
         projection->kind==NL_CHECKED_FIELD_REF &&
         write->field.index==0 &&
         write->field.parent==first->field.parent &&
         write->field.child==first->field.child &&
         write->field.old_value==first->field.new_value &&
         second_id<step.operation);
    NLSourceView src={0};
    MUST(nl_source_view(nl_checked_source(f),write->span,&src));
    static const char needle[]="replace(a_next@next";
    bool source_exact=false;
    for(size_t i=0;i+sizeof(needle)-1<=src.length;i++) {
        if(!memcmp(src.bytes+i,needle,sizeof(needle)-1)) source_exact=true;
    }
    MUST(source_exact);
    const NLSemanticContext *before=step.before,*after=step.after;
    R318Inventory a,b,preceding;
    MUST(r318_inventory(before,&a) && r318_inventory(after,&b) &&
         r318_inventory(second->change.after,&preceding));
    MUST(r318_equal_inventory(a,b) && r318_equal_inventory(a,preceding));
    MUST(write->field.parent==a.roots[1]);
    NLSemanticPlaceView root[2],field[2];
    MUST(nl_semantic_place_view(before,a.roots[1],&root[0]) &&
         nl_semantic_place_view(after,a.roots[1],&root[1]) &&
         nl_semantic_place_view(before,write->field.child,&field[0]) &&
         nl_semantic_place_view(after,write->field.child,&field[1]));
    MUST(root[0].governing_domain==2 && root[1].governing_domain==2 &&
         root[0].incarnation==root[1].incarnation &&
         root[0].current_fact!=root[1].current_fact &&
         field[0].current_fact!=field[1].current_fact &&
         field[0].current_fact==write->field.child_fact &&
         field[1].current_fact==write->field.child_post_fact &&
         root[0].fixed_fields[0]==write->field.child);
    MUST(r318_sum_target(before,write->field.child,3));
    MUST(r318_sum_target(after,write->field.child,4));
    NLSemanticOccurrenceView old,dead,fresh;
    MUST(field[0].payload_occurrence && field[1].payload_occurrence &&
         field[0].payload_occurrence!=field[1].payload_occurrence);
    MUST(nl_semantic_occurrence_view(before,field[0].payload_occurrence,&old) && old.live &&
         nl_semantic_occurrence_view(after,field[0].payload_occurrence,&dead) && !dead.live &&
         nl_semantic_occurrence_view(after,field[1].payload_occurrence,&fresh) && fresh.live);
    NLSemanticPlaceView old_payload={0};
    MUST(nl_semantic_place_view(after,old.payload_place,&old_payload) &&
         !old_payload.live && !old_payload.current_fact && !old_payload.current_value);
    /* Independently compare all five roots and their 20 fixed fields at the
     * second B after-world against A direct before-world and A after-world. */
    for(size_t region=1;region<=5;region++) {
        NLSemanticPlaceView prior,oldroot,newroot;
        NLPlaceId rootid=a.roots[region-1];
        MUST(nl_semantic_place_view(second->change.after,rootid,&prior) &&
             nl_semantic_place_view(before,rootid,&oldroot) &&
             nl_semantic_place_view(after,rootid,&newroot) &&
             !memcmp(&prior,&oldroot,sizeof(prior)));
        if(region!=2) MUST(!memcmp(&oldroot,&newroot,sizeof(oldroot)));
        for(size_t index=0;index<4;index++) {
            NLSemanticPlaceView x,y,z;
            MUST(nl_semantic_place_view(second->change.after,prior.fixed_fields[index],&x) &&
                 nl_semantic_place_view(before,prior.fixed_fields[index],&y) &&
                 nl_semantic_place_view(after,prior.fixed_fields[index],&z) &&
                 !memcmp(&x,&y,sizeof(x)));
            if(region!=2 || index!=0) MUST(!memcmp(&y,&z,sizeof(y)));
        }
    }
    /* The topology after all three real operations, including sibling fields. */
    const size_t expected[5][3]={{0,0,2},{4,4,0},{0,0,0},{0,3,0},{0,0,0}};
    for(size_t region=1;region<=5;region++) {
        NLSemanticPlaceView rootview;
        MUST(nl_semantic_place_view(after,a.roots[region-1],&rootview));
        for(size_t index=0;index<3;index++)
            MUST(r318_sum_target(after,rootview.fixed_fields[index],expected[region-1][index]));
    }
    NLSemanticBindingView current_B_alloc={0},current_B_domain={0},old_B_alloc={0},old_B_domain={0};
    NLSymbolId new_a=nl_semantic_find_binding(after,"allocation_B_current");
    NLSymbolId new_d=nl_semantic_find_binding(after,"life_B_current");
    NLSymbolId old_a=nl_semantic_find_binding(after,"allocation_B");
    NLSymbolId old_d=nl_semantic_find_binding(after,"life_B");
    MUST(new_a && new_d && old_a && old_d &&
         nl_semantic_binding_view(after,new_a,&current_B_alloc) &&
         nl_semantic_binding_view(after,new_d,&current_B_domain) &&
         nl_semantic_binding_view(after,old_a,&old_B_alloc) &&
         nl_semantic_binding_view(after,old_d,&old_B_domain));
    MUST(current_B_alloc.availability==NL_AVAILABLE &&
         current_B_domain.availability==NL_AVAILABLE &&
         current_B_alloc.value==b.alloc[2] && current_B_domain.value==b.domain[2] &&
         old_B_alloc.availability!=NL_AVAILABLE && old_B_domain.availability!=NL_AVAILABLE);
    NLSemanticScopeView active,closed;
    const NLScopeId scope=projection->reference_result.scope;
    MUST(scope && nl_semantic_scope_view(before,scope,&active) && active.active &&
         nl_semantic_scope_view(nl_checked_context(f),scope,&closed) && !closed.active);
    bool correct_original_A_loan=false;
    for(size_t i=1;i<=nl_checked_node_count(f);i++){
        const NLCheckedNodeView *n=nl_checked_node_view(f,i);
        if(n && n->kind==NL_CHECKED_LOAN_HEADER && n->loan.scope==scope &&
           n->loan.domain==2 && n->loan.source==nl_semantic_find_binding(before,"life_A") &&
           !n->loan.is_exclusive) correct_original_A_loan=true;
    }
    MUST(correct_original_A_loan);
    printf("R318_DIRECT_A_OK node=%zu original_A_root=%zu region=2 domain=2 old_target_region=3 new_target_region=4 old_occ=%zu ended=1 new_occ=%zu live=1 original_B_allocation=%zu domain_value=%zu unique=1 five_root_siblings_current=1 second_to_third_contiguous=1\n",
           step.operation,a.roots[1],field[0].payload_occurrence,field[1].payload_occurrence,b.alloc[2],b.domain[2]);
    return true;
}
static bool r318_strict_audit(const NLCheckedFragment *f){
    NLScopedFieldCallView calls[2]={{0}};
    NLCheckedNodeId ids[2]={0};
    size_t count=0;
    MUST(nl_checked_captured_change_count(f)==7);
    for(size_t i=1;i<=nl_checked_node_count(f);i++) {
        NLScopedFieldCallView candidate={0};
        if(nl_checked_scoped_field_call_view(f,i,&candidate)) {
            MUST(count<2);
            ids[count]=i;calls[count]=candidate;count++;
        }
    }
    MUST(count==2 && ids[0]<ids[1]);
    const NLCheckedNodeView *one=nl_checked_node_view(f,ids[0]);
    const NLCheckedNodeView *two=nl_checked_node_view(f,ids[1]);
    MUST(one && two && one->function==two->function && calls[0].body!=calls[1].body);
    MUST(r318_call_event(f,ids[0],&calls[0],0));
    MUST(r318_call_event(f,ids[1],&calls[1],1));
    MUST(r318_direct(f,&calls[1],ids[1]));
    puts("R318_STRICT_SOURCE_OWNED_OK");
    return true;
}
#endif
