#include "../support/node_checked.h"
static bool walk(const NLCheckedFragment *f)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->producer.entry_proved) {
            const NLSemanticContext *b = nl_checked_producer_return(f, i);
            const NLCheckedFragment *body = nl_checked_call_body(f, i);
            printf("producer=%zu valid=%d root=%zu incarnation=%zu region=%zu "
                   "domain=%zu result=%zu\n",
                   i, nl_checked_producer_valid(f, i), n->producer.root,
                   n->producer.incarnation, n->producer.range.region,
                   n->producer.domain, n->producer.result);
            for (size_t j = 1; j <= nl_checked_node_count(body); ++j) {
                const NLCheckedNodeView *v = nl_checked_node_view(body, j);
                printf("node=%zu kind=%d symbol=%zu init=%zu first=%zu "
                       "next=%zu argc=%zu result=%zu use=%d span=%zu:%zu\n",
                       j, v->kind, v->symbol, v->initializer, v->first_argument,
                       v->next_item, v->argument_count,
                       v->result_count ? v->results[0].value : 0, v->value_use,
                       v->span.start_byte, v->span.end_byte);
                NLSemanticBindingView bound;
                if (v->symbol && nl_semantic_binding_view(b, v->symbol, &bound))
                    printf("binding=%zu value=%zu place=%zu available=%d\n",
                           v->symbol, bound.value, bound.place,
                           bound.availability);
            }
        }
        const NLCheckedFragment *child = nl_checked_call_body(f, i);
        if (child && !walk(child))
            return false;
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < n->item_count; ++a)
                if (!walk(nl_checked_match_arm(f, i, a)))
                    return false;
    }
    return true;
}
int main(int argc, char **argv)
{
    TestNode n = {0};
    if (argc != 2 || !node_checked_load(argv[1], &n))
        return 1;
    bool ok = walk(n.entry);
    node_checked_destroy(&n);
    return ok ? 0 : 1;
}
