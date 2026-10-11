/*
 * Track R independent blind observer, Issue #318.
 * Only public include/newlang API. No compiler/test implementation copied.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "newlang/source.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/checked.h"
#include "newlang/captured_closure.h"
#include "newlang/raw_storage.h"
#include "r318_strict_audit.h"

typedef struct {
    unsigned matches, captured_success, captured_failure;
    unsigned scoped_calls, scoped_success, scoped_failure;
    unsigned direct_changes, change_snapshots, named_call_nodes;
    unsigned trace_index, audit_attempted, audit_ok, worlds_ok;
} Metrics;
static int same_snapshot(NLSemanticSnapshot a, NLSemanticSnapshot b) {
    return a.types==b.types && a.bindings==b.bindings &&
        a.values==b.values && a.places==b.places &&
        a.domains==b.domains && a.scopes==b.scopes &&
        a.functions==b.functions && a.occurrences==b.occurrences &&
        a.backing_regions==b.backing_regions &&
        a.raw_intervals==b.raw_intervals && a.last_incarnation==b.last_incarnation &&
        a.last_value_fact==b.last_value_fact;
}
static void check_change(const NLCheckedFragment *f, Metrics *m) {
    size_t n = nl_checked_captured_change_count(f);
    for (size_t i=0;i<n;i++) {
        NLCapturedChangeView v={0};
        if (!nl_checked_captured_change_view(f,i,&v) || !v.before || !v.after) {
            printf("CAPTURED_CHANGE invalid index=%zu\n",i);
            continue;
        }
        m->change_snapshots++;
        const NLCheckedNodeView *node=nl_checked_node_view(f,v.operation);
        printf("CAPTURED_CHANGE[%u] node=%zu kind=%d before=%p after=%p",
                ++m->trace_index, v.operation, node?(int)node->kind:-1,
                (const void *)v.before, (const void *)v.after);
        if(node) {
            printf(" field=%zu parent=%zu child=%zu old=%zu new=%zu occ_before=%zu occ_after=%zu",
                node->field.index,node->field.parent,node->field.child,
                node->field.old_value,node->field.new_value,
                node->field.payload_occurrence,node->field.post_payload_occurrence);
        }
        putchar('\n');
    }
}
static void inspect(const NLCheckedFragment *f, Metrics *m, unsigned depth) {
    if(!f || depth>32) {printf("INVALID fragment/depth\n");return;}
    check_change(f,m);
    if(nl_checked_captured_change_count(f)==7){m->audit_attempted++;if(r318_strict_audit(f))m->audit_ok++;}
    size_t count=nl_checked_node_count(f);
    for(size_t i=1;i<=count;i++){
        const NLCheckedNodeView *v=nl_checked_node_view(f,i);
        if(!v)continue;
        if(v->kind==NL_CHECKED_REPLACE) m->direct_changes++;
        if(v->kind==NL_CHECKED_REGISTERED_CALL) {
            m->named_call_nodes++;
            NLScopedFieldCallView cv={0};
            if(nl_checked_scoped_field_call_view(f,i,&cv)){
                m->scoped_calls++;
                NLCheckStatus st=nl_checked_scoped_field_call_validate(f,i);
                if(st==NL_CHECK_OK)m->scoped_success++;else m->scoped_failure++;
                printf("PUBLIC_SCOPED_VALIDATOR node=%zu status=%d projection=%zu operation=%zu\n",
                       i,(int)st,cv.projection,cv.change.operation);
            }
            const NLCheckedFragment *body=nl_checked_call_body(f,i);
            if(body)inspect(body,m,depth+1);
        }
        if(v->kind==NL_CHECKED_MATCH){
            m->matches++;
            NLCapturedClosureView c={0};
            if(nl_checked_captured_closure_view(f,i,&c)){
                bool worlds_valid=(c.count==depth && c.count<=4);
                for(size_t world=0;world<6;world++)
                    if(c.release_worlds[world]!=(world>=c.count?1u:0u))worlds_valid=false;
                for(size_t j=0;j<c.count;j++){
                    const NLCapturedOriginal *o=&c.originals[j];
                    if(o->extent.region!=j+1 || o->domain!=j+1 ||
                       o->allocation_availability!=NL_AVAILABLE ||
                       o->domain_availability!=NL_AVAILABLE)worlds_valid=false;
                }
                if(worlds_valid)m->worlds_ok++;
                else {m->captured_failure++;puts("R318_CAPTURED_RELEASE_WORLD_MISMATCH");}
                NLCheckStatus st=nl_checked_captured_closure_validate(f,i);
                if(st==NL_CHECK_OK)m->captured_success++;else m->captured_failure++;
                printf("PUBLIC_CAPTURED_VALIDATOR node=%zu status=%d count=%zu worlds=[%zu,%zu,%zu,%zu,%zu,%zu]\n",
                      i,(int)st,c.count,c.release_worlds[0],c.release_worlds[1],
                      c.release_worlds[2],c.release_worlds[3],c.release_worlds[4],
                      c.release_worlds[5]);
                for(size_t j=0;j<c.count;j++) {
                    const NLCapturedOriginal *o=&c.originals[j];
                    printf(" ORIGINAL[%zu] region=%zu root=%zu incarnation=%zu domain=%zu allocationValue=%zu(%d) domainValue=%zu(%d)\n",
                        j,o->extent.region,o->root,o->incarnation,o->domain,
                        o->allocation_value,(int)o->allocation_availability,
                        o->domain_value,(int)o->domain_availability);
                }
            }
            for(size_t arm=0;arm<2;arm++){
                const NLCheckedFragment *a=nl_checked_match_arm(f,i,arm);
                if(a)inspect(a,m,depth+1);
            }
        }
        if(v->kind==NL_CHECKED_IF) {
            for(size_t arm=0;arm<2;arm++) {
                const NLCheckedFragment *a=nl_checked_if_arm(f,i,arm);
                if(a)inspect(a,m,depth+1);
            }
        }
    }
}
int main(int argc,char **argv){
    if(argc!=2 && argc!=3){fprintf(stderr,"usage: %s witness.nl [clean-retry.nl]\n",argv[0]);return 2;}
    int verified=0,rejected=0;
    NLSource *src=NULL,*call_source=NULL,*clean_source=NULL;
    NLParser *parser=NULL,*call_parser=NULL,*clean_parser=NULL;
    NLSyntaxTree *tree=NULL,*calltree=NULL,*clean_tree=NULL;
    NLSemanticContext *ctx=NULL;
    NLCheckedFragment *owned=NULL;
    NLParseDiagnostic pd={0};NLCheckDiagnostic cd={0};
    NLFunctionUnitDiagnostic fd={0};
    NLParseStatus ps=0;NLCheckStatus st=0;
    if(nl_source_load(argv[1],&src)!=NL_SOURCE_OK){puts("SOURCE_LOAD_FAIL");return 3;}
    ps=nl_parser_create(src,&parser);
    if(ps!=NL_PARSE_OK){printf("PARSER_CREATE=%d\n",ps);goto end;}
    ps=nl_parser_parse_function_unit(parser,&tree,&pd);
    printf("PARSE_STATUS=%d\n",(int)ps);
    if(ps!=NL_PARSE_OK){if(pd.diagnostic.code)printf("PARSE_DIAG=%s at_bytes=%zu..%zu\n",pd.diagnostic.code,pd.span.start_byte,pd.span.end_byte);goto end;}
    st=nl_semantic_create(&ctx);
    if(st!=NL_CHECK_OK){printf("SEMANTIC_CREATE=%d\n",(int)st);goto end;}
    NLSemanticSnapshot base={0},after={0};
    nl_semantic_snapshot(ctx,&base);
    const NLSyntaxTree *inputs[1]={tree};
    st=nl_semantic_register_function_unit(ctx,inputs,1,&fd);
    printf("REGISTER_STATUS=%d\n",(int)st);
    if(st!=NL_CHECK_OK){
        printf("REGISTER_DIAG=%s bytes=%zu..%zu\n",fd.diagnostic.diagnostic.code?fd.diagnostic.diagnostic.code:"(none)",fd.diagnostic.span.start_byte,fd.diagnostic.span.end_byte);
        nl_semantic_snapshot(ctx,&after);
        bool rolled_back=same_snapshot(base,after);
        printf("REGISTER_ROLLBACK=%d\n",rolled_back);
        if(argc!=3 || !rolled_back)goto end;
        rejected=1;
        if(nl_source_load(argv[2],&clean_source)!=NL_SOURCE_OK)goto end;
        if(nl_parser_create(clean_source,&clean_parser)!=NL_PARSE_OK)goto end;
        if(nl_parser_parse_function_unit(clean_parser,&clean_tree,&pd)!=NL_PARSE_OK)goto end;
        const NLSyntaxTree *clean_input[1]={clean_tree};
        st=nl_semantic_register_function_unit(ctx,clean_input,1,&fd);
        printf("SAME_CONTEXT_RETRY_REGISTER=%d\n",(int)st);
        if(st!=NL_CHECK_OK)goto end;
    } else if(argc==3) {
        puts("R318_INVALID_SOURCE_UNEXPECTEDLY_ACCEPTED");
        goto end;
    }
    static const char call[]="main()";
    if(nl_source_create(call,sizeof(call)-1,"R318 call",&call_source)!=NL_SOURCE_OK)goto end;
    if(nl_parser_create(call_source,&call_parser)!=NL_PARSE_OK)goto end;
    if(nl_parser_parse_expression_fragment(call_parser,&calltree,&pd)!=NL_PARSE_OK){puts("CALL_PARSE_FAIL");goto end;}
    nl_semantic_snapshot(ctx,&base);
    st=nl_semantic_check_expression(ctx,calltree,&owned,&cd);
    printf("MAIN_CHECK_STATUS=%d OWNED=%d\n",(int)st,owned!=NULL);
    if(st!=NL_CHECK_OK){
        printf("MAIN_DIAG=%s bytes=%zu..%zu\n",cd.diagnostic.code?cd.diagnostic.code:"(none)",cd.span.start_byte,cd.span.end_byte);
        nl_semantic_snapshot(ctx,&after);
        printf("CHECK_ROLLBACK=%d\n",same_snapshot(base,after));
        goto end;
    }
    Metrics m={0};
    inspect(owned,&m,0);
    printf("R318_SUMMARY matches=%u public_captured_ok=%u captured_fail=%u public_scoped_calls=%u scoped_ok=%u scoped_fail=%u named_call_nodes=%u replace_nodes=%u captured_change_snapshots=%u\n",
        m.matches,m.captured_success,m.captured_failure,m.scoped_calls,m.scoped_success,m.scoped_failure,m.named_call_nodes,m.direct_changes,m.change_snapshots);
    if(m.captured_success!=5 || m.scoped_success!=2 || m.scoped_failure!=0 || m.captured_failure!=0 || m.scoped_calls!=2 || m.named_call_nodes<2 || m.change_snapshots<7 || m.audit_attempted!=1 || m.audit_ok!=1 || m.worlds_ok!=5) { puts("R318_POSITIVE_NOT_PROVEN"); goto end; }
    puts("R318_BASIC_PUBLIC_VALIDATORS_OK");
    puts("R318_FIVE_STATIC_RELEASE_WORLDS_OK");
    if(rejected)puts("R318_REJECT_ROLLBACK_SAME_CONTEXT_RETRY_OK");
    verified=1;
end:
    nl_checked_destroy(owned);
    nl_semantic_destroy(ctx);
    nl_syntax_tree_destroy(calltree);nl_parser_destroy(call_parser);nl_source_destroy(call_source);
    nl_syntax_tree_destroy(tree);nl_parser_destroy(parser);nl_source_destroy(src);
    nl_syntax_tree_destroy(clean_tree);nl_parser_destroy(clean_parser);nl_source_destroy(clean_source);
    return verified?0:1; /* Positives, refusals and same-context retry must pass. */
}
