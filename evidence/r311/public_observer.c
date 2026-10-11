/* Track R #311 independent PUBLIC checked-artifact audit. No private API. */
#include "newlang/captured_closure.h"
#include "newlang/parser.h"
#include "newlang/raw_storage.h"
#include "newlang/source.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(q) do { if(!(q)) {fprintf(stderr,"R311 FAIL line %d: %s\\n",__LINE__,#q); goto end;} } while(0)
static NLCheckedNodeId try_match(const NLCheckedFragment *f) {
 for(size_t id=1;id<=nl_checked_node_count(f);id++){
  const NLCheckedNodeView *n=nl_checked_node_view(f,id);
  const NLCheckedNodeView *i=n?nl_checked_node_view(f,n->initializer):NULL;
  if(n&&n->kind==NL_CHECKED_MATCH&&i&&i->kind==NL_CHECKED_TRY_ALLOCATE_ONE)return id;
 }return 0;
}
static NLSymbolId locate_symbol(const NLCheckedFragment *f,const char *name){
 const NLSource *s=nl_checked_source(f);
 for(size_t id=1;id<=nl_checked_node_count(f);id++){
  const NLCheckedNodeView *n=nl_checked_node_view(f,id);
  NLSourceView v;
  if(n&&n->symbol&&nl_source_view(s,n->name,&v)&&
    v.length==strlen(name)&&!memcmp(v.bytes,name,v.length)) return n->symbol;
 }return 0;
}
static bool unchanged_sibling(const NLSemanticPlaceView *a,const NLSemanticPlaceView *b){
 return a->live&&b->live&&a->incarnation==b->incarnation&&
        a->current_fact==b->current_fact&&
        a->current_value==b->current_value&&
        a->payload_occurrence==b->payload_occurrence;
}
int main(int argc,char **argv){
 NLSource *source=NULL,*entrysrc=NULL;
 NLParser *parser=NULL,*ep=NULL;
 NLSyntaxTree *tree=NULL,*expression=NULL;
 NLSemanticContext *c=NULL;
 NLCheckedFragment *entry=NULL;
 NLFunctionUnitDiagnostic fd={0}; NLCheckDiagnostic ed={0};
 int ok=0;
 if(argc!=3){fprintf(stderr,"usage: observer <fresh.nl> <none|some>\\n");return 2;}
 REQUIRE(nl_source_load(argv[1],&source)==NL_SOURCE_OK);
 REQUIRE(nl_parser_create(source,&parser)==NL_PARSE_OK);
 REQUIRE(nl_parser_parse_function_unit(parser,&tree,NULL)==NL_PARSE_OK);
 REQUIRE(nl_semantic_create(&c)==NL_CHECK_OK);
 const NLSyntaxTree *unit[]={tree};
 NLCheckStatus register_status=nl_semantic_register_function_unit(c,unit,1,&fd);
 if(register_status!=NL_CHECK_OK){
  fprintf(stderr,"R311 register status=%d\\n",(int)register_status);
  nl_check_diagnostic_render(stderr,source,&fd.diagnostic);
  goto end;
 }
 REQUIRE(nl_source_create("main()",6,"<R311-entry>",&entrysrc)==NL_SOURCE_OK);
 REQUIRE(nl_parser_create(entrysrc,&ep)==NL_PARSE_OK);
 REQUIRE(nl_parser_parse_expression_fragment(ep,&expression,NULL)==NL_PARSE_OK);
 REQUIRE(nl_semantic_check_expression(c,expression,&entry,&ed)==NL_CHECK_OK&&entry);
 const NLCheckedFragment *f=nl_checked_call_body(entry,nl_checked_root(entry));
 REQUIRE(f);
 NLCapturedOriginal original={0};
 for(size_t site=0;site<5;site++){
  NLCheckedNodeId m=try_match(f);
  REQUIRE(m);
  REQUIRE(nl_checked_captured_closure_validate(f,m)==NL_CHECK_OK);
  NLCapturedClosureView closure={0};
  REQUIRE(nl_checked_captured_closure_view(f,m,&closure));
  for(size_t j=0;j<closure.count;j++)
   if(closure.originals[j].domain==3)original=closure.originals[j];
  f=nl_checked_match_arm(f,m,1);
  REQUIRE(f);
 }
 REQUIRE(original.root&&original.extent.region==3);
 REQUIRE(nl_checked_captured_change_count(f)==6);
 NLScopedFieldCallView callview={0},candidate={0};
 NLCheckedNodeId callid=0;
 for(size_t id=1;id<=nl_checked_node_count(f);id++){
  if(nl_checked_scoped_field_call_view(f,id,&candidate)){
   REQUIRE(!callid);callid=id;callview=candidate;
  }
 }
 REQUIRE(callid&&nl_checked_scoped_field_call_validate(f,callid)==NL_CHECK_OK);
 REQUIRE(nl_checked_captured_change_count(callview.body)==1);
 const NLCheckedNodeView *change=nl_checked_node_view(callview.body,callview.change.operation);
 const NLCheckedNodeView *projection=nl_checked_node_view(f,callview.projection);
 REQUIRE(change&&change->kind==NL_CHECKED_REPLACE);
 REQUIRE(projection&&projection->kind==NL_CHECKED_FIELD_REF&&projection->field.index==1);
 REQUIRE(change->field.index==1&&change->field.child==projection->field.child);
 const NLSemanticContext *pre=callview.change.before,*post=callview.change.after;
 REQUIRE(pre&&post);
 NLSemanticPlaceView r0,r1,p0,p1,next0,next1,child0,child1,old_payload0,old_payload1;
 REQUIRE(nl_semantic_place_view(pre,change->field.parent,&r0));
 REQUIRE(nl_semantic_place_view(post,change->field.parent,&r1));
 REQUIRE(r0.live&&r1.live&&r0.placement.region==3&&r1.placement.region==3);
 REQUIRE(r0.governing_domain==3&&r1.governing_domain==3);
 REQUIRE(r0.incarnation==r1.incarnation&&r0.current_fact!=r1.current_fact);
 REQUIRE(r0.fixed_field_count==4&&r0.fixed_fields[1]==change->field.child);
 REQUIRE(r0.fixed_fields[0]!=r0.fixed_fields[1]&&r0.fixed_fields[2]!=r0.fixed_fields[1]);
 REQUIRE(nl_semantic_place_view(pre,r0.fixed_fields[1],&p0));
 REQUIRE(nl_semantic_place_view(post,r0.fixed_fields[1],&p1));
 REQUIRE(nl_semantic_place_view(pre,r0.fixed_fields[0],&next0));
 REQUIRE(nl_semantic_place_view(post,r0.fixed_fields[0],&next1));
 REQUIRE(nl_semantic_place_view(pre,r0.fixed_fields[2],&child0));
 REQUIRE(nl_semantic_place_view(post,r0.fixed_fields[2],&child1));
 REQUIRE(p0.live&&p1.live&&p0.incarnation==p1.incarnation&&p0.current_fact!=p1.current_fact);
 REQUIRE(unchanged_sibling(&next0,&next1)&&unchanged_sibling(&child0,&child1));
 REQUIRE(p0.current_value==change->field.old_value&&p1.current_value==change->field.new_value);
 NLSemanticValueView oldval,newval;
 REQUIRE(nl_semantic_value_view(pre,p0.current_value,&oldval));
 REQUIRE(nl_semantic_value_view(post,p1.current_value,&newval));
 REQUIRE(oldval.variant==2&&p0.payload_occurrence);
 NLSemanticOccurrenceView oldocc0,oldocc1,newocc;
 REQUIRE(nl_semantic_occurrence_view(pre,p0.payload_occurrence,&oldocc0)&&oldocc0.live);
 REQUIRE(nl_semantic_occurrence_view(post,p0.payload_occurrence,&oldocc1)&&!oldocc1.live);
 REQUIRE(nl_semantic_place_view(pre,oldocc0.payload_place,&old_payload0)&&old_payload0.live);
 REQUIRE(nl_semantic_place_view(post,oldocc0.payload_place,&old_payload1)&&!old_payload1.live);
 if(!strcmp(argv[2],"none"))REQUIRE(newval.variant==1&&!p1.payload_occurrence);
 else{
  REQUIRE(!strcmp(argv[2],"some")&&newval.variant==2&&p1.payload_occurrence!=p0.payload_occurrence);
  REQUIRE(nl_semantic_occurrence_view(post,p1.payload_occurrence,&newocc)&&newocc.live);
 }
 NLSemanticBindingView first_d,first_a,moved_d,moved_a;
 REQUIRE(nl_semantic_binding_view(original.origin,original.domain_binding,&first_d));
 REQUIRE(nl_semantic_binding_view(original.origin,original.allocation_binding,&first_a));
 REQUIRE(first_d.availability==NL_AVAILABLE&&first_a.availability==NL_AVAILABLE);
 NLSymbolId md=locate_symbol(f,"moved_d_B"),ma=locate_symbol(f,"moved_a_B");
 REQUIRE(md&&ma);
 REQUIRE(nl_semantic_binding_view(pre,md,&moved_d));
 REQUIRE(nl_semantic_binding_view(pre,ma,&moved_a));
 REQUIRE(moved_d.availability==NL_AVAILABLE&&moved_a.availability==NL_AVAILABLE);
 REQUIRE(moved_d.value==first_d.value&&moved_a.value==first_a.value);
 REQUIRE(original.root==change->field.parent&&original.incarnation==r0.incarnation);
 NLSemanticPlaceView old_root,newcarrier;
 REQUIRE(nl_semantic_place_view(original.origin,original.root,&old_root)&&old_root.live);
 REQUIRE(nl_semantic_place_view(pre,moved_d.place,&newcarrier)&&newcarrier.live);
 REQUIRE(newcarrier.current_value==first_d.value);
 NLSymbolId donor=locate_symbol(f,"life_B"),holder=locate_symbol(f,"holder_B");
 NLSemanticBindingView dead,spent;
 REQUIRE(donor&&holder);
 REQUIRE(nl_semantic_binding_view(pre,donor,&dead)&&dead.availability==NL_CONSUMED);
 REQUIRE(nl_semantic_binding_view(pre,holder,&spent)&&spent.availability==NL_CONSUMED);
 bool freshloan=false;
 for(size_t id=1;id<=nl_checked_node_count(f);id++){
  const NLCheckedNodeView *n=nl_checked_node_view(f,id);
  if(n&&n->kind==NL_CHECKED_LOAN_HEADER&&n->loan.scope==projection->reference_result.scope){
   REQUIRE(n->loan.domain==3&&n->loan.place==moved_d.place);
   freshloan=true;
  }
 }
 REQUIRE(freshloan);
 NLSemanticScopeView scope;
 REQUIRE(nl_semantic_scope_view(pre,projection->reference_result.scope,&scope)&&scope.active);
 REQUIRE(nl_semantic_scope_view(nl_checked_context(f),projection->reference_result.scope,&scope)&&!scope.active);
 printf("R311_OK mode=%s closure_validators=5 scoped_validator=1 caller_changes=6 callee_changes=1 B_region=%zu B_domain=%zu B_root=%zu B_inc=%zu A_B=%zu D_B=%zu moved_D_place=%zu prev_idx=1 prev_old_occ=%zu prev_new_occ=%zu next_fact=%zu child_fact=%zu\\n",
  argv[2],r0.placement.region,r0.governing_domain,original.root,r0.incarnation,first_a.value,first_d.value,moved_d.place,p0.payload_occurrence,p1.payload_occurrence,next0.current_fact,child0.current_fact);
 ok=1;
end:
 nl_checked_destroy(entry);
 nl_semantic_destroy(c);
 nl_syntax_tree_destroy(expression);nl_parser_destroy(ep);nl_source_destroy(entrysrc);
 nl_syntax_tree_destroy(tree);nl_parser_destroy(parser);nl_source_destroy(source);
 return ok?0:1;
}
