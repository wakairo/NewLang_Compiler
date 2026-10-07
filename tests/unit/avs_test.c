#include "../support/semantic_check.h"
#include <stdlib.h>

static const char declaration[] = "struct Pair{left:u8,right:u8,}";
static const char witness[] =
    "struct Pair{left:u8,right:u8,}"
    "fn main()->unit{let p=Pair{left:u8(7),right:u8(9),};"
    "let Pair{left,right}=p;left;right;unit}";

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
    CHECK(nl_source_create(text, strlen(text), "avs", source) == NL_SOURCE_OK);
    NLParser *p = NULL;
    CHECK(nl_parser_create(*source, &p) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(p, tree, NULL) == NL_PARSE_OK);
    nl_parser_destroy(p);
    return true;
}
static bool register_source(NLSemanticContext *c, const char *text)
{
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(parse(text, &source, &tree));
    const NLSyntaxTree *inputs[] = {tree};
    NLFunctionUnitDiagnostic d = {0};
    const NLCheckStatus status =
        nl_semantic_register_function_unit(c, inputs, 1, &d);
    if (status != NL_CHECK_OK)
        fprintf(stderr, "registration %d: %s\n", status,
                d.diagnostic.diagnostic.code);
    CHECK(status == NL_CHECK_OK);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    return true;
}
static bool fields(const NLSemanticContext *c, NLTypeId type)
{
    NLSemanticTypeView shape;
    CHECK(nl_semantic_type_view(c, type, &shape));
    CHECK(shape.kind == NL_TYPE_NOMINAL && shape.field_count == 2 &&
          shape.is_copy && shape.is_discardable && !shape.layout_known);
    NLAggregateField f;
    CHECK(nl_semantic_aggregate_field_view(c, type, 0, &f));
    CHECK(strcmp(f.name, "left") == 0 &&
          f.type == nl_semantic_core_type(c, NL_TYPE_U8));
    CHECK(nl_semantic_aggregate_field_view(c, type, 1, &f));
    CHECK(strcmp(f.name, "right") == 0 &&
          f.type == nl_semantic_core_type(c, NL_TYPE_U8));
    const NLAggregateField saved = f;
    CHECK(!nl_semantic_aggregate_field_view(c, type, 2, &f));
    CHECK(f.name == saved.name && f.type == saved.type);
    CHECK(!nl_semantic_aggregate_field_view(c, 0, 0, &f));
    CHECK(!nl_semantic_aggregate_field_view(
        c, nl_semantic_core_type(c, NL_TYPE_U8), 0, &f));
    return true;
}
static bool parser(void)
{
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(parse(witness, &source, &tree));
    const NLSyntaxView *unit = nl_syntax_node_view(nl_syntax_tree_root(tree));
    CHECK(unit->data.function_unit.count == 2);
    const NLSyntaxNode *decl = unit->data.function_unit.declarations;
    const NLSyntaxView *v = nl_syntax_node_view(decl);
    CHECK(v->kind == NL_SYNTAX_AVS_STRUCT && v->data.avs_struct.count == 2);
    CHECK(nl_syntax_node_view(nl_syntax_next_argument(decl))->kind ==
          NL_SYNTAX_FUNCTION);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    /* These are implementation-profile fences, not decisions about general
     * declaration validity, ordering, recursion or other scalar families. */
    const char *outside[] = {
        "struct Pair{left:bool,right:u8}fn main()->unit{unit}",
        "struct Pair{left:u8,right:u8,third:u8}fn main()->unit{unit}",
        "fn main()->unit{unit}struct Pair{left:u8,right:u8}",
        ("struct Pair{left:u8,right:u8}struct Other{a:u8,b:u8}fn "
         "main()->unit{unit}")};
    for (size_t i = 0; i < sizeof(outside) / sizeof(outside[0]); ++i) {
        source = NULL;
        CHECK(nl_source_create(outside[i], strlen(outside[i]), "avs-profile",
                               &source) == NL_SOURCE_OK);
        NLParser *p = NULL;
        CHECK(nl_parser_create(source, &p) == NL_PARSE_OK);
        for (size_t repeat = 0; repeat < 2; ++repeat) {
            tree = NULL;
            NLParseDiagnostic d = {0};
            CHECK(nl_parser_parse_function_unit(p, &tree, &d) ==
                  NL_PARSE_SYNTAX_UNSUPPORTED);
            CHECK(tree == NULL &&
                  strcmp(d.diagnostic.code, "AVS-DECL-PROFILE") == 0);
        }
        nl_parser_destroy(p);
        nl_source_destroy(source);
    }
    return true;
}
static bool unit_fence(void)
{
    NLSource *sources[2] = {NULL, NULL};
    NLSyntaxTree *trees[2] = {NULL, NULL};
    CHECK(parse(witness, &sources[0], &trees[0]));
    CHECK(parse("fn helper()->unit{unit}", &sources[1], &trees[1]));
    const NLSyntaxTree *inputs[] = {trees[0], trees[1]};
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState before;
    CHECK(test_state(c, &before));
    NLFunctionUnitDiagnostic d = {0};
    CHECK(nl_semantic_register_function_unit(c, inputs, 2, &d) ==
          NL_CHECK_SEMANTIC_UNSUPPORTED);
    CHECK(strcmp(d.diagnostic.diagnostic.code, "AVS-UNIT-PROFILE") == 0);
    CHECK(test_unchanged(c, &before));
    for (size_t i = 0; i < 2; ++i) {
        nl_syntax_tree_destroy(trees[i]);
        nl_source_destroy(sources[i]);
    }
    nl_semantic_destroy(c);
    return true;
}

static bool evidence_case(const char *text, bool reverse)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(c, &before));
    CHECK(register_source(c,
                          text)); /* No host-only aggregate pre-registration. */
    CHECK(nl_semantic_snapshot(c, &after));
    CHECK(after.types == before.types + 1);
    TestChecked call = {0};
    CHECK(test_run(c, "main()", TEST_EXPRESSION, NL_CHECK_OK, NULL, &call));
    const NLCheckedFragment *body =
        nl_checked_call_body(call.artifact, nl_checked_root(call.artifact));
    CHECK(body != NULL);
    const NLCheckedNodeView *root =
        nl_checked_node_view(body, nl_checked_root(body));
    CHECK(root != NULL && root->kind == NL_CHECKED_BLOCK);
    const NLCheckedNodeView *binding =
        nl_checked_node_view(body, root->first_item);
    CHECK(binding != NULL && binding->kind == NL_CHECKED_BINDING);
    const NLCheckedNodeView *construction =
        nl_checked_node_view(body, binding->initializer);
    CHECK(construction != NULL && construction->kind == NL_CHECKED_AGGREGATE &&
          construction->argument_count == 2);
    const NLTypeId pair = construction->type;
    CHECK(fields(
        c, pair)); /* E1: retained source nominal identity/field registry. */
    NLCheckedNodeId id = construction->first_argument;
    for (size_t i = 0; i < 2; ++i) {
        const size_t index = reverse ? 1 - i : i;
        const NLCheckedNodeView *f = nl_checked_node_view(body, id);
        CHECK(f != NULL && f->kind == NL_CHECKED_AGGREGATE_FIELD &&
              f->field_index == index);
        const NLCheckedNodeView *value =
            nl_checked_node_view(body, f->initializer);
        CHECK(value != NULL && value->kind == NL_CHECKED_U8_LITERAL &&
              value->scalar_result.known);
        CHECK(value->type == nl_semantic_core_type(c, NL_TYPE_U8) &&
              value->scalar_result.value == (index == 0 ? 7 : 9));
        id = f->next_argument;
    }
    CHECK(id ==
          0); /* E2: complete checked fields, source order != field identity. */
    const NLCheckedNodeView *destructure =
        nl_checked_node_view(body, binding->next_item);
    CHECK(destructure != NULL &&
          destructure->kind == NL_CHECKED_AGGREGATE_BINDING &&
          destructure->argument_count == 2);
    const NLCheckedNodeView *rhs =
        nl_checked_node_view(body, destructure->initializer);
    CHECK(rhs != NULL && rhs->type == pair && rhs->symbol == binding->symbol &&
          rhs->value_use == NL_VALUE_COPIED);
    id = destructure->first_argument;
    NLCheckedNodeId statement = destructure->next_item;
    for (size_t i = 0; i < 2; ++i) {
        const size_t index = reverse ? 1 - i : i;
        const NLCheckedNodeView *r = nl_checked_node_view(body, id);
        CHECK(r != NULL && r->kind == NL_CHECKED_RECEIVER &&
              r->field_index == index);
        NLSemanticBindingView receiver;
        NLSemanticValueView member;
        CHECK(nl_semantic_binding_view(c, r->symbol, &receiver));
        CHECK(nl_semantic_value_view(c, receiver.value, &member));
        CHECK(member.type == nl_semantic_core_type(c, NL_TYPE_U8) &&
              member.scalar_known &&
              member.scalar_value == (index == 0 ? 7 : 9));
        const NLCheckedNodeView *s = nl_checked_node_view(body, statement);
        const NLCheckedNodeView *use =
            s == NULL ? NULL : nl_checked_node_view(body, s->initializer);
        CHECK(use != NULL && use->symbol == r->symbol &&
              use->value_use == NL_VALUE_COPIED);
        id = r->next_argument;
        statement = s->next_item;
    }
    CHECK(id ==
          0); /* E3: same Pair, resolved receiver indices/symbols/values. */
    test_checked_destroy(&call);
    nl_semantic_destroy(c);
    return true;
}
static bool evidence(void)
{
    CHECK(evidence_case(witness, false));
    CHECK(evidence_case("struct Pair{left:u8,right:u8}fn main()->unit{"
                        "let p=Pair{right:u8(9),left:u8(7)};"
                        "let Pair{right,left}=p;right;left;unit}",
                        true));
    return true;
}
static bool reject_body(const char *body, const char *code)
{
    char text[1024];
    CHECK(snprintf(text, sizeof(text), "%sfn main()->unit{%s}", declaration,
                   body) > 0);
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    CHECK(parse(text, &source, &tree));
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState before;
    CHECK(test_state(c, &before));
    const NLSyntaxTree *inputs[] = {tree};
    for (size_t repeat = 0; repeat < 2; ++repeat) {
        NLFunctionUnitDiagnostic d = {0};
        CHECK(nl_semantic_register_function_unit(c, inputs, 1, &d) ==
              NL_CHECK_SEMANTIC_ERROR);
        CHECK(d.input_index == 0 &&
              strcmp(d.diagnostic.diagnostic.code, code) == 0);
        CHECK(nl_source_span_valid(source, d.diagnostic.span));
        CHECK(test_unchanged(
            c, &before)); /* Includes rollback of source nominal. */
    }
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    nl_semantic_destroy(c);
    return true;
}
static bool rejection(void)
{
    CHECK(reject_body("let p=Pair{left:u8(7),nope:u8(9)};unit",
                      "P5-AGGREGATE-FIELD"));
    CHECK(
        reject_body("let p=Pair{left:u8(7)};unit", "P5-AGGREGATE-FIELD-COUNT"));
    CHECK(reject_body("let p=Pair{left:u8(7),left:u8(9)};unit",
                      "P5-AGGREGATE-FIELD"));
    CHECK(reject_body(
        "let p=Pair{left:u8(7),right:u8(9)};let Pair{left,nope}=p;unit",
        "P5-AGGREGATE-FIELD"));
    /* Source-order evaluation, not declaration-order evaluation. No second
     * scalar family or new operation is introduced by these controls. */
    CHECK(reject_body("let p=Pair{right:missing(),left:u8(256)};unit",
                      "P3-UNKNOWN-CALLEE"));
    CHECK(reject_body("let p=Pair{left:u8(256),right:missing()};unit",
                      "V1-U8-LITERAL-RANGE"));
    return true;
}
static bool ownership(void)
{
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(register_source(
        c, "struct Pair{left:u8,right:u8}fn main()->unit{unit}"));
    TestChecked a = {0};
    CHECK(test_run(c, "let p=Pair{left:u8(7),right:u8(9)};", TEST_SOURCE,
                   NL_CHECK_OK, NULL, &a));
    test_checked_destroy(&a);
    NLSemanticBindingView original;
    const NLSymbolId p = nl_semantic_find_binding(c, "p");
    CHECK(nl_semantic_binding_view(c, p, &original));
    CHECK(fields(c, original.type));
    CHECK(test_rejected(c, "let Pair{left,nope}=p;", TEST_SOURCE,
                        NL_CHECK_SEMANTIC_ERROR, "P5-AGGREGATE-FIELD"));
    CHECK(nl_semantic_find_binding(c, "left") == 0);
    CHECK(test_run(c, "let Pair{right,left}=p;", TEST_SOURCE, NL_CHECK_OK, NULL,
                   &a));
    test_checked_destroy(&a);
    NLSemanticBindingView after;
    CHECK(nl_semantic_binding_view(c, p, &after) &&
          after.availability == NL_AVAILABLE && after.value == original.value);
    NLSemanticValueView aggregate;
    CHECK(nl_semantic_value_view(c, original.value, &aggregate));
    CHECK(aggregate.field_count == 2);
    for (size_t i = 0; i < 2; ++i) {
        NLSemanticValueView v;
        CHECK(nl_semantic_value_view(c, aggregate.fields[i], &v));
        CHECK(v.aggregate_owner == original.value && v.scalar_known &&
              v.scalar_value == (i == 0 ? 7 : 9));
    }
    nl_semantic_destroy(c);
    return true;
}
static bool failures(void)
{
    NLSource *source = NULL;
    CHECK(nl_source_create(witness, strlen(witness), "avs-oom", &source) ==
          NL_SOURCE_OK);
    NLParser *p = NULL;
    CHECK(nl_parser_create(source, &p) == NL_PARSE_OK);
    NLSyntaxTree *tree = NULL;
    bool done = false;
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        const NLParseStatus status =
            nl_parser_parse_function_unit(p, &tree, NULL);
        injecting = false;
        if (status == NL_PARSE_OK) {
            done = true;
            break;
        }
        CHECK(status == NL_PARSE_OUT_OF_MEMORY && tree == NULL);
    }
    CHECK(done && fail_at > 0);
    nl_parser_destroy(p);
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState before;
    CHECK(test_state(c, &before));
    const NLSyntaxTree *inputs[] = {tree};
    done = false;
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        allocation_index = 0;
        injecting = true;
        const NLCheckStatus status =
            nl_semantic_register_function_unit(c, inputs, 1, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            done = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && test_unchanged(c, &before));
    }
    CHECK(done && fail_at > 0);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    source = NULL;
    CHECK(nl_source_create("main()", 6, "avs-call", &source) == NL_SOURCE_OK);
    p = NULL;
    tree = NULL;
    CHECK(nl_parser_create(source, &p) == NL_PARSE_OK);
    CHECK(nl_parser_parse_source_fragment(p, &tree, NULL) == NL_PARSE_OK);
    nl_parser_destroy(p);
    CHECK(test_state(c, &before));
    done = false;
    for (fail_at = 0; fail_at < 5000; ++fail_at) {
        NLCheckedFragment *a = NULL;
        allocation_index = 0;
        injecting = true;
        const NLCheckStatus status =
            nl_semantic_check_source_fragment(c, tree, &a, NULL);
        injecting = false;
        if (status == NL_CHECK_OK) {
            nl_checked_destroy(a);
            done = true;
            break;
        }
        CHECK(status == NL_CHECK_OUT_OF_MEMORY && a == NULL &&
              test_unchanged(c, &before));
        CHECK(fields(c, before.counts.types));
    }
    CHECK(done && fail_at > 0);
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    nl_semantic_destroy(c);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "parser") == 0)
        return (parser() && unit_fence()) ? 0 : 1;
    if (strcmp(argv[1], "evidence") == 0)
        return evidence() ? 0 : 1;
    if (strcmp(argv[1], "rejection") == 0)
        return rejection() ? 0 : 1;
    if (strcmp(argv[1], "ownership") == 0)
        return ownership() ? 0 : 1;
    if (strcmp(argv[1], "failures") == 0)
        return failures() ? 0 : 1;
    return 2;
}
