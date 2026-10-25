#include "copy_body_internal.h"
#include "newlang/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(x)                                                             \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "P920 line%d: %s\n", __LINE__, #x);                \
            exit(1);                                                           \
        }                                                                      \
    } while (0)
static bool legacy;
static int inject;
static size_t hits;
static void change(NLCheckedFragment *root, int mode)
{
    if (mode == 1) {
        NLCheckedArm e = root->arms[0];
        root->arms[0] = root->arms[1];
        root->arms[1] = e;
    }
    if (mode == 2)
        root->arms[0].match = 0;
    if (mode == 3)
        root->arms[0].context_origin = root->arms[1].context_origin;
}
void __real_nl_definition_owned_take(NLCheckedFragment **,
                                     NLSemanticContext **);
void __wrap_nl_definition_owned_take(NLCheckedFragment **root,
                                     NLSemanticContext **world)
{
    if (!inject || !root || !*root || (*root)->arm_count != 2) {
        __real_nl_definition_owned_take(root, world);
        return;
    }
    NLCheckedFragment *original = *root;
    NLSemanticContext *context = *world;
    NLCheckedArm saved[2] = {original->arms[0], original->arms[1]};
    change(original, inject);
    ++hits;
    __real_nl_definition_owned_take(root, world);
    bool moved = *root == NULL;
    REQUIRE(legacy ? moved : (*root == original && *world == context));
    original->arms[0] = saved[0];
    original->arms[1] = saved[1];
    printf("take fault%d hit, original transferred=%d (expected%d), original "
           "edges restored\n",
           inject, moved, legacy);
}
static NLDefinitionOwned *construct(const char *path, int mode)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLSemanticContext *context = NULL;
    REQUIRE(nl_source_load(path, &source) == NL_SOURCE_OK &&
            nl_parser_create(source, &parser) == NL_PARSE_OK &&
            nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK &&
            nl_semantic_create(&context) == NL_CHECK_OK);
    size_t bindings = context->binding_count, values = context->value_count,
           places = context->place_count, functions = context->function_count;
    NLDefinitionOwned *w = NULL;
    REQUIRE(nl_definition_owned_open(&w) == NL_CHECK_OK);
    inject = mode;
    hits = 0;
    const NLSyntaxTree *units[] = {tree};
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus s = nl_semantic_register_function_unit(context, units, 1, &d);
    inject = 0;
    nl_definition_owned_close(w);
    REQUIRE(s == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
            !strcmp(d.diagnostic.diagnostic.code, "P6-JOIN-PRECISION") &&
            context->binding_count == bindings &&
            context->value_count == values && context->place_count == places &&
            context->function_count == functions);
    REQUIRE(!mode || hits == 1);
    if (mode && !legacy) {
        REQUIRE(nl_definition_owned_status(w) ==
                    NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
                nl_definition_owned_count(w) == 0 &&
                !nl_definition_owned_link(w, 0));
        nl_definition_owned_destroy(w);
        w = NULL;
        REQUIRE(nl_definition_owned_open(&w) == NL_CHECK_OK);
        s = nl_semantic_register_function_unit(context, units, 1, &d);
        nl_definition_owned_close(w);
        REQUIRE(s == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
                !strcmp(d.diagnostic.diagnostic.code, "P6-JOIN-PRECISION") &&
                context->binding_count == bindings &&
                context->value_count == values &&
                context->place_count == places &&
                context->function_count == functions);
        printf("take fault%d: rejected with zero records, SAME registration "
               "context retry complete\n",
               mode);
    }
    REQUIRE(nl_definition_owned_status(w) == NL_CHECK_OK &&
            nl_definition_owned_count(w) == 2);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return w;
}
static void verify(NLDefinitionOwned *w, NLCheckStatus expected,
                   const char *label)
{
    for (size_t i = 0; i < 2; ++i) {
        const NLDefinitionOwnedLink *v = nl_definition_owned_link(w, i);
        NLDefinitionCallResult r = {0};
        REQUIRE(v && v->arm == i);
        NLCheckStatus s = nl_definition_owned_validate(w, i, v, &r);
        printf("%s arm%zu private=%d PRE%d old%d close%d\n", label, i, s,
               r.pre_verified, r.return_old_verified, r.loan_closed);
        REQUIRE(s == expected);
        if (s == NL_CHECK_OK)
            REQUIRE(r.pre_verified && r.return_old_verified && r.loan_closed &&
                    r.replacements == 1 && r.old_known && r.sibling_known &&
                    r.old_value == 94 && r.sibling == 36 &&
                    r.incoming == (i ? 207 : 13) && v->field.index == 1);
    }
}
static void post(const char *path)
{
    NLDefinitionOwned *w = construct(path, 0);
    verify(w, NL_CHECK_OK, "original after input/registration destruction");
    const NLDefinitionOwnedLink *a = nl_definition_owned_link(w, 0),
                                *b = nl_definition_owned_link(w, 1);
    NLCheckedFragment *root = (NLCheckedFragment *)a->parent;
    REQUIRE(root == b->parent && root->arm_count == 2 &&
            a->caller != b->caller && a->world != b->world &&
            a->call == b->call && a->callee->context == a->caller->context &&
            b->callee->context == b->caller->context);
    NLCheckedNodeId conditional = a->caller->if_parent_node;
    REQUIRE(conditional == b->caller->if_parent_node &&
            nl_checked_if_arm(root, conditional, 0) == a->caller &&
            nl_checked_if_arm(root, conditional, 1) == b->caller);
    NLCheckedArm saved[2] = {root->arms[0], root->arms[1]};
    for (int mode = 1; mode <= 3; ++mode) {
        change(root, mode);
        if (mode == 1)
            REQUIRE(nl_checked_if_arm(root, conditional, 0) == b->caller &&
                    nl_checked_if_arm(root, conditional, 1) == a->caller);
        if (mode == 2)
            REQUIRE(nl_checked_if_arm(root, conditional, 0) == b->caller &&
                    nl_checked_if_arm(root, conditional, 1) == NULL);
        if (mode == 3)
            REQUIRE(nl_checked_if_arm(root, conditional, 0) == a->caller &&
                    root->arms[0].context_origin == b->world);
        printf("post fault%d: canonical edge mutation reached\n", mode);
        verify(w, legacy ? NL_CHECK_OK : NL_CHECK_ANALYSIS_PRECISION_LIMIT,
               "mutated parent edge");
        root->arms[0] = saved[0];
        root->arms[1] = saved[1];
        verify(w, NL_CHECK_OK, "restored exact entries");
    }
    nl_definition_owned_destroy(w);
}
int main(int argc, char **argv)
{
    REQUIRE(argc >= 4);
    legacy = !strcmp(argv[1], "legacy");
    bool before = !strcmp(argv[2], "take");
    for (int i = 3; i < argc; ++i) {
        if (!before) {
            post(argv[i]);
            continue;
        }
        for (int mode = 1; mode <= 3; ++mode) {
            NLDefinitionOwned *w = construct(argv[i], mode);
            verify(w, NL_CHECK_OK, "healthy restored/retry owner");
            nl_definition_owned_destroy(w);
        }
    }
    printf("P920 %s %s finished; source CHECK3 unchanged\n", argv[1], argv[2]);
    return 0;
}
