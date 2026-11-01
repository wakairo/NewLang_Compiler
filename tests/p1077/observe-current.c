/* Read-only P1077 observation of actual CHECK-issued calls and ordered Changes.
 */
#include "../p1062/support.h"

static int returned_event(const NLCopyBodyView *v,
                          const NLCopyBodyEventView *events, size_t count,
                          size_t *hops)
{
    const NLCheckedFragment *f = v->body;
    const NLCheckedNodeView *root = nl_checked_node_view(f, v->root);
    if (!root || root->item_count > nl_checked_node_count(f))
        return -1;
    NLCheckedNodeId expression = root->tail, item = root->first_item;
    if (!expression) {
        const NLCheckedNodeView *last = NULL;
        for (size_t i = 0; i < root->item_count; ++i) {
            last = nl_checked_node_view(f, item);
            if (!last)
                return -1;
            item = last->next_item;
        }
        if (!last || last->kind != NL_CHECKED_RETURN)
            return -1;
        expression = last->initializer;
    }
    *hops = 0;
    for (size_t step = 0; step <= nl_checked_node_count(f); ++step) {
        for (size_t i = 0; i < count; ++i)
            if (expression == events[i].node)
                return (int)i;
        const NLCheckedNodeView *n = nl_checked_node_view(f, expression);
        if (!n || n->kind != NL_CHECKED_IDENTIFIER ||
            n->value_use != NL_VALUE_COPIED)
            return -1;
        NLCheckedNodeId init = 0;
        item = root->first_item;
        for (size_t i = 0; i < root->item_count; ++i) {
            const NLCheckedNodeView *b = nl_checked_node_view(f, item);
            if (!b)
                return -1;
            if (b->kind == NL_CHECKED_BINDING && b->symbol == n->symbol) {
                if (init)
                    return -1;
                init = b->initializer;
            }
            item = b->next_item;
        }
        if (!init)
            return -1;
        expression = init;
        ++*hops;
    }
    return -1;
}
static size_t scalar(const NLSemanticContext *c, NLValueId id)
{
    NLSemanticValueView v;
    if (!nl_semantic_value_view(c, id, &v) || !v.scalar_known ||
        v.scalar_value > 255)
        abort();
    return v.scalar_value;
}
static bool observe(const char *path)
{
    NLSemanticContext *base = NULL;
    TestNode node = {0};
    P1062Call calls[2];
    CHECK(registry(path, &base) && checked(base, &node) &&
          p1062_calls(&node, calls) && p1062_valid(calls) &&
          nl_h_origin_validate(node.entry));
    for (size_t i = 0; i < 2; ++i) {
        P1062Call *c = &calls[i];
        NLCopyBodyView v;
        NLCopyBodyParameters p;
        NLCopyBodyEventView event;
        CHECK(nl_checked_copy_body_view(c->caller, c->call, &v) &&
              nl_checked_copy_body_parameters(c->caller, c->call, &p));
        NLCheckStatus bs =
                          nl_checked_copy_body_validate(c->caller, c->call, &v),
                      ps = nl_checked_copy_body_parameters_validate(
                          c->caller, c->call, &p);
        size_t hops = 0;
        CHECK(bs == NL_CHECK_OK && ps == NL_CHECK_OK && v.event_count == 1 &&
              nl_checked_copy_body_event(c->caller, c->call, 0, &event) &&
              nl_checked_copy_body_event_validate(c->caller, c->call, 0,
                                                  &event) == NL_CHECK_OK &&
              event.kind == NL_COPY_BODY_CHANGE &&
              returned_event(&v, &event, 1, &hops) == 0);
        NLSemanticPlaceView before, after, root, sibling;
        CHECK(nl_semantic_place_view(p.entry, p.field.child, &before) &&
              nl_semantic_place_view(p.handoff, p.field.child, &after) &&
              nl_semantic_place_view(p.entry, p.field.parent, &root) &&
              p.field.index < 2 &&
              nl_semantic_place_view(
                  p.handoff, root.fixed_fields[1 - p.field.index], &sibling));
        printf(
            "{\"kind\":\"call\",\"call\":%zu,\"function\":%zu,\"root\":%zu,"
            "\"child\":%zu,\"index\":%zu,\"old\":%zu,\"next\":%zu,\"sibling\":%"
            "zu,\"body_current\":%d,\"parameters_current\":%d}\n",
            c->call, v.function, p.field.parent, p.field.child, p.field.index,
            scalar(p.entry, before.current_value),
            scalar(p.handoff, after.current_value),
            scalar(p.handoff, sibling.current_value), (int)bs, (int)ps);
    }
    const NLCheckedFragment *f = calls[0].caller;
    const NLSemanticContext *context = nl_checked_context(f);
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (n->kind == NL_CHECKED_REPLACE &&
            n->type == nl_semantic_core_type(context, NL_TYPE_U8)) {
            CHECK(n->field.present && n->result_count == 1);
            printf("{\"kind\":\"direct\",\"node\":%zu,\"root\":%zu,\"child\":%"
                   "zu,\"index\":%zu,\"old\":%zu,\"next\":%zu,\"old_fact\":%zu,"
                   "\"new_fact\":%zu}\n",
                   id, n->field.parent, n->field.child, n->field.index,
                   scalar(context, n->field.old_value),
                   scalar(context, n->field.new_value), n->field.child_fact,
                   n->field.child_post_fact);
        }
        if (n->kind == NL_CHECKED_BINDING && n->symbol &&
            n->symbol <= context->binding_count) {
            const char *name = context->bindings[n->symbol - 1].name;
            if (!strcmp(name, "old_a") || !strcmp(name, "old_b") ||
                !strcmp(name, "batch_result") || !strcmp(name, "final_old")) {
                const NLCheckedNodeView *init =
                    nl_checked_node_view(f, n->initializer);
                CHECK(init && init->result_count == 1);
                printf("{\"kind\":\"binding\",\"name\":\"%s\",\"symbol\":%zu,"
                       "\"value\":%zu}\n",
                       name, n->symbol,
                       scalar(context, init->results[0].value));
            }
        }
    }
    puts("{\"kind\":\"ownership\",\"origin\":true,\"custody\":true}");
    node_checked_destroy(&node);
    nl_semantic_destroy(base);
    return true;
}
int main(int argc, char **argv)
{
    (void)node_backend;
    return argc == 2 && observe(argv[1]) ? 0 : 1;
}
