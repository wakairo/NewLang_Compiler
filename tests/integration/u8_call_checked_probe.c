/* Actual-source owned evidence and driver-private fail-closed controls.
 * No registration seeds, synthesized callable, argument or result package. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"

static bool validates(const NLCheckedFragment *f)
{
    V0Program p = {
        .unit = nl_semantic_unit_type(nl_checked_context(f)),
        .u8 = nl_semantic_core_type(nl_checked_context(f), NL_TYPE_U8)};
    NLSemanticSnapshot before = {0}, after = {0};
    if (!nl_semantic_snapshot(nl_checked_context(f), &before))
        return false;
    const bool valid = v0_validate_call(f, nl_checked_root(f), &p);
    if (!nl_semantic_snapshot(nl_checked_context(f), &after) ||
        memcmp(&before, &after, sizeof(before)) != 0)
        return false;
    return valid;
}

static bool walk(const NLCheckedFragment *f, size_t *calls)
{
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        const NLCheckedFragment *body = nl_checked_call_body(f, i);
        if (body == NULL)
            continue;
        CHECK(nl_checked_context(f) == nl_checked_context(body));
        if (v->argument_count == 1) {
            V0Program p = {
                .unit = nl_semantic_unit_type(nl_checked_context(f)),
                .u8 = nl_semantic_core_type(nl_checked_context(f), NL_TYPE_U8)};
            NLSymbolId formal;
            CHECK(v1_call_parameter(f, i, &p, &formal));
            const NLCheckedNodeView *arg =
                nl_checked_node_view(f, v->first_argument);
            NLSemanticBindingView binding;
            NLSemanticValueView result;
            CHECK(nl_semantic_binding_view(nl_checked_context(body), formal,
                                           &binding));
            CHECK(binding.type == p.u8 &&
                  binding.value == arg->results[0].value);
            CHECK(nl_semantic_value_view(nl_checked_context(f),
                                         v->results[0].value, &result));
            CHECK(result.type == p.u8 &&
                  result.dependencies == NL_DEPENDENCY_FREE);
            printf("CALL %zu %zu %zu %zu %zu %zu %zu\n", v->function, i, formal,
                   arg->results[0].value, binding.value, v->results[0].value,
                   result.scalar_value);
            ++*calls;
        }
        CHECK(walk(body, calls));
    }
    return true;
}

static bool controls(TestNode *n)
{
    const NLCheckedFragment *parent =
        nl_checked_call_body(n->entry, nl_checked_root(n->entry));
    CHECK(parent != NULL);
    NLCheckedNodeId id = 0;
    for (size_t i = 1; i <= nl_checked_node_count(parent); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(parent, i);
        if (v->kind == NL_CHECKED_REGISTERED_CALL && v->argument_count == 1) {
            id = i;
            break;
        }
    }
    CHECK(id != 0);
    /* Mutating test-owned artifacts only, restoring before disposal. */
    NLCheckedNodeView *call = &((NLCheckedFragment *)parent)->nodes[id - 1];
    const NLCheckedNodeView saved = *call;
    call->body_backed = false;
    CHECK(!validates(n->entry));
    *call = saved;
    call->function =
        nl_checked_node_view(n->entry, nl_checked_root(n->entry))->function;
    CHECK(!validates(n->entry));
    *call = saved;
    call->results[0].value = 0;
    CHECK(!validates(n->entry));
    *call = saved;
    call->argument_count = 2;
    CHECK(!validates(n->entry));
    *call = saved;
    NLCheckedFragment *body =
        (NLCheckedFragment *)nl_checked_call_body(parent, id);
    NLFunctionBody *owner = body->body_owner;
    body->body_owner = NULL;
    CHECK(!validates(n->entry));
    body->body_owner = owner;
    const NLSemanticContext *world = body->context;
    NLSemanticContext *clone = NULL;
    CHECK(nl_sem_clone(world, &clone) == NL_CHECK_OK);
    body->context = clone;
    CHECK(!validates(n->entry));
    body->context = world;
    nl_semantic_destroy(clone);
    V0Program p = {.unit = nl_semantic_unit_type(world),
                   .u8 = nl_semantic_core_type(world, NL_TYPE_U8)};
    NLSymbolId formal;
    CHECK(v1_call_parameter(parent, id, &p, &formal));
    NLSemanticContext *c = n->context;
    NLValueId value = c->bindings[formal - 1].view.value;
    c->bindings[formal - 1].view.value = 0;
    CHECK(!validates(n->entry));
    c->bindings[formal - 1].view.value = value;
    CHECK(validates(n->entry));
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
    puts("CONTROLS 8 REJECT restored PASS snapshot UNCHANGED");
    return true;
}

int main(int argc, char **argv)
{
    TestNode n = {0};
    size_t calls = 0;
    if (argc != 2 || !node_checked_load(argv[1], &n) || !validates(n.entry) ||
        !walk(n.entry, &calls) || calls == 0 || !controls(&n))
        return 1;
    printf("OWNED %zu\n", calls);
    node_checked_destroy(&n);
    return 0;
}
