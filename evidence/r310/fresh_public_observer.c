#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "newlang/source.h"
#include "newlang/parser.h"
#include "newlang/syntax.h"
#include "newlang/semantic.h"
#include "newlang/checked.h"

static int snapshot_equal(const NLSemanticSnapshot *a, const NLSemanticSnapshot *b) {
    return a->types == b->types && a->bindings == b->bindings &&
      a->values == b->values && a->places == b->places &&
      a->domains == b->domains && a->scopes == b->scopes &&
      a->functions == b->functions &&
      a->last_incarnation == b->last_incarnation &&
      a->last_value_fact == b->last_value_fact &&
      a->backing_regions == b->backing_regions &&
      a->raw_intervals == b->raw_intervals &&
      a->occurrences == b->occurrences;
}
static unsigned found_prev = 0, found_other = 0, found_calls = 0, found_matches = 0;
static unsigned found_replacements = 0, found_loan = 0, found_allocs = 0;
static void inspect(const NLCheckedFragment *f, unsigned depth) {
    if (!f || depth > 14) return;
    size_t count = nl_checked_node_count(f);
    if (depth <= 2) {
      printf("R310_OWNED_FRAGMENT depth=%u root=%zu nodes=%zu context=%d\n",
        depth, nl_checked_root(f), count, nl_checked_context(f)!=NULL);
    }
    if (count > 20000) return;
    for (NLCheckedNodeId id=1; id<=count; ++id) {
      const NLCheckedNodeView *v = nl_checked_node_view(f,id);
      if(!v) continue;
      if(v->kind == NL_CHECKED_FIELD_REF && v->field.present) {
        if(v->field.index==1) ++found_prev; else ++found_other;
        if(found_prev+found_other < 35)
          printf("R310_FIELD depth=%u id=%zu index=%zu parent=%zu child=%zu parent_inc=%zu child_inc=%zu dep=%d old_occ=%zu post_occ=%zu\n",
            depth,id,v->field.index,v->field.parent,v->field.child,
            v->field.parent_incarnation,v->field.child_incarnation,
            v->field.dependency_compatible,v->field.payload_occurrence,
            v->field.post_payload_occurrence);
      }
      if(v->kind == NL_CHECKED_REPLACE) ++found_replacements;
      if(v->kind == NL_CHECKED_LOAN_HEADER || v->kind == NL_CHECKED_REF_FROM_PTR) ++found_loan;
      if(v->kind == NL_CHECKED_TRY_ALLOCATE_ONE) ++found_allocs;
      if(v->kind == NL_CHECKED_MATCH) {
        ++found_matches;
        for(size_t arm=0;arm<2;++arm) inspect(nl_checked_match_arm(f,id,arm),depth+1);
      }
      if(v->kind == NL_CHECKED_REGISTERED_CALL) {
        ++found_calls;
        inspect(nl_checked_call_body(f,id),depth+1);
      }
    }
}
int main(int argc,char **argv) {
  if(argc!=2) return 99;
  NLSource *source=NULL;
  NLSourceStatus sr=nl_source_load(argv[1], &source);
  printf("R310_SOURCE_LOAD=%d path=%s\n",(int)sr,argv[1]);
  if(sr!=0) return 11;
  NLParser *parser=NULL;
  NLParseStatus pc=nl_parser_create(source,&parser);
  printf("R310_PARSER_CREATE=%d\n",(int)pc);
  if(pc!=NL_PARSE_OK) return 12;
  NLSyntaxTree *unit=NULL;
  NLParseDiagnostic pd={0};
  NLParseStatus ps=nl_parser_parse_function_unit(parser,&unit,&pd);
  printf("R310_PARSE=%d UNIT_NONNULL=%d\n",(int)ps,unit!=NULL);
  if(ps!=NL_PARSE_OK) {nl_parse_diagnostic_render(stdout,source,&pd);goto cleanup_parser;}
  NLSemanticContext *ctx=NULL;
  NLCheckStatus sc=nl_semantic_create(&ctx);
  printf("R310_SEM_CREATE=%d\n",(int)sc);
  if(sc!=NL_CHECK_OK) goto cleanup_parser;
  NLSemanticSnapshot before={0}, after={0};
  nl_semantic_snapshot(ctx,&before);
  const NLSyntaxTree *input[1]={unit};
  NLFunctionUnitDiagnostic diag={0};
  NLCheckStatus reg=nl_semantic_register_function_unit(ctx,input,1,&diag);
  nl_semantic_snapshot(ctx,&after);
  printf("R310_REGISTER=%d SNAPSHOT_UNCHANGED=%d BEFORE{types=%zu values=%zu roots=%zu domains=%zu functions=%zu} AFTER{types=%zu values=%zu roots=%zu domains=%zu functions=%zu}\n",
    (int)reg,snapshot_equal(&before,&after),before.types,before.values,
    before.places,before.domains,before.functions,after.types,after.values,
    after.places,after.domains,after.functions);
  if(reg!=NL_CHECK_OK) {
    nl_check_diagnostic_render(stdout,source,&diag.diagnostic);
    printf("R310_NO_SOURCE_OWNED_ARTIFACT register_failed\n");
  } else {
    NLSource *call_src=NULL; NLParser *call_parser=NULL;
    NLSyntaxTree *call_tree=NULL; NLCheckedFragment *owned=NULL;
    NLSourceStatus cr=nl_source_create("main()",6,"r310-invocation",&call_src);
    printf("R310_CALL_SOURCE=%d\n",(int)cr);
    if(cr==0) {
      NLParseStatus cp=nl_parser_create(call_src,&call_parser);
      printf("R310_CALL_PARSER=%d\n",(int)cp);
      if(cp==NL_PARSE_OK) {
        NLParseDiagnostic cpd={0};
        NLParseStatus ce=nl_parser_parse_expression_fragment(call_parser,&call_tree,&cpd);
        printf("R310_CALL_PARSE=%d\n",(int)ce);
        if(ce==NL_PARSE_OK) {
          NLCheckDiagnostic cd={0};
          NLCheckStatus ck=nl_semantic_check_expression(ctx,call_tree,&owned,&cd);
          printf("R310_CALL_CHECK=%d OWNED_NONNULL=%d\n",(int)ck,owned!=NULL);
          if(ck!=NL_CHECK_OK) nl_check_diagnostic_render(stdout,call_src,&cd);
          if(owned){
            inspect(owned,0);
            printf("R310_OWNED_COUNTS calls=%u matches=%u allocs=%u field_prev_index1=%u field_siblings=%u replace=%u loans=%u\n",
              found_calls,found_matches,found_allocs,found_prev,found_other,
              found_replacements,found_loan);
            printf("R310_SPECIALIZED_PUBLIC_VALIDATOR_NOT_APPLICABLE_TO_GENERIC_PREV_SCOPED_PROFILE\n");
          }
        }
      }
    }
    nl_checked_destroy(owned);
    nl_syntax_tree_destroy(call_tree);
    nl_parser_destroy(call_parser);
    nl_source_destroy(call_src);
  }
  nl_semantic_destroy(ctx);
cleanup_parser:
  nl_syntax_tree_destroy(unit);
  nl_parser_destroy(parser);
  nl_source_destroy(source);
  return 0;
}
