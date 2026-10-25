#include "../../src/copy_body_internal.h"
#include "../support/node_checked.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(x)                                                             \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "P917 line%d: %s\n", __LINE__, #x);                \
            exit(1);                                                           \
        }                                                                      \
    } while (0)
typedef struct {
    NLSource *source;
    NLParser *parser;
    NLSyntaxTree *tree;
    NLSemanticContext *context;
} Input;
static size_t observations, refusals;
static void observe(const NLDefinitionCallCapture *p,
                    const NLDefinitionCallView *v, NLCheckStatus status,
                    void *unused)
{
    (void)unused;
    NLDefinitionCallResult r;
    REQUIRE(status == NL_CHECK_OK && p && v &&
            nl_definition_call_validate(p, v, &r) == NL_CHECK_OK &&
            r.pre_verified && r.return_old_verified && r.loan_closed);
    ++observations; /* No borrowed pointer escapes this callback. */
}
static Input load(const char *path)
{
    Input in = {0};
    REQUIRE(nl_source_load(path, &in.source) == NL_SOURCE_OK &&
            nl_parser_create(in.source, &in.parser) == NL_PARSE_OK &&
            nl_parser_parse_function_unit(in.parser, &in.tree, NULL) ==
                NL_PARSE_OK &&
            nl_semantic_create(&in.context) == NL_CHECK_OK);
    return in;
}
static void release(Input *in)
{
    nl_semantic_destroy(in->context);
    nl_syntax_tree_destroy(in->tree);
    nl_parser_destroy(in->parser);
    nl_source_destroy(in->source);
    *in = (Input){0};
}
static NLDefinitionOwned *construct(Input *in, bool observer)
{
    TestState *before = calloc(1, sizeof(*before));
    REQUIRE(before && test_state(in->context, before));
    NLDefinitionOwned *w = NULL;
    REQUIRE(nl_definition_owned_open(&w) == NL_CHECK_OK);
    NLDefinitionOwned *nested = NULL;
    REQUIRE(nl_definition_owned_open(&nested) ==
                NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
            !nested); /* Session reentry is explicitly unsupported. */
    observations = 0;
    if (observer)
        nl_definition_call_observer(observe, NULL);
    const NLSyntaxTree *units[] = {in->tree};
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus s =
        nl_semantic_register_function_unit(in->context, units, 1, &d);
    nl_definition_call_observer(NULL, NULL);
    nl_definition_owned_close(w);
    REQUIRE(s == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
            !strcmp(d.diagnostic.diagnostic.code, "P6-JOIN-PRECISION") &&
            test_unchanged(in->context, before));
    if (observer)
        REQUIRE(observations == 2);
    free(before);
    return w;
}
static void refuse(const NLDefinitionOwned *w, size_t i,
                   const NLDefinitionOwnedLink *v)
{
    NLDefinitionCallResult r;
    REQUIRE(nl_definition_owned_validate(w, i, v, &r) ==
            NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    ++refusals;
}
static void faults(NLDefinitionOwned *w, NLDefinitionOwned *foreign, size_t i)
{
    const NLDefinitionOwnedLink *v = nl_definition_owned_link(w, i);
    const NLDefinitionOwnedLink *alien = nl_definition_owned_link(foreign, i);
    REQUIRE(v && alien && v->caller != alien->caller &&
            v->caller->context != alien->caller->context &&
            v->call == alien->call);
#define BAD(change)                                                            \
    do {                                                                       \
        NLDefinitionOwnedLink bad = *v;                                        \
        change;                                                                \
        refuse(w, i, &bad);                                                    \
    } while (0)
    BAD(bad.caller = alien->caller);
    BAD(bad.parent = alien->parent);
    BAD(bad.callee = alien->callee);
    BAD(bad.world = alien->world);
    BAD(bad.pre = v->post);
    BAD(bad.arm = 1 - v->arm);
    BAD(bad.arguments[0] = v->arguments[1]);
    BAD(bad.formals[0] = v->formals[1]);
    BAD(bad.field.parent = v->other);
    BAD(bad.field.child = v->sibling);
    BAD(bad.field.index = 1 - v->field.index);
    BAD(++bad.field.parent_incarnation);
    BAD(++bad.field.child_incarnation);
    BAD(++bad.field.parent_fact);
    BAD(++bad.field.child_fact);
    BAD(bad.scope = 0);
    BAD(bad.ordinal = 1);
    BAD(bad.pending_count = 2);
    BAD(bad.call = v->loan);
    BAD(bad.body = alien->body);
#undef BAD
    NLCheckedFragment *f = (NLCheckedFragment *)v->caller;
    NLCheckedFragment *body = (NLCheckedFragment *)v->callee;
    /* Actual graph edge mutation with authentic equal-number foreign graph. */
    NLCheckedFragment *saved_body = f->bodies[0];
    f->bodies[0] = (NLCheckedFragment *)alien->callee;
    refuse(w, i, v);
    f->bodies[0] = saved_body;
    const NLDefinitionOwnedLink *other = nl_definition_owned_link(w, 1 - i);
    REQUIRE(other && other->call == v->call && other->world != v->world);
    f->bodies[0] = (NLCheckedFragment *)other->callee;
    refuse(w, i, v);
    f->bodies[0] = saved_body;
    NLCheckedNodeView saved = f->nodes[v->call - 1];
    f->nodes[v->call - 1].first_argument = v->arguments[1];
    refuse(w, i, v);
    f->nodes[v->call - 1] = saved;
    NLCheckedNodeView loan = f->nodes[v->loan - 1];
    f->nodes[v->loan - 1].loan.scope = 0;
    refuse(w, i, v);
    f->nodes[v->loan - 1] = loan;
    NLSemanticContext *world = (NLSemanticContext *)v->world;
    world->scopes[v->scope - 1].active = true;
    refuse(w, i, v);
    world->scopes[v->scope - 1].active = false;
    ++world->places[v->field.child - 1].current_fact;
    refuse(w, i, v);
    --world->places[v->field.child - 1].current_fact;
    ++world->places[v->sibling - 1].current_fact;
    refuse(w, i, v);
    --world->places[v->sibling - 1].current_fact;
    for (size_t j = 0; j < body->count; ++j) {
        NLCheckedNodeView node = body->nodes[j];
        if (node.kind == NL_CHECKED_REPLACE) {
            body->nodes[j].field.child = v->sibling;
            refuse(w, i, v);
            body->nodes[j] = node;
            ++body->nodes[j].field.parent_post_fact;
            refuse(w, i, v);
            body->nodes[j] = node;
        }
        if (node.kind == NL_CHECKED_RETURN) {
            body->nodes[j].returned.value =
                world->places[v->field.child - 1].current_value;
            refuse(w, i, v); /* post-current is not historical returned old */
            body->nodes[j] = node;
        }
    }
    const NLSource *source = f->source;
    f->source = body->source;
    refuse(w, i, v);
    f->source = source;
    NLControlTarget *target = body->function_target;
    body->function_target = v->parent->function_target;
    refuse(w, i, v);
    body->function_target = target;
    NLDefinitionCallResult r;
    REQUIRE(nl_definition_owned_validate(w, i, v, &r) == NL_CHECK_OK &&
            r.return_old_verified && r.loan_closed);
}
static void run(const char *path, bool negative)
{
    bool quota = strstr(path, "quota") != NULL;
    bool bad_return = strstr(path, "wrong_return") != NULL;
    bool absent =
        strstr(path, "missing_call") || strstr(path, "duplicate_call");
    Input in = load(path);
    NLDefinitionOwned *w = construct(&in, !negative);
    release(
        &in); /* All input/registration owners gone BEFORE any owned replay. */
    if (absent) {
        REQUIRE(nl_definition_owned_status(w) ==
                    NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
                nl_definition_owned_count(w) == 0 &&
                !nl_definition_owned_link(w, 0));
        nl_definition_owned_destroy(w);
        return;
    }
    REQUIRE(nl_definition_owned_status(w) == NL_CHECK_OK &&
            nl_definition_owned_count(w) == 2);
    NLDefinitionOwned *foreign = NULL;
    if (negative && !bad_return) {
        Input other = load(path);
        foreign = construct(&other, false);
        release(&other);
        REQUIRE(nl_definition_owned_count(foreign) == 2);
    }
    for (size_t i = 0; i < 2; ++i) {
        const NLDefinitionOwnedLink *v = nl_definition_owned_link(w, i);
        NLDefinitionCallResult r;
        NLCheckStatus s = nl_definition_owned_validate(w, i, v, &r);
        if (bad_return) {
            REQUIRE(s == NL_CHECK_ANALYSIS_PRECISION_LIMIT && r.pre_verified &&
                    !r.return_old_verified);
            ++refusals;
            continue;
        }
        REQUIRE(s == NL_CHECK_OK && r.pre_verified && r.return_old_verified &&
                r.loan_closed && r.replacements == 1 && r.old_known &&
                r.sibling_known && r.old_value == (quota ? 17 : 231) &&
                r.sibling == (quota ? 203 : 6) &&
                r.incoming == (quota ? (i ? 122 : 64) : (i ? 173 : 19)) &&
                v->field.index == (quota ? 1U : 0U));
        printf("owned after destruction: %s arm%zu CALL%zu root%zu child%zu "
               "inc%zu/%zu old%llu new%llu sibling%llu PRE/RETURN/close OK\n",
               path, i, v->call, v->field.parent, v->field.child,
               v->field.parent_incarnation, v->field.child_incarnation,
               (unsigned long long)r.old_value, (unsigned long long)r.incoming,
               (unsigned long long)r.sibling);
        if (foreign)
            faults(w, foreign, i);
    }
    nl_definition_owned_destroy(foreign);
    nl_definition_owned_destroy(w);
}
int main(int argc, char **argv)
{
    REQUIRE(argc > 2);
    bool negative = !strcmp(argv[1], "negative");
    for (int i = 2; i < argc; ++i)
        run(argv[i], negative);
    printf(
        "P917 private owned source-instance replay; refusals%zu; CHECK3 kept\n",
        refusals);
    return 0;
}
