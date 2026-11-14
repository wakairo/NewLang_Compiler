#include "../../src/h_origin_internal.h"
#include "../../src/semantic_internal.h"
#include "../../src/unchecked_ref_internal.h"
#include "../support/semantic_check.h"
#include "newlang/checked_c_node.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    NLSource *source, *call_source;
    NLParser *parser, *call_parser;
    NLSyntaxTree *unit, *call;
    NLSemanticContext *context;
    NLCheckedFragment *artifact;
} Program;

static bool fail_emitter, hit_emitter;
void *__real_malloc(size_t);
void *__wrap_malloc(size_t size)
{
    if (fail_emitter && size == 131072) {
        fail_emitter = false;
        hit_emitter = true;
        return NULL;
    }
    return __real_malloc(size);
}

static void destroy(Program *p)
{
    nl_checked_destroy(p->artifact);
    nl_syntax_tree_destroy(p->call);
    nl_parser_destroy(p->call_parser);
    nl_source_destroy(p->call_source);
    nl_semantic_destroy(p->context);
    nl_syntax_tree_destroy(p->unit);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    *p = (Program){0};
}

static bool actual(const char *path, Program *p)
{
    CHECK(nl_source_load(path, &p->source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(p->source, &p->parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(p->parser, &p->unit, NULL) ==
          NL_PARSE_OK);
    CHECK(nl_semantic_create(&p->context) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {p->unit};
    NLFunctionUnitDiagnostic definition = {0};
    CHECK(nl_semantic_register_function_unit(p->context, units, 1,
                                             &definition) == NL_CHECK_OK);
    nl_syntax_tree_destroy(p->unit);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    p->unit = NULL;
    p->parser = NULL;
    p->source = NULL;
    CHECK(nl_source_create("main()", 6, "p1426-call", &p->call_source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(p->call_source, &p->call_parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(p->call_parser, &p->call, NULL) ==
          NL_PARSE_OK);
    CHECK(nl_semantic_check_expression(p->context, p->call, &p->artifact,
                                       NULL) == NL_CHECK_OK);
    return true;
}

static NLCheckedFragment *find(NLCheckedFragment *f,
                               NLCheckedLocalAssumptionView *record)
{
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        if (nl_checked_local_assumption_view(f, id, record))
            return f;
        const NLCheckedNodeView *node = nl_checked_node_view(f, id);
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        NLCheckedFragment *found =
            body ? find((NLCheckedFragment *)body, record) : NULL;
        if (found)
            return found;
        if (node->kind == NL_CHECKED_IF || node->kind == NL_CHECKED_MATCH)
            for (size_t arm = 0; arm < node->item_count; ++arm) {
                const NLCheckedFragment *child =
                    node->kind == NL_CHECKED_IF
                        ? nl_checked_if_arm(f, id, arm)
                        : nl_checked_match_arm(f, id, arm);
                found = child ? find((NLCheckedFragment *)child, record) : NULL;
                if (found)
                    return found;
            }
    }
    return NULL;
}

typedef struct {
    NLCheckedFragment *owner;
    NLUncheckedProof *recipe;
    NLSemanticContext *world;
    NLCheckedNodeView *condition, *binding, *call;
    NLFunctionEntry *function;
    NLCheckedLocalAssumptionView read;
} Trace;

static bool trace(Program *p, Trace *t)
{
    *t = (Trace){0};
    t->owner = find(p->artifact, &t->read);
    CHECK(t->owner && t->read.owner == t->owner);
    t->recipe = (NLUncheckedProof *)nl_h_unchecked_data(t->owner);
    t->world = (NLSemanticContext *)nl_checked_context(t->owner);
    CHECK(t->recipe && t->world && t->recipe->has_pointer &&
          t->recipe->has_read);
    t->condition = (NLCheckedNodeView *)nl_checked_node_view(
        t->owner, t->recipe->condition);
    CHECK(t->condition && t->condition->kind == NL_CHECKED_IDENTIFIER);
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(t->owner); ++id) {
        NLCheckedNodeView *node =
            (NLCheckedNodeView *)nl_checked_node_view(t->owner, id);
        if (node->kind == NL_CHECKED_BINDING &&
            node->symbol == t->condition->symbol) {
            CHECK(!t->binding);
            t->binding = node;
        }
    }
    CHECK(t->binding);
    t->call = (NLCheckedNodeView *)nl_checked_node_view(
        t->owner, t->binding->initializer);
    CHECK(t->call && t->call->function && t->world->functions &&
          t->call->function <= t->world->function_count);
    t->function = &t->world->functions[t->call->function - 1];
    return true;
}

static bool emit_ok(Program *p)
{
    char *out = NULL;
    size_t length = 98765;
    CHECK(nl_checked_c_node(p->artifact, &out, &length) == NL_NODE_C_OK);
    CHECK(out && length > 1000 && strstr(out, "nl_qualified_predicate") &&
          strstr(out, "NL_QUALIFIED_CONDITION") &&
          strstr(out, "NL_HEAP_ROOT") && strstr(out, "NL_HEAP_COPY"));
    free(out);
    return true;
}

static bool positive(int argc, char **argv)
{
    for (int i = 2; i < argc; ++i) {
        Program p = {0};
        Trace t;
        CHECK(actual(argv[i], &p) && trace(&p, &t));
        CHECK(nl_checked_proof_class(p.artifact) == NL_CHECKED_AUTHOR_PREMISE);
        CHECK(nl_checked_local_assumption_origin(t.owner, t.read.operation) ==
              NL_CHECK_OK);
        CHECK(emit_ok(&p));
        destroy(&p);
    }
    return true;
}

static bool rejected(Program *p)
{
    char *out = NULL;
    size_t length = 98765;
    CHECK(nl_checked_c_node(p->artifact, &out, &length) ==
          NL_NODE_C_UNSUPPORTED);
    CHECK(!out && length == 98765);
    return true;
}

#define DAMAGE(program, target, value)                                         \
    do {                                                                       \
        unsigned char saved[sizeof(target)];                                   \
        memcpy(saved, &(target), sizeof(target));                              \
        (target) = (value);                                                    \
        CHECK(rejected(program));                                              \
        memcpy(&(target), saved, sizeof(target));                              \
    } while (0)

static bool faults(const char *path)
{
    Program p = {0};
    Trace t;
    CHECK(actual(path, &p) && trace(&p, &t));
    DAMAGE(&p, t.recipe->read.assumed_conditions, 0u);
    DAMAGE(&p, t.recipe->read.operation, t.recipe->read.operation + 1);
    DAMAGE(&p, t.recipe->condition, t.recipe->pointer_if);
    DAMAGE(&p, t.recipe->copies[0].node, t.recipe->pointer_if);
    DAMAGE(&p, t.binding->initializer, t.recipe->pointer_if);
    DAMAGE(&p, t.call->function, t.world->function_count + 1);
    DAMAGE(&p, t.world->functions, NULL);
    DAMAGE(&p, t.function->parameters, NULL);
    DAMAGE(&p, t.function->caller_effects, true);
    CHECK(emit_ok(&p));
    destroy(&p);
    return true;
}

static bool oom(const char *path)
{
    Program p = {0};
    CHECK(actual(path, &p));
    char *out = NULL;
    size_t length = 98765;
    hit_emitter = false;
    fail_emitter = true;
    CHECK(nl_checked_c_node(p.artifact, &out, &length) ==
          NL_NODE_C_OUT_OF_MEMORY);
    fail_emitter = false;
    CHECK(hit_emitter && !out && length == 98765);
    CHECK(emit_ok(&p));
    destroy(&p);
    return true;
}

int main(int argc, char **argv)
{
    if (argc < 3)
        return 2;
    bool ok = false;
    if (!strcmp(argv[1], "positive"))
        ok = positive(argc, argv);
    else if (!strcmp(argv[1], "faults") && argc == 3)
        ok = faults(argv[2]);
    else if (!strcmp(argv[1], "oom") && argc == 3)
        ok = oom(argv[2]);
    return ok ? 0 : 1;
}
