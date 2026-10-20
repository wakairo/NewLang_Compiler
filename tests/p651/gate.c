/* Genuine source-issued owned artifact; private corruptions are test-only.
 * No semantic seeds or guest-state injection. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool tracking;
static size_t allocations;
void *__wrap_malloc(size_t n)
{
    if (tracking)
        ++allocations;
    return __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    if (tracking)
        ++allocations;
    return __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (tracking)
        ++allocations;
    return __real_realloc(p, n);
}
static bool valid(const TestNode *n)
{
    V0Program p = {.unit = nl_semantic_unit_type(n->context),
                   .u8 = nl_semantic_core_type(n->context, NL_TYPE_U8)};
    allocations = 0;
    tracking = true;
    const bool result =
        v0_validate_call(n->entry, nl_checked_root(n->entry), &p);
    tracking = false;
    return result && allocations == 0;
}
static uint64_t world_hash(const NLSemanticContext *c, uint64_t h)
{
    const void *data[] = {c, c->values, c->places, c->bindings, c->scopes};
    const size_t size[] = {sizeof(*c), c->value_count * sizeof(*c->values),
                           c->place_count * sizeof(*c->places),
                           c->binding_count * sizeof(*c->bindings),
                           c->scope_count * sizeof(*c->scopes)};
    for (size_t j = 0; j < 5; ++j)
        for (size_t i = 0; i < size[j]; ++i)
            h = (h ^ ((const unsigned char *)data[j])[i]) *
                UINT64_C(1099511628211);
    return h;
}
static uint64_t fingerprint(const NLCheckedFragment *f,
                            const NLCheckedFragment *arms[2])
{
    uint64_t h =
        world_hash(nl_checked_context(f), UINT64_C(14695981039346656037));
    for (size_t a = 0; a < 2; ++a)
        h = world_hash(nl_checked_context(arms[a]), h);
    const NLCheckedFragment *fragments[3] = {f, arms[0], arms[1]};
    for (size_t k = 0; k < 3; ++k) {
        const unsigned char *data = (const unsigned char *)fragments[k]->nodes;
        for (size_t i = 0;
             i < fragments[k]->count * sizeof(*fragments[k]->nodes); ++i)
            h = (h ^ data[i]) * UINT64_C(1099511628211);
    }
    return h;
}
static bool probe(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n) && valid(&n));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    NLCheckedNodeId choice = 0;
    const NLCheckedNodeView *decl[2] = {0}, *loans[2] = {0}, *writes[2] = {0};
    size_t roots = 0, scopes = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_IF) {
            CHECK(choice == 0);
            choice = i;
        }
        if (v->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, v->initializer);
            if (init->kind == NL_CHECKED_AGGREGATE) {
                CHECK(roots < 2);
                decl[roots++] = v;
            }
        }
        if (v->kind == NL_CHECKED_LOAN_HEADER) {
            CHECK(scopes < 2);
            loans[scopes] = v;
            const NLCheckedNodeView *body =
                nl_checked_node_view(f, v->initializer);
            writes[scopes++] = nl_checked_node_view(f, body->tail);
        }
    }
    CHECK(roots == 2 && scopes == 2 && choice);
    const NLCheckedFragment *arms[2] = {nl_checked_if_arm(f, choice, 0),
                                        nl_checked_if_arm(f, choice, 1)};
    CHECK(arms[0] && arms[1] &&
          nl_checked_context(arms[0]) != nl_checked_context(arms[1]));
    const NLSemanticContext *entry = nl_checked_context(arms[0]);
    const NLSemanticContext *c = nl_checked_context(f);
    const NLCheckedNodeView *selection = nl_checked_node_view(f, choice);
    const NLCheckedNodeView *cond =
        nl_checked_node_view(f, selection->initializer);
    NLSemanticValueView truth;
    CHECK(nl_semantic_value_view(c, cond->results[0].value, &truth) &&
          truth.scalar_known);
    printf("{\"if\":%zu,\"condition_symbol\":%zu,\"truth\":%zu,\"arms\":[",
           choice, cond->symbol, truth.scalar_value);
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedNodeView *r =
            nl_checked_node_view(arms[a], nl_checked_root(arms[a]));
        NLSemanticValueView v;
        CHECK(nl_semantic_value_view(nl_checked_context(arms[a]),
                                     r->results[0].value, &v) &&
              v.scalar_known);
        printf("%s%zu", a ? "," : "", v.scalar_value);
    }
    printf("],\"roots\":[");
    for (size_t k = 0; k < 2; ++k) {
        NLSemanticBindingView b;
        NLSemanticPlaceView r, child[2];
        CHECK(nl_semantic_binding_view(entry, decl[k]->symbol, &b) &&
              nl_semantic_place_view(entry, b.place, &r) &&
              r.fixed_field_count == 2);
        size_t values[2], read_symbols[2] = {0}, old_symbol = 0;
        for (size_t j = 0; j < 2; ++j) {
            NLSemanticValueView v;
            CHECK(nl_semantic_place_view(entry, r.fixed_fields[j], &child[j]) &&
                  nl_semantic_value_view(entry, child[j].current_value, &v) &&
                  v.scalar_known);
            values[j] = v.scalar_value;
        }
        for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
            const NLCheckedNodeView *v = nl_checked_node_view(f, i);
            if (v->kind != NL_CHECKED_BINDING)
                continue;
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, v->initializer);
            if (init == loans[k])
                old_symbol = v->symbol;
            if (init->kind == NL_CHECKED_FIELD_READ &&
                init->field.parent == b.place) {
                CHECK(init->field.child == r.fixed_fields[init->field.index] &&
                      init->field.child_incarnation ==
                          child[init->field.index].incarnation);
                if (init->field.index == 1)
                    CHECK(init->field.child_fact == child[1].current_fact);
                read_symbols[init->field.index] = v->symbol;
            }
        }
        CHECK(old_symbol && read_symbols[0] && read_symbols[1] &&
              writes[k]->field.parent == b.place &&
              writes[k]->field.child == r.fixed_fields[0]);
        printf("%s{\"symbol\":%zu,\"place\":%zu,\"incarnation\":%zu,\"fields\":"
               "[%zu,%zu],"
               "\"initial\":[%zu,%zu],\"old_symbol\":%zu,\"read_symbols\":[%zu,"
               "%zu],"
               "\"ref_symbol\":%zu,\"scope\":%zu,\"facts\":[%zu,%zu,%zu,%zu]}",
               k ? "," : "", decl[k]->symbol, b.place, r.incarnation,
               r.fixed_fields[0], r.fixed_fields[1], values[0], values[1],
               old_symbol, read_symbols[0], read_symbols[1],
               loans[k]->loan.ref_symbol, loans[k]->loan.scope,
               writes[k]->field.parent_fact, writes[k]->field.parent_post_fact,
               writes[k]->field.child_fact, writes[k]->field.child_post_fact);
    }
    puts("]}");
    NLCheckedNodeView *second = (NLCheckedNodeView *)writes[1];
    const NLCheckedNodeView saved = *second;
    size_t rejected = 0;
    const uint64_t original = fingerprint(f, arms);
#define REFUSE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        const uint64_t mutated = fingerprint(f, arms);                         \
        CHECK(!valid(&n) && allocations == 0 &&                                \
              mutated == fingerprint(f, arms));                                \
        restore;                                                               \
        CHECK(original == fingerprint(f, arms) && valid(&n));                  \
        ++rejected;                                                            \
    } while (0)
    REFUSE(second->field.parent = writes[0]->field.parent, *second = saved);
    REFUSE(second->field.child = writes[0]->field.child, *second = saved);
    REFUSE(second->field.base = writes[0]->field.base, *second = saved);
    REFUSE(second->field.index = 1, *second = saved);
    REFUSE(second->field.old_value = writes[0]->field.old_value,
           *second = saved);
    REFUSE(second->field.child_fact = writes[0]->field.child_fact,
           *second = saved);
    REFUSE(second->field.parent_fact = writes[0]->field.parent_fact,
           *second = saved);
    REFUSE(second->field.parent_post_fact = writes[0]->field.parent_post_fact,
           *second = saved);
    REFUSE(second->field.new_value = 0, *second = saved);
    NLSemanticContext *world = (NLSemanticContext *)c;
    const NLSemanticPlaceView closed =
        world->places[writes[0]->field.parent - 1];
    REFUSE(world->places[writes[0]->field.parent - 1].current_fact = 1,
           world->places[writes[0]->field.parent - 1] = closed);
    const NLSemanticScopeView scope = world->scopes[1];
    REFUSE(world->scopes[1].active = true, world->scopes[1] = scope);
    NLCheckedFragment *arm = (NLCheckedFragment *)arms[0];
    const NLSemanticContext *saved_context = arm->context;
    REFUSE(arm->context = (NLSemanticContext *)nl_checked_context(arms[1]),
           arm->context = saved_context);
    const size_t saved_index = arm->if_arm_index;
    REFUSE(arm->if_arm_index = 1, arm->if_arm_index = saved_index);
    NLSemanticContext *w = (NLSemanticContext *)nl_checked_context(arms[0]);
    const NLPlaceId root = writes[0]->field.parent,
                    child = writes[0]->field.child;
    const NLSemanticPlaceView old_root = w->places[root - 1],
                              old_child = w->places[child - 1];
    REFUSE(w->places[root - 1].current_value = 0,
           w->places[root - 1] = old_root);
    REFUSE(w->places[child - 1].current_value = w->value_count + 1,
           w->places[child - 1] = old_child);
#undef REFUSE
    printf("{\"mutants_rejected\":%zu,\"original\":true,\"worlds_unchanged\":"
           "true,\"driver_allocations\":0}\n",
           rejected);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && probe(argv[1]) ? 0 : 1;
}
