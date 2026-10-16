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
    CHECK(load_controls(file, &n, &same_registry));
    CHECK(valid(&n));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    size_t id = 0;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_IF)
            id = i + 1;
    CHECK(id != 0);
    NLCheckedNodeView *join = &f->nodes[id - 1],
                      *condition = &f->nodes[join->initializer - 1];
    NLCheckedFragment *arms[2] = {
        (NLCheckedFragment *)nl_checked_if_arm(f, id, 0),
        (NLCheckedFragment *)nl_checked_if_arm(f, id, 1)};
    size_t arm_slot[2] = {SIZE_MAX, SIZE_MAX};
    for (size_t i = 0; i < f->arm_count; ++i)
        for (size_t a = 0; a < 2; ++a)
            if (f->arms[i].artifact == arms[a])
                arm_slot[a] = i;
    CHECK(arm_slot[0] != SIZE_MAX && arm_slot[1] != SIZE_MAX);
    const NLCheckedNodeView saved = *join, cond_saved = *condition;
    const NLSemanticValueView joined_value =
        f->context->values[join->results[0].value - 1];
    NLSemanticContext *parent = (NLSemanticContext *)f->context;
    observed_count = 0;
    world_bytes(n.context);
    world_bytes(parent);
    bytes(f, sizeof(*f));
    bytes(f->nodes, f->count * sizeof(*f->nodes));
    bytes(f->arms, f->arm_count * sizeof(*f->arms));
    for (size_t a = 0; a < 2; ++a) {
        world_bytes(arms[a]->context);
        bytes(arms[a], sizeof(*arms[a]));
        bytes(arms[a]->nodes, arms[a]->count * sizeof(*arms[a]->nodes));
    }
    size_t rejected = 0;
#define REFUSE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        const uint64_t before_bytes = fingerprint();                           \
        const bool accepted = valid(&n);                                       \
        const bool untouched = before_bytes == fingerprint();                  \
        restore;                                                               \
        CHECK(!accepted && untouched && valid(&n));                            \
        ++rejected;                                                            \
    } while (0)
    REFUSE(join->normal_arms = 1, *join = saved);
    REFUSE(join->item_count = 1, *join = saved);
    REFUSE(join->kind = NL_CHECKED_BLOCK, *join = saved);
    REFUSE(join->initializer = 0, *join = saved);
    REFUSE(join->initializer = 1, *join = saved);
    REFUSE(join->span.start_byte += 1, *join = saved);
    REFUSE(join->span.end_byte -= 1, *join = saved);
    REFUSE(join->result_count = 0, *join = saved);
    REFUSE(join->results[0].value = condition->results[0].value, *join = saved);
    REFUSE(join->results[0].type = 1, *join = saved);
    REFUSE(join->has_scalar_result = true, *join = saved);
    REFUSE(condition->symbol = 1, *condition = cond_saved);
    REFUSE(condition->span.start_byte += 1, *condition = cond_saved);
    REFUSE(condition->results[0].value = 1, *condition = cond_saved);
    REFUSE(parent->values[join->results[0].value - 1].scalar_known = true,
           parent->values[join->results[0].value - 1] = joined_value);
    REFUSE(parent->values[join->results[0].value - 1].scalar_value = 7,
           parent->values[join->results[0].value - 1] = joined_value);
    REFUSE(parent->values[join->results[0].value - 1].owner_place = 1,
           parent->values[join->results[0].value - 1] = joined_value);
    REFUSE(f->arms[arm_slot[0]].artifact = arms[1],
           f->arms[arm_slot[0]].artifact = arms[0]);
    REFUSE(f->arms[arm_slot[0]].match = id + 1,
           f->arms[arm_slot[0]].match = id);
    {
        NLCheckedFragment *a0 = f->arms[arm_slot[0]].artifact,
                          *a1 = f->arms[arm_slot[1]].artifact;
        REFUSE((f->arms[arm_slot[0]].artifact = a1,
                f->arms[arm_slot[1]].artifact = a0),
               (f->arms[arm_slot[0]].artifact = a0,
                f->arms[arm_slot[1]].artifact = a1));
    }
    for (size_t a = 0; a < 2; ++a) {
        NLCheckedFragment *arm = arms[a];
        NLSemanticContext *w = (NLSemanticContext *)arm->context;
        NLCheckedNodeView *ar = &arm->nodes[arm->root - 1],
                          *block = &arm->nodes[ar->initializer - 1],
                          *tail = &arm->nodes[block->tail - 1];
        const NLCheckedNodeView as = *ar, bs = *block, ts = *tail;
        NLSemanticValueView *value = &w->values[tail->results[0].value - 1];
        const NLSemanticValueView vs = *value;
        const NLCheckedFragment *owner = arm->if_parent;
        const NLCheckedNodeId owner_node = arm->if_parent_node;
        const size_t owner_index = arm->if_arm_index;
        REFUSE(arm->if_parent = arms[1 - a], arm->if_parent = owner);
        REFUSE(arm->if_parent_node = owner_node + 1,
               arm->if_parent_node = owner_node);
        REFUSE(arm->if_arm_index = 1 - a, arm->if_arm_index = owner_index);
        const size_t prefix = arm->branch_value_prefix,
                     facts = arm->branch_fact_prefix;
        REFUSE(ar->terminates = true, *ar = as);
        REFUSE(ar->results[0].value = 1, *ar = as);
        REFUSE(ar->results[0].type = 1, *ar = as);
        REFUSE(ar->initializer = arm->root, *ar = as);
        REFUSE(block->tail = arm->root, *block = bs);
        REFUSE(block->item_count = 1, *block = bs);
        REFUSE(block->results[0].value = 1, *block = bs);
        REFUSE(tail->results[0].value = 1, *tail = ts);
        REFUSE(tail->type = 1, *tail = ts);
        REFUSE(tail->argument_count = 1, *tail = ts);
        REFUSE(tail->span.end_byte += 1, *tail = ts);
        REFUSE(value->scalar_value = 99, *value = vs);
        REFUSE(value->scalar_known = false, *value = vs);
        REFUSE(value->reference.place = 1, *value = vs);
        REFUSE(value->reference.provenance = NL_PROVENANCE_VALID, *value = vs);
        REFUSE(value->value_dependency_count = 1, *value = vs);
        REFUSE(value->allocation_region = 1, *value = vs);
        REFUSE(arm->branch_value_prefix = prefix + 1,
               arm->branch_value_prefix = prefix);
        REFUSE(arm->branch_fact_prefix = facts + 1,
               arm->branch_fact_prefix = facts);
        const NLValueFactId oldfact = w->places[0].current_fact;
        REFUSE(w->places[0].current_fact = oldfact + 1,
               w->places[0].current_fact = oldfact);
        const size_t scopes = w->scope_count;
        REFUSE(w->scope_count = 1, w->scope_count = scopes);
        const NLSource *source = arm->source;
        REFUSE(arm->source = n.entry_source, arm->source = source);
        if (tail->kind == NL_CHECKED_U8_LITERAL) {
            REFUSE(tail->scalar_result.value = 99, *tail = ts);
            REFUSE((tail->scalar_result.value = 99, value->scalar_value = 99),
                   (*tail = ts, *value = vs));
            REFUSE(tail->has_scalar_result = false, *tail = ts);
        } else {
            REFUSE(tail->symbol = 1, *tail = ts);
            REFUSE(tail->value_use = NL_VALUE_CONSUMED, *tail = ts);
        }
    }
    /* Original-root proof corruption: only test-owned actual issued evidence.
     */
    NLCheckedNodeView *write = NULL, *loan = NULL, *left = NULL, *right = NULL;
    for (size_t i = 0; i < f->count; ++i) {
        NLCheckedNodeView *v = &f->nodes[i];
        if (v->kind == NL_CHECKED_REPLACE)
            write = v;
        if (v->kind == NL_CHECKED_LOAN_HEADER)
            loan = v;
        if (v->kind == NL_CHECKED_FIELD_READ) {
            if (v->field.index == 0)
                left = v;
            else
                right = v;
        }
    }
    CHECK(write && loan && left && right);
    const NLCheckedNodeView ws = *write, ls = *loan, lhs = *left, rhs = *right;
    REFUSE(write->field.base = 2, *write = ws);
    REFUSE(write->field.parent = 2, *write = ws);
    REFUSE(write->field.child = 3, *write = ws);
    REFUSE(write->field.index = 1, *write = ws);
    REFUSE(write->field.parent_incarnation += 1, *write = ws);
    REFUSE(write->field.child_incarnation += 1, *write = ws);
    REFUSE(write->field.parent_fact += 1, *write = ws);
    REFUSE(write->field.child_fact += 1, *write = ws);
    REFUSE(write->field.parent_post_fact = ws.field.parent_fact, *write = ws);
    REFUSE(write->field.child_post_fact = ws.field.child_fact, *write = ws);
    REFUSE(write->field.old_value = ws.field.new_value, *write = ws);
    REFUSE(write->field.new_value = ws.field.old_value, *write = ws);
    REFUSE(write->field.access = NL_ACCESS_READ, *write = ws);
    REFUSE(write->field.dependency_compatible = false, *write = ws);
    REFUSE(loan->loan.source = 2, *loan = ls);
    REFUSE(loan->loan.place = 3, *loan = ls);
    REFUSE(loan->loan.domain = 1, *loan = ls);
    REFUSE(loan->loan.scope = 2, *loan = ls);
    REFUSE(loan->loan.body_nonescape_proved = false, *loan = ls);
    REFUSE(loan->loan.normal_result_forwarded = false, *loan = ls);
    REFUSE(loan->field.parent_fact += 1, *loan = ls);
    REFUSE(left->field.parent_post_fact = 99, *left = lhs);
    REFUSE(left->field.parent_fact = ws.field.parent_fact, *left = lhs);
    REFUSE(left->field.child_fact = ws.field.child_fact, *left = lhs);
    REFUSE(left->field.old_value = ws.field.old_value, *left = lhs);
    REFUSE(right->field.child = 2, *right = rhs);
    REFUSE(right->field.child_fact += 1, *right = rhs);
    REFUSE(right->field.old_value = ws.field.new_value, *right = rhs);
    REFUSE(right->field.base = 2, *right = rhs);
    REFUSE((write->field.parent_post_fact += 1, left->field.parent_fact += 1,
            right->field.parent_fact += 1),
           (*write = ws, *left = lhs, *right = rhs));
    REFUSE((write->field.child_post_fact += 1, left->field.child_fact += 1),
           (*write = ws, *left = lhs));
    REFUSE(right->field.payload_occurrence = 1, *right = rhs);
    REFUSE(parent->scopes[0].active = true, parent->scopes[0].active = false);
    for (size_t a = 0; a < 2; ++a) {
        NLSemanticContext *w = (NLSemanticContext *)arms[a]->context;
        for (size_t j = 0; j < 3; ++j) {
            NLSemanticPlaceView original = w->places[j];
            REFUSE(w->places[j].incarnation += 1, w->places[j] = original);
            REFUSE(w->places[j].governing_domain = 1, w->places[j] = original);
            REFUSE(w->places[j].current_fact += 1, w->places[j] = original);
            REFUSE(w->places[j].current_value = 0, w->places[j] = original);
        }
    }
    NLCheckedNodeView *wr = &f->nodes[write->first_argument - 1];
    NLSemanticValueView *rp = &parent->values[wr->results[0].value - 1];
    const NLSemanticValueView saved_ref = *rp;
    REFUSE(rp->reference.place = 3, *rp = saved_ref);
    REFUSE(rp->reference.incarnation += 1, *rp = saved_ref);
    REFUSE(rp->reference.scope = 2, *rp = saved_ref);
    REFUSE(rp->reference.writable = false, *rp = saved_ref);
    REFUSE(rp->reference.provenance = NL_PROVENANCE_UNKNOWN, *rp = saved_ref);
    REFUSE(rp->value_dependency_count = 1, *rp = saved_ref);
    REFUSE(rp->domain = 1, *rp = saved_ref);
    REFUSE(rp->allocation_region = 1, *rp = saved_ref);
    TestNode foreign = {0};
    CHECK(node_checked_load(file, &foreign));
    const NLCheckedFragment *other =
        nl_checked_call_body(foreign.entry, nl_checked_root(foreign.entry));
    NLCheckedFragment *foreign_arm =
        (NLCheckedFragment *)nl_checked_if_arm(other, id, 0);
    CHECK(foreign_arm != NULL);
    REFUSE(f->arms[arm_slot[0]].artifact = foreign_arm,
           f->arms[arm_slot[0]].artifact = arms[0]);
    node_checked_destroy(&foreign);
    TestChecked second_call = {0};
    CHECK(test_run(same_registry, "main()", TEST_EXPRESSION, NL_CHECK_OK, NULL,
                   &second_call));
    const NLCheckedFragment *same_plan = nl_checked_call_body(
        second_call.artifact, nl_checked_root(second_call.artifact));
    CHECK(same_plan != NULL &&
          nl_checked_source(same_plan) == nl_checked_source(f));
    NLCheckedFragment *same_source_arm =
        (NLCheckedFragment *)nl_checked_if_arm(same_plan, id, 0);
    CHECK(same_source_arm != NULL);
    REFUSE(f->arms[arm_slot[0]].artifact = same_source_arm,
           f->arms[arm_slot[0]].artifact = arms[0]);
    test_checked_destroy(&second_call);
    nl_semantic_destroy(same_registry);
    /* Wrong copied selected value: no source/package substitution accepted. */
    for (size_t i = id; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_IDENTIFIER &&
            f->nodes[i].type == saved.type) {
            NLCheckedNodeView *copy = &f->nodes[i];
            NLSemanticValueView *v =
                                    &parent->values[copy->results[0].value - 1],
                                savedv = *v;
            REFUSE(v->scalar_known = true, *v = savedv);
            break;
        }
#undef REFUSE
    /* Read-only gate: no new allocation even when every wrapped allocation
     * would fail. Observe complete public semantic state in every world. */
    TestState *states[4];
    NLSemanticContext *worlds[4] = {n.context, parent,
                                    (NLSemanticContext *)arms[0]->context,
                                    (NLSemanticContext *)arms[1]->context};
    for (size_t i = 0; i < 4; ++i) {
        states[i] = malloc(sizeof(*states[i]));
        CHECK(states[i] != NULL && test_state(worlds[i], states[i]));
    }
    fail_at = 0;
    allocation_index = 0;
    injecting = true;
    const bool readonly = valid(&n);
    injecting = false;
    CHECK(readonly && allocation_index == 0);
    for (size_t i = 0; i < 4; ++i) {
        CHECK(test_unchanged(worlds[i], states[i]));
        free(states[i]);
    }
    printf("P514_GATE rejects=%zu original=PASS worlds=UNCHANGED "
           "driver_allocations=0\n",
           rejected);
    node_checked_destroy(&n);
    return true;
}
static bool oom(const char *file)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(nl_source_load(file, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK);
    nl_parser_destroy(parser);
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState *before = malloc(sizeof(*before));
    CHECK(before != NULL && test_state(c, before));
    const NLSyntaxTree *units[] = {tree};
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        NLCheckStatus s = nl_semantic_register_function_unit(c, units, 1, NULL);
        injecting = false;
        if (s == NL_CHECK_OK)
            break;
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && test_unchanged(c, before));
    }
    CHECK(fail_at > 0 && fail_at < 8000);
    printf("P514_OOM registration=%zu rollback=PASS\n", fail_at);
    CHECK(test_state(c, before));
    parser = NULL;
    NLSource *entry = NULL;
    NLSyntaxTree *call = NULL;
    CHECK(nl_source_create("main()", 6, "entry", &entry) == NL_SOURCE_OK &&
          nl_parser_create(entry, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(parser, &call, NULL) ==
          NL_PARSE_OK);
    nl_parser_destroy(parser);
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        NLCheckedFragment *out = NULL;
        NLCheckStatus s = nl_semantic_check_expression(c, call, &out, NULL);
        injecting = false;
        if (s == NL_CHECK_OK) {
            nl_checked_destroy(out);
            break;
        }
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && out == NULL &&
              test_unchanged(c, before));
    }
    CHECK(fail_at > 0 && fail_at < 8000);
    printf("P514_OOM main=%zu rollback=PASS output=NULL\n", fail_at);
    free(before);
    nl_syntax_tree_destroy(call);
    nl_source_destroy(entry);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    nl_semantic_destroy(c);
    return true;
}
static bool rejection(const char *file)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLSemanticContext *c = NULL;
    CHECK(nl_source_load(file, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_semantic_create(&c) == NL_CHECK_OK);
    TestState *before = malloc(sizeof(*before));
    CHECK(before && test_state(c, before));
    NLParseDiagnostic parsed = {0};
    if (nl_parser_parse_function_unit(parser, &tree, &parsed) != NL_PARSE_OK) {
        CHECK(tree == NULL && test_unchanged(c, before));
        printf("P514_REJECT phase=parser code=%s state=UNCHANGED "
               "semantic=NOT_ENTERED artifact=NULL\n",
               parsed.diagnostic.code);
        free(before);
        nl_parser_destroy(parser);
        nl_source_destroy(source);
        nl_semantic_destroy(c);
        return true;
    }
    const NLSyntaxTree *inputs[] = {tree};
    NLFunctionUnitDiagnostic registration = {0};
    NLCheckStatus status =
        nl_semantic_register_function_unit(c, inputs, 1, &registration);
    NLCheckDiagnostic diagnostic = registration.diagnostic;
    const char *phase = "registration";
    if (status == NL_CHECK_OK) {
        CHECK(test_state(c, before));
        NLSource *entry = NULL;
        NLParser *call_parser = NULL;
        NLSyntaxTree *call = NULL;
        NLCheckedFragment *out = NULL;
        CHECK(nl_source_create("main()", 6, "entry", &entry) == NL_SOURCE_OK &&
              nl_parser_create(entry, &call_parser) == NL_PARSE_OK &&
              nl_parser_parse_expression_fragment(call_parser, &call, NULL) ==
                  NL_PARSE_OK);
        status = nl_semantic_check_expression(c, call, &out, &diagnostic);
        CHECK(out == NULL);
        phase = "actual_main";
        nl_syntax_tree_destroy(call);
        nl_parser_destroy(call_parser);
        nl_source_destroy(entry);
    }
    CHECK(status != NL_CHECK_OK && status != NL_CHECK_OUT_OF_MEMORY &&
          status != NL_CHECK_INTERNAL_ERROR && test_unchanged(c, before));
    printf("P514_REJECT phase=%s code=%s rollback=PASS artifact=NULL\n", phase,
           diagnostic.diagnostic.code);
    free(before);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    nl_semantic_destroy(c);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    return strcmp(argv[1], "poison") == 0   ? (controls(argv[2]) ? 0 : 1)
           : strcmp(argv[1], "oom") == 0    ? (oom(argv[2]) ? 0 : 1)
           : strcmp(argv[1], "reject") == 0 ? (rejection(argv[2]) ? 0 : 1)
                                            : 2;
}
