/* P-owned observation, never an authority issuer or a production validator. */
#include "../p965/shared.h"
#include "newlang/copy_body.h"

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
static bool number(const NLSemanticContext *c, NLValueId id, size_t *out)
{
    NLSemanticValueView v;
    CHECK(nl_semantic_value_view(c, id, &v) && v.scalar_known &&
          v.scalar_value <= 255);
    *out = v.scalar_value;
    return true;
}
static bool h_observe(const NLCheckedFragment *f, size_t depth)
{
    CHECK(f && depth < 12);
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (n->producer.entry_proved)
            printf("{\"P1062\":\"H_producer\",\"call\":%zu,\"entry_GET\":%s,"
                   "\"valid\":%s,\"return_proved\":%s}\n",
                   id, nl_checked_producer_entry(f, id) ? "true" : "false",
                   nl_checked_producer_valid(f, id) ? "true" : "false",
                   n->producer.return_proved ? "true" : "false");
        if (n->owner_call.entry_proved)
            printf("{\"P1062\":\"H_terminal\",\"call\":%zu,\"entry_GET\":%s,"
                   "\"post_proved\":%s}\n",
                   id, nl_checked_owner_entry(f, id) ? "true" : "false",
                   n->owner_call.post_proved ? "true" : "false");
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (body)
            CHECK(h_observe(body, depth + 1));
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t i = 0; i < n->item_count; ++i)
                CHECK(h_observe(nl_checked_match_arm(f, id, i), depth + 1));
    }
    return true;
}
static bool run(const char *path, size_t expected)
{
    NLSemanticContext *base = NULL;
    TestNode node = {0};
    CHECK(registry(path, &base) && checked(base, &node));
    const NLCheckedFragment *caller =
        nl_checked_call_body(node.entry, nl_checked_root(node.entry));
    CHECK(caller);
    CHECK(nl_h_origin_validate(node.entry));
    printf("{\"P1062\":\"H_origin\",\"current\":true}\n");
    size_t count = 0;
    NLSymbolId first_binding = 0;
    NLTypeId u8 = nl_semantic_core_type(nl_checked_context(caller), NL_TYPE_U8);
    for (size_t id = 1; id <= nl_checked_node_count(caller); ++id) {
        const NLCheckedNodeView *call = nl_checked_node_view(caller, id);
        if (call->kind != NL_CHECKED_REGISTERED_CALL || call->type != u8)
            continue;
        NLCopyBodyView v;
        NLCopyBodyParameters p;
        bool body = nl_checked_copy_body_view(caller, id, &v),
             parameters = nl_checked_copy_body_parameters(caller, id, &p);
        NLCheckStatus bs = nl_checked_copy_body_validate(caller, id,
                                                         body ? &v : NULL),
                      ps = nl_checked_copy_body_parameters_validate(
                          caller, id, parameters ? &p : NULL);
        printf("{\"P1062\":\"current\",\"ordinal\":%zu,\"call\":%zu,"
               "\"function\":%zu,\"GET_"
               "body\":%s,\"GET_parameters\":%s,\"body\":%d,\"parameters\":%d}",
               count, id, call->function, body ? "true" : "false",
               parameters ? "true" : "false", (int)bs, (int)ps);
        puts("");
        if (body && parameters && bs == NL_CHECK_OK && ps == NL_CHECK_OK) {
            NLCopyBodyEventView events[8];
            CHECK(v.event_count && v.event_count <= 8);
            for (size_t i = 0; i < v.event_count; ++i) {
                CHECK(nl_checked_copy_body_event(caller, id, i, &events[i]));
                NLCheckStatus es = nl_checked_copy_body_event_validate(
                    caller, id, i, &events[i]);
                printf("{\"P1062\":\"event\",\"ordinal\":%zu,\"event\":%zu,"
                       "\"kind\":%d,\"current\":%d}\n",
                       count, i, (int)events[i].kind, (int)es);
                CHECK(es == NL_CHECK_OK);
            }
            size_t hops = 0;
            int origin = returned_event(&v, events, v.event_count, &hops);
            NLSemanticPlaceView before, after, root, sibling;
            size_t old, next, result, other;
            CHECK(nl_semantic_place_view(p.entry, p.field.child, &before) &&
                  nl_semantic_place_view(p.handoff, p.field.child, &after) &&
                  nl_semantic_place_view(p.entry, p.field.parent, &root) &&
                  root.fixed_field_count == 2 && p.field.index < 2 &&
                  nl_semantic_place_view(p.handoff,
                                         root.fixed_fields[1 - p.field.index],
                                         &sibling) &&
                  number(p.entry, before.current_value, &old) &&
                  number(p.handoff, after.current_value, &next) &&
                  number(p.handoff, v.result.value, &result) &&
                  number(p.handoff, sibling.current_value, &other));
            NLSymbolId binding = 0;
            for (size_t j = 1; j <= nl_checked_node_count(caller); ++j) {
                const NLCheckedNodeView *b = nl_checked_node_view(caller, j);
                if (b->kind == NL_CHECKED_BINDING && b->initializer == p.loan) {
                    CHECK(!binding);
                    binding = b->symbol;
                }
            }
            CHECK(binding);
            bool forward = true;
            if (!count)
                first_binding = binding;
            else {
                const NLCheckedNodeView *arg =
                    nl_checked_node_view(caller, p.arguments[1]);
                forward = arg && arg->kind == NL_CHECKED_IDENTIFIER &&
                          arg->symbol == first_binding &&
                          arg->value_use == NL_VALUE_COPIED;
            }
            printf("{\"P1062\":\"origin\",\"ordinal\":%zu,\"event_count\":%zu,"
                   "\"returned_event\":%d,\"binding_hops\":%zu,"
                   "\"index\":%zu,\"old\":%zu,\"next\":%zu,\"returned\":%zu,"
                   "\"sibling\":%zu,\"root\":%zu,\"incarnation\":%zu,"
                   "\"result_binding\":%zu,\"old_forward_source\":%s}\n",
                   count, v.event_count, origin, hops, p.field.index, old, next,
                   result, other, p.field.parent, p.field.parent_incarnation,
                   binding, forward ? "true" : "false");
        }
        ++count;
    }
    for (size_t id = 1; id <= nl_checked_node_count(caller); ++id) {
        const NLCheckedNodeView *replace = nl_checked_node_view(caller, id);
        if (replace->kind != NL_CHECKED_REPLACE || replace->type != u8)
            continue;
        NLCheckedNodeId loan = 0;
        for (size_t j = 1; j <= nl_checked_node_count(caller); ++j) {
            const NLCheckedNodeView *n = nl_checked_node_view(caller, j);
            const NLCheckedNodeView *block =
                n->kind == NL_CHECKED_LOAN_HEADER
                    ? nl_checked_node_view(caller, n->initializer)
                    : NULL;
            if (block && block->tail == id) {
                CHECK(!loan);
                loan = j;
            }
        }
        NLSymbolId binding = 0;
        for (size_t j = 1; j <= nl_checked_node_count(caller); ++j) {
            const NLCheckedNodeView *n = nl_checked_node_view(caller, j);
            if (n->kind == NL_CHECKED_BINDING && n->initializer == loan) {
                CHECK(!binding);
                binding = n->symbol;
            }
        }
        size_t old, next;
        CHECK(loan && binding &&
              number(nl_checked_context(caller), replace->field.old_value,
                     &old) &&
              number(nl_checked_context(caller), replace->field.new_value,
                     &next));
        printf("{\"P1062\":\"direct\",\"binding\":%zu,\"root\":%zu,"
               "\"index\":%zu,\"old\":%zu,\"next\":%zu}\n",
               binding, replace->field.parent, replace->field.index, old, next);
    }
    CHECK(h_observe(node.entry, 0));
    CHECK(count == expected);
    node_checked_destroy(&node);
    nl_semantic_destroy(base);
    return true;
}
int main(int argc, char **argv)
{
    (void)node_backend;
    return argc == 3 && run(argv[1], (size_t)strtoul(argv[2], NULL, 10)) ? 0
                                                                         : 1;
}
