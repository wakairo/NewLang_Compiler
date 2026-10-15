/* R437 fresh public-API-only checked-owned certificate and forgery tests. */
#include "newlang/source.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/checked.h"
#include "newlang/node_join.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const NLCheckedFragment *find_body(const NLCheckedFragment *f) {
    size_t n = nl_checked_node_count(f);
    for (size_t i=1; i<=n; ++i) {
        const NLCheckedNodeView *node=nl_checked_node_view(f,i);
        if (node && node->kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedFragment *body = nl_checked_call_body(f,i);
            if (body) return body;
        }
    }
    return NULL;
}
static int reject(const NLCheckedFragment *f, NLCheckedNodeId id,
                  NLNodeJoinView *v, const char *name) {
    NLCheckStatus st = nl_checked_node_join_validate(f,id,v);
    fprintf(stderr,"R437 PUBLIC MUTATION %s %d %s\n", name, (int)st,
            st != NL_CHECK_OK ? "REFUSED" : "ACCEPTED");
    return st != NL_CHECK_OK;
}
int main(int argc, char **argv) {
    if (argc != 2) return 80;
    NLSource *source=NULL,*query=NULL;
    NLParser *parser=NULL,*qparser=NULL;
    NLSyntaxTree *tree=NULL,*qtree=NULL;
    NLSemanticContext *context=NULL;
    NLCheckedFragment *entry=NULL;
    NLParseDiagnostic pdiag={0};
    NLCheckDiagnostic sdiag={0};
    if (nl_source_load(argv[1],&source) != NL_SOURCE_OK ||
        nl_parser_create(source,&parser) != NL_PARSE_OK ||
        nl_parser_parse_function_unit(parser,&tree,&pdiag) != NL_PARSE_OK ||
        nl_semantic_create(&context) != NL_CHECK_OK) {
        fprintf(stderr,"R437 API SETUP/PARSE BLOCKED\n");return 81;
    }
    const NLSyntaxTree *units[]={tree};
    NLFunctionUnitDiagnostic ud={0};
    NLCheckStatus st=nl_semantic_register_function_unit(context,units,1,&ud);
    if (st != NL_CHECK_OK) {
        fprintf(stderr,"R437 UNIT REGISTER BLOCKED %d\n",(int)st);return 82;
    }
    if (nl_source_create("main()",6,"R437 main call",&query) != NL_SOURCE_OK ||
        nl_parser_create(query,&qparser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(qparser,&qtree,&pdiag) != NL_PARSE_OK) {
        fprintf(stderr,"R437 CALL QUERY BLOCKED\n");return 83;
    }
    st=nl_semantic_check_expression(context,qtree,&entry,&sdiag);
    if (st != NL_CHECK_OK) {
        fprintf(stderr,"R437 MAIN CALL CHECK BLOCKED %d\n",(int)st);return 84;
    }
    const NLCheckedFragment *body=find_body(entry);
    if (!body) {fprintf(stderr,"R437 OWNED BODY NOT EXPOSED\n");return 85;}
    NLCheckedNodeId match=0;
    for(size_t i=1;i<=nl_checked_node_count(body);++i) {
        const NLCheckedNodeView *v=nl_checked_node_view(body,i);
        if(v && v->kind==NL_CHECKED_MATCH && v->normal_arms==2) {
            if(match) {fprintf(stderr,"R437 AMBIGUOUS MATCH COUNT\n");return 86;}
            match=i;
        }
    }
    if (!match) {fprintf(stderr,"R437 NO TWO-NORMAL MATCH\n");return 87;}
    NLNodeJoinView view={0};
    if (!nl_checked_node_join_view(body,match,&view)) {
        fprintf(stderr,"R437 PUBLIC VIEW ABSENT\n");return 88;
    }
    st=nl_checked_node_join_validate(body,match,NULL);
    if(st!=NL_CHECK_OK) {fprintf(stderr,"R437 ISSUED PROOF INVALID %d\n",(int)st);return 89;}
    st=nl_checked_node_join_validate(body,match,&view);
    if(st!=NL_CHECK_OK) {fprintf(stderr,"R437 COPIED PROOF INVALID %d\n",(int)st);return 90;}
    fprintf(stderr,"R437 VALIDATED issued/copied match=%zu original_places=%zu,%zu changed=%zu sibling=%zu operations=%zu,%zu\n",
            (size_t)match,(size_t)view.originals[0],(size_t)view.originals[1],
            (size_t)view.changed,(size_t)view.sibling,
            (size_t)view.arms[0].operation,(size_t)view.arms[1].operation);
    int rejected=0,attempted=0;
#define TEST(NAME,CHANGE) do { NLNodeJoinView bad=view; CHANGE; ++attempted; rejected+=reject(body,match,&bad,NAME); } while(0)
    TEST("same-root-alias", bad.originals[0]=view.originals[1]);
    TEST("wrong-changed-field", bad.changed=view.sibling);
    TEST("foreign-arm-artifact", bad.arms[0].artifact=NULL);
    TEST("wrong-arm-variant", bad.arms[0].variant=999);
    TEST("cross-arm-world", bad.arms[1].world=view.arms[0].world);
    TEST("missing-public-world", bad.arms[1].entry=NULL);
    size_t operative = view.arms[0].operation ? 0 : (view.arms[1].operation ? 1 : 2);
    if(operative != 2) {
        TEST("missing-changed-operation", bad.arms[operative].operation=0);
        TEST("stale-before-change", bad.arms[operative].before_change=view.arms[operative].after_change);
        TEST("bad-change-projection", bad.arms[operative].change.index=(size_t)-1);
    } else {fprintf(stderr,"R437 NO ACTUAL OPERATION IN PUBLIC VIEW\n");}
#undef TEST
    fprintf(stderr,"R437 PUBLIC MUTATIONS refused=%d attempted=%d\n",rejected,attempted);
    int result=(rejected==attempted && attempted>=9)?0:91;
    nl_checked_destroy(entry);
    nl_syntax_tree_destroy(qtree);nl_parser_destroy(qparser);nl_source_destroy(query);
    nl_syntax_tree_destroy(tree);nl_parser_destroy(parser);nl_source_destroy(source);
    nl_semantic_destroy(context);
    return result;
}
