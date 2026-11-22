#include "division_usize_internal.h"
#include "h_origin_internal.h"
#include "newlang/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(condition)                                                     \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "p1654:%d: %s\n", __LINE__, #condition);           \
            return false;                                                      \
        }                                                                      \
    } while (0)

typedef enum {
    NO_FAULT,
    HELPER_SEAL,
    ENTRY_WORLD
} Fault;
static Fault fault;
static size_t allocation, visits, hits, failed_bytes;

void *__real_malloc(size_t);
void *__wrap_malloc(size_t size)
{
    if (!allocation || --allocation)
        return __real_malloc(size);
    ++hits;
    failed_bytes = size;
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
    const NLCheckedNodeView *actual =
        call ? nl_checked_node_view(parent, call->first_argument) : NULL;
    if (fault == HELPER_SEAL && !world && call && call->argument_count == 1 &&
        actual && actual->kind == NL_CHECKED_IDENTIFIER &&
        actual->parameter_type ==
            nl_semantic_core_type(nl_checked_context(parent), NL_TYPE_BOOL)) {
        ++visits;
        allocation = 1;
    } else if (fault == ENTRY_WORLD && world && call && !call->argument_count) {
        ++visits;
        /* The entry result stores the node snapshot before the owned world. */
        allocation = 2;
    }
    const NLCheckStatus status =
        __real_nl_h_custody_seal_result(parent, child, id, world);
    allocation = 0;
    return status;
}

static bool custody(const NLCheckedFragment *entry)
{
    REQUIRE(nl_checked_proof_class(entry) == NL_CHECKED_MACHINE_PROVED);
    NLDivisionUsizePlan main;
    const NLCheckedNodeId entry_call = nl_checked_root(entry);
    REQUIRE(nl_quantity_usize_call_valid(entry, entry_call, &main) &&
            nl_checked_call_body(entry, entry_call) == main.body &&
            nl_h_custody_result_world_present(entry, entry_call));
    const NLSemanticContext *world = nl_checked_context(main.body);
    const NLTypeId boolean = nl_semantic_core_type(world, NL_TYPE_BOOL);
    const NLCheckedFragment *first = NULL;
    NLValueId previous_actual = 0, previous_result = 0;
    size_t seen = 0;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(main.body); ++id) {
        const NLCheckedNodeView *call = nl_checked_node_view(main.body, id);
        if (call->kind != NL_CHECKED_REGISTERED_CALL)
            continue;
        NLDivisionUsizePlan plan;
        REQUIRE(seen < 2 && call->argument_count == 1 &&
                nl_quantity_usize_call_valid(main.body, id, &plan) &&
                nl_h_custody_result_valid(main.body, id) &&
                nl_checked_call_body(main.body, id) == plan.body &&
                nl_checked_context(plan.body) == world && plan.formal &&
                !plan.second_formal && !plan.division && first != plan.body);
        NLCheckedNodeId resolved = 0;
        REQUIRE(nl_h_custody_body_caller(plan.body, world, &resolved) ==
                    main.body &&
                resolved == id);
        const NLCheckedNodeView *actual =
            nl_checked_node_view(main.body, call->first_argument);
        NLSemanticBindingView formal, original;
        NLSemanticValueView value;
        REQUIRE(
            actual && actual->kind == NL_CHECKED_IDENTIFIER &&
            actual->value_use == NL_VALUE_COPIED && actual->result_count == 1 &&
            actual->parameter_type == boolean && !actual->next_argument &&
            nl_semantic_binding_view(world, actual->symbol, &original) &&
            nl_semantic_binding_view(world, plan.formal, &formal) &&
            formal.type == boolean &&
            formal.value == actual->results[0].value &&
            formal.value > previous_actual && formal.value != original.value &&
            formal.place != original.place &&
            nl_semantic_value_view(world, formal.value, &value) &&
            value.scalar_known && value.scalar_value == (seen == 0));
        const NLCheckedNodeView *body =
            nl_checked_node_view(plan.body, nl_checked_root(plan.body));
        REQUIRE(body && body->result_count == 1 && call->result_count == 1 &&
                body->results[0].value == call->results[0].value &&
                call->results[0].value > previous_result);
        first = plan.body;
        previous_actual = formal.value;
        previous_result = call->results[0].value;
        ++seen;
    }
    REQUIRE(seen == 2);
    puts("CUSTODY two ordered actual Copy slots, distinct fresh formals and "
         "BODY/results");
    return true;
}

static bool rollback(NLSemanticContext *c, const NLSyntaxTree *expression,
                     NLCheckedFragment **entry)
{
    NLSymbolId symbol = 0;
    const NLScalarValue seed = {nl_semantic_core_type(c, NL_TYPE_USIZE), true,
                                1654};
    REQUIRE(nl_semantic_seed_scalar(c, "p1654_anchor", seed, &symbol) ==
                NL_CHECK_OK &&
            symbol);
    const NLSemanticBindingView b = c->bindings[symbol - 1].view;
    unsigned char context[sizeof(*c)], binding[sizeof(*c->bindings)],
        place[sizeof(*c->places)], value[sizeof(*c->values)];
    memcpy(context, c, sizeof(context));
    memcpy(binding, &c->bindings[symbol - 1], sizeof(binding));
    memcpy(place, &c->places[b.place - 1], sizeof(place));
    memcpy(value, &c->values[b.value - 1], sizeof(value));
    for (Fault site = HELPER_SEAL; site <= ENTRY_WORLD; ++site) {
        fault = site;
        visits = hits = failed_bytes = 0;
        const NLCheckStatus status =
            nl_semantic_check_expression(c, expression, entry, NULL);
        fault = NO_FAULT;
        REQUIRE(status == NL_CHECK_OUT_OF_MEMORY && !*entry && visits == 1 &&
                hits == 1 && failed_bytes &&
                !memcmp(context, c, sizeof(context)) &&
                !memcmp(binding, &c->bindings[symbol - 1], sizeof(binding)) &&
                !memcmp(place, &c->places[b.place - 1], sizeof(place)) &&
                !memcmp(value, &c->values[b.value - 1], sizeof(value)) &&
                nl_sem_validate(c) == NL_CHECK_OK);
        printf("OOM %s reached=%zu hit=%zu bytes=%zu rollback=PASS\n",
               site == HELPER_SEAL ? "bool-helper-seal" : "entry-result-world",
               visits, hits, failed_bytes);
    }
    REQUIRE(nl_semantic_check_expression(c, expression, entry, NULL) ==
                NL_CHECK_OK &&
            *entry && custody(*entry));
    char *text = NULL;
    size_t length = 1654;
    hits = failed_bytes = 0;
    allocation = 1;
    const NLCheckStatus status = nl_quantity_usize_c17(*entry, &text, &length);
    allocation = 0;
    const bool preserved = !text && length == 1654 && hits == 1 && failed_bytes;
    free(text);
    REQUIRE(status == NL_CHECK_OUT_OF_MEMORY && preserved);
    printf("OOM C17 staging hit=%zu bytes=%zu output-preserved=PASS\n", hits,
           failed_bytes);
    const bool emitted =
        nl_quantity_usize_c17(*entry, &text, &length) == NL_CHECK_OK && text &&
        length && strstr(text, "(_Bool nl_local_") &&
        strstr(text, "UINTMAX_C(19)");
    free(text);
    REQUIRE(emitted);
    puts("CHECK/C17 retry PASS");
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *unit = NULL, *expression = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *entry = NULL;
    bool ready =
        nl_source_load(argv[1], &source) == NL_SOURCE_OK &&
        nl_parser_create(source, &parser) == NL_PARSE_OK &&
        nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK &&
        nl_semantic_create(&context) == NL_CHECK_OK;
    const NLSyntaxTree *units[] = {unit};
    ready = ready &&
            nl_semantic_register_function_unit(context, units, 1, NULL) ==
                NL_CHECK_OK &&
            nl_source_create("main()", sizeof("main()") - 1, "<p1654-entry>",
                             &entry_source) == NL_SOURCE_OK &&
            nl_parser_create(entry_source, &entry_parser) == NL_PARSE_OK &&
            nl_parser_parse_expression_fragment(entry_parser, &expression,
                                                NULL) == NL_PARSE_OK;
    const bool ok = ready && rollback(context, expression, &entry);
    nl_checked_destroy(entry);
    nl_syntax_tree_destroy(expression);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return ok ? 0 : 1;
}
