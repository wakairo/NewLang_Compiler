/* P1079 read-only observation of actual source CHECK instances. */
#include "../p965/shared.h"
#include "newlang/copy_body.h"
#include "newlang/if_field.h"

static bool walk(const NLCheckedFragment *f, size_t depth)
{
    CHECK(f && depth < 20);
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        NLCopyBodyView copy = {0};
        NLCopyBodyParameters params = {0};
        bool cg = nl_checked_copy_body_view(f, id, &copy);
        bool pg = nl_checked_copy_body_parameters(f, id, &params);
        if (cg || pg) {
            NLCheckStatus cv =
                nl_checked_copy_body_validate(f, id, cg ? &copy : NULL);
            NLCheckStatus pv = nl_checked_copy_body_parameters_validate(
                f, id, pg ? &params : NULL);
            printf("{\"kind\":\"Copy\",\"call\":%zu,\"GET_body\":%d,\"GET_"
                   "parameters\":%d,\"body_current\":%d,\"parameters_current\":"
                   "%d,\"events\":%zu}\n",
                   id, (int)cg, (int)pg, (int)cv, (int)pv, copy.event_count);
        }
        if (n->kind == NL_CHECKED_IF) {
            NLIfFieldView v = {0};
            bool got = nl_checked_if_field_view(f, id, &v);
            NLCheckStatus status =
                nl_checked_if_field_validate(f, id, got ? &v : NULL);
            NLSemanticPlaceView before = {0}, after = {0};
            bool b = got && nl_semantic_place_view(v.entry, v.child, &before);
            bool a = got && nl_semantic_place_view(v.post, v.child, &after);
            NLSemanticValueView result = {0};
            bool r = got && v.result.value &&
                     nl_semantic_value_view(v.post, v.result.value, &result);
            printf(
                "{\"kind\":\"IF\",\"node\":%zu,\"normal_arms\":%zu,\"GET\":%d,"
                "\"current\":%d,\"root\":%zu,\"child\":%zu,\"index\":%zu,"
                "\"result_type\":%zu,\"result_value\":%zu,\"result_seen\":%d,"
                "\"result_known\":%d,\"before_fact\":%zu,\"post_fact\":%zu}\n",
                id, n->normal_arms, (int)got, (int)status, v.root, v.child,
                v.index, v.result.type, v.result.value, (int)r,
                (int)result.scalar_known, b ? before.current_fact : 0,
                a ? after.current_fact : 0);
            if (got)
                for (size_t arm = 0; arm < 2; ++arm) {
                    printf("{\"kind\":\"arm\",\"node\":%zu,\"arm\":%zu,"
                           "\"events\":%zu,\"changes\":%zu}\n",
                           id, arm, v.arms[arm].event_count,
                           v.arms[arm].change_count);
                    for (size_t i = 0; i < v.arms[arm].event_count; ++i) {
                        NLIfFieldEvent e = {0};
                        bool eg = nl_checked_if_field_event(f, id, arm, i, &e);
                        NLCheckStatus ev = nl_checked_if_field_event_validate(
                            f, id, arm, i, eg ? &e : NULL);
                        printf("{\"kind\":\"event\",\"node\":%zu,\"arm\":%zu,"
                               "\"ordinal\":%zu,\"GET\":%d,\"current\":%d,"
                               "\"change\":%d,\"event_node\":%zu,\"field_"
                               "child\":%zu}\n",
                               id, arm, i, (int)eg, (int)ev, (int)e.change,
                               e.node, e.field.child);
                    }
                }
        }
        if (n->kind == NL_CHECKED_LOOP)
            printf("{\"kind\":\"loop\",\"node\":%zu,\"inductive\":%d,"
                   "\"continues\":%zu,\"breaks\":%zu,\"returns\":%zu,\"body_"
                   "GET\":%d}\n",
                   id, (int)n->header_inductive, n->continue_edges,
                   n->break_edges, n->return_edges,
                   (int)(nl_checked_loop_body(f, id) != NULL));
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (body)
            CHECK(walk(body, depth + 1));
        if (n->kind == NL_CHECKED_MATCH || n->kind == NL_CHECKED_IF)
            for (size_t arm = 0; arm < n->item_count; ++arm)
                CHECK(walk(n->kind == NL_CHECKED_IF
                               ? nl_checked_if_arm(f, id, arm)
                               : nl_checked_match_arm(f, id, arm),
                           depth + 1));
        if (n->kind == NL_CHECKED_LOOP && nl_checked_loop_body(f, id))
            CHECK(walk(nl_checked_loop_body(f, id), depth + 1));
    }
    return true;
}
static bool observe(const char *path)
{
    NLSemanticContext *base = NULL;
    TestNode node = {0};
    CHECK(registry(path, &base) && checked(base, &node));
    printf("{\"kind\":\"origin\",\"valid\":%d}\n",
           (int)nl_h_origin_validate(node.entry));
    CHECK(walk(node.entry, 0));
    node_checked_destroy(&node);
    nl_semantic_destroy(base);
    return true;
}
int main(int argc, char **argv)
{
    (void)node_backend;
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
