#include "division_usize_internal.h"
#include "h_origin_internal.h"
#include "newlang/parser.h"
#include "newlang/usize_division.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(condition)                                                     \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "P1549 line %d: %s\n", __LINE__, #condition);      \
            return false;                                                      \
        }                                                                      \
    } while (0)

typedef enum {
    P1549_NO_FAULT,
    P1549_SECOND_SEAL,
    P1549_WORLD_SEAL,
    P1549_STAGING
} P1549Fault;

static P1549Fault fault;
static size_t entered_seals, alloc_ordinal, fault_ordinal, hits, unknown_events;
static bool observer_error;

void *__real_malloc(size_t);
NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);
NLCheckStatus __real_nl_usize_arithmetic_capture(NLCheckedFragment *,
                                                 NLCheckedNodeId,
                                                 const NLSyntaxNode *,
                                                 const NLSemanticContext *);

void *__wrap_malloc(size_t size)
{
    if (fault_ordinal && ++alloc_ordinal == fault_ordinal && !hits) {
        ++hits;
        return NULL;
    }
    return __real_malloc(size);
}

NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *parent,
                                              NLCheckedFragment *child,
                                              NLCheckedNodeId call,
                                              const NLSemanticContext *world)
{
    if (fault == P1549_SECOND_SEAL && !world && ++entered_seals == 2)
        fault_ordinal = 1; /* Second invocation's real checked-node seal. */
    if (fault == P1549_WORLD_SEAL && world) {
        ++entered_seals;
        fault_ordinal =
            2; /* Outer node snapshot, then committed world bytes. */
    }
    const NLCheckStatus status =
        __real_nl_h_custody_seal_result(parent, child, call, world);
    fault_ordinal = 0;
    return status;
}

NLCheckStatus __wrap_nl_usize_arithmetic_capture(NLCheckedFragment *f,
                                                 NLCheckedNodeId id,
                                                 const NLSyntaxNode *syntax,
                                                 const NLSemanticContext *c)
{
    const NLCheckStatus status =
        __real_nl_usize_arithmetic_capture(f, id, syntax, c);
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    if (status == NL_CHECK_OK && n && n->kind == NL_CHECKED_USIZE_DIVISION &&
        !n->scalar_result.known) {
        NLUsizeDivisionView receipt;
        NLSemanticValueView value;
        ++unknown_events;
        if (!nl_checked_usize_division_view(f, id, &receipt) ||
            receipt.numerator_known || receipt.quotient_known ||
            receipt.numbers[0] || receipt.quotient || !receipt.numbers[1] ||
            !nl_semantic_value_view(c, receipt.result, &value) ||
            value.scalar_known || value.scalar_value)
            observer_error = true;
    }
    return status;
}

typedef struct {
    NLSource *source, *entry_source;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *unit, *entry;
    NLSemanticContext *context;
    NLCheckedFragment *artifact;
} P1549Program;

static void p1549_destroy(P1549Program *p)
{
    nl_checked_destroy(p->artifact);
    nl_syntax_tree_destroy(p->entry);
    nl_parser_destroy(p->entry_parser);
    nl_source_destroy(p->entry_source);
    nl_syntax_tree_destroy(p->unit);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    nl_semantic_destroy(p->context);
}

static bool p1549_prepare(const char *path, P1549Program *p)
{
    REQUIRE(nl_source_load(path, &p->source) == NL_SOURCE_OK);
    REQUIRE(nl_parser_create(p->source, &p->parser) == NL_PARSE_OK);
    REQUIRE(nl_parser_parse_function_unit(p->parser, &p->unit, NULL) ==
            NL_PARSE_OK);
    puts("SOURCE PASS");
    REQUIRE(nl_semantic_create(&p->context) == NL_CHECK_OK);
    const NLSyntaxTree *units[] = {p->unit};
    REQUIRE(nl_semantic_register_function_unit(p->context, units, 1, NULL) ==
            NL_CHECK_OK);
    nl_syntax_tree_destroy(p->unit);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    p->unit = NULL;
    p->parser = NULL;
    p->source = NULL;
    REQUIRE(nl_source_create("main()", 6, "<p1549-entry>", &p->entry_source) ==
            NL_SOURCE_OK);
    REQUIRE(nl_parser_create(p->entry_source, &p->entry_parser) == NL_PARSE_OK);
    REQUIRE(nl_parser_parse_expression_fragment(p->entry_parser, &p->entry,
                                                NULL) == NL_PARSE_OK);
    return true;
}

static NLCheckStatus p1549_check(P1549Program *p)
{
    NLCheckDiagnostic diagnostic = {0};
    const NLCheckStatus status = nl_semantic_check_expression(
        p->context, p->entry, &p->artifact, &diagnostic);
    if (status != NL_CHECK_OK)
        fprintf(stderr, "CHECK status=%d code=%s\n", status,
                diagnostic.diagnostic.code ? diagnostic.diagnostic.code
                                           : "(none)");
    return status;
}

static bool p1549_scalar(const NLSemanticContext *context, NLValueId value,
                         size_t *number)
{
    NLSemanticValueView view;
    if (!nl_semantic_value_view(context, value, &view) ||
        view.type != nl_semantic_core_type(context, NL_TYPE_USIZE) ||
        !view.scalar_known || view.dependencies != NL_DEPENDENCY_FREE ||
        view.value_dependency_count || view.domain || view.reference_count ||
        view.reference.place || view.allocation_region)
        return false;
    *number = view.scalar_value;
    return true;
}

static bool p1549_call(const NLCheckedFragment *caller, NLCheckedNodeId id,
                       const NLSemanticContext *context)
{
    const NLCheckedNodeView *call = nl_checked_node_view(caller, id);
    const NLCheckedFragment *body = nl_checked_call_body(caller, id);
    REQUIRE(call && body && call->argument_count == 1 &&
            call->result_count == 1);
    const NLCheckedNodeView *actual =
        nl_checked_node_view(caller, call->first_argument);
    const NLCheckedNodeView *root = nl_checked_node_view(body, body->root);
    REQUIRE(actual && actual->result_count == 1 && root);
    NLCheckedNodeId division = 0;
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(body); ++i) {
        if (nl_checked_node_view(body, i)->kind == NL_CHECKED_USIZE_DIVISION) {
            REQUIRE(!division);
            division = i;
        }
    }
    NLUsizeDivisionView proof;
    REQUIRE(division && nl_checked_usize_division_view(body, division, &proof));
    REQUIRE(nl_checked_usize_division_valid(body, division, &proof));
    REQUIRE(proof.origin == body && proof.numerator_known &&
            proof.quotient_known && proof.target_max == SIZE_MAX &&
            proof.numbers[1] &&
            proof.quotient == proof.numbers[0] / proof.numbers[1] &&
            proof.result != proof.values[0] && proof.result != proof.values[1]);
    NLSemanticBindingView formal;
    REQUIRE(nl_semantic_binding_view(context, proof.symbols[0], &formal) &&
            formal.place == proof.places[0] &&
            formal.value == actual->results[0].value);
    const NLValueId returned =
        root->terminates ? root->returned.value : root->results[0].value;
    size_t actual_number, returned_number;
    REQUIRE(call->results[0].value == returned && returned != proof.result &&
            p1549_scalar(context, formal.value, &actual_number) &&
            p1549_scalar(context, returned, &returned_number) &&
            actual_number == proof.numbers[0] &&
            returned_number == proof.quotient);
    printf("CALL %zu %zu %zu %s %zu %zu %zu %zu seal=%d\n", actual_number,
           proof.numbers[1], proof.quotient,
           root->terminates ? "return" : "tail", formal.value, proof.symbols[0],
           proof.result, returned, nl_h_custody_result_valid(caller, id));
    return true;
}

static bool p1549_inspect(P1549Program *p)
{
    REQUIRE(p1549_check(p) == NL_CHECK_OK);
    REQUIRE(unknown_events && !observer_error);
    printf("DEFINITION unknown=%zu\n", unknown_events);
    puts("CHECK PASS");
    const NLCheckedNodeId entry_id = nl_checked_root(p->artifact);
    const NLCheckedFragment *main = nl_checked_call_body(p->artifact, entry_id);
    REQUIRE(main &&
            nl_checked_proof_class(p->artifact) == NL_CHECKED_MACHINE_PROVED);
    const NLCheckedNodeView *root = nl_checked_node_view(main, main->root);
    REQUIRE(root && root->kind == NL_CHECKED_BLOCK);
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *binding = nl_checked_node_view(main, id);
        REQUIRE(binding && binding->kind == NL_CHECKED_BINDING);
        const NLCheckedNodeView *init =
            nl_checked_node_view(main, binding->initializer);
        REQUIRE(init && init->result_count == 1);
        NLSemanticBindingView name;
        REQUIRE(nl_semantic_binding_view(p->context, binding->symbol, &name) &&
                name.value == init->results[0].value);
        size_t number;
        REQUIRE(p1549_scalar(p->context, name.value, &number));
        if (init->kind == NL_CHECKED_REGISTERED_CALL)
            REQUIRE(p1549_call(main, binding->initializer, p->context));
        printf("BIND %s %zu %zu %zu\n",
               p->context->bindings[binding->symbol - 1].name, binding->symbol,
               number, name.value);
        id = binding->next_item;
    }
    REQUIRE(nl_h_custody_result_valid(p->artifact, entry_id) &&
            nl_h_custody_result_world_present(p->artifact, entry_id));
    puts("WORLD seal=1 bytes=1");
    return true;
}

static bool p1549_original_plan(const P1549Program *program,
                                NLDivisionUsizePlan *main,
                                NLDivisionUsizePlan *helper)
{
    return nl_division_usize_call_valid(
               program->artifact, nl_checked_root(program->artifact), main) &&
           nl_division_usize_call_valid(main->body, main->call, helper);
}

static bool p1549_rejects(const NLCheckedFragment *entry)
{
    char sentinel[] = "original";
    char *text = sentinel;
    size_t size = 137;
    const NLCheckStatus status = nl_division_usize_c17(entry, &text, &size);
    if (status == NL_CHECK_OK)
        free(text);
    return status == NL_CHECK_SEMANTIC_UNSUPPORTED && text == sentinel &&
           size == 137 && !strcmp(sentinel, "original");
}

static bool p1549_authenticate(P1549Program *p, P1549Program *other)
{
    REQUIRE(p1549_check(p) == NL_CHECK_OK && p1549_check(other) == NL_CHECK_OK);
    NLDivisionUsizePlan main, helper, other_main, other_helper;
    REQUIRE(p1549_original_plan(p, &main, &helper));
    REQUIRE(p1549_original_plan(other, &other_main, &other_helper));
    NLUsizeDivisionView receipt;
    REQUIRE(
        nl_checked_usize_division_view(helper.body, helper.division, &receipt));
    REQUIRE(nl_checked_usize_division_valid(helper.body, helper.division,
                                            &receipt));
    REQUIRE(!nl_checked_usize_division_valid(other_helper.body,
                                             other_helper.division, &receipt));
    REQUIRE(nl_checked_usize_division_valid(other_helper.body,
                                            other_helper.division, NULL));

    NLCheckedNodeView *call = &main.body->nodes[main.call - 1];
    const NLValueId authentic = call->results[0].value;
    const NLCheckedNodeView *root =
        nl_checked_node_view(main.body, main.body->root);
    NLValueId equal_copy = 0;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *binding = nl_checked_node_view(main.body, id);
        const NLCheckedNodeView *init =
            nl_checked_node_view(main.body, binding->initializer);
        if (init->kind == NL_CHECKED_IDENTIFIER &&
            p->context->bindings[init->symbol - 1].view.value == authentic) {
            equal_copy = init->results[0].value;
            break;
        }
        id = binding->next_item;
    }
    size_t original_value, equal_value;
    REQUIRE(equal_copy != authentic &&
            p1549_scalar(p->context, equal_copy, &equal_value) &&
            p1549_scalar(p->context, authentic, &original_value) &&
            equal_value == original_value);
    call->results[0].value = equal_copy;
    const bool rejected = p1549_rejects(p->artifact);
    call->results[0].value = authentic;
    REQUIRE(rejected && p1549_original_plan(p, &main, &helper));
    puts("CUSTODY foreign receipt and equal-value wrong result rejected");
    return true;
}

static bool p1549_retry_check(P1549Program *p, P1549Fault site)
{
    NLSemanticContext before;
    memcpy(&before, p->context, sizeof before);
    fault = site;
    const NLCheckStatus status = p1549_check(p);
    fault = P1549_NO_FAULT;
    REQUIRE(status == NL_CHECK_OUT_OF_MEMORY && !p->artifact && hits == 1 &&
            entered_seals == (site == P1549_SECOND_SEAL ? 2u : 1u) &&
            alloc_ordinal == (site == P1549_SECOND_SEAL ? 1u : 2u) &&
            !memcmp(&before, p->context, sizeof before) &&
            nl_sem_validate(p->context) == NL_CHECK_OK);
    REQUIRE(p1549_inspect(p));
    char *text = NULL;
    size_t size = 0;
    const NLCheckStatus retry =
        nl_division_usize_c17(p->artifact, &text, &size);
    free(text);
    REQUIRE(retry == NL_CHECK_OK && size);
    puts("OOM hit=1 no-clobber=PASS retry=PASS");
    return true;
}

static bool p1549_retry_emit(P1549Program *p)
{
    REQUIRE(p1549_check(p) == NL_CHECK_OK);
    char *original = NULL;
    size_t original_size = 0;
    REQUIRE(nl_division_usize_c17(p->artifact, &original, &original_size) ==
            NL_CHECK_OK);
    char sentinel[] = "original";
    char *text = sentinel;
    size_t size = 137;
    fault = P1549_STAGING;
    fault_ordinal = 1;
    const NLCheckStatus status =
        nl_division_usize_c17(p->artifact, &text, &size);
    fault = P1549_NO_FAULT;
    fault_ordinal = 0;
    const bool held = status == NL_CHECK_OUT_OF_MEMORY && hits == 1 &&
                      text == sentinel && size == 137 &&
                      !strcmp(sentinel, "original");
    if (status == NL_CHECK_OK)
        free(text);
    text = NULL;
    size = 0;
    const NLCheckStatus retry =
        nl_division_usize_c17(p->artifact, &text, &size);
    const bool same = retry == NL_CHECK_OK && size == original_size &&
                      !memcmp(text, original, size);
    free(original);
    free(text);
    REQUIRE(held && same);
    puts("OOM hit=1 no-clobber=PASS retry=PASS");
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    const char *mode = argv[1];
    P1549Program program = {0}, other = {0};
    bool ok = p1549_prepare(argv[2], &program);
    if (ok && !strcmp(mode, "inspect"))
        ok = p1549_inspect(&program);
    else if (ok && !strcmp(mode, "authenticity"))
        ok = p1549_prepare(argv[2], &other) &&
             p1549_authenticate(&program, &other);
    else if (ok && !strcmp(mode, "second-seal"))
        ok = p1549_retry_check(&program, P1549_SECOND_SEAL);
    else if (ok && !strcmp(mode, "world-seal"))
        ok = p1549_retry_check(&program, P1549_WORLD_SEAL);
    else if (ok && !strcmp(mode, "staging"))
        ok = p1549_retry_emit(&program);
    else
        ok = false;
    p1549_destroy(&other);
    p1549_destroy(&program);
    return ok ? 0 : 1;
}
