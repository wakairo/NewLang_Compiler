/* R1023 independent source-only public-current probe. NEVER RUN in C120.
 * The only synthesized expression is the compiler's ordinary main() envelope;
 * the definition, helper calls and owned worlds come from the frozen source.
 */
#include "newlang/source.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/checked.h"
#include "newlang/copy_body.h"
#include <stdio.h>
typedef struct {unsigned calls,validated,missing,producers;} Counts;
static unsigned walked;
static void scan(const NLCheckedFragment *f,unsigned depth,Counts *s){
    if(!f || depth>24 || ++walked>8192){puts("R1023 NOT_OBSERVABLE traversal bound");return;}
    size_t n=nl_checked_node_count(f);
    for(NLCheckedNodeId id=1;id<=n;++id){
        const NLCheckedNodeView *v=nl_checked_node_view(f,id);
        if(!v)continue;
        if(v->kind==NL_CHECKED_REGISTERED_CALL){
            ++s->calls;
            NLCopyBodyView body={0};NLCopyBodyParameters p={0};
            bool b=nl_checked_copy_body_view(f,id,&body);
            bool q=nl_checked_copy_body_parameters(f,id,&p);
            printf("R1023 CALL depth=%u id=%zu function=%zu GET_BODY=%u GET_PARAMETERS=%u\n",
                   depth,(size_t)id,v->function,(unsigned)b,(unsigned)q);
            if(b&&q&&body.body==nl_checked_call_body(f,id)){
                NLCheckStatus actual=nl_checked_copy_body_validate(f,id,NULL);
                NLCheckStatus params=nl_checked_copy_body_parameters_validate(f,id,&p);
                bool valid=actual==NL_CHECK_OK&&params==NL_CHECK_OK&&body.event_count>0&&body.event_count<=64;
                printf("R1023 CURRENT id=%zu validate=%d parameters=%d events=%zu loan=%zu args=%zu,%zu parent=%zu child=%zu\n",
                    (size_t)id,(int)actual,(int)params,body.event_count,
                    (size_t)p.loan,(size_t)p.arguments[0],(size_t)p.arguments[1],
                    (size_t)p.field.parent,(size_t)p.field.child);
                for(size_t k=0;k<body.event_count&&k<64;++k){
                    NLCopyBodyEventView e={0};
                    bool got=nl_checked_copy_body_event(f,id,k,&e);
                    NLCheckStatus ev=nl_checked_copy_body_event_validate(f,id,k,NULL);
                    printf("R1023 EVENT id=%zu event=%zu GET=%u validate=%d kind=%d before=%u after=%u parent=%zu child=%zu old=%zu new=%zu\n",
                        (size_t)id,k,(unsigned)got,(int)ev,got?(int)e.kind:-1,
                        (unsigned)(got&&e.before!=NULL),(unsigned)(got&&e.after!=NULL),
                        got?(size_t)e.field.parent:0,got?(size_t)e.field.child:0,
                        got?(size_t)e.field.old_value:0,got?(size_t)e.field.new_value:0);
                    if(!got||ev!=NL_CHECK_OK)valid=false;
                }
                if(valid)++s->validated;
                else {++s->missing;puts("R1023 NOT_OBSERVABLE current validator incomplete or refused");}
            }else{++s->missing;puts("R1023 NOT_OBSERVABLE copy-body public GET at this actual call");}
            if(v->producer.entry_proved||v->producer.return_proved){
                bool ok=nl_checked_producer_valid(f,id);
                const NLSemanticContext *a=nl_checked_producer_entry(f,id);
                const NLSemanticContext *z=nl_checked_producer_return(f,id);
                printf("R1023 H_PRODUCER id=%zu valid=%u entry=%u returned=%u root=%zu incarnation=%zu domain=%zu\n",
                    (size_t)id,(unsigned)ok,(unsigned)(a!=NULL),(unsigned)(z!=NULL),
                    (size_t)v->producer.root,(size_t)v->producer.incarnation,(size_t)v->producer.domain);
                if(ok&&a&&z)++s->producers;
            }
            const NLCheckedFragment *callee=nl_checked_call_body(f,id);
            if(callee)scan(callee,depth+1,s);
        }
        if(v->kind==NL_CHECKED_MATCH){
            for(size_t k=0;k<16;++k){const NLCheckedFragment *arm=nl_checked_match_arm(f,id,k);
                if(!arm)break;scan(arm,depth+1,s);}
        }else if(v->kind==NL_CHECKED_IF){
            for(size_t k=0;k<2;++k){const NLCheckedFragment *arm=nl_checked_if_arm(f,id,k);
                if(arm)scan(arm,depth+1,s);}
        }else if(v->kind==NL_CHECKED_LOOP){
            const NLCheckedFragment *loop=nl_checked_loop_body(f,id);
            if(loop)scan(loop,depth+1,s);
        }
    }
}
int main(int argc,char **argv){
    if(argc!=2){fputs("R1023 probe: one immutable source path\n",stderr);return 64;}
    NLSource *src=NULL,*entry_source=NULL;
    NLParser *parser=NULL,*entry_parser=NULL;
    NLSyntaxTree *tree=NULL,*entry_tree=NULL;
    NLSemanticContext *context=NULL;
    NLCheckedFragment *owned=NULL;
    NLParseDiagnostic pd={0};NLFunctionUnitDiagnostic ud={0};NLCheckDiagnostic cd={0};
    int result=2;
    NLSourceStatus ss=nl_source_load(argv[1],&src);
    if(ss!=NL_SOURCE_OK){printf("R1023 UNREACHED SOURCE %d\n",(int)ss);goto done;}
    NLParseStatus ps=nl_parser_create(src,&parser);
    if(ps!=NL_PARSE_OK){printf("R1023 UNREACHED PARSER %d\n",(int)ps);goto done;}
    ps=nl_parser_parse_function_unit(parser,&tree,&pd);
    if(ps!=NL_PARSE_OK){printf("R1023 UNREACHED PARSE %d span=%zu\n",(int)ps,pd.span.start_byte);goto done;}
    NLCheckStatus cs=nl_semantic_create(&context);
    if(cs!=NL_CHECK_OK){printf("R1023 UNREACHED CONTEXT %d\n",(int)cs);goto done;}
    const NLSyntaxTree *inputs[]={tree};
    cs=nl_semantic_register_function_unit(context,inputs,1,&ud);
    if(cs!=NL_CHECK_OK){printf("R1023 UNREACHED REGISTER %d input=%zu\n",(int)cs,ud.input_index);goto done;}
    static const char call[]="main()";
    ss=nl_source_create(call,sizeof(call)-1,"<R1023-entry>",&entry_source);
    if(ss!=NL_SOURCE_OK){printf("R1023 UNREACHED ENTRY_SOURCE %d\n",(int)ss);goto done;}
    ps=nl_parser_create(entry_source,&entry_parser);
    if(ps!=NL_PARSE_OK){printf("R1023 UNREACHED ENTRY_PARSER %d\n",(int)ps);goto done;}
    ps=nl_parser_parse_expression_fragment(entry_parser,&entry_tree,&pd);
    if(ps!=NL_PARSE_OK){printf("R1023 UNREACHED ENTRY_PARSE %d\n",(int)ps);goto done;}
    cs=nl_semantic_check_expression(context,entry_tree,&owned,&cd);
    if(cs!=NL_CHECK_OK){printf("R1023 UNREACHED CHECK %d\n",(int)cs);goto done;}
    Counts counts={0};scan(owned,0,&counts);
    printf("R1023 SOURCE_OWNED calls=%u current_valid=%u current_missing=%u h_producer_valid=%u\n",
        counts.calls,counts.validated,counts.missing,counts.producers);
    if(!counts.validated)puts("R1023 NOT_OBSERVABLE current GET/VALIDATE");
    puts("R1023 NOT_OBSERVABLE physical H R/A/D or T original field/sibling: IDs are not C pointers");
    result=counts.validated?0:2;/* Never equate exit zero with physical/native PASS. */
done:
    nl_checked_destroy(owned); /* all borrowed worlds expire here */
    nl_syntax_tree_destroy(entry_tree);nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);nl_semantic_destroy(context);
    nl_syntax_tree_destroy(tree);nl_parser_destroy(parser);nl_source_destroy(src);
    return result;
}
