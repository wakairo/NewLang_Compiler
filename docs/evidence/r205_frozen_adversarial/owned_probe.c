/* Track R #205: read-only independent checked artifact / rollback probe. */
#include "semantic_internal.h"
#include "node_checked.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
static unsigned checks=0, owners=0;
static bool invalid(const char *msg) { fprintf(stderr,"R205_OWNED_PROBE_FAILURE %s\n",msg);return false; }
#define PROVE(cond, msg) do { ++checks; if (!(cond)) return invalid(msg); } while(0)
static bool inspect(const NLCheckedFragment *f,const char *name) {
 if (!f) return invalid("null artifact");
 for (NLCheckedNodeId i=1;i<=nl_checked_node_count(f);++i) {
  const NLCheckedNodeView *v=nl_checked_node_view(f,i);
  PROVE(v!=NULL,"node view");
  if (v->owner_call.entry_proved) {
   ++owners;
   const NLSemanticContext *pre=nl_checked_owner_entry(f,i),*post=nl_checked_context(f);
   const NLCheckedFragment *body=nl_checked_call_body(f,i);
   PROVE(v->owner_call.post_proved && body!=NULL,"owner call missing post/body");
   PROVE(pre && post && pre!=post,"owner before/after snapshot separation");
   NLPlaceId o=v->owner_call.root;
   NLBackingRegionId r=v->owner_call.range.region;
   NLDomainId d=v->owner_call.domain;
   PROVE(o>0 && o<=pre->place_count && o<=post->place_count,"root bounds");
   PROVE(r>0 && r<=pre->region_count && r<=post->region_count,"region bounds");
   PROVE(d>0 && d<=pre->domain_count && d<=post->domain_count,"domain bounds");
   PROVE(pre->places[o-1].live && pre->regions[r-1].view.live && pre->domains[d-1].live,"live entry O/R/D");
   PROVE(pre->places[o-1].incarnation==v->owner_call.incarnation && pre->places[o-1].governing_domain==d,"entry incarnation governs");
   PROVE(!post->places[o-1].live && !post->regions[r-1].view.live && !post->domains[d-1].live,"post O/R/D terminated");
   PROVE(post->places[o-1].placement.region==0 && post->places[o-1].current_value==0,"post root cleared");
   PROVE(pre->region_count==2 && pre->domain_count==2,"two independent allocations");
   unsigned other=0;
   for(size_t p=0;p<pre->place_count;p++)
    if(pre->places[p].live && pre->places[p].independent_root && pre->places[p].placement.region && p+1!=o) {
     ++other;
     PROVE(pre->places[p].placement.region!=r && pre->places[p].governing_domain!=d,"head/tail nonalias");
   }
   PROVE(other==1,"exactly one other root");
   for(unsigned j=1;j<=2;j++){
    NLSymbolId donor=v->owner_call.donor[j],param=v->owner_call.parameters[j];
    NLValueId input=v->owner_call.inputs[j];
    PROVE(donor && param && donor!=param && input,"fresh authority parameter");
    PROVE(donor<=pre->binding_count && donor<=post->binding_count && param<=post->binding_count && input<=post->value_count,"authority bounds");
    PROVE(pre->bindings[donor-1].view.availability==NL_CONSUMED &&
          post->bindings[donor-1].view.availability==NL_CONSUMED,"donor consumed both states");
    PROVE(post->bindings[param-1].view.availability==NL_CONSUMED,"callee authority consumed");
    PROVE(post->bindings[param-1].view.value==input,"same authority identity");
    PROVE(post->values[input-1].carrier==NL_CARRIER_ENDED,"authority value ended");
   }
   PROVE(pre->values[v->owner_call.inputs[1]-1].allocation_region==r,"allocation original region");
   PROVE(pre->values[v->owner_call.inputs[2]-1].domain==d,"domain original identity");
   unsigned ends=0,frees=0;
   for(NLCheckedNodeId k=1;k<=nl_checked_node_count(body);k++){
    const NLCheckedNodeView *op=nl_checked_node_view(body,k);
    if(op->kind==NL_CHECKED_DESTROY) {
     ++ends;
     PROVE(op->lifetime_place==o && op->lifetime_incarnation==v->owner_call.incarnation &&
           op->lifetime_domain==d && op->backing==r,"EndRoot original identity");
    }
    if(op->kind==NL_CHECKED_DEALLOCATE) ++frees;
   }
   PROVE(ends==1 && frees==1,"exactly one EndRoot and one deallocation in receiver");
   printf("R205_OWNED_CALL source=%s O=%zu R=%zu D=%zu entry_live=1 post_live=0 donors_consumed=2 callee_consumed=2 receiver_ends=1 receiver_frees=1 independent_head=1\n",name,(size_t)o,(size_t)r,(size_t)d);
  }
  const NLCheckedFragment *body=nl_checked_call_body(f,i);
  if(body && !inspect(body,name))return false;
  if(v->kind==NL_CHECKED_MATCH)
   for(size_t a=0;a<v->item_count;a++)
    if(!inspect(nl_checked_match_arm(f,i,a),name))return false;
 }
 return true;
}
static bool positive(const char *path) {
 TestNode n={0};
 if(!node_checked_load(path,&n))return invalid("valid source failed source->checked");
 owners=0;
 bool ok=inspect(n.entry,path);
 if(ok && owners!=1)ok=invalid("expected one checked owner call");
 node_checked_destroy(&n);
 return ok;
}
/* Source-founded failed registration/failed call: no partial evidence or changed caller world. */
static bool negative(const char *path) {
 NLSource *source=NULL,*entry=NULL;
 NLParser *parser=NULL,*ep=NULL;
 NLSyntaxTree *unit=NULL,*et=NULL;
 NLSemanticContext *c=NULL;
 NLCheckedFragment *out=NULL;
 NLCheckDiagnostic diagnostic={0};
 NLFunctionUnitDiagnostic registration_diagnostic={0};
 bool ok=false;
 if(nl_source_load(path,&source)!=NL_SOURCE_OK)return invalid("load negative source");
 if(nl_parser_create(source,&parser)!=NL_PARSE_OK)return invalid("parser negative");
 if(nl_parser_parse_function_unit(parser,&unit,NULL)!=NL_PARSE_OK)return invalid("parse negative");
 if(nl_semantic_create(&c)!=NL_CHECK_OK)return invalid("semantic create");
 NLSemanticSnapshot before={0},after={0};
 if(!nl_semantic_snapshot(c,&before))return invalid("before reg snapshot");
 const NLSyntaxTree *inputs[]={unit};
 NLCheckStatus reg=nl_semantic_register_function_unit(c,inputs,1,&registration_diagnostic);
 if(reg!=NL_CHECK_OK) {
  if(!nl_semantic_snapshot(c,&after) || memcmp(&before,&after,sizeof(before))!=0)
   return invalid("failed registration published state");
  printf("R205_OWNED_ROLLBACK source=%s failure=registration status=%d snapshot_unchanged=1 checked_published=0\n",path,(int)reg);
  ok=true;goto done;
 }
 if(nl_source_create("main()",6,"r205-entry",&entry)!=NL_SOURCE_OK)return invalid("entry source");
 if(nl_parser_create(entry,&ep)!=NL_PARSE_OK)return invalid("entry parser");
 if(nl_parser_parse_expression_fragment(ep,&et,NULL)!=NL_PARSE_OK)return invalid("entry syntax");
 if(!nl_semantic_snapshot(c,&before))return invalid("pre-call snapshot");
 NLCheckStatus st=nl_semantic_check_expression(c,et,&out,&diagnostic);
 if(st==NL_CHECK_OK || out!=NULL)return invalid("bad source published checked result");
 if(!nl_semantic_snapshot(c,&after) || memcmp(&before,&after,sizeof(before))!=0)
  return invalid("failed call changed caller state");
 printf("R205_OWNED_ROLLBACK source=%s failure=call status=%d snapshot_unchanged=1 checked_published=0\n",path,(int)st);
 ok=true;
done:
 nl_checked_destroy(out);
 nl_syntax_tree_destroy(et);nl_parser_destroy(ep);nl_source_destroy(entry);
 nl_syntax_tree_destroy(unit);nl_parser_destroy(parser);nl_source_destroy(source);nl_semantic_destroy(c);
 return ok;
}
int main(int argc,char **argv) {
 if(argc<3)return 2;
 for(int i=1;i<argc;i++){
  bool good=strstr(argv[i],"/p")!=NULL || strstr(argv[i],"/b_p")!=NULL;
  if(!(good?positive(argv[i]):negative(argv[i])))return 1;
 }
 printf("R205_OWNED_PROBE_PASS cases=%d checks=%u\n",argc-1,checks);
 return 0;
}
