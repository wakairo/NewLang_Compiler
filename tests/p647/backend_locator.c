/* Diagnostic call of unchanged Production admission predicates, not a public
 * validator or a new lowering path. No world mutation or permission seed. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"
static bool locate(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    const NLCheckedNodeView *root = nl_checked_node_view(f, nl_checked_root(f));
    V0Program p = {.unit = nl_semantic_unit_type(n.context),
                   .u8 = nl_semantic_core_type(n.context, NL_TYPE_U8)};
    CHECK(f && root && root->kind == NL_CHECKED_BLOCK);
    bool stopped = false;
    for (NLCheckedNodeId item = root->first_item; item != 0;) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, item);
        CHECK(v);
        if (!v0_validate_node(f, item, &p)) {
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, v->initializer);
            NLSemanticSnapshot s;
            CHECK(v->kind == NL_CHECKED_BINDING && init &&
                  init->kind == NL_CHECKED_IF &&
                  nl_semantic_snapshot(nl_checked_context(f), &s) &&
                  s.scopes == 2);
            size_t bindings = 0;
            for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
                const NLCheckedNodeView *b = nl_checked_node_view(f, i);
                if (b->kind == NL_CHECKED_BINDING &&
                    b->span.end_byte <= init->span.start_byte)
                    ++bindings;
            }
            /* Establish the unchanged selection predicates before the frame
             * call. This is diagnostic readback, never a new admission rule. */
            const NLSemanticContext *c = nl_checked_context(f);
            const NLCheckedNodeView *cond =
                nl_checked_node_view(f, init->initializer);
            const NLCheckedNodeId origin =
                bool_source_origin(f, init->initializer);
            const NLCheckedNodeView *from = nl_checked_node_view(f, origin);
            NLSemanticValueView cv, ov, result;
            CHECK(!init->terminates && init->normal_arms == 2 &&
                  init->item_count == 2 && !init->has_scalar_result &&
                  init->type == p.u8 && init->result_count == 1 &&
                  init->results[0].type == p.u8 &&
                  bool_root_value(f, v->initializer) && c->domain_count == 0 &&
                  c->region_count == 0 && c->occurrence_count == 0 &&
                  p.scalar_depth == 0 && cond && from &&
                  init->initializer < v->initializer &&
                  cond->kind == NL_CHECKED_IDENTIFIER &&
                  cond->result_count == 1 &&
                  cond->type == nl_semantic_core_type(c, NL_TYPE_BOOL) &&
                  boolean_scalar_at(f, origin, &p, 0) &&
                  nl_semantic_value_view(c, cond->results[0].value, &cv) &&
                  nl_semantic_value_view(c, from->results[0].value, &ov) &&
                  copy_if_plain(cv, from->type) && cv.scalar_known &&
                  cv.scalar_value == ov.scalar_value &&
                  cond->results[0].value > from->results[0].value &&
                  init->results[0].value == cond->results[0].value + 1 &&
                  nl_semantic_value_view(c, init->results[0].value, &result) &&
                  copy_if_plain(result, p.u8) && !result.scalar_known &&
                  result.carrier == NL_CARRIER_ENDED &&
                  result.owner_place == 0 && result.scalar_value == 0 &&
                  p.aggregate != 0 && c->value_count < NL_SEMANTIC_MAX_ENTRIES);
            CHECK(!copy_if_pair_frame(f, v->initializer, &p, bindings));
            printf("{\"first_rejected_binding\":%zu,\"initializer_if\":%zu,"
                   "\"span\":[%zu,%zu],\"aggregate_type\":%zu,\"actual_"
                   "scopes\":%zu,"
                   "\"required_scopes\":1,\"private_frame_predicate\":false,"
                   "\"selection_prefix_passed\":true}\n",
                   v->symbol, v->initializer, init->span.start_byte,
                   init->span.end_byte, p.aggregate, s.scopes);
            stopped = true;
            break;
        }
        item = v->next_item;
    }
    CHECK(stopped);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && locate(argv[1]) ? 0 : 1;
}
