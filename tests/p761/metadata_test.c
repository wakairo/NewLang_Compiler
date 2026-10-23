#include "../../src/semantic_internal.h"
#include "../support/semantic_check.h"
static bool metadata(const char *path)
{
    NLSource *s = NULL;
    NLParser *p = NULL;
    NLSyntaxTree *tree = NULL;
    NLSemanticContext *c = NULL;
    CHECK(nl_source_load(path, &s) == NL_SOURCE_OK &&
          nl_parser_create(s, &p) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(p, &tree, NULL) == NL_PARSE_OK &&
          nl_semantic_create(&c) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {tree};
    CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
          NL_CHECK_OK);
    NLSemanticSnapshot snapshot;
    CHECK(nl_semantic_snapshot(c, &snapshot));
    CHECK(snapshot.values == 0 && snapshot.places == 0 &&
          snapshot.domains == 0 && snapshot.backing_regions == 0);
    size_t originals = 0;
    for (size_t id = 1; id <= snapshot.types; ++id) {
        CHECK(c->types[id - 1].one_backing_target == 0 &&
              c->types[id - 1].allocated_target == 0);
        if (nl_semantic_recursive_local_type(c, id)) {
            NLSemanticTypeView h;
            CHECK(nl_semantic_type_view(c, id, &h) && h.layout_known &&
                  h.size == 24 && h.alignment == 8);
            ++originals;
        }
    }
    CHECK(originals == 1);
    puts("REGISTRATION: stable existing H target metadata; "
         "values/places/domains/backing/OneBacking=0; no allocation authority");
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(p);
    nl_source_destroy(s);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && metadata(argv[1]) ? 0 : 1;
}
