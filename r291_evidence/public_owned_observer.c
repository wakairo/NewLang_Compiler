/* R #291: independent public source -> owned artifact observer.
 * Never seeds a semantic context, mutates an artifact or uses P fixtures. */
#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/source.h"
#include "newlang/semantic.h"
#include <stdio.h>
#include <stdlib.h>

static unsigned witnessed = 0;
static void describe(const NLSemanticContext *c, NLValueId id,
                     const char *tag, unsigned depth)
{
    NLSemanticValueView v;
    if (!c || !id || !nl_semantic_value_view(c,id,&v)) {
        printf(" R291_VALUE %s missing=%zu\n",tag,id);
        return;
    }
    printf(" R291_VALUE %s id=%zu type=%zu carrier=%d fields=%zu domain=%zu allocation=%zu backing=%zu ref.place=%zu ref.inc=%zu owner.place=%zu\n",
        tag,id,v.type,(int)v.carrier,v.field_count,v.domain,
        v.allocation_region,v.occupancy.region,v.reference.place,
        v.reference.incarnation,v.owner_place);
    if (depth < 3)
        for(size_t i=0;i<v.field_count;i++)
            describe(c,v.fields[i],"owned-child",depth+1);
}
static void walk(const NLCheckedFragment *f,unsigned depth)
{
    if(!f || depth>18) return;
    for(NLCheckedNodeId id=1;id<=nl_checked_node_count(f);id++) {
        NLWholeValueCallView w={0};
        if(nl_checked_whole_value_call_view(f,id,&w)) {
            witnessed++;
            printf("R291_WHOLE witness=%u node=%zu depth=%u worlds(entry,returned,received)=%d,%d,%d count=%zu receiver=%zu result=%zu\n",
              witnessed,id,depth,!!w.entry,!!w.returned,!!w.received,
              w.count,w.receiver,w.result);
            for(size_t k=0;k<w.count;k++){
                NLSemanticBindingView donor={0};
                int exists=nl_semantic_binding_view(w.entry,w.donors[k],&donor);
                printf(" R291_DONOR k=%zu input=%zu symbol=%zu registered=%d available_at_entry=%d\n",
                  k,w.inputs[k],w.donors[k],exists,exists?(int)donor.availability:-1);
                describe(w.entry,w.inputs[k],"actual-entry",0);
            }
            describe(w.returned,w.result,"actual-returned",0);
            describe(w.received,w.result,"actual-received",0);
        }
        const NLCheckedFragment *b=nl_checked_call_body(f,id);
        if(b) walk(b,depth+1);
        for(size_t arm=0;arm<2;arm++) {
            b=nl_checked_match_arm(f,id,arm);
            if(b) walk(b,depth+1);
            b=nl_checked_if_arm(f,id,arm);
            if(b) walk(b,depth+1);
        }
    }
}
int main(int argc,char **argv)
{
    if(argc!=2) return 2;
    NLSource *source=NULL,*entrysrc=NULL;
    NLParser *parser=NULL,*entryparser=NULL;
    NLSyntaxTree *unit=NULL,*entrytree=NULL;
    NLSemanticContext *context=NULL;
    NLCheckedFragment *artifact=NULL;
    if(nl_source_load(argv[1],&source)!=NL_SOURCE_OK ||
       nl_parser_create(source,&parser)!=NL_PARSE_OK) return 10;
    NLParseDiagnostic pd={0};
    NLParseStatus p=nl_parser_parse_function_unit(parser,&unit,&pd);
    printf("R291_PARSE status=%d code=%s\n",(int)p,pd.diagnostic.code?pd.diagnostic.code:"none");
    if(p!=NL_PARSE_OK) return 11;
    if(nl_semantic_create(&context)!=NL_CHECK_OK) return 12;
    const NLSyntaxTree *inputs[]={unit};
    NLFunctionUnitDiagnostic ud={0};
    NLCheckStatus s=nl_semantic_register_function_unit(context,inputs,1,&ud);
    printf("R291_REGISTER status=%d code=%s\n",(int)s,ud.diagnostic.diagnostic.code?ud.diagnostic.diagnostic.code:"none");
    if(s!=NL_CHECK_OK)return 13;
    const char call[]="main()";
    if(nl_source_create(call,sizeof(call)-1,"r291-entry",&entrysrc)!=NL_SOURCE_OK ||
       nl_parser_create(entrysrc,&entryparser)!=NL_PARSE_OK) return 14;
    p=nl_parser_parse_expression_fragment(entryparser,&entrytree,&pd);
    if(p!=NL_PARSE_OK) return 15;
    NLCheckDiagnostic cd={0};
    s=nl_semantic_check_expression(context,entrytree,&artifact,&cd);
    printf("R291_CHECK status=%d code=%s owned_artifact=%d\n",(int)s,cd.diagnostic.code?cd.diagnostic.code:"none",artifact!=NULL);
    if(s!=NL_CHECK_OK)return 16;
    walk(artifact,0);
    printf("R291_WHOLE_TOTAL %u\n",witnessed);
    printf("R291_PUBLIC_VALIDATOR available_two_root=nl_checked_two_root_call_validate; whole_value_validator=NOT_DECLARED_IN_public_checked_h\n");
    nl_checked_destroy(artifact);
    nl_syntax_tree_destroy(entrytree);nl_parser_destroy(entryparser);
    nl_source_destroy(entrysrc);nl_semantic_destroy(context);
    nl_syntax_tree_destroy(unit);nl_parser_destroy(parser);
    nl_source_destroy(source);
    return witnessed ? 0 : 17;
}
