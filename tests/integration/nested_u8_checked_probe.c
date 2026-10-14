/* Actual-source nested frames, including the unchanged-emitter boundary. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"

static bool frames(const NLCheckedFragment *f, V0Program p, size_t *calls)
{
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        const NLCheckedFragment *body = nl_checked_call_body(f, i);
        if (body == NULL)
            continue;
        NLSymbolId formal = 0;
        CHECK(v1_call_parameter(f, i, &p, &formal));
        if (formal != 0) {
            const NLCheckedNodeView *arg =
                nl_checked_node_view(f, v->first_argument);
            const NLCheckedNodeView *root =
                nl_checked_node_view(body, nl_checked_root(body));
            NLSemanticBindingView binding;
            NLSemanticValueView result;
            CHECK(nl_semantic_binding_view(nl_checked_context(body), formal,
                                           &binding));
            CHECK(binding.value == arg->results[0].value &&
                  binding.type == p.u8);
            CHECK(root->result_count == 1 && v->result_count == 1);
            CHECK(root->results[0].value == v->results[0].value);
            CHECK(nl_semantic_value_view(nl_checked_context(f),
                                         v->results[0].value, &result));
            CHECK(result.type == p.u8 &&
                  result.dependencies == NL_DEPENDENCY_FREE);
            printf("FRAME function=%zu node=%zu formal=%zu actual=%zu "
                   "bound=%zu returned=%zu known=%d scalar=%zu\n",
                   v->function, i, formal, arg->results[0].value, binding.value,
                   v->results[0].value, result.scalar_known,
                   result.scalar_value);
            ++*calls;
        }
        V0Program inner = p;
        inner.formal_body = body;
        inner.formal = formal;
        CHECK(frames(body, inner, calls));
    }
    return true;
}

static bool validates(const NLCheckedFragment *f)
{
    V0Program p = {
        .unit = nl_semantic_unit_type(nl_checked_context(f)),
        .u8 = nl_semantic_core_type(nl_checked_context(f), NL_TYPE_U8)};
    NLSemanticSnapshot before = {0}, after = {0};
    if (!nl_semantic_snapshot(nl_checked_context(f), &before))
        abort();
    const bool valid = v0_validate_call(f, nl_checked_root(f), &p);
    if (!nl_semantic_snapshot(nl_checked_context(f), &after) ||
        memcmp(&before, &after, sizeof(before)) != 0)
        abort();
    /* The CLI emits only after complete validation. Exercise that same gate. */
    FILE *output = tmpfile();
    if (output == NULL)
        return false;
    bool emitted = valid && v0_emit_c(output, &p);
    bool empty = ftell(output) == 0;
    fclose(output);
    if (!valid && !empty)
        abort();
    return valid && emitted; /* rejected artifacts yield false/C0 */
}

static bool controls(TestNode *n)
{
    const NLCheckedFragment *main_body =
        nl_checked_call_body(n->entry, nl_checked_root(n->entry));
    CHECK(main_body != NULL);
    /* Both outer actual frames, and the inner frame of each, independently. */
    size_t rejected = 0;
    for (NLCheckedNodeId outer = 1; outer <= nl_checked_node_count(main_body);
         ++outer) {
        const NLCheckedFragment *relay = nl_checked_call_body(main_body, outer);
        if (relay == NULL)
            continue;
        for (size_t level = 0; level < 2; ++level) {
            const NLCheckedFragment *parent = level == 0 ? main_body : relay;
            NLCheckedNodeId id =
                level == 0
                    ? outer
                    : nl_checked_node_view(relay, nl_checked_root(relay))->tail;
            NLCheckedNodeView *call =
                &((NLCheckedFragment *)parent)->nodes[id - 1];
            const NLCheckedNodeView saved = *call;
            NLCheckedFragment *body =
                (NLCheckedFragment *)nl_checked_call_body(parent, id);
            CHECK(body != NULL);
            call->body_backed = false;
            CHECK(!validates(n->entry));
            *call = saved;
            call->function =
                nl_checked_node_view(n->entry, nl_checked_root(n->entry))
                    ->function;
            CHECK(!validates(n->entry));
            *call = saved;
            /* A genuine but wrong package, not only the zero sentinel. */
            call->results[0].value =
                nl_checked_node_view(parent, call->first_argument)
                    ->results[0]
                    .value;
            CHECK(!validates(n->entry));
            *call = saved;
            NLFunctionBody *owner = body->body_owner;
            body->body_owner = NULL;
            CHECK(!validates(n->entry));
            body->body_owner = owner;
            V0Program p = {.unit = nl_semantic_unit_type(n->context),
                           .u8 = nl_semantic_core_type(n->context, NL_TYPE_U8)};
            if (level != 0) {
                NLSymbolId outer_formal = 0;
                CHECK(v1_call_parameter(main_body, outer, &p, &outer_formal));
                p.formal_body = relay;
                p.formal = outer_formal;
            }
            NLSymbolId formal = 0;
            CHECK(v1_call_parameter(parent, id, &p, &formal));
            NLValueId value = n->context->bindings[formal - 1].view.value;
            n->context->bindings[formal - 1].view.value =
                saved.results[0].value;
            CHECK(!validates(n->entry));
            n->context->bindings[formal - 1].view.value = value;
            NLCheckedNodeView *use = NULL;
            for (size_t i = 0; i < body->count; ++i)
                if (body->nodes[i].kind == NL_CHECKED_IDENTIFIER &&
                    body->nodes[i].symbol == formal) {
                    use = &body->nodes[i];
                    break;
                }
            CHECK(use != NULL);
            use->symbol = 0;
            CHECK(!validates(n->entry));
            use->symbol = formal;
            CHECK(validates(n->entry));
            rejected += 6;
        }
    }
    CHECK(rejected == 24);
    puts("CONTROLS 24 REJECT C0 restored PASS semantic snapshot UNCHANGED");
    return true;
}

int main(int argc, char **argv)
{
    TestNode n = {0};
    size_t calls = 0;
    if ((argc != 2 && argc != 3) || !node_checked_load(argv[1], &n))
        return 1;
    V0Program p = {.unit = nl_semantic_unit_type(n.context),
                   .u8 = nl_semantic_core_type(n.context, NL_TYPE_U8)};
    if (!frames(n.entry, p, &calls) || calls != 4)
        return 1;
    printf("SOURCE CHECK OWNED PASS frames=%zu emitter=%d\n", calls,
           v0_validate_call(n.entry, nl_checked_root(n.entry), &p));
    if (argc == 3 && (!validates(n.entry) || !controls(&n)))
        return 1;
    node_checked_destroy(&n);
    return 0;
}
