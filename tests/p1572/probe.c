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
            fprintf(stderr, "P1572 line %d: %s\n", __LINE__, #condition);      \
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

static void p1572_destroy(Program *p)
{
    nl_checked_destroy(p->checked);
    nl_syntax_tree_destroy(p->entry);
    nl_parser_destroy(p->entry_parser);
    nl_source_destroy(p->entry_source);
    nl_semantic_destroy(p->context);
}

static bool p1572_prepare(const char *path, Program *p)
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
    REQUIRE(nl_source_create("main()", 6, "<p1572-entry>", &p->entry_source) ==
            NL_SOURCE_OK);
    REQUIRE(nl_parser_create(p->entry_source, &p->entry_parser) == NL_PARSE_OK);
    REQUIRE(nl_parser_parse_expression_fragment(p->entry_parser, &p->entry,
                                                NULL) == NL_PARSE_OK);
    return true;
}

static NLCheckStatus p1572_check(Program *p)
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

static bool p1572_scalar(const NLSemanticContext *c, NLValueId id, size_t *out)
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

static bool operation_node(const NLCheckedNodeView *n)
{
    return n && (n->kind == NL_CHECKED_USIZE_DIVISION ||
                 n->kind == NL_CHECKED_USIZE_REMAINDER);
}

static bool report_copy(const NLSemanticContext *c, const NLCheckedFragment *f,
                        NLCheckedNodeId id, size_t number, NLValueId *value)
{
    const NLCheckedNodeView *use = nl_checked_node_view(f, id);
    NLSemanticBindingView from;
    size_t before, after;
    REQUIRE(use && use->kind == NL_CHECKED_IDENTIFIER &&
            use->value_use == NL_VALUE_COPIED && use->result_count == 1 &&
            nl_semantic_binding_view(c, use->symbol, &from) &&
            from.value < use->results[0].value &&
            p1572_scalar(c, from.value, &before) &&
            p1572_scalar(c, use->results[0].value, &after) &&
            before == number && after == number);
    *value = use->results[0].value;
    return true;
}

static bool report_event(const NLCheckedFragment *body, NLCheckedNodeId id,
                         size_t *lhs, size_t *rhs, size_t *number,
                         NLValueId *event, NLCheckedNodeId *operand)
{
    const NLCheckedNodeView *n = nl_checked_node_view(body, id);
    REQUIRE(operation_node(n));
    if (n->kind == NL_CHECKED_USIZE_DIVISION) {
        NLUsizeDivisionView q;
        REQUIRE(nl_checked_usize_division_view(body, id, &q) &&
                nl_checked_usize_division_valid(body, id, &q) &&
                q.origin == body && q.numerator_known && q.quotient_known &&
                q.target_max == SIZE_MAX && q.numbers[1] &&
                q.quotient == q.numbers[0] / q.numbers[1]);
        *lhs = q.numbers[0];
        *rhs = q.numbers[1];
        *number = q.quotient;
        *event = q.result;
        *operand = q.operands[0];
    } else {
        NLUsizeRemainderView r;
        REQUIRE(nl_checked_usize_remainder_view(body, id, &r) &&
                nl_checked_usize_remainder_valid(body, id, &r) &&
                r.origin == body && r.source == body->source &&
                r.numerator_known && r.remainder_known &&
                r.target_max == SIZE_MAX && r.numbers[1] &&
                r.remainder == r.numbers[0] % r.numbers[1]);
        *lhs = r.numbers[0];
        *rhs = r.numbers[1];
        *number = r.remainder;
        *event = r.result;
        *operand = r.operands[0];
    }
    return true;
}

static bool p1572_call(const NLSemanticContext *c, const NLCheckedFragment *f,
                       NLCheckedNodeId site, const char *destination)
{
    const NLCheckedNodeView *call = nl_checked_node_view(f, site);
    const NLCheckedFragment *body = nl_checked_call_body(f, site);
    REQUIRE(call && body && call->function &&
            call->function <= c->function_count);
    const NLFunctionEntry *fn = &c->functions[call->function - 1];
    const NLCheckedNodeView *arg =
        nl_checked_node_view(f, call->first_argument);
    const NLCheckedNodeView *root = nl_checked_node_view(body, body->root);
    REQUIRE(fn->body == body->body_owner && fn->body->count == 1 && arg &&
            arg->result_count == 1 && call->result_count == 1 && root);

    const NLValueId actual_id = arg->results[0].value;
    const NLValueId result_id = call->results[0].value;
    const NLValueId body_result =
        root->terminates ? root->returned.value : root->results[0].value;
    NLSymbolId formal = 0;
    for (size_t i = 0; i < c->binding_count; ++i) {
        if (c->bindings[i].view.value == actual_id &&
            !strcmp(c->bindings[i].name, fn->body->parameter_names[0])) {
            REQUIRE(!formal);
            formal = i + 1;
        }
    }
    size_t actual, result;
    REQUIRE(formal && p1572_scalar(c, actual_id, &actual) &&
            p1572_scalar(c, result_id, &result) && result_id == body_result);
    if (arg->kind == NL_CHECKED_IDENTIFIER) {
        NLValueId copy;
        REQUIRE(report_copy(c, f, call->first_argument, actual, &copy) &&
                copy == actual_id && arg->symbol < formal);
    } else {
        REQUIRE(arg->kind == NL_CHECKED_USIZE_LITERAL);
    }

    NLCheckedNodeId op_id = 0, exit_id = root->tail;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *item = nl_checked_node_view(body, id);
        REQUIRE(item && (!item->next_item || item->next_item > id));
        if (item->kind == NL_CHECKED_RETURN) {
            REQUIRE(root->terminates && !item->next_item && !exit_id);
            exit_id = item->initializer;
            break;
        }
        REQUIRE(item->kind == NL_CHECKED_BINDING);
        if (operation_node(nl_checked_node_view(body, item->initializer))) {
            REQUIRE(!op_id);
            op_id = item->initializer;
        }
        id = item->next_item;
    }
    if (!op_id)
        op_id = exit_id;
    size_t lhs, rhs, number;
    NLValueId event, operand_value;
    NLCheckedNodeId operand;
    REQUIRE(report_event(body, op_id, &lhs, &rhs, &number, &event, &operand) &&
            lhs == actual && number == result &&
            report_copy(c, body, operand, actual, &operand_value) &&
            actual_id < operand_value && operand_value < event &&
            event <= result_id);
    const NLCheckedNodeView *op = nl_checked_node_view(body, op_id);
    const NLCheckedNodeView *lhs_use = nl_checked_node_view(body, operand);
    const char sign = op->kind == NL_CHECKED_USIZE_DIVISION ? '/' : '%';

    size_t before = 0, after = 0;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *item = nl_checked_node_view(body, id);
        if (item->kind == NL_CHECKED_RETURN)
            break;
        const NLCheckedNodeId init = item->initializer;
        NLSemanticBindingView binding;
        REQUIRE(nl_semantic_binding_view(c, item->symbol, &binding));
        if (init == op_id) {
            REQUIRE(binding.value == event);
        } else {
            const bool prefix = init < op_id;
            const NLCheckedNodeView *use = nl_checked_node_view(body, init);
            NLValueId copied;
            REQUIRE(
                report_copy(c, body, init, prefix ? actual : result, &copied) &&
                binding.value == copied && use->symbol < item->symbol);
            printf("COPY %s %s %s %zu %zu %zu\n", destination,
                   prefix ? "pre" : "post", c->bindings[item->symbol - 1].name,
                   use->symbol, item->symbol, copied);
            if (prefix)
                ++before;
            else
                ++after;
        }
        id = item->next_item;
    }
    if (op_id != exit_id) {
        NLValueId carried;
        REQUIRE(report_copy(c, body, exit_id, number, &carried) &&
                carried == result_id && event < carried);
    }
    NLDivisionUsizePlan plan;
    const bool valid = nl_quantity_usize_call_valid(f, site, &plan);
    if (valid)
        REQUIRE(plan.body == body && plan.function == call->function &&
                plan.formal == formal && plan.division == op_id &&
                plan.result == exit_id && plan.divisor == rhs);
    printf(
        "CALL %s %s %c %zu %zu %zu %zu %zu %s %zu %zu %zu %zu %zu %zu %zu %zu "
        "seal=%d plan=%d event=1\n",
        destination, fn->name, sign, actual, rhs, result, before, after,
        root->terminates ? "return" : "tail", call->function, actual_id, formal,
        lhs_use->symbol, operand_value, event, result_id, arg->symbol,
        nl_h_custody_result_valid(f, site), valid);
    return true;
}

static bool report_direct(const NLSemanticContext *c,
                          const NLCheckedFragment *f, NLCheckedNodeId id,
                          const char *dest, NLValueId bound_value,
                          size_t bound_number)
{
    size_t lhs, rhs, result;
    NLValueId event, copied;
    NLCheckedNodeId operand;
    REQUIRE(report_event(f, id, &lhs, &rhs, &result, &event, &operand) &&
            event == bound_value && result == bound_number &&
            report_copy(c, f, operand, lhs, &copied) && copied < event);
    const NLCheckedNodeView *op = nl_checked_node_view(f, id);
    const NLCheckedNodeView *numerator = nl_checked_node_view(f, operand);
    const NLSemanticBindingView source =
        c->bindings[numerator->symbol - 1].view;
    const char sign = op->kind == NL_CHECKED_USIZE_DIVISION ? '/' : '%';
    printf("DIRECT %s %c %s %zu %zu %zu %zu %zu %zu %zu event=1\n", dest, sign,
           c->bindings[numerator->symbol - 1].name, lhs, rhs, result,
           numerator->symbol, source.value, copied, event);
    return true;
}

static bool report_unit_exit(const NLCheckedFragment *f,
                             const NLCheckedNodeView *root,
                             const NLCheckedNodeView *ret, NLTypeId unit)
{
    const NLCheckedNodeView *atom =
        nl_checked_node_view(f, ret ? ret->initializer : root->tail);
    REQUIRE(atom && atom->kind == NL_CHECKED_UNIT && atom->type == unit &&
            !atom->terminates && !atom->result_count &&
            !atom->results[0].value);
    const size_t count = nl_control_exits_count(f->exits);
    if (ret) {
        const NLControlExitView *exit = nl_control_exit_view(f->exits, 0);
        REQUIRE(root->terminates && !root->type && !root->tail &&
                !root->result_count && root->returned.type == unit &&
                !root->returned.value && ret->terminates && !ret->next_item &&
                !ret->result_count && ret->returned.type == unit &&
                !ret->returned.value && count == 1 && exit && exit->state &&
                exit->kind == NL_EXIT_RETURN &&
                exit->target == f->function_target &&
                nl_control_exits_only_return(f->exits, f->function_target) &&
                exit->count == 1 && exit->values[0].type == unit &&
                !exit->values[0].value);
    } else {
        REQUIRE(!count && !root->terminates && root->type == unit &&
                !root->result_count && !root->results[0].value);
    }
    printf("EXIT %s edges=%zu payload=0\n", ret ? "return" : "tail", count);
    return true;
}

static bool p1572_report(const Program *p)
{
    const NLSemanticContext *c = p->context;
    const NLTypeId unit = nl_semantic_unit_type(c);
    const NLCheckedNodeId outer = nl_checked_root(p->checked);
    const NLCheckedNodeView *call = nl_checked_node_view(p->checked, outer);
    const NLCheckedFragment *f = nl_checked_call_body(p->checked, outer);
    REQUIRE(f && !observer_error && call &&
            call->kind == NL_CHECKED_REGISTERED_CALL && call->body_backed &&
            !call->terminates && !call->argument_count &&
            !call->first_argument && call->type == unit &&
            !call->result_count && !call->results[0].value && call->function &&
            call->function <= c->function_count &&
            !strcmp(c->functions[call->function - 1].name, "main") &&
            c->functions[call->function - 1].body == f->body_owner);
    NLDivisionUsizePlan plan;
    const bool valid = nl_quantity_usize_call_valid(p->checked, outer, &plan);
    if (valid)
        REQUIRE(plan.body == f && !plan.formal &&
                plan.function == call->function);
    printf("SOURCE PASS\nCHECK PASS\nDEFINITION unknown=%zu\n", unknown_events);
    printf("OUTER result=%d world=%d plan=%d machine=%d calls=%zu\n",
           nl_h_custody_result_valid(p->checked, outer),
           nl_h_custody_result_world_present(p->checked, outer), valid,
           nl_checked_proof_class(p->checked) == NL_CHECKED_MACHINE_PROVED,
           f->body_count);
    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    REQUIRE(root && root->kind == NL_CHECKED_BLOCK);
    const NLCheckedNodeView *ret = NULL;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        REQUIRE(n && (!n->next_item || n->next_item > id));
        if (n->kind == NL_CHECKED_RETURN) {
            ret = n;
            break;
        }
        REQUIRE(n->kind == NL_CHECKED_BINDING);
        const NLCheckedNodeView *init = nl_checked_node_view(f, n->initializer);
        size_t number;
        NLSemanticBindingView binding;
        REQUIRE(init && init->result_count == 1 &&
                nl_semantic_binding_view(c, n->symbol, &binding) &&
                binding.value == init->results[0].value &&
                p1572_scalar(c, binding.value, &number));
        const char *name = c->bindings[n->symbol - 1].name;
        if (init->kind == NL_CHECKED_REGISTERED_CALL)
            REQUIRE(p1572_call(c, f, n->initializer, name));
        else if (operation_node(init))
            REQUIRE(report_direct(c, f, n->initializer, name, binding.value,
                                  number));
        else if (init->kind == NL_CHECKED_IDENTIFIER) {
            NLValueId copied;
            REQUIRE(report_copy(c, f, n->initializer, number, &copied) &&
                    copied == binding.value);
        }
        printf("BIND %s %zu %zu %zu\n", name, n->symbol, number, binding.value);
        id = n->next_item;
    }
    REQUIRE(report_unit_exit(f, root, ret, unit));
    return true;
}

static bool refuses(const NLCheckedFragment *entry)
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

static bool report_excluded(const Program *p)
{
    const NLCheckedFragment *f =
        nl_checked_call_body(p->checked, nl_checked_root(p->checked));
    REQUIRE(f && f->body_count == 1 && refuses(p->checked));
    const NLCheckedNodeId site = f->body_calls[0];
    const NLCheckedNodeView *call = nl_checked_node_view(f, site);
    REQUIRE(call && call->result_count == 1 &&
            !nl_h_custody_result_valid(f, site));
    const NLCheckedNodeView *actual =
        nl_checked_node_view(f, call->first_argument);
    REQUIRE(actual && actual->kind == NL_CHECKED_IDENTIFIER &&
            actual->value_use == NL_VALUE_COPIED && actual->result_count == 1);
    size_t number, actual_value;
    REQUIRE(p1572_scalar(p->context, call->results[0].value, &number) &&
            p1572_scalar(p->context, actual->results[0].value, &actual_value));
    printf("EXCLUDED actual=%zu helper=%zu leaf seal=0 no-clobber=PASS\n",
           actual_value, number);
    return true;
}

static bool check_identity(const Program *p, const Program *other)
{
    const NLCheckedNodeId call = nl_checked_root(p->checked);
    NLDivisionUsizePlan main, foreign;
    REQUIRE(nl_quantity_usize_call_valid(p->checked, call, &main) &&
            nl_quantity_usize_call_valid(
                other->checked, nl_checked_root(other->checked), &foreign) &&
            !main.formal && !main.call && !main.body->body_count &&
            !foreign.formal && !foreign.call && refuses(main.body));
    NLCheckedFragment *body = (NLCheckedFragment *)main.body;
    const NLCheckedNodeView *entry = nl_checked_node_view(body, body->root);
    REQUIRE(entry && entry->terminates &&
            body->function_target != foreign.body->function_target);
    const NLCheckedNodeView *trips =
        nl_checked_node_view(body, entry->first_item);
    REQUIRE(trips);
    const NLCheckedNodeView *carried =
        nl_checked_node_view(body, trips->next_item);
    REQUIRE(carried);
    const NLCheckedNodeView *full =
        nl_checked_node_view(body, carried->next_item);
    REQUIRE(full);
    NLUsizeDivisionView receipt;
    REQUIRE(
        nl_checked_usize_division_view(body, full->initializer, &receipt) &&
        nl_checked_usize_division_valid(body, full->initializer, &receipt) &&
        nl_checked_usize_division_valid(foreign.body, full->initializer,
                                        NULL) &&
        !nl_checked_usize_division_valid(foreign.body, full->initializer,
                                         &receipt) &&
        receipt.symbols[0] == carried->symbol);
    NLSemanticBindingView binding;
    size_t numerator;
    REQUIRE(nl_semantic_binding_view(p->context, trips->symbol, &binding) &&
            p1572_scalar(p->context, binding.value, &numerator) &&
            numerator == receipt.numbers[0] &&
            trips->symbol != receipt.symbols[0]);
    NLUsizeDivisionView wrong_operand = receipt;
    wrong_operand.symbols[0] = trips->symbol;
    REQUIRE(!nl_checked_usize_division_valid(body, full->initializer,
                                             &wrong_operand));

    const NLCheckedNodeView *spill =
        nl_checked_node_view(body, full->next_item);
    REQUIRE(spill);
    const NLCheckedNodeView *saved =
        nl_checked_node_view(body, spill->next_item);
    size_t number;
    REQUIRE(saved &&
            nl_semantic_binding_view(p->context, saved->symbol, &binding) &&
            p1572_scalar(p->context, binding.value, &number) &&
            number == receipt.quotient && binding.value != receipt.result);
    NLUsizeDivisionView wrong_result = receipt;
    wrong_result.result = binding.value;
    REQUIRE(!nl_checked_usize_division_valid(body, full->initializer,
                                             &wrong_result));

    /* Test-only substitutions. Restore each field before the next case. */
    NLCheckedNodeView *operand = &body->nodes[receipt.operands[0] - 1];
    operand->symbol = trips->symbol;
    const bool operand_rejected = refuses(p->checked);
    operand->symbol = receipt.symbols[0];
    NLCheckedNodeView *operation = &body->nodes[full->initializer - 1];
    operation->results[0].value = binding.value;
    const bool result_rejected = refuses(p->checked);
    operation->results[0].value = receipt.result;
    NLControlExits *exits = body->exits;
    body->exits = foreign.body->exits;
    const bool exit_rejected = refuses(p->checked);
    body->exits = exits;

    char *output = NULL;
    size_t length = 0;
    const NLCheckStatus status =
        nl_quantity_usize_c17(p->checked, &output, &length);
    free(output);
    REQUIRE(operand_rejected && result_rejected && exit_rejected &&
            status == NL_CHECK_OK && length);
    puts("IDENTITY body-only, foreign receipt/RETURN and equal-valued "
         "substitutions rejected; restoration PASS");
    return true;
}

static bool p1572_retry_check(Program *p, FaultSite site)
{
    NLSemanticContext *c = p->context;
    const NLTypeId type = nl_semantic_core_type(c, NL_TYPE_USIZE);
    NLSymbolId anchor;
    /* A separate test-only fixture tests rollback of actual prior storage. */
    REQUIRE(nl_semantic_seed_scalar(c, "p1572_anchor",
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
    const NLCheckStatus status = p1572_check(p);
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
            nl_semantic_find_binding(c, "p1572_anchor") == anchor &&
            p1572_scalar(c, value, &checked_value) && checked_value == 73 &&
            nl_sem_validate(c) == NL_CHECK_OK);
    REQUIRE(p1572_check(p) == NL_CHECK_OK && p1572_report(p));
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

static bool p1572_retry_emit(Program *p)
{
    REQUIRE(p1572_check(p) == NL_CHECK_OK);
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
    REQUIRE(held && same && p1572_report(p));
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    const char *mode = argv[1];
    Program program = {0}, foreign = {0};
    bool ok = p1572_prepare(argv[2], &program);
    if (ok && !strcmp(mode, "inspect"))
        ok = p1572_check(&program) == NL_CHECK_OK && p1572_report(&program);
    else if (ok && !strcmp(mode, "excluded"))
        ok = p1572_check(&program) == NL_CHECK_OK && report_excluded(&program);
    else if (ok && !strcmp(mode, "identity"))
        ok = p1572_check(&program) == NL_CHECK_OK &&
             p1572_prepare(argv[2], &foreign) &&
             p1572_check(&foreign) == NL_CHECK_OK &&
             check_identity(&program, &foreign);
    else if (ok) {
        if (!strcmp(mode, "second-leaf-oom"))
            ok = p1572_retry_check(&program, FAULT_SECOND_LEAF);
        else if (!strcmp(mode, "outer-nodes-oom"))
            ok = p1572_retry_check(&program, FAULT_OUTER_NODES);
        else if (!strcmp(mode, "outer-world-oom"))
            ok = p1572_retry_check(&program, FAULT_OUTER_WORLD);
        else if (!strcmp(mode, "staging-oom"))
            ok = p1572_retry_emit(&program);
        else
            ok = false;
        if (ok)
            printf("OOM %s hit=1 no-clobber=PASS retry=PASS\n", mode);
    }
    p1572_destroy(&foreign);
    p1572_destroy(&program);
    return ok ? 0 : 1;
}
