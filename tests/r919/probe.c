#include "copy_body_internal.h"
#include "newlang/parser.h"
#include "newlang/source.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    NLSource *source;
    NLSyntaxTree *tree;
    NLSemanticContext *context;
} Input;
typedef struct {
    unsigned old_value, incoming[2], sibling;
    size_t index;
} Oracle;

static int failures;

static void require(bool ok, const char *what)
{
    if (!ok) {
        fprintf(stderr, "R919 STOP: %s\n", what);
        ++failures;
    }
}

static bool input_open(Input *in, const char *path)
{
    NLParser *p = NULL;
    NLParseDiagnostic diagnostic = {0};
    bool ok = nl_source_load(path, &in->source) == NL_SOURCE_OK &&
              nl_parser_create(in->source, &p) == NL_PARSE_OK &&
              nl_parser_parse_function_unit(p, &in->tree, &diagnostic) ==
                  NL_PARSE_OK &&
              nl_semantic_create(&in->context) == NL_CHECK_OK;
    nl_parser_destroy(p);
    return ok;
}

static void input_destroy(Input *in)
{
    nl_semantic_destroy(in->context);
    nl_syntax_tree_destroy(in->tree);
    nl_source_destroy(in->source);
    *in = (Input){0};
}

static bool registration(Input *in)
{
    const NLSyntaxTree *units[] = {in->tree};
    NLFunctionUnitDiagnostic diagnostic = {0};
    NLCheckStatus status =
        nl_semantic_register_function_unit(in->context, units, 1, &diagnostic);
    fprintf(stderr, "R919 CHECK status=%d first=%s\n", status,
            diagnostic.diagnostic.diagnostic.code);
    return status == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
           strcmp(diagnostic.diagnostic.diagnostic.code,
                  "P6-JOIN-PRECISION") == 0;
}

static NLDefinitionOwned *construct(Input *in)
{
    NLDefinitionOwned *owner = NULL;
    require(nl_definition_owned_open(&owner) == NL_CHECK_OK && owner != NULL,
            "private owner open");
    if (!owner)
        return NULL;
    NLDefinitionOwned *nested = NULL;
    require(nl_definition_owned_open(&nested) != NL_CHECK_OK && !nested,
            "nested session fail-closed");
    require(registration(in), "original CHECK3 unchanged");
    nl_definition_owned_close(owner);
    return owner;
}

static bool validate(const NLDefinitionOwned *owner, size_t index, Oracle o)
{
    const NLDefinitionOwnedLink *link =
        nl_definition_owned_link(owner, index);
    NLDefinitionCallResult result = {0};
    NLCheckStatus status =
        nl_definition_owned_validate(owner, index, link, &result);
    if (status != NL_CHECK_OK || !link ||
        !result.pre_verified || !result.return_old_verified ||
        !result.loan_closed || !result.old_known || !result.sibling_known ||
        result.replacements != 1 || link->arm > 1 ||
        result.old_value != o.old_value ||
        result.incoming != o.incoming[link->arm] ||
        result.sibling != o.sibling || link->field.index != o.index ||
        !link->field.parent || !link->field.child ||
        link->field.parent == link->other || !link->callee ||
        !link->caller || !link->parent || !link->world || !link->pre ||
        !link->post || !link->body || !link->loan || !link->formals[0] ||
        !link->formals[1]) {
        fprintf(stderr, "R919 invalid arm index=%zu status=%d old=%llu "
                        "incoming=%llu sibling=%llu\n",
                index, status, (unsigned long long)result.old_value,
                (unsigned long long)result.incoming,
                (unsigned long long)result.sibling);
        return false;
    }
    fprintf(stderr, "R919 REAL old=%u incoming=%u sibling=%u "
                    "arm=%zu field-index=%zu root=%zu child=%zu other=%zu\n",
            o.old_value, o.incoming[link->arm], o.sibling, link->arm,
            link->field.index, link->field.parent, link->field.child,
            link->other);
    return true;
}

static NLDefinitionOwned *make_owner(const char *path, Oracle o)
{
    Input in = {0};
    if (!input_open(&in, path)) {
        require(false, "parse/source/input setup");
        input_destroy(&in);
        return NULL;
    }
    NLDefinitionOwned *owner = construct(&in);
    input_destroy(&in);
    if (!owner)
        return NULL;
    require(nl_definition_owned_status(owner) == NL_CHECK_OK &&
                nl_definition_owned_count(owner) == 2,
            "two completed owned normal arm records after original teardown");
    unsigned arms = 0;
    for (size_t i = 0; i < nl_definition_owned_count(owner); ++i) {
        const NLDefinitionOwnedLink *l = nl_definition_owned_link(owner, i);
        if (l && l->arm < 2)
            arms |= 1u << l->arm;
        require(validate(owner, i, o), "post-original-lifetime replay");
    }
    require(arms == 3, "both independently checked normal worlds");
    return owner;
}

static void foreign_probe(NLDefinitionOwned *original, Oracle oracle,
                          NLDefinitionOwned *foreign)
{
    const NLDefinitionOwnedLink *a = nl_definition_owned_link(original, 0);
    const NLDefinitionOwnedLink *b = nl_definition_owned_link(foreign, 0);
    if (!a || !b) {
        require(false, "foreign owners unavailable");
        return;
    }
    require(a->call == b->call, "real graph numeric NodeId collision");
    require(a->callee != b->callee && a->caller != b->caller &&
                a->world != b->world &&
                nl_checked_source(a->callee) != nl_checked_source(b->callee),
            "two distinct genuine checked/source owner graphs");
    NLCheckedFragment *caller = (NLCheckedFragment *)(void *)a->caller;
    if (caller->body_count != 1 || caller->bodies[0] != a->callee) {
        require(false, "actual original callee edge missing");
        return;
    }
    require(validate(original, 0, oracle), "positive before cross-owner");
    NLCheckedFragment *saved = caller->bodies[0];
    caller->bodies[0] = (NLCheckedFragment *)(void *)b->callee;
    NLDefinitionCallResult rejected = {0};
    NLCheckStatus refusal = nl_definition_owned_validate(
        original, 0, a, &rejected);
    caller->bodies[0] = saved;
    require(refusal != NL_CHECK_OK, "actual foreign owned graph accepted");
    require(validate(original, 0, oracle), "positive after restoration");
    fprintf(stderr, "R919 foreign_actual_edge_result=%d NodeId=%zu\n",
            refusal, a->call);
}

static void graph_probe(NLDefinitionOwned *owner, Oracle oracle)
{
    const NLDefinitionOwnedLink *link = nl_definition_owned_link(owner, 0);
    if (!link || !link->callee) {
        require(false, "callee graph absent");
        return;
    }
    NLCheckedFragment *graph = (NLCheckedFragment *)(void *)link->callee;
    NLCheckedNodeId id = 0;
    for (size_t i = 1; i <= nl_checked_node_count(graph); ++i) {
        const NLCheckedNodeView *node = nl_checked_node_view(graph, i);
        if (node && node->kind == NL_CHECKED_RETURN) {
            id = i;
            break;
        }
    }
    if (!id) {
        require(false, "genuine callee return node absent");
        return;
    }
    require(validate(owner, 0, oracle), "positive before checked return");
    NLCheckedNodeView *node = &graph->nodes[id - 1];
    NLCheckedKind saved = node->kind;
    node->kind = NL_CHECKED_UNIT;
    NLDefinitionCallResult result = {0};
    NLCheckStatus refused = nl_definition_owned_validate(
        owner, 0, link, &result);
    node->kind = saved;
    require(refused != NL_CHECK_OK, "actual checked RETURN forgery accepted");
    require(validate(owner, 0, oracle), "positive after RETURN restoration");
    fprintf(stderr, "R919 true return corruption refused=%d node=%zu\n",
            refused, id);
}

static void wrong_return(const char *path)
{
    Input in = {0};
    if (!input_open(&in, path)) {
        require(false, "wrong-return parse");
        input_destroy(&in);
        return;
    }
    NLDefinitionOwned *owner = construct(&in);
    input_destroy(&in);
    if (!owner)
        return;
    bool accepted = false;
    for (size_t i = 0; i < nl_definition_owned_count(owner); ++i) {
        const NLDefinitionOwnedLink *v = nl_definition_owned_link(owner, i);
        NLDefinitionCallResult result = {0};
        if (nl_definition_owned_validate(owner, i, v, &result) == NL_CHECK_OK &&
            result.return_old_verified)
            accepted = true;
    }
    require(!accepted, "wrong source RETURN promoted to old-Copy");
    fprintf(stderr, "R919 wrong-return count=%zu accepted=%d\n",
            nl_definition_owned_count(owner), accepted);
    nl_definition_owned_destroy(owner);
}

static void absent_or_duplicate(const char *path)
{
    Input in = {0};
    if (!input_open(&in, path)) {
        require(false, "negative source parse");
        input_destroy(&in);
        return;
    }
    NLDefinitionOwned *owner = construct(&in);
    input_destroy(&in);
    if (!owner)
        return;
    require(nl_definition_owned_count(owner) == 0,
            "missing/duplicate CALL must not construct owned evidence");
    nl_definition_owned_destroy(owner);
}

int main(int argc, char **argv)
{
    if (argc != 9)
        return 2;
    Oracle harbor = {.old_value = 216, .incoming = {48, 117},
                     .sibling = 74, .index = 0};
    Oracle orchard = {.old_value = 27, .incoming = {189, 53},
                      .sibling = 90, .index = 1};
    NLDefinitionOwned *first = make_owner(argv[1], harbor);
    NLDefinitionOwned *second = make_owner(argv[2], orchard);
    /* A distinct registration context of the same corpus produces colliding
     * artifact-local numeric IDs but never identical graph ownership. */
    NLDefinitionOwned *foreign = make_owner(argv[1], harbor);
    if (first && foreign)
        foreign_probe(first, harbor, foreign);
    if (second)
        graph_probe(second, orchard);
    nl_definition_owned_destroy(foreign);
    nl_definition_owned_destroy(second);
    nl_definition_owned_destroy(first);
    for (int i = 3; i <= 6; ++i)
        wrong_return(argv[i]);
    absent_or_duplicate(argv[7]);
    absent_or_duplicate(argv[8]);
    fprintf(stderr, "R919 finite owner/hard-negative failures=%d\n", failures);
    return failures ? 1 : 0;
}
