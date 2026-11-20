#include "division_usize_internal.h"
#include "h_origin_internal.h"
#include "newlang/parser.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(condition)                                                     \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "P1561 line %d: %s\n", __LINE__, #condition);      \
            return false;                                                      \
        }                                                                      \
    } while (0)

typedef enum {
    FAULT_NONE,
    FAULT_SECOND_LEAF,
    FAULT_OUTER_NODES,
    FAULT_OUTER_WORLD
} FaultSite;

static FaultSite fault_site;
static size_t allocation, fault_ordinal, hits, seal_visits, unknown_events;
static NLCheckStatus injected_status;
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
    if (fault_ordinal && ++allocation == fault_ordinal) {
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
    if (fault_site == FAULT_SECOND_LEAF && !world && ++seal_visits == 2)
        fault_ordinal = 1; /* The second actual leaf's checked-node seal. */
    if (world &&
        (fault_site == FAULT_OUTER_NODES || fault_site == FAULT_OUTER_WORLD)) {
        ++seal_visits;
        fault_ordinal = fault_site == FAULT_OUTER_NODES ? 1 : 2;
    }
    const NLCheckStatus status =
        __real_nl_h_custody_seal_result(parent, child, call, world);
    if (fault_ordinal)
        injected_status = status;
    fault_ordinal = 0;
    return status;
}

NLCheckStatus __wrap_nl_usize_arithmetic_capture(NLCheckedFragment *f,
                                                 NLCheckedNodeId id,
                                                 const NLSyntaxNode *source,
                                                 const NLSemanticContext *c)
{
    const NLCheckStatus status =
        __real_nl_usize_arithmetic_capture(f, id, source, c);
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    if (status != NL_CHECK_OK || !n || n->scalar_result.known)
        return status;
    bool unknown;
    if (n->kind == NL_CHECKED_USIZE_DIVISION) {
        NLUsizeDivisionView event;
        unknown = nl_checked_usize_division_view(f, id, &event) &&
                  !event.numerator_known && !event.quotient_known &&
                  !event.numbers[0] && !event.quotient && event.numbers[1];
    } else if (n->kind == NL_CHECKED_USIZE_REMAINDER) {
        NLUsizeRemainderView event;
        unknown = nl_checked_usize_remainder_view(f, id, &event) &&
                  !event.numerator_known && !event.remainder_known &&
                  !event.numbers[0] && !event.remainder && event.numbers[1];
    } else
        return status;
    NLSemanticValueView value;
    ++unknown_events;
    if (!unknown || !nl_semantic_value_view(c, n->results[0].value, &value) ||
        value.scalar_known || value.scalar_value)
        observer_error = true;
    return status;
}

typedef struct {
    NLSource *entry_source;
    NLParser *entry_parser;
    NLSyntaxTree *entry;
    NLSemanticContext *context;
    NLCheckedFragment *checked;
} Program;

static void p1561_destroy(Program *p)
{
    nl_checked_destroy(p->checked);
    nl_syntax_tree_destroy(p->entry);
    nl_parser_destroy(p->entry_parser);
    nl_source_destroy(p->entry_source);
    nl_semantic_destroy(p->context);
}

static bool p1561_prepare(const char *path, Program *p)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    const bool parsed =
        nl_source_load(path, &source) == NL_SOURCE_OK &&
        nl_parser_create(source, &parser) == NL_PARSE_OK &&
        nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK;
    bool registered = false;
    if (parsed && nl_semantic_create(&p->context) == NL_CHECK_OK) {
        const NLSyntaxTree *units[] = {tree};
        registered = nl_semantic_register_function_unit(p->context, units, 1,
                                                        NULL) == NL_CHECK_OK;
    }
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    REQUIRE(parsed && registered);
    REQUIRE(nl_source_create("main()", 6, "<p1561-entry>", &p->entry_source) ==
            NL_SOURCE_OK);
    REQUIRE(nl_parser_create(p->entry_source, &p->entry_parser) == NL_PARSE_OK);
    REQUIRE(nl_parser_parse_expression_fragment(p->entry_parser, &p->entry,
                                                NULL) == NL_PARSE_OK);
    return true;
}

static NLCheckStatus p1561_check(Program *p)
{
    NLCheckDiagnostic diagnostic = {0};
    const NLCheckStatus status = nl_semantic_check_expression(
        p->context, p->entry, &p->checked, &diagnostic);
    if (status != NL_CHECK_OK)
        fprintf(stderr, "CHECK status=%d code=%s\n", status,
                diagnostic.diagnostic.code ? diagnostic.diagnostic.code
                                           : "(none)");
    return status;
}

static bool p1561_scalar(const NLSemanticContext *c, NLValueId id, size_t *out)
{
    NLSemanticValueView v;
    if (!nl_semantic_value_view(c, id, &v) ||
        v.type != nl_semantic_core_type(c, NL_TYPE_USIZE) || !v.scalar_known ||
        v.dependencies != NL_DEPENDENCY_FREE || v.value_dependency_count ||
        v.domain || v.reference_count || v.reference.place ||
        v.allocation_region)
        return false;
    *out = v.scalar_value;
    return true;
}

static bool p1561_call(const NLSemanticContext *c, const NLCheckedFragment *f,
                       NLCheckedNodeId site, const char *destination)
{
    NLDivisionUsizePlan plan;
    REQUIRE(nl_quantity_usize_call_valid(f, site, &plan) && plan.formal &&
            nl_h_custody_result_valid(f, site));
    const NLCheckedNodeView *call = nl_checked_node_view(f, site);
    const NLCheckedFragment *body = nl_checked_call_body(f, site);
    REQUIRE(call && body && body == plan.body &&
            plan.function == call->function);
    const NLCheckedNodeView *arg =
        nl_checked_node_view(f, call->first_argument);
    const NLCheckedNodeView *root = nl_checked_node_view(body, body->root);
    REQUIRE(arg && root && call->function &&
            call->function <= c->function_count &&
            c->functions[call->function - 1].body == body->body_owner);
    const NLCheckedNodeId operation = plan.division;
    const NLCheckedNodeView *event_node = nl_checked_node_view(body, operation);
    REQUIRE(event_node && (event_node->kind == NL_CHECKED_USIZE_DIVISION ||
                           event_node->kind == NL_CHECKED_USIZE_REMAINDER));
    const bool division = event_node->kind == NL_CHECKED_USIZE_DIVISION;
    NLSymbolId formal_symbol;
    size_t lhs, rhs, number;
    NLValueId event, operand;
    NLSourceSpan op_span;
    if (division) {
        NLUsizeDivisionView evidence;
        REQUIRE(nl_checked_usize_division_view(body, operation, &evidence) &&
                nl_checked_usize_division_valid(body, operation, &evidence) &&
                evidence.origin == body && evidence.numerator_known &&
                evidence.quotient_known && evidence.target_max == SIZE_MAX);
        formal_symbol = evidence.symbols[0];
        lhs = evidence.numbers[0];
        rhs = evidence.numbers[1];
        number = evidence.quotient;
        event = evidence.result;
        operand = evidence.values[0];
        op_span = evidence.operator_span;
    } else {
        NLUsizeRemainderView evidence;
        REQUIRE(nl_checked_usize_remainder_view(body, operation, &evidence) &&
                nl_checked_usize_remainder_valid(body, operation, &evidence) &&
                evidence.origin == body && evidence.source == body->source &&
                evidence.numerator_known && evidence.remainder_known &&
                evidence.target_max == SIZE_MAX);
        formal_symbol = evidence.symbols[0];
        lhs = evidence.numbers[0];
        rhs = evidence.numbers[1];
        number = evidence.remainder;
        event = evidence.result;
        operand = evidence.values[0];
        op_span = evidence.operator_span;
    }
    REQUIRE(formal_symbol == plan.formal);
    const char op = division ? '/' : '%';
    NLSourceView token;
    REQUIRE(nl_source_view(body->source, op_span, &token) &&
            token.length == 1 && token.bytes[0] == op && rhs &&
            number == (division ? lhs / rhs : lhs % rhs));
    REQUIRE(arg->result_count == 1 && call->result_count == 1);
    NLSemanticBindingView formal;
    const NLValueId actual_id = arg->results[0].value;
    const NLValueId result_id = call->results[0].value;
    const NLValueId body_result =
        root->terminates ? root->returned.value : root->results[0].value;
    size_t actual, result;
    REQUIRE(nl_semantic_binding_view(c, formal_symbol, &formal) &&
            formal.value == actual_id && result_id == body_result &&
            actual_id < operand && operand < event && event <= result_id &&
            p1561_scalar(c, actual_id, &actual) && actual == lhs &&
            p1561_scalar(c, result_id, &result) && result == number &&
            ((operation == root->tail) == (event == result_id)));

    const char *argument = "-";
    NLValueId input = 0;
    if (arg->kind == NL_CHECKED_IDENTIFIER) {
        NLSemanticBindingView source;
        size_t before_copy;
        REQUIRE(arg->value_use == NL_VALUE_COPIED &&
                nl_semantic_binding_view(c, arg->symbol, &source) &&
                source.place != formal.place && source.value < actual_id &&
                p1561_scalar(c, source.value, &before_copy) &&
                before_copy == lhs);
        argument = c->bindings[arg->symbol - 1].name;
        input = source.value;
    } else
        REQUIRE(arg->kind == NL_CHECKED_USIZE_LITERAL);
    printf(
        "CALL %s %s %c %s %zu %zu %zu %s %zu %zu %zu %zu %zu %zu %zu seal=1\n",
        destination, c->functions[call->function - 1].name, op, argument,
        actual, rhs, result, root->terminates ? "return" : "tail",
        call->function, actual_id, formal_symbol, event, result_id, input,
        operand);
    return true;
}

static bool p1561_report(const Program *p)
{
    const NLCheckedNodeId outer = nl_checked_root(p->checked);
    const NLCheckedFragment *f = nl_checked_call_body(p->checked, outer);
    NLDivisionUsizePlan plan, legacy;
    REQUIRE(f && unknown_events && !observer_error &&
            nl_checked_proof_class(p->checked) == NL_CHECKED_MACHINE_PROVED &&
            nl_quantity_usize_call_valid(p->checked, outer, &plan) &&
            plan.body == f && !plan.formal);
    REQUIRE(nl_h_custody_result_valid(p->checked, outer) &&
            nl_h_custody_result_world_present(p->checked, outer));
    const bool division =
        nl_division_usize_call_valid(p->checked, outer, &legacy);
    const bool remainder =
        nl_remainder_usize_call_valid(p->checked, outer, &legacy);
    printf("SOURCE PASS\nCHECK PASS\nDEFINITION unknown=%zu\n", unknown_events);
    printf("OUTER result=1 world=1 division=%d remainder=%d\n", division,
           remainder);
    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    REQUIRE(root && root->kind == NL_CHECKED_BLOCK);
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        REQUIRE(n && n->kind == NL_CHECKED_BINDING);
        const NLCheckedNodeView *init = nl_checked_node_view(f, n->initializer);
        size_t number;
        NLSemanticBindingView binding;
        REQUIRE(init && init->result_count == 1 &&
                nl_semantic_binding_view(p->context, n->symbol, &binding) &&
                binding.value == init->results[0].value &&
                p1561_scalar(p->context, binding.value, &number));
        const char *name = p->context->bindings[n->symbol - 1].name;
        if (init->kind == NL_CHECKED_REGISTERED_CALL)
            REQUIRE(p1561_call(p->context, f, n->initializer, name));
        if (init->kind == NL_CHECKED_IDENTIFIER) {
            NLSemanticBindingView from;
            size_t input;
            REQUIRE(init->value_use == NL_VALUE_COPIED &&
                    nl_semantic_binding_view(p->context, init->symbol, &from) &&
                    from.value < binding.value &&
                    p1561_scalar(p->context, from.value, &input) &&
                    input == number);
            printf("COPY %s %s %zu %zu\n", name,
                   p->context->bindings[init->symbol - 1].name, from.value,
                   binding.value);
        }
        printf("BIND %s %zu %zu %zu\n", name, n->symbol, number, binding.value);
        id = n->next_item;
    }
    return true;
}

static bool p1561_refuses(const NLCheckedFragment *entry)
{
    NLDivisionUsizePlan plan;
    char sentinel[] = "untouched";
    char *output = sentinel;
    size_t length = 137;
    const bool valid =
        nl_quantity_usize_call_valid(entry, nl_checked_root(entry), &plan);
    const NLCheckStatus status = nl_quantity_usize_c17(entry, &output, &length);
    if (status == NL_CHECK_OK)
        free(output);
    return !valid && status == NL_CHECK_SEMANTIC_UNSUPPORTED &&
           output == sentinel && length == 137 &&
           !strcmp(sentinel, "untouched");
}

static bool p1561_identity(const Program *p, const Program *foreign)
{
    NLDivisionUsizePlan main, other_main, phase, lanes, other_phase;
    REQUIRE(nl_quantity_usize_call_valid(p->checked,
                                         nl_checked_root(p->checked), &main) &&
            nl_quantity_usize_call_valid(foreign->checked,
                                         nl_checked_root(foreign->checked),
                                         &other_main));
    REQUIRE(main.body->body_count == 5 && other_main.body->body_count == 5);
    const NLCheckedNodeId phase_call = main.body->body_calls[1];
    const NLCheckedNodeId lane_call = main.body->body_calls[2];
    REQUIRE(nl_quantity_usize_call_valid(main.body, phase_call, &phase) &&
            nl_quantity_usize_call_valid(main.body, lane_call, &lanes) &&
            nl_quantity_usize_call_valid(
                other_main.body, other_main.body->body_calls[1], &other_phase));
    NLUsizeRemainderView r;
    NLUsizeDivisionView q;
    REQUIRE(nl_checked_usize_remainder_view(phase.body, phase.division, &r) &&
            nl_checked_usize_division_view(lanes.body, lanes.division, &q) &&
            nl_checked_usize_remainder_valid(phase.body, phase.division, &r) &&
            nl_checked_usize_division_valid(lanes.body, lanes.division, &q) &&
            r.remainder_known && q.quotient_known &&
            r.remainder == q.quotient && r.result != q.result &&
            phase.function != lanes.function);
    REQUIRE(
        !nl_checked_usize_remainder_valid(lanes.body, lanes.division, NULL) &&
        !nl_checked_usize_division_valid(phase.body, phase.division, NULL) &&
        !nl_checked_usize_remainder_valid(other_phase.body,
                                          other_phase.division, &r));
    NLUsizeRemainderView same_number = r;
    same_number.result = q.result;
    REQUIRE(!nl_checked_usize_remainder_valid(phase.body, phase.division,
                                              &same_number));
    NLUsizeRemainderView wrong_source = r;
    wrong_source.source = nl_checked_source(other_phase.body);
    REQUIRE(wrong_source.source != r.source &&
            !nl_checked_usize_remainder_valid(phase.body, phase.division,
                                              &wrong_source));

    /* Test-only negative instrumentation of a published checked graph. */
    NLCheckedNodeView *call = &main.body->nodes[phase_call - 1];
    const NLCheckedNodeView *lane = nl_checked_node_view(main.body, lane_call);
    REQUIRE(lane && lane->kind == NL_CHECKED_REGISTERED_CALL);
    const size_t original_function = call->function;
    const NLValueId original_result = call->results[0].value;
    size_t original_value, substitute_value;
    REQUIRE(
        original_result != lane->results[0].value &&
        p1561_scalar(p->context, original_result, &original_value) &&
        p1561_scalar(p->context, lane->results[0].value, &substitute_value) &&
        original_value == substitute_value);
    call->function = lane->function;
    const bool callee_rejected = p1561_refuses(p->checked);
    call->function = original_function;
    call->results[0].value = lane->results[0].value;
    const bool result_rejected = p1561_refuses(p->checked);
    call->results[0].value = original_result;

    NLCheckedNodeView *op = &phase.body->nodes[phase.division - 1];
    const NLCheckedKind original_kind = op->kind;
    op->kind = NL_CHECKED_USIZE_DIVISION;
    const bool operation_rejected = p1561_refuses(p->checked);
    op->kind = original_kind;

    NLCheckedFragment *body = (NLCheckedFragment *)phase.body;
    const NLSource *source = body->source;
    body->source = wrong_source.source;
    const bool source_rejected = p1561_refuses(p->checked);
    body->source = source;

    char *restored = NULL;
    size_t length = 0;
    const NLCheckStatus status =
        nl_quantity_usize_c17(p->checked, &restored, &length);
    free(restored);
    REQUIRE(callee_rejected && result_rejected && operation_rejected &&
            source_rejected && status == NL_CHECK_OK && length);
    puts("IDENTITY callee/operator/result/source rejected with equal values; "
         "restoration PASS");
    return true;
}

static bool p1561_retry_check(Program *p, FaultSite site)
{
    NLSemanticContext *c = p->context;
    const NLTypeId type = nl_semantic_core_type(c, NL_TYPE_USIZE);
    NLSymbolId anchor;
    /* A separate test-only fixture tests rollback of actual prior storage. */
    REQUIRE(nl_semantic_seed_scalar(c, "p1561_anchor",
                                    (NLScalarValue){type, true, 73},
                                    &anchor) == NL_CHECK_OK &&
            anchor && !p->checked);
    const NLSemanticBindingView *prior = &c->bindings[anchor - 1].view;
    const NLValueId value = prior->value;
    const NLPlaceId place = prior->place;
    REQUIRE(place && value);
    unsigned char header[sizeof *c], binding[sizeof *c->bindings],
        value_bytes[sizeof *c->values], place_bytes[sizeof *c->places];
    memcpy(header, c, sizeof header);
    memcpy(binding, &c->bindings[anchor - 1], sizeof binding);
    memcpy(value_bytes, &c->values[value - 1], sizeof value_bytes);
    memcpy(place_bytes, &c->places[place - 1], sizeof place_bytes);
    size_t checked_value;
    fault_site = site;
    const NLCheckStatus status = p1561_check(p);
    fault_site = FAULT_NONE;
    printf("FAULT status=%d allocation=%zu visits=%zu hits=%zu\n", status,
           allocation, seal_visits, hits);
    REQUIRE(status == NL_CHECK_OUT_OF_MEMORY && injected_status == status &&
            hits == 1 && !p->checked &&
            seal_visits == (site == FAULT_SECOND_LEAF ? 2u : 1u) &&
            allocation == (site == FAULT_OUTER_WORLD ? 2u : 1u) &&
            !memcmp(header, c, sizeof header) &&
            !memcmp(binding, &c->bindings[anchor - 1], sizeof binding) &&
            !memcmp(value_bytes, &c->values[value - 1], sizeof value_bytes) &&
            !memcmp(place_bytes, &c->places[place - 1], sizeof place_bytes) &&
            nl_semantic_find_binding(c, "p1561_anchor") == anchor &&
            p1561_scalar(c, value, &checked_value) && checked_value == 73 &&
            nl_sem_validate(c) == NL_CHECK_OK);
    REQUIRE(p1561_check(p) == NL_CHECK_OK && p1561_report(p));
    char *output = NULL;
    size_t length = 0;
    const NLCheckStatus retry =
        nl_quantity_usize_c17(p->checked, &output, &length);
    const bool emitted = retry == NL_CHECK_OK && length && output &&
                         strstr(output, " / ") && strstr(output, " % ");
    free(output);
    REQUIRE(emitted);
    return true;
}

static bool p1561_retry_emit(Program *p)
{
    REQUIRE(p1561_check(p) == NL_CHECK_OK);
    char *clean = NULL;
    size_t clean_size = 0;
    REQUIRE(nl_quantity_usize_c17(p->checked, &clean, &clean_size) ==
            NL_CHECK_OK);
    char sentinel[] = "untouched";
    char *output = sentinel;
    size_t length = 137;
    fault_ordinal = 1;
    const NLCheckStatus status =
        nl_quantity_usize_c17(p->checked, &output, &length);
    fault_ordinal = 0;
    printf("FAULT status=%d allocation=%zu hits=%zu\n", status, allocation,
           hits);
    const bool held = status == NL_CHECK_OUT_OF_MEMORY && hits == 1 &&
                      allocation == 1 && output == sentinel && length == 137 &&
                      !strcmp(sentinel, "untouched");
    if (status == NL_CHECK_OK)
        free(output);
    output = NULL;
    length = 0;
    const NLCheckStatus retry =
        nl_quantity_usize_c17(p->checked, &output, &length);
    const bool same = retry == NL_CHECK_OK && length == clean_size && length &&
                      !memcmp(output, clean, length);
    free(clean);
    free(output);
    REQUIRE(held && same && p1561_report(p));
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    const char *mode = argv[1];
    Program program = {0}, foreign = {0};
    bool ok = p1561_prepare(argv[2], &program);
    if (ok && !strcmp(mode, "inspect"))
        ok = p1561_check(&program) == NL_CHECK_OK && p1561_report(&program);
    else if (ok && !strcmp(mode, "authenticity"))
        ok = p1561_check(&program) == NL_CHECK_OK &&
             p1561_prepare(argv[2], &foreign) &&
             p1561_check(&foreign) == NL_CHECK_OK &&
             p1561_identity(&program, &foreign);
    else if (ok) {
        if (!strcmp(mode, "second-leaf-oom"))
            ok = p1561_retry_check(&program, FAULT_SECOND_LEAF);
        else if (!strcmp(mode, "outer-nodes-oom"))
            ok = p1561_retry_check(&program, FAULT_OUTER_NODES);
        else if (!strcmp(mode, "outer-world-oom"))
            ok = p1561_retry_check(&program, FAULT_OUTER_WORLD);
        else if (!strcmp(mode, "staging-oom"))
            ok = p1561_retry_emit(&program);
        else
            ok = false;
        if (ok)
            printf("OOM %s hit=1 no-clobber=PASS retry=PASS\n", mode);
    }
    p1561_destroy(&foreign);
    p1561_destroy(&program);
    return ok ? 0 : 1;
}
