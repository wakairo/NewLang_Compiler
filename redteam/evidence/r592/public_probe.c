/* R592: independently authored public source-issued addition evidence probe.
 * No host seed_scalar, seed_root, host constructed checked fragment or P testcase.
 */
#include "newlang/source.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/checked.h"
#include "newlang/usize_addition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(C, S) do { if(!(C)) { fprintf(stderr,"R592_PROBE_FAIL %s line=%d\n",S,__LINE__); return 1; } } while(0)
static const char program[] =
    "fn main() -> unit {\n"
    "    let alpha = usize(37);\n"
    "    let beta = usize(59);\n"
    "    let answer = alpha + beta;\n"
    "    unit\n"
    "}\n";
int main(void) {
    NLSource *source=NULL,*caller=NULL;
    NLParser *parser=NULL,*cp=NULL;
    NLSyntaxTree *tree=NULL,*ctree=NULL;
    NLSemanticContext *ctx=NULL;
    NLCheckedFragment *call=NULL;
    NLParseDiagnostic pd={0};
    NLCheckDiagnostic cd={0};
    NLFunctionUnitDiagnostic fd={0};
    CHECK(nl_source_create(program,strlen(program),"r592_distinct_names.nl",&source)==NL_SOURCE_OK,"src create");
    CHECK(nl_parser_create(source,&parser)==NL_PARSE_OK,"parser create");
    NLParseStatus ps=nl_parser_parse_function_unit(parser,&tree,&pd);
    if (ps!=NL_PARSE_OK) fprintf(stderr,"REGISTER_PARSE_STATUS %d\n",(int)ps);
    CHECK(ps==NL_PARSE_OK,"program source parse");
    CHECK(nl_semantic_create(&ctx)==NL_CHECK_OK,"semantic ctx");
    const NLSyntaxTree *inputs[]={tree};
    NLCheckStatus cs=nl_semantic_register_function_unit(ctx,inputs,1,&fd);
    if (cs!=NL_CHECK_OK) fprintf(stderr,"REGISTER_STATUS %d\n",(int)cs);
    CHECK(cs==NL_CHECK_OK,"register actual function");
    nl_syntax_tree_destroy(tree); tree=NULL;
    nl_parser_destroy(parser); parser=NULL;
    nl_source_destroy(source); source=NULL; /* critical: original source/AST torn down */
    static const char invocation[]="main()";
    CHECK(nl_source_create(invocation,strlen(invocation),"r592_call",&caller)==NL_SOURCE_OK,"call source");
    CHECK(nl_parser_create(caller,&cp)==NL_PARSE_OK,"call parser");
    ps=nl_parser_parse_source_fragment(cp,&ctree,&pd);
    if (ps!=NL_PARSE_OK) fprintf(stderr,"CALL_PARSE_STATUS %d\n",(int)ps);
    CHECK(ps==NL_PARSE_OK,"call source parse");
    cs=nl_semantic_check_source_fragment(ctx,ctree,&call,&cd);
    if (cs!=NL_CHECK_OK) fprintf(stderr,"CALL_CHECK_STATUS %d\n",(int)cs);
    CHECK(cs==NL_CHECK_OK && call!=NULL,"source checked call");
    nl_syntax_tree_destroy(ctree); ctree=NULL;
    nl_parser_destroy(cp); cp=NULL;
    nl_source_destroy(caller); caller=NULL;
    const NLCheckedFragment *body=NULL;
    for (NLCheckedNodeId n=1;n<=nl_checked_node_count(call);++n) {
        const NLCheckedNodeView *v=nl_checked_node_view(call,n);
        if (v && v->kind==NL_CHECKED_REGISTERED_CALL) {
            body=nl_checked_call_body(call,n);
            if (body) break;
        }
    }
    CHECK(body!=NULL,"genuine registered checked body");
    NLCheckedNodeId add=0;
    for (NLCheckedNodeId n=1;n<=nl_checked_node_count(body);++n) {
        const NLCheckedNodeView *v=nl_checked_node_view(body,n);
        if (v && v->kind==NL_CHECKED_USIZE_ADDITION) {
            CHECK(add==0,"one flat addition");
            add=n;
        }
    }
    CHECK(add!=0,"source-issued checked addition");
    NLUsizeAdditionView event={0};
    CHECK(nl_checked_usize_addition_view(body,add,&event),"published view");
    CHECK(nl_checked_usize_addition_valid(body,add,NULL),"original event public validator");
    CHECK(nl_checked_usize_addition_valid(body,add,&event),"original view public validator");
    CHECK(event.numbers[0]==37 && event.numbers[1]==59 && event.sum==96,"original carriers and sum");
    CHECK(event.values[0]!=0 && event.values[1]!=0 && event.result!=0,"nonzero source values");
    CHECK(event.values[0]!=event.values[1] && event.result!=event.values[0] && event.result!=event.values[1],"fresh result");
    CHECK(event.symbols[0]!=0 && event.symbols[1]!=0 && event.symbols[0]!=event.symbols[1],"different original bindings");
    CHECK(event.facts[0]!=0 && event.facts[1]!=0,"historical ValueFacts");
    printf("R592_PROOF_BASELINE_PASS: operands=%zu,%zu result=%zu node=%zu IDs=%zu,%zu,%zu spans=%zu:%zu op=%zu:%zu\n",
            event.numbers[0],event.numbers[1],event.sum,add,event.values[0],event.values[1],event.result,
            event.span.start_byte,event.span.end_byte,event.operator_span.start_byte,event.operator_span.end_byte);
    int attacked=0;
#define MUTATE(EXPR,NAME) do { NLUsizeAdditionView copy=event; EXPR; CHECK(!nl_checked_usize_addition_valid(body,add,&copy),NAME); CHECK(nl_checked_usize_addition_valid(body,add,&event),"baseline restore"); ++attacked; printf("R592_TAMPER_REJECT %s\n",NAME); } while(0)
    MUTATE(copy.operands[0]=event.operands[1],"swap operand node");
    MUTATE(copy.values[0]=event.values[1],"swap operand value");
    MUTATE(copy.sum=97,"sum tamper");
    MUTATE(copy.result=event.values[0],"result identity");
    MUTATE(copy.type=0,"result type");
    MUTATE(copy.operator_span.start_byte++,"operator span");
    MUTATE(copy.span.end_byte--,"whole span");
    MUTATE(copy.numbers[0]++,"numeric operand");
    MUTATE(copy.facts[0]++,"ValueFact");
    MUTATE(copy.places[0]++,"historical place");
    MUTATE(copy.incarnations[1]++,"historical incarnation");
    MUTATE(copy.symbols[0]=event.symbols[1],"original binding swap");
    MUTATE(copy.numbers[0]=38;copy.sum=97,"coupled operand/sum");
    printf("R592_PROOF_BOUNDED_PASS tamper=%d (no claim internal owned event mutable, foreign source, or OOM)\n",attacked);
    nl_checked_destroy(call);
    nl_semantic_destroy(ctx);
    return 0;
}
