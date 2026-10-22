/* Reuse the exact existing direct-return backend controls; new private
 * mutations exercise local/consume/source and control-origin evidence. */
#define main p733_existing_backend_main
#include "../unit/live_return_backend_test.c"
#undef main
#include "../../src/producer_local_internal.h"
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void *__wrap_calloc(size_t n, size_t s)
{
    return inject && calls++ == at ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return inject && calls++ == at ? NULL : __real_realloc(p, n);
}
static bool local_controls(const char *path)
{
    CHECK(controls(path));
    TestNode n = {0}, foreign = {0};
    CHECK(node_checked_load(path, &n) && node_checked_load(path, &foreign));
    NLCheckedNodeId id = 0, otherid = 0;
    NLCheckedFragment *f = producer(n.entry, &id),
                      *other = producer(foreign.entry, &otherid);
    NLProducerLocalReturnView v, unrelated;
    CHECK(f && other && nl_checked_producer_local_return_view(f, id, &v) &&
          nl_checked_producer_local_return_view(other, otherid, &unrelated) &&
          nl_checked_producer_local_return_valid(f, id, &v));
    CHECK(id == otherid && v.result.value == unrelated.result.value &&
          v.returned != unrelated.returned);
    char *baseline = NULL;
    size_t length = 0;
    CHECK(nl_checked_c_node(n.entry, &baseline, &length) == NL_NODE_C_OK);
    size_t rejects = 0;
#define VIEW(change)                                                           \
    do {                                                                       \
        NLProducerLocalReturnView bad = v;                                     \
        change;                                                                \
        CHECK(!nl_checked_producer_local_return_valid(f, id, &bad));           \
        ++rejects;                                                             \
    } while (0)
    VIEW(bad.body = unrelated.body);
    VIEW(bad.entry = unrelated.entry);
    VIEW(bad.returned = unrelated.returned);
    VIEW(bad.local++);
    VIEW(bad.declaration++);
    VIEW(bad.construction++);
    VIEW(bad.consumer++);
    VIEW(bad.return_node++);
    VIEW(bad.result.value++);
    VIEW(bad.result.type++);
#undef VIEW
    NLCheckedFragment *body = (NLCheckedFragment *)v.body;
#define REFUSE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        CHECK(!nl_checked_producer_local_return_valid(f, id, NULL));           \
        CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));                    \
        restore;                                                               \
        CHECK(nl_checked_producer_local_return_valid(f, id, &v));              \
        CHECK(expect(&n, NL_NODE_C_OK, baseline));                             \
        ++rejects;                                                             \
    } while (0)
    for (size_t i = 0; i < body->count; ++i) {
        NLCheckedNodeView saved = body->nodes[i];
        REFUSE(body->nodes[i].span.start_byte++, body->nodes[i] = saved);
        if (saved.symbol) {
            REFUSE(body->nodes[i].symbol++, body->nodes[i] = saved);
        }
        if (saved.result_count) {
            REFUSE(body->nodes[i].results[0].value++, body->nodes[i] = saved);
        }
        if (saved.kind == NL_CHECKED_IDENTIFIER) {
            REFUSE(body->nodes[i].value_use = NL_VALUE_USE_NONE,
                   body->nodes[i] = saved);
        }
        if (saved.kind == NL_CHECKED_BINDING) {
            REFUSE(body->nodes[i].next_item = 0, body->nodes[i] = saved);
            REFUSE(body->nodes[i].initializer = v.consumer,
                   body->nodes[i] = saved);
        }
        if (saved.kind == NL_CHECKED_RETURN) {
            REFUSE(body->nodes[i].returned.value++, body->nodes[i] = saved);
            REFUSE(body->nodes[i].terminates = false, body->nodes[i] = saved);
        }
    }
    NLSemanticContext *b = (NLSemanticContext *)v.returned;
    NLBindingEntry binding = b->bindings[v.local - 1];
    REFUSE(b->bindings[v.local - 1].view.availability = NL_AVAILABLE,
           b->bindings[v.local - 1] = binding);
    REFUSE(b->bindings[v.local - 1].hidden = false,
           b->bindings[v.local - 1] = binding);
    REFUSE(b->bindings[v.local - 1].view.place = f->nodes[id - 1].producer.root,
           b->bindings[v.local - 1] = binding);
    const NLControlTarget *target = body->function_target;
    NLControlExits *exits = body->exits;
    NLCheckedFragment *otherbody = (NLCheckedFragment *)unrelated.body;
    REFUSE(body->function_target = otherbody->function_target,
           body->function_target = (NLControlTarget *)target);
    REFUSE(body->exits = otherbody->exits, body->exits = exits);
    REFUSE((body->exits = otherbody->exits,
            body->function_target = otherbody->function_target),
           (body->exits = exits,
            body->function_target = (NLControlTarget *)target));
    NLControlExitView *edge =
        (NLControlExitView *)nl_control_exit_view(body->exits, 0);
    NLControlExitView edge_saved = *edge;
    REFUSE(edge->kind = NL_EXIT_BREAK, *edge = edge_saved);
    REFUSE(edge->values[0].value++, *edge = edge_saved);
    REFUSE(edge->count = 0, *edge = edge_saved);
    const NLSource *source = body->source;
    REFUSE(body->source = otherbody->source, body->source = (NLSource *)source);
    NLProducerLocalProof *seal = body->producer_local;
    printf("local-return issuer-seal bytes=%zu nodes=%zu "
           "additional_semantic_worlds=0\n",
           sizeof(*seal) + seal->count * sizeof(*seal->nodes), seal->count);
    NLProducerLocalProof issued = *seal;
    REFUSE(seal->caller = other, *seal = issued);
    REFUSE(seal->body = otherbody, *seal = issued);
    REFUSE(seal->source = otherbody->source, *seal = issued);
    REFUSE(seal->plan = otherbody->body_owner, *seal = issued);
    REFUSE(seal->context = otherbody->context, *seal = issued);
    REFUSE(seal->call++, *seal = issued);
    REFUSE(seal->count++, *seal = issued);
    REFUSE(seal->root++, *seal = issued);
    REFUSE(seal->origin = otherbody->nodes, *seal = issued);
    REFUSE(seal->nodes = NULL, *seal = issued);
    REFUSE(seal->issued.local++, *seal = issued);
    REFUSE(seal->call_node.producer.root++, *seal = issued);
    REFUSE(seal->exit_state = nl_control_exit_view(otherbody->exits, 0)->state,
           *seal = issued);
    NLCheckedNodeView consumed = body->nodes[v.consumer - 1];
    REFUSE((body->nodes[v.consumer - 1].value_use = NL_VALUE_COPIED,
            seal->nodes[v.consumer - 1] = body->nodes[v.consumer - 1]),
           (body->nodes[v.consumer - 1] = consumed,
            seal->nodes[v.consumer - 1] = consumed));
#undef REFUSE
    TestState *current = malloc(sizeof(*current)),
              *returned = malloc(sizeof(*returned));
    CHECK(current && returned && test_state(n.context, current) &&
          test_state(b, returned));
    size_t validation = 0, emission = 0;
    for (at = 0; at < 10000; ++at) {
        calls = 0;
        inject = true;
        bool ok = nl_checked_producer_local_return_valid(f, id, &v);
        inject = false;
        CHECK(test_unchanged(n.context, current) &&
              test_unchanged(b, returned));
        if (ok)
            break;
        CHECK(calls > at && nl_checked_producer_local_return_valid(f, id, &v));
        ++validation;
    }
    CHECK(at < 10000 && validation);
    for (at = 0; at < 10000; ++at) {
        calls = 0;
        inject = true;
        char *out = NULL;
        size_t sentinel = 37;
        NLNodeCStatus status = nl_checked_c_node(n.entry, &out, &sentinel);
        inject = false;
        CHECK(test_unchanged(n.context, current) &&
              test_unchanged(b, returned));
        if (status == NL_NODE_C_OK) {
            CHECK(out && strcmp(out, baseline) == 0);
            free(out);
            break;
        }
        CHECK(calls > at && !out && sentinel == 37 &&
              (status == NL_NODE_C_UNSUPPORTED ||
               status == NL_NODE_C_OUT_OF_MEMORY));
        CHECK(nl_checked_producer_local_return_valid(f, id, &v));
        ++emission;
    }
    CHECK(at < 10000 && emission);
    free(current);
    free(returned);
    printf("local-return structural/origin rejects=%zu; validator "
           "malloc/calloc/realloc OOM=%zu; admission/staging OOM=%zu; "
           "rollback+recovery+NULL output PASS\n",
           rejects, validation, emission);
    free(baseline);
    node_checked_destroy(&n);
    node_checked_destroy(&foreign);
    return true;
}
int main(int argc, char **argv)
{
    if (argc < 2)
        return 1;
    for (int i = 1; i < argc; ++i)
        if (!local_controls(argv[i]))
            return 1;
    return 0;
}
