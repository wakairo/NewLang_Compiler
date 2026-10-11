/*
 * Track R / Issue 301: independent public-only, read-only source-owned observer.
 * No private headers; no semantic seeds, forged match grants or compiler edits.
 * The three world pointers come from ONE checked call's own public certificate.
 */
#include "newlang/captured_closure.h"
#include "newlang/parser.h"
#include "newlang/raw_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(x) do { if (!(x)) { \
  fprintf(stderr,"OBSERVE_FAIL line=%d expression=%s\n",__LINE__,#x); \
  return false; } } while (0)

typedef struct {
  NLValueId packet, p, a, d;
  NLBackingRegionId region, allocation_region;
  NLDomainId domain;
  NLPlaceId root;
  NLIncarnationId incarnation;
} Original;

static bool extract(const NLSemanticContext *world, NLValueId id, Original *out) {
  NLSemanticValueView packet, ptr, alloc, domain;
  NLSemanticPlaceView root;
  REQUIRE(nl_semantic_value_view(world,id,&packet) && packet.field_count==3);
  REQUIRE(nl_semantic_value_view(world,packet.fields[0],&ptr));
  REQUIRE(nl_semantic_value_view(world,packet.fields[1],&alloc));
  REQUIRE(nl_semantic_value_view(world,packet.fields[2],&domain));
  REQUIRE(ptr.reference.provenance==NL_PROVENANCE_VALID);
  REQUIRE(ptr.reference.scope==0);
  REQUIRE(nl_semantic_place_view(world,ptr.reference.place,&root));
  REQUIRE(root.live && root.independent_root);
  REQUIRE(root.incarnation==ptr.reference.incarnation);
  REQUIRE(root.governing_domain==domain.domain);
  REQUIRE(root.placement.region != 0 && alloc.allocation_region != 0 && domain.domain != 0);
  REQUIRE(root.placement.start==0 && root.placement.length>0);
  *out=(Original){id,packet.fields[0],packet.fields[1],packet.fields[2],
        root.placement.region,alloc.allocation_region,domain.domain,
        ptr.reference.place,ptr.reference.incarnation};
  return true;
}
/* Only follow publicly stated value carriers: an aggregate child has one
 * owning parent, a place has one current value, a loose result has no binding. */
static bool has_current_carrier(const NLSemanticContext *world, NLValueId id,
                                NLValueId loose_result) {
  for (size_t depth=0;depth<16;++depth) {
    NLSemanticValueView v;
    REQUIRE(nl_semantic_value_view(world,id,&v));
    if(v.carrier==NL_CARRIER_LOOSE) return loose_result && id==loose_result;
    if(v.carrier==NL_CARRIER_PLACE) {
      NLSemanticPlaceView p; NLSemanticSnapshot s;
      REQUIRE(nl_semantic_place_view(world,v.owner_place,&p) &&
              p.live && p.current_value==id && nl_semantic_snapshot(world,&s));
      size_t matching=0;
      for(size_t b=1;b<=s.bindings;++b) {
        NLSemanticBindingView bv;
        REQUIRE(nl_semantic_binding_view(world,b,&bv));
        matching+=bv.availability==NL_AVAILABLE && bv.place==v.owner_place && bv.value==id;
      }
      REQUIRE(matching==1);
      return true;
    }
    REQUIRE(v.carrier==NL_CARRIER_AGGREGATE && v.aggregate_owner!=0);
    NLSemanticValueView parent;
    REQUIRE(nl_semantic_value_view(world,v.aggregate_owner,&parent));
    size_t occurrences=0;
    for(size_t k=0;k<parent.field_count;++k) occurrences+=parent.fields[k]==id;
    REQUIRE(occurrences==1);
    id=v.aggregate_owner;
  }
  return false;
}
static bool inventory(const NLSemanticContext *world, NLValueId result, const char *stage) {
  NLSemanticSnapshot s;
  REQUIRE(nl_semantic_snapshot(world,&s) && s.backing_regions==5);
  unsigned allocation[6]={0}, domain[6]={0};
  for(size_t id=1;id<=s.values;++id) {
    NLSemanticValueView v; NLSemanticTypeView t;
    REQUIRE(nl_semantic_value_view(world,id,&v) &&
            nl_semantic_type_view(world,v.type,&t));
    if(v.carrier==NL_CARRIER_ENDED) continue;
    if(t.kind==NL_TYPE_ALLOCATION) {
      REQUIRE(v.allocation_region>=1 && v.allocation_region<=5);
      REQUIRE(has_current_carrier(world,id,result));
      ++allocation[v.allocation_region];
    }
    if(v.type==nl_semantic_domain_type(world)) {
      REQUIRE(v.domain>=1 && v.domain<=5);
      REQUIRE(has_current_carrier(world,id,result));
      ++domain[v.domain];
    }
  }
  for(size_t r=1;r<=5;++r) {
    NLSemanticBackingView backing;
    REQUIRE(nl_semantic_backing_view(world,r,&backing));
    REQUIRE(allocation[r]==(unsigned)backing.live);
    REQUIRE(domain[r]==(unsigned)backing.live);
    REQUIRE(backing.live && allocation[r]==1 && domain[r]==1);
    printf("INVENTORY stage=%s region=%zu live=%d alloc=%u domain=%u\n",
           stage,r,(int)backing.live,allocation[r],domain[r]);
  }
  return true;
}
static NLCheckedNodeId find_allocation(const NLCheckedFragment *artifact) {
  for(size_t id=1;id<=nl_checked_node_count(artifact);++id) {
    const NLCheckedNodeView *v=nl_checked_node_view(artifact,id);
    if(!v || v->kind!=NL_CHECKED_MATCH) continue;
    const NLCheckedNodeView *subject=nl_checked_node_view(artifact,v->initializer);
    if(subject && subject->kind==NL_CHECKED_TRY_ALLOCATE_ONE) return id;
  }
  return 0;
}
static bool inspect(const NLCheckedFragment *source_generated) {
  const NLCheckedFragment *branch=nl_checked_call_body(
      source_generated,nl_checked_root(source_generated));
  REQUIRE(branch);
  NLCapturedClosureView certificate={0};
  for(size_t depth=0;depth<5;++depth) {
    NLCheckedNodeId match=find_allocation(branch);
    REQUIRE(match!=0);
    NLCheckStatus checked=nl_checked_captured_closure_validate(branch,match);
    printf("PUBLIC_CLOSURE depth=%zu status=%d\n",depth,checked);
    REQUIRE(checked==NL_CHECK_OK);
    if(depth==4) {
      REQUIRE(nl_checked_captured_closure_view(branch,match,&certificate));
      REQUIRE(certificate.count==4 && certificate.ancestor!=NULL);
      for(size_t k=0;k<4;++k) {
        REQUIRE(certificate.originals[k].origin==certificate.ancestor);
        REQUIRE(certificate.originals[k].root && certificate.originals[k].domain);
        REQUIRE(certificate.originals[k].incarnation);
      }
    }
    branch=nl_checked_match_arm(branch,match,1); /* Some success path */
    REQUIRE(branch!=NULL);
  }
  NLWholeValueCallView view={0}; unsigned count=0;
  for(size_t node=1;node<=nl_checked_node_count(branch);++node) {
    NLWholeValueCallView candidate={0};
    if(nl_checked_whole_value_call_view(branch,node,&candidate)) {
      view=candidate; ++count;
      REQUIRE(nl_checked_call_body(branch,node)!=NULL);
    }
  }
  REQUIRE(count==1 && view.count==3);
  REQUIRE(view.entry && view.returned && view.received && view.result);
  const NLSemanticContext *world[3]={view.entry,view.returned,view.received};
  const char *stages[3]={"entry","returned","received"};
  Original before[2], after[2];
  for(size_t i=0;i<2;++i) {
    NLSemanticBindingView donor0,donor1,formal;
    REQUIRE(view.inputs[i] && view.donors[i] && view.parameters[i]);
    REQUIRE(nl_semantic_binding_view(world[0],view.donors[i],&donor0));
    REQUIRE(donor0.availability==NL_AVAILABLE && donor0.value==view.inputs[i]);
    REQUIRE(nl_semantic_binding_view(world[1],view.donors[i],&donor1));
    REQUIRE(donor1.availability==NL_CONSUMED);
    REQUIRE(nl_semantic_binding_view(world[1],view.parameters[i],&formal));
    REQUIRE(formal.availability==NL_CONSUMED && formal.value==view.inputs[i]);
    REQUIRE(donor0.place!=formal.place);
    REQUIRE(extract(world[0],view.inputs[i],&before[i]));
    REQUIRE(extract(world[2],view.inputs[i],&after[i]));
    REQUIRE(has_current_carrier(world[0],view.inputs[i],0));
    REQUIRE(before[i].p==after[i].p && before[i].a==after[i].a &&
            before[i].d==after[i].d &&
            before[i].root==after[i].root &&
            before[i].incarnation==after[i].incarnation &&
            before[i].region==after[i].region &&
            before[i].allocation_region==after[i].allocation_region &&
            before[i].domain==after[i].domain);
    /* World-qualified snapshots from one call certificate. The value IDs
     * are compared only within the artifact's linked entry/return/receive
     * lineage; never to an unrelated hypothetical match arm. */
    Original middle;
    REQUIRE(extract(world[1],view.inputs[i],&middle));
    REQUIRE(middle.p==before[i].p && middle.a==before[i].a && middle.d==before[i].d);
    REQUIRE(middle.root==before[i].root && middle.incarnation==before[i].incarnation);
    REQUIRE(middle.region==before[i].region &&
            middle.allocation_region==before[i].allocation_region &&
            middle.domain==before[i].domain);
  }
  REQUIRE(before[0].region!=before[1].region &&
          before[0].allocation_region!=before[1].allocation_region &&
          before[0].domain!=before[1].domain && before[0].root!=before[1].root);
  /* Verify copied inert pointer: token's original donor remains available.
   * It does not gain domain, allocation or subobject ownership. */
  NLSemanticBindingView p0,p1,p2;
  NLSemanticValueView token[3];
  NLSemanticTypeView pointer_type;
  REQUIRE(nl_semantic_binding_view(world[0],view.donors[2],&p0) &&
          nl_semantic_binding_view(world[2],view.donors[2],&p2) &&
          nl_semantic_binding_view(world[1],view.parameters[2],&p1));
  REQUIRE(p0.availability==NL_AVAILABLE && p2.availability==NL_AVAILABLE &&
          p0.value==p2.value && p1.availability==NL_CONSUMED &&
          p1.value==view.inputs[2]);
  REQUIRE(nl_semantic_type_view(world[0],p0.type,&pointer_type));
  REQUIRE(pointer_type.kind==NL_TYPE_PTR && pointer_type.is_copy);
  REQUIRE(nl_semantic_value_view(world[0],p0.value,&token[0]));
  REQUIRE(nl_semantic_value_view(world[1],view.inputs[2],&token[1]));
  REQUIRE(nl_semantic_value_view(world[2],view.inputs[2],&token[2]));
  REQUIRE(token[0].reference.place==token[1].reference.place &&
          token[1].reference.place==token[2].reference.place &&
          token[0].reference.incarnation==token[1].reference.incarnation &&
          token[1].reference.incarnation==token[2].reference.incarnation);
  for(size_t i=0;i<3;++i) {
    REQUIRE(token[i].allocation_region==0 && token[i].domain==0 &&
            token[i].reference.scope==0 &&
            token[i].reference.provenance==NL_PROVENANCE_VALID);
  }
  NLSemanticValueView result1,result2;
  NLSemanticBindingView receiver;
  REQUIRE(nl_semantic_value_view(world[1],view.result,&result1) &&
          nl_semantic_value_view(world[2],view.result,&result2));
  REQUIRE(result1.carrier==NL_CARRIER_LOOSE && result2.carrier==NL_CARRIER_PLACE &&
          result1.field_count==2 && result2.field_count==2);
  REQUIRE(nl_semantic_binding_view(world[2],view.receiver,&receiver) &&
          receiver.availability==NL_AVAILABLE && receiver.value==view.result);
  bool matches[2]={false,false};
  for(size_t index=0;index<2;++index) {
    NLValueId id=result2.fields[index];
    REQUIRE(id==result1.fields[index]);
    Original member; REQUIRE(extract(world[2],id,&member));
    unsigned matched=0;
    for(size_t i=0;i<2;++i) {
      if(member.p==before[i].p && member.a==before[i].a &&
         member.d==before[i].d && member.root==before[i].root &&
         member.incarnation==before[i].incarnation) {
         REQUIRE(!matches[i]); matches[i]=true; ++matched;
      }
    }
    REQUIRE(matched==1);
    printf("TRANSFER received_slot=%zu value=%zu region=%zu A_region=%zu D=%zu "
           "root=%zu incarnation=%zu p=%zu a=%zu d=%zu\n",
           index,id,member.region,member.allocation_region,member.domain,
           member.root,member.incarnation,member.p,member.a,member.d);
  }
  REQUIRE(matches[0] && matches[1]);
  for(size_t i=0;i<2;++i)
    printf("SOURCE original=%zu value=%zu region=%zu A_region=%zu D=%zu "
           "root=%zu incarnation=%zu donor=%zu formal=%zu\n",
           i,before[i].packet,before[i].region,before[i].allocation_region,
           before[i].domain,before[i].root,before[i].incarnation,
           view.donors[i],view.parameters[i]);
  for(size_t i=0;i<3;++i) REQUIRE(inventory(world[i],i==1?view.result:0,stages[i]));
  printf("OWNED_PUBLIC_IDENTITY_PASS worlds=3 originals=2 checks=5 "
         "copy_ptr_no_authority=1 entry_donors_consumed=1 receiver_unique=1\n");
  return true;
}
int main(int argc,char **argv) {
  if(argc!=2){fprintf(stderr,"usage: observer source.nl\n");return 64;}
  NLSource *source=NULL,*call_source=NULL;
  NLParser *parser=NULL,*call_parser=NULL;
  NLSyntaxTree *unit=NULL,*call_syntax=NULL;
  NLSemanticContext *context=NULL;
  NLCheckedFragment *checked=NULL;
  NLFunctionUnitDiagnostic diagnostic={0};
  NLCheckDiagnostic call_diag={0};
  bool okay=false;
  NLCheckStatus register_status=NL_CHECK_INTERNAL_ERROR;
  NLCheckStatus check_status=NL_CHECK_INTERNAL_ERROR;
  if(nl_source_load(argv[1],&source)!=NL_SOURCE_OK ||
     nl_parser_create(source,&parser)!=NL_PARSE_OK ||
     nl_parser_parse_function_unit(parser,&unit,NULL)!=NL_PARSE_OK ||
     nl_semantic_create(&context)!=NL_CHECK_OK) {
    fprintf(stderr,"FIRST_GATE parse_or_context_failed\n");goto done;
  }
  const NLSyntaxTree *input[1]={unit};
  register_status=nl_semantic_register_function_unit(context,input,1,&diagnostic);
  printf("REGISTER source=%s status=%d\n",argv[1],register_status);
  if(register_status!=NL_CHECK_OK) {
    fprintf(stderr,"FIRST_GATE register_function_unit code=%s\n",
        diagnostic.diagnostic.diagnostic.code?
        diagnostic.diagnostic.diagnostic.code:"(none)");
    nl_check_diagnostic_render(stderr,source,&diagnostic.diagnostic);
    goto done;
  }
  if(nl_source_create("main()",6,"R301-main-call",&call_source)!=NL_SOURCE_OK ||
     nl_parser_create(call_source,&call_parser)!=NL_PARSE_OK ||
     nl_parser_parse_expression_fragment(call_parser,&call_syntax,NULL)!=NL_PARSE_OK) {
    fprintf(stderr,"FIRST_GATE main_expression_parse\n");goto done;
  }
  check_status=nl_semantic_check_expression(context,call_syntax,&checked,&call_diag);
  printf("CHECK source=%s status=%d artifact=%d\n",argv[1],check_status,checked!=NULL);
  if(check_status!=NL_CHECK_OK) {
    fprintf(stderr,"FIRST_GATE semantic_check_expression code=%s\n",
        call_diag.diagnostic.code?call_diag.diagnostic.code:"(none)");
    nl_check_diagnostic_render(stderr,call_source,&call_diag);
    goto done;
  }
  okay=checked && inspect(checked);
done:
  nl_checked_destroy(checked); nl_semantic_destroy(context);
  nl_syntax_tree_destroy(call_syntax); nl_parser_destroy(call_parser);
  nl_source_destroy(call_source);
  nl_syntax_tree_destroy(unit); nl_parser_destroy(parser); nl_source_destroy(source);
  return okay?0:1;
}
