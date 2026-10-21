#include "fragment.h"
static bool observe(void)
{
    NLSemanticContext *c = NULL;
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    const char *text = "struct Other{left:u8,right:u8}fn other()->unit{unit}";
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK &&
          nl_source_create(text, strlen(text), "p671-other", &source) ==
              NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK);
    const NLSyntaxTree *inputs[] = {unit};
    CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    TestChecked creation = {0};
    CHECK(test_run(c, "let other_pair=Other{left:u8(7),right:u8(9)};",
                   TEST_SOURCE, NL_CHECK_OK, NULL, &creation));
    test_checked_destroy(&creation);
    /* Distinct source-registered nominal identities must not collapse merely
     * because their field layouts match. Frozen separately before execution. */
    source = NULL;
    parser = NULL;
    unit = NULL;
    const char *second =
        "struct Pair{left:u8,right:u8}fn pair_unused()->unit{unit}";
    CHECK(nl_source_create(second, strlen(second), "p671-pair-type", &source) ==
              NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK);
    const NLSyntaxTree *second_inputs[] = {unit};
    CHECK(nl_semantic_register_function_unit(c, second_inputs, 1, NULL) ==
          NL_CHECK_OK);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    CHECK(test_rejected(c, "let Pair{left,right}=other_pair;", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P5-DESTRUCTURE-TYPE"));
    CHECK(p671_other_read(c, "other_pair", "other_pair@left", 7, 9));
    NLSemanticBindingView b;
    NLSemanticPlaceView root, first, sibling;
    CHECK(nl_semantic_binding_view(c, nl_semantic_find_binding(c, "other_pair"),
                                   &b) &&
          nl_semantic_place_view(c, b.place, &root) &&
          nl_semantic_place_view(c, root.fixed_fields[0], &first) &&
          nl_semantic_place_view(c, root.fixed_fields[1], &sibling));
    printf("{\"source\":\"registered Other + fragment\",\"nominal\":%zu,"
           "\"root\":%zu,\"incarnation\":%zu,\"children\":[%zu,%zu],"
           "\"facts\":[%zu,%zu,%zu],\"current\":true,\"values\":[7,9],"
           "\"cross_nominal\":\"P5-DESTRUCTURE-TYPE rollback PASS\"}\n",
           b.type, b.place, root.incarnation, root.fixed_fields[0],
           root.fixed_fields[1], root.current_fact, first.current_fact,
           sibling.current_fact);
    nl_semantic_destroy(c);
    return true;
}
int main(void)
{
    return observe() ? 0 : 1;
}
