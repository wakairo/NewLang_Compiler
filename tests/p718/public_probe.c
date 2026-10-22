#include "../support/node_checked.h"
#include "newlang/if_field.h"
static bool observe(const char *file)
{
    TestNode n = {0};
    CHECK(node_checked_load(file, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    size_t count = 0;
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        NLIfReturnView v;
        if (!nl_checked_if_return_view(f, id, &v))
            continue;
        NLCheckStatus status = nl_checked_if_return_validate(f, id, &v);
        if (status != NL_CHECK_OK)
            fprintf(stderr, "return validator status=%d IF=%zu normal=%zu\n",
                    status, id, v.normal_arm);
        CHECK(status == NL_CHECK_OK && !count);
        NLIfFieldView invalid;
        CHECK(!nl_checked_if_field_view(f, id, &invalid) &&
              nl_checked_if_field_validate(f, id, NULL) ==
                  NL_CHECK_ANALYSIS_PRECISION_LIMIT);
        const NLIfFieldView *p = &v.field;
        NLSemanticPlaceView entry, post, child, sibling, prior_sibling, live,
            ended;
        NLSemanticValueView current, old;
        CHECK(nl_semantic_place_view(p->entry, p->root, &entry) &&
              nl_semantic_place_view(p->post, p->root, &post) &&
              nl_semantic_place_view(p->post, p->child, &child) &&
              nl_semantic_place_view(p->post, p->sibling, &sibling) &&
              nl_semantic_place_view(p->entry, p->sibling, &prior_sibling) &&
              nl_semantic_place_view(v.return_live, p->root, &live) &&
              nl_semantic_place_view(v.return_ended, p->root, &ended));
        CHECK(entry.incarnation == post.incarnation &&
              post.incarnation == p->incarnation &&
              child.incarnation == p->child_incarnation && live.live &&
              !ended.live && !ended.current_fact && !ended.current_value &&
              !ended.fixed_field_count &&
              entry.current_fact != post.current_fact &&
              sibling.current_fact == prior_sibling.current_fact &&
              sibling.current_value == prior_sibling.current_value &&
              nl_semantic_value_view(p->post, child.current_value, &current) &&
              nl_semantic_value_view(p->post, p->result.value, &old) &&
              current.scalar_known && old.scalar_known &&
              v.returned.type == nl_semantic_unit_type(p->entry) &&
              !v.returned.value && p->post != p->arms[v.normal_arm].world &&
              p->post != v.return_live && p->post != v.return_ended);
        size_t binding = 0;
        for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
            const NLCheckedNodeView *b = nl_checked_node_view(f, i);
            if (b->kind == NL_CHECKED_BINDING && b->initializer == id)
                binding = b->symbol;
        }
        printf("{\"if\":%zu,\"normal\":%zu,\"return_node\":%zu,\"base\":%zu,"
               "\"root\":%zu,\"child\":%zu,\"index\":%zu,\"binding\":%zu,"
               "\"current\":%zu,\"old\":%zu,\"parent_before\":%zu,\"parent_"
               "after\":%zu,\"streams\":[",
               id, v.normal_arm, v.return_node, p->base, p->root, p->child,
               p->index, binding, current.scalar_value, old.scalar_value,
               entry.current_fact, post.current_fact);
        for (size_t a = 0; a < 3; ++a) {
            size_t changes =
                a == 2 ? v.replay_changes : p->arms[a].change_count;
            size_t events = a == 2 ? v.replay_events : p->arms[a].event_count;
            const NLCheckedFragment *graph = a == 2 ? f : p->arms[a].artifact;
            printf("%s{\"changes\":%zu,\"events\":[", a ? "," : "", changes);
            for (size_t i = 0; i < events; ++i) {
                NLIfFieldEvent e;
                CHECK(nl_checked_if_return_event(f, id, a, i, &e) &&
                      nl_checked_if_return_event_validate(f, id, a, i, &e) ==
                          NL_CHECK_OK);
                const NLCheckedNodeView *op =
                    nl_checked_node_view(graph, e.node);
                const NLCheckedNodeView *ref =
                    e.change ? nl_checked_node_view(graph, op->first_argument)
                             : NULL;
                printf("%s{\"node\":%zu,\"change\":%s,\"old\":%zu,\"incoming\":"
                       "%zu,\"ref\":%zu}",
                       i ? "," : "", e.node, e.change ? "true" : "false",
                       e.field.old_value, e.incoming, ref ? ref->symbol : 0);
            }
            fputs("]}", stdout);
        }
        fputs("],\"reads\":[", stdout);
        size_t reads = 0;
        const NLCheckedNodeView *choice = nl_checked_node_view(f, id);
        for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
            const NLCheckedNodeView *b = nl_checked_node_view(f, i);
            const NLCheckedNodeView *read =
                b->kind == NL_CHECKED_BINDING
                    ? nl_checked_node_view(f, b->initializer)
                    : NULL;
            if (read && read->kind == NL_CHECKED_FIELD_READ &&
                read->field.parent == p->root &&
                read->span.start_byte > choice->span.end_byte) {
                printf("%s{\"symbol\":%zu,\"index\":%zu}", reads ? "," : "",
                       b->symbol, read->field.index);
                ++reads;
            }
        }
        puts("]}");
        ++count;
    }
    CHECK(count);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
