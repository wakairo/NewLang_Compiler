/* Test-private faults only. Trusted source issuer and immutable receipt are
 * not exposed for alteration by any production public API. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"

static bool driver(const TestNode *n)
{
    V0Program p = {.unit = nl_semantic_unit_type(n->context),
                   .u8 = nl_semantic_core_type(n->context, NL_TYPE_U8)};
    return v0_validate_call(n->entry, nl_checked_root(n->entry), &p);
}
static uint64_t bytes_hash(uint64_t h, const void *p, size_t n)
{
    const unsigned char *b = p;
    for (size_t i = 0; i < n; ++i)
        h = (h ^ b[i]) * UINT64_C(1099511628211);
    return h;
}
static uint64_t fingerprint(const NLCheckedFragment *f)
{
    const NLSemanticContext *c = f->context;
    uint64_t h = UINT64_C(14695981039346656037);
    h = bytes_hash(h, f, sizeof(*f));
    h = bytes_hash(h, f->nodes, f->count * sizeof(*f->nodes));
    h = bytes_hash(h, c, sizeof(*c));
    h = bytes_hash(h, c->types, c->type_count * sizeof(*c->types));
    h = bytes_hash(h, c->values, c->value_count * sizeof(*c->values));
    h = bytes_hash(h, c->bindings, c->binding_count * sizeof(*c->bindings));
    h = bytes_hash(h, c->places, c->place_count * sizeof(*c->places));
    h = bytes_hash(h, c->scopes, c->scope_count * sizeof(*c->scopes));
    return h;
}
static bool controls(const char *path)
{
    TestNode n = {0}, foreign = {0};
    CHECK(node_checked_load(path, &n) && node_checked_load(path, &foreign));
    CHECK(driver(&n));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    const NLCheckedFragment *other =
        nl_checked_call_body(foreign.entry, nl_checked_root(foreign.entry));
    size_t id = 0;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_USIZE_ADDITION)
            id = i + 1;
    CHECK(id);
    NLUsizeAdditionView v, alt;
    CHECK(nl_checked_usize_addition_view(f, id, &v));
    CHECK(nl_checked_usize_addition_view(other, id, &alt));
    CHECK(v.node == alt.node && v.result == alt.result &&
          v.origin != alt.origin);
    CHECK(!nl_checked_usize_addition_valid(f, id, &alt));
    size_t refused = 1;
#define COPY_FAULT(change)                                                     \
    do {                                                                       \
        alt = v;                                                               \
        change;                                                                \
        CHECK(!nl_checked_usize_addition_valid(f, id, &alt));                  \
        ++refused;                                                             \
    } while (0)
    COPY_FAULT(alt.type = 1);
    COPY_FAULT(alt.node += 1);
    COPY_FAULT(alt.result = v.values[0]);
    COPY_FAULT(alt.operands[0] = v.operands[1]);
    COPY_FAULT(alt.values[0] = v.values[1]);
    COPY_FAULT(alt.numbers[0] ^= 1);
    COPY_FAULT(alt.sum += 1);
    COPY_FAULT((alt.numbers[0] = SIZE_MAX, alt.numbers[1] = 1));
    COPY_FAULT(alt.operator_span.start_byte += 1);
    COPY_FAULT(alt.span.end_byte -= 1);
    COPY_FAULT(alt.operand_spans[0].end_byte -= 1);
    COPY_FAULT(alt.facts[0] += 1);
    COPY_FAULT(alt.incarnations[0] += 1);
    COPY_FAULT(alt.places[0] += 1);
    COPY_FAULT(alt.symbols[0] += 1);
#undef COPY_FAULT
    NLCheckedNodeView *op = &f->nodes[id - 1],
                      *lhs = &f->nodes[v.operands[0] - 1],
                      *rhs = &f->nodes[v.operands[1] - 1];
    const NLCheckedNodeView os = *op, ls = *lhs, rs = *rhs;
    NLSemanticContext *c = (NLSemanticContext *)f->context;
    NLSemanticValueView *result = &c->values[v.result - 1],
                        *left = &c->values[v.values[0] - 1];
    const NLSemanticValueView result_saved = *result, left_saved = *left;
#define FAULT(change, restore)                                                 \
    do {                                                                       \
        change;                                                                \
        NLSemanticSnapshot before = {0}, after = {0};                          \
        CHECK(nl_semantic_snapshot(c, &before));                               \
        const uint64_t before_bytes = fingerprint(f);                          \
        const bool accepted = nl_checked_usize_addition_valid(f, id, NULL);    \
        CHECK(!driver(&n));                                                    \
        CHECK(nl_semantic_snapshot(c, &after));                                \
        const bool same = memcmp(&before, &after, sizeof before) == 0 &&       \
                          before_bytes == fingerprint(f);                      \
        restore;                                                               \
        CHECK(!accepted && same && driver(&n));                                \
        ++refused;                                                             \
    } while (0)
    FAULT(op->kind = NL_CHECKED_USIZE_EQUALITY, *op = os);
    FAULT(op->type = 1, *op = os);
    FAULT(op->result_count = 0, *op = os);
    FAULT(op->argument_count = 1, *op = os);
    FAULT(op->first_argument = v.operands[1], *op = os);
    FAULT(op->name.start_byte += 1, *op = os);
    FAULT(op->span.end_byte -= 1, *op = os);
    FAULT(op->scalar_result.value += 1, *op = os);
    FAULT(op->has_scalar_result = false, *op = os);
    FAULT(op->results[0].value = v.values[0], *op = os);
    FAULT(lhs->next_argument = 0, *lhs = ls);
    FAULT(rhs->next_argument = v.operands[0], *rhs = rs);
    FAULT(lhs->results[0].value = v.values[1], *lhs = ls);
    FAULT(lhs->span.start_byte += 1, *lhs = ls);
    FAULT(left->scalar_value ^= 1, *left = left_saved);
    FAULT(left->scalar_known = false, *left = left_saved);
    FAULT(left->type = 1, *left = left_saved);
    FAULT(result->type = 1, *result = result_saved);
    FAULT(result->reference.readable = true, *result = result_saved);
    FAULT(left->dependencies = NL_HIDDEN_DEPENDENCIES, *left = left_saved);
    FAULT(left->value_dependency_count = 1, *left = left_saved);
    FAULT(left->domain = 1, *left = left_saved);
    FAULT(left->reference.place = 1, *left = left_saved);
    FAULT(left->reference.scope = 1, *left = left_saved);
    FAULT(left->reference.incarnation = 1, *left = left_saved);
    FAULT(left->reference.occurrence_dependency = 1, *left = left_saved);
    FAULT(result->allocation_region = 1, *result = result_saved);
    FAULT(result->scalar_value += 1, *result = result_saved);
    if (v.symbols[0]) {
        NLSemanticPlaceView *place = &c->places[v.places[0] - 1];
        const NLSemanticPlaceView saved = *place;
        NLSemanticBindingView *binding = &c->bindings[v.symbols[0] - 1].view;
        const NLSemanticBindingView bs = *binding;
        FAULT(binding->value = v.values[0], *binding = bs);
        FAULT(binding->place = 0, *binding = bs);
        FAULT(place->incarnation += 1, *place = saved);
        CHECK(!place->live);
        FAULT(place->current_fact = v.facts[0], *place = saved);
        FAULT(place->current_value = v.values[0], *place = saved);
        FAULT(lhs->symbol = v.symbols[1], *lhs = ls);
        FAULT(lhs->value_use = NL_VALUE_CONSUMED, *lhs = ls);
    }
    struct NLUsizeAdditionProof *issued = f->usize_additions;
    FAULT(f->usize_additions = NULL, f->usize_additions = issued);
    FAULT(f->usize_additions = other->usize_additions,
          f->usize_additions = issued);
    NLFunctionBody *owner = f->body_owner;
    FAULT(f->body_owner = NULL, f->body_owner = owner);
    const NLSource *source = f->source;
    FAULT(f->source = n.entry_source, f->source = source);
#undef FAULT
    /* A provisional standalone source fragment is observable, never certified.
     */
    NLSemanticContext *provisional_context = NULL;
    NLCheckedFragment *provisional = NULL;
    CHECK(nl_semantic_create(&provisional_context) == NL_CHECK_OK);
    CHECK(nl_semantic_check_source_fragment(provisional_context, owner->syntax,
                                            &provisional, NULL) == NL_CHECK_OK);
    size_t provisional_id = 0;
    for (size_t i = 1; i <= nl_checked_node_count(provisional); ++i)
        if (nl_checked_node_view(provisional, i)->kind ==
            NL_CHECKED_USIZE_ADDITION)
            provisional_id = i;
    CHECK(provisional_id);
    CHECK(nl_checked_usize_addition_view(provisional, provisional_id, &alt));
    CHECK(!nl_checked_usize_addition_valid(provisional, provisional_id, &alt));
    NLSemanticSnapshot before = {0}, after = {0};
    CHECK(nl_semantic_snapshot(provisional_context, &before));
    NLCheckedFragment *slot = provisional;
    CHECK(nl_semantic_check_source_fragment(provisional_context, owner->syntax,
                                            &slot,
                                            NULL) == NL_CHECK_INTERNAL_ERROR &&
          slot == provisional);
    CHECK(nl_semantic_snapshot(provisional_context, &after) &&
          memcmp(&before, &after, sizeof before) == 0);
    nl_checked_destroy(provisional);
    nl_semantic_destroy(provisional_context);
    printf("rejected=%zu; original-validator-and-driver=PASS; "
           "receipt-view-bytes=%zu\n",
           refused, sizeof(v));
    node_checked_destroy(&foreign);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && controls(argv[1]) ? 0 : 1;
}
