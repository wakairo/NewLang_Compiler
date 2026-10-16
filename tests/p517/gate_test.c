/* Driver-private controls mutate only test-owned genuine issued artifacts.
 * No production hook, semantic seed, output-value injection or public proof
 * claim. Every mutation is restored before original-artifact revalidation. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"
void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
void *__real_calloc(size_t, size_t);
static bool injecting;
static size_t allocation_index, fail_at;
void *__wrap_malloc(size_t n)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_realloc(p, n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    if (injecting && allocation_index++ == fail_at)
        return NULL;
    return __real_calloc(n, s);
}
static bool valid(const TestNode *n)
{
    V0Program p = {.unit = 1,
                   .u8 = nl_semantic_core_type(n->context, NL_TYPE_U8)};
    return v0_validate_call(n->entry, nl_checked_root(n->entry), &p);
}
typedef struct {
    const unsigned char *bytes;
    size_t size;
} ObservedBytes;
static ObservedBytes observed[48];
static size_t observed_count;
static void bytes(const void *p, size_t n)
{
    if (p != NULL && n != 0 && observed_count < 48)
        observed[observed_count++] = (ObservedBytes){p, n};
}
static void world_bytes(const NLSemanticContext *c)
{
    bytes(c, sizeof(*c));
    bytes(c->values, c->value_count * sizeof(*c->values));
    bytes(c->places, c->place_count * sizeof(*c->places));
    bytes(c->bindings, c->binding_count * sizeof(*c->bindings));
    bytes(c->scopes, c->scope_count * sizeof(*c->scopes));
}
static uint64_t fingerprint(void)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < observed_count; ++i)
        for (size_t j = 0; j < observed[i].size; ++j)
            hash = (hash ^ observed[i].bytes[j]) * UINT64_C(1099511628211);
    return hash;
}
/* Fork only the actually registered, value-free source registry, before
 * main(). Both later artifacts are issued by the real checker and share the
 * exact retained source plan; no host value, root or permission is seeded. */
static bool load_controls(const char *file, TestNode *n,
                          NLSemanticContext **same_registry)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    CHECK(nl_source_load(file, &source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK);
    CHECK(nl_semantic_create(&n->context) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {unit};
    CHECK(nl_semantic_register_function_unit(n->context, inputs, 1, NULL) ==
          NL_CHECK_OK);
    CHECK(nl_sem_clone(n->context, same_registry) == NL_CHECK_OK);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    TestChecked entry = {0};
    CHECK(test_run(n->context, "main()", TEST_EXPRESSION, NL_CHECK_OK, NULL,
                   &entry));
    n->entry_source = entry.source;
    n->entry = entry.artifact;
    return true;
}
static bool controls(const char *file)
{
    TestNode n = {0};
    NLSemanticContext *same_registry = NULL;
    CHECK(load_controls(file, &n, &same_registry) && valid(&n));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    NLCheckedNodeId id = 0;
    NLCheckedNodeView *write = NULL, *loan = NULL, *reads[2] = {0},
                      *declaration = NULL;
    for (size_t i = 0; i < f->count; ++i) {
        NLCheckedNodeView *v = &f->nodes[i];
        if (v->kind == NL_CHECKED_IF)
            id = i + 1;
        if (v->kind == NL_CHECKED_REPLACE)
            write = v;
        if (v->kind == NL_CHECKED_LOAN_HEADER)
            loan = v;
        if (v->kind == NL_CHECKED_FIELD_READ)
            reads[v->field.index] = v;
    }
    CHECK(id && write && loan && reads[0] && reads[1]);
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_BINDING &&
            f->nodes[i].symbol == write->field.base)
            declaration = &f->nodes[i];
    CHECK(declaration);
    NLCheckedNodeView *join = &f->nodes[id - 1],
                      *condition = &f->nodes[join->initializer - 1];
    const NLCheckedNodeView js = *join, cs = *condition, ds = *declaration,
                            ws = *write, ls = *loan;
    NLCheckedFragment *arms[2] = {
        (NLCheckedFragment *)nl_checked_if_arm(f, id, 0),
        (NLCheckedFragment *)nl_checked_if_arm(f, id, 1)};
    NLSemanticContext *parent = (NLSemanticContext *)f->context;
    const NLPlaceId root = ws.field.parent;
    const NLPlaceId child[2] = {ws.field.child, reads[1]->field.child};
    size_t slots[2] = {0};
    for (size_t i = 0; i < f->arm_count; ++i)
        for (size_t a = 0; a < 2; ++a)
            if (f->arms[i].artifact == arms[a])
                slots[a] = i;
    observed_count = 0;
    bytes(f, sizeof(*f));
    bytes(f->nodes, f->count * sizeof(*f->nodes));
    bytes(f->arms, f->arm_count * sizeof(*f->arms));
    world_bytes(parent);
    for (size_t a = 0; a < 2; ++a) {
        world_bytes(arms[a]->context);
        bytes(arms[a], sizeof(*arms[a]));
        bytes(arms[a]->nodes, arms[a]->count * sizeof(*arms[a]->nodes));
    }
    size_t refused = 0;
#define REFUSE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        const uint64_t before = fingerprint();                                 \
        const bool accepted = valid(&n);                                       \
        CHECK(fingerprint() == before);                                        \
        restore;                                                               \
        CHECK(!accepted && valid(&n));                                         \
        ++refused;                                                             \
    } while (0)
    REFUSE(declaration->symbol = 0, *declaration = ds);
    REFUSE(declaration->symbol = ws.field.base == 1 ? 2 : 1, *declaration = ds);
    REFUSE(join->normal_arms = 1, *join = js);
    REFUSE(join->initializer = 0, *join = js);
    REFUSE(join->span.start_byte += 1, *join = js);
    REFUSE(condition->results[0].value = 1, *condition = cs);
    REFUSE(condition->span.end_byte -= 1, *condition = cs);
    if (condition->kind == NL_CHECKED_USIZE_EQUALITY) {
        REFUSE(condition->kind = NL_CHECKED_IDENTIFIER, *condition = cs);
        REFUSE(condition->argument_count = 1, *condition = cs);
        REFUSE(condition->first_argument = 0, *condition = cs);
        REFUSE(condition->scalar_result.value = 1 - cs.scalar_result.value,
               *condition = cs);
        NLSemanticValueView *truth =
            &parent->values[condition->results[0].value - 1];
        const NLSemanticValueView original_truth = *truth;
        REFUSE((condition->scalar_result.value = 1 - cs.scalar_result.value,
                truth->scalar_value = 1 - original_truth.scalar_value),
               (*condition = cs, *truth = original_truth));
        NLCheckedNodeView *left = &f->nodes[condition->first_argument - 1];
        const NLCheckedNodeView left_saved = *left;
        REFUSE(left->symbol = ws.field.base, *left = left_saved);
        REFUSE(left->next_argument = 0, *left = left_saved);
    }
    REFUSE(f->arms[slots[0]].artifact = arms[1],
           f->arms[slots[0]].artifact = arms[0]);
    REFUSE((f->arms[slots[0]].artifact = arms[1],
            f->arms[slots[1]].artifact = arms[0]),
           (f->arms[slots[0]].artifact = arms[0],
            f->arms[slots[1]].artifact = arms[1]));
    REFUSE(write->field.base = 0, *write = ws);
    REFUSE(write->field.parent = root == 1 ? 2 : 1, *write = ws);
    REFUSE(write->field.child = child[1], *write = ws);
    REFUSE(write->field.parent_fact += 1, *write = ws);
    REFUSE(write->field.parent_incarnation += 1, *write = ws);
    REFUSE(write->field.child_incarnation += 1, *write = ws);
    REFUSE(write->field.child_fact += 1, *write = ws);
    REFUSE(write->field.old_value = ws.field.new_value, *write = ws);
    REFUSE(write->field.new_value = ws.field.old_value, *write = ws);
    REFUSE(loan->loan.domain = 1, *loan = ls);
    REFUSE(loan->loan.place = child[1], *loan = ls);
    REFUSE(loan->loan.source = ws.field.base == 1 ? 2 : 1, *loan = ls);
    REFUSE(loan->loan.scope = 2, *loan = ls);
    REFUSE(loan->loan.body_nonescape_proved = false, *loan = ls);
    REFUSE(parent->scopes[0].active = true, parent->scopes[0].active = false);
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView saved = *reads[i];
        REFUSE(reads[i]->field.parent = root == 1 ? 2 : 1, *reads[i] = saved);
        REFUSE(reads[i]->field.child_fact += 1, *reads[i] = saved);
        REFUSE(reads[i]->field.parent_fact = ws.field.parent_fact,
               *reads[i] = saved);
        REFUSE(reads[i]->field.old_value = 0, *reads[i] = saved);
    }
    for (size_t a = 0; a < 2; ++a) {
        NLCheckedFragment *arm = arms[a];
        NLSemanticContext *w = (NLSemanticContext *)arm->context;
        const NLCheckedFragment *owner = arm->if_parent;
        REFUSE(arm->if_parent = arms[1 - a], arm->if_parent = owner);
        const size_t arm_index = arm->if_arm_index;
        REFUSE(arm->if_arm_index = 1 - a, arm->if_arm_index = arm_index);
        const NLSemanticBindingView b = w->bindings[ws.field.base - 1].view;
        REFUSE(w->bindings[ws.field.base - 1].view.place = root == 1 ? 2 : 1,
               w->bindings[ws.field.base - 1].view = b);
        REFUSE(w->bindings[ws.field.base - 1].view.value = 1,
               w->bindings[ws.field.base - 1].view = b);
        for (size_t j = 0; j < 3; ++j) {
            NLPlaceId which = j == 0 ? root : child[j - 1];
            NLSemanticPlaceView saved = w->places[which - 1];
            REFUSE(w->places[which - 1].incarnation += 1,
                   w->places[which - 1] = saved);
            REFUSE(w->places[which - 1].current_fact += 1,
                   w->places[which - 1] = saved);
            REFUSE(w->places[which - 1].current_value = 0,
                   w->places[which - 1] = saved);
            REFUSE(w->places[which - 1].governing_domain = 1,
                   w->places[which - 1] = saved);
        }
        NLSemanticPlaceView saved = w->places[root - 1];
        REFUSE(w->places[root - 1].fixed_fields[0] = child[1],
               w->places[root - 1] = saved);
        NLCheckedNodeView *ar = &arm->nodes[arm->root - 1];
        const NLCheckedNodeView ar_saved = *ar;
        REFUSE(ar->terminates = true, *ar = ar_saved);
        NLSemanticValueView *result = &w->values[ar->results[0].value - 1],
                            saved_value = *result;
        REFUSE(result->scalar_value += 1, *result = saved_value);
    }
    TestChecked second = {0};
    CHECK(test_run(same_registry, "main()", TEST_EXPRESSION, NL_CHECK_OK, NULL,
                   &second));
    const NLCheckedFragment *other =
        nl_checked_call_body(second.artifact, nl_checked_root(second.artifact));
    CHECK(nl_checked_source(f) == nl_checked_source(other));
    NLCheckedFragment *foreign =
        (NLCheckedFragment *)nl_checked_if_arm(other, id, 0);
    REFUSE(f->arms[slots[0]].artifact = foreign,
           f->arms[slots[0]].artifact = arms[0]);
    test_checked_destroy(&second);
    nl_semantic_destroy(same_registry);
#undef REFUSE
    NLSemanticContext *world[3] = {parent,
                                   (NLSemanticContext *)arms[0]->context,
                                   (NLSemanticContext *)arms[1]->context};
    TestState *snapshots[3];
    for (size_t i = 0; i < 3; ++i) {
        snapshots[i] = malloc(sizeof(*snapshots[i]));
        CHECK(snapshots[i] && test_state(world[i], snapshots[i]));
    }
    fail_at = 0;
    allocation_index = 0;
    injecting = true;
    const bool readonly = valid(&n);
    injecting = false;
    CHECK(readonly && allocation_index == 0);
    for (size_t i = 0; i < 3; ++i) {
        CHECK(test_unchanged(world[i], snapshots[i]));
        free(snapshots[i]);
    }
    printf("P517_GATE rejects=%zu original=PASS worlds=UNCHANGED "
           "driver_allocations=0\n",
           refused);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && controls(argv[1]) ? 0 : 1;
}
