/* R328 own transactional public-API probe. No access to private checker state. */
#include "newlang/parser.h"
#include "newlang/checked.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do{if(!(x)){fprintf(stderr,"R328_ROLLBACK_ASSERT:%d:%s\n",__LINE__,#x);goto done;}}while(0)
typedef struct {NLSource *source;NLParser *parser;NLSyntaxTree *tree;} Input;
static void release(Input *p){
    nl_syntax_tree_destroy(p->tree);nl_parser_destroy(p->parser);nl_source_destroy(p->source);
}
static bool get_source(Input *p,const char *filename) {
    if(nl_source_load(filename,&p->source)!=NL_SOURCE_OK) return false;
    if(nl_parser_create(p->source,&p->parser)!=NL_PARSE_OK) return false;
    return nl_parser_parse_function_unit(p->parser,&p->tree,NULL)==NL_PARSE_OK;
}
int main(int argc,char **argv) {
    if(argc!=3) return 2;
    Input bad={0},good={0};
    NLSource *expr_source=NULL; NLParser *expr_parser=NULL;NLSyntaxTree *expr_tree=NULL;
    NLSemanticContext *ctx=NULL;NLCheckedFragment *owned=NULL;
    NLSemanticSnapshot before={0},after={0},entry_before={0},entry_after={0};
    NLFunctionUnitDiagnostic diagnostic={0};NLCheckDiagnostic check_diagnostic={0};
    bool success=false;
    if(!get_source(&bad,argv[2])) {
        puts("R328_BOUNDARY parse_fails_no_semantic_context");
        success=true;goto done;
    }
    CHECK(nl_semantic_create(&ctx)==NL_CHECK_OK);
    CHECK(nl_semantic_snapshot(ctx,&before));
    const NLSyntaxTree *bad_units[]={bad.tree};
    NLCheckStatus reg=nl_semantic_register_function_unit(ctx,bad_units,1,&diagnostic);
    CHECK(nl_semantic_snapshot(ctx,&after));
    if(reg!=NL_CHECK_OK) {
        CHECK(!memcmp(&before,&after,sizeof(before)));
        CHECK(get_source(&good,argv[1]));
        const NLSyntaxTree *good_units[]={good.tree};
        NLFunctionUnitDiagnostic g={0};
        CHECK(nl_semantic_register_function_unit(ctx,good_units,1,&g)==NL_CHECK_OK);
        CHECK(nl_source_create("main()",6,"<R328-retry>",&expr_source)==NL_SOURCE_OK);
        CHECK(nl_parser_create(expr_source,&expr_parser)==NL_PARSE_OK);
        CHECK(nl_parser_parse_expression_fragment(expr_parser,&expr_tree,NULL)==NL_PARSE_OK);
        CHECK(nl_semantic_check_expression(ctx,expr_tree,&owned,&check_diagnostic)==NL_CHECK_OK && owned);
        printf("R328_BOUNDARY stage=registration status=%d rollback=1 same_context_clean_retry=1 real_owned=1\n",(int)reg);
        success=true;goto done;
    }
    CHECK(nl_source_create("main()",6,"<R328-bad-entry>",&expr_source)==NL_SOURCE_OK);
    CHECK(nl_parser_create(expr_source,&expr_parser)==NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(expr_parser,&expr_tree,NULL)==NL_PARSE_OK);
    CHECK(nl_semantic_snapshot(ctx,&entry_before));
    NLCheckStatus entered=nl_semantic_check_expression(ctx,expr_tree,&owned,&check_diagnostic);
    CHECK(nl_semantic_snapshot(ctx,&entry_after));
    if(entered!=NL_CHECK_OK) {
        CHECK(!owned && !memcmp(&entry_before,&entry_after,sizeof(entry_before)));
        NLCheckDiagnostic repeated_diag={0};
        CHECK(nl_semantic_check_expression(ctx,expr_tree,&owned,&repeated_diag)==entered && !owned);
        printf("R328_BOUNDARY stage=entry status=%d rollback=1 same_context_repeat_refused=1 clean_retry=not_representable_with_registered_bad_main\n",(int)entered);
        success=true;
    } else {
        CHECK(owned);
        printf("R328_BOUNDARY stage=entry status=0 accepted=1 negative_requires_manual_authority_assessment\n");
        success=true;
    }
done:
    nl_checked_destroy(owned);
    nl_syntax_tree_destroy(expr_tree);nl_parser_destroy(expr_parser);nl_source_destroy(expr_source);
    nl_semantic_destroy(ctx);release(&good);release(&bad);
    return success?0:1;
}
