/* Actual source and public observations only; no evidence mutation. */
#include "../support/semantic_check.h"

static bool observe(void)
{
    NLSemanticContext *c = NULL;
    NLSource *s = NULL;
    NLParser *p = NULL;
    NLSyntaxTree *tree = NULL;
    const char *text = "struct Other{left:u8,right:u8}fn other()->unit{unit}";
    CHECK(nl_source_create(text, strlen(text), "legacy-other-unit", &s) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(s, &p) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(p, &tree, NULL) == NL_PARSE_OK);
    const NLSyntaxTree *inputs[] = {tree};
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(p);
    nl_source_destroy(s);
    TestChecked init = {0};
    CHECK(test_run(c, "let other_pair=Other{left:u8(7),right:u8(9)};",
                   TEST_SOURCE, NL_CHECK_OK, NULL, &init));
    test_checked_destroy(&init);
    NLSemanticBindingView binding;
    NLSemanticTypeView type;
    NLSemanticPlaceView root;
    NLAggregateField fields[2];
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    CHECK(nl_semantic_binding_view(c, nl_semantic_find_binding(c, "other_pair"),
                                   &binding) &&
          nl_semantic_type_view(c, binding.type, &type) &&
          nl_semantic_place_view(c, binding.place, &root) &&
          nl_semantic_aggregate_field_view(c, binding.type, 0, &fields[0]) &&
          nl_semantic_aggregate_field_view(c, binding.type, 1, &fields[1]));
    const bool shape = type.kind == NL_TYPE_NOMINAL && type.field_count == 2 &&
                       type.is_copy && type.is_discardable &&
                       fields[0].type == u8 && fields[1].type == u8;
    CHECK(shape && root.live && root.independent_root &&
          binding.availability == NL_AVAILABLE && root.fixed_field_count == 0);
    NLSemanticSnapshot before = {0}, after = {0};
    CHECK(nl_semantic_snapshot(c, &before));
    TestChecked read = {0};
    CHECK(test_run(c, "other_pair@left", TEST_SOURCE,
                   NL_CHECK_SEMANTIC_UNSUPPORTED, "FIELD-PROFILE", &read));
    CHECK(nl_semantic_snapshot(c, &after) &&
          memcmp(&before, &after, sizeof(before)) == 0 &&
          read.artifact == NULL);
    printf("{\"source_kind\":\"registered Other + source fragment\","
           "\"shape_two_u8_copy_discardable\":true,\"type\":%zu,"
           "\"original_root\":%zu,\"incarnation\":%zu,\"available\":true,"
           "\"fixed_children\":0,\"implicit_local\":%s,"
           "\"field_labels\":[\"%s\",\"%s\"],\"read_status\":2,"
           "\"diagnostic\":\"FIELD-PROFILE\","
           "\"public_snapshot_unchanged\":true,\"read_artifact\":null}\n",
           binding.type, binding.place, root.incarnation,
           root.implicit_local ? "true" : "false", fields[0].name,
           fields[1].name);
    test_checked_destroy(&read);
    nl_semantic_destroy(c);
    return true;
}
int main(void)
{
    return observe() ? 0 : 1;
}
