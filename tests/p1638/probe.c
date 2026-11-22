#include "division_usize_internal.h"
#include "h_origin_internal.h"
#include "newlang/parser.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(condition)                                                     \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "line %d: %s\n", __LINE__, #condition);            \
            return false;                                                      \
        }                                                                      \
    } while (0)

typedef struct {
    NLSource *source;
    NLParser *parser;
    NLSyntaxTree *expression;
    NLSemanticContext *context;
    NLCheckedFragment *entry;
} Program;

typedef struct {
    NLCheckedFragment *main_body, *helper, *return_arm, *normal_arm;
    NLCheckedNodeId conditional, returned, return_operation, normal_operation;
} Evidence;

static bool prepare(Program *p, const char *file)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    bool ok =
        nl_source_load(file, &source) == NL_SOURCE_OK &&
        nl_parser_create(source, &parser) == NL_PARSE_OK &&
        nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK &&
        nl_semantic_create(&p->context) == NL_CHECK_OK;
    if (ok) {
        const NLSyntaxTree *units[] = {unit};
        ok = nl_semantic_register_function_unit(p->context, units, 1, NULL) ==
             NL_CHECK_OK;
    }
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return ok &&
           nl_source_create("main()", 6, "p1638-entry", &p->source) ==
               NL_SOURCE_OK &&
           nl_parser_create(p->source, &p->parser) == NL_PARSE_OK &&
           nl_parser_parse_expression_fragment(p->parser, &p->expression,
                                               NULL) == NL_PARSE_OK;
}

static NLCheckStatus check(Program *p)
{
    NLCheckDiagnostic diagnostic = {0};
    const NLCheckStatus status = nl_semantic_check_expression(
        p->context, p->expression, &p->entry, &diagnostic);
    printf("CHECK %d %s\n", status,
           diagnostic.diagnostic.code ? diagnostic.diagnostic.code : "-");
    return status;
}

static void destroy(Program *p)
{
    nl_checked_destroy(p->entry);
    nl_syntax_tree_destroy(p->expression);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    nl_semantic_destroy(p->context);
}

static NLCheckedNodeId find_kind(const NLCheckedFragment *f, NLCheckedKind kind,
                                 size_t *count)
{
    NLCheckedNodeId found = 0;
    *count = 0;
    for (NLCheckedNodeId id = 1; id <= f->count; ++id) {
        const NLCheckedNodeView *node = nl_checked_node_view(f, id);
        if (node && node->kind == kind) {
            found = id;
            ++*count;
        }
    }
    return found;
}

static bool inspect_call(NLCheckedFragment *parent, NLCheckedNodeId site,
                         size_t ordinal, Evidence *saved)
{
    const NLCheckedNodeView *call = nl_checked_node_view(parent, site);
    NLCheckedFragment *body = parent->bodies[ordinal];
    NLDivisionUsizePlan plan;
    REQUIRE(call && body && parent->body_calls[ordinal] == site &&
            nl_h_custody_result_valid(parent, site) &&
            nl_quantity_usize_call_valid(parent, site, &plan) &&
            plan.body == body && plan.formal && plan.second_formal &&
            plan.result && plan.joined_result == body->root &&
            call->kind == NL_CHECKED_REGISTERED_CALL && call->body_backed &&
            call->argument_count == 2 && body->arm_count == 2 &&
            nl_control_exits_count(body->exits) == 1);

    size_t count = 0;
    const NLCheckedNodeId conditional = find_kind(body, NL_CHECKED_IF, &count);
    REQUIRE(count == 1 && conditional);
    const NLCheckedNodeView *choice = nl_checked_node_view(body, conditional);
    REQUIRE(choice->normal_arms == 1 && choice->tail &&
            choice->result_count == 1 && !choice->terminates);
    NLCheckedFragment *arms[] = {
        (NLCheckedFragment *)nl_checked_if_arm(body, conditional, 0),
        (NLCheckedFragment *)nl_checked_if_arm(body, conditional, 1)};
    REQUIRE(arms[0] && arms[1] && arms[0]->context != arms[1]->context);
    const NLCheckedNodeView *roots[] = {
        nl_checked_node_view(arms[0], arms[0]->root),
        nl_checked_node_view(arms[1], arms[1]->root)};
    const size_t returned_index = roots[0]->terminates ? 0 : 1;
    const size_t normal_index = returned_index ^ 1u;
    NLCheckedFragment *return_arm = arms[returned_index];
    NLCheckedFragment *normal_arm = arms[normal_index];
    REQUIRE(roots[returned_index]->terminates &&
            !roots[normal_index]->terminates &&
            nl_control_exits_count(return_arm->exits) == 1 &&
            !nl_control_exits_count(normal_arm->exits));

    size_t returns = 0, divisions = 0, remainders = 0;
    const NLCheckedNodeId returned =
        find_kind(return_arm, NL_CHECKED_RETURN, &returns);
    const NLCheckedNodeId division =
        find_kind(return_arm, NL_CHECKED_USIZE_DIVISION, &divisions);
    const NLCheckedNodeId remainder =
        find_kind(normal_arm, NL_CHECKED_USIZE_REMAINDER, &remainders);
    REQUIRE(returns == 1 && divisions == 1 && remainders == 1 && returned &&
            division && remainder);
    const NLCheckedNodeView *return_node =
        nl_checked_node_view(return_arm, returned);
    const NLCheckedNodeView *divide =
        nl_checked_node_view(return_arm, division);
    const NLCheckedNodeView *modulo =
        nl_checked_node_view(normal_arm, remainder);
    REQUIRE(return_node->initializer && return_node->terminates &&
            divide->has_scalar_result && divide->scalar_result.known &&
            divide->scalar_result.value == 12 && modulo->has_scalar_result &&
            modulo->scalar_result.known && modulo->scalar_result.value == 5);

    size_t function = 0;
    const NLControlExitView *edge = nl_control_exit_view(return_arm->exits, 0);
    const NLControlExitView *outer = nl_control_exit_view(body->exits, 0);
    REQUIRE(nl_h_custody_if_body(return_arm, NULL, &function) == body &&
            function == call->function && edge && outer && edge->state &&
            outer->state && edge->kind == NL_EXIT_RETURN &&
            outer->kind == NL_EXIT_RETURN && edge->count == 1 &&
            outer->count == 1 &&
            nl_control_target_same(edge->target, body->function_target) &&
            nl_control_target_same(outer->target, body->function_target) &&
            return_arm->source == body->source &&
            normal_arm->source == body->source);

    /* This is a private quantity profile. Existing public arm arithmetic
     * validators remain withheld for a one-normal Return conditional. */
    REQUIRE(!nl_checked_usize_division_valid(return_arm, division, NULL) &&
            !nl_checked_usize_remainder_valid(normal_arm, remainder, NULL));

    const NLCheckedNodeView *normal = nl_checked_node_view(body, plan.result);
    const NLCheckedNodeView *joined =
        nl_checked_node_view(body, plan.joined_result);
    NLSemanticValueView normal_value, joined_value;
    REQUIRE(normal && joined && normal->result_count == 1 &&
            joined->result_count == 1 &&
            normal->results[0].value + 1 == joined->results[0].value &&
            nl_semantic_value_view(body->context, normal->results[0].value,
                                   &normal_value) &&
            nl_semantic_value_view(body->context, joined->results[0].value,
                                   &joined_value) &&
            normal_value.scalar_known && normal_value.scalar_value == 5 &&
            !joined_value.scalar_known && !joined_value.scalar_value &&
            normal_value.carrier == NL_CARRIER_ENDED &&
            joined_value.carrier == NL_CARRIER_ENDED);

    const NLCheckedNodeView *first =
        nl_checked_node_view(parent, call->first_argument);
    const NLCheckedNodeView *flag =
        first ? nl_checked_node_view(parent, first->next_argument) : NULL;
    NLSemanticValueView actual;
    REQUIRE(first && flag &&
            flag->parameter_type ==
                nl_semantic_core_type(parent->context, NL_TYPE_BOOL) &&
            nl_semantic_value_view(parent->context, flag->results[0].value,
                                   &actual) &&
            actual.scalar_known && actual.scalar_value == (ordinal == 0));
    printf("CALL %zu flag=%zu return=%zu normal=%zu joined=Unknown "
           "public-arm=withheld\n",
           ordinal, actual.scalar_value, divide->scalar_result.value,
           modulo->scalar_result.value);

    if (saved) {
        *saved = (Evidence){.helper = body,
                            .return_arm = return_arm,
                            .normal_arm = normal_arm,
                            .conditional = conditional,
                            .returned = returned,
                            .return_operation = division,
                            .normal_operation = remainder};
    }
    return true;
}

static bool inspect(Program *p, Evidence *saved)
{
    const NLCheckedNodeId site = nl_checked_root(p->entry);
    const NLCheckedNodeView *call = nl_checked_node_view(p->entry, site);
    NLCheckedFragment *main_body = p->entry->bodies[0];
    NLDivisionUsizePlan main;
    REQUIRE(call && main_body && p->entry->body_count == 1 &&
            nl_checked_proof_class(p->entry) == NL_CHECKED_MACHINE_PROVED &&
            nl_h_custody_validate(p->entry) &&
            nl_h_custody_result_valid(p->entry, site) &&
            nl_h_custody_result_world_present(p->entry, site) &&
            nl_quantity_usize_call_valid(p->entry, site, &main) &&
            main.body == main_body && !main.formal && !main.second_formal &&
            !main.result && !main.joined_result && main_body->body_count == 2);
    Evidence first = {.main_body = main_body};
    REQUIRE(inspect_call(main_body, main_body->body_calls[0], 0, &first) &&
            inspect_call(main_body, main_body->body_calls[1], 1, NULL));
    first.main_body = main_body;
    if (saved)
        *saved = first;
    puts("EVIDENCE canonical BODY/ARM/Return target and normal replay PASS");
    return true;
}

static bool emit(const Program *p, char **output, size_t *length)
{
    return nl_quantity_usize_c17(p->entry, output, length) == NL_CHECK_OK &&
           *output && *length;
}

static bool withheld(const Program *p)
{
    char sentinel[] = "keep\0return";
    char original[sizeof(sentinel)];
    memcpy(original, sentinel, sizeof(sentinel));
    char *output = sentinel;
    size_t length = sizeof(sentinel);
    const NLCheckStatus status =
        nl_quantity_usize_c17(p->entry, &output, &length);
    const bool held = status == NL_CHECK_SEMANTIC_UNSUPPORTED &&
                      output == sentinel && length == sizeof(sentinel) &&
                      !memcmp(sentinel, original, sizeof(sentinel));
    if (output != sentinel)
        free(output);
    return held;
}

static bool rejected(const Program *p)
{
    NLDivisionUsizePlan unused;
    return !nl_quantity_usize_call_valid(p->entry, nl_checked_root(p->entry),
                                         &unused) &&
           withheld(p);
}

static bool identity(Program *p)
{
    Evidence e;
    REQUIRE(check(p) == NL_CHECK_OK && inspect(p, &e));
    char *pristine = NULL;
    size_t size = 0;
    REQUIRE(emit(p, &pristine, &size));

    const NLCheckedFragment *if_parent = e.return_arm->if_parent;
    e.return_arm->if_parent = e.normal_arm;
    const bool other_parent = rejected(p);
    e.return_arm->if_parent = if_parent;

    const size_t arm_index = e.return_arm->if_arm_index;
    e.return_arm->if_arm_index = e.normal_arm->if_arm_index;
    const bool other_arm = rejected(p);
    e.return_arm->if_arm_index = arm_index;

    const NLSource *source = e.return_arm->source;
    e.return_arm->source = p->source;
    const bool other_source = rejected(p);
    e.return_arm->source = source;

    const NLSemanticContext *world = e.return_arm->context;
    e.return_arm->context = e.normal_arm->context;
    const bool other_world = rejected(p);
    e.return_arm->context = world;

    const bool other_target =
        e.main_body->function_target &&
        !nl_control_exits_only_return(e.return_arm->exits,
                                      e.main_body->function_target);

    NLCheckedNodeView *conditional = &e.helper->nodes[e.conditional - 1];
    const NLCheckedNodeId replay = conditional->tail;
    conditional->tail = 0;
    const bool missing_replay = rejected(p);
    conditional->tail = replay;

    NLCheckedNodeView *returned = &e.return_arm->nodes[e.returned - 1];
    const NLCheckedKind kind = returned->kind;
    returned->kind = NL_CHECKED_STATEMENT;
    const bool fake_return = rejected(p);
    returned->kind = kind;

    char *retry = NULL;
    size_t retry_size = 0;
    const bool restored = inspect(p, NULL) && emit(p, &retry, &retry_size) &&
                          retry_size == size && !memcmp(retry, pristine, size);
    free(retry);
    free(pristine);
    printf("NEGATIVE parent=%d arm=%d source=%d world=%d target=%d "
           "replay=%d return=%d no-clobber=1 retry=%d\n",
           other_parent, other_arm, other_source, other_world, other_target,
           missing_replay, fake_return, restored);
    return other_parent && other_arm && other_source && other_world &&
           other_target && missing_replay && fake_return && restored;
}

static bool inject_seal;
static size_t fail_after, visits, hits, bytes;
static bool reached;

void *__real_malloc(size_t);
void *__wrap_malloc(size_t size)
{
    if (!fail_after)
        return __real_malloc(size);
    ++visits;
    if (--fail_after)
        return __real_malloc(size);
    ++hits;
    bytes = size;
    return NULL;
}

NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *parent,
                                              NLCheckedFragment *child,
                                              NLCheckedNodeId id,
                                              const NLSemanticContext *world)
{
    const NLCheckedNodeView *call = nl_checked_node_view(parent, id);
    if (inject_seal && !reached && !world && call &&
        call->argument_count == 2 && child->arm_count == 2) {
        reached =
            !nl_h_custody_result_valid(parent, id) && call->result_count == 1;
        fail_after = 1;
    }
    const NLCheckStatus status =
        __real_nl_h_custody_seal_result(parent, child, id, world);
    fail_after = 0;
    return status;
}

static bool same_emission(const Program *p)
{
    char *first = NULL, *second = NULL;
    size_t first_size = 0, second_size = 0;
    const bool same =
        emit(p, &first, &first_size) && emit(p, &second, &second_size) &&
        first_size == second_size && !memcmp(first, second, first_size);
    free(second);
    free(first);
    return same;
}

static bool result_oom(Program *p)
{
    NLSymbolId anchor = 0;
    const NLScalarValue seed = {
        nl_semantic_core_type(p->context, NL_TYPE_USIZE), true, 1638};
    REQUIRE(nl_semantic_seed_scalar(p->context, "p1638_anchor", seed,
                                    &anchor) == NL_CHECK_OK &&
            anchor);
    const NLSemanticBindingView prior = p->context->bindings[anchor - 1].view;
    unsigned char header[sizeof(*p->context)],
        binding[sizeof(*p->context->bindings)],
        place[sizeof(*p->context->places)], value[sizeof(*p->context->values)];
    memcpy(header, p->context, sizeof(header));
    memcpy(binding, &p->context->bindings[anchor - 1], sizeof(binding));
    memcpy(place, &p->context->places[prior.place - 1], sizeof(place));
    memcpy(value, &p->context->values[prior.value - 1], sizeof(value));

    inject_seal = true;
    const NLCheckStatus status = check(p);
    inject_seal = false;
    REQUIRE(
        status == NL_CHECK_OUT_OF_MEMORY && reached && visits == 1 &&
        hits == 1 && bytes && !p->entry &&
        !memcmp(header, p->context, sizeof(header)) &&
        !memcmp(binding, &p->context->bindings[anchor - 1], sizeof(binding)) &&
        !memcmp(place, &p->context->places[prior.place - 1], sizeof(place)) &&
        !memcmp(value, &p->context->values[prior.value - 1], sizeof(value)) &&
        nl_sem_validate(p->context) == NL_CHECK_OK);
    REQUIRE(check(p) == NL_CHECK_OK && inspect(p, NULL) && same_emission(p));
    printf("OOM result-seal hit=%zu bytes=%zu rollback=1 retry=1\n", hits,
           bytes);
    return true;
}

static bool staging_oom(Program *p)
{
    REQUIRE(check(p) == NL_CHECK_OK && inspect(p, NULL));
    char *pristine = NULL;
    size_t pristine_size = 0;
    REQUIRE(emit(p, &pristine, &pristine_size));
    char sentinel[] = "keep staging output";
    char *output = sentinel;
    size_t length = 1638;
    visits = hits = bytes = 0;
    fail_after = 1;
    const NLCheckStatus status =
        nl_quantity_usize_c17(p->entry, &output, &length);
    fail_after = 0;
    const bool held = status == NL_CHECK_OUT_OF_MEMORY && visits == 1 &&
                      hits == 1 && bytes > pristine_size &&
                      output == sentinel && length == 1638 &&
                      !strcmp(sentinel, "keep staging output");
    if (output != sentinel)
        free(output);
    char *retry = NULL;
    size_t retry_size = 0;
    const bool restored = emit(p, &retry, &retry_size) &&
                          retry_size == pristine_size &&
                          !memcmp(retry, pristine, pristine_size);
    free(retry);
    free(pristine);
    printf("OOM staging hit=%zu bytes=%zu no-clobber=%d retry=%d\n", hits,
           bytes, held, restored);
    return held && restored;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    Program p = {0};
    bool ok = prepare(&p, argv[2]);
    if (ok && !strcmp(argv[1], "identity"))
        ok = identity(&p);
    else if (ok && !strcmp(argv[1], "result-oom"))
        ok = result_oom(&p);
    else if (ok && !strcmp(argv[1], "staging-oom"))
        ok = staging_oom(&p);
    else
        ok = false;
    destroy(&p);
    return ok ? 0 : 1;
}
