#include "../../src/semantic_internal.h"
#include "../support/semantic_check.h"
#include <stdlib.h>

static const char decl[] = "struct Node{next:Option<ptr<Node>>,payload:u8,}";
static const char body[] =
    "fn main()->unit{let n=Node{next:Option<ptr<Node>>.None,payload:u8(7)};"
    "let Node{next,payload}=n;next;payload;unit}";
static const char witness[] =
    "struct Node{next:Option<ptr<Node>>,payload:u8,}"
    "fn main()->unit{let n=Node{next:Option<ptr<Node>>.None,payload:u8(7)};"
    "let Node{next,payload}=n;next;payload;unit}";
void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
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
static bool parse(const char *text, NLSource **source, NLSyntaxTree **tree)
{
    CHECK(nl_source_create(text, strlen(text), "recursive", source) ==
          NL_SOURCE_OK);
    NLParser *p = NULL;
    CHECK(nl_parser_create(*source, &p) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(p, tree, NULL) == NL_PARSE_OK);
    nl_parser_destroy(p);
    return true;
}
static bool register_text(NLSemanticContext *c, const char *text,
                          NLCheckStatus expected, const char *code)
{
    NLSource *s = NULL;
    NLSyntaxTree *t = NULL;
    CHECK(parse(text, &s, &t));
    const NLSyntaxTree *inputs[] = {t};
    NLFunctionUnitDiagnostic d = {0};
    const NLCheckStatus status =
        nl_semantic_register_function_unit(c, inputs, 1, &d);
    if (status != expected)
        fprintf(stderr, "status %d expected %d: %s\n", status, expected,
                d.diagnostic.diagnostic.code);
    CHECK(status == expected);
    if (code != NULL)
        CHECK(strcmp(code, d.diagnostic.diagnostic.code) == 0);
    nl_syntax_tree_destroy(t);
    nl_source_destroy(s);
    return true;
}
static NLTypeId named(const NLSemanticContext *c, const char *name)
{
    for (size_t i = 0; i < c->type_count; ++i)
        if (c->types[i].name != NULL && strcmp(c->types[i].name, name) == 0)
            return i + 1;
    return 0;
}
static bool run(NLSemanticContext *c, const char *text)
{
    TestChecked t = {0};
    CHECK(test_run(c, text, TEST_SOURCE, NL_CHECK_OK, NULL, &t));
    test_checked_destroy(&t);
    return true;
}
static bool parser_tests(void)
{
    NLSource *s = NULL;
    NLSyntaxTree *t = NULL;
    CHECK(parse(decl, &s, &t));
    const NLSyntaxView *root = nl_syntax_node_view(nl_syntax_tree_root(t));
    const NLSyntaxView *d =
        nl_syntax_node_view(root->data.function_unit.declarations);
    CHECK(root->data.function_unit.count == 1 &&
          d->kind == NL_SYNTAX_RECURSIVE_STRUCT);
    CHECK(
        nl_syntax_node_view(
            nl_syntax_node_view(d->data.avs_struct.fields)->data.parameter.type)
            ->kind == NL_SYNTAX_OPTION_PTR);
    nl_syntax_tree_destroy(t);
    nl_source_destroy(s);
    const char *outside[] = {"struct N{a:Foo<ptr<N>>,b:u8}",
                             "struct N{a:Option<N>,b:u8}",
                             "struct N{a:Option<ref<read,N>>,b:u8}",
                             "struct N{a:Option<ptr<ptr<N>>>,b:u8}",
                             "struct N{a:u8,b:Option<ptr<N>>}",
                             "struct N{a:Option<ptr<N>>,b:u8,c:u8}",
                             "struct N{a:Option<ptr<N>>,b:bool}",
                             "struct N;"};
    for (size_t i = 0; i < sizeof(outside) / sizeof(outside[0]); ++i) {
        NLParser *p = NULL;
        s = NULL;
        t = NULL;
        CHECK(nl_source_create(outside[i], strlen(outside[i]), "outside", &s) ==
              NL_SOURCE_OK);
        CHECK(nl_parser_create(s, &p) == NL_PARSE_OK);
        CHECK(nl_parser_parse_function_unit(p, &t, NULL) != NL_PARSE_OK &&
              t == NULL);
        nl_parser_destroy(p);
        nl_source_destroy(s);
    }
    return true;
}
static bool identity(void)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    NLTypeId h, p, o, again;
    CHECK(nl_recursive_header(c, "Cell", &h) == NL_CHECK_OK);
    bool complete = true;
    NLSemanticTypeView v;
    CHECK(nl_semantic_type_completion(c, h, &complete) && !complete);
    CHECK(!nl_semantic_type_view(c, h, &v));
    CHECK(nl_sem_compound(c, NL_TYPE_REF, h, NL_ACCESS_READ, false, &p) ==
          NL_CHECK_SEMANTIC_UNSUPPORTED);
    CHECK(nl_sem_compound(c, NL_TYPE_SLOT, h, NL_ACCESS_READ, false, &p) ==
          NL_CHECK_SEMANTIC_UNSUPPORTED);
    CHECK(nl_semantic_compound_type(c, NL_TYPE_PTR, h, NL_ACCESS_READ, false,
                                    &p) == NL_CHECK_OK);
    CHECK(nl_sem_compound(c, NL_TYPE_REF, p, NL_ACCESS_READ, false, &again) ==
          NL_CHECK_SEMANTIC_UNSUPPORTED);
    CHECK(nl_sem_compound(c, NL_TYPE_PTR, p, NL_ACCESS_READ, false, &again) ==
          NL_CHECK_SEMANTIC_UNSUPPORTED);
    CHECK(nl_semantic_type_completion(c, h, &complete) &&
          !complete); /* clone retains header */
    CHECK(nl_semantic_type_view(c, p, &v) && v.is_copy && v.is_discardable &&
          v.target == h);
    CHECK(nl_recursive_option(c, p, &o) == NL_CHECK_OK);
    CHECK(nl_recursive_option(c, p, &again) == NL_CHECK_OK && again == o);
    CHECK(nl_sem_compound(c, NL_TYPE_PTR, o, NL_ACCESS_READ, false, &again) ==
          NL_CHECK_SEMANTIC_UNSUPPORTED);
    CHECK(nl_semantic_type_view(c, o, &v) && v.kind == NL_TYPE_SUM &&
          v.variant_count == 2 && v.is_copy && v.is_discardable);
    CHECK(c->value_count == 0 && c->place_count == 0 &&
          c->occurrence_count == 0 && c->domain_count == 0 &&
          c->last_incarnation == 0);
    NLValueId value = 0;
    NLPlaceId place = 0;
    CHECK(nl_sem_new_value(c, (NLSemanticValueView){.type = h}, &value) ==
          NL_CHECK_SEMANTIC_ERROR);
    CHECK(nl_sem_new_value(c, (NLSemanticValueView){.type = p}, &value) ==
          NL_CHECK_SEMANTIC_ERROR);
    CHECK(nl_sem_new_value(c, (NLSemanticValueView){.type = o, .variant = 1},
                           &value) == NL_CHECK_SEMANTIC_ERROR);
    CHECK(nl_sem_new_place(c, h, 0, true, 0, &place) ==
          NL_CHECK_SEMANTIC_ERROR);
    CHECK(nl_semantic_set_layout(c, h, 1, 1) == NL_CHECK_SEMANTIC_ERROR);
    CHECK(nl_semantic_register_function(c, "bad", &h, 1, 1, false, false) ==
          NL_CHECK_SEMANTIC_ERROR);
    CHECK(nl_recursive_validate(c) == NL_CHECK_SEMANTIC_ERROR);
    const size_t type_count = c->type_count;
    const size_t function_count = c->function_count;
    CHECK(register_text(c, "fn unrelated()->unit{unit}",
                        NL_CHECK_SEMANTIC_ERROR, NULL));
    CHECK(c->type_count == type_count && c->function_count == function_count &&
          c->types[h - 1].incomplete);
    const NLAggregateField fields[] = {
        {"tail", o}, {"data", nl_semantic_core_type(c, NL_TYPE_U8)}};
    CHECK(nl_recursive_complete(c, h, fields, 2) == NL_CHECK_OK);
    CHECK(named(c, "Cell") == h &&
          nl_semantic_type_completion(c, h, &complete) && complete);
    CHECK(nl_semantic_type_view(c, h, &v) && v.field_count == 2 && v.is_copy &&
          v.is_discardable && !v.layout_known);
    TestState before;
    CHECK(test_state(c, &before));
    CHECK(nl_recursive_complete(c, h, fields, 2) == NL_CHECK_SEMANTIC_ERROR);
    NLAggregateField changed[] = {
        {"different", o}, {"data", nl_semantic_core_type(c, NL_TYPE_U8)}};
    CHECK(nl_recursive_complete(c, h, changed, 2) == NL_CHECK_SEMANTIC_ERROR);
    CHECK(test_unchanged(c, &before));
    CHECK(nl_recursive_validate(c) == NL_CHECK_OK);
    nl_semantic_destroy(c);
    return true;
}
static bool source(void)
{
    const char *variants[] = {
        witness,
        "fn main()->unit{let "
        "n=Node{next:Option<ptr<Node>>.None,payload:u8(7)};unit}struct "
        "Node{next:Option<ptr<Node>>,payload:u8}",
        "struct Cell{tail:Option<ptr<Cell>>,data:u8}fn main()->unit{let "
        "c=Cell{tail:Option<ptr<Cell>>.None,data:u8(7)};unit}"};
    for (size_t k = 0; k < 3; ++k) {
        NLSemanticContext *c = NULL;
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
        CHECK(register_text(c, variants[k], NL_CHECK_OK, NULL));
        NLTypeId h = named(c, k == 2 ? "Cell" : "Node");
        NLSemanticTypeView v;
        CHECK(h != 0 && nl_semantic_type_view(c, h, &v) && v.is_copy &&
              v.is_discardable && v.field_count == 2);
        CHECK(c->value_count == 0 &&
              c->place_count ==
                  0); /* registration does not publish validation fixtures */
        CHECK(run(c, "main()"));
        CHECK(nl_sem_validate(c) == NL_CHECK_OK);
        nl_semantic_destroy(c);
    }
    /* Physical input permutation uses the same header and concrete type IDs. */
    NLSource *sources[2] = {NULL, NULL};
    NLSyntaxTree *trees[2] = {NULL, NULL};
    CHECK(parse(decl, &sources[0], &trees[0]) &&
          parse(body, &sources[1], &trees[1]));
    NLTypeId ids[2] = {0, 0};
    for (size_t k = 0; k < 2; ++k) {
        NLSemanticContext *c = NULL;
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
        const NLSyntaxTree *inputs[] = {trees[k], trees[1 - k]};
        CHECK(nl_semantic_register_function_unit(c, inputs, 2, NULL) ==
              NL_CHECK_OK);
        const NLTypeId h = named(c, "Node");
        ids[k] = h;
        const NLTypeId option = c->types[h - 1].field_types[0];
        const NLTypeId ptr = c->types[option - 1].variant_types[1];
        CHECK(c->types[option - 1].option_target == ptr &&
              c->types[ptr - 1].view.target == h);
        CHECK(run(c, "main()"));
        nl_semantic_destroy(c);
    }
    CHECK(ids[0] == ids[1]);
    for (size_t k = 0; k < 2; ++k) {
        nl_syntax_tree_destroy(trees[k]);
        nl_source_destroy(sources[k]);
    }
    /* Existing Option Some/copy/occurrence ownership, with a trusted ptr
     * control. */
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(register_text(c, decl, NL_CHECK_OK, NULL));
    const size_t types_before = c->type_count;
    TestChecked type_evidence = {0};
    CHECK(test_run(c, "Option<ptr<Node>>", TEST_TYPE, NL_CHECK_OK, NULL,
                   &type_evidence));
    CHECK(test_root(&type_evidence)->type ==
          c->types[named(c, "Node") - 1].field_types[0]);
    CHECK(c->type_count == types_before);
    test_checked_destroy(&type_evidence);
    CHECK(run(c, "let n=Node{next:Option<ptr<Node>>.None,payload:u8(7)};"));
    const NLPlaceId place =
        c->bindings[nl_semantic_find_binding(c, "n") - 1].view.place;
    NLSymbolId token;
    CHECK(test_reference(c, "token", place, NL_TYPE_PTR, NL_ACCESS_READ, false,
                         &token, NULL));
    CHECK(run(c, "let link=Option<ptr<Node>>.Some(token);"));
    CHECK(c->occurrence_count == 1);
    CHECK(run(
        c,
        "let linked=Node{next:Option<ptr<Node>>.Some(token),payload:u8(9)};"));
    CHECK(run(c, "let copied=linked;"));
    CHECK(nl_sem_validate(c) == NL_CHECK_OK);
    const NLValueId a =
        c->bindings[nl_semantic_find_binding(c, "linked") - 1].view.value;
    const NLValueId b =
        c->bindings[nl_semantic_find_binding(c, "copied") - 1].view.value;
    CHECK(c->values[a - 1].fields[0] != c->values[b - 1].fields[0]);
    CHECK(c->values[c->values[a - 1].fields[0] - 1].sum_payload !=
          c->values[c->values[b - 1].fields[0] - 1].sum_payload);
    CHECK(test_rejected(c, "n.next", TEST_SOURCE, NL_CHECK_SEMANTIC_UNSUPPORTED,
                        "FIELD-PROFILE"));
    nl_semantic_destroy(c);
    return true;
}
static bool cycle(void)
{
    for (size_t kind = 0; kind < 2; ++kind) {
        NLSemanticContext *c = NULL;
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
        NLTypeId h, p, o;
        CHECK(nl_recursive_header(c, "N", &h) == NL_CHECK_OK);
        CHECK(nl_sem_compound(c, NL_TYPE_PTR, h, NL_ACCESS_READ, false, &p) ==
              NL_CHECK_OK);
        CHECK(nl_recursive_option(c, p, &o) == NL_CHECK_OK);
        if (kind == 1)
            c->types[o - 1].variant_types[1] =
                h; /* adversarial sum containment cycle */
        const NLAggregateField fields[] = {
            {"next", kind == 0 ? h : o},
            {"payload", nl_semantic_core_type(c, NL_TYPE_U8)}};
        CHECK(nl_recursive_complete(c, h, fields, 2) ==
              NL_CHECK_SEMANTIC_ERROR);
        bool complete = true;
        CHECK(nl_semantic_type_completion(c, h, &complete) && !complete);
        CHECK(c->types[h - 1].view.field_count == 0 && c->value_count == 0);
        nl_semantic_destroy(c);
    }
    return true;
}
static bool negatives(void)
{
    const char *text[] = {
        "struct N{a:Option<ptr<Missing>>,b:u8}",
        "struct N{a:Option<ptr<u8>>,b:u8}",
        "struct N{a:Option<ptr<N>>,a:u8}",
        "struct N{a:Option<ptr<N>>,b:u8}struct N{a:Option<ptr<N>>,b:u8}",
        "struct N{a:Option<ptr<N>>,b:u8}fn N()->unit{unit}",
        "struct unit{a:Option<ptr<unit>>,b:u8}"};
    for (size_t i = 0; i < sizeof(text) / sizeof(text[0]); ++i) {
        NLSemanticContext *c = NULL;
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
        TestState before;
        CHECK(test_state(c, &before));
        CHECK(register_text(c, text[i],
                            i == 1 ? NL_CHECK_SEMANTIC_UNSUPPORTED
                                   : NL_CHECK_SEMANTIC_ERROR,
                            NULL));
        CHECK(test_unchanged(c, &before));
        nl_semantic_destroy(c);
    }
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(register_text(c, decl, NL_CHECK_OK, NULL));
    TestState before;
    CHECK(test_state(c, &before));
    CHECK(register_text(c, "struct Other{link:Option<ptr<Node>>,data:u8}",
                        NL_CHECK_SEMANTIC_UNSUPPORTED, "REC-SELF-TARGET"));
    CHECK(test_unchanged(c, &before));
    CHECK(
        register_text(c, decl, NL_CHECK_SEMANTIC_ERROR, "REC-DECL-DUPLICATE"));
    CHECK(test_unchanged(c, &before));
    nl_semantic_destroy(c);
    return true;
}
static bool failures(void)
{
    bool success = false;
    for (fail_at = 0; fail_at < 512; ++fail_at) {
        NLSource *s = NULL;
        NLParser *p = NULL;
        NLSyntaxTree *t = NULL;
        CHECK(nl_source_create(witness, strlen(witness), "oom-parse", &s) ==
              NL_SOURCE_OK);
        CHECK(nl_parser_create(s, &p) == NL_PARSE_OK);
        allocation_index = 0;
        injecting = true;
        const NLParseStatus status = nl_parser_parse_function_unit(p, &t, NULL);
        injecting = false;
        if (status == NL_PARSE_OK)
            success = true;
        else {
            CHECK(status == NL_PARSE_OUT_OF_MEMORY && t == NULL);
            CHECK(nl_parser_parse_function_unit(p, &t, NULL) == NL_PARSE_OK);
        }
        nl_syntax_tree_destroy(t);
        nl_parser_destroy(p);
        nl_source_destroy(s);
        if (success)
            break;
    }
    CHECK(success);
    NLSource *s = NULL;
    NLSyntaxTree *t = NULL;
    CHECK(parse(witness, &s, &t));
    for (size_t mode = 0; mode < 2; ++mode) {
        success = false;
        for (fail_at = 0; fail_at < 512; ++fail_at) {
            NLSemanticContext *c = NULL;
            CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
            if (mode == 1)
                CHECK(register_text(c, witness, NL_CHECK_OK, NULL));
            TestState before;
            CHECK(test_state(c, &before));
            const NLSyntaxTree *inputs[] = {t};
            TestChecked call = {0};
            NLSource *call_source = NULL;
            NLParser *parser = NULL;
            NLSyntaxTree *call_tree = NULL;
            if (mode == 1) {
                CHECK(nl_source_create("main()", 6, "oom-body", &call_source) ==
                      NL_SOURCE_OK);
                CHECK(nl_parser_create(call_source, &parser) == NL_PARSE_OK);
                CHECK(nl_parser_parse_source_fragment(parser, &call_tree,
                                                      NULL) == NL_PARSE_OK);
            }
            allocation_index = 0;
            injecting = true;
            const NLCheckStatus status =
                mode == 0
                    ? nl_semantic_register_function_unit(c, inputs, 1, NULL)
                    : nl_semantic_check_source_fragment(c, call_tree,
                                                        &call.artifact, NULL);
            injecting = false;
            if (status == NL_CHECK_OK) {
                success = true;
                CHECK(nl_recursive_validate(c) == NL_CHECK_OK &&
                      nl_sem_validate(c) == NL_CHECK_OK);
            } else {
                CHECK(status == NL_CHECK_OUT_OF_MEMORY &&
                      call.artifact == NULL);
                CHECK(test_unchanged(c, &before));
            }
            nl_checked_destroy(call.artifact);
            nl_syntax_tree_destroy(call_tree);
            nl_parser_destroy(parser);
            nl_source_destroy(call_source);
            nl_semantic_destroy(c);
            if (success)
                break;
        }
        CHECK(success);
    }
    nl_syntax_tree_destroy(t);
    nl_source_destroy(s);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "parser") == 0)
        return parser_tests() ? 0 : 1;
    if (strcmp(argv[1], "identity") == 0)
        return identity() ? 0 : 1;
    if (strcmp(argv[1], "source") == 0)
        return source() ? 0 : 1;
    if (strcmp(argv[1], "cycle") == 0)
        return cycle() ? 0 : 1;
    if (strcmp(argv[1], "negatives") == 0)
        return negatives() ? 0 : 1;
    if (strcmp(argv[1], "failures") == 0)
        return failures() ? 0 : 1;
    return 2;
}
