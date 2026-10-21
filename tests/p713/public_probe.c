#include "../support/node_checked.h"
#include "newlang/if_field.h"
static bool probe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    size_t count = 0;
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        NLIfFieldView v;
        if (!nl_checked_if_field_view(f, id, &v))
            continue;
        CHECK(!count && nl_checked_if_field_validate(f, id, &v) == NL_CHECK_OK);
        NLSemanticPlaceView before, after, child, sibling, old_sibling;
        NLSemanticValueView unknown, whole, result;
        CHECK(nl_semantic_place_view(v.entry, v.root, &before) &&
              nl_semantic_place_view(v.post, v.root, &after) &&
              nl_semantic_place_view(v.post, v.child, &child) &&
              nl_semantic_place_view(v.post, v.sibling, &sibling) &&
              nl_semantic_place_view(v.entry, v.sibling, &old_sibling) &&
              before.incarnation == after.incarnation &&
              after.incarnation == v.incarnation &&
              child.incarnation == v.child_incarnation &&
              before.current_fact != after.current_fact &&
              old_sibling.current_fact == sibling.current_fact &&
              old_sibling.current_value == sibling.current_value &&
              nl_semantic_value_view(v.post, child.current_value, &unknown) &&
              !unknown.scalar_known &&
              unknown.type == nl_semantic_core_type(v.post, NL_TYPE_U8) &&
              unknown.dependencies == NL_DEPENDENCY_FREE &&
              !unknown.reference.place && !unknown.domain &&
              !unknown.allocation_region &&
              nl_semantic_value_view(v.post, after.current_value, &whole) &&
              whole.field_count == 2 &&
              whole.fields[v.index] == child.current_value &&
              whole.fields[1 - v.index] == sibling.current_value &&
              (!v.result.value ||
               (nl_semantic_value_view(v.post, v.result.value, &result) &&
                !result.scalar_known)));
        const NLCheckedNodeView *choice = nl_checked_node_view(f, id);
        const NLCheckedNodeView *condition =
            nl_checked_node_view(f, v.condition);
        CHECK(choice && condition && condition->result_count == 1);
        printf("{\"if\":%zu,\"condition\":%zu,\"condition_value\":%zu,\"base\":"
               "%zu,"
               "\"root\":%zu,\"child\":%zu,\"index\":%zu,\"incarnation\":%zu,"
               "\"parent_before\":%zu,\"parent_after\":%zu,\"child_after\":%zu,"
               "\"result_id\":%zu,\"arms\":[",
               id, v.condition, condition->results[0].value, v.base, v.root,
               v.child, v.index, v.incarnation, before.current_fact,
               after.current_fact, child.current_fact, v.result.value);
        for (size_t a = 0; a < 2; ++a) {
            CHECK(v.arms[a].artifact == nl_checked_if_arm(f, id, a) &&
                  v.arms[a].world != v.post && v.arms[a].world != v.entry &&
                  v.arms[0].world != v.arms[1].world);
            NLSemanticPlaceView root, field, sib;
            CHECK(nl_semantic_place_view(v.arms[a].world, v.root, &root) &&
                  nl_semantic_place_view(v.arms[a].world, v.child, &field) &&
                  nl_semantic_place_view(v.arms[a].world, v.sibling, &sib) &&
                  root.incarnation == v.incarnation &&
                  field.incarnation == v.child_incarnation &&
                  sib.current_fact == old_sibling.current_fact &&
                  sib.current_value == old_sibling.current_value);
            if (!v.arms[a].change_count)
                CHECK(root.current_fact == before.current_fact);
            NLSemanticSnapshot counters;
            CHECK(nl_semantic_snapshot(v.arms[a].world, &counters));
            for (size_t s = 1; s <= counters.scopes; ++s) {
                NLSemanticScopeView scope;
                CHECK(nl_semantic_scope_view(v.arms[a].world, s, &scope) &&
                      !scope.active);
            }
            printf("%s{\"changes\":%zu,\"parent_fact\":%zu,\"child_fact\":%zu,"
                   "\"events\":[",
                   a ? "," : "", v.arms[a].change_count, root.current_fact,
                   field.current_fact);
            for (size_t i = 0; i < v.arms[a].event_count; ++i) {
                NLIfFieldEvent e;
                CHECK(nl_checked_if_field_event(f, id, a, i, &e) &&
                      nl_checked_if_field_event_validate(f, id, a, i, &e) ==
                          NL_CHECK_OK);
                const NLCheckedNodeView *op =
                    nl_checked_node_view(v.arms[a].artifact, e.node);
                size_t ref = 0, incoming = 0;
                if (e.change) {
                    const NLCheckedNodeView *arg = nl_checked_node_view(
                        v.arms[a].artifact, op->first_argument);
                    CHECK(arg);
                    ref = arg->symbol;
                    const NLCheckedNodeView *value = nl_checked_node_view(
                        v.arms[a].artifact, arg->next_argument);
                    CHECK(value);
                    incoming = value->symbol;
                }
                printf(
                    "%s{\"node\":%zu,\"change\":%s,\"old_id\":%zu,\"new_id\":%"
                    "zu,\"result_id\":%zu,\"ref\":%zu,\"incoming_symbol\":%zu}",
                    i ? "," : "", e.node, e.change ? "true" : "false",
                    e.field.old_value, e.incoming, e.result, ref, incoming);
            }
            fputs("]}", stdout);
        }
        size_t binding = 0;
        for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
            const NLCheckedNodeView *b = nl_checked_node_view(f, i);
            if (b->kind == NL_CHECKED_BINDING && b->initializer == id)
                binding = b->symbol;
        }
        printf("],\"result_binding\":%zu,\"reads\":[", binding);
        size_t reads = 0;
        for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
            const NLCheckedNodeView *b = nl_checked_node_view(f, i);
            const NLCheckedNodeView *read =
                b->kind == NL_CHECKED_BINDING
                    ? nl_checked_node_view(f, b->initializer)
                    : NULL;
            if (read && read->kind == NL_CHECKED_FIELD_READ &&
                read->field.parent == v.root &&
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
    return argc == 2 && probe(argv[1]) ? 0 : 1;
}
