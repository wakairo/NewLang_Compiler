/* Genuine source-issued worlds only. No semantic seed or host-created owner.
 * Direct shared-validator tests are independent of IF backend admission. */
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
static uint64_t fingerprint(const NLSemanticContext *c)
{
    uint64_t h = UINT64_C(14695981039346656037);
    const unsigned char *arrays[] = {(const unsigned char *)c,
                                     (const unsigned char *)c->places,
                                     (const unsigned char *)c->values};
    const size_t lengths[] = {sizeof(*c), c->place_count * sizeof(*c->places),
                              c->value_count * sizeof(*c->values)};
    for (size_t a = 0; a < 3; ++a)
        for (size_t i = 0; i < lengths[a]; ++i)
            h = (h ^ arrays[a][i]) * UINT64_C(1099511628211);
    return h;
}
static bool controls(NLSemanticContext *c, const TestNode *n)
{
    CHECK(nl_fixed_validate(c) == NL_CHECK_OK &&
          nl_sem_validate(c) == NL_CHECK_OK);
    NLPlaceId root = 0;
    for (size_t i = 0; i < c->place_count; ++i)
        if (c->places[i].live && c->places[i].independent_root &&
            nl_fixed_type(c, c->places[i].type)) {
            CHECK(root == 0);
            root = i + 1;
        }
    CHECK(root != 0);
    NLSemanticPlaceView *r = &c->places[root - 1];
    CHECK(r->fixed_field_count == 2 && r->current_value != 0);
    NLSemanticPlaceView *child = &c->places[r->fixed_fields[0] - 1];
    NLSemanticValueView *aggregate = &c->values[r->current_value - 1];
    const NLSemanticPlaceView saved_root = *r, saved_child = *child;
    const NLSemanticValueView saved_aggregate = *aggregate;
    TestState *before = malloc(sizeof(*before));
    CHECK(before != NULL && test_state(c, before));
    size_t rejected = 0;
#define REFUSE(label, change)                                                  \
    do {                                                                       \
        change;                                                                \
        const uint64_t mutated = fingerprint(c);                               \
        const NLCheckStatus status = nl_fixed_validate(c);                     \
        const bool readonly = mutated == fingerprint(c);                       \
        *r = saved_root;                                                       \
        *child = saved_child;                                                  \
        *aggregate = saved_aggregate;                                          \
        if (status != NL_CHECK_INTERNAL_ERROR)                                 \
            fprintf(stderr, "%s: returned %d\n", label, (int)status);          \
        CHECK(status == NL_CHECK_INTERNAL_ERROR && readonly &&                 \
              test_unchanged(c, before) && nl_sem_validate(c) == NL_CHECK_OK); \
        if (n != NULL)                                                         \
            CHECK(driver(n));                                                  \
        ++rejected;                                                            \
    } while (0)
    REFUSE("root_value_zero", r->current_value = 0);
    REFUSE("root_value_past_end", r->current_value = c->value_count + 1);
    REFUSE("child_value_zero", child->current_value = 0);
    REFUSE("child_value_past_end", child->current_value = c->value_count + 1);
    REFUSE("parent_past_end", child->parent_aggregate = c->place_count + 1);
    REFUSE("field_past_declared",
           child->parent_field_index = r->fixed_field_count);
    REFUSE("field_past_array",
           child->parent_field_index = NL_SEMANTIC_MAX_FIELDS);
    REFUSE("field_hostile_count",
           (r->fixed_field_count = NL_SEMANTIC_MAX_FIELDS + 1,
            child->parent_field_index = NL_SEMANTIC_MAX_FIELDS));
    REFUSE("root_type_zero", r->type = 0);
    REFUSE("root_type_past_end", r->type = c->type_count + 1);
    REFUSE("child_type_zero", child->type = 0);
    REFUSE("child_type_past_end", child->type = c->type_count + 1);
    REFUSE("aggregate_type_zero", aggregate->type = 0);
    REFUSE("aggregate_type_past_end", aggregate->type = c->type_count + 1);
    REFUSE("aggregate_field_count",
           aggregate->field_count = NL_SEMANTIC_MAX_FIELDS + 1);
    REFUSE("member_value_zero", aggregate->fields[0] = 0);
    REFUSE("member_value_past_end", aggregate->fields[0] = c->value_count + 1);
#undef REFUSE
    if (n != NULL) {
        /* Zero parent is a legitimate no-parent sentinel, not an invalid
         * subscript. The original-child relation is still refused by the
         * private source consumer; no new universal place rule is added. */
        child->parent_aggregate = 0;
        const bool accepted = driver(n);
        *child = saved_child;
        CHECK(!accepted && test_unchanged(c, before) && driver(n));
        puts("zero_parent_sentinel=private_relation_REJECT");
    }
    printf("P636 source_world root=%zu direct_rejects=%zu "
           "status=NL_CHECK_INTERNAL_ERROR "
           "original=PASS restored=UNCHANGED readonly=PASS\n",
           root, rejected);
    free(before);
    return true;
}
static bool registered(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n) && driver(&n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    NLCheckedNodeId id = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_node_view(f, i)->kind == NL_CHECKED_IF)
            id = i;
    CHECK(id != 0);
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, id, a);
        CHECK(arm != NULL &&
              controls((NLSemanticContext *)nl_checked_context(arm), &n));
    }
    CHECK(driver(&n));
    node_checked_destroy(&n);
    return true;
}
static bool plain(void)
{
    static const char declarations[] =
        "struct Pair{left:u8,right:u8}fn unused()->unit{unit}";
    static const char creation[] = "let p=Pair{left:u8(31),right:u8(117)};";
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    NLSemanticContext *c = NULL;
    CHECK(nl_source_create(declarations, sizeof(declarations) - 1,
                           "p636-registry", &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK &&
          nl_semantic_create(&c) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {unit};
    CHECK(nl_semantic_register_function_unit(c, units, 1, NULL) == NL_CHECK_OK);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    TestChecked checked = {0};
    CHECK(test_run(c, creation, TEST_SOURCE, NL_CHECK_OK, NULL, &checked) &&
          controls(c, NULL));
    test_checked_destroy(&checked);
    nl_semantic_destroy(c);
    return true;
}
int main(int argc, char **argv)
{
    return (argc == 1 ? plain() : argc == 2 && registered(argv[1])) ? 0 : 1;
}
