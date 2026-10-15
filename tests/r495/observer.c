/* R495 public observation only. Frozen preflight on Issue #495. */
#include "../support/node_checked.h"
#include "newlang/ordinary_call.h"
#include <stdio.h>
#define R(c) do { if(!(c)) { fprintf(stderr,"R495 FAIL line %d: %s\n",__LINE__,#c); return 1; } } while(0)
int main(int argc,char **argv) {
  R(argc==2);
  TestNode n={0}; R(node_checked_load(argv[1],&n));
  const NLCheckedFragment *f=nl_checked_call_body(n.entry,nl_checked_root(n.entry)); R(f);
  size_t nc=0,np=0; NLCheckedNodeId call=0; NLOrdinaryCallView v={0};
  for(size_t i=1;i<=nl_checked_node_count(f);i++){
    const NLCheckedNodeView *p=nl_checked_node_view(f,i); R(p);
    if(p->kind==NL_CHECKED_REGISTERED_CALL){
      ++nc; call=i;
      if(nl_checked_ordinary_call_view(f,i,&v)) ++np;
    }
  }
  R(nc==1);
#ifdef R495_EXPECT_OFF
  R(np==0);
  R(nl_checked_ordinary_call_validate(f,call,NULL)==NL_CHECK_ANALYSIS_PRECISION_LIMIT);
  puts("R495 OFF: source CHECK succeeded; qualified public proof not issued");
#else
  R(np==1 && v.caller==f && v.call==call && v.body==nl_checked_call_body(f,call));
  R(v.originals[0]!=v.originals[1] &&
    (v.root==v.originals[0] || v.root==v.originals[1]));
  NLPlaceId sibling=v.root==v.originals[0]?v.originals[1]:v.originals[0];
  NLSemanticPlaceView tr,hr,tf,tfp,tfc,hf,hfp;
  R(nl_semantic_place_view(v.entry,v.root,&tr));
  R(nl_semantic_place_view(v.entry,sibling,&hr));
  R(tr.live && hr.live && tr.incarnation!=hr.incarnation &&
    tr.placement.region==0 && hr.placement.region==0 &&
    tr.governing_domain==0 && hr.governing_domain==0 &&
    tr.fixed_fields[0]==v.field && hr.fixed_fields[0]!=v.field);
  R(nl_semantic_place_view(v.entry,v.field,&tf));
  R(nl_semantic_place_view(v.post,v.field,&tfp));
  R(nl_semantic_place_view(v.closed_loan,v.field,&tfc));
  R(nl_semantic_place_view(v.entry,hr.fixed_fields[0],&hf));
  R(nl_semantic_place_view(v.post,hr.fixed_fields[0],&hfp));
  R(tf.type==hf.type && tf.live && tfp.live &&
    tf.incarnation==tfp.incarnation && tf.current_fact!=tfp.current_fact &&
    tf.payload_occurrence==0 && tfp.payload_occurrence!=0 &&
    tfp.payload_occurrence==tfc.payload_occurrence &&
    hf.current_fact==hfp.current_fact && hf.current_value==hfp.current_value &&
    hf.payload_occurrence==hfp.payload_occurrence);
  NLSemanticValueView before,after,head,old,locator;
  R(nl_semantic_value_view(v.entry,tf.current_value,&before) && before.variant==1);
  R(nl_semantic_value_view(v.post,tfp.current_value,&after) &&
    after.variant==2 && after.sum_payload!=0);
  R(nl_semantic_value_view(v.post,hfp.current_value,&head) && head.variant==2);
  R(nl_semantic_value_view(v.post,v.returned,&old) &&
    old.variant==1 && old.carrier==NL_CARRIER_LOOSE);
  R(nl_semantic_value_view(v.post,after.sum_payload,&locator) &&
    locator.reference.place==v.root &&
    locator.reference.incarnation==tr.incarnation &&
    locator.reference.scope==0 && locator.domain==0 &&
    locator.allocation_region==0);
  NLSemanticScopeView a,b,c;
  R(v.reference.scope!=0 &&
    nl_semantic_scope_view(v.entry,v.reference.scope,&a) &&
    nl_semantic_scope_view(v.post,v.reference.scope,&b) &&
    nl_semantic_scope_view(v.closed_loan,v.reference.scope,&c) &&
    a.active && b.active && !c.active);
  NLSemanticOccurrenceView occ;
  R(nl_semantic_occurrence_view(v.closed_loan,tfp.payload_occurrence,&occ) &&
    occ.live);
  R(nl_checked_ordinary_call_validate(f,call,NULL)==NL_CHECK_OK);
  R(nl_checked_ordinary_call_validate(f,call,&v)==NL_CHECK_OK);
  NLOrdinaryCallView wrong=v;
  wrong.reference.place=hr.fixed_fields[0];
  wrong.reference.incarnation=hf.incarnation;
  R(nl_checked_ordinary_call_validate(f,call,&wrong)==
    NL_CHECK_ANALYSIS_PRECISION_LIMIT);
  wrong=v; wrong.closed_loan=v.post;
  R(nl_checked_ordinary_call_validate(f,call,&wrong)==
    NL_CHECK_ANALYSIS_PRECISION_LIMIT);
  wrong=v; wrong.reference.occurrence_dependency=tfp.payload_occurrence;
  R(nl_checked_ordinary_call_validate(f,call,&wrong)==
    NL_CHECK_ANALYSIS_PRECISION_LIMIT);
  wrong=v; ++wrong.returned;
  R(nl_checked_ordinary_call_validate(f,call,&wrong)==
    NL_CHECK_ANALYSIS_PRECISION_LIMIT);
  wrong=v; wrong.originals[0]=wrong.originals[1];
  R(nl_checked_ordinary_call_validate(f,call,&wrong)==
    NL_CHECK_ANALYSIS_PRECISION_LIMIT);
  R(nl_checked_ordinary_call_validate(f,call,&v)==NL_CHECK_OK);
  NLSemanticPlaceView unchanged;
  R(nl_semantic_place_view(v.entry,v.field,&unchanged) &&
    unchanged.current_fact==tf.current_fact &&
    unchanged.current_value==tf.current_value);
  printf("R495 qualified call=%zu tail=%zu field=%zu sibling=%zu scope=%zu "
         "old=None; Some(T) installed at T.next, H.next unchanged; "
         "5 public negatives refused; historical scope closed\n",
         call,v.root,v.field,sibling,v.reference.scope);
#endif
  node_checked_destroy(&n);
  return 0;
}
