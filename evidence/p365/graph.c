/* Public-only inspection of the real legacy calibration, never grants rights.
 */
#include "newlang/captured_closure.h"
#include "newlang/parser.h"
#include "test.h"
#include <stdlib.h>
#include <string.h>
static NLCheckedNodeId allocation_match(const NLCheckedFragment *f)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i),
                                *a = nl_checked_node_view(f, n->initializer);
        if (n->kind == NL_CHECKED_MATCH && a &&
            a->kind == NL_CHECKED_TRY_ALLOCATE_ONE)
            return i;
    }
    return 0;
}
static bool heap_equal(const NLSemanticContext *a, const NLSemanticContext *b,
                       const NLPlaceId roots[5])
{
    for (size_t i = 0; i < 5; ++i) {
        NLSemanticPlaceView x, y;
        if (!nl_semantic_place_view(a, roots[i], &x) ||
            !nl_semantic_place_view(b, roots[i], &y) ||
            memcmp(&x, &y, sizeof(x)))
            return false;
        for (size_t k = 0; k < 4; ++k) {
            NLSemanticPlaceView p, q;
            NLSemanticValueView v, w;
            if (!nl_semantic_place_view(a, x.fixed_fields[k], &p) ||
                !nl_semantic_place_view(b, y.fixed_fields[k], &q) ||
                memcmp(&p, &q, sizeof(p)) ||
                !nl_semantic_value_view(a, p.current_value, &v) ||
                !nl_semantic_value_view(b, q.current_value, &w) ||
                memcmp(&v, &w, sizeof(v)))
                return false;
        }
    }
    return true;
}
static bool edge(const char *label, const NLSemanticContext *a,
                 const NLSemanticContext *b, const NLPlaceId roots[5])
{
    bool equal = heap_equal(a, b, roots);
    printf("{\"edge\":\"%s\",\"heap_root_and_all_20_fields_equal\":%s,\"AD_"
           "carriers\":[",
           label, equal ? "true" : "false");
    size_t ids[] = {1, 6, 15, 20, 29, 34, 43, 48, 57, 62};
    for (size_t j = 0; j < 10; ++j) {
        NLSemanticValueView x, y;
        CHECK(nl_semantic_value_view(a, ids[j], &x) &&
              nl_semantic_value_view(b, ids[j], &y));
        printf("%s{\"value\":%zu,\"before_carrier\":%d,\"after_carrier\":%d,"
               "\"before_place\":%zu,\"after_place\":%zu,\"before_aggregate\":%"
               "zu,\"after_aggregate\":%zu}",
               j ? "," : "", ids[j], x.carrier, y.carrier, x.owner_place,
               y.owner_place, x.aggregate_owner, y.aggregate_owner);
    }
    printf("]}\n");
    return true;
}
static bool inspect(const NLCheckedFragment *entry)
{
    const NLCheckedFragment *f =
        nl_checked_call_body(entry, nl_checked_root(entry));
    NLPlaceId roots[5] = {0};
    for (size_t k = 0; k < 5; ++k) {
        NLCheckedNodeId m = allocation_match(f);
        CHECK(m && nl_checked_captured_closure_validate(f, m) == NL_CHECK_OK);
        f = nl_checked_match_arm(f, m, 1);
        CHECK(f);
        for (size_t j = 1; j <= nl_checked_node_count(f); ++j) {
            const NLCheckedNodeView *n = nl_checked_node_view(f, j);
            if (n->kind == NL_CHECKED_INITIALIZE)
                roots[k] = n->lifetime_place;
        }
        CHECK(roots[k]);
    }
    NLScopedFieldCallView calls[2] = {0};
    NLCheckedNodeId ids[2] = {0}, join = 0;
    size_t count = 0;
    NLWholeValueCallView whole = {0};
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        NLScopedFieldCallView c;
        if (nl_checked_scoped_field_call_view(f, i, &c)) {
            CHECK(count < 2);
            ids[count] = i;
            calls[count++] = c;
            CHECK(nl_checked_scoped_field_call_validate(f, i) == NL_CHECK_OK);
        }
        if (nl_checked_whole_value_call_view(f, i, &whole))
            join = i;
    }
    CHECK(count == 2 && join);
    CHECK(nl_checked_whole_value_call_view(f, join, &whole));
    NLCapturedChangeView d6;
    CHECK(nl_checked_captured_change_view(f, 5, &d6));
    NLSemanticSnapshot start, finish;
    CHECK(nl_semantic_snapshot(d6.after, &start) &&
          nl_semantic_snapshot(calls[0].change.before, &finish));
    printf("{\"D6_after_counts\":[%zu,%zu,%zu,%zu,%zu,%zu],\"C1_before_"
           "counts\":[%zu,%zu,%zu,%zu,%zu,%zu]}\n",
           start.types, start.bindings, start.values, start.places,
           start.scopes, start.last_value_fact, finish.types, finish.bindings,
           finish.values, finish.places, finish.scopes, finish.last_value_fact);
    CHECK(edge("D6.after -> M -> C1.before", d6.after, calls[0].change.before,
               roots));
    NLSemanticPlaceView moved_domain;
    CHECK(nl_semantic_place_view(calls[0].change.before, 96, &moved_domain));
    printf("{\"actual_C1_before_place96_implicit_local\":%s}\n",
           moved_domain.implicit_local ? "true" : "false");
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if ((i >= 100 && i <= 154) ||
            (n->kind == NL_CHECKED_BLOCK && i < ids[0]))
            printf("{\"node\":%zu,\"kind\":%d,\"span\":[%zu,%zu],\"scope\":%zu,"
                   "\"type\":%zu,\"symbol\":%zu,\"use\":%d,\"init\":%zu,"
                   "\"first_arg\":%zu,\"next_arg\":%zu,\"argc\":%zu,\"field\":%"
                   "zu,\"first_item\":%zu,\"next_item\":%zu,\"tail\":%zu,"
                   "\"items\":%zu,\"results\":%zu,\"value\":%zu}\n",
                   i, n->kind, n->span.start_byte, n->span.end_byte,
                   n->loan.scope, n->type, n->symbol, n->value_use,
                   n->initializer, n->first_argument, n->next_argument,
                   n->argument_count, n->field_index, n->first_item,
                   n->next_item, n->tail, n->item_count, n->result_count,
                   n->results[0].value);
    }
    printf("{\"calibration_actual_owned\":true,\"caller_changes\":%zu,\"helper_"
           "calls\":[%zu,%zu],\"whole_call\":%zu,\"composition_ninth_owned\":"
           "false}\n",
           nl_checked_captured_change_count(f), ids[0], ids[1], join);
    return true;
}
static bool run(const char *path)
{
    NLSource *s = NULL, *es = NULL;
    NLParser *p = NULL, *ep = NULL;
    NLSyntaxTree *t = NULL, *et = NULL;
    NLSemanticContext *c = NULL;
    NLCheckedFragment *owned = NULL;
    CHECK(nl_source_load(path, &s) == NL_SOURCE_OK &&
          nl_parser_create(s, &p) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(p, &t, NULL) == NL_PARSE_OK &&
          nl_semantic_create(&c) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {t};
    CHECK(nl_semantic_register_function_unit(c, units, 1, NULL) ==
              NL_CHECK_OK &&
          nl_source_create("main()", 6, "<P362 calibration>", &es) ==
              NL_SOURCE_OK &&
          nl_parser_create(es, &ep) == NL_PARSE_OK &&
          nl_parser_parse_expression_fragment(ep, &et, NULL) == NL_PARSE_OK &&
          nl_semantic_check_expression(c, et, &owned, NULL) == NL_CHECK_OK &&
          owned);
    CHECK(inspect(owned));
    nl_checked_destroy(owned);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(et);
    nl_parser_destroy(ep);
    nl_source_destroy(es);
    nl_syntax_tree_destroy(t);
    nl_parser_destroy(p);
    nl_source_destroy(s);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && run(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
