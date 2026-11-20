#include "division_usize_internal.h"
#include "h_origin_internal.h"
#include "newlang/parser.h"
#include "newlang/usize_addition.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"
#include "newlang/usize_subtraction.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(condition)                                                     \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "P1579 line %d: %s\n", __LINE__, #condition);      \
            return false;                                                      \
        }                                                                      \
    } while (0)

typedef enum {
    FAULT_NONE,
    FAULT_NESTED_RESULT,
    FAULT_OUTER_NODES,
    FAULT_OUTER_WORLD
} FaultSite;

static FaultSite fault_site;
static size_t allocation, fault_ordinal, hits, seal_visits, unknown_events;
static size_t fault_children;
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
    if (fault_site == FAULT_NESTED_RESULT && !world && child->body_count &&
        ++seal_visits == 1) {
        fault_children = child->body_count;
        fault_ordinal = 1;
    }
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
    const char *callee = NULL;
    for (size_t i = 0; i < c->function_count; ++i)
        if (c->functions[i].body == f->body_owner)
            callee = c->functions[i].name;
    if (callee)
        printf("UNKNOWN %s %c\n", callee,
               n->kind == NL_CHECKED_USIZE_DIVISION ? '/' : '%');
    else
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

static void p1579_destroy(Program *p)
{
    nl_checked_destroy(p->checked);
    nl_syntax_tree_destroy(p->entry);
    nl_parser_destroy(p->entry_parser);
    nl_source_destroy(p->entry_source);
    nl_semantic_destroy(p->context);
}

static bool p1579_prepare(const char *path, Program *p)
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
    REQUIRE(nl_source_create("main()", 6, "<p1579-entry>", &p->entry_source) ==
            NL_SOURCE_OK);
    REQUIRE(nl_parser_create(p->entry_source, &p->entry_parser) == NL_PARSE_OK);
    REQUIRE(nl_parser_parse_expression_fragment(p->entry_parser, &p->entry,
                                                NULL) == NL_PARSE_OK);
    return true;
}

static NLCheckStatus p1579_check(Program *p)
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

static bool p1579_scalar(const NLSemanticContext *c, NLValueId id, size_t *out)
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

static bool arithmetic_kind(NLCheckedKind kind)
{
    return kind == NL_CHECKED_USIZE_DIVISION ||
           kind == NL_CHECKED_USIZE_REMAINDER ||
           kind == NL_CHECKED_USIZE_ADDITION ||
           kind == NL_CHECKED_USIZE_SUBTRACTION;
}

static bool report_event(const NLCheckedFragment *f, NLCheckedNodeId id,
                         const char *function, const NLSemanticContext *c)
{
    const NLCheckedNodeView *node = nl_checked_node_view(f, id);
    const NLCheckedNodeView *left =
        nl_checked_node_view(f, node->first_argument);
    const NLCheckedNodeView *right =
        left ? nl_checked_node_view(f, left->next_argument) : NULL;
    size_t lhs, rhs, number;
    REQUIRE(left && right && left->result_count == 1 &&
            right->result_count == 1 && node->result_count == 1 &&
            p1579_scalar(c, left->results[0].value, &lhs) &&
            p1579_scalar(c, right->results[0].value, &rhs) &&
            p1579_scalar(c, node->results[0].value, &number));
    char op;
    switch (node->kind) {
    case NL_CHECKED_USIZE_DIVISION: {
        NLUsizeDivisionView v;
        REQUIRE(nl_checked_usize_division_view(f, id, &v) &&
                nl_checked_usize_division_valid(f, id, &v) && v.origin == f &&
                v.numerator_known && v.quotient_known && v.numbers[0] == lhs &&
                v.numbers[1] == rhs && v.quotient == number && rhs &&
                number == lhs / rhs);
        op = '/';
        break;
    }
    case NL_CHECKED_USIZE_REMAINDER: {
        NLUsizeRemainderView v;
        REQUIRE(nl_checked_usize_remainder_view(f, id, &v) &&
                nl_checked_usize_remainder_valid(f, id, &v) && v.origin == f &&
                v.source == f->source && v.numerator_known &&
                v.remainder_known && v.numbers[0] == lhs &&
                v.numbers[1] == rhs && v.remainder == number && rhs &&
                number == lhs % rhs);
        op = '%';
        break;
    }
    case NL_CHECKED_USIZE_ADDITION: {
        NLUsizeAdditionView v;
        REQUIRE(nl_checked_usize_addition_view(f, id, &v) &&
                nl_checked_usize_addition_valid(f, id, &v) && v.origin == f &&
                v.numbers[0] == lhs && v.numbers[1] == rhs && v.sum == number &&
                rhs <= SIZE_MAX - lhs && number == lhs + rhs);
        op = '+';
        break;
    }
    case NL_CHECKED_USIZE_SUBTRACTION: {
        NLUsizeSubtractionView v;
        REQUIRE(nl_checked_usize_subtraction_view(f, id, &v) &&
                nl_checked_usize_subtraction_valid(f, id, &v) &&
                v.origin == f && v.numbers[0] == lhs && v.numbers[1] == rhs &&
                v.difference == number && lhs >= rhs && number == lhs - rhs);
        op = '-';
        break;
    }
    default:
        return false;
    }
    printf("EVENT %s %c %zu %zu %zu %zu %zu %zu valid=1\n", function, op, lhs,
           rhs, number, left->results[0].value, right->results[0].value,
           node->results[0].value);
    return true;
}

static bool report_body(const NLCheckedFragment *, const NLSemanticContext *,
                        const char *, bool, size_t);

static bool report_call(const NLCheckedFragment *f, const NLSemanticContext *c,
                        NLCheckedNodeId site, const char *caller, size_t depth)
{
    const NLCheckedNodeView *call = nl_checked_node_view(f, site);
    const NLCheckedFragment *child = nl_checked_call_body(f, site);
    REQUIRE(depth < NL_SEMANTIC_MAX_DEPTH && call && child &&
            call->body_backed && call->function &&
            call->function <= c->function_count && call->argument_count == 1 &&
            call->result_count == 1);
    const NLFunctionEntry *fn = &c->functions[call->function - 1];
    const NLCheckedNodeView *arg =
        nl_checked_node_view(f, call->first_argument);
    const NLCheckedNodeView *root = nl_checked_node_view(child, child->root);
    REQUIRE(fn->body == child->body_owner && fn->body->count == 1 && arg &&
            arg->result_count == 1 && root);
    NLValueId actual_id = arg->results[0].value;
    NLValueId result_id = call->results[0].value;
    REQUIRE(result_id ==
            (root->terminates ? root->returned.value : root->results[0].value));
    NLSymbolId formal = 0;
    for (size_t i = 0; i < c->binding_count; ++i)
        if (c->bindings[i].view.value == actual_id &&
            !strcmp(c->bindings[i].name, fn->body->parameter_names[0])) {
            REQUIRE(!formal);
            formal = i + 1;
        }
    size_t actual, result;
    REQUIRE(formal && p1579_scalar(c, actual_id, &actual) &&
            p1579_scalar(c, result_id, &result));
    if (arg->kind == NL_CHECKED_IDENTIFIER) {
        NLSemanticBindingView source;
        size_t input;
        REQUIRE(arg->value_use == NL_VALUE_COPIED &&
                nl_semantic_binding_view(c, arg->symbol, &source) &&
                source.value < actual_id &&
                p1579_scalar(c, source.value, &input) && input == actual);
    } else {
        REQUIRE(arg->kind == NL_CHECKED_USIZE_LITERAL);
    }
    NLDivisionUsizePlan plan;
    const bool valid = nl_quantity_usize_call_valid(f, site, &plan);
    if (valid)
        REQUIRE(plan.body == child && plan.function == call->function &&
                plan.formal == formal);
    printf("VISIT %s %s %zu %zu %s %zu %zu %zu %zu depth=%zu seal=%d plan=%d\n",
           caller, fn->name, actual, result,
           root->terminates ? "return" : "tail", call->function, actual_id,
           formal, result_id, depth + 1, nl_h_custody_result_valid(f, site),
           valid);
    return report_body(child, c, fn->name, false, depth + 1);
}

static bool report_expression(const NLCheckedFragment *f,
                              const NLSemanticContext *c, NLCheckedNodeId id,
                              const char *function, size_t depth, size_t *calls)
{
    const NLCheckedNodeView *node = nl_checked_node_view(f, id);
    REQUIRE(node);
    if (arithmetic_kind(node->kind))
        return report_event(f, id, function, c);
    if (node->kind == NL_CHECKED_REGISTERED_CALL) {
        REQUIRE(*calls < f->body_count && id == f->body_calls[*calls]);
        ++*calls;
        return report_call(f, c, id, function, depth);
    }
    return true;
}

static bool report_body(const NLCheckedFragment *f, const NLSemanticContext *c,
                        const char *function, bool main, size_t depth)
{
    const NLCheckedNodeView *root = nl_checked_node_view(f, f->root);
    REQUIRE(root && root->kind == NL_CHECKED_BLOCK);
    size_t calls = 0, items = 0;
    const NLCheckedNodeView *ret = NULL;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        REQUIRE(n && (!n->next_item || n->next_item > id));
        ++items;
        if (n->kind == NL_CHECKED_RETURN) {
            REQUIRE(!n->next_item);
            ret = n;
            break;
        }
        REQUIRE(
            n->kind == NL_CHECKED_BINDING &&
            report_expression(f, c, n->initializer, function, depth, &calls));
        NLSemanticBindingView binding;
        const NLCheckedNodeView *init = nl_checked_node_view(f, n->initializer);
        size_t number;
        REQUIRE(init && init->result_count == 1 &&
                nl_semantic_binding_view(c, n->symbol, &binding) &&
                binding.value == init->results[0].value &&
                p1579_scalar(c, binding.value, &number));
        if (init->kind == NL_CHECKED_IDENTIFIER) {
            NLSemanticBindingView source;
            size_t original;
            REQUIRE(init->value_use == NL_VALUE_COPIED &&
                    nl_semantic_binding_view(c, init->symbol, &source) &&
                    source.value < binding.value &&
                    p1579_scalar(c, source.value, &original) &&
                    original == number);
            printf("COPY %s %zu %zu\n", function, source.value, binding.value);
        }
        if (main)
            printf("BIND %s %zu %zu %zu\n", c->bindings[n->symbol - 1].name,
                   n->symbol, number, binding.value);
        id = n->next_item;
    }
    const NLCheckedNodeId exit_id = ret ? ret->initializer : root->tail;
    REQUIRE(items == root->item_count &&
            report_expression(f, c, exit_id, function, depth, &calls) &&
            calls == f->body_count);
    const NLCheckedNodeView *end = nl_checked_node_view(f, exit_id);
    const NLTypeId type = main ? nl_semantic_unit_type(c)
                               : nl_semantic_core_type(c, NL_TYPE_USIZE);
    REQUIRE(end->type == type && end->result_count == (main ? 0u : 1u));
    const size_t edges = nl_control_exits_count(f->exits);
    if (ret) {
        const NLControlExitView *exit = nl_control_exit_view(f->exits, 0);
        REQUIRE(root->terminates && !root->tail &&
                root->returned.type == type && ret->returned.type == type &&
                root->returned.value == end->results[0].value &&
                ret->returned.value == end->results[0].value && edges == 1 &&
                exit && exit->state && exit->count == 1 &&
                exit->values[0].type == type &&
                nl_control_exits_only_return(f->exits, f->function_target));
        if (main)
            REQUIRE(!exit->values[0].value && end->kind == NL_CHECKED_UNIT);
    } else {
        REQUIRE(!edges && !root->terminates && root->type == type &&
                root->result_count == end->result_count &&
                root->results[0].value == end->results[0].value);
    }
    if (main)
        printf("EXIT %s edges=%zu payload=0\n", ret ? "return" : "tail", edges);
    return true;
}

static bool p1579_report(const Program *p)
{
    const NLSemanticContext *c = p->context;
    const NLCheckedNodeId site = nl_checked_root(p->checked);
    const NLCheckedNodeView *n = nl_checked_node_view(p->checked, site);
    const NLCheckedFragment *f = nl_checked_call_body(p->checked, site);
    REQUIRE(f && n && n->kind == NL_CHECKED_REGISTERED_CALL && n->body_backed &&
            !observer_error && n->function &&
            n->function <= c->function_count && !n->argument_count &&
            !n->result_count && n->type == nl_semantic_unit_type(c) &&
            !strcmp(c->functions[n->function - 1].name, "main") &&
            c->functions[n->function - 1].body == f->body_owner);
    NLDivisionUsizePlan plan;
    const bool valid = nl_quantity_usize_call_valid(p->checked, site, &plan);
    if (valid)
        REQUIRE(plan.body == f && !plan.formal && plan.function == n->function);
    printf("SOURCE PASS\nCHECK PASS\nDEFINITION unknown=%zu\n", unknown_events);
    printf("OUTER result=%d world=%d plan=%d machine=%d calls=%zu\n",
           nl_h_custody_result_valid(p->checked, site),
           nl_h_custody_result_world_present(p->checked, site), valid,
           nl_checked_proof_class(p->checked) == NL_CHECKED_MACHINE_PROVED,
           f->body_count);
    return report_body(f, c, "main", true, 0);
}

static bool refuses(const NLCheckedFragment *entry)
{
    NLDivisionUsizePlan plan;
    char sentinel[] = "untouched";
    char *output = sentinel;
    size_t length = 173;
    const bool valid =
        nl_quantity_usize_call_valid(entry, nl_checked_root(entry), &plan);
    const NLCheckStatus status = nl_quantity_usize_c17(entry, &output, &length);
    if (status == NL_CHECK_OK)
        free(output);
    return !valid && status == NL_CHECK_SEMANTIC_UNSUPPORTED &&
           output == sentinel && length == 173 &&
           !strcmp(sentinel, "untouched");
}

static bool same_number(const NLSemanticContext *c, NLValueId a, NLValueId b)
{
    size_t first, second;
    return a != b && p1579_scalar(c, a, &first) &&
           p1579_scalar(c, b, &second) && first == second;
}

static bool check_identity(const Program *p, const Program *other)
{
    NLDivisionUsizePlan main, foreign;
    REQUIRE(nl_quantity_usize_call_valid(p->checked,
                                         nl_checked_root(p->checked), &main) &&
            nl_quantity_usize_call_valid(
                other->checked, nl_checked_root(other->checked), &foreign));
    NLCheckedFragment *root = (NLCheckedFragment *)main.body;
    REQUIRE(root->body_count == 4 && foreign.body->body_count == 4 &&
            root->function_target != foreign.body->function_target &&
            refuses(root));

    /* The second ledger invocation uses definitions already selected by the
     * first one. Its nested actual and result still have their own identities.
     */
    const NLCheckedFragment *ledger =
        nl_checked_call_body(root, root->body_calls[1]);
    const NLCheckedFragment *other_ledger =
        nl_checked_call_body(foreign.body, foreign.body->body_calls[1]);
    REQUIRE(ledger && other_ledger && ledger->body_count == 2 &&
            other_ledger->body_count == 2);
    const NLCheckedNodeId basket_site = ledger->body_calls[1];
    const NLCheckedFragment *basket = nl_checked_call_body(ledger, basket_site);
    const NLCheckedFragment *other_basket =
        nl_checked_call_body(other_ledger, other_ledger->body_calls[1]);
    REQUIRE(basket && other_basket && basket->body_count == 2 &&
            other_basket->body_count == 2);
    const NLCheckedNodeView *first =
        nl_checked_node_view(basket, basket->body_calls[0]);
    const NLCheckedNodeView *peer =
        nl_checked_node_view(basket, basket->body_calls[1]);
    const NLCheckedFragment *parts =
        nl_checked_call_body(basket, basket->body_calls[0]);
    const NLCheckedFragment *foreign_parts =
        nl_checked_call_body(other_basket, other_basket->body_calls[0]);
    REQUIRE(first && peer && parts && foreign_parts);
    const NLCheckedNodeView *parts_root =
        nl_checked_node_view(parts, parts->root);
    REQUIRE(parts_root);
    const NLCheckedNodeView *seen =
        nl_checked_node_view(parts, parts_root->first_item);
    REQUIRE(seen);
    const NLCheckedNodeView *counted =
        nl_checked_node_view(parts, seen->next_item);
    REQUIRE(counted);
    NLUsizeDivisionView issued;
    REQUIRE(
        nl_checked_usize_division_view(parts, counted->initializer, &issued) &&
        nl_checked_usize_division_valid(parts, counted->initializer, &issued) &&
        nl_checked_usize_division_valid(foreign_parts, counted->initializer,
                                        NULL) &&
        !nl_checked_usize_division_valid(foreign_parts, counted->initializer,
                                         &issued));

    NLCheckedNodeView *argument = &basket->nodes[first->first_argument - 1];
    NLSemanticBindingView source;
    REQUIRE(nl_semantic_binding_view(p->context, argument->symbol, &source) &&
            same_number(p->context, source.value, argument->results[0].value));
    NLCheckedNodeView *parent_call = &ledger->nodes[basket_site - 1];
    const NLValueId actual = argument->results[0].value;
    const NLValueId result = parent_call->results[0].value;
    REQUIRE(same_number(p->context, result, peer->results[0].value));

    argument->results[0].value = source.value;
    const bool rejected_actual = refuses(p->checked);
    argument->results[0].value = actual;
    parent_call->results[0].value = peer->results[0].value;
    const bool rejected_result = refuses(p->checked);
    parent_call->results[0].value = result;
    NLControlExits *exits = root->exits;
    root->exits = foreign.body->exits;
    const bool rejected_exit = refuses(p->checked);
    root->exits = exits;

    char *output = NULL;
    size_t length = 0;
    const NLCheckStatus restored =
        nl_quantity_usize_c17(p->checked, &output, &length);
    free(output);
    REQUIRE(rejected_actual && rejected_result && rejected_exit &&
            restored == NL_CHECK_OK && length);
    puts("IDENTITY later repeated nested actual/result, foreign receipt/RETURN "
         "rejected; restoration PASS");
    return true;
}

static bool p1579_retry_check(Program *p, FaultSite site)
{
    NLSemanticContext *c = p->context;
    const NLTypeId type = nl_semantic_core_type(c, NL_TYPE_USIZE);
    NLSymbolId anchor;
    /* A separate test-only fixture tests rollback of actual prior storage. */
    REQUIRE(nl_semantic_seed_scalar(c, "p1579_anchor",
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
    const NLCheckStatus status = p1579_check(p);
    fault_site = FAULT_NONE;
    printf(
        "FAULT status=%d allocation=%zu visits=%zu hits=%zu nested_calls=%zu\n",
        status, allocation, seal_visits, hits, fault_children);
    REQUIRE(status == NL_CHECK_OUT_OF_MEMORY && injected_status == status &&
            hits == 1 && !p->checked && seal_visits == 1 &&
            (site != FAULT_NESTED_RESULT || fault_children == 2) &&
            allocation == (site == FAULT_OUTER_WORLD ? 2u : 1u) &&
            !memcmp(header, c, sizeof header) &&
            !memcmp(binding, &c->bindings[anchor - 1], sizeof binding) &&
            !memcmp(value_bytes, &c->values[value - 1], sizeof value_bytes) &&
            !memcmp(place_bytes, &c->places[place - 1], sizeof place_bytes) &&
            nl_semantic_find_binding(c, "p1579_anchor") == anchor &&
            p1579_scalar(c, value, &checked_value) && checked_value == 73 &&
            nl_sem_validate(c) == NL_CHECK_OK);
    REQUIRE(p1579_check(p) == NL_CHECK_OK && p1579_report(p));
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

static bool p1579_retry_emit(Program *p)
{
    REQUIRE(p1579_check(p) == NL_CHECK_OK);
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
    REQUIRE(held && same && p1579_report(p));
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    const char *mode = argv[1];
    Program program = {0}, other = {0};
    bool ok = p1579_prepare(argv[2], &program);
    if (ok && !strcmp(mode, "inspect"))
        ok = p1579_check(&program) == NL_CHECK_OK && p1579_report(&program);
    else if (ok && !strcmp(mode, "identity"))
        ok = p1579_check(&program) == NL_CHECK_OK &&
             p1579_prepare(argv[2], &other) &&
             p1579_check(&other) == NL_CHECK_OK &&
             check_identity(&program, &other);
    else if (ok) {
        if (!strcmp(mode, "nested-result-oom"))
            ok = p1579_retry_check(&program, FAULT_NESTED_RESULT);
        else if (!strcmp(mode, "outer-nodes-oom"))
            ok = p1579_retry_check(&program, FAULT_OUTER_NODES);
        else if (!strcmp(mode, "outer-world-oom"))
            ok = p1579_retry_check(&program, FAULT_OUTER_WORLD);
        else if (!strcmp(mode, "staging-oom"))
            ok = p1579_retry_emit(&program);
        else
            ok = false;
        if (ok)
            printf("OOM %s hit=1 no-clobber=PASS retry=PASS\n", mode);
    }
    p1579_destroy(&other);
    p1579_destroy(&program);
    return ok ? 0 : 1;
}
