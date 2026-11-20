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
            fprintf(stderr, "P1554 line %d: %s\n", __LINE__, #condition);      \
            return false;                                                      \
        }                                                                      \
    } while (0)

typedef enum {
    NO_FAULT,
    RECEIPT,
    SECOND_RESULT,
    WORLD
} FaultSite;

static FaultSite site;
static bool fail_receipt, observer_error;
static size_t visits, fault_ordinal, allocations, hits, unknown_events;
static NLCheckStatus injected_status;

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
NLCheckStatus __real_nl_usize_arithmetic_capture(NLCheckedFragment *,
                                                 NLCheckedNodeId,
                                                 const NLSyntaxNode *,
                                                 const NLSemanticContext *);
NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);

void *__wrap_malloc(size_t size)
{
    if (fault_ordinal && ++allocations == fault_ordinal && !hits) {
        ++hits;
        return NULL;
    }
    return __real_malloc(size);
}

void *__wrap_calloc(size_t count, size_t size)
{
    if (fail_receipt && !hits) {
        ++hits;
        return NULL;
    }
    return __real_calloc(count, size);
}

NLCheckStatus __wrap_nl_usize_arithmetic_capture(NLCheckedFragment *f,
                                                 NLCheckedNodeId id,
                                                 const NLSyntaxNode *syntax,
                                                 const NLSemanticContext *c)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    const bool remainder = n && n->kind == NL_CHECKED_USIZE_REMAINDER;
    fail_receipt = site == RECEIPT && remainder && n->scalar_result.known;
    if (fail_receipt)
        ++visits;
    const NLCheckStatus status =
        __real_nl_usize_arithmetic_capture(f, id, syntax, c);
    if (fail_receipt)
        injected_status = status;
    fail_receipt = false;
    if (remainder && status == NL_CHECK_OK && !n->scalar_result.known) {
        NLUsizeRemainderView proof;
        NLSemanticValueView numerator, result;
        ++unknown_events;
        if (!nl_checked_usize_remainder_view(f, id, &proof) ||
            proof.numerator_known || proof.remainder_known ||
            proof.numbers[0] || proof.remainder || !proof.numbers[1] ||
            !nl_semantic_value_view(c, proof.values[0], &numerator) ||
            !nl_semantic_value_view(c, proof.result, &result) ||
            numerator.scalar_known || numerator.scalar_value ||
            result.scalar_known || result.scalar_value)
            observer_error = true;
    }
    return status;
}

NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *parent,
                                              NLCheckedFragment *child,
                                              NLCheckedNodeId call,
                                              const NLSemanticContext *world)
{
    if (site == SECOND_RESULT && !world && ++visits == 2)
        fault_ordinal = 1; /* The second helper invocation's node snapshot. */
    if (site == WORLD && world) {
        ++visits;
        fault_ordinal = 2; /* After the outer nodes, committed world bytes. */
    }
    const NLCheckStatus status =
        __real_nl_h_custody_seal_result(parent, child, call, world);
    if (fault_ordinal)
        injected_status = status;
    fault_ordinal = 0;
    return status;
}

typedef struct {
    NLSource *entry_source;
    NLParser *entry_parser;
    NLSyntaxTree *entry;
    NLSemanticContext *context;
    NLCheckedFragment *artifact;
} Program;

static void p1554_destroy(Program *p)
{
    nl_checked_destroy(p->artifact);
    nl_syntax_tree_destroy(p->entry);
    nl_parser_destroy(p->entry_parser);
    nl_source_destroy(p->entry_source);
    nl_semantic_destroy(p->context);
}

static bool p1554_prepare(const char *path, const char *entry, Program *p)
{
    NLSource *input = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    bool parsed =
        nl_source_load(path, &input) == NL_SOURCE_OK &&
        nl_parser_create(input, &parser) == NL_PARSE_OK &&
        nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK;
    bool registered = false;
    if (parsed && nl_semantic_create(&p->context) == NL_CHECK_OK) {
        const NLSyntaxTree *units[] = {unit};
        registered = nl_semantic_register_function_unit(p->context, units, 1,
                                                        NULL) == NL_CHECK_OK;
    }
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(input);
    REQUIRE(parsed && registered);
    REQUIRE(nl_source_create(entry, strlen(entry), "<p1554-entry>",
                             &p->entry_source) == NL_SOURCE_OK);
    REQUIRE(nl_parser_create(p->entry_source, &p->entry_parser) == NL_PARSE_OK);
    REQUIRE(nl_parser_parse_expression_fragment(p->entry_parser, &p->entry,
                                                NULL) == NL_PARSE_OK);
    return true;
}

static NLCheckStatus p1554_check(Program *p)
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

static bool p1554_scalar(const NLSemanticContext *c, NLValueId id, size_t *out)
{
    NLSemanticValueView v;
    if (!nl_semantic_value_view(c, id, &v) ||
        v.type != nl_semantic_core_type(c, NL_TYPE_USIZE) || !v.scalar_known ||
        v.dependencies != NL_DEPENDENCY_FREE || v.value_dependency_count ||
        v.domain || v.reference.place || v.reference_count ||
        v.allocation_region)
        return false;
    *out = v.scalar_value;
    return true;
}

static bool p1554_plans(const Program *p, NLDivisionUsizePlan *main,
                        NLDivisionUsizePlan *helper)
{
    return nl_remainder_usize_call_valid(p->artifact,
                                         nl_checked_root(p->artifact), main) &&
           nl_remainder_usize_call_valid(main->body, main->call, helper);
}

static bool p1554_call_report(const NLCheckedFragment *parent,
                              NLCheckedNodeId id,
                              const NLSemanticContext *context)
{
    NLDivisionUsizePlan plan;
    REQUIRE(nl_remainder_usize_call_valid(parent, id, &plan));
    NLUsizeRemainderView proof;
    REQUIRE(nl_checked_usize_remainder_view(plan.body, plan.division, &proof));
    REQUIRE(nl_checked_usize_remainder_valid(plan.body, plan.division, &proof));

    NLSourceView op;
    REQUIRE(proof.origin == plan.body &&
            proof.source == nl_checked_source(plan.body) &&
            nl_source_view(proof.source, proof.operator_span, &op) &&
            op.length == 1 && op.bytes[0] == '%' &&
            proof.target_max == SIZE_MAX);
    REQUIRE(proof.numerator_known && proof.remainder_known &&
            proof.numbers[1] && proof.remainder < proof.numbers[1] &&
            proof.remainder == proof.numbers[0] % proof.numbers[1] &&
            proof.result != proof.values[0] && proof.result != proof.values[1]);

    const NLCheckedNodeView *call = nl_checked_node_view(parent, id);
    const NLCheckedNodeView *arg =
        nl_checked_node_view(parent, call->first_argument);
    const NLCheckedNodeView *root =
        nl_checked_node_view(plan.body, plan.body->root);
    NLSemanticBindingView formal;
    REQUIRE(arg && arg->result_count == 1 && root &&
            proof.symbols[0] == plan.formal &&
            nl_semantic_binding_view(context, plan.formal, &formal) &&
            formal.place == proof.places[0] &&
            formal.value == arg->results[0].value);
    const NLValueId returned =
        root->terminates ? root->returned.value : root->results[0].value;
    size_t actual_number, returned_number;
    REQUIRE(call->results[0].value == returned &&
            ((plan.division == plan.result) == (proof.result == returned)) &&
            p1554_scalar(context, formal.value, &actual_number) &&
            p1554_scalar(context, returned, &returned_number) &&
            actual_number == proof.numbers[0] &&
            returned_number == proof.remainder);
    printf("CALL %zu %zu %zu %s %zu %zu %zu %zu seal=1\n", actual_number,
           proof.numbers[1], proof.remainder,
           root->terminates ? "return" : "tail", formal.value, plan.formal,
           proof.result, returned);
    return true;
}

static bool p1554_report(const Program *p)
{
    REQUIRE(unknown_events && !observer_error);
    NLDivisionUsizePlan main, helper;
    REQUIRE(p1554_plans(p, &main, &helper));
    printf("SOURCE PASS\nCHECK PASS\nDEFINITION unknown=%zu\n", unknown_events);
    const NLCheckedNodeView *root =
        nl_checked_node_view(main.body, main.body->root);
    REQUIRE(root && root->kind == NL_CHECKED_BLOCK);
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *binding = nl_checked_node_view(main.body, id);
        REQUIRE(binding && binding->kind == NL_CHECKED_BINDING);
        const NLCheckedNodeView *init =
            nl_checked_node_view(main.body, binding->initializer);
        NLSemanticBindingView name;
        size_t value;
        REQUIRE(init && init->result_count == 1 &&
                nl_semantic_binding_view(p->context, binding->symbol, &name) &&
                name.value == init->results[0].value &&
                p1554_scalar(p->context, name.value, &value));
        if (init->kind == NL_CHECKED_REGISTERED_CALL)
            REQUIRE(
                p1554_call_report(main.body, binding->initializer, p->context));
        printf("BIND %s %zu %zu %zu\n",
               p->context->bindings[binding->symbol - 1].name, binding->symbol,
               value, name.value);
        id = binding->next_item;
    }
    REQUIRE(nl_h_custody_result_world_present(p->artifact,
                                              nl_checked_root(p->artifact)));
    puts("WORLD seal=1 bytes=1");
    return true;
}

static bool p1554_rejects(const NLCheckedFragment *entry)
{
    char sentinel[] = "existing output";
    char *text = sentinel;
    size_t size = 137;
    const NLCheckStatus status = nl_remainder_usize_c17(entry, &text, &size);
    if (status == NL_CHECK_OK)
        free(text);
    return status == NL_CHECK_SEMANTIC_UNSUPPORTED && text == sentinel &&
           size == 137 && !strcmp(sentinel, "existing output");
}

static bool p1554_authenticate(Program *p, Program *foreign)
{
    REQUIRE(p1554_check(p) == NL_CHECK_OK &&
            p1554_check(foreign) == NL_CHECK_OK);
    NLDivisionUsizePlan main, helper, foreign_main, foreign_helper, second;
    REQUIRE(p1554_plans(p, &main, &helper) &&
            p1554_plans(foreign, &foreign_main, &foreign_helper) &&
            main.body->body_count == 3 &&
            nl_remainder_usize_call_valid(main.body, main.body->body_calls[1],
                                          &second));
    NLUsizeRemainderView genuine, equal;
    REQUIRE(
        nl_checked_usize_remainder_view(helper.body, helper.division,
                                        &genuine) &&
        nl_checked_usize_remainder_view(second.body, second.division, &equal) &&
        nl_checked_usize_remainder_valid(helper.body, helper.division,
                                         &genuine));
    REQUIRE(genuine.remainder == equal.remainder &&
            genuine.result != equal.result);
    REQUIRE(!nl_checked_usize_remainder_valid(
                foreign_helper.body, foreign_helper.division, &genuine) &&
            nl_checked_usize_remainder_valid(foreign_helper.body,
                                             foreign_helper.division, NULL));
    NLUsizeRemainderView wrong = genuine;
    wrong.source = nl_checked_source(foreign_helper.body);
    REQUIRE(wrong.source != genuine.source &&
            !nl_checked_usize_remainder_valid(helper.body, helper.division,
                                              &wrong));
    wrong = genuine;
    wrong.result = equal.result;
    REQUIRE(!nl_checked_usize_remainder_valid(helper.body, helper.division,
                                              &wrong));
    wrong = genuine;
    wrong.operator_span = genuine.operand_spans[0];
    REQUIRE(!nl_checked_usize_remainder_valid(helper.body, helper.division,
                                              &wrong));
    wrong = genuine;
    wrong.remainder_known = false;
    REQUIRE(!nl_checked_usize_remainder_valid(helper.body, helper.division,
                                              &wrong));

    NLCheckedNodeView *first_call = &main.body->nodes[main.call - 1];
    const NLValueId saved = first_call->results[0].value;
    const NLValueId second_result =
        main.body->nodes[main.body->body_calls[1] - 1].results[0].value;
    size_t first_value, second_value;
    REQUIRE(saved != second_result &&
            p1554_scalar(p->context, saved, &first_value) &&
            p1554_scalar(p->context, second_result, &second_value) &&
            first_value == second_value);
    first_call->results[0].value = second_result;
    const bool rejected = p1554_rejects(p->artifact);
    first_call->results[0].value = saved;
    REQUIRE(rejected && p1554_plans(p, &main, &helper));
    puts("CUSTODY wrong-source foreign wrong-span wrong-known "
         "equal-value-result rejected");
    return true;
}

static bool p1554_wrong_operation(Program *zero, Program *division)
{
    REQUIRE(p1554_check(zero) == NL_CHECK_OK &&
            p1554_check(division) == NL_CHECK_OK);
    NLDivisionUsizePlan zero_main, rem, div_main, div;
    REQUIRE(p1554_plans(zero, &zero_main, &rem) &&
            nl_division_usize_call_valid(division->artifact,
                                         nl_checked_root(division->artifact),
                                         &div_main) &&
            nl_division_usize_call_valid(div_main.body, div_main.call, &div));
    NLUsizeRemainderView residue;
    NLUsizeDivisionView quotient;
    REQUIRE(
        nl_checked_usize_remainder_view(rem.body, rem.division, &residue) &&
        nl_checked_usize_division_view(div.body, div.division, &quotient) &&
        nl_checked_usize_division_valid(div.body, div.division, &quotient) &&
        residue.remainder_known && quotient.quotient_known &&
        residue.remainder == 0 && quotient.quotient == residue.remainder);
    unsigned char before[sizeof residue];
    memcpy(before, &residue, sizeof before);
    REQUIRE(
        !nl_checked_usize_remainder_view(div.body, div.division, &residue) &&
        !memcmp(before, &residue, sizeof before) &&
        !nl_checked_usize_remainder_valid(div.body, div.division, NULL) &&
        !nl_checked_usize_division_valid(rem.body, rem.division, NULL) &&
        p1554_rejects(division->artifact));
    puts("OPERATION known-zero division and remainder are disjoint");
    return true;
}

static bool p1554_unknown_call(const char *path)
{
    Program p = {0};
    bool prepared = p1554_prepare(path, "unfinished_page(unknown_amount)", &p);
    if (prepared) {
        const NLTypeId type = nl_semantic_core_type(p.context, NL_TYPE_USIZE);
        NLSymbolId symbol;
        prepared = nl_semantic_seed_value(p.context, "unknown_amount", type,
                                          NL_DEPENDENCY_FREE,
                                          &symbol) == NL_CHECK_OK &&
                   p1554_check(&p) == NL_CHECK_OK;
    }
    const NLCheckedNodeId call = nl_checked_root(p.artifact);
    NLDivisionUsizePlan helper;
    NLUsizeRemainderView genuine;
    bool valid = prepared &&
                 nl_remainder_usize_call_valid(p.artifact, call, &helper) &&
                 nl_checked_usize_remainder_view(helper.body, helper.division,
                                                 &genuine) &&
                 nl_checked_usize_remainder_valid(helper.body, helper.division,
                                                  &genuine) &&
                 nl_h_custody_result_world_present(p.artifact, call) &&
                 !genuine.numerator_known && !genuine.remainder_known &&
                 genuine.numbers[0] == 0 && genuine.remainder == 0;
    if (valid) {
        genuine.numerator_known = genuine.remainder_known = true;
        valid = !nl_checked_usize_remainder_valid(helper.body, helper.division,
                                                  &genuine);
    }
    p1554_destroy(&p);
    REQUIRE(valid && !observer_error);
    puts("UNKNOWN retained caller evidence rejects forged Known zero");
    return true;
}

static bool p1554_check_oom(Program *p, FaultSite target)
{
    unsigned char before[sizeof *p->context];
    memcpy(before, p->context, sizeof before);
    REQUIRE(!p->artifact);
    site = target;
    const NLCheckStatus status = p1554_check(p);
    site = NO_FAULT;
    printf("FAULT status=%d visits=%zu allocations=%zu hits=%zu\n", status,
           visits, allocations, hits);
    REQUIRE(status == NL_CHECK_OUT_OF_MEMORY && injected_status == status &&
            hits == 1 && visits == (target == SECOND_RESULT ? 2u : 1u) &&
            allocations == (target == WORLD           ? 2u
                            : target == SECOND_RESULT ? 1u
                                                      : 0u) &&
            !p->artifact && !memcmp(before, p->context, sizeof before) &&
            nl_sem_validate(p->context) == NL_CHECK_OK);
    REQUIRE(p1554_check(p) == NL_CHECK_OK && p1554_report(p));
    char *text = NULL;
    size_t length = 0;
    const NLCheckStatus retry =
        nl_remainder_usize_c17(p->artifact, &text, &length);
    const bool emitted =
        retry == NL_CHECK_OK && text && length && strstr(text, " % ");
    free(text);
    REQUIRE(emitted);
    return true;
}

static bool p1554_stage_oom(Program *p)
{
    REQUIRE(p1554_check(p) == NL_CHECK_OK);
    char *baseline = NULL;
    size_t baseline_size = 0;
    REQUIRE(nl_remainder_usize_c17(p->artifact, &baseline, &baseline_size) ==
            NL_CHECK_OK);
    char sentinel[] = "existing output";
    char *text = sentinel;
    size_t length = 137;
    fault_ordinal = 1;
    const NLCheckStatus status =
        nl_remainder_usize_c17(p->artifact, &text, &length);
    fault_ordinal = 0;
    printf("FAULT status=%d allocations=%zu hits=%zu\n", status, allocations,
           hits);
    const bool held = status == NL_CHECK_OUT_OF_MEMORY && hits == 1 &&
                      allocations == 1 && text == sentinel && length == 137 &&
                      !strcmp(sentinel, "existing output");
    if (status == NL_CHECK_OK)
        free(text);
    text = NULL;
    length = 0;
    const NLCheckStatus retry =
        nl_remainder_usize_c17(p->artifact, &text, &length);
    const bool same = retry == NL_CHECK_OK && length == baseline_size &&
                      !memcmp(text, baseline, length);
    free(baseline);
    free(text);
    REQUIRE(held && same && p1554_report(p));
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 5)
        return 2;
    const char *mode = argv[1];
    Program program = {0}, foreign = {0}, zero = {0}, division = {0};
    bool ok = p1554_prepare(argv[2], "main()", &program);
    if (ok && !strcmp(mode, "inspect"))
        ok = p1554_check(&program) == NL_CHECK_OK && p1554_report(&program);
    else if (ok && argc == 5 && !strcmp(mode, "authenticity"))
        ok = p1554_prepare(argv[2], "main()", &foreign) &&
             p1554_authenticate(&program, &foreign) &&
             p1554_prepare(argv[3], "main()", &zero) &&
             p1554_prepare(argv[4], "main()", &division) &&
             p1554_wrong_operation(&zero, &division) &&
             p1554_unknown_call(argv[3]);
    else if (ok) {
        if (!strcmp(mode, "receipt-oom"))
            ok = p1554_check_oom(&program, RECEIPT);
        else if (!strcmp(mode, "result-oom"))
            ok = p1554_check_oom(&program, SECOND_RESULT);
        else if (!strcmp(mode, "world-oom"))
            ok = p1554_check_oom(&program, WORLD);
        else if (!strcmp(mode, "staging-oom"))
            ok = p1554_stage_oom(&program);
        else
            ok = false;
        if (ok)
            printf("OOM %s hit=1 no-clobber=PASS retry=PASS\n", mode);
    }
    p1554_destroy(&division);
    p1554_destroy(&zero);
    p1554_destroy(&foreign);
    p1554_destroy(&program);
    return ok ? 0 : 1;
}
