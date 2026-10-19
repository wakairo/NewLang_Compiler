/* Test-only read-only-consumer probe. Replays genuine registered source after
 * AST teardown, then redirects one untrusted shallow arm copy to another
 * genuine arm's context. No semantic package or permission is constructed. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"
static uint64_t hash_bytes(uint64_t h, const void *data, size_t length)
{
    const unsigned char *bytes = data;
    for (size_t i = 0; i < length; ++i)
        h = (h ^ bytes[i]) * UINT64_C(1099511628211);
    return h;
}
static uint64_t hash_fragment(uint64_t h, const NLCheckedFragment *f)
{
    const NLSemanticContext *w = nl_checked_context(f);
    h = hash_bytes(h, f, sizeof(*f));
    h = hash_bytes(h, f->nodes, f->count * sizeof(*f->nodes));
    h = hash_bytes(h, w, sizeof(*w));
    h = hash_bytes(h, w->values, w->value_count * sizeof(*w->values));
    h = hash_bytes(h, w->places, w->place_count * sizeof(*w->places));
    h = hash_bytes(h, w->bindings, w->binding_count * sizeof(*w->bindings));
    return hash_bytes(h, w->scopes, w->scope_count * sizeof(*w->scopes));
}
static bool validates(const NLCheckedFragment *entry)
{
    V0Program p = {
        .unit = nl_semantic_unit_type(nl_checked_context(entry)),
        .u8 = nl_semantic_core_type(nl_checked_context(entry), NL_TYPE_U8)};
    return v0_validate_call(entry, nl_checked_root(entry), &p);
}
static bool load_replay(const char *path, TestNode *original,
                        TestChecked *replay, NLSemanticContext **registry)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK &&
          nl_semantic_create(&original->context) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {unit};
    CHECK(nl_semantic_register_function_unit(original->context, inputs, 1,
                                             NULL) == NL_CHECK_OK);
    NLSemanticSnapshot empty;
    CHECK(nl_semantic_snapshot(original->context, &empty) &&
          empty.values == 0 && empty.bindings == 0 && empty.places == 0 &&
          empty.scopes == 0 && empty.domains == 0 &&
          empty.backing_regions == 0);
    CHECK(nl_sem_clone(original->context, registry) == NL_CHECK_OK);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    TestChecked first = {0};
    CHECK(test_run(original->context, "main()", TEST_EXPRESSION, NL_CHECK_OK,
                   NULL, &first));
    original->entry_source = first.source;
    original->entry = first.artifact;
    CHECK(test_run(*registry, "main()", TEST_EXPRESSION, NL_CHECK_OK, NULL,
                   replay));
    return true;
}
static bool probe(const char *path)
{
    TestNode original = {0};
    TestChecked replay = {0};
    NLSemanticContext *registry = NULL;
    CHECK(load_replay(path, &original, &replay, &registry));
    CHECK(validates(original.entry));
    CHECK(validates(replay.artifact));
    const NLCheckedFragment *first =
        nl_checked_call_body(original.entry, nl_checked_root(original.entry));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        replay.artifact, nl_checked_root(replay.artifact));
    CHECK(first != NULL && f != NULL && first != f &&
          nl_checked_source(first) == nl_checked_source(f));
    NLCheckedNodeId id = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_node_view(f, i)->kind == NL_CHECKED_IF) {
            CHECK(id == 0);
            id = i;
        }
    CHECK(id != 0);
    const NLCheckedFragment *arms[2] = {nl_checked_if_arm(f, id, 0),
                                        nl_checked_if_arm(f, id, 1)};
    CHECK(arms[0] && arms[1] && arms[0] != arms[1] &&
          arms[0]->context != arms[1]->context);
    const NLCheckedNodeView *local[2], *tail[2];
    NLSemanticValueView values[2];
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedNodeView *r =
            nl_checked_node_view(arms[a], nl_checked_root(arms[a]));
        const NLCheckedNodeView *b =
            nl_checked_node_view(arms[a], r->initializer);
        CHECK(b && b->item_count == 1);
        local[a] = nl_checked_node_view(arms[a], b->first_item);
        tail[a] = nl_checked_node_view(arms[a], b->tail);
        CHECK(local[a] && tail[a] &&
              nl_semantic_value_view(arms[a]->context,
                                     tail[a]->results[0].value, &values[a]));
    }
    CHECK(local[0]->symbol == local[1]->symbol &&
          tail[0]->results[0].value == tail[1]->results[0].value);
    NLSourceView name[2];
    CHECK(nl_source_view(nl_checked_source(f), local[0]->name, &name[0]) &&
          nl_source_view(nl_checked_source(f), local[1]->name, &name[1]) &&
          name[0].length == name[1].length && name[0].length != 0 &&
          memcmp(name[0].bytes, name[1].bytes, name[0].length) == 0);
    const NLCheckedFragment *protected[] = {original.entry,
                                            first,
                                            nl_checked_if_arm(first, id, 0),
                                            nl_checked_if_arm(first, id, 1),
                                            replay.artifact,
                                            f,
                                            arms[0],
                                            arms[1]};
    uint64_t before = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < sizeof(protected) / sizeof(protected[0]); ++i)
        before = hash_fragment(before, protected[i]);
    size_t slots[2] = {SIZE_MAX, SIZE_MAX};
    for (size_t i = 0; i < f->arm_count; ++i)
        for (size_t a = 0; a < 2; ++a)
            if (f->arms[i].artifact == arms[a])
                slots[a] = i;
    CHECK(slots[0] != SIZE_MAX && slots[1] != SIZE_MAX);
    NLCheckedFragment untrusted[2] = {*arms[0], *arms[1]};
    untrusted[0].context = arms[1]->context;
    f->arms[slots[0]].artifact = &untrusted[0];
    const bool accepted = validates(replay.artifact);
    f->arms[slots[0]].artifact = (NLCheckedFragment *)arms[0];
    untrusted[1].context = arms[0]->context;
    f->arms[slots[0]].artifact = &untrusted[0];
    f->arms[slots[1]].artifact = &untrusted[1];
    const bool paired_swap = validates(replay.artifact);
    f->arms[slots[0]].artifact = (NLCheckedFragment *)arms[0];
    f->arms[slots[1]].artifact = (NLCheckedFragment *)arms[1];
    uint64_t after = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < sizeof(protected) / sizeof(protected[0]); ++i)
        after = hash_fragment(after, protected[i]);
    CHECK(before == after && validates(original.entry) &&
          validates(replay.artifact));
    printf("{\"branch_value_prefix\":[%zu,%zu],\"branch_fact_prefix\":[%zu,%zu]"
           ",\"local_symbols\":[%zu,%zu],\"tail_ids\":[%zu,%zu],\"arm_values\":"
           "[%zu,%zu],",
           arms[0]->branch_value_prefix, arms[1]->branch_value_prefix,
           arms[0]->branch_fact_prefix, arms[1]->branch_fact_prefix,
           local[0]->symbol, local[1]->symbol, tail[0]->results[0].value,
           tail[1]->results[0].value, values[0].scalar_value,
           values[1].scalar_value);
    printf("\"cross_arm_context_accepted\":%s,\"paired_context_swap_"
           "accepted\":%s,\"original_valid\":true,"
           "\"replay_valid\":true,\"source_plan_shared\":true,\"worlds_"
           "distinct\":true,\"protected_worlds_unchanged\":true}\n",
           accepted ? "true" : "false", paired_swap ? "true" : "false");
    test_checked_destroy(&replay);
    nl_semantic_destroy(registry);
    node_checked_destroy(&original);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && probe(argv[1]) ? 0 : 1;
}
