/* R #291: independent public source -> owned artifact observer.
 * Never seeds a semantic context, mutates an artifact or uses P fixtures. */
#include "newlang/checked.h"
#include "newlang/captured_closure.h"
/* Internals are used only to apply deliberate post-check corruption. */
#include "semantic_internal.h"
#include "newlang/parser.h"
#include "newlang/source.h"
#include "newlang/semantic.h"
#include <stdio.h>
#include <stdlib.h>

static unsigned witnessed = 0, certificates = 0;
static NLWholeValueCallView actual = {0};
static const NLCheckedFragment *outer = NULL, *inner = NULL;
static NLCheckedNodeId outer_match = 0;
static size_t outer_depth = (size_t)-1, inner_count = 0;
static unsigned clean_rejects = 0, poison_count = 0;

static NLCheckStatus recheck(void) {
    return nl_checked_captured_closure_validate(outer, outer_match);
}
static void poison(const char *name, size_t *target, size_t invalid) {
    if (!target || !outer) return;
    const size_t saved = *target;
    *target = invalid;
    const NLCheckStatus changed = recheck();
    *target = saved;
    const NLCheckStatus restored = recheck();
    const int rejected = changed != NL_CHECK_OK && restored == NL_CHECK_OK;
    clean_rejects += (unsigned)rejected;
    poison_count++;
    printf("R291_POISON name=%s changed=%d restored=%d rejected=%d\n",
           name, (int)changed, (int)restored, rejected);
}
static void attack(void) {
    if (!outer || !outer_match || !actual.entry || !actual.returned ||
        !actual.received || actual.result==0 || recheck()!=NL_CHECK_OK) {
        puts("R291_POISON_UNAVAILABLE baseline_or_world_missing");
        return;
    }
    NLSemanticContext *entry = (NLSemanticContext*)actual.entry;
    NLSemanticContext *returned = (NLSemanticContext*)actual.returned;
    NLSemanticContext *received = (NLSemanticContext*)actual.received;
    NLValueId whole=actual.result;
    NLValueId r0=entry->values[whole-1].fields[0];
    NLValueId r1=entry->values[whole-1].fields[1];
    if (!r0 || !r1) {puts("R291_POISON_UNAVAILABLE member_ids");return;}
    NLValueId a0=entry->values[r0-1].fields[1];
    NLValueId a1=entry->values[r1-1].fields[1];
    NLValueId d0=entry->values[r0-1].fields[2];
    NLValueId d1=entry->values[r1-1].fields[2];
    NLValueId p0=entry->values[r0-1].fields[0];
    NLValueId p1=entry->values[r1-1].fields[0];
    printf("R291_IDENTITY ptr0=%zu ptr1=%zu allocation0=%zu allocation1=%zu domain0=%zu domain1=%zu root0=%zu root1=%zu\n",
           p0,p1,a0,a1,d0,d1,r0,r1);
    poison("wrong_allocation_region",
           &received->values[a0-1].allocation_region,
           received->values[a1-1].allocation_region);
    poison("swapped_D_returned",
           &returned->values[r0-1].fields[2], d1);
    poison("missing_field",
           &returned->values[whole-1].fields[1], 0);
    poison("field_permutation",
           &returned->values[whole-1].fields[0], r1);
    poison("fabricated_D",
           &returned->values[d0-1].domain, returned->values[d1-1].domain+10);
    poison("fabricated_A",
           &returned->values[a0-1].allocation_region,
           returned->values[a1-1].allocation_region+10);
    poison("overwritten_O_R",
           &returned->values[p0-1].reference.place,
           returned->values[p1-1].reference.place);
    NLAvailability old = received->bindings[actual.donors[0]-1].view.availability;
    received->bindings[actual.donors[0]-1].view.availability = NL_AVAILABLE;
    NLCheckStatus duplicated = recheck();
    received->bindings[actual.donors[0]-1].view.availability = old;
    NLCheckStatus reset = recheck();
    int blocked=duplicated!=NL_CHECK_OK && reset==NL_CHECK_OK;
    poison_count++; clean_rejects+=(unsigned)blocked;
    printf("R291_POISON name=duplicate_donor_AVAILABLE changed=%d restored=%d rejected=%d\n",
           (int)duplicated,(int)reset,blocked);
    if(inner && inner->captured_closure && inner->captured_closure->view.count) {
        NLCapturedClosure *cl=((NLCheckedFragment*)inner)->captured_closure;
        poison("original_root_identity",
               &cl->view.originals[0].root,cl->view.originals[0].root+1);
        const NLSemanticContext *saved=cl->branches[0].entry_origin;
        cl->branches[0].entry_origin=cl->branches[1].entry;
        NLCheckStatus wrong=recheck();
        cl->branches[0].entry_origin=saved;
        NLCheckStatus restored=recheck();
        blocked=wrong!=NL_CHECK_OK && restored==NL_CHECK_OK;
        poison_count++;clean_rejects+=(unsigned)blocked;
        printf("R291_POISON name=forged_branch_origin changed=%d restored=%d rejected=%d\n",
               (int)wrong,(int)restored,blocked);
    }
    printf("R291_POISON_SUMMARY tried=%u rejected=%u baseline=%d\n",
           poison_count,clean_rejects,(int)recheck());
}

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
            actual=w;
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
        NLCapturedClosureView cert={0};
        if(nl_checked_captured_closure_view(f,id,&cert)) {
            certificates++;
            NLCheckStatus status=nl_checked_captured_closure_validate(f,id);
            printf("R291_PUBLIC_CAPTURED depth=%u node=%zu originals=%zu status=%d releases=",
                   depth,id,cert.count,(int)status);
            for(size_t j=0;j<6;j++)
                printf("%zu%s",cert.release_worlds[j],j==5?"":",");
            printf("\n");
            if(status==NL_CHECK_OK && depth<outer_depth) {
                outer=f;outer_match=id;outer_depth=depth;
            }
            if(status==NL_CHECK_OK && cert.count>inner_count) {
                inner=f;inner_count=cert.count;
            }
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
    printf("R291_PUBLIC_CERTIFICATES count=%u outer_depth=%zu inner_count=%zu\n",certificates,outer_depth,inner_count);
    attack();
    nl_checked_destroy(artifact);
    nl_syntax_tree_destroy(entrytree);nl_parser_destroy(entryparser);
    nl_source_destroy(entrysrc);nl_semantic_destroy(context);
    nl_syntax_tree_destroy(unit);nl_parser_destroy(parser);
    nl_source_destroy(source);
    return witnessed ? 0 : 17;
}
