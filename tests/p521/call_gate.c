/* Test-owned corruptions of genuine source-issued evidence. Read-only driver
 * predicates are reused; no new issuer/public validator or semantic seed. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"
static bool valid(const TestNode *n)
{
    V0Program p = {.unit = 1,
                   .u8 = nl_semantic_core_type(n->context, NL_TYPE_U8)};
    return v0_validate_call(n->entry, nl_checked_root(n->entry), &p);
}
static uint64_t hash(uint64_t h, const void *p, size_t size)
{
    const unsigned char *b = p;
    for (size_t i = 0; i < size; ++i)
        h = (h ^ b[i]) * UINT64_C(1099511628211);
    return h;
}
static uint64_t fingerprint(const NLCheckedFragment *f)
{
    const NLSemanticContext *c = nl_checked_context(f);
    uint64_t h = hash(UINT64_C(14695981039346656037), f, sizeof(*f));
    h = hash(h, f->nodes, f->count * sizeof(*f->nodes));
    h = hash(h, c, sizeof(*c));
    h = hash(h, c->bindings, c->binding_count * sizeof(*c->bindings));
    h = hash(h, c->values, c->value_count * sizeof(*c->values));
    h = hash(h, c->places, c->place_count * sizeof(*c->places));
    for (size_t i = 0; i < f->body_count; ++i)
        if (f->bodies[i])
            h ^= fingerprint(f->bodies[i]);
    for (size_t i = 0; i < f->arm_count; ++i)
        if (f->arms[i].artifact)
            h ^= fingerprint(f->arms[i].artifact);
    return h;
}
static bool controls(const char *path)
{
    TestNode n = {0}, other = {0};
    CHECK(node_checked_load(path, &n) && node_checked_load(path, &other) &&
          valid(&n));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    const NLCheckedFragment *of =
        nl_checked_call_body(other.entry, nl_checked_root(other.entry));
    NLCheckedNodeId id = 0;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_REGISTERED_CALL)
            id = i + 1;
    CHECK(id);
    NLCheckedNodeView *call = &f->nodes[id - 1], saved = *call;
    NLCheckedNodeView *arg = &f->nodes[call->first_argument - 1], as = *arg;
    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(f, id);
    CHECK(body);
    NLSymbolId formal = 0;
    V0Program p = {.unit = 1,
                   .u8 = nl_semantic_core_type(n.context, NL_TYPE_U8)};
    CHECK(v1_call_parameter(f, id, &p, &formal));
    NLBindingEntry *binding = &n.context->bindings[formal - 1], bs = *binding;
    NLCheckedNodeView *tail =
                          &body->nodes[body->nodes[body->root - 1].tail - 1],
                      ts = *tail;
    size_t refused = 0;
#define REFUSE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        const uint64_t h = fingerprint(n.entry);                               \
        const bool accepted = valid(&n);                                       \
        CHECK(h == fingerprint(n.entry));                                      \
        restore;                                                               \
        CHECK(!accepted && valid(&n));                                         \
        ++refused;                                                             \
    } while (0)
    REFUSE(call->body_backed = false, *call = saved);
    REFUSE(
        call->function =
            nl_checked_node_view(n.entry, nl_checked_root(n.entry))->function,
        *call = saved);
    REFUSE(call->argument_count = 2, *call = saved);
    REFUSE(call->first_argument = 0, *call = saved);
    REFUSE(call->results[0].value = 0, *call = saved);
    REFUSE(arg->parameter_type =
               nl_semantic_core_type(n.context, NL_TYPE_USIZE),
           *arg = as);
    REFUSE(arg->symbol = 0, *arg = as);
    REFUSE(binding->hidden = false, *binding = bs);
    REFUSE(binding->view.value = 0, *binding = bs);
    REFUSE(tail->symbol = 0, *tail = ts);
    REFUSE(tail->results[0].value = arg->results[0].value, *tail = ts);
    NLFunctionBody *owner = body->body_owner;
    REFUSE(body->body_owner = NULL, body->body_owner = owner);
    const NLSource *source = body->source;
    REFUSE(body->source = nl_checked_source(f), body->source = source);
    const NLSemanticContext *context = body->context;
    REFUSE(body->context = other.context, body->context = context);
    NLSemanticValueView *result =
                            &n.context->values[call->results[0].value - 1],
                        rs = *result;
    REFUSE(result->scalar_known = true, *result = rs);
    REFUSE(result->domain = 1, *result = rs);
    REFUSE(result->allocation_region = 1, *result = rs);
    for (size_t i = 0; i < f->body_count; ++i)
        if (f->bodies[i] == body) {
            REFUSE(f->bodies[i] = NULL, f->bodies[i] = body);
            REFUSE(f->bodies[i] =
                       (NLCheckedFragment *)nl_checked_call_body(of, id),
                   f->bodies[i] = body);
        }
#undef REFUSE
    printf(
        "P521_CALL_GATE rejects=%zu restored=PASS evidence/worlds=UNCHANGED\n",
        refused);
    node_checked_destroy(&other);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && controls(argv[1]) ? 0 : 1;
}
