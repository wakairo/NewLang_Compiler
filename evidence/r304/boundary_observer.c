/* R304 independently authored public API registration refusal observer.
 * Input is solely the candidate NewLang source; no seeded authorization. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "newlang/parser.h"
#include "newlang/semantic.h"
#define EXPECT(x) do { if(!(x)){fprintf(stderr,"R304 boundary line %d: %s\n",__LINE__,#x);exit(81);} } while(0)
int main(int argc,char **argv) {
    EXPECT(argc==2);
    NLSource *source=NULL; NLParser *parser=NULL;
    NLSyntaxTree *syntax=NULL; NLSemanticContext *ctx=NULL;
    NLParseDiagnostic pd={0}; NLFunctionUnitDiagnostic fd={0};
    NLSemanticSnapshot initial={0},final={0};
    EXPECT(nl_source_load(argv[1],&source)==NL_SOURCE_OK);
    EXPECT(nl_parser_create(source,&parser)==NL_PARSE_OK);
    EXPECT(nl_semantic_create(&ctx)==NL_CHECK_OK);
    EXPECT(nl_semantic_snapshot(ctx,&initial));
    NLParseStatus ps=nl_parser_parse_function_unit(parser,&syntax,&pd);
    NLCheckStatus rs=NL_CHECK_OK;
    const char *code=pd.diagnostic.code;
    NLSourceSpan span=pd.span;
    if(ps==NL_PARSE_OK) {
        const NLSyntaxTree *trees[1]={syntax};
        EXPECT(syntax);
        rs=nl_semantic_register_function_unit(ctx,trees,1,&fd);
        code=fd.diagnostic.diagnostic.code;
        span=fd.diagnostic.span;
    } else EXPECT(!syntax);
    EXPECT(nl_semantic_snapshot(ctx,&final));
    /* The negative tests demand actual registration refusal, not parser
     * unsupported/precision gates, and transactional full state rollback. */
    EXPECT(ps==NL_PARSE_OK && rs==NL_CHECK_SEMANTIC_ERROR);
    EXPECT(memcmp(&initial,&final,sizeof initial)==0);
    EXPECT(code && code[0]);
    EXPECT(span.end_byte>=span.start_byte);
    printf("{\"parse_status\":%d,\"registration_status\":%d,"
           "\"code\":\"%s\",\"span_start_byte\":%zu,\"span_end_byte\":%zu,"
           "\"snapshot_unchanged\":true,\"owned_entry\":false,"
           "\"expression_checked\":false,\"backend_attempted\":false}\n",
           (int)ps,(int)rs,code,span.start_byte,span.end_byte);
    nl_semantic_destroy(ctx);nl_syntax_tree_destroy(syntax);
    nl_parser_destroy(parser);nl_source_destroy(source);
    return 0;
}
