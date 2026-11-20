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
            fprintf(stderr, "P1567 line %d: %s\n", __LINE__, #condition);      \
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

static void p1567_destroy(Program *p)
{
    nl_checked_destroy(p->checked);
    nl_syntax_tree_destroy(p->entry);
    nl_parser_destroy(p->entry_parser);
    nl_source_destroy(p->entry_source);
    nl_semantic_destroy(p->context);
}

static bool p1567_prepare(const char *path, Program *p)
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
    REQUIRE(nl_source_create("main()", 6, "<p1567-entry>", &p->entry_source) ==
            NL_SOURCE_OK);
    REQUIRE(nl_parser_create(p->entry_source, &p->entry_parser) == NL_PARSE_OK);
    REQUIRE(nl_parser_parse_expression_fragment(p->entry_parser, &p->entry,
                                                NULL) == NL_PARSE_OK);
    return true;
}

static NLCheckStatus p1567_check(Program *p)
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

static bool p1567_scalar(const NLSemanticContext *c, NLValueId id, size_t *out)
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
            p1567_scalar(c, from.value, &before) &&
            p1567_scalar(c, use->results[0].value, &after) &&
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

static bool p1567_call(const NLSemanticContext *c, const NLCheckedFragment *f,
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
    REQUIRE(formal && p1567_scalar(c, actual_id, &actual) &&
            p1567_scalar(c, result_id, &result) && result_id == body_result);
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
    const NLCheckedNodeId outer = nl_checked_root(p->checked);
    const NLCheckedFragment *body = nl_checked_call_body(p->checked, outer);
    REQUIRE(body && body->body_count == 1);
    const NLCheckedNodeId site = body->body_calls[0];
    const NLCheckedNodeView *call = nl_checked_node_view(body, site);
    size_t scalar;
    REQUIRE(call && call->result_count == 1 &&
            p1567_scalar(p->context, call->results[0].value, &scalar) &&
            !nl_h_custody_result_valid(body, site) && refuses(p->checked));
    printf("EXCLUDED CHECK value=%zu selected seal=0 no-clobber=PASS\n",
           scalar);
    return true;
}

static bool check_identity(const Program *p, const Program *foreign)
{
    NLDivisionUsizePlan main, leaf, other_main, other_leaf;
    REQUIRE(nl_quantity_usize_call_valid(p->checked,
                                         nl_checked_root(p->checked), &main) &&
            nl_quantity_usize_call_valid(foreign->checked,
                                         nl_checked_root(foreign->checked),
                                         &other_main) &&
            main.body->body_count == 4 && other_main.body->body_count == 4);
    const NLCheckedNodeId site = main.body->body_calls[1];
    REQUIRE(nl_quantity_usize_call_valid(main.body, site, &leaf) &&
            nl_quantity_usize_call_valid(
                other_main.body, other_main.body->body_calls[1], &other_leaf));
    NLUsizeRemainderView issued;
    REQUIRE(
        nl_checked_usize_remainder_view(leaf.body, leaf.division, &issued) &&
        nl_checked_usize_remainder_valid(leaf.body, leaf.division, &issued) &&
        !nl_checked_usize_remainder_valid(other_leaf.body, other_leaf.division,
                                          &issued));

    const NLCheckedNodeView *root =
        nl_checked_node_view(leaf.body, leaf.body->root);
    REQUIRE(root);
    const NLCheckedNodeView *first =
        nl_checked_node_view(leaf.body, root->first_item);
    REQUIRE(first);
    const NLCheckedNodeView *sibling =
        nl_checked_node_view(leaf.body, first->next_item);
    NLSemanticBindingView binding;
    size_t same_number;
    REQUIRE(sibling && sibling->kind == NL_CHECKED_BINDING &&
            nl_semantic_binding_view(p->context, sibling->symbol, &binding) &&
            !strcmp(p->context->bindings[sibling->symbol - 1].name, "other") &&
            sibling->symbol != issued.symbols[0] &&
            binding.value != issued.values[0] &&
            p1567_scalar(p->context, binding.value, &same_number) &&
            same_number == issued.numbers[0]);
    NLUsizeRemainderView substitute = issued;
    substitute.symbols[0] = sibling->symbol;
    REQUIRE(!nl_checked_usize_remainder_valid(leaf.body, leaf.division,
                                              &substitute));

    /* Test-only substitutions in an already checked graph. */
    NLCheckedNodeView *operand = &leaf.body->nodes[issued.operands[0] - 1];
    operand->symbol = sibling->symbol;
    const bool operand_rejected = refuses(p->checked);
    operand->symbol = issued.symbols[0];

    NLCheckedNodeView *call = &main.body->nodes[site - 1];
    const NLCheckedNodeView *repeat =
        nl_checked_node_view(main.body, main.body->body_calls[3]);
    const NLValueId result = call->results[0].value;
    size_t result_number, repeated_number;
    REQUIRE(
        repeat && repeat->function == call->function &&
        repeat->results[0].value != result &&
        p1567_scalar(p->context, result, &result_number) &&
        p1567_scalar(p->context, repeat->results[0].value, &repeated_number) &&
        result_number == repeated_number);
    call->results[0].value = repeat->results[0].value;
    const bool result_rejected = refuses(p->checked);
    call->results[0].value = result;

    char *output = NULL;
    size_t length = 0;
    const NLCheckStatus status =
        nl_quantity_usize_c17(p->checked, &output, &length);
    free(output);
    REQUIRE(operand_rejected && result_rejected && status == NL_CHECK_OK &&
            length);
    puts("IDENTITY equal-valued operand/result and foreign receipt rejected; "
         "restoration PASS");
    return true;
}

static bool p1567_report(const Program *p)
{
    const NLCheckedNodeId outer = nl_checked_root(p->checked);
    const NLCheckedFragment *f = nl_checked_call_body(p->checked, outer);
    REQUIRE(f && unknown_events && !observer_error);
    NLDivisionUsizePlan plan;
    const bool valid = nl_quantity_usize_call_valid(p->checked, outer, &plan);
    if (valid)
        REQUIRE(plan.body == f && !plan.formal);
    printf("SOURCE PASS\nCHECK PASS\nDEFINITION unknown=%zu\n", unknown_events);
    printf("OUTER result=%d world=%d plan=%d machine=%d\n",
           nl_h_custody_result_valid(p->checked, outer),
           nl_h_custody_result_world_present(p->checked, outer), valid,
           nl_checked_proof_class(p->checked) == NL_CHECKED_MACHINE_PROVED);
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
                p1567_scalar(p->context, binding.value, &number));
        const char *name = p->context->bindings[n->symbol - 1].name;
        if (init->kind == NL_CHECKED_REGISTERED_CALL)
            REQUIRE(p1567_call(p->context, f, n->initializer, name));
        if (init->kind == NL_CHECKED_IDENTIFIER) {
            NLValueId copied;
            REQUIRE(
                report_copy(p->context, f, n->initializer, number, &copied) &&
                copied == binding.value);
        }
        printf("BIND %s %zu %zu %zu\n", name, n->symbol, number, binding.value);
        id = n->next_item;
    }
    return true;
}
static bool p1567_retry_check(Program *p, FaultSite site)
{
    NLSemanticContext *c = p->context;
    const NLTypeId type = nl_semantic_core_type(c, NL_TYPE_USIZE);
    NLSymbolId anchor;
    /* A separate test-only fixture tests rollback of actual prior storage. */
    REQUIRE(nl_semantic_seed_scalar(c, "p1567_anchor",
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
    const NLCheckStatus status = p1567_check(p);
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
            nl_semantic_find_binding(c, "p1567_anchor") == anchor &&
            p1567_scalar(c, value, &checked_value) && checked_value == 73 &&
            nl_sem_validate(c) == NL_CHECK_OK);
    REQUIRE(p1567_check(p) == NL_CHECK_OK && p1567_report(p));
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

static bool p1567_retry_emit(Program *p)
{
    REQUIRE(p1567_check(p) == NL_CHECK_OK);
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
    REQUIRE(held && same && p1567_report(p));
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    const char *mode = argv[1];
    Program program = {0}, foreign = {0};
    bool ok = p1567_prepare(argv[2], &program);
    if (ok && !strcmp(mode, "inspect"))
        ok = p1567_check(&program) == NL_CHECK_OK && p1567_report(&program);
    else if (ok && !strcmp(mode, "excluded"))
        ok = p1567_check(&program) == NL_CHECK_OK && report_excluded(&program);
    else if (ok && !strcmp(mode, "identity"))
        ok = p1567_check(&program) == NL_CHECK_OK &&
             p1567_prepare(argv[2], &foreign) &&
             p1567_check(&foreign) == NL_CHECK_OK &&
             check_identity(&program, &foreign);
    else if (ok) {
        if (!strcmp(mode, "second-leaf-oom"))
            ok = p1567_retry_check(&program, FAULT_SECOND_LEAF);
        else if (!strcmp(mode, "outer-nodes-oom"))
            ok = p1567_retry_check(&program, FAULT_OUTER_NODES);
        else if (!strcmp(mode, "outer-world-oom"))
            ok = p1567_retry_check(&program, FAULT_OUTER_WORLD);
        else if (!strcmp(mode, "staging-oom"))
            ok = p1567_retry_emit(&program);
        else
            ok = false;
        if (ok)
            printf("OOM %s hit=1 no-clobber=PASS retry=PASS\n", mode);
    }
    p1567_destroy(&foreign);
    p1567_destroy(&program);
    return ok ? 0 : 1;
}
