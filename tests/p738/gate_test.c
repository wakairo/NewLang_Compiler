#define main p738_existing_backend_main
#include "../unit/live_return_backend_test.c"
#undef main
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
static bool flow(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    NLCheckedNodeId call_id = 0;
    NLCheckedFragment *f = producer(n.entry, &call_id);
    CHECK(f && nl_checked_producer_local_return_valid(f, call_id, NULL));
    char *baseline = NULL;
    size_t length = 0;
    CHECK(nl_checked_c_node(n.entry, &baseline, &length) == NL_NODE_C_OK);
    NLCheckedNodeView *call = &f->nodes[call_id - 1], *attach = NULL;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_REPLACE &&
            f->nodes[i].field.parent == call->producer.head.parent)
            attach = &f->nodes[i];
    CHECK(attach);
    NLCheckedNodeView *project = &f->nodes[attach->first_argument - 1],
                      *some = &f->nodes[project->next_argument - 1],
                      *payload = &f->nodes[some->initializer - 1];
    size_t rejected = 0;
#define REFUSE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));                    \
        restore;                                                               \
        CHECK(nl_checked_producer_local_return_valid(f, call_id, NULL));       \
        CHECK(expect(&n, NL_NODE_C_OK, baseline));                             \
        ++rejected;                                                            \
    } while (0)
    NLCheckedNodeView saved = *payload, initial = *attach, constructor = *some;
    NLSymbolId head_symbol = 0;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_REF_FROM_PTR &&
            f->nodes[i].lifetime_place == call->producer.head.parent) {
            head_symbol = f->nodes[f->nodes[i].first_argument - 1].symbol;
            break;
        }
    CHECK(head_symbol && head_symbol != payload->symbol);
    REFUSE(payload->symbol = head_symbol, *payload = saved);
    REFUSE(payload->results[0].value++, *payload = saved);
    REFUSE(payload->value_use = NL_VALUE_CONSUMED, *payload = saved);
    REFUSE(some->initializer = 0, *some = constructor);
    REFUSE(some->variant = 1, *some = constructor);
    REFUSE(some->results[0].value++, *some = constructor);
    REFUSE(attach->kind = NL_CHECKED_UNIT, *attach = initial);
    REFUSE(attach->field.index = 1, *attach = initial);
    REFUSE(attach->field.parent = call->producer.root, *attach = initial);
    REFUSE(attach->field.child = call->producer.root, *attach = initial);
    REFUSE(attach->field.child_incarnation++, *attach = initial);
    REFUSE(attach->field.parent_post_fact++, *attach = initial);
    REFUSE(attach->field.child_post_fact++, *attach = initial);
    REFUSE(attach->field.post_payload_occurrence = 0, *attach = initial);
    REFUSE(attach->field.payload_occurrence = 1, *attach = initial);
    REFUSE(attach->field.new_value++, *attach = initial);
    NLCheckedNodeView *actual = &f->nodes[call->first_argument - 1];
    NLCheckedNodeView field_saved = *actual;
    NLCheckedNodeView *pointer = &f->nodes[actual->next_argument - 1];
    NLCheckedNodeView pointer_saved = *pointer;
    REFUSE(pointer->symbol = head_symbol, *pointer = pointer_saved);
    REFUSE(actual->field.parent = call->producer.root, *actual = field_saved);
    REFUSE(actual->field.index = 1, *actual = field_saved);
    REFUSE(actual->field.child_fact++, *actual = field_saved);
    REFUSE(actual->field.payload_occurrence = 0, *actual = field_saved);
    NLCheckedNodeView *base = &f->nodes[actual->first_argument - 1],
                      *binding = NULL;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_BINDING &&
            f->nodes[i].symbol == base->symbol)
            binding = &f->nodes[i];
    CHECK(binding);
    NLCheckedNodeView declaration = *binding;
    NLCheckedNodeView *ref = &f->nodes[binding->initializer - 1];
    NLCheckedNodeView reference = *ref;
    REFUSE(ref->reference_result.writable = false, *ref = reference);
    REFUSE(ref->reference_result.scope++, *ref = reference);
    REFUSE(ref->lifetime_domain = call->producer.domain, *ref = reference);
    REFUSE(ref->lifetime_place = call->producer.root, *ref = reference);
    REFUSE(binding->initializer = call_id, *binding = declaration);
    REFUSE(binding->argument_count = 0, *binding = declaration);
    NLCheckedNodeView *first_binding = NULL;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_BINDING && f->nodes[i].initializer &&
            f->nodes[f->nodes[i].initializer - 1].kind ==
                NL_CHECKED_LOAN_HEADER &&
            f->nodes[i].next_item && f->nodes[i].type == 1) {
            first_binding = &f->nodes[i];
            break;
        }
    CHECK(first_binding);
    NLCheckedNodeView repeated = *first_binding;
    REFUSE(first_binding->next_item = (size_t)(first_binding - f->nodes) + 1,
           *first_binding = repeated);
    TestNode foreign = {0};
    CHECK(node_checked_load(path, &foreign));
    NLCheckedNodeId other_id = 0;
    NLCheckedFragment *other = producer(foreign.entry, &other_id);
    CHECK(other && other_id == call_id && other != f &&
          other->nodes[other_id - 1].producer.result == call->producer.result);
    NLCheckedNodeView issued = *call;
    REFUSE(*call = other->nodes[other_id - 1], *call = issued);
    node_checked_destroy(&foreign);
#undef REFUSE
    printf("P738 attach/ref/currentness/edge/world correspondence rejects=%zu "
           "restore_identical_C=PASS\n",
           rejected);
    free(baseline);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    if (argc < 2)
        return 1;
    for (int i = 1; i < argc; ++i)
        if (!flow(argv[i]))
            return 1;
    return 0;
}
