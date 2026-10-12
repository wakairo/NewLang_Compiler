/*
 * Track R / Issue 359: independent read-only public-API source observer.
 * This is a host C17 test program, never NewLang-emitted native code.
 * It does not inspect or mutate private compiler state.
 */
#include "newlang/source.h"
#include "newlang/parser.h"
#include "newlang/syntax.h"
#include "newlang/semantic.h"
#include "newlang/checked.h"
#include "newlang/captured_closure.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static size_t total_replace, total_changes, closure_seen, closure_ok;
static size_t old_new_distinct, old_new_same, closure_bad;
static size_t max_depth;
static const NLCheckedFragment *visited[4096];
static size_t visited_len;
static int failed_invariant;

static const char *level_name(NLCheckStatus s) {
    switch (s) {
        case NL_CHECK_OK: return "OK";
        case NL_CHECK_SEMANTIC_ERROR: return "SEMANTIC_ERROR";
        case NL_CHECK_SEMANTIC_UNSUPPORTED: return "SEMANTIC_UNSUPPORTED";
        case NL_CHECK_ANALYSIS_PRECISION_LIMIT: return "ANALYSIS_PRECISION_LIMIT";
        case NL_CHECK_OUT_OF_MEMORY: return "OUT_OF_MEMORY";
        case NL_CHECK_RESOURCE_LIMIT: return "RESOURCE_LIMIT";
        case NL_CHECK_INTERNAL_ERROR: return "INTERNAL_ERROR";
    }
    return "INVALID";
}
static const char *parse_name(NLParseStatus s) {
    switch (s) {
        case NL_PARSE_OK: return "OK";
        case NL_PARSE_LEXICALLY_UNSUPPORTED: return "LEXICALLY_UNSUPPORTED";
        case NL_PARSE_SYNTAX_UNSUPPORTED: return "SYNTAX_UNSUPPORTED";
        case NL_PARSE_SYNTAX_ERROR: return "SYNTAX_ERROR";
        case NL_PARSE_OUT_OF_MEMORY: return "OUT_OF_MEMORY";
        case NL_PARSE_RESOURCE_LIMIT: return "RESOURCE_LIMIT";
        case NL_PARSE_INTERNAL_ERROR: return "INTERNAL_ERROR";
    }
    return "INVALID";
}

/* Re-check unaffected siblings using independently owned public pre/post worlds. */
static void verify_transition(const NLCapturedChangeView *cv,
                              const NLCheckedNodeView *op,
                              size_t depth, size_t ordinal) {
    if (!cv->before || !cv->after || !op || op->kind != NL_CHECKED_REPLACE) {
        puts("TRANSITION_ERROR absent independent public before/after");
        failed_invariant = 1; return;
    }
    NLSemanticPlaceView bp = {0}, ap = {0};
    if (!nl_semantic_place_view(cv->before, op->field.parent, &bp) ||
        !nl_semantic_place_view(cv->after, op->field.parent, &ap) ||
        !bp.live || !ap.live || bp.independent_root != ap.independent_root ||
        !bp.independent_root || bp.incarnation != ap.incarnation ||
        bp.governing_domain == 0 || bp.governing_domain != ap.governing_domain ||
        bp.placement.region == 0 || bp.placement.region != ap.placement.region ||
        bp.fixed_field_count != 4 || ap.fixed_field_count != 4 ||
        op->field.index >= 3) {
        printf("TRANSITION_ERROR depth=%zu ordinal=%zu root_identity\n",
               depth, ordinal);
        failed_invariant = 1; return;
    }
    size_t stable_siblings = 0;
    for (size_t j = 0; j < 4; ++j) {
        NLPlaceId before_id = bp.fixed_fields[j], after_id = ap.fixed_fields[j];
        NLSemanticPlaceView bc = {0}, ac = {0};
        if (!before_id || before_id != after_id ||
            !nl_semantic_place_view(cv->before, before_id, &bc) ||
            !nl_semantic_place_view(cv->after, after_id, &ac) ||
            !bc.live || !ac.live || bc.incarnation != ac.incarnation ||
            bc.parent_aggregate != op->field.parent ||
            ac.parent_aggregate != op->field.parent) {
            printf("TRANSITION_ERROR depth=%zu ordinal=%zu field=%zu identity\n",
                   depth, ordinal, j);
            failed_invariant = 1; continue;
        }
        printf("FIELD_FACT depth=%zu ordinal=%zu field=%zu before=%zu after=%zu"
               " oldocc=%zu newocc=%zu\n",
               depth, ordinal, j, (size_t)bc.current_fact,
               (size_t)ac.current_fact, (size_t)bc.payload_occurrence,
               (size_t)ac.payload_occurrence);
        if (j == op->field.index) {
            if (!bc.current_fact || !ac.current_fact ||
                bc.current_fact == ac.current_fact ||
                ac.payload_occurrence == 0 ||
                ac.payload_occurrence == bc.payload_occurrence) {
                printf("TRANSITION_ERROR depth=%zu ordinal=%zu target_change\n",
                       depth, ordinal);
                failed_invariant = 1;
            }
            NLSemanticOccurrenceView fresh = {0};
            if (!nl_semantic_occurrence_view(cv->after,
                                             ac.payload_occurrence, &fresh) ||
                !fresh.live || fresh.root != after_id) {
                printf("TRANSITION_ERROR depth=%zu ordinal=%zu fresh_occurrence\n",
                       depth, ordinal);
                failed_invariant = 1;
            }
            if (bc.payload_occurrence) {
                NLSemanticOccurrenceView old_pre = {0}, old_post = {0};
                if (!nl_semantic_occurrence_view(cv->before,
                                                 bc.payload_occurrence, &old_pre) ||
                    !nl_semantic_occurrence_view(cv->after,
                                                 bc.payload_occurrence, &old_post) ||
                    !old_pre.live || old_post.live || old_pre.root != before_id) {
                    printf("TRANSITION_ERROR depth=%zu ordinal=%zu old_end\n",
                           depth, ordinal);
                    failed_invariant = 1;
                }
            }
        } else {
            if (!bc.current_fact || bc.current_fact != ac.current_fact ||
                bc.payload_occurrence != ac.payload_occurrence) {
                printf("TRANSITION_ERROR depth=%zu ordinal=%zu field=%zu sibling_stale\n",
                       depth, ordinal, j);
                failed_invariant = 1;
            } else {
                ++stable_siblings;
            }
            if (j == 3) {
                NLSemanticValueView bv = {0}, av = {0};
                if (!nl_semantic_value_view(cv->before, bc.current_value, &bv) ||
                    !nl_semantic_value_view(cv->after, ac.current_value, &av) ||
                    !bv.scalar_known || !av.scalar_known ||
                    bv.scalar_value != av.scalar_value) {
                    printf("TRANSITION_ERROR depth=%zu ordinal=%zu scalar_payload\n",
                           depth, ordinal);
                    failed_invariant = 1;
                }
            }
        }
    }
    printf("TRANSITION depth=%zu ordinal=%zu originalR=%zu originalD=%zu"
           " originalO=%zu rootinc=%zu siblings_current=%zu/3\n",
           depth, ordinal, (size_t)bp.placement.region,
           (size_t)bp.governing_domain, (size_t)op->field.parent,
           (size_t)bp.incarnation, stable_siblings);
    if (stable_siblings != 3) failed_invariant = 1;
}

static void examine(const NLCheckedFragment *f, size_t depth) {
    if (!f) return;
    if (depth > max_depth) max_depth = depth;
    if (depth > 128 || visited_len >= 4096) { failed_invariant = 1; return; }
    for (size_t j = 0; j < visited_len; ++j) if (visited[j] == f) return;
    visited[visited_len++] = f;
    size_t n = nl_checked_node_count(f);
    printf("ARTIFACT depth=%zu root=%zu nodes=%zu\n", depth,
           (size_t)nl_checked_root(f), n);
    size_t nc = nl_checked_captured_change_count(f);
    printf("CAPTURED_CHANGES depth=%zu count=%zu\n", depth, nc);
    total_changes += nc;
    for (size_t i = 0; i < nc; ++i) {
        NLCapturedChangeView cv = {0};
        if (!nl_checked_captured_change_view(f, i, &cv)) {
            printf("CAPTURE_ERROR depth=%zu ordinal=%zu\n", depth, i);
            failed_invariant = 1; continue;
        }
        const NLCheckedNodeView *v = nl_checked_node_view(f, cv.operation);
        printf("CAPTURE depth=%zu ordinal=%zu node=%zu world=%p before=%p after=%p"
               " opkind=%d fieldidx=%zu place=%zu incarnation=%zu"
               " oldvalue=%zu newvalue=%zu oldocc=%zu newocc=%zu"
               " parentfact=%zu postparentfact=%zu childfact=%zu postchildfact=%zu\n",
               depth, i, (size_t)cv.operation, (const void *)cv.world,
               (const void *)cv.before, (const void *)cv.after,
               v ? (int)v->kind : -1, v ? v->field.index : 0,
               v ? (size_t)v->field.child : 0,
               v ? (size_t)v->field.parent_incarnation : 0,
               v ? (size_t)v->field.old_value : 0,
               v ? (size_t)v->field.new_value : 0,
               v ? (size_t)v->field.payload_occurrence : 0,
               v ? (size_t)v->field.post_payload_occurrence : 0,
               v ? (size_t)v->field.parent_fact : 0,
               v ? (size_t)v->field.parent_post_fact : 0,
               v ? (size_t)v->field.child_fact : 0,
               v ? (size_t)v->field.child_post_fact : 0);
        if (v && v->kind != NL_CHECKED_REPLACE) failed_invariant = 1;
        verify_transition(&cv, v, depth, i);
        if (v && v->field.payload_occurrence && v->field.post_payload_occurrence) {
            if (v->field.payload_occurrence != v->field.post_payload_occurrence) {
                ++old_new_distinct;
            } else {
                ++old_new_same;
                failed_invariant = 1;
            }
        }
    }
    for (size_t id = 1; id <= n; ++id) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        if (!v) continue;
        if (v->kind == NL_CHECKED_REPLACE) {
            ++total_replace;
            printf("REPLACE depth=%zu node=%zu span=%zu:%zu"
                   " field=%zu present=%d compatible=%d"
                   " parent=%zu child=%zu parentinc=%zu childinc=%zu\n",
                   depth, id, v->span.start_byte, v->span.end_byte,
                   v->field.index, (int)v->field.present,
                   (int)v->field.dependency_compatible,
                   (size_t)v->field.parent, (size_t)v->field.child,
                   (size_t)v->field.parent_incarnation,
                   (size_t)v->field.child_incarnation);
        }
        if (v->kind == NL_CHECKED_MATCH) {
            NLCapturedClosureView view = {0};
            if (nl_checked_captured_closure_view(f, id, &view)) {
                NLCheckStatus s = nl_checked_captured_closure_validate(f, id);
                ++closure_seen;
                if (s == NL_CHECK_OK) ++closure_ok; else ++closure_bad;
                printf("CLOSURE depth=%zu match=%zu status=%s originals=%zu"
                       " ancestor=%p post=%p releases=[",
                       depth, id, level_name(s), view.count,
                       (const void *)view.ancestor, (const void *)view.closed_post);
                for (size_t w = 0; w <= NL_CAPTURED_MAX_RELEASES; ++w)
                    printf("%s%zu", w ? "," : "", view.release_worlds[w]);
                puts("]");
                for (size_t j = 0; j < view.count; ++j) {
                    const NLCapturedOriginal *o = &view.originals[j];
                    printf("ORIGINAL match=%zu i=%zu world=%p R=%zu extent=%zu+%zu"
                           " O=%zu inc=%zu D=%zu A_binding=%zu D_binding=%zu"
                           " A_value=%zu D_value=%zu A_avail=%d D_avail=%d\n",
                           id, j, (const void *)o->origin,
                           (size_t)o->extent.region, o->extent.start,
                           o->extent.length, (size_t)o->root,
                           (size_t)o->incarnation, (size_t)o->domain,
                           (size_t)o->allocation_binding,
                           (size_t)o->domain_binding,
                           (size_t)o->allocation_value,
                           (size_t)o->domain_value,
                           (int)o->allocation_availability,
                           (int)o->domain_availability);
                }
            }
            for (size_t j = 0; j < 2; ++j) {
                const NLCheckedFragment *a = nl_checked_match_arm(f, id, j);
                if (a) examine(a, depth + 1);
            }
        }
        if (v->kind == NL_CHECKED_IF) {
            for (size_t j = 0; j < 2; ++j) {
                const NLCheckedFragment *a = nl_checked_if_arm(f, id, j);
                if (a) examine(a, depth + 1);
            }
        }
        if (v->kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedFragment *b = nl_checked_call_body(f, id);
            if (b) examine(b, depth + 1);
        }
    }
}
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: observer POS|NEG path.nl\n");
        return 64;
    }
    const int positive = strcmp(argv[1], "POS") == 0;
    NLSource *source = NULL, *call_src = NULL;
    NLParser *parser = NULL, *call_parser = NULL;
    NLSyntaxTree *unit = NULL, *call = NULL;
    NLSemanticContext *ctx = NULL;
    NLCheckedFragment *owned = NULL;
    NLParseDiagnostic pd = {0};
    NLFunctionUnitDiagnostic fd = {0};
    NLCheckDiagnostic cd = {0};
    int exitcode = 70;
    NLSourceStatus sr = nl_source_load(argv[2], &source);
    printf("SOURCE path=%s status=%d\n", argv[2], (int)sr);
    if (sr != NL_SOURCE_OK) goto done;
    NLParseStatus ps = nl_parser_create(source, &parser);
    printf("PARSE_CREATE status=%s\n", parse_name(ps));
    if (ps != NL_PARSE_OK) goto done;
    ps = nl_parser_parse_function_unit(parser, &unit, &pd);
    printf("PARSE_UNIT status=%s span=%zu:%zu diagnostic=%s\n",
           parse_name(ps), pd.span.start_byte, pd.span.end_byte,
           pd.diagnostic.code ? pd.diagnostic.code : "(none)");
    if (ps != NL_PARSE_OK) { exitcode = positive ? 20 : 0; goto done; }
    NLCheckStatus s = nl_semantic_create(&ctx);
    printf("CONTEXT_CREATE status=%s\n", level_name(s));
    if (s != NL_CHECK_OK) goto done;
    const NLSyntaxTree *units[] = {unit};
    s = nl_semantic_register_function_unit(ctx, units, 1, &fd);
    printf("REGISTER status=%s input=%zu span=%zu:%zu diagnostic=%s\n",
           level_name(s), fd.input_index, fd.diagnostic.span.start_byte,
           fd.diagnostic.span.end_byte,
           fd.diagnostic.diagnostic.code ? fd.diagnostic.diagnostic.code : "(none)");
    if (s != NL_CHECK_OK) { exitcode = positive ? 21 : 0; goto done; }
    static const char main_call[] = "main()";
    sr = nl_source_create(main_call, sizeof(main_call)-1, "r359-public-main()", &call_src);
    if (sr != NL_SOURCE_OK) goto done;
    ps = nl_parser_create(call_src, &call_parser);
    if (ps != NL_PARSE_OK) goto done;
    ps = nl_parser_parse_expression_fragment(call_parser, &call, &pd);
    printf("PARSE_CALL status=%s\n", parse_name(ps));
    if (ps != NL_PARSE_OK) goto done;
    s = nl_semantic_check_expression(ctx, call, &owned, &cd);
    printf("CHECK_MAIN status=%s span=%zu:%zu diagnostic=%s owned=%p\n",
           level_name(s), cd.span.start_byte, cd.span.end_byte,
           cd.diagnostic.code ? cd.diagnostic.code : "(none)",
           (void *)owned);
    if (s != NL_CHECK_OK) {
        exitcode = positive ? 22 : (owned ? 50 : 0);
        goto done;
    }
    if (!positive) {
        puts("FINDINGS: mutated source accepted by public checker");
        exitcode = 51; goto done;
    }
    if (!owned) { exitcode = 23; goto done; }
    examine(owned, 0);
    printf("SUMMARY replaced=%zu captured=%zu closure_seen=%zu closure_ok=%zu"
           " closure_bad=%zu changed_old_to_fresh=%zu stale=%zu"
           " fragments=%zu depth=%zu invariant_failure=%d\n",
           total_replace,total_changes,closure_seen,closure_ok,closure_bad,
           old_new_distinct,old_new_same,visited_len,max_depth,failed_invariant);
    exitcode = (total_replace == 9 && total_changes == 9
                && closure_seen >= 5 && closure_ok == closure_seen
                && !closure_bad && !failed_invariant) ? 0 : 24;
done:
    printf("RESULT kind=%s exit=%d owned=%s\n",
           positive ? "POS" : "NEG", exitcode, owned ? "yes" : "no");
    nl_checked_destroy(owned);
    nl_syntax_tree_destroy(call);
    nl_parser_destroy(call_parser);
    nl_source_destroy(call_src);
    nl_semantic_destroy(ctx);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return exitcode;
}
