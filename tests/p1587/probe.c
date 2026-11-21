#include "division_usize_internal.h"
#include "h_origin_internal.h"
#include "newlang/parser.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    NLSemanticContext *context;
    NLSource *entry_source;
    NLParser *entry_parser;
    NLSyntaxTree *entry;
    NLCheckedFragment *artifact;
} Program;

static void destroy_program(Program *p)
{
    nl_checked_destroy(p->artifact);
    nl_syntax_tree_destroy(p->entry);
    nl_parser_destroy(p->entry_parser);
    nl_source_destroy(p->entry_source);
    nl_semantic_destroy(p->context);
}

static bool prepare_program(Program *p, const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    const bool parsed =
        nl_source_load(path, &source) == NL_SOURCE_OK &&
        nl_parser_create(source, &parser) == NL_PARSE_OK &&
        nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK;
    bool registered = false;
    if (parsed && nl_semantic_create(&p->context) == NL_CHECK_OK) {
        const NLSyntaxTree *units[] = {unit};
        NLFunctionUnitDiagnostic diagnostic = {0};
        const NLCheckStatus status = nl_semantic_register_function_unit(
            p->context, units, 1, &diagnostic);
        if (status != NL_CHECK_OK)
            fprintf(stderr, "REGISTER status=%d diagnostic=%s\n", status,
                    diagnostic.diagnostic.diagnostic.code
                        ? diagnostic.diagnostic.diagnostic.code
                        : "(none)");
        registered = status == NL_CHECK_OK;
    }
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return registered &&
           nl_source_create("main()", 6, "p1587-entry", &p->entry_source) ==
               NL_SOURCE_OK &&
           nl_parser_create(p->entry_source, &p->entry_parser) == NL_PARSE_OK &&
           nl_parser_parse_expression_fragment(p->entry_parser, &p->entry,
                                               NULL) == NL_PARSE_OK;
}

#define REQUIRE(c)                                                             \
    do {                                                                       \
        if (!(c)) {                                                            \
            fprintf(stderr, "P1587 line %d: %s\n", __LINE__, #c);              \
            return false;                                                      \
        }                                                                      \
    } while (0)

typedef enum {
    NONE,
    JOIN,
    RESULT
} Fault;
static Fault fault;
static const NLSemanticContext *join_world;
static bool join_realloc;
static size_t malloc_at, allocations, hits, visits, result_arms;
static NLCheckStatus injected;

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
NLCheckStatus __real_nl_if_field_plan(NLCheckedFragment *, NLCheckedNodeId,
                                      const NLSemanticContext *, NLPlaceId *);
NLCheckStatus __real_nl_sem_new_value(NLSemanticContext *, NLSemanticValueView,
                                      NLValueId *);
NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);

void *__wrap_malloc(size_t size)
{
    if (malloc_at && ++allocations == malloc_at) {
        ++hits;
        return NULL;
    }
    return __real_malloc(size);
}

void *__wrap_realloc(void *ptr, size_t size)
{
    if (join_realloc && ++allocations == 1) {
        ++hits;
        return NULL;
    }
    return __real_realloc(ptr, size);
}

NLCheckStatus __wrap_nl_if_field_plan(NLCheckedFragment *f, NLCheckedNodeId id,
                                      const NLSemanticContext *c,
                                      NLPlaceId *place)
{
    const NLCheckStatus status = __real_nl_if_field_plan(f, id, c, place);
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    if (fault == JOIN && !visits && status == NL_CHECK_OK && !*place && n &&
        n->normal_arms == 2 && nl_checked_if_arm(f, id, 0) &&
        nl_checked_if_arm(f, id, 1)) {
        ++visits;
        join_world = c;
    }
    return status;
}

NLCheckStatus __wrap_nl_sem_new_value(NLSemanticContext *c,
                                      NLSemanticValueView v, NLValueId *out)
{
    const bool target = fault == JOIN && join_world == c &&
                        v.type == nl_semantic_core_type(c, NL_TYPE_USIZE) &&
                        !v.scalar_known;
    if (target) {
        join_world = NULL;
        join_realloc = true;
    }
    const NLCheckStatus status = __real_nl_sem_new_value(c, v, out);
    if (target) {
        injected = status;
        join_realloc = false;
    }
    return status;
}

NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *parent,
                                              NLCheckedFragment *child,
                                              NLCheckedNodeId call,
                                              const NLSemanticContext *world)
{
    const bool target = fault == RESULT && world && child->arm_count && !visits;
    if (target) {
        ++visits;
        result_arms = child->arm_count;
        malloc_at = 2;
    }
    const NLCheckStatus status =
        __real_nl_h_custody_seal_result(parent, child, call, world);
    if (target) {
        injected = status;
        malloc_at = 0;
    }
    return status;
}

static NLCheckStatus check_program(Program *p)
{
    NLCheckDiagnostic diagnostic = {0};
    const NLCheckStatus status = nl_semantic_check_expression(
        p->context, p->entry, &p->artifact, &diagnostic);
    if (status != NL_CHECK_OK)
        printf("CHECK status=%d code=%s\n", status,
               diagnostic.diagnostic.code ? diagnostic.diagnostic.code
                                          : "(none)");
    return status;
}

static bool scalar(const NLCheckedFragment *f, NLCheckedResult result,
                   NLSemanticValueView *value)
{
    return nl_semantic_value_view(f->context, result.value, value) &&
           value->type == result.type && !value->value_dependency_count &&
           value->dependencies == NL_DEPENDENCY_FREE &&
           (value->scalar_known || value->scalar_value == 0);
}

static bool walk(const NLCheckedFragment *, const NLCheckedFragment *, size_t);

static bool observe_if(const NLCheckedFragment *f,
                       const NLCheckedFragment *main, NLCheckedNodeId id,
                       size_t depth)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    const NLCheckedNodeView *cond = nl_checked_node_view(f, n->initializer);
    NLSemanticValueView joined, test;
    REQUIRE(n->normal_arms == 2 && n->result_count == 1 &&
            scalar(f, n->results[0], &joined) && !joined.scalar_known &&
            joined.type == nl_semantic_core_type(f->context, NL_TYPE_USIZE) &&
            cond && cond->result_count == 1 &&
            scalar(f, cond->results[0], &test) &&
            test.type == nl_semantic_core_type(f->context, NL_TYPE_BOOL));
    printf("IF %zu %d %zu\n", id, test.scalar_known, test.scalar_value);
    size_t numbers[2];
    const NLCheckedFragment *children[2];
    for (size_t arm_index = 0; arm_index < 2; ++arm_index) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, id, arm_index);
        REQUIRE(arm && arm->if_parent == f && arm->if_parent_node == id &&
                arm->if_arm_index == arm_index && arm->context != f->context &&
                arm->branch_value_prefix == cond->results[0].value);
        const NLCheckedNodeView *root = nl_checked_node_view(arm, arm->root);
        const NLCheckedNodeView *block =
            root ? nl_checked_node_view(arm, root->initializer) : NULL;
        const NLCheckedNodeView *tail =
            block ? nl_checked_node_view(arm, block->tail) : NULL;
        NLSemanticValueView value;
        REQUIRE(root && root->kind == NL_CHECKED_IF_ARM &&
                root->result_count == 1 && !root->terminates && tail &&
                tail->results[0].value == block->results[0].value &&
                tail->results[0].value == root->results[0].value &&
                scalar(arm, root->results[0], &value) &&
                value.type == joined.type && value.scalar_known);
        children[arm_index] = arm;
        numbers[arm_index] = value.scalar_value;
    }
    REQUIRE(children[0]->context != children[1]->context);
    printf("FORK %zu %zu\n", numbers[0], numbers[1]);
    return walk(children[0], main, depth + 1) &&
           walk(children[1], main, depth + 1);
}

static bool observe_arithmetic(const NLCheckedFragment *f, NLCheckedNodeId id,
                               const NLCheckedNodeView *node)
{
    NLSemanticValueView value;
    REQUIRE(scalar(f, node->results[0], &value));
    if (node->kind == NL_CHECKED_USIZE_DIVISION) {
        NLUsizeDivisionView v;
        REQUIRE(nl_checked_usize_division_view(f, id, &v) &&
                nl_checked_usize_division_valid(f, id, &v) && v.origin == f &&
                v.numbers[1] && v.quotient_known == value.scalar_known &&
                v.numerator_known == value.scalar_known &&
                v.quotient == value.scalar_value);
    } else {
        NLUsizeRemainderView v;
        REQUIRE(nl_checked_usize_remainder_view(f, id, &v) &&
                nl_checked_usize_remainder_valid(f, id, &v) && v.origin == f &&
                v.numbers[1] && v.remainder_known == value.scalar_known &&
                v.numerator_known == value.scalar_known &&
                v.remainder == value.scalar_value);
    }
    printf("ARITH %c %d %zu\n",
           node->kind == NL_CHECKED_USIZE_DIVISION ? '/' : '%',
           value.scalar_known, value.scalar_value);
    return true;
}

static bool walk(const NLCheckedFragment *f, const NLCheckedFragment *main,
                 size_t depth)
{
    REQUIRE(f && depth < NL_SEMANTIC_MAX_DEPTH);
    const NLSemanticContext *c = f->context;
    for (NLCheckedNodeId id = 1; id <= f->count; ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        switch (n->kind) {
        case NL_CHECKED_REGISTERED_CALL: {
            const NLCheckedFragment *body = nl_checked_call_body(f, id);
            NLDivisionUsizePlan plan;
            REQUIRE(body && n->function && n->function <= c->function_count &&
                    nl_quantity_usize_call_valid(f, id, &plan) &&
                    plan.body == body && plan.function == n->function);
            printf("FUNCTION %zu %s\n", n->function,
                   c->functions[n->function - 1].name);
            REQUIRE(walk(body, main, depth + 1));
            break;
        }
        case NL_CHECKED_IF:
            REQUIRE(observe_if(f, main, id, depth));
            break;
        case NL_CHECKED_BINDING:
            if (f == main)
                printf("BIND %zu %s\n", n->symbol,
                       c->bindings[n->symbol - 1].name);
            break;
        case NL_CHECKED_USIZE_DIVISION:
        case NL_CHECKED_USIZE_REMAINDER:
            REQUIRE(observe_arithmetic(f, id, n));
            break;
        default:
            break;
        }
    }
    return true;
}

static bool observe(const Program *p)
{
    const NLCheckedFragment *entry = p->artifact;
    const NLCheckedNodeId call = nl_checked_root(entry);
    REQUIRE(nl_checked_proof_class(entry) == NL_CHECKED_MACHINE_PROVED &&
            nl_h_custody_result_world_present(entry, call));
    return walk(entry, nl_checked_call_body(entry, call), 0);
}

static bool refuses(const NLCheckedFragment *f)
{
    char sentinel[] = "keep this buffer";
    char *out = sentinel;
    size_t length = 130;
    const NLCheckStatus status = nl_quantity_usize_c17(f, &out, &length);
    if (status == NL_CHECK_OK)
        free(out);
    return status == NL_CHECK_SEMANTIC_UNSUPPORTED && out == sentinel &&
           length == 130 && !strcmp(sentinel, "keep this buffer");
}

static bool restored(const NLCheckedFragment *f)
{
    char *out = NULL;
    size_t length = 0;
    const bool ok =
        nl_quantity_usize_c17(f, &out, &length) == NL_CHECK_OK && out && length;
    free(out);
    return ok;
}

static bool identity(Program *p)
{
    NLCheckedFragment *entry = p->artifact;
    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(
        entry, nl_checked_root(entry));
    REQUIRE(body && body->arm_count == 4 && refuses(body));
    NLCheckedFragment *first = body->arms[1].artifact;
    NLCheckedFragment *repeated = body->arms[3].artifact;
    REQUIRE(first != repeated && first->context != repeated->context &&
            first->body_count == 1 && repeated->body_count == 1);
    const NLCheckedNodeView *call =
        nl_checked_node_view(first, first->body_calls[0]);
    const NLCheckedNodeView *other =
        nl_checked_node_view(repeated, repeated->body_calls[0]);
    NLSemanticValueView before, after;
    REQUIRE(call && other && call->function == other->function &&
            scalar(first, call->results[0], &before) &&
            scalar(repeated, other->results[0], &after) &&
            before.scalar_known && after.scalar_known &&
            before.scalar_value == after.scalar_value);
    body->arms[1].artifact = repeated;
    const bool foreign_arm = refuses(entry);
    body->arms[1].artifact = first;

    NLCheckedFragment *callee = first->bodies[0];
    first->bodies[0] = repeated->bodies[0];
    const bool foreign_call = refuses(entry);
    first->bodies[0] = callee;

    NLCheckedNodeView *actual = &first->nodes[call->first_argument - 1];
    const NLValueId copy = actual->results[0].value;
    const NLValueId source =
        first->context->bindings[actual->symbol - 1].view.value;
    REQUIRE(source != copy && scalar(first, actual->results[0], &before) &&
            scalar(first, (NLCheckedResult){actual->type, source}, &after) &&
            before.scalar_known && after.scalar_known &&
            before.scalar_value == after.scalar_value);
    actual->results[0].value = source;
    const bool replaced_copy = refuses(entry);
    actual->results[0].value = copy;

    NLCheckedNodeView *joined = &body->nodes[body->arms[0].match - 1];
    const NLValueId original = joined->results[0].value;
    const NLValueId substitute =
        body->nodes[body->arms[2].match - 1].results[0].value;
    REQUIRE(original != substitute &&
            scalar(body, joined->results[0], &before) &&
            scalar(body, (NLCheckedResult){joined->type, substitute}, &after) &&
            !before.scalar_known && !after.scalar_known);
    joined->results[0].value = substitute;
    const bool replaced_result = refuses(entry);
    joined->results[0].value = original;

    REQUIRE(foreign_arm && foreign_call && replaced_copy && replaced_result &&
            restored(entry));
    puts("IDENTITY same-valued arm/callee/Copy and foreign Unknown join "
         "rejected; restore PASS");
    return true;
}

static bool return_identity(Program *p)
{
    NLCheckedFragment *entry = p->artifact;
    const NLCheckedFragment *main =
        nl_checked_call_body(entry, nl_checked_root(entry));
    REQUIRE(main && main->body_count == 2);
    NLCheckedFragment *first = main->bodies[0];
    const NLCheckedFragment *repeated = main->bodies[1];
    NLControlExits *original = first->exits;
    REQUIRE(original && repeated->exits && original != repeated->exits);
    first->exits = repeated->exits;
    const bool rejected = refuses(entry);
    first->exits = original;
    REQUIRE(rejected && restored(entry));
    puts("IDENTITY foreign actual RETURN rejected; restore PASS");
    return true;
}

static bool check_oom(Program *p, Fault site)
{
    NLSemanticContext *c = p->context;
    NLSymbolId anchor = 0;
    REQUIRE(
        nl_semantic_seed_scalar(
            c, "p1587_rollback_anchor",
            (NLScalarValue){nl_semantic_core_type(c, NL_TYPE_USIZE), true, 67},
            &anchor) == NL_CHECK_OK &&
        anchor && !p->artifact);
    const NLSemanticBindingView prior = c->bindings[anchor - 1].view;
    REQUIRE(prior.place && prior.value);
    unsigned char header[sizeof *c], binding[sizeof *c->bindings],
        place[sizeof *c->places], value[sizeof *c->values];
    memcpy(header, c, sizeof header);
    memcpy(binding, &c->bindings[anchor - 1], sizeof binding);
    memcpy(place, &c->places[prior.place - 1], sizeof place);
    memcpy(value, &c->values[prior.value - 1], sizeof value);
    fault = site;
    const NLCheckStatus status = check_program(p);
    fault = NONE;
    join_world = NULL;
    printf(
        "OOM site=%s status=%d allocation=%zu hits=%zu visits=%zu arms=%zu\n",
        site == JOIN ? "join" : "result-world", status, allocations, hits,
        visits, result_arms);
    REQUIRE(status == NL_CHECK_OUT_OF_MEMORY && injected == status &&
            !p->artifact && visits == 1 && hits == 1 &&
            allocations == (site == JOIN ? 1u : 2u) &&
            (site == JOIN || result_arms == 4) &&
            !memcmp(header, c, sizeof header) &&
            !memcmp(binding, &c->bindings[anchor - 1], sizeof binding) &&
            !memcmp(place, &c->places[prior.place - 1], sizeof place) &&
            !memcmp(value, &c->values[prior.value - 1], sizeof value) &&
            nl_semantic_find_binding(c, "p1587_rollback_anchor") == anchor &&
            nl_sem_validate(c) == NL_CHECK_OK);
    REQUIRE(check_program(p) == NL_CHECK_OK && observe(p) &&
            restored(p->artifact));
    puts("OOM prior binding/value/place unchanged; normal retry PASS");
    return true;
}

static bool staging_oom(Program *p)
{
    char *clean = NULL;
    size_t clean_size = 0;
    REQUIRE(nl_quantity_usize_c17(p->artifact, &clean, &clean_size) ==
                NL_CHECK_OK &&
            clean && clean_size);
    char sentinel[] = "keep staging output";
    char *out = sentinel;
    size_t length = 142;
    malloc_at = 1;
    const NLCheckStatus status =
        nl_quantity_usize_c17(p->artifact, &out, &length);
    malloc_at = 0;
    const bool held = status == NL_CHECK_OUT_OF_MEMORY && hits == 1 &&
                      allocations == 1 && out == sentinel && length == 142 &&
                      !strcmp(sentinel, "keep staging output");
    if (status == NL_CHECK_OK)
        free(out);
    out = NULL;
    length = 0;
    const NLCheckStatus retry =
        nl_quantity_usize_c17(p->artifact, &out, &length);
    const bool same = retry == NL_CHECK_OK && out && length == clean_size &&
                      !memcmp(clean, out, length);
    free(out);
    free(clean);
    printf("OOM site=staging status=%d allocation=%zu hits=%zu retry=%d\n",
           status, allocations, hits, retry);
    REQUIRE(held && same);
    puts("OOM staged pointer/length/bytes unchanged; byte-equal retry PASS");
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    Program p = {0};
    bool good = prepare_program(&p, argv[2]);
    if (good) {
        const char *mode = argv[1];
        if (!strcmp(mode, "join-oom") || !strcmp(mode, "result-oom"))
            good = check_oom(&p, mode[0] == 'j' ? JOIN : RESULT);
        else if (check_program(&p) != NL_CHECK_OK)
            good = false;
        else if (!strcmp(mode, "observe"))
            good = observe(&p);
        else if (!strcmp(mode, "identity"))
            good = identity(&p);
        else if (!strcmp(mode, "return-identity"))
            good = return_identity(&p);
        else if (!strcmp(mode, "staging-oom"))
            good = staging_oom(&p);
        else
            good = false;
    }
    destroy_program(&p);
    return good ? 0 : 1;
}
